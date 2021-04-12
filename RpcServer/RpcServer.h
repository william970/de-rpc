#ifndef _RPCSERVER_H_
#define _RPCSERVER_H_

#include "RpcServerGlobal.h"
#include "IOWorker.h"
#include "BusinessWorker.h"
#include "IRpcServer.h"

/************************************************************************
����˹��ܽṹ������
1��accept�߳�accept���ӣ�
2��accept�̵߳���IOWorker���е�IOWorker�ӹ����ӣ�ÿ��IOWorker�ɽӹܶ�����ӣ�
3��IOWorker����ҵ��Worker���е�ҵ��Worker�������󣬵õ�����Ӧ������ӦIOWorker���ؿͻ��ˡ�
************************************************************************/

/************************************************************************
��  �ܣ�libevent �������ɶ���ص��˺���
��  ������libevent event_callback_fn����
����ֵ����
************************************************************************/
void serverNotifiedCallback(evutil_socket_t fd, short events, void *pArg);

/************************************************************************
��  �ܣ�libevent accept���Ӻ�ص��˺���
��  ������libevent evconnlistener_cb����
����ֵ����
************************************************************************/
void acceptCallback(evconnlistener *pListener, evutil_socket_t fd, struct sockaddr *pAddr, int socklen, void *pArg);

//������
class RpcServer : public IRpcServer
{
public:
	/************************************************************************
	��  �ܣ����췽��
	��  ����
		ip�����룬������������ip��ַ
		port�����룬�����������Ķ˿ں�
		IOWorkerNum�����룬IOWorker��Worker����
		IOWorkerAcceptQueueMaxSize�����룬IOWorker�ӹ����Ӷ��е���󳤶�
		IOWorkerWriteQueueMaxSize�����룬IOWorker���Ͷ��е���󳤶�
		businessWorkerNum�����룬ҵ��Worker��Worker����
		businessWorkerQueueMaxSize�����룬ҵ��Workerҵ��������е���󳤶�
	����ֵ����
	************************************************************************/
	RpcServer(const string &ip, int port, 
		unsigned int IOWorkerNum, unsigned int IOWorkerAcceptQueueMaxSize, unsigned int IOWorkerCompleteQueueMaxSize, 
		unsigned int businessWorkerNum, unsigned int businessWorkerQueueMaxSize);

	/************************************************************************
	��  �ܣ���������
	��  ������
	����ֵ����
	************************************************************************/
	~RpcServer();

	/************************************************************************
	��  �ܣ�ע�����
	��  ����
		pService������ָ��
	����ֵ����
	************************************************************************/
	virtual void registerService(google::protobuf::Service *pService);

	/************************************************************************
	��  �ܣ�����RpcServer
	��  ������
	����ֵ����
	************************************************************************/
	virtual void start();

	/************************************************************************
	��  �ܣ�����RpcServer
	��  ������
	����ֵ����
	************************************************************************/
	virtual void end();

	/************************************************************************
	��  �ܣ���������¼�ѭ����֪ͨ
	��  ������
	����ֵ����
	************************************************************************/
	void handleEnd();

	/************************************************************************
	��  �ܣ�����libevent accept������
	��  ����
		fd�����룬libevent accept������������
	����ֵ����
	************************************************************************/
	void handleAccept(evutil_socket_t fd);

private:
	/************************************************************************
	��  �ܣ���IOWorker���е���һ��IOWorker�����ӹ�����
	��  ������
	����ֵ��ѡ�е�IOWorkerָ�룬��ΪNULL����ʾ����ʧ��
	************************************************************************/
	IOWorker *schedule();

	//������������ip��ַ
	string m_ip;
	//�����������Ķ˿ں�
	int m_port;
	//�ⲿע������з���map<��������, pair<����ָ��, vector<��������ָ��> > >
	map<string, pair<google::protobuf::Service *, vector<const google::protobuf::MethodDescriptor *> > > m_mapRegisteredService;
	//IOWorker�أ�map<IOWorker��Ӧ��socketpair��д��������, IOWorkerָ��>
	map<evutil_socket_t, IOWorker *> m_mapIOWorker;
	//ҵ��Worker�أ�vector<ҵ��Workerָ��>
	vector<BusinessWorker *> m_vecBusinessWorker;
	//event_baseָ��
	event_base *m_pEvBase;
	//socketpair�Ķ��������������ڽ���server������֪ͨ
	evutil_socket_t m_notified_fd;
	//socketpair��д��������������֪ͨserver����
	evutil_socket_t m_notify_fd;
	//RpcServer�Ƿ��Ѿ���ʼ����
	bool m_bStarted;
	//RpcServer�Ƿ��Ѿ���ʼ����
	bool m_bEnded;
};

#endif