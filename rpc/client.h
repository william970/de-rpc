#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/asio.hpp>
#include<queue>
#include <memory>
#include "router.h"

#include "utility.hpp"

namespace rpc
{
	using boost::asio::ip::tcp;

	class client:
		public std::enable_shared_from_this<client>,
		private boost::noncopyable
	{
	public:
		client(boost::asio::io_service& io_service);
		~client();
		bool connect(const std::string& addr, short port);//连接

		template<typename... Args>
		std::string call(const std::string & handler_name, Args&&... args);

		template<typename HandlerT, typename... Args>
		void async_call(const char* handler_name, HandlerT &handler, Args&&... args);

		template <typename T>
		auto get_result(const std::string &buf);

		template<typename Function>
		void register_handler(std::string const & name, const Function & f);

		template<typename Function, typename Self>
		void register_handler(std::string const & name, const Function & f, Self * self);

		std::string get_async_data();


		void read_head()
		{

			boost::asio::async_read(socket_, boost::asio::buffer(head_), [this](boost::system::error_code ec, std::size_t length)
			{
				if (!socket_.is_open())
				{
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
	private:
		inline std::string call(const std::string& serial_str);

		template<typename HandlerT>
		inline void async_call(const std::string& json_str, HandlerT &handler);

		template<typename ...Args>
		static std::string make_serial(const std::string &handler_name, Args && ...args);

		void read_body(std::size_t size);
		void response(std::string json_str,std::string id);
	private:
		boost::asio::io_service& io_service_;//线程管理
		bool if_connected_;
		tcp::socket socket_;//标识一个连接
		std::string recv_buf_;
		char head_[4];
		char data_[MAX_BUF_LEN];
		std::array<boost::asio::mutable_buffer, 2> message_;
		std::deque<std::array<boost::asio::mutable_buffer, 2>> write_data_q;
		u_int len_;
	};

	template<typename HandlerT, typename... Args>
	void client::async_call(const char* handler_name, HandlerT &handler, Args&&... args)
	{
		auto serial_str = make_serial(handler_name, std::forward<Args>(args)...);
		try {
			async_call(serial_str, handler);
		}
		catch (const boost::system::system_error& ec)
		{
			std::cerr << ec.what() << std::endl;
		}
	}

	template<typename HandlerT>
	inline void client::async_call(const std::string& serial_str, HandlerT& handler)
	{
		len_ = (u_int)serial_str.length();

		std::vector<boost::asio::const_buffer> message;
		message.push_back(boost::asio::buffer(&len_, 4));
		message.push_back(boost::asio::buffer(serial_str));
		socket_.send(message);

		socket_.receive(boost::asio::buffer(&len_, 4));
		recv_buf_.resize(len_);
		socket_.receive(boost::asio::buffer(&recv_buf_[0], len_));
		int res = get_result<int>(recv_buf_);
		cout << "计算结果 = " << res << endl;
	}


	//template<typename HandlerT>
	//inline void client::async_call(const std::string & serial_str, HandlerT &handler)
	//{
	//	len_ = (u_int)serial_str.length();
	//	
	//	std::vector<boost::asio::const_buffer> message;
	//	message.push_back(boost::asio::buffer(&len_, 4));
	//	message.push_back(boost::asio::buffer(serial_str));
	//	socket_.send(message);
	//	
	//	socket_.async_receive(boost::asio::buffer(&len_, 4), [&, this](boost::system::error_code ec, std::size_t length)
	//	{
	//		if (!ec)
	//		{
	//			recv_buf_.resize(len_);
	//			socket_.async_receive(boost::asio::buffer(&recv_buf_[0], len_), handler);
	//		}
	//		else
	//		{
	//			std::cerr << "async receive head failed, code: " << ec << std::endl;
	//		}
	//	});
	//}

	template <typename T>
	inline auto client::get_result(const std::string &buf)
	{
		int num_result;
		T result;
		std::stringstream ss (buf);
		boost::archive::text_iarchive ia(ss);

		ia >> num_result;
		ia >> result;
		return result;
	}

	template<typename ...Args>
	std::string client::call(const std::string &handler_name, Args && ...args)
	{
		std::string serial_str = make_serial(handler_name, std::forward<Args>(args)...);
		cout << serial_str << endl;
		std::string buf = "";

		try
		{
			buf = call(serial_str);
		}
		catch (const boost::system::system_error& ec)
		{
			std::cerr << ec.what() << std::endl;
			buf = "";
		}

		return buf;
	}
	

	std::string client::call(const std::string & serial_str)
	{
		len_ = (u_int)serial_str.length();

		std::vector<boost::asio::const_buffer> message;
		message.push_back(boost::asio::buffer(&len_, 4));
		message.push_back(boost::asio::buffer(serial_str));
		socket_.send(message);

		//socket_.receive(boost::asio::buffer(&len_, 4));
		//recv_buf_.resize(len_);
		//socket_.receive(boost::asio::buffer(&recv_buf_[0], len_));
		//std::cout << "123123" << std::endl;
		//read_head();
		return recv_buf_;
	}

	template<typename ...Args>
	static inline std::string client::make_serial(const std::string &handler_name, Args && ...args)
	{
		std::stringstream ss;
		boost::archive::text_oarchive oa(ss);
		rpc::encode(handler_name, ss, oa);
		rpc::encode(sizeof ...(args), ss,oa);
		std::initializer_list <int>{ (encode(args, ss, oa), 0)...};
		return ss.str();
	}

	template<typename Function>
	inline void client::register_handler(std::string const & name, const Function & f)
	{
		router::get().register_handler(name, f);
	}


	template<typename Function, typename Self>
	inline void client::register_handler(std::string const & name, const Function & f, Self * self)
	{
		router::get().register_handler(name, f, self);
	}

}
