#pragma once
#include <chrono>
#include <unordered_map>
#include "const.h"

class HttpConnection :public std::enable_shared_from_this<HttpConnection>
{
public:
	friend class LogicSystem;
	HttpConnection(boost::asio::io_context& ioc );
	tcp::socket& GetSocket() { return _socket; }
	void Start();
	
private:
	void PreParseGetParam();
	//超时检测
	void CheckDeadline();
	//动态应答
	void WriteResponse();
	//处理请求
	void HandleReq();
	tcp::socket _socket;

	//设置缓冲区
	beast::flat_buffer _buffer{8192};

	http::request<http::dynamic_body> _request;
	http::response<http::dynamic_body> _response;
	//定时器是一个底层的调度，要绑定一个调度器
	net::steady_timer deadline_{ _socket.get_executor(),std::chrono::seconds(60)};

	//get请求的参数解析
	std::string _get_url;
	std::unordered_map<std::string, std::string> _get_params;
};

