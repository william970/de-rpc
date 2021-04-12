#include "MiddleConnection.h"
#include"MiddleRouter.h"


MiddleConnection::MiddleConnection(int conid,boost::asio::io_service & io_service, std::size_t timeout_milli) :
	socket_(io_service),
	message_{ (boost::asio::buffer(head_),
	boost::asio::buffer(read_data_)) },
	timer_(io_service),
	timeout_milli_(timeout_milli),
	conid_(conid)
{

}

tcp::socket & MiddleConnection::socket()
{
	return socket_;
}

void MiddleConnection::read_head()
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

void MiddleConnection::do_write() {
	auto self(this->shared_from_this());
	boost::asio::async_write(socket_, write_data_q.front(),[this, self](boost::system::error_code ec, std::size_t)
	{
		if (!ec) {
			write_data_q.pop_front();
			if (!write_data_q.empty()) {
				do_write();
			}
		}
		else {
			std::cerr << "write message error" << std::endl;
		}
	});
}

void MiddleConnection::read_body(std::size_t size)
{
	auto self(this->shared_from_this());
	boost::asio::async_read(socket_, boost::asio::buffer(read_data_, size), [this, self](boost::system::error_code ec, std::size_t length)
	{
		cancel_timer();

		if (!socket_.is_open())
			return;

		if (!ec)
		{
			//router& _router = router::get();
			//_router.route(data_, length, [this](const char* json) { response(json); });//具体调用rpc
			MiddleRouter& router = MiddleRouter::get_instance();
			router.route(read_data_, length,this->conid_);
			read_head(); //继续
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
void MiddleConnection::response(const char * json_str)
{
	auto len = strlen(json_str);
	std::cout << "len = " << len << std::endl;
	std::cout << "json_str = " << json_str << std::endl;
	message_[0] = boost::asio::buffer(&len, 4);
	message_[1] = boost::asio::buffer((char*)json_str, len);
	bool write_in_progress = !write_data_q.empty();
	write_data_q.push_back(message_);
	if (!write_in_progress) {
		do_write();
	}
}

void MiddleConnection::reset_timer()
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


void MiddleConnection::cancel_timer()
{
	if (timeout_milli_ == 0)
		return;

	timer_.cancel();
}

void MiddleConnection::start()
{

	read_head();
}


void MiddleConnection::close()
{
	boost::system::error_code ignored_ec;
	socket_.close(ignored_ec);
}