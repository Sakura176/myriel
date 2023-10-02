#pragma once

#include <sys/syscall.h>
#include <unistd.h>
#include <execinfo.h>
#include <sys/stat.h>

#include <thread>
#include <string>
#include <sstream>

#include "log.h"

namespace myriel {

/**
 * @brief 获取当前线程ID
 * 
 * @return std::string 
 */
long GetThreadID();

// 不要在栈上分配很大的内存空间
void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);

std::string BacktraceToString(int size = 64, int skip = 2, const std::string &prefix = "");
}