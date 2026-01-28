#include "RedisMgr.h"
#include <string>
#include <hiredis/hiredis.h>
#include <iostream>
#include <cstring>
#include "const.h"
#include "ConfigMgr.h"
#include "DistLock.h"

// 假设你已有 Defer 类型（你在 HDel 里使用了）
// 若没有，请确保 Defer 的析构会执行传入的 lambda。

RedisMgr::RedisMgr() {
	auto& gCfgMgr = ConfigMgr::Inst();
	auto host = gCfgMgr["Redis"]["Host"];
	auto port = gCfgMgr["Redis"]["Port"];
	auto pwd = gCfgMgr["Redis"]["Passwd"];
	_con_pool.reset(new RedisConPool(10, host.c_str(), atoi(port.c_str()), pwd.c_str()));
}

RedisMgr::~RedisMgr() {
	// 最小改动：确保资源释放（前提：RedisConPool::Close 可重复调用且内部 joinable 判断）
	try {
		Close();
	}
	catch (...) {
		// 析构函数不要抛异常
	}
}

bool RedisMgr::Get(const std::string& key, std::string& value)
{
	auto connect = _con_pool->getConnection();
	if (!connect) return false;

	Defer defer([&]() { _con_pool->returnConnection(connect); });

	auto reply = (redisReply*)redisCommand(connect, "GET %s", key.c_str());
	if (!reply) {
		std::cout << "[ GET  " << key << " ] failed" << std::endl;
		return false;
	}
	Defer deferReply([&]() { freeReplyObject(reply); });

	// key 不存在一般是 NIL，你按原逻辑当失败
	if (reply->type != REDIS_REPLY_STRING || !reply->str) {
		std::cout << "[ GET  " << key << " ] failed" << std::endl;
		return false;
	}

	value = reply->str;
	std::cout << "Succeed to execute command [ GET " << key << "  ]" << std::endl;
	return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& value) {
	auto connect = _con_pool->getConnection();
	if (!connect) return false;

	Defer defer([&]() { _con_pool->returnConnection(connect); });

	auto reply = (redisReply*)redisCommand(connect, "SET %s %s", key.c_str(), value.c_str());
	if (!reply) {
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		return false;
	}
	Defer deferReply([&]() { freeReplyObject(reply); });

	if (!(reply->type == REDIS_REPLY_STATUS && reply->str &&
		(strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0)))
	{
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		return false;
	}

	std::cout << "Execut command [ SET " << key << "  " << value << " ] success ! " << std::endl;
	return true;
}

bool RedisMgr::LPush(const std::string& key, const std::string& value)
{
	auto connect = _con_pool->getConnection();
	if (!connect) return false;

	Defer defer([&]() { _con_pool->returnConnection(connect); });

	auto reply = (redisReply*)redisCommand(connect, "LPUSH %s %s", key.c_str(), value.c_str());
	if (!reply) {
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		return false;
	}
	Defer deferReply([&]() { freeReplyObject(reply); });

	if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		return false;
	}

	std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] success ! " << std::endl;
	return true;
}

bool RedisMgr::LPop(const std::string& key, std::string& value) {
	auto connect = _con_pool->getConnection();
	if (!connect) return false;

	Defer defer([&]() { _con_pool->returnConnection(connect); });

	auto reply = (redisReply*)redisCommand(connect, "LPOP %s", key.c_str());
	if (!reply) {
		std::cout << "Execut command [ LPOP " << key << " ] failure ! " << std::endl;
		return false;
	}
	Defer deferReply([&]() { freeReplyObject(reply); });

	if (reply->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ LPOP " << key << " ] failure ! " << std::endl;
		return false;
	}

	if (reply->type != REDIS_REPLY_STRING || !reply->str) {
		std::cout << "Execut command [ LPOP " << key << " ] failure ! " << std::endl;
		return false;
	}

	value = reply->str;
	std::cout << "Execut command [ LPOP " << key << " ] success ! " << std::endl;
	return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value) {
	auto connect = _con_pool->getConnection();
	if (!connect) return false;

	Defer defer([&]() { _con_pool->returnConnection(connect); });

	auto reply = (redisReply*)redisCommand(connect, "RPUSH %s %s", key.c_str(), value.c_str());
	if (!reply) {
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		return false;
	}
	Defer deferReply([&]() { freeReplyObject(reply); });

	if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		return false;
	}

	std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] success ! " << std::endl;
	return true;
}

bool RedisMgr::RPop(const std::string& key, std::string& value) {
	auto connect = _con_pool->getConnection();
	if (!connect) return false;

	Defer defer([&]() { _con_pool->returnConnection(connect); });

	auto reply = (redisReply*)redisCommand(connect, "RPOP %s", key.c_str());
	if (!reply) {
		std::cout << "Execut command [ RPOP " << key << " ] failure ! " << std::endl;
		return false;
	}
	Defer deferReply([&]() { freeReplyObject(reply); });

	if (reply->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ RPOP " << key << " ] failure ! " << std::endl;
		return false;
	}

	if (reply->type != REDIS_REPLY_STRING || !reply->str) {
		std::cout << "Execut command [ RPOP " << key << " ] failure ! " << std::endl;
		return false;
	}

	value = reply->str;
	std::cout << "Execut command [ RPOP " << key << " ] success ! " << std::endl;
	return true;
}

bool RedisMgr::HSet(const std::string& key, const std::string& hkey, const std::string& value) {
	auto connect = _con_pool->getConnection();
	if (!connect) return false;

	Defer defer([&]() { _con_pool->returnConnection(connect); });

	auto reply = (redisReply*)redisCommand(connect, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
	if (!reply) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] failure ! " << std::endl;
		return false;
	}
	Defer deferReply([&]() { freeReplyObject(reply); });

	if (reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] failure ! " << std::endl;
		return false;
	}

	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] success ! " << std::endl;
	return true;
}

bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen)
{
	auto connect = _con_pool->getConnection();
	if (!connect) return false;

	Defer defer([&]() { _con_pool->returnConnection(connect); });

	const char* argv[4];
	size_t argvlen[4];
	argv[0] = "HSET";  argvlen[0] = 4;
	argv[1] = key;     argvlen[1] = strlen(key);
	argv[2] = hkey;    argvlen[2] = strlen(hkey);
	argv[3] = hvalue;  argvlen[3] = hvaluelen;

	auto reply = (redisReply*)redisCommandArgv(connect, 4, argv, argvlen);
	if (!reply) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
		return false;
	}
	Defer deferReply([&]() { freeReplyObject(reply); });

	if (reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
		return false;
	}

	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] success ! " << std::endl;
	return true;
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hkey)
{
	auto connect = _con_pool->getConnection();
	if (!connect) return "";

	Defer defer([&]() { _con_pool->returnConnection(connect); });

	const char* argv[3];
	size_t argvlen[3];
	argv[0] = "HGET";        argvlen[0] = 4;
	argv[1] = key.c_str();   argvlen[1] = key.length();
	argv[2] = hkey.c_str();  argvlen[2] = hkey.length();

	auto reply = (redisReply*)redisCommandArgv(connect, 3, argv, argvlen);
	if (!reply) {
		std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
		return "";
	}
	Defer deferReply([&]() { freeReplyObject(reply); });

	if (reply->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
		return "";
	}

	if (reply->type != REDIS_REPLY_STRING || !reply->str) {
		std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
		return "";
	}

	std::cout << "Execut command [ HGet " << key << " " << hkey << " ] success ! " << std::endl;
	return std::string(reply->str);
}

bool RedisMgr::HDel(const std::string& key, const std::string& field)
{
	auto connect = _con_pool->getConnection();
	if (!connect) return false;

	Defer defer([&]() { _con_pool->returnConnection(connect); });

	redisReply* reply = (redisReply*)redisCommand(connect, "HDEL %s %s", key.c_str(), field.c_str());
	if (!reply) {
		std::cerr << "HDEL command failed" << std::endl;
		return false;
	}
	Defer deferReply([&]() { freeReplyObject(reply); });

	if (reply->type != REDIS_REPLY_INTEGER) return false;
	return reply->integer > 0;
}

bool RedisMgr::Del(const std::string& key)
{
	auto connect = _con_pool->getConnection();
	if (!connect) return false;

	Defer defer([&]() { _con_pool->returnConnection(connect); });

	auto reply = (redisReply*)redisCommand(connect, "DEL %s", key.c_str());
	if (!reply) {
		std::cout << "Execut command [ Del " << key << " ] failure ! " << std::endl;
		return false;
	}
	Defer deferReply([&]() { freeReplyObject(reply); });

	if (reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ Del " << key << " ] failure ! " << std::endl;
		return false;
	}

	std::cout << "Execut command [ Del " << key << " ] success ! " << std::endl;
	return true;
}

bool RedisMgr::ExistsKey(const std::string& key)
{
	auto connect = _con_pool->getConnection();
	if (!connect) return false;

	Defer defer([&]() { _con_pool->returnConnection(connect); });

	auto reply = (redisReply*)redisCommand(connect, "EXISTS %s", key.c_str());
	if (!reply) {
		std::cout << "Not Found [ Key " << key << " ]  ! " << std::endl;
		return false;
	}
	Defer deferReply([&]() { freeReplyObject(reply); });

	if (reply->type != REDIS_REPLY_INTEGER || reply->integer == 0) {
		std::cout << "Not Found [ Key " << key << " ]  ! " << std::endl;
		return false;
	}

	std::cout << " Found [ Key " << key << " ] exists ! " << std::endl;
	return true;
}

std::string RedisMgr::acquireLock(const std::string& lockName,
	int lockTimeout, int acquireTimeout) {

	auto connect = _con_pool->getConnection();
	if (!connect) return "";

	Defer defer([&]() { _con_pool->returnConnection(connect); });

	return DistLock::Inst().acquireLock(connect, lockName, lockTimeout, acquireTimeout);
}

bool RedisMgr::releaseLock(const std::string& lockName,
	const std::string& identifier) {
	if (identifier.empty()) return true;

	auto connect = _con_pool->getConnection();
	if (!connect) return false;

	Defer defer([&]() { _con_pool->returnConnection(connect); });

	return DistLock::Inst().releaseLock(connect, lockName, identifier);
}

void RedisMgr::IncreaseCount(std::string server_name)
{
	auto lock_key = LOCK_COUNT;
	auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
	if (identifier.empty()) return; // 最小修复：没拿到锁就别改数据

	Defer defer2([&, identifier, lock_key]() {
		RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
		});

	auto rd_res = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server_name);
	int count = 0;
	if (!rd_res.empty()) count = std::stoi(rd_res);

	count++;
	RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, std::to_string(count));
}

void RedisMgr::DecreaseCount(std::string server_name)
{
	auto lock_key = LOCK_COUNT;
	auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
	if (identifier.empty()) return;

	Defer defer2([&, identifier, lock_key]() {
		RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
		});

	auto rd_res = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server_name);
	int count = 0;
	if (!rd_res.empty()) {
		count = std::stoi(rd_res);
		if (count > 0) count--;
	}

	RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, std::to_string(count));
}

void RedisMgr::InitCount(std::string server_name) {
	auto lock_key = LOCK_COUNT;
	auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
	if (identifier.empty()) return;

	Defer defer2([&, identifier, lock_key]() {
		RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
		});

	RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");
}

void RedisMgr::DelCount(std::string server_name) {
	auto lock_key = LOCK_COUNT;
	auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
	if (identifier.empty()) return;

	Defer defer2([&, identifier, lock_key]() {
		RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
		});

	RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
}
