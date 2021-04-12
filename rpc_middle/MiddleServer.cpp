#include "MiddleServer.h"
#include "MiddleRouter.h"

MiddleServer::MiddleServer(short port, size_t size, size_t timeout_milli) :
	io_service_pool_(size), timeout_milli_(timeout_milli),//线程池线程数，过期时间
	acceptor_(io_service_pool_.get_io_service(), tcp::endpoint(tcp::v4(), port)), //将一个线程拿来用来接收三次握手
	conn_id(0)
{
	do_accept();
}

MiddleServer::~MiddleServer()
{
	io_service_pool_.stop();
	thd_->join();
}

void MiddleServer::run()
{
	thd_ = std::make_shared<std::thread>([this] {io_service_pool_.run(); });
}


void MiddleServer::do_accept()
{
	conn_.reset(new MiddleConnection(conn_id,io_service_pool_.get_io_service(), timeout_milli_));
	MiddleRouter& router = MiddleRouter::get_instance();
	router.addCon(conn_id,conn_);
	conn_id++;
	acceptor_.async_accept(conn_->socket(), [this](boost::system::error_code ec)
	{
		if (!ec)
		{
			conn_->start();

		}
		else
		{//todo log
			std::cerr << "do_accept failed " << ec << std::endl;
		}

		do_accept();
	});
}
