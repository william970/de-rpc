#ifndef _RPCCHANNEL_H_
#define _RPCCHANNEL_H_

#include "RpcClientGlobal.h"
#include "IRpcChannel.h"
#include "RpcClient.h"

//Rpcͨ����һ��ͨ������һ������
class RpcChannel : public IRpcChannel
{
public:
	/************************************************************************
	��  �ܣ����췽��
	��  ����
		pClient�����룬RpcClientָ��
		ip�����룬������������ip��ַ
		port�����룬�����������Ķ˿ں�
	����ֵ����
	************************************************************************/
	RpcChannel(RpcClient *pClient, const string &ip, int port);

	/************************************************************************
	��  �ܣ���������
	��  ������
	����ֵ����
	************************************************************************/
	~RpcChannel();

	/************************************************************************
	��  �ܣ����÷���
	��  ������google::protobuf::RpcChannel CallMethod()����
	����ֵ����
	************************************************************************/
	virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
		google::protobuf::RpcController* controller,
		const google::protobuf::Message* request,
		google::protobuf::Message* response,
		google::protobuf::Closure* done);

private:
	//RpcClientָ��
	RpcClient *m_pClient;
	//IOWorkerָ��
	IOWorker *m_pWorker;
	//�Ƿ��������
	bool m_bConnected;
	//����ͬ�����õ�socketpair�Ƿ���Ч
	bool m_bSyncValid;
	//����id
	unsigned int m_connId;
	//������������ip��ַ
	string m_ip;
	//�����������Ķ˿ں�
	int m_port;
	//����ͬ�����õ�socketpair����������
	evutil_socket_t m_sync_read_fd;
	//����ͬ�����õ�socketpairд��������
	evutil_socket_t m_sync_write_fd;
};

#endif