#include "rpc/client.h"
#include <iostream>
#include <string>
#include <thread>
#include <windows.h>
using namespace std;

boost::asio::io_service io_service;
rpc::client c_sync(io_service); //����һ��client
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
		cout << "add����� : " << c_async.get_result <int>(c_async.get_async_data()) << endl;
	else
		cerr << "ballback failed, code: " << ec << endl;
}

void async()
{
	if (c_async.connect("127.0.0.1", 9002, false))
	{
		c_async.async_call("add", callback, 1, 2);
	}
	
}

void sync()
{
	std::string buf;
	buf.reserve(1024);

	if (c_sync.connect("127.0.0.1", 9002,false))
	{
		c_sync.async_call("add", callback,1,2);
	}
}



int main()
{

	thread t(io_thread);
	t.detach();
	std::string buf;
	buf.reserve(1024);

	if (c_sync.connect("127.0.0.1", 5000,false))
	{
		cout << "123123" << endl;
		c_sync.async_call("add", callback, 1, 2);
		c_sync.async_call("del", callback, 11, 1);
	}

	//sync();
	//Sleep(1000);
	//sync();
	//Sleep(1000);
	//sync();
	//async();
	getchar();
	return 0;
}