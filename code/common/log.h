#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <list>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <string>
#include <stdarg.h>

#include "fixbuffer.hpp"
#include "singleton.hpp"
#include "utils.h"
#include "fiber.h"

/**
 * @brief 使用流式方式将日志级别level的日志写入到logger
 */
#define LOG_LEVEL(logger, level)                    						\
	if (logger->getLevel() <= level)                						\
		myriel::LogEventWrap(myriel::LogEvent::ptr(new myriel::LogEvent( 	\
			logger, level, __FILE__, __LINE__, myriel::GetThreadID(), 		\
			myriel::Fiber::GetFiberId(), time(0), "test"))).getSS()

#define LOG_TRACE(logger) LOG_LEVEL(logger, myriel::LogLevel::TRACE)

#define LOG_DEBUG(logger) LOG_LEVEL(logger, myriel::LogLevel::DEBUG)

#define LOG_INFO(logger) LOG_LEVEL(logger, myriel::LogLevel::INFO)

#define LOG_WRAN(logger) LOG_LEVEL(logger, myriel::LogLevel::WRAN)

#define LOG_ERROR(logger) LOG_LEVEL(logger, myriel::LogLevel::ERROR)

#define LOG_FATAL(logger) LOG_LEVEL(logger, myriel::LogLevel::FATAL)

#define LOG_ROOT() myriel::LoggerMgr::GetInstance()->getRoot()

#define LOG_NAME(name) myriel::LoggerMgr::GetInstance()->getLogger(name)

#define LOG_FILE(name, path) myriel::LoggerMgr::GetInstance()->getFileLogger(name, path)

namespace myriel {

class Logger;
class LogFormatter;

class LogLevel {
public:
	enum Level {
		UNKNOW = 0,		// 未知类别
		TRACE,			// 
		DEBUG,			// 调试
		INFO,			// 一般
		WRAN,			// 警告
		ERROR,			// 错误
		FATAL			// 致命错误
	};

	static const char *ToString(LogLevel::Level level);

	static LogLevel::Level FromString(const std::string &str);
};

/**
 * @brief 日志事件类
 */
class LogEvent {
public:
	using ptr = std::shared_ptr<LogEvent>;

	/**
	 * @brief 日志事件构造函数
	 * 
	 * @param logger 日志器
	 * @param level 日志级别
	 * @param fileName 写入日志的代码文件名
	 * @param line 行数
	 * @param threadId 线程id
	 * @param time 日志写入时间
	 * @param threadName 线程名
	 */
	LogEvent(std::shared_ptr<Logger> logger,
			 LogLevel::Level level,
			 std::string fileName,
			 uint32_t line,
			 long threadId,
			 uint64_t fiberId,
			 uint64_t time,
			 std::string threadName);

	/**
	 * @brief 获取日志器指针
	 */
	std::shared_ptr<Logger> getLogger() const { return m_logger; }

	/**
	 * @brief 获取日志事件级别
	 */
	LogLevel::Level getLevel() const { return m_level; }

	/**
	 * @brief 返回日志内容
	*/
	std::string getContent() const { return m_ss.str(); }

	/**
	 * @brief 返回日志时间
	*/
	time_t getTime() const { return m_time; }

	/**
	 * @brief 返回线程ID
	*/
	long getThreadId() const { return m_threadId; }

	uint64_t getFiberId() const { return m_fiberId; }

	/**
	 * @brief 返回线程名称
	*/
	std::string getThreadName() const { return m_threadName; }

	/**
	 * @brief 返回文件名
	*/
	std::string getFileName() const { return m_fileName; }

	/**
	 * @brief 返回文件行数
	*/
	uint32_t getFileLine() const { return m_fileLine; }

	/**
	 * @brief 获取日志字节流
	 */
	std::stringstream &getSS() { return m_ss; }

	/**
	 * @brief 流式写入日志
	 * 
	 * @param fmt 
	 * @param ... 
	 */
	void format(const char* fmt, ...);

	/**
	 * @brief 格式化写入日志
	 * 
	 * @param fmt 
	 * @param va 
	 */
	void format(const char* fmt, va_list va);

private:
	long m_threadId = 0;					// 线程id
	uint64_t m_fiberId = 0;
	uint32_t m_fileLine = 0;				// 文件行数
	uint64_t m_time;						// 日志写入时间
	std::stringstream m_ss;					// 字节流
	std::string m_fileName;					// 文件名
	std::string m_threadName = "root";		// 线程名
	std::shared_ptr<Logger> m_logger;		// 日志器
	LogLevel::Level m_level;				// 日志级别
};

/**
 * @brief 日志事件包装器
 */
class LogEventWrap {
public:
	/**
	 * @brief 构造函数，初始化日志事件
	 * 
	 * @param e 日志事件
	 */
	LogEventWrap(LogEvent::ptr e);

	/**
	 * @brief 析构函数，调用对应函数，写入日志
	 * 
	 */
	~LogEventWrap();

	/**
	 * @brief 获取当前日志事件
	 * 
	 * @return 返回日志事件智能指针
	 */
	LogEvent::ptr getEvent() const { return m_event; }

	std::stringstream &getSS();

private:
	LogEvent::ptr m_event;
};

class LogFormatter
{
public:
	using ptr = std::shared_ptr<LogFormatter>;
	LogFormatter(const std::string &pattern);

public:
	/**
	 * @brief 日志内容项格式化，策略模式
	 */
	class FormatItem {
	public:
		using ptr = std::shared_ptr<FormatItem>;
		virtual ~FormatItem() {}

		virtual void format(std::ostream &ofs,
								std::shared_ptr<Logger> logger,
								LogLevel::Level level,
								LogEvent::ptr event) = 0;
	};

	std::string format(std::shared_ptr<Logger> logger,
				LogLevel::Level level,
				LogEvent::ptr event);

	std::ostream &format(std::ostream &ofs,
						  std::shared_ptr<Logger> logger,
						  LogLevel::Level level,
						  LogEvent::ptr event);

	/**
	 * @brief 解析日志模板
	 */
	void init();

private:
	std::string m_pattern;
	std::vector<FormatItem::ptr> m_items;
	bool m_error;
};

class LogAppender {
friend class Logger;
public:
	using ptr = std::shared_ptr<LogAppender>;
	virtual ~LogAppender() {}

	virtual void log(std::shared_ptr<Logger> logger, 
					 LogLevel::Level level, 
					 LogEvent::ptr event
					 ) = 0;

	LogFormatter::ptr getFormatter();

protected:
	LogLevel::Level m_level = LogLevel::DEBUG;
	LogFormatter::ptr m_formatter;
	std::mutex m_mutex;
};

class StdoutLogAppender : public LogAppender {
public:
	using ptr = std::shared_ptr<StdoutLogAppender>;
	void log(std::shared_ptr<Logger> logger,
			 LogLevel::Level level,
			 LogEvent::ptr event) override;
};

class AsycLogAppender : public LogAppender {
public:
	using ptr = std::shared_ptr<AsycLogAppender>;
	using Buffer = FixBuffer<LargeBuffer>;
	using BufferPtr = std::unique_ptr<Buffer>;
	using Buffers = std::vector<BufferPtr>;

	void log(std::shared_ptr<Logger> logger,
			 LogLevel::Level level,
			 LogEvent::ptr event) override;

	~AsycLogAppender() { if(m_run) stop();}

	void append(std::string log, int len);

	void stop() { 
		m_run = false; 
		m_con.notify_all();
		m_workThread.join();
	}

	explicit AsycLogAppender(std::string path, int flushTimeVal = 1);
private:
	void backendThread();

private:
	std::string m_path;
	std::atomic<bool> m_run;			// 原子变量，是否运行
	int m_flushTimeInterval;			// 刷新间隔

	std::thread m_workThread;			// 后端工作线程
	std::mutex m_mutex;
	std::condition_variable m_con;		// 条件变量
	BufferPtr m_curBuffer;				// 当前缓冲区指针
	BufferPtr m_nextBuffer;				// 备用缓冲区指针
	Buffers m_buffers;					// 缓冲区
};

class Logger : public std::enable_shared_from_this<Logger> {
friend class LoggerManager;
public:
	using ptr = std::shared_ptr<Logger>;

	/**
	 * @brief 构造函数
	 * @param name 日志名称
	 */
	Logger(const std::string& name = "root");

	/**
	 * @brief 写日志
	 * 
	 * @param level 日志级别
	 * @param event 日志事件
	 */
	void log(LogLevel::Level level, LogEvent::ptr event);

	void setFormatter(LogFormatter::ptr val);

	void setFormatter(std::string val);

	LogFormatter::ptr getFormatter() const { return m_formatter; }

	void setLevel(LogLevel::Level level);

	LogLevel::Level getLevel() const { return m_level; }

	void addAppender(LogAppender::ptr appender);

	void delAppender(LogAppender::ptr appender);

	void clearAppender();

	std::string getName() const { return m_name; }
private:
	std::string m_name;
	LogLevel::Level m_level;
	LogFormatter::ptr m_formatter;
	std::list<LogAppender::ptr> m_appenders;
	Logger::ptr m_root;

	std::mutex m_mutex;
};

/**
 * @brief 日志器管理类
*/
class LoggerManager {
public:

	/**
	 * @brief 构造函数
	*/
	LoggerManager();

	/**
	 * @brief 获取日志器
	 * @param[in] name 日志器名称
	*/
	Logger::ptr getLogger(const std::string& name);

	Logger::ptr getFileLogger(const std::string &name, std::string path);

	/**
	 * @brief 初始化
	*/
	void init();

	/**
	 * @brief 返回日志器
	*/
	Logger::ptr getRoot() const { return m_root; };

private:
	// 日志器容器
	std::map<std::string, Logger::ptr> m_loggers;
	// 主日志器
	Logger::ptr m_root;

	std::mutex m_mutex;
};

// 日志器管理类单例模式
using LoggerMgr =  myriel::Singleton<LoggerManager>;
}