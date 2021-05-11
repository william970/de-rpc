#include "MiddleRouter.h"
using namespace std;

MiddleRouter& MiddleRouter::get_instance()
{
	static MiddleRouter instance;
	return instance;
}

//call_back:���������ģ��Ļص�����
void MiddleRouter::route(const std::string &text, std::size_t length,int connid)
{
	rpc::token_parser parser(text);

	std::string result = "";
	std::string func_name = parser.get<std::string>();
	std::size_t num_parament = parser.get <std::size_t>();

	//cout << "text = " << text << endl;

	if (func_name == "register") {
		std::string real_func_name = parser.get<std::string>();//ע��ĺ�����
		fun_conid_map[real_func_name] = connid;
		conid_con_map[connid]->response("10000000 register success");
		cout << "ע���� : " << real_func_name << endl;
	}
	else if (func_name == "response") {
		//�ظ�
		std::string id = parser.get<std::string>();
		std::string result = parser.get<std::string>();
		cout << "ִ�еĽ����" << result << endl;
		int msg_id = atoi(id.c_str());
		int target_id = msgid_conid_map[msg_id];
		std::shared_ptr<MiddleConnection> target_server_con = conid_con_map[target_id];
		msgid_conid_map.erase(msg_id);
		target_server_con->response(result.c_str());
	}
	else {
		cout << "�пͻ������� : " << func_name << endl;
		//���������ĸ�����������
		if (fun_conid_map.find(func_name) == fun_conid_map.end()) {
			cout << "û�д�ID" << endl;
			conid_con_map[connid]->response("no this function");
		}
		else {
			int target_server_conid = fun_conid_map[func_name];
			std::shared_ptr<MiddleConnection> target_server_con = conid_con_map[target_server_conid];
			msgid_conid_map[msg_id] = connid;
			std::string s = std::to_string(msg_id++);
			std::string s1 = "";
			while (8 - s1.length() - s.length() > 0) {
				s1 += '0';
			}
			s1 += s;
			s1 += text;
			//s1 id + ��������text
			target_server_con->response(s1.c_str());
		}
	}
}

void MiddleRouter::addCon(int id, std::shared_ptr<MiddleConnection> conn) {
	std::unique_lock<std::mutex> l(mtx);
	conid_con_map[id] = conn;
}