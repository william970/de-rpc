#include "rpc/server.h"
#include <iostream>
#include"rpc_middle/MiddleServer.h"

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
	MiddleServer s(5000, std::thread::hardware_concurrency());//�˿ںţ�CPU����
	s.run();
	
	getchar();
	return 0;
}