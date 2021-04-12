#include "RpcServer.h"

IRpcServer::~IRpcServer() {}

IRpcServer *IRpcServer::createRpcServer(const string &ip, int port, 
	unsigned int IOWorkerNum, unsigned int IOWorkerAcceptQueueMaxSize, unsigned int IOWorkerCompleteQueueMaxSize, 
	unsigned int businessWorkerNum, unsigned int businessWorkerQueueMaxSize)
{
	return new RpcServer(ip, port, IOWorkerNum, IOWorkerAcceptQueueMaxSize, IOWorkerCompleteQueueMaxSize, businessWorkerNum, businessWorkerQueueMaxSize);
}

void IRpcServer::releaseRpcServer(IRpcServer *pIRpcServer)
{
	SAFE_DELETE(pIRpcServer);
}

RpcServer::RpcServer(const string &ip, int port, 
	unsigned int IOWorkerNum, unsigned int IOWorkerAcceptQueueMaxSize, unsigned int IOWorkerCompleteQueueMaxSize, 
	unsigned int businessWorkerNum, unsigned int businessWorkerQueueMaxSize)
{
	m_ip = ip;
	m_port = port;
	m_pEvBase = NULL;
	m_bStarted = false;
	m_bEnded = false;

#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	//����socketpair�����ⲿ֪ͨserver����
	evutil_socket_t notif_fds[2];
	evutil_socketpair(AF_INET, SOCK_STREAM, 0, notif_fds);
	m_notified_fd = notif_fds[0];
	m_notify_fd = notif_fds[1];

	//����ҵ��Worker��
	for (unsigned int i = 0; i < businessWorkerNum; ++i)
		m_vecBusinessWorker.push_back(new BusinessWorker(businessWorkerQueueMaxSize));

	//����IOWorker��
	for (unsigned int i = 0; i < IOWorkerNum; ++i)
	{
		evutil_socket_t fds[2];
		if (evutil_socketpair(AF_INET, SOCK_STREAM, 0, fds) < 0)
			continue;
		//��socketpair��2�������������óɷ�����
		evutil_make_socket_nonblocking(fds[0]);
		evutil_make_socket_nonblocking(fds[1]);
		m_mapIOWorker[fds[1]] = new IOWorker(fds, IOWorkerAcceptQueueMaxSize, IOWorkerCompleteQueueMaxSize, &m_vecBusinessWorker);
	}
}

RpcServer::~RpcServer()
{
	end();
}

void RpcServer::registerService(google::protobuf::Service *pService)
{
	if (NULL == pService)
		return;
	//��ȡ��������ָ��
	const google::protobuf::ServiceDescriptor *pServiceDescriptor = pService->GetDescriptor();
	if (NULL == pServiceDescriptor)
		return;

	pair<google::protobuf::Service *, vector<const google::protobuf::MethodDescriptor *> > &v = m_mapRegisteredService[pServiceDescriptor->full_name()];
	//��ŷ���ָ��
	v.first = pService;
	//��ŷ����Ӧ�ķ�������ָ��
	for (int i = 0; i < pServiceDescriptor->method_count(); ++i)
	{
		const google::protobuf::MethodDescriptor *pMethodDescriptor = pServiceDescriptor->method(i);
		if (NULL == pMethodDescriptor)
			continue;
		v.second.emplace_back(pMethodDescriptor);
	}
}

void RpcServer::start()
{
	//������RpcServer�ظ�����
	if (m_bStarted)
		return;
	m_bStarted = true;

	//����event_base
	event_base *pEvBase = event_base_new();
	if (NULL == pEvBase)
		return;
	//��������event_base������ָ��󶨣����ô�����event_base�Զ�����
	unique_ptr<event_base, function<void(event_base *)> > ptrEvBase(pEvBase, event_base_free);

	//����������������ַ
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	if (0 == evutil_inet_pton(AF_INET, m_ip.c_str(), &(serverAddr.sin_addr)))
		return;
	serverAddr.sin_port = htons(m_port);
	//��libevent listen��accept��accept�󽫻ص�acceptCallback()����
	evconnlistener *pListener = evconnlistener_new_bind(pEvBase, acceptCallback, this, LEV_OPT_CLOSE_ON_FREE, 128, (sockaddr *)(&serverAddr), sizeof(serverAddr));
	if (NULL == pListener)
		return;
	//��������evconnlistener������ָ��󶨣����ô�����evconnlistener�Զ�����
	unique_ptr<evconnlistener, function<void(evconnlistener *)> > ptrListener(pListener, evconnlistener_free);

	//����IOWorker��
	for (auto it = m_mapIOWorker.begin(); it != m_mapIOWorker.end(); ++it)
	{
		if (NULL != it->second)
			it->second->start();
	}

	//����ҵ��Worker��
	for (auto it = m_vecBusinessWorker.begin(); it != m_vecBusinessWorker.end(); ++it)
	{
		if (NULL != *it)
		{
			//��ע������з����֪ҵ��Worker
			(*it)->setRegisteredServices(&m_mapRegisteredService);
			(*it)->start();
		}
	}

	//����socketpair�Ķ����������ϵĿɶ��¼�
	event *pNotifiedEv = event_new(pEvBase, m_notified_fd, EV_READ | EV_PERSIST, serverNotifiedCallback, this);
	if (NULL == pNotifiedEv)
		return;
	//���������¼�������ָ��󶨣����ô������¼��Զ�����
	unique_ptr<event, function<void(event *)> > ptrNotifiedEv(pNotifiedEv, event_free);
	//��socketpair�Ķ����������ϵĿɶ��¼�����Ϊδ����
	event_add(pNotifiedEv, NULL);

	m_pEvBase = pEvBase;
	//�����¼�ѭ��
	event_base_dispatch(pEvBase);
}

void RpcServer::end()
{
	//������RpcServer�ظ�����
	if (m_bEnded)
		return;
	m_bEnded = true;

	//����IOWorker��
	for (auto it = m_mapIOWorker.begin(); it != m_mapIOWorker.end(); ++it)
	{
		if (NULL != it->second)
			SAFE_DELETE(it->second)
			//�ر�socketpair��д��������
			evutil_closesocket(it->first);
	}

	//����ҵ��Worker��
	for (auto it = m_vecBusinessWorker.begin(); it != m_vecBusinessWorker.end(); ++it)
	{
		if (NULL != *it)
			SAFE_DELETE(*it)
	}

	//֪ͨRpcServer�����¼�ѭ��
	char buf[1] = { NOTIFY_SERVER_END };
	send(m_notify_fd, buf, 1, 0);
}

void serverNotifiedCallback(evutil_socket_t fd, short events, void *pArg)
{
	if (NULL == pArg)
		return;

	//ͨ��socketpair�Ķ�����������ȡ֪ͨ����
	char buf[1];
	recv(fd, buf, 1, 0);

	if (NOTIFY_SERVER_END == buf[0])
		((RpcServer *)pArg)->handleEnd();
}

void RpcServer::handleEnd()
{
	//�ر�socketpair�Ķ���д��������
	evutil_closesocket(m_notified_fd);
	evutil_closesocket(m_notify_fd);

	//�˳��¼�ѭ��
	if (NULL != m_pEvBase)
		event_base_loopexit(m_pEvBase, NULL);
}

void acceptCallback(evconnlistener *pListener, evutil_socket_t fd, struct sockaddr *pAddr, int socklen, void *pArg)
{
	if (NULL != pArg)
		((RpcServer *)pArg)->handleAccept(fd);
}

void RpcServer::handleAccept(evutil_socket_t fd)
{
	//��libevent accept���������������óɷ�����
	evutil_make_socket_nonblocking(fd);

	//����IOWorker�ӹ�����
	IOWorker *pSelectedWorker = schedule();
	if (NULL != pSelectedWorker)
	{
		//��ȡsocketpair��д��������
		evutil_socket_t selectedNotify_fd = pSelectedWorker->getNotify_fd();
		//����������������ѡ��IOWorker��accept����
		pSelectedWorker->m_acceptQueue.put(fd);
		//֪ͨѡ�е�IOWorker
		char buf[1] = { NOTIFY_IOWORKER_ACCEPT };
		send(selectedNotify_fd, buf, 1, 0);
	}
}

IOWorker *RpcServer::schedule()
{
	IOWorker *pSelectedWorker = NULL;
	unsigned int minBusyLevel;

	auto it = m_mapIOWorker.begin();
	for ( ; it != m_mapIOWorker.end(); ++it)
	{
		IOWorker *pWorker = it->second;
		if (NULL != pWorker)
		{
			pSelectedWorker = pWorker;
			//��ȡIOWorker�ķ�æ�̶ȣ�Խ��æ��IOWorkerԽ���ױ�ѡ��
			minBusyLevel = pSelectedWorker->getBusyLevel();
			break;
		}
	}
	if (NULL == pSelectedWorker || minBusyLevel <= 0)
		return pSelectedWorker;

	for (++it; it != m_mapIOWorker.end(); ++it)
	{
		IOWorker *pWorker = it->second;
		if (NULL != pWorker)
		{
			unsigned int busyLevel = pWorker->getBusyLevel();
			if (busyLevel < minBusyLevel)
			{
				pSelectedWorker = pWorker;
				minBusyLevel = busyLevel;
				//��IOWorker��ȫ���У���������ѡ��IOWorker
				if (minBusyLevel <= 0)
					break;
			}
		}
	}
	return pSelectedWorker;
}