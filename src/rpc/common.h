#pragma once
#include <atomic>

//resultҪô�ǻ������ͣ�Ҫô�ǽṹ�壻������ɹ�ʱ��codeΪ0, ����������޷������͵ģ���resultΪ��; 
//������з���ֵ�ģ���resultΪ����ֵ��response_msg�����л�Ϊһ����׼��json�����ط����ͻ��ˡ� 
//������Ϣ�ĸ�ʽ��length+body����4���ֽڵĳ�����Ϣ������ָʾ����ĳ��ȣ��Ӱ�����ɡ� 
template<typename T>
struct response_msg
{
	int code;
	T result; 
};
 
enum result_code
{
	OK = 0,
	FAIL = 1,
	EXCEPTION = 2,
};

const int MAX_BUF_LEN = 8192;