#pragma once
#include "Singleton.h"
#include <functional>
#include <map>
#include "const.h"

class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;
class LogicSystem :public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem() = default;
	bool HandleGet(const std::string& url, std::shared_ptr<HttpConnection> conn);
	bool HandlePost(const std::string& url, std::shared_ptr<HttpConnection> conn);
	
	void RegGet(std::string url, HttpHandler handler);
	void RegPost(std::string url, HttpHandler handler);
private:
	LogicSystem();
	std::map<std::string, HttpHandler>  _get_handlers;
	std::map<std::string, HttpHandler>  _post_handlers;
	
};

