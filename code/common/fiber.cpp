#include <cassert>

#include "scheduler.h"
#include "fiber.h"
#include "log.h"
#include "utils.h"

namespace myriel {

// 全局静态变量，用于生成协程id
static std::atomic<uint64_t> s_fiber_id{0};
// 全局静态变量，用于统计当前的协程数
static std::atomic<uint64_t> s_fiber_count{0};

// 线程局部变量，当前运行的协程
static thread_local Fiber *t_fiber = nullptr;
// 线程局部变量，当前线程的主协程
static thread_local Fiber::ptr t_thread_fiber = nullptr;

static uint32_t g_fiber_stack_size = 128 * 1024;

static Logger::ptr g_logger = LOG_ROOT();

class MallocStackAllocator {
public:
	/**
	 * @brief 静态函数，申请内存空间
	 * 
	 * @param size 申请空间大小
	 */
	static void *Alloc(size_t size) {
		return malloc(size);
	}

	/**
	 * @brief 回收内存空间
	 * 
	 * @param vp 回收内存空间指针
	 * @param size 回收内存空间大小
	 */
	static void Dealloc(void* vp, size_t size) {
		return free(vp);
	}
};

using StackAllocator = MallocStackAllocator;

Fiber::Fiber() {
	SetThis(this);
	m_state = EXEC;

	if(getcontext(&m_ctx)) {
		ASSERT2(false, "system error: getcontext failed!");
	}
	++s_fiber_count;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler) 
: m_id(++s_fiber_id), m_cb(cb), m_scheduler(run_in_scheduler) {
	
	++s_fiber_count;
	m_stacksize = stacksize ? stacksize : g_fiber_stack_size;

	m_stack = StackAllocator::Alloc(m_stacksize);
	if(getcontext(&m_ctx)) {
		ASSERT2(false, "system error: getcontext failed!");
	}
	m_ctx.uc_link = nullptr;
	m_ctx.uc_stack.ss_sp = m_stack;
	m_ctx.uc_stack.ss_size = m_stacksize;

	makecontext(&m_ctx, &Fiber::MainFunc, 0);
}

Fiber::~Fiber() {
	--s_fiber_count;
	if(m_stack) {
		assert(m_state == TERM);
		StackAllocator::Dealloc(m_stack, m_stacksize);
	} else {
		assert(!m_cb);
		assert(m_state == EXEC);
		Fiber *cur = t_fiber;
		if (cur == this) {
			SetThis(nullptr);
		}
	}
}

void Fiber::reset(std::function<void()> cb) {
	assert(m_stack);
	assert(m_state == TERM);

	m_cb = cb;
	if(getcontext(&m_ctx)) {
		ASSERT2(false, "system error: getcontext failed!");
	}

	m_ctx.uc_link = nullptr;
	m_ctx.uc_stack.ss_sp = m_stack;
	m_ctx.uc_stack.ss_size = m_stacksize;

	makecontext(&m_ctx, &Fiber::MainFunc, 0);
	m_state = READY;
}

void Fiber::resume() {
	assert(m_state != TERM && m_state != EXEC);
	SetThis(this);
	m_state = EXEC;
	if(m_scheduler) {
		if (swapcontext(&(Scheduler::GetMainFiber()->m_ctx), &m_ctx)) {
			ASSERT2(false, "system error: swapcontext failed!");
		}
	} else {
		if (swapcontext(&(t_thread_fiber->m_ctx), &m_ctx)) {
			ASSERT2(false, "system error: swapcontext failed!");
		}
	}
}

void Fiber::yield() {
	assert(m_state == EXEC || m_state == TERM);
	SetThis(t_thread_fiber.get());

	if(m_state != TERM) {
		m_state = READY;
	}

	if(m_scheduler) {
		if(swapcontext(&m_ctx, &(Scheduler::GetMainFiber()->m_ctx))) {
			ASSERT2(false, "system error: swapcontext failed!");
		}
	} else {
		if(swapcontext(&m_ctx, &(t_thread_fiber->m_ctx))) {
			ASSERT2(false, "system error: swapcontext failed!");
		}
	}
}

void Fiber::SetThis(Fiber *f) {
	t_fiber = f;
}

Fiber::ptr Fiber::GetThis() {
	if(t_fiber) {
		return t_fiber->shared_from_this();
	}

	Fiber::ptr main_fiber(new Fiber);
	assert(t_fiber == main_fiber.get());
	t_thread_fiber = main_fiber;
	return t_fiber->shared_from_this();
}

void Fiber::MainFunc() {
	// 指针未释放，导致存在协程未销毁的问题
	Fiber::ptr cur = GetThis();
	assert(cur);
	try {
		cur->m_cb();
		cur->m_cb = nullptr;
		cur->m_state = TERM;
	} catch (std::exception& ex) {
		LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
								   << " fiber_id = " << cur->getId()
								   << std::endl
								   << myriel::BacktraceToString();
	} catch (...) {
		LOG_ERROR(g_logger) << "Fiber Except" 
								   << " fiber_id = " << cur->getId()
								   << std::endl
								   << myriel::BacktraceToString();
	}

	auto raw_ptr = cur.get();
	cur.reset(); 
	// 在协程调度器中使用fiber类时，这里应使用back函数
	raw_ptr->yield();

	ASSERT2(false, "never reach fiber_id = " + std::to_string(raw_ptr->getId()));
}

uint64_t Fiber::GetFiberId() {
	if (t_fiber) {
		return t_fiber->getId();
	}
	return 0;
}

uint64_t Fiber::GetTotalFibers() {
	return s_fiber_count;
}
}