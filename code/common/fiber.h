#pragma once

#include <memory>
#include <thread>
#include <functional>

#include <ucontext.h>

#include "macro.h"

namespace myriel {
class Scheduler;

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
	using ptr = std::shared_ptr<Fiber>;
	/**
	 * @brief 协程状态枚举
	 */
	enum State {
		READY,			// 就绪态
		EXEC,				// 运行态
		TERM				// 结束态
	};

private:
	/**
	 * @brief 无参构造函数
	 * @attention 用于初始化当前线程的协程功能，构造线程主协程对象，
	 * 以及对t_fiber和t_thread_fiber进行赋值。
	 */
	Fiber();

public:
	/**
	 * @brief 有参构造函数，创建协程并绑定协程函数
	 * 
	 * @param cb 协程函数
	 * @param stacksize 协程栈大小
	 * @param run_in_scheduler 是否由调度器调度
	 */
	Fiber(std::function<void()> cb, size_t stacksize = 0, bool run_in_scheduler = true);
	
	/**
	 * @brief 析构函数，对调度协程和普通协程进行不同的析构处理
	 */
	~Fiber();

	/**
	 * @brief 重置协程
	 * 
	 * @param cb 协程执行函数
	 */
	void reset(std::function<void()> cb);

	/**
	 * @brief 切换协程到运行状态
	 * 
	 */
	void resume();

	/**
	 * @brief 切换协程到后台
	 * 
	 */
	void yield();

	/**
	 * @brief 获取协程id
	 * 
	 * @return uint64_t 
	 */
	uint64_t getId() const { return m_id; }

	/**
	 * @brief 获取协程状态
	 * 
	 * @return State 
	 */
	State getState() const { return m_state; }

public:
	/**
	 * @brief 设置当前协程
	 * @param f 运行协程
	 */
	static void SetThis(Fiber *f);

	/**
	 * @brief 返回当前协程的智能指针
	 * 
	 * @return Fiber::ptr 
	 */
	static Fiber::ptr GetThis();

	/**
	 * @brief 协程执行函数
	 * 
	 */
	static void MainFunc();

	/**
	 * @brief 静态函数，获取当前协程的ID
	 * 
	 * @return uint64_t 
	 */
	static uint64_t GetFiberId();

	static uint64_t GetTotalFibers();

private:
	uint64_t m_id = 0;			// 协程id
	uint32_t m_stacksize = 0;	// 协程运行栈大小
	State m_state = READY;		// 协程状态

	ucontext_t m_ctx;			// 协程上下文
	void *m_stack = nullptr;	// 协程运行栈指针

	std::function<void()> m_cb;	// 协程运行函数

	bool m_scheduler;
};
}