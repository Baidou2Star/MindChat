#pragma once
#include "const.h"

//创建接收器的工作
class CServer :public std::enable_shared_from_this<CServer>
{
public:
	CServer(boost::asio::io_context &ioc,short port);
	void Start();
private:
	boost::asio::io_context& _ioc;
	boost::asio::ip::tcp::acceptor _acceptor;
};

