#include "../rpc/server.h"
#include <iostream>

using namespace std;

int add(int a, int b)
{
	cout << "a + b: " << a + b << endl;
	return  a + b;
}

struct messager
{
	std::string translate(const std::string& orignal)
	{
		std::string temp = orignal;
		for (auto & c : temp) c = toupper(c);
		cout << temp << endl;;
		return temp;
	}
};


int main()
{
	messager m;
	rpc::server s(9002, std::thread::hardware_concurrency());//¶Ë¿ÚºÅ£¬CPUºËÊý
	s.register_handler("add", &add);;
	s.register_handler("translate", &messager::translate, &m);
	s.run();
	
	getchar();
	return 0;
}