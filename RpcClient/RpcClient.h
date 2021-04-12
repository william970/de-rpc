#ifndef _RPCCLIENT_H_
#define _RPCCLIENT_H_

#include "IRpcClient.h"
#include "IOWorker.h"

//�ͻ���
class RpcClient : public IRpcClient
{
public:
	/************************************************************************
	��  �ܣ����췽��
	��  ����
		IOWorkerNum�����룬IOWorker��Worker����
		IOWorkerQueueMaxSize�����룬IOWorker���е���󳤶�
		heartbeatInterval�����룬������PING�����ķ�������
	����ֵ����
	************************************************************************/
	RpcClient(unsigned int IOWorkerNum, unsigned int IOWorkerQueueMaxSize, timeval heartbeatInterval);

	/************************************************************************
	��  �ܣ���������
	��  ������
	����ֵ����
	************************************************************************/
	~RpcClient();

	/************************************************************************
	��  �ܣ�����RpcClient
	��  ������
	����ֵ����
	************************************************************************/
	virtual void start();

	/************************************************************************
	��  �ܣ�����RpcClient
	��  ������
	����ֵ����
	************************************************************************/
	virtual void end();

	/************************************************************************
	��  �ܣ���IOWorker���е���һ��IOWorker���ڸ�IOWorker�Ϸ�������
	��  ����
		pConn����������������
	����ֵ��ѡ�е�IOWorkerָ�룬��ΪNULL����ʾ����ʧ��
	************************************************************************/
	IOWorker *schedule(Conn *&pConn);

private:
	//IOWorker�أ�map<IOWorker��Ӧ��socketpair��д��������, IOWorkerָ��>
	map<evutil_socket_t, IOWorker *> m_mapWorker;
	//RpcServer�Ƿ��Ѿ���ʼ����
	bool m_bStarted;
	//RpcServer�Ƿ��Ѿ���ʼ����
	bool m_bEnded;
};

#endif