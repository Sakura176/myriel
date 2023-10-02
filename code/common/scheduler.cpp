#include "scheduler.h"
#include "log.h"
#include "utils.h"

#include <cassert>

namespace myriel {
static Logger::ptr g_logger = LOG_ROOT();

// 当前线程的调度器，同一个调度器下的所有线程指向同一个调度器实例
static thread_local Scheduler *t_scheduler = nullptr;
// 当前线程的调度协程，每个线程都独有一份，包括caller线程
static thread_local Fiber *t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const::std::string &name) : m_name(name) {
	assert(threads > 0);
	m_useCaller = use_caller;

	if (use_caller) {
		// 创建协程
		Fiber::GetThis();
		--threads;

		assert(GetThis() == nullptr);
		t_scheduler = this;

		/**
		 * 在user_caller为true的情况下，初始化caller线程的调度协程
		 * caller线程的调度协程不会被调度器调度，而且，caller线程的调度协程停止时，应该返回caller线程的主协程
		 */
		m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));
		// Thread::SetName(m_name);

		// 将调度协程赋值给局部变量
		t_scheduler_fiber = m_rootFiber.get();
		m_rootThread = std::this_thread::get_id();
		m_threadIds.emplace_back(m_rootThread);
	} else {
		m_rootThread = std::thread::id();
	}
	m_threadCount = threads;
}

Scheduler::~Scheduler() {
	assert(m_stopping);
	if(GetThis() == this) {
		t_scheduler = nullptr;
	}
}

Scheduler* Scheduler::GetThis() {
	return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
	return t_scheduler_fiber;
}

void Scheduler::start() {
	std::lock_guard<std::mutex> locker(m_mutex);
	if(!m_stopping) {
		return;
	}

	assert(m_threads.empty());

	m_threads.resize(m_threadCount);
	for (size_t i = 0; i < m_threadCount; ++i) {
		m_threads[i] = std::move(std::thread(std::bind(&Scheduler::run, this)));
		m_threadIds.push_back(m_threads[i].get_id());
	}
}

void Scheduler::stop() {
	// LOG_DEBUG(g_logger) << "Scheduler::stop()";
	if(stopping()) {
		// LOG_DEBUG(g_logger) << "Scheduler::m_useCaller";
		return;
	}
	m_stopping = true;

	if(m_useCaller) {
		assert(GetThis() == this);
	} else {
		assert(GetThis() != this);
	}
	// 根据线程数调用tickle()
	for (size_t i = 0; i < m_threadCount; ++i) {
		tickle();
	}

	if(m_rootFiber) {
		// SERVER_LOG_DEBUG(g_logger) << "scheduler::m_rootFiber";
		tickle();
	}

	if(m_rootFiber) {
		// LOG_DEBUG(g_logger) << "m_rootFiber resume";
		m_rootFiber->resume();
		// LOG_DEBUG(g_logger) << "m_rootFiber end";
	}
	// LOG_DEBUG(g_logger) << "Scheduler::m_rootFiber";
	std::vector<std::thread> thrs;
	{
        std::lock_guard<std::mutex> locker(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs) {
        i.join();
    }
}

void Scheduler::setThis() {
	t_scheduler = this;
}

/**
 * @details 1. 设置当前线程的shceduler
 * 2. 设置当前线程的run，fiber
 * 3. 协程调度循环while(true)
 * 		1. 协程消息队列里面是否有任务
 * 		2. 无任务执行，执行idle
 */
void Scheduler::run() {
	// SERVER_LOG_DEBUG(g_logger) << m_name << " Scheduler::run()";
	// server::Fiber::EnableFiber();
	// set_hook_enable(true);
	setThis();
	if(std::this_thread::get_id() != m_rootThread) {
		t_scheduler_fiber = Fiber::GetThis().get();
	}

	Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
	Fiber::ptr cb_fiber;

	SchedulerTask task;
	while(true) {
		task.reset();
		bool tickle_me = false;		// 是否tickle其它线程进行任务调度
		{
			// LOG_INFO(g_logger) << "begin while";
			std::lock_guard<std::mutex> locker(m_mutex);
			auto it = m_tasks.begin();
			// 遍历所有调度任务
			while(it != m_tasks.end()) {
				// 任务指定了线程，且不在当前线程
				if(it->thread != std::thread::id() && it->thread != std::this_thread::get_id()) {
					// 指定了调度线程，但不是在当前线程上调度，标记一下需要通知其它线程进行调度
					// 然后跳过这个任务，继续下一个
					// LOG_INFO(g_logger) << "tickle other thread";
					++it;
					tickle_me = true;
					continue;
				}

				// 任务未指定线程或者指定了当前线程
				assert(it->fiber || it->cb);
				if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
					// LOG_INFO(g_logger) << "current or no thread m_task size: " << m_tasks.size();
					++it;
					continue;
				}

				// 取出该任务
				task = *it;
				m_tasks.erase(it++);
				// LOG_DEBUG(g_logger) << "m_task.size() after erase: " << m_tasks.size();
				// 增加活跃线程数
				++m_activeThreadCount;
				break;
			}
			tickle_me |= (it != m_tasks.end());
			// LOG_DEBUG(g_logger) << "m_task.empty(): " << m_tasks.empty();
		}

		// 通知其他线程，有任务了
		if(tickle_me) {
			tickle();
		}

		// 该任务协程存在且协程状态不为结束和异常
		if(task.fiber) {			
			// 保存调度协程上下文，并切换到当前协程ft的上下文
			task.fiber->resume();
			--m_activeThreadCount;
			task.reset();
		} else if(task.cb) {
			// 若为任务函数，则初始化任务协程
			if(cb_fiber) {
				cb_fiber->reset(task.cb);
			} else {
				cb_fiber.reset(new Fiber(task.cb));
			}
			task.reset();
			cb_fiber->resume();
			--m_activeThreadCount;
			cb_fiber.reset();
		} else {
			// 若都不是，即没有任务，则运行idle协程
			if(idle_fiber->getState() == Fiber::TERM) {
				// SERVER_LOG_INFO(g_logger) << "idle fiber term";
				break;
			}

			++m_idleThreadCount;
			// LOG_DEBUG(g_logger) << "Scheduler::idle_fiber resume";
			idle_fiber->resume();		// 这里直接析构，程序结束了
			// LOG_DEBUG(g_logger) << "Scheduler::idle_fiber resume after";
			--m_idleThreadCount;
		}
	}
	// LOG_DEBUG(g_logger) << "Scheduler::run() exit";
}

void Scheduler::tickle()
{
	// LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
	std::lock_guard<std::mutex> locker(m_mutex);
	return m_stopping && m_tasks.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
	// LOG_INFO(g_logger) << "idle";
	while(!stopping()) {
		Fiber::GetThis()->yield();
	}
	// LOG_DEBUG(g_logger) << "stopping!";
}
}