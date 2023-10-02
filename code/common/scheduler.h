#pragma once

#include <memory>
#include <list>
#include <vector>
#include <mutex>
#include <atomic>


#include "fiber.h"

namespace myriel {
/**
 * @brief 协程调度器
 * @details 封装的是N-M的调度器
 * 			内部存在一个线程池，支持协程在线程池里切换
*/
class Scheduler {
public:
	using ptr = std::shared_ptr<Scheduler>;

	/**
	 * @brief 创建调度器
	 * 
	 * @param threads 线程数
	 * @param use_caller 是否将当前线程也作为调度线程
	 * @param name 调度器名称
	 */
	Scheduler(size_t threads = 1, bool use_caller = true, const::std::string &name = "Scheduler");

	/**
	 * @brief 析构函数，销毁调度器
	 */
	virtual ~Scheduler();

	/**
	 * @brief 获取调度器名称
	 * 
	 * @return const std::string& 调度器名称
	 */
	const std::string &getName() const { return m_name; }

	/**
	 * @brief 静态函数，获取当前调度器的指针
	 * 
	 * @return Scheduler* 调度器指针
	 */
	static Scheduler *GetThis();

	/**
	 * @brief 静态函数，获取主协程
	 * 
	 * @return Fiber* 主协程指针
	 */
	static Fiber *GetMainFiber();

	/**
	 * @brief 启动调度器，并绑定协程调度函数
	 */
	void start();

	/**
	 * @brief 切换至协程调度函数运行，等所有调度任务都执行完则结束协程调度器并返回
	 */
	void stop();

	/**
	 * @brief 添加调度任务
	 * 
	 * @tparam FiberOrCb 调度任务类型，可以是协程对象或函数指针
	 * @param fc 协程对象或指针
	 * @param thread 指定运行该任务的线程号，
	 */
	template<class FiberOrCb>
	void schedule(FiberOrCb fc, std::thread::id thread = std::thread::id()) {
		bool need_tickle = false; {
			std::lock_guard<std::mutex> locker(m_mutex);
			need_tickle = scheduleNoLock(fc, thread);
		}

		if(need_tickle) {
			tickle();
		}
	}

	template<class InputIterator>
	void schedule(InputIterator begin, InputIterator end)
	{
		bool need_tickle = false; {
			std::lock_guard<std::mutex> locker(m_mutex);
			while(begin != end) {
				need_tickle = scheduleNoLock(&*begin, std::thread::id()) || need_tickle;
				++begin;
			}
		}
		if(need_tickle) {
			tickle();
		}
	}

protected:

	/**
	 * @brief 通知协程调度器有任务了
	 */
	virtual void tickle();

	/**
	 * @brief 协程调度函数
	 */
	void run();

	virtual bool stopping();

	/**
	 * @brief 无任务调度时执行idle协程
	 */
	virtual void idle();

	/**
	 * @brief 设置当前的协程调度器
	 */
	void setThis();

	bool hasIdleThread() { return m_idleThreadCount > 0;}
private:
	/**
	 * @brief 添加调度任务，无锁
	 * 
	 * @tparam FiberOrCb 调度任务类型，可以是协程对象或函数指针
	 * @param fc 协程对象或指针
	 * @param thread 指定运行该任务的线程号，
	 */
	template<class FiberOrCb>
	bool scheduleNoLock(FiberOrCb fc, std::thread::id thread) {
		bool need_tickle = m_tasks.empty();
		SchedulerTask task(fc, thread);
		if(task.fiber || task.cb) {
			m_tasks.push_back(task);
		}
		return need_tickle;
	}

private:
	/**
	 * @brief 调度任务，协程/函数二选一
	 * 
	 */
	struct SchedulerTask
	{
		Fiber::ptr fiber;						// 协程
		std::function<void()> cb;				// 任务
		std::thread::id thread;					// 指定线程ID

		/**
		 * @brief 构造函数
		 * @attention 直接传递智能指针对象，对象在栈上，栈上的局部变量只要执行到花括号就会释放
		 * @param f 
		 * @param thrId 
		 */
		SchedulerTask(Fiber::ptr f, std::thread::id thr)
			: fiber(f), thread(thr) {}

		/**
		 * @brief 构造函数
		 * @attention 传递智能指针的指针，智能指针对象本身在堆区，不会自动析构
		 * @param f 
		 * @param thrId 
		 */
		SchedulerTask(Fiber::ptr *f, std::thread::id thr)
			: thread(thr) { fiber.swap(*f); }

		SchedulerTask(std::function<void()> f, std::thread::id thr)
			: cb(f), thread(thr) {  }

		SchedulerTask(std::function<void()> *f, std::thread::id thr)
			: thread(thr) { cb.swap(*f); }

		SchedulerTask()
			: thread(-1) {}
		
		/**
		 * @brief 将任务置为空
		 */
		void reset() {
			fiber = nullptr;
			cb = nullptr;
			thread = std::thread::id();
		}
	};

private: 
	std::mutex m_mutex;									// 锁
	std::vector<std::thread> m_threads;					// 线程池
	std::list<SchedulerTask> m_tasks;					// 协程任务队列
	std::string m_name;									// 调度器名称
	Fiber::ptr m_rootFiber;								// 调度器所在线程的调度协程
	bool m_useCaller;

protected:
	std::vector<std::thread::id> m_threadIds;			// 线程ID数组
	size_t m_threadCount = 0;							// 工作线程数
	std::atomic<size_t> m_activeThreadCount = {0};		// 活跃线程数
	std::atomic<size_t> m_idleThreadCount = {0};		// idle线程数
	bool m_stopping = false;							// 是否正在停止
	std::thread::id m_rootThread;						// 主线程id
};
}