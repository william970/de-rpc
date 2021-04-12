#pragma once
#include <thread>
#include "connection.h"
#include "io_service_pool.h"
#include "router.h"

namespace rpc	
{
	using boost::asio::ip::tcp;

	class server
	{
	public:
		server(short port, size_t size, size_t timeout_milli = 0);
		~server();
		void run();

		template<typename Function>
		void register_handler(std::string const & name, const Function& f);
		
		template<typename Function, typename Self>
		void register_handler(std::string const & name, const Function& f, Self* self);
		
		void remove_handler(std::string const& name);
	private:
		void do_accept();
	private:
		io_service_pool io_service_pool_;
		tcp::acceptor acceptor_;
		std::shared_ptr<connection> conn_;
		std::shared_ptr<std::thread> thd_;
		std::size_t timeout_milli_;
	};

	template<typename Function>
	inline void server::register_handler(std::string const & name, const Function & f)
	{
		router::get().register_handler(name, f);
	}

	
	template<typename Function, typename Self>
	inline void server::register_handler(std::string const & name, const Function & f, Self * self)
	{
		router::get().register_handler(name, f, self);
	}
	
}
