#include "connection.h"


rpc::connection::connection(boost::asio::io_service & io_service, std::size_t timeout_milli):
	socket_ (io_service),
	message_{ (boost::asio::buffer(head_),
	boost::asio::buffer (data_) )},
	timer_ (io_service),
	timeout_milli_ (timeout_milli)
{

}

tcp::socket & rpc::connection::socket()
{
	return socket_;
}

void rpc::connection::read_head()
{
	reset_timer();
	auto self(this->shared_from_this());

	boost::asio::async_read(socket_, boost::asio::buffer(head_), [this, self](boost::system::error_code ec, std::size_t length)
	{
		if (!socket_.is_open())
		{
			cancel_timer();
			return;
		}

		if (!ec)
		{
			const int body_len = *(int*)head_;
			if (body_len > 65536)
			{
				std::cerr << "Out of range" << std::endl;
			}
			else
			{
				read_body(body_len);
			}
		}
		else
		{
			//log
			std::cerr << "read message error" << std::endl;
			return;
		}
	});
}

void rpc::connection::read_body(std::size_t size)
{
	auto self(this->shared_from_this());
	boost::asio::async_read(socket_, boost::asio::buffer(data_, size), [this, self](boost::system::error_code ec, std::size_t length)
	{
		cancel_timer();

		if (!socket_.is_open())
			return;

		if (!ec)
		{
			router& _router = router::get();
			_router.route(data_, length, [this](const char* json) { response(json); });//具体调用rpc
		}
		else
		{
			//log
			std::cerr << "read_body failed\n";
			return;
		}
	});
}

//发消息
void rpc::connection::response(const char * json_str)
{
	auto self(this->shared_from_this());
	auto len = strlen(json_str);
	message_[0] = boost::asio::buffer(&len, 4);
	message_[1] = boost::asio::buffer((char*)json_str, len);

	boost::asio::async_write(socket_, message_, [this, self](boost::system::error_code ec, std::size_t length)
	{
		if (!ec)
		{
			read_head();
		}
		else
		{
			//log
			std::cout << "client offline\n";
			close();
			return;
		}
	});
}

void rpc::connection::reset_timer()
{
	if (timeout_milli_ == 0)
		return;

	auto self(this->shared_from_this());
	timer_.expires_from_now(boost::posix_time::milliseconds(timeout_milli_));
	timer_.async_wait([this, self](const boost::system::error_code& ec)
	{
		if (!socket_.is_open())
		{
			std::cerr << "socket error\n";
			return;
		}

		if (ec)
		{
			std::cerr << "wait error\n";
			return;
		}

		std::cout << "timeout" << std::endl;

		close();
	});
}


void rpc::connection::cancel_timer()
{
	if (timeout_milli_ == 0)
		return;

	timer_.cancel();
}

void rpc::connection::start()
{
	read_head();
}


void rpc::connection::close()
{
	boost::system::error_code ignored_ec;
	socket_.close(ignored_ec);
}
