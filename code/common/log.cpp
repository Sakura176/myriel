#include "log.h"

namespace myriel{

const char *LogLevel::ToString(LogLevel::Level level) {

	switch (level)
	{
		case TRACE:
			return "TARCE";
			break;
		case DEBUG:
			return "DEBUG";
			break;
		case INFO:
			return "INFO";
			break;
		case WRAN:
			return "WARN";
			break;
		case ERROR:
			return "ERROR";
			break;
		case FATAL:
			return "FATAL";
			break;
		default:
			return "UNKNOW";
	}
}

LogLevel::Level LogLevel::FromString(const std::string &str) {
	if(str == "TRACE" || str == "trace") {
		return LogLevel::TRACE;
	}
	else if(str == "DEBUG" || str == "debug") {
		return LogLevel::DEBUG;
	}
	else if (str == "INFO" || str == "info") {
		return LogLevel::INFO;
	}
	else if(str == "WARN" || str == "warn") {
		return LogLevel::WRAN;
	}
	else if(str == "ERROR" || str == "error") {
		return LogLevel::ERROR;
	}
	else if(str == "FATAL" || str == "fatal") {
		return LogLevel::FATAL;
	}
	return LogLevel::UNKNOW;
}

LogEvent::LogEvent(std::shared_ptr<Logger> logger,
						LogLevel::Level level,
						std::string fileName,
						uint32_t line,
						long threadId,
						uint64_t fiberId,
						uint64_t time,
						std::string threadName) 
: m_threadId(threadId), m_fiberId(fiberId), m_fileLine(line), m_time(time)
, m_fileName(fileName), m_threadName(threadName), m_logger(logger) ,m_level(level) {
	
}

void LogEvent::format(const char* fmt, ...) {
	va_list va;
	va_start(va, fmt);
	format(fmt, va);
	va_end(va);
}

void LogEvent::format(const char* fmt, va_list va)
{
	char* buf = nullptr;
	int len = vasprintf(&buf, fmt, va);
	if(len != -1)
	{
		m_ss << std::string(buf, len);
		free(buf);
	}
}

LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e) {
	// std::cout << "LogEventWrap" << std::endl;
}

LogEventWrap::~LogEventWrap() {
	// std::cout << "~LogEventWrap" << std::endl;
	m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream &LogEventWrap::getSS() {
	return m_event->getSS();
}

/**************************日志格式化类**********************/
class MessageFormatItem : public LogFormatter::FormatItem {
public:
	MessageFormatItem(const std::string &format) {}
	void format(std::ostream &ofs,
				std::shared_ptr<Logger> logger,
				LogLevel::Level level,
				LogEvent::ptr event) override {

		ofs << event->getContent();
	}
};

class DateFormatItem : public LogFormatter::FormatItem {
public:
	DateFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
	: m_format(format) {
		if(m_format.empty()) {
			m_format = "%Y-%m-%d %H:%M:%S";
		}
	}
	void format(std::ostream &ofs,
								std::shared_ptr<Logger> logger,
								LogLevel::Level level,
								LogEvent::ptr event) override{
		struct tm tm;
		time_t time = event->getTime();
		localtime_r(&time, &tm);
		char buf[64];
		strftime(buf, sizeof(buf), m_format.c_str(), &tm);
		ofs << buf;
	}

private:
	std::string m_format;
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
	ThreadIdFormatItem(const std::string &format) {}
	void format(std::ostream &ofs,
								std::shared_ptr<Logger> logger,
								LogLevel::Level level,
								LogEvent::ptr event) override {
		ofs << event->getThreadId();
	}
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
	FiberIdFormatItem(const std::string &format) {}
	void format(std::ostream &ofs,
								std::shared_ptr<Logger> logger,
								LogLevel::Level level,
								LogEvent::ptr event) override {
		ofs << event->getFiberId();
	}
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
	ThreadNameFormatItem(const std::string &format) {}
	void format(std::ostream &ofs,
								std::shared_ptr<Logger> logger,
								LogLevel::Level level,
								LogEvent::ptr event) override{
		ofs << event->getThreadName();
	}
};

class FileNameFormatItem : public LogFormatter::FormatItem {
public:
	FileNameFormatItem(const std::string &format) {}
	void format(std::ostream &ofs,
								std::shared_ptr<Logger> logger,
								LogLevel::Level level,
								LogEvent::ptr event) override{
		ofs << event->getFileName();
	}
};

class FileLineFormatItem : public LogFormatter::FormatItem {
public:
	FileLineFormatItem(const std::string &format) {}
	void format(std::ostream &ofs,
								std::shared_ptr<Logger> logger,
								LogLevel::Level level,
								LogEvent::ptr event) override{
		ofs << std::to_string(event->getFileLine());
	}
}; 

class LevelFormatItem : public LogFormatter::FormatItem {
public:
	LevelFormatItem(const std::string &format) {}
	void format(std::ostream &ofs,
								std::shared_ptr<Logger> logger,
								LogLevel::Level level,
								LogEvent::ptr event) override{
		ofs << LogLevel::ToString(event->getLevel());
	}
}; 

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
	NewLineFormatItem(const std::string& format) {}
	void format(std::ostream &ofs,
								std::shared_ptr<Logger> logger,
								LogLevel::Level level,
								LogEvent::ptr event) override {
		ofs << std::endl;
	}
}; 

class TabFormatItem : public LogFormatter::FormatItem {
public:
	TabFormatItem(const std::string& format) {}
	void format(std::ostream &ofs,
								std::shared_ptr<Logger> logger,
								LogLevel::Level level,
								LogEvent::ptr event) override {
		ofs << "\t";
	}
}; 

class NameFormatItem : public LogFormatter::FormatItem {
public:
	NameFormatItem(const std::string& format) {}
	void format(std::ostream &ofs,
								std::shared_ptr<Logger> logger,
								LogLevel::Level level,
								LogEvent::ptr event) override {
		ofs << event->getLogger()->getName();
	}
}; 

class StringFormatItem : public LogFormatter::FormatItem {
public:
	StringFormatItem(const std::string& str) : m_string(str) {}
	void format(std::ostream &ofs,
								std::shared_ptr<Logger> logger,
								LogLevel::Level level,
								LogEvent::ptr event) override {
		ofs << m_string;
	}
private:
	std::string m_string;
};
/**************************日志格式化类**********************/

LogFormatter::LogFormatter(const std::string &pattern)
	: m_pattern(pattern){
	init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger,
							LogLevel::Level level,
							LogEvent::ptr event) {
	std::stringstream ss;

	for (auto &i : m_items) {
		i->format(ss, logger, level, event);
	}
	return ss.str();
}

std::ostream &LogFormatter::format(std::ostream &ofs,
							std::shared_ptr<Logger> logger,
							LogLevel::Level level,
							LogEvent::ptr event) {
	// std::cout << "LogFormatter format" << std::endl;
	for (auto &i : m_items) {
		i->format(ofs, logger, level, event);
	}
	return ofs;
}

void LogFormatter::init() {
	std::vector<std::tuple<std::string, std::string, int> > vec;
	std::string nstr;
	// std::cout << "LogFormatter init mpattern -->" << m_pattern << std::endl;
	for(size_t i = 0; i < m_pattern.size(); ++i) {
		// 非占位符，则将字符加入nstr,进入下次循环
		if(m_pattern[i] != '%') {
			nstr.append(1, m_pattern[i]);
			continue;
		}

		if((i + 1) < m_pattern.size()) {
			if(m_pattern[i + 1] == '%') {
				nstr.append(1, '%');
				continue;
			}
		}

		size_t n = i + 1;
		int fmt_status = 0;
		size_t fmt_begin = 0;

		std::string str;
		std::string fmt;
		while(n < m_pattern.size()) {
			if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
				&& m_pattern[n] != '}')) {
				str = m_pattern.substr(i + 1, n - i - 1);
				break;
			}
			if(fmt_status == 0) {
				if(m_pattern[n] == '{') {
					str = m_pattern.substr(i + 1, n - i - 1);
					fmt_status = 1; // 解析格式
					fmt_begin = n;
					++n;
					continue;
				}
			}
			else if(fmt_status == 1) {
				if(m_pattern[n] == '}') {
					fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
					fmt_status = 0;
					++n;
					break;
				}
			}
			++n;
			if(n == m_pattern.size()) {
				if(str.empty()) {
					str = m_pattern.substr(i + 1);
				}
			}
		}

		if(fmt_status == 0) {
			if(!nstr.empty()) {
				vec.push_back(std::make_tuple(nstr, std::string(), 0));
				nstr.clear();
			}
			vec.push_back(std::make_tuple(str, fmt, 1));
			i = n - 1;
		}
		else if(fmt_status == 1) {
			std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
			m_error = true;
			vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
		}
	}
	if(!nstr.empty()) {
		vec.push_back(std::make_tuple(nstr, "", 0));
	}

	static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> s_format_items = {
#define XX(str, C)                                                               \
	{#str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); }}

		XX(d, DateFormatItem),
		XX(t, ThreadIdFormatItem),
		XX(N, ThreadNameFormatItem),
		XX(f, FileNameFormatItem),
		XX(l, FileLineFormatItem),
		XX(T, TabFormatItem),
		XX(n, NewLineFormatItem),
		XX(p, LevelFormatItem),
		XX(c, NameFormatItem),
		XX(m, MessageFormatItem),
		XX(F, FiberIdFormatItem),
#undef XX
	};

	for (auto &i : vec) {
		// std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
		if (std::get<2>(i) == 0)
		{
			m_items.emplace_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
		}
		else
		{
			auto it = s_format_items.find(std::get<0>(i));
			if(it == s_format_items.end()) {
				m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
				m_error = true;
			} else {
				m_items.push_back(it->second(std::get<1>(i)));
			}
		}
	}
}

LogFormatter::ptr LogAppender::getFormatter() {
	std::lock_guard<std::mutex> locker(m_mutex);
	return m_formatter;
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
	if(level >= m_level) {
		std::lock_guard<std::mutex> locker(m_mutex);
		std::cout << m_formatter->format(logger, level, event);
	}
}

AsycLogAppender::AsycLogAppender(std::string path, int flushTimeVal)
: m_path(path), m_run(true), m_flushTimeInterval(flushTimeVal)
, m_mutex(), m_con(), m_curBuffer(new Buffer)
, m_nextBuffer(new Buffer), m_buffers() {
	// 初始化缓冲区
	m_curBuffer->bezero();
	m_nextBuffer->bezero();

	m_buffers.reserve(16);
	m_workThread = std::move(std::thread([this] {
		backendThread();
	}));
}

void AsycLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
	std::string log;
	if (level >= m_level) {
		log = m_formatter->format(logger, level, event);
		append(log, log.length());
	}
}

void AsycLogAppender::append(std::string log, int len) {
	{
		// std::cout << log << std::endl;
		std::lock_guard<std::mutex> locker(m_mutex);
		// 当前缓冲区空间充足, 写入当前缓冲区
		if(m_curBuffer->avail() > len) {
			m_curBuffer->append(log.c_str(), len);
		} else {
			// 否则将当前缓冲区内容移入数组中
			m_buffers.emplace_back(std::move(m_curBuffer));
			if(m_nextBuffer) {
				m_curBuffer = std::move(m_nextBuffer);
			} else {
				m_curBuffer = std::make_unique<Buffer>();
			}
			m_curBuffer->append(log.c_str(), len);
			m_con.notify_all();
		}
	}
}

void AsycLogAppender::backendThread() {
	std::ofstream ofs(m_path, std::ios::app);
	if(!ofs.is_open()) {
		std::cout << "open file: " << m_path << " failed" << std::endl;
	}

	BufferPtr replaceBuffer1(new Buffer);
	BufferPtr replaceBuffer2(new Buffer);
	// 初始化
	replaceBuffer1->bezero();
	replaceBuffer2->bezero();

	// 输出到文件的buffer
	Buffers BufferToWrite;
	BufferToWrite.reserve(16);
	while(m_run) {
		{
			std::unique_lock<std::mutex> locker(m_mutex);
			if(m_buffers.empty()) {
				m_con.wait_for(locker, std::chrono::seconds(m_flushTimeInterval));
			}
			m_buffers.emplace_back(std::move(m_curBuffer));
			m_curBuffer = std::move(replaceBuffer1);
			BufferToWrite.swap(m_buffers);
			if(!m_nextBuffer) {
				m_nextBuffer = std::move(replaceBuffer2);
			}
		}

		for(const auto &buffer : BufferToWrite) {
			ofs.write(buffer->data(), buffer->length());
		}
		if(BufferToWrite.size() > 2) {
			BufferToWrite.resize(2);
		}
		if(!replaceBuffer1) {
			replaceBuffer1 = std::move(BufferToWrite.back());
			BufferToWrite.pop_back();
			replaceBuffer1->reset();
		}
		if(!replaceBuffer2) {
			replaceBuffer2 = std::move(BufferToWrite.back());
			BufferToWrite.pop_back();
			replaceBuffer2->reset();
		}
		BufferToWrite.clear();

		ofs.flush();
	}
	ofs.flush();
	if(ofs.is_open()) {
		ofs.close();
	}
}

Logger::Logger(const std::string &name) 
: m_name(name), m_level(LogLevel::DEBUG) {
	// 初始化默认日志格式
	m_formatter.reset(new LogFormatter(
		"%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
	));
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {

	if (level >= m_level) {
		auto self = shared_from_this();
		std::lock_guard<std::mutex> locker(m_mutex);
		if (!m_appenders.empty()) {
			for (auto &i : m_appenders) {
				i->log(self, level, event);
			}
		}
		else {
			m_root->log(level, event);
		}
	}
}

void Logger::setFormatter(LogFormatter::ptr val) {
	std::lock_guard<std::mutex> locker(m_mutex);
	m_formatter = std::move(val);

	for (auto &i : m_appenders) {
		// std::lock_guard<std::mutex> locker1(m_mutex);
		if (!i->m_formatter)
		{
			i->m_formatter = m_formatter;
		}
	}
}

void Logger::setFormatter(std::string val) {
	m_formatter.reset(new LogFormatter(val));
}

void Logger::setLevel(LogLevel::Level level) {
	m_level = level;
}

void Logger::addAppender(LogAppender::ptr appender) {
	std::lock_guard<std::mutex> locker(m_mutex);
	if (!appender->getFormatter()) {
		// std::lock_guard<std::mutex> locker1(m_mutex);
		appender->m_formatter = m_formatter;
	}
	m_appenders.emplace_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
	std::lock_guard<std::mutex> locker(m_mutex);
	for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
		if(*it == appender) {
			m_appenders.erase(it);
			break;
		}
	}
}

void Logger::clearAppender() {
	std::lock_guard<std::mutex> locker(m_mutex);
	m_appenders.clear();
}

LoggerManager::LoggerManager() {
	m_root.reset(new Logger);
	m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

	m_loggers[m_root->m_name] = m_root;

	init();
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
	std::lock_guard<std::mutex> locker(m_mutex);
	auto it = m_loggers.find(name);
	if(it != m_loggers.end()) {
		return it->second;
	}

	Logger::ptr logger(new Logger(name));
	logger->m_root = m_root;
	m_loggers[name] = logger;
	return logger;
}

Logger::ptr LoggerManager::getFileLogger(const std::string& name, std::string path) {
	std::lock_guard<std::mutex> locker(m_mutex);
	auto it = m_loggers.find(name);
	if(it != m_loggers.end()) {
		return it->second;
	}

	Logger::ptr logger(new Logger(name));
	logger->m_root = m_root;
	AsycLogAppender::ptr asycLogAppender(new AsycLogAppender(path));
	logger->addAppender(asycLogAppender);
	m_loggers[name] = logger;
	return logger;
}

void LoggerManager::init() {

}
}