#ifndef _RPCCLIENTGLOBAL_H_
#define _RPCCLIENTGLOBAL_H_

#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <boost/thread/thread.hpp>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include "SyncQueue.h"
#include "UniqueIdGenerator.h"
#include "Global.h"
#include "ProtocolBody.pb.h"
#ifdef WIN32
#include <winsock2.h>
#endif

using namespace std;

//��IOWorker���͵�֪ͨ����
enum NOTIFY_IOWORKER_TYPE
{
	//IO����֪ͨIOWorker����IO�����������Ͱ������ӡ��Ͽ����ӡ�����
	NOTIFY_IOWORKER_IOTASK = 0, 
	//������֪ͨIOWorker���Ž���
	NOTIFY_IOWORKER_END = 1
};

//�ͻ��˵���
struct Call
{
	//����id
	callId_t callId;
	//����id
	unsigned int connId;
	//����ͬ�����õ�socketpairд��������
	evutil_socket_t sync_write_fd;
	//����������л�����ڴ�ָ��
	string *pStrReq;
	//��Ӧ��Messageָ��
	google::protobuf::Message *pRespMessage;
	//�����첽�ص���Closureָ��
	google::protobuf::Closure *pClosure;
	//���ڱ�ʾ�ɹ���񼰴���ԭ���RpcControllerָ��
	google::protobuf::RpcController *pController;
	//���õķ�������
	string serviceName;
	//���õķ��������±�
	uint32_t methodIndex;

	Call()
	{
		pStrReq = NULL;
		pRespMessage = NULL;
		pClosure = NULL;
		pController = NULL;
	}
};

class IOWorker;
//�ͻ�������
struct Conn
{
	//��ǰ�����ϵ���������״̬
	PROTOCOL_PART inState;
	//��ǰ�����ϵ��������ݵ�body����
	bodySize_t inBodySize;

	//�Ƿ����ӳɹ�
	bool bConnected;
	//�Ƿ�������Ϊʧȥ����
	bool bConnectionMightLost;
	//����id
	unsigned int connId;
	//����������
	evutil_socket_t fd;
	//���Ӷ�Ӧ��buffer�¼�
	bufferevent *pBufEv;
	//Ψһ����id������
	UniqueIdGenerator<callId_t> idGen;
	//���е���
	map<callId_t, Call *> mapCall;
	//����������IOWorker
	IOWorker *pWorker;
	//�����������ĵ�ַ
	sockaddr_in serverAddr;

	Conn()
	{
		inState = PROTOCOL_HEAD;
		inBodySize = 0;
		bConnected = false;
		bConnectionMightLost = false;
		pBufEv = NULL;
		pWorker = NULL;
	}
};

//IO����
struct IOTask
{
	//IO�������Ͷ���
	enum TYPE
	{
		//����
		CONNECT = 0, 
		//�Ͽ�����
		DISCONNECT = 1, 
		//����
		CALL = 2
	};
	//IO��������
	IOTask::TYPE type;
	//��������ָ�룬�������type���в�ͬ������ת��
	void *pData;

	IOTask()
	{
		pData = NULL;
	}
};

#endif