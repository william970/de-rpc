#include "client.h"
#include"router.h"
#include<iostream>
using namespace std;

rpc::client::client(boost::asio::io_service & io_service):
	io_service_(io_service),
	socket_(io_service),
	recv_buf_(""),
	if_connected_ (false),
	len_ (0)
{
}

rpc::client::~client()
{
	io_service_.stop();
	socket_.close();
}

bool rpc::client::connect(const std::string & addr, short port)
{
	auto ret = true;
	if (!if_connected_)
	{
		try
		{
			tcp::endpoint e(boost::asio::ip::address::from_string(addr), port);
			socket_.connect(e);
			read_head();
			if_connected_ = true;
			ret = true;
		}
		
		catch (const boost::system::system_error& ec)
		{
			std::cerr << ec.what() << std::endl;
			ret = false;
		}
	}

	return ret;
}

std::string rpc::client::get_async_data()
{
	return recv_buf_;
}



void rpc::client::read_body(std::size_t size)
{
	boost::asio::async_read(socket_, boost::asio::buffer(data_, size), [this](boost::system::error_code ec, std::size_t length)
	{

		if (!socket_.is_open())
			return;

		if (!ec)
		{
			cout << "收到新消息 : " << data_ << endl;
			router& _router = router::get();
			char* tmp_char = new char[8 + 1];
			memcpy(tmp_char, data_, 8);
			tmp_char[8] = '\0';
			if (tmp_char[0] == '1') {
				read_head();
				return;
			}
			_router.route2(data_ + 8, length, [tmp_char,this](std::string json) { response(json, tmp_char); });//具体调用rpc
			delete[] tmp_char;
			cout << "到这里了1" << endl;
			read_head();
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
void rpc::client::response(std::string json_str,std::string id)
{
	//auto self(this->shared_from_this());
	std::string func_name = "response";
	auto serial_str = make_serial(func_name, id,json_str);
	auto len = (u_int)serial_str.length();
	message_[0] = boost::asio::buffer(&len, 4);
	message_[1] = boost::asio::buffer(serial_str);
	cout << "rpc执行结果" << serial_str << endl;
	socket_.send(message_);

	//boost::asio::async_write(socket_, message_, [this](boost::system::error_code ec, std::size_t length)
	//{
	//	if (!ec)
	//	{
	//		cout << "success" << endl;
	//	}
	//	else
	//	{
	//		//log
	//		std::cout << "server offline\n";
	//		//close();
	//		return;
	//	}
	//});
}




