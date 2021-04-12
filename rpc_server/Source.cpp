#include "../rpc/client.h"
#include <iostream>
#include <string>
#include <thread>
#include <windows.h>
using namespace std;

boost::asio::io_service io_service;
rpc::client c_sync(io_service); //¿ªÆôÒ»¸öclient
rpc::client c_async(io_service);

void io_thread()
{
	while (1)
	{
		io_service.run();
	}
}
void callback(boost::system::error_code ec, std::size_t)
{
	if (!ec)
		cout << c_async.get_result <int>(c_async.get_async_data()) << endl;
	else
		cerr << "ballback failed, code: " << ec << endl;
}

void async()
{
	if (c_async.connect("127.0.0.1", 9002))
	{
		c_async.async_call("add", callback, 1, 2);
	}

}

void sync()
{
	std::string buf;
	buf.reserve(1024);

	if (c_sync.connect("127.0.0.1", 5000))
	{
		buf = c_sync.call("translate", std::string("helloworld"));
	}
}

int add(int a, int b)
{
	cout << "a + b: " << a + b << endl;
	return  a + b;
}

int main()
{
	thread t(io_thread);
	std::string buf;
	buf.reserve(1024);
	c_sync.register_handler("add", &add);
	c_sync.connect("127.0.0.1", 5000);
	buf = c_sync.call("register", std::string("add"));
	cout << buf << endl;
	getchar();
	return 0;
}