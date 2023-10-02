#include <iostream>
#include <chrono>

#include <unistd.h>

#include "../../code/common/log.h"

class Timer {
public:
    Timer(){
        m_curTime = std::chrono::high_resolution_clock::now();
    }
    ~Timer(){
        auto end = std::chrono::high_resolution_clock::now();
        auto startTime = std::chrono::time_point_cast<std::chrono::microseconds>(m_curTime)
                .time_since_epoch().count();
        auto endTime = std::chrono::time_point_cast<std::chrono::microseconds>(end)
                .time_since_epoch().count();
        auto duration = endTime - startTime;
        double ms = duration * 0.001;//得到毫秒
        printf("%ld us (%lf ms)\n", duration, ms);
        fflush(stdout);
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_curTime;
};

void test_log_time() {
    myriel::Logger::ptr logger(new myriel::Logger("system"));
    // myriel::LogFormatter::ptr formatter(new myriel::LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T[%p]%T[%c]%T%f:%l%T%m%n"));
    // logger->setFormatter(formatter);

    {
        Timer timer;
        for(int i = 0 ;i < 10000; ++i){
            LOG_INFO(logger) << "这是一条日志";
        }
    }
}

void test1() {
    myriel::Logger::ptr logger = LOG_FILE("system", "./bin/log/system.log");
    {
        Timer timer;
        for (int i = 0; i < 1000; ++i) {
            LOG_DEBUG(logger) << "这是一条日志: " << i;
        }
    }
}

int main() {
    // test_log_thread();
    test1();
}