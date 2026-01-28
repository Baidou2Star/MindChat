#include <vector>
#include <memory>
#include <thread>
#include <boost/asio.hpp>
#include "Singleton.h"

class AsioIOServicePool : public Singleton<AsioIOServicePool>
{
    friend Singleton<AsioIOServicePool>;
public:
    using IOService = boost::asio::io_context;
    // 使用新的 executor_work_guard 替代 work
    using WorkGuard = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

    ~AsioIOServicePool();
    AsioIOServicePool(const AsioIOServicePool&) = delete;
    AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;

    boost::asio::io_context& GetIOService();
    void Stop();

private:
    AsioIOServicePool(std::size_t size = std::thread::hardware_concurrency());

    std::vector<IOService> _ioServices;
    // 直接存储 work_guard，不需要 unique_ptr，因为 work_guard 本身支持移动语义
    std::vector<WorkGuard> _workGuards;
    std::vector<std::thread> _threads;
    std::size_t _nextIOService;
};

