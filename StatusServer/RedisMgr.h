#pragma once
#include <string>
#include <hiredis/hiredis.h>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <memory>
#include <iostream>
#include "Singleton.h"

// 连接池类：完全同步新版本的健康检查与自动重连逻辑
class RedisConPool {
public:
    RedisConPool(size_t poolSize, std::string host, int port, std::string pwd);
    ~RedisConPool();

    void ClearConnections();
    redisContext* getConnection();
    redisContext* getConNonBlock();
    void returnConnection(redisContext* context);
    void Close();

private:
    void checkThreadPro();
    bool reconnect();

    std::atomic<bool> b_stop_;
    size_t poolSize_;
    std::string host_;
    std::string pwd_;
    int port_;
    std::queue<redisContext*> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::thread check_thread_;
    int counter_;
    std::atomic<int> fail_count_;
};

// Redis管理类：包含新版结构及您需要的扩展接口
class RedisMgr : public Singleton<RedisMgr>, public std::enable_shared_from_this<RedisMgr>
{
    friend class Singleton<RedisMgr>;
public:
    ~RedisMgr();

    // 基础操作
    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value);
    bool SetWithExpire(const std::string& key, const std::string& value, int expire_seconds);
    bool Del(const std::string& key);
    bool ExistsKey(const std::string& key);

    // 列表操作
    bool LPush(const std::string& key, const std::string& value);
    bool LPop(const std::string& key, std::string& value);
    bool RPush(const std::string& key, const std::string& value);
    bool RPop(const std::string& key, std::string& value);

    // 哈希操作
    bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
    bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
    std::string HGet(const std::string& key, const std::string& hkey);
    bool HDel(const std::string& key, const std::string& field);

    // 分布式锁相关接口
    std::string acquireLock(const std::string& lockName, int lockTimeout, int acquireTimeout);
    bool releaseLock(const std::string& lockName, const std::string& identifier);

    void Close() {
        _con_pool->Close();
        _con_pool->ClearConnections();
    }

private:
    RedisMgr();
    std::unique_ptr<RedisConPool> _con_pool;
};