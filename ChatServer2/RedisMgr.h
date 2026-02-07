#pragma once
#include <string>
#include <hiredis/hiredis.h>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <iostream>
#include "Singleton.h"

class RedisConPool {
public:
	RedisConPool(size_t poolSize, std::string host, int port, std::string pwd)
		: b_stop_(false), poolSize_(poolSize), host_(host), pwd_(pwd), port_(port), counter_(0), fail_count_(0) {
		for (size_t i = 0; i < poolSize_; ++i) {
			auto* context = redisConnect(host_.c_str(), port_);
			if (!context || context->err != 0) {
				if (context) redisFree(context);
				continue;
			}

			if (!authConnection(context)) {
				redisFree(context);
				continue;
			}
			connections_.push(context);
		}

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

	~RedisConPool() {
		Close();
		ClearConnections();
	}

	void ClearConnections() {
		std::lock_guard<std::mutex> lock(mutex_);
		while (!connections_.empty()) {
			auto* context = connections_.front();
			redisFree(context);
			connections_.pop();
		}
	}

	redisContext* getConnection() {
		std::unique_lock<std::mutex> lock(mutex_);
		if (!cond_.wait_for(lock, std::chrono::seconds(2), [this] {
			return b_stop_ || !connections_.empty();
		})) {
			return nullptr;
		}

		if (b_stop_ || connections_.empty()) {
			return nullptr;
		}

		auto* context = connections_.front();
		connections_.pop();
		return context;
	}

	redisContext* getConNonBlock() {
		std::lock_guard<std::mutex> lock(mutex_);
		if (b_stop_ || connections_.empty()) return nullptr;

		auto* context = connections_.front();
		connections_.pop();
		return context;
	}

	void returnConnection(redisContext* context) {
		if (!context) return;

		std::lock_guard<std::mutex> lock(mutex_);
		if (b_stop_) {
			redisFree(context);
			return;
		}

		connections_.push(context);
		cond_.notify_one();
	}

	void Close() {
		b_stop_ = true;
		cond_.notify_all();
		if (check_thread_.joinable()) {
			check_thread_.join();
		}
	}

private:
	void checkThreadPro() {
		size_t pool_size = 0;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			pool_size = connections_.size();
		}

		for (size_t i = 0; i < pool_size && !b_stop_; ++i) {
			redisContext* context = getConNonBlock();
			if (!context) break;

			redisReply* reply = (redisReply*)redisCommand(context, "PING");
			if (!reply || context->err || reply->type == REDIS_REPLY_ERROR) {
				if (reply) freeReplyObject(reply);
				redisFree(context);
				fail_count_++;
				continue;
			}

			freeReplyObject(reply);
			returnConnection(context);
		}

		while (fail_count_ > 0 && !b_stop_) {
			if (reconnect()) {
				fail_count_--;
			} else {
				break;
			}
		}
	}

	bool reconnect() {
		auto* context = redisConnect(host_.c_str(), port_);
		if (!context || context->err != 0) {
			if (context) redisFree(context);
			return false;
		}

		if (!authConnection(context)) {
			redisFree(context);
			return false;
		}

		returnConnection(context);
		return true;
	}

	bool authConnection(redisContext* context) {
		if (!context) return false;
		if (pwd_.empty()) return true;

		auto* reply = (redisReply*)redisCommand(context, "AUTH %s", pwd_.c_str());
		if (!reply) {
			std::cout << "Redis AUTH failed: empty reply" << std::endl;
			return false;
		}

		bool ok = (reply->type != REDIS_REPLY_ERROR);
		if (!ok && reply->str) {
			const std::string err(reply->str);
			if (err.find("without any password configured") != std::string::npos ||
				err.find("no password is set") != std::string::npos) {
				ok = true;
			}
		}

		if (!ok) {
			std::cout << "Redis AUTH failed: " << (reply->str ? reply->str : "unknown error") << std::endl;
		}

		freeReplyObject(reply);
		return ok;
	}

private:
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

class RedisMgr : public Singleton<RedisMgr>,
	public std::enable_shared_from_this<RedisMgr>
{
	friend class Singleton<RedisMgr>;
public:
	~RedisMgr();
	bool Get(const std::string& key, std::string& value);
	bool Set(const std::string& key, const std::string& value);
	bool LPush(const std::string& key, const std::string& value);
	bool LPop(const std::string& key, std::string& value);
	bool RPush(const std::string& key, const std::string& value);
	bool RPop(const std::string& key, std::string& value);
	bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
	bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
	std::string HGet(const std::string& key, const std::string& hkey);
	bool HDel(const std::string& key, const std::string& field);
	bool Del(const std::string& key);
	bool ExistsKey(const std::string& key);
	void Close() {
		_con_pool->Close();
		_con_pool->ClearConnections();
	}

	std::string acquireLock(const std::string& lockName,
		int lockTimeout, int acquireTimeout);

	bool releaseLock(const std::string& lockName,
		const std::string& identifier);

	void IncreaseCount(std::string server_name);
	void DecreaseCount(std::string server_name);
	void InitCount(std::string server_name);
	void DelCount(std::string server_name);
private:
	RedisMgr();
	std::unique_ptr<RedisConPool>  _con_pool;
};

