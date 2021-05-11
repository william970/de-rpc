#include "RpcChannel.h"

IRpcChannel::~IRpcChannel() {}

IRpcChannel *IRpcChannel::createRpcChannel(IRpcClient *pIClient, const string &ip, int port)
{
	return new ::RpcChannel((RpcClient *)pIClient, ip, port);
}

void IRpcChannel::releaseRpcChannel(IRpcChannel *pIRpcChannel)
{
	SAFE_DELETE(pIRpcChannel)
}

RpcChannel::RpcChannel(RpcClient *pClient, const string &ip, int port)
{
	m_pClient = pClient;
	m_pWorker = NULL;
	m_ip = ip;
	m_port = port;
	m_bConnected = false;
	m_bSyncValid = false;

	//��������ͬ�����õ�socketpair
	evutil_socket_t fds[2];
	if (evutil_socketpair(AF_INET, SOCK_STREAM, 0, fds) >= 0)
	{
		//socketpair��2��������Ĭ������
		m_sync_read_fd = fds[0];
		m_sync_write_fd = fds[1];
		m_bSyncValid = true;
	}
}

RpcChannel::~RpcChannel()
{
	//�ر�����ͬ�����õ�socketpair��������
	if (m_bSyncValid)
	{
		evutil_closesocket(m_sync_read_fd);
		evutil_closesocket(m_sync_write_fd);
	}

	//֪ͨIOWorker�Ͽ�����
	if (m_bConnected && NULL != m_pWorker)
	{
		IOTask task;
		task.type = IOTask::DISCONNECT;
		task.pData = new unsigned int(m_connId);

		m_pWorker->m_queue.put(task);
		char buf[1] = { NOTIFY_IOWORKER_IOTASK };
		send(m_pWorker->getNotify_fd(), buf, 1, 0);
	}
}

void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
	google::protobuf::RpcController* controller,
	const google::protobuf::Message* request,
	google::protobuf::Message* response,
	google::protobuf::Closure* done)
{
	if (NULL != controller)
		controller->Reset();

	if (NULL == method || NULL == request || NULL == response || !m_bSyncValid)
	{
		if (NULL != controller)
			controller->SetFailed("method == NULL or request == NULL or response == NULL or sync socketpair created failed");
		return;
	}

	if (NULL == m_pClient)
	{
		if (NULL != controller)
			controller->SetFailed("RpcClient object does not exist");
		return;
	}

	//δ���������
	if (!m_bConnected)
	{
		//��RpcClient����IOWorker�ز���ѡ�е�IOWorker�Ϸ�������
		Conn *pConn = NULL;
		m_pWorker = m_pClient->schedule(pConn);
		if (NULL == m_pWorker)
		{
			//����ʧ��
			if (NULL != controller)
				controller->SetFailed("connection refused");
			return;
		}
		m_connId = pConn->connId;
		//֪ͨIOWorker����
		pConn->pWorker = m_pWorker;
		//������������ַ
		pConn->serverAddr.sin_family = AF_INET;
		if (0 == evutil_inet_pton(AF_INET, m_ip.c_str(), &(pConn->serverAddr.sin_addr)))
			return;
		pConn->serverAddr.sin_port = htons(m_port);

		IOTask task;
		task.type = IOTask::CONNECT;
		task.pData = pConn;

		m_pWorker->m_queue.put(task);
		char buf[1] = { NOTIFY_IOWORKER_IOTASK };
		send(m_pWorker->getNotify_fd(), buf, 1, 0);
		m_bConnected = true;
	}
	//���л��������
	string *pStr = new string();
	if (!request->SerializeToString(pStr))
	{
		if (NULL != controller)
			controller->SetFailed("request serialized failed");
		SAFE_DELETE(pStr)
		return;
	}
	//֪ͨIOWorker����
	Call *pCall = new Call();
	pCall->connId = m_connId;
	pCall->sync_write_fd = m_sync_write_fd;
	pCall->pStrReq = pStr;
	pCall->pRespMessage = response;
	pCall->pClosure = done;
	pCall->pController = controller;
	pCall->serviceName = method->service()->full_name();
	pCall->methodIndex = method->index();

	IOTask task;
	task.type = IOTask::CALL;
	task.pData = pCall;

	m_pWorker->m_queue.put(task);
	char buf[1] = { NOTIFY_IOWORKER_IOTASK };
	send(m_pWorker->getNotify_fd(), buf, 1, 0);

	//��Ϊͬ�����ã�ͨ���������Ķ���������ʹ���߳�������ֱ�����÷���ʱ��д��������д��
	//��Ϊ�첽���ã�ֱ�ӷ��أ����÷���ʱ����ûص�����
	if (NULL == done)
	{
		char buf[1];
		recv(m_sync_read_fd, buf, 1, 0);
	}
}