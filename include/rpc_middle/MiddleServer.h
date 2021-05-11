#pragma once
#include <thread>
#include "../rpc/connection.h"
#include "MiddleConnection.h"
#include "../rpc/io_service_pool.h"
#include "../rpc/router.h"



using boost::asio::ip::tcp;

class MiddleServer
{
public:
	MiddleServer(short port, size_t size, size_t timeout_milli = 0);
	~MiddleServer();
	void run();
private:
	void do_accept();
private:

	rpc::io_service_pool io_service_pool_;
	tcp::acceptor acceptor_;
	std::shared_ptr <MiddleConnection> conn_;
	int conn_id;
	std::shared_ptr<std::thread> thd_;
	std::size_t timeout_milli_;
};

