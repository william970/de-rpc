#pragma once
#include <boost/noncopyable.hpp>
#include <utility>
#include <string>
#include <map>
#include <mutex>

#include "../rpc/token_parser.h"
#include "../rpc/common.h"
#include "MiddleConnection.h"
#include "../rpc/utility.hpp"
#include "../rpc/function_traits.hpp"


class MiddleRouter : boost::noncopyable
{
private:
	std::map<std::string, int> fun_conid_map;//������<->����ID
	std::map<int, std::shared_ptr<MiddleConnection>> conid_con_map;//����ID<->��������
	std::map<int, int> msgid_conid_map;//��ϢID->����ID
	std::mutex mtx; //������
	int msg_id = 0;
public:
	static MiddleRouter& get_instance();
	void route(const std::string &text, std::size_t length,int connid);
	void addCon(int id, std::shared_ptr<MiddleConnection> conn);
};

