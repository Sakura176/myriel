#pragma once

#include <cstring>

namespace myriel {

// 前后端统一的buffer
// 这是缓冲区用到的buffer
// 缓冲区buffer的大小设定
const int LargeBuffer = 4096 * 1000; // 4MB
const int SmallBuffer = 4096;
// 非类型参数的模板类 来指定缓冲buffer的大小
template <int SIZE> class FixBuffer {
public:
	/**
	 * @brief 构造函数
	 */
	FixBuffer() : m_cur(m_data) {}

	~FixBuffer() = default;

	/**
	 * @brief 获取缓存中的数据
	 * 
	 * @return const char* 数据指针
	 */
	const char *data() const { return m_data; }

	/**
	 * @brief 获取数据长度
	 * 
	 * @return int 数据长度
	 */
	int length() const { return static_cast<int>(m_cur - m_data); }

	/**
	 * @brief 获取当前指针
	 * 
	 * @return char* 
	 */
	char *current() { return m_cur; }

	/**
	 * @brief 返回缓冲区剩余空间大小
	 * 
	 * @return int 缓冲区剩余空间大小
	 */
	int avail() const { return static_cast<int>(end() - m_cur); }

	/**
	 * @brief 向缓冲区存入数据
	 * 
	 * @param buf 数据指针
	 * @param len 数据长度
	 */
	void append(const char *buf, size_t len) {
		if (static_cast<size_t>(avail()) > len) {
			// 给字符地址初始化
			memcpy(m_cur, buf, len);
			m_cur = m_cur + len;
		}
	}

	/**
	 * @brief 重置当前指针
	 * 
	 */
	void reset() { m_cur = m_data; }

	/**
	 * @brief 缓冲区中数据置0
	 * 
	 */
	void bezero() { bzero(m_data, sizeof(m_data)); }

private:
	/**
	 * @brief 获取数据尾部指针
	 * 
	 * @return const char* 
	 */
	const char *end() const { return m_data + sizeof m_data; }

	char m_data[SIZE]{};	// 缓冲区数组

	char *m_cur;			// 当前指针
};
} // namespace myriel