#pragma once
#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "common.h"
#include "router.h"

using tcp = boost::asio::ip::tcp;

namespace rpc
{
	
class connection :
	public std::enable_shared_from_this<connection>,
	private boost::noncopyable
{
private:
	tcp::socket socket_;
	char head_[4];
	char data_[MAX_BUF_LEN];
	std::array<boost::asio::mutable_buffer, 2> message_;
	boost::asio::deadline_timer timer_;
	std::size_t timeout_milli_;

public:
	explicit connection(boost::asio::io_service& io_service, std::size_t timeout_milli);
	
	tcp::socket& socket();
	void read_head();
	void read_body(std::size_t size);
	void response(const char* json_str);
	void reset_timer();
	void cancel_timer();
	void start();
	void close();
};
}

