#ifndef _RPCSERVERDEFINE_H_
#define _RPCSERVERDEFINE_H_

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <boost/thread/thread.hpp>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include "SyncQueue.h"
#include "Global.h"
#ifdef WIN32
#include <winsock2.h>
#endif

using namespace std;

//��RpcServer���͵�֪ͨ����
enum NOTIFY_SERVER_TYPE
{
	//������֪ͨRpcServer�����¼�ѭ��
	NOTIFY_SERVER_END = 0
};

//��IOWorker���͵�֪ͨ����
enum NOTIFY_IOWORKER_TYPE
{
	//�ӹ����ӣ���accept�߳�accept���ӣ�Ȼ��֪ͨĳ��IOWorker�ӹ�����
	NOTIFY_IOWORKER_ACCEPT = 0, 
	//�������ݣ���ҵ��Worker������Ӧ���ݣ�Ȼ��֪ͨ��ӦIOWorker������Ӧ����
	NOTIFY_IOWORKER_WRITE = 1, 
	//������֪ͨIOWorker���Ž���
	NOTIFY_IOWORKER_END = 2
};

class IOWorker;
//���������
struct Conn
{
	//��ǰ�����ϵ���������״̬
	PROTOCOL_PART inState;
	//��ǰ�����ϵ��������ݵ�body����
	bodySize_t inBodySize;

	//����������
	evutil_socket_t fd;
	//���Ӷ�Ӧ��buffer�¼�
	bufferevent *pBufEv;
	//����������IOWorker
	IOWorker *pWorker;
	//��������δ��������󣨲�����PING����������
	unsigned int todoCount;
	//�����Ƿ���Ч
	bool bValid;

	Conn()
	{
		inState = PROTOCOL_HEAD;
		inBodySize = 0;
		pBufEv = NULL;
		pWorker = NULL;
		todoCount = 0;
		bValid = true;
	}
};

//ҵ������
struct BusinessTask
{
	//����ҵ�������IOWorker
	IOWorker *pWorker;
	//����ҵ�����������������
	evutil_socket_t conn_fd;
	//����ҵ����������ӵ���ʱevbuffer
	evbuffer *pBuf;

	BusinessTask()
	{
		pWorker = NULL;
		pBuf = NULL;
	}
};

#endif