#include "AsioIOServicePool.h"
#include <iostream>

AsioIOServicePool::AsioIOServicePool(std::size_t size)
    : _ioServices(size), _nextIOService(0) {

    // 初始化 WorkGuard
    for (std::size_t i = 0; i < size; ++i) {
        _workGuards.emplace_back(boost::asio::make_work_guard(_ioServices[i]));
    }

    // 启动线程池
    for (std::size_t i = 0; i < size; ++i) {
        _threads.emplace_back([this, i]() {
            _ioServices[i].run();
            });
    }
}

AsioIOServicePool::~AsioIOServicePool() {
    Stop();
    std::cout << "AsioIOServicePool destruct" << std::endl;
}

boost::asio::io_context& AsioIOServicePool::GetIOService() {
    // 轮询获取 io_context
    auto& service = _ioServices[_nextIOService++];
    if (_nextIOService == _ioServices.size()) {
        _nextIOService = 0;
    }
    return service;
}

void AsioIOServicePool::Stop() {
    // 1. 停止所有 work_guard，允许 run() 在任务完成后退出
    for (auto& wg : _workGuards) {
        wg.reset();
    }

    // 2. 如果需要立即强行停止（不等待当前排队的任务），可以显式调用 stop()
    for (auto& service : _ioServices) {
        service.stop();
    }

    // 3. 等待所有线程结束
    for (auto& t : _threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}