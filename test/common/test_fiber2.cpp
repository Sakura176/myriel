#include "../../code/common/fiber.h"
#include "../../code/common/log.h"
#include <string>
#include <vector>

myriel::Logger::ptr g_logger = LOG_ROOT();

void run_in_fiber2() {
    LOG_INFO(g_logger) << "run_in_fiber2 begin";
    LOG_INFO(g_logger) << "run_in_fiber2 end";
}

void run_in_fiber() {
    LOG_INFO(g_logger) << "run_in_fiber begin";

    /**
     * 非对称协程，子协程不能创建并运行新的子协程，下面的操作是有问题的，
     * 子协程再创建子协程，原来的主协程就跑飞了
     */
    // myriel::Fiber::ptr fiber(new myriel::Fiber(run_in_fiber2, 0, false));
    // fiber->resume();

    LOG_INFO(g_logger) << "run_in_fiber end";
}

int main(int argc, char *argv[]) {
    // myriel::EnvMgr::GetInstance()->init(argc, argv);
    // myriel::Config::LoadFromConfDir(myriel::EnvMgr::GetInstance()->getConfigPath());

    LOG_INFO(g_logger) << "main begin";

    myriel::Fiber::GetThis();

    myriel::Fiber::ptr fiber(new myriel::Fiber(run_in_fiber, 0, false));
    fiber->resume();

    LOG_INFO(g_logger) << "main end";
    return 0;
}