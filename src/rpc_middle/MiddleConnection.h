#pragma once
#include <iostream>
#include <memory>
#include <queue>

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "../rpc/common.h"
#include <boost/noncopyable.hpp>

using tcp = boost::asio::ip::tcp;

class MiddleConnection	:
	public std::enable_shared_from_this<MiddleConnection>,
		private boost::noncopyable
	{
	private:
		tcp::socket socket_;
		int conid_;
		char head_[4];
		char read_data_[MAX_BUF_LEN];
		std::array<boost::asio::mutable_buffer, 2> message_;
		std::deque<std::array<boost::asio::mutable_buffer, 2>> write_data_q;
		boost::asio::deadline_timer timer_;
		std::size_t timeout_milli_;

	public:
		explicit MiddleConnection(int conid,boost::asio::io_service& io_service, std::size_t timeout_milli);

		tcp::socket& socket();
		void read_head();
		void read_body(std::size_t size);
		void response(const char* json_str);
		void do_write();
		void reset_timer();
		void cancel_timer();
		void start();
		void close();
};

