#include "router.h"
#include<iostream>
using namespace std;

rpc::invoker_function::invoker_function(const std::function<void(token_parser&, std::string&)>& function, std::size_t size):
	function_ (function),
	param_size_ (size)
{
}

void rpc::invoker_function::operator()(token_parser & parser, std::string & result)
{
	function_(parser, result);
}

const std::size_t rpc::invoker_function::param_size() const
{
	return param_size_;
}

rpc::router & rpc::router::get()
{
	static router instance;
	return instance;
}

void rpc::router::remove_handler(std::string const & name)
{  
	this->map_invokers_.erase(name);
}

//call_back:���������ģ��Ļص�����
void rpc::router::route(const std::string &text, std::size_t length, const std::function<void(const char*)>& callback)
{
	cout << text << endl;
	assert(callback);
	token_parser parser(text);
						
	std::string result = "";
	std::string func_name = parser.get<std::string>();
	cout << func_name << endl;
	std::size_t num_parament = parser.get <std::size_t>();
	cout << num_parament << endl;

	auto it = map_invokers_.find(func_name);
	//�ڻص�����鲻��
	if (it == map_invokers_.end())
	{
		result = rpc::get_serial(result_code::EXCEPTION, "unknown function: " + func_name);
		callback(result.c_str());
		return;
	}
		
	//����������ƥ��
	if (it->second.param_size() != num_parament)
	{
		result = get_serial(result_code::EXCEPTION, "parameter number is not match" + func_name);
		callback(result.c_str());
		return;
	}
		
	//����RPC����
	it->second(parser, result);
	callback(result.c_str());
}

//call_back:���������ģ��Ļص�����
void rpc::router::route2(const std::string &text, std::size_t length, const std::function<void(std::string)>& callback)
{
	assert(callback);
	token_parser parser(text);
	cout << text << endl;

	std::string result = "";
	std::string func_name = parser.get<std::string>();
	cout << func_name << endl;
	std::size_t num_parament = parser.get <std::size_t>();
	cout << num_parament << endl;

	auto it = map_invokers_.find(func_name);
	//�ڻص�����鲻��
	if (it == map_invokers_.end())
	{
		result = rpc::get_serial(result_code::EXCEPTION, "unknown function: " + func_name);
		callback(result.c_str());
		return;
	}

	//����������ƥ��
	if (it->second.param_size() != num_parament)
	{
		result = get_serial(result_code::EXCEPTION, "parameter number is not match" + func_name);
		callback(result.c_str());
		return;
	}

	//����RPC����
	it->second(parser, result);
	callback(result.c_str());
}
