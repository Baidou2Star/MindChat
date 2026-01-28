#include "RedisMgr.h"
#include <string>
#include <hiredis/hiredis.h>
#include "const.h"
#include "ConfigMgr.h"
#include "DistLock.h"
#include <iostream>
#include <cstring>

// =================================================================
// 1. RedisConPool 实现部分
// =================================================================

RedisConPool::RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
    : poolSize_(poolSize), host_(host), port_(port), b_stop_(false), pwd_(pwd), counter_(0), fail_count_(0) {

    for (size_t i = 0; i < poolSize_; ++i) {
        auto* context = redisConnect(host, port);
        if (context == nullptr || context->err != 0) {
            if (context != nullptr) redisFree(context);
            continue;
        }

        auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
        if (reply->type == REDIS_REPLY_ERROR) {
            std::cout << "Redis认证失败" << std::endl;
            freeReplyObject(reply);
            continue;
        }

        freeReplyObject(reply);
        std::cout << "Redis认证成功" << std::endl;
        connections_.push(context);
    }

    // 启动心跳检查线程
    check_thread_ = std::thread([this]() {
        while (!b_stop_) {
            counter_++;
            if (counter_ >= 60) {
                checkThreadPro();
                counter_ = 0;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        });
}

RedisConPool::~RedisConPool() {
    Close();
    ClearConnections();
}

void RedisConPool::ClearConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connections_.empty()) {
        auto* context = connections_.front();
        redisFree(context);
        connections_.pop();
    }
}

redisContext* RedisConPool::getConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this] {
        if (b_stop_) return true;
        return !connections_.empty();
        });

    if (b_stop_) return nullptr;

    auto* context = connections_.front();
    connections_.pop();
    return context;
}

redisContext* RedisConPool::getConNonBlock() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (b_stop_ || connections_.empty()) return nullptr;

    auto* context = connections_.front();
    connections_.pop();
    return context;
}

void RedisConPool::returnConnection(redisContext* context) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (b_stop_) return;
    connections_.push(context);
    cond_.notify_one();
}

void RedisConPool::Close() {
    b_stop_ = true;
    cond_.notify_all();
    if (check_thread_.joinable()) {
        check_thread_.join();
    }
}

void RedisConPool::checkThreadPro() {
    size_t pool_size;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_size = connections_.size();
    }

    for (int i = 0; i < pool_size && !b_stop_; i++) {
        redisContext* context = getConNonBlock();
        if (context == nullptr) break;

        redisReply* reply = (redisReply*)redisCommand(context, "PING");
        if (!reply || context->err || reply->type == REDIS_REPLY_ERROR) {
            std::cout << "Redis连接失效，准备重连..." << std::endl;
            if (reply) freeReplyObject(reply);
            redisFree(context);
            fail_count_++;
        }
        else {
            freeReplyObject(reply);
            returnConnection(context);
        }
    }

    while (fail_count_ > 0 && !b_stop_) {
        if (reconnect()) fail_count_--;
        else break;
    }
}

bool RedisConPool::reconnect() {
    auto* context = redisConnect(host_, port_);
    if (context == nullptr || context->err != 0) {
        if (context != nullptr) redisFree(context);
        return false;
    }

    auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd_);
    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        redisFree(context);
        return false;
    }

    freeReplyObject(reply);
    returnConnection(context);
    return true;
}

// =================================================================
// 2. RedisMgr 实现部分
// =================================================================

RedisMgr::RedisMgr() {
    auto& gCfgMgr = ConfigMgr::Inst();
    auto host = gCfgMgr["Redis"]["Host"];
    auto port = gCfgMgr["Redis"]["Port"];
    auto pwd = gCfgMgr["Redis"]["Passwd"];
    // 按照新版逻辑初始化连接池，默认池大小设为10
    _con_pool.reset(new RedisConPool(10, host.c_str(), atoi(port.c_str()), pwd.c_str()));
}

RedisMgr::~RedisMgr() {
    // 智能指针会自动处理 _con_pool 的释放
}

bool RedisMgr::Get(const std::string& key, std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) {
        return false;
    }

    auto reply = (redisReply*)redisCommand(connect, "GET %s", key.c_str());
    if (reply == nullptr) {
        std::cout << "[ GET " << key << " ] failed" << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        std::cout << "[ GET " << key << " ] failed: not a string" << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    value = reply->str;
    freeReplyObject(reply);
    std::cout << "Succeed to execute command [ GET " << key << " ]" << std::endl;
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return false;

    auto reply = (redisReply*)redisCommand(connect, "SET %s %s", key.c_str(), value.c_str());
    if (reply == nullptr) {
        _con_pool->returnConnection(connect);
        return false;
    }

    // 兼容性修改：使用 strcmp 并检查 OK 或 ok
    bool success = (reply->type == REDIS_REPLY_STATUS &&
        (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0));

    freeReplyObject(reply);
    std::cout << "Execute command [ SET " << key << " ] " << (success ? "success!" : "failure!") << std::endl;
    _con_pool->returnConnection(connect);
    return success;
}

bool RedisMgr::SetWithExpire(const std::string& key, const std::string& value, int expire_seconds) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return false;

    auto reply = (redisReply*)redisCommand(connect, "SETEX %s %d %s", key.c_str(), expire_seconds, value.c_str());
    if (reply == nullptr) {
        _con_pool->returnConnection(connect);
        return false;
    }

    bool success = (reply->type == REDIS_REPLY_STATUS &&
        (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0));

    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return success;
}

bool RedisMgr::LPush(const std::string& key, const std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return false;

    auto reply = (redisReply*)redisCommand(connect, "LPUSH %s %s", key.c_str(), value.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        std::cout << "Execute command [ LPUSH " << key << " ] failure!" << std::endl;
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::LPop(const std::string& key, std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return false;

    auto reply = (redisReply*)redisCommand(connect, "LPOP %s", key.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    value = reply->str;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return false;

    auto reply = (redisReply*)redisCommand(connect, "RPUSH %s %s", key.c_str(), value.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::RPop(const std::string& key, std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return false;

    auto reply = (redisReply*)redisCommand(connect, "RPOP %s", key.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    value = reply->str;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::HSet(const std::string& key, const std::string& hkey, const std::string& value) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return false;

    auto reply = (redisReply*)redisCommand(connect, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return false;

    const char* argv[4];
    size_t argvlen[4];
    argv[0] = "HSET"; argvlen[0] = 4;
    argv[1] = key; argvlen[1] = strlen(key);
    argv[2] = hkey; argvlen[2] = strlen(hkey);
    argv[3] = hvalue; argvlen[3] = hvaluelen;

    auto reply = (redisReply*)redisCommandArgv(connect, 4, argv, argvlen);
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hkey) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return "";

    auto reply = (redisReply*)redisCommand(connect, "HGET %s %s", key.c_str(), hkey.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return "";
    }

    std::string value = reply->str;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return value;
}

bool RedisMgr::HDel(const std::string& key, const std::string& field) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return false;

    auto reply = (redisReply*)redisCommand(connect, "HDEL %s %s", key.c_str(), field.c_str());
    if (reply == nullptr) {
        _con_pool->returnConnection(connect);
        return false;
    }

    bool success = (reply->type == REDIS_REPLY_INTEGER && reply->integer >= 0);
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return success;
}

bool RedisMgr::Del(const std::string& key) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return false;

    auto reply = (redisReply*)redisCommand(connect, "DEL %s", key.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::ExistsKey(const std::string& key) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return false;

    auto reply = (redisReply*)redisCommand(connect, "EXISTS %s", key.c_str());
    if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER || reply->integer == 0) {
        if (reply) freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

// --- 分布式锁接口实现 ---

std::string RedisMgr::acquireLock(const std::string& lockName, int lockTimeout, int acquireTimeout) {
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return "";

    // 调用分布式锁类的单例
    std::string identifier = DistLock::Inst().acquireLock(connect, lockName, lockTimeout, acquireTimeout);

    _con_pool->returnConnection(connect);
    return identifier;
}

bool RedisMgr::releaseLock(const std::string& lockName, const std::string& identifier) {
    if (identifier.empty()) return true;
    auto connect = _con_pool->getConnection();
    if (connect == nullptr) return false;

    bool res = DistLock::Inst().releaseLock(connect, lockName, identifier);

    _con_pool->returnConnection(connect);
    return res;
}