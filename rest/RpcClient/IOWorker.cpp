#include "IOWorker.h"

IOWorker::IOWorker(evutil_socket_t *fds, unsigned int queueMaxSize, timeval heartbeatInterval)
{
	m_notified_fd = fds[0];
	m_notify_fd = fds[1];
	m_pEvBase = NULL;
	m_heartbeatInterval = heartbeatInterval;
	m_bStarted = false;
	m_bEnded = false;
	m_queue.setMaxSize(queueMaxSize);
}

IOWorker::~IOWorker()
{
	//֪ͨIOWorker�����¼�ѭ��
	char buf[1] = { NOTIFY_IOWORKER_END };
	send(m_notify_fd, buf, 1, 0);
	//�ȴ��߳̽���
	m_thd.join();
	//�ر�socketpair�Ķ���������
	evutil_closesocket(m_notified_fd);
}

void IOWorker::start()
{
	//������IOWorker�ظ�����
	if (m_bStarted)
		return;
	m_bStarted = true;

	//�����߳�
	m_thd = std::move(boost::thread(&IOWorker::threadMain, this, m_notified_fd));
}

void IOWorker::threadMain(evutil_socket_t notified_fd)
{
	//����event_base
	event_base *pEvBase = event_base_new();
	if (NULL == pEvBase)
	{
		evutil_closesocket(notified_fd);
		return;
	}
	//��������event_base������ָ��󶨣����ô�����event_base�Զ����ٲ��ͷ���Դ
	unique_ptr<event_base, function<void(event_base *)> > ptrEvBase(pEvBase, [&notified_fd](event_base *p) {
		event_base_free(p);
		//�ر�socketpair�Ķ���������
		evutil_closesocket(notified_fd);
	});

	//����socketpair�Ķ����������ϵĿɶ��¼�
	event *pNotifiedEv = event_new(pEvBase, notified_fd, EV_READ | EV_PERSIST, notifiedCallback, this);
	if (NULL == pNotifiedEv)
		return;
	//���������¼�������ָ��󶨣����ô������¼��Զ�����
	unique_ptr<event, function<void(event *)> > ptrNotifiedEv(pNotifiedEv, event_free);

	//��socketpair�Ķ����������ϵĿɶ��¼�����Ϊδ����
	if (0 != event_add(pNotifiedEv, NULL))
		return;

	m_pEvBase = pEvBase;
	//�����¼�ѭ��
	event_base_dispatch(pEvBase);
}

void notifiedCallback(evutil_socket_t fd, short events, void *pArg)
{
	if (NULL == pArg)
		return;

	//ͨ��socketpair�Ķ�����������ȡ֪ͨ����
	char buf[1];
	recv(fd, buf, 1, 0);
	IOWorker *pWorker = (IOWorker *)pArg;

	if (NOTIFY_IOWORKER_IOTASK == buf[0])
	{
		pWorker->handleIOTask();
	}
	else if (NOTIFY_IOWORKER_END == buf[0])
		pWorker->handleEnd();
}

void IOWorker::handleIOTask()
{
	//Ϊ����������������һ���ԴӶ�����ȡ������IO����
	list<IOTask> queue;
	m_queue.takeAll(queue);
	for (auto it = queue.begin(); it != queue.end(); ++it)
	{
		if (IOTask::CONNECT == (*it).type)
		{
			connect((Conn *)(*it).pData);
		}
		else if (IOTask::DISCONNECT == (*it).type)
		{
			unsigned int *pConnId = (unsigned int *)(*it).pData;
			if (NULL != pConnId)
			{
				handleDisconnect(*pConnId);
				delete pConnId;
			}
		}
		else if (IOTask::CALL == (*it).type)
			handleCall((Call *)(*it).pData);
	}
}

void IOWorker::handleDisconnect(unsigned int connId)
{
	//��m_mapConn��������
	auto it = m_mapConn.find(connId);
	if (it == m_mapConn.end())
		return;

	Conn *pConn = it->second;
	if (NULL != pConn)
	{
		//�ͷ�����
		freeConn(pConn);
		delete pConn;
	}
	m_mapConn.erase(it);
}

void IOWorker::handleCall(Call *pCall)
{
	if (NULL == pCall)
		return;

	//��m_mapConn��������
	auto it = m_mapConn.find(pCall->connId);
	if (it == m_mapConn.end())
	{
		m_connIdGen.back(pCall->connId);
		SAFE_DELETE(pCall)
		return;
	}
	Conn *pConn = it->second;
	if (NULL == pConn)
	{
		m_connIdGen.back(pCall->connId);
		m_mapConn.erase(it);
		SAFE_DELETE(pCall)
		return;
	}
	if (!pConn->bConnected)
	{
		//��������δ�����������ѶϿ������½�����ָ��Ż�IO�������
		IOTask task;
		task.type = IOTask::CALL;
		task.pData = pCall;
		m_queue.put(task);
		return;
	}
	if (NULL == pConn->pBufEv)
	{
		SAFE_DELETE(pCall)
		return;
	}
	evbuffer *pOutBuf = bufferevent_get_output(pConn->pBufEv);
	if (NULL == pOutBuf)
	{
		SAFE_DELETE(pCall)
		return;
	}
	callId_t callId;
	if (!pConn->idGen.generate(callId))
	{
		//���ô�����Ϣ������id��Դ���㣬Ȼ�󷵻��û�����
		pCall->pController->SetFailed("call id not enough");
		rpcCallback(pCall->pClosure, pCall->sync_write_fd);
		SAFE_DELETE(pCall)
		return;
	}
	
	//���������Э��body
	ProtocolBodyRequest bodyReq;
	bodyReq.set_callid(callId);
	bodyReq.set_servicename(pCall->serviceName);
	bodyReq.set_methodindex(pCall->methodIndex);
	bodyReq.set_content(*pCall->pStrReq);
	//���л������Э��body
	string strBuf;
	if (!bodyReq.SerializeToString(&strBuf))
	{
		//���ô�����Ϣ���������л�ʧ�ܣ�Ȼ�󷵻��û�����
		pCall->pController->SetFailed("request serialized failed");
		rpcCallback(pCall->pClosure, pCall->sync_write_fd);
		SAFE_DELETE(pCall)
		return;
	}
	bodySize_t bodyLen = strBuf.size();
	unsigned char arr[HEAD_SIZE] = {0};
	arr[0] = DATA_TYPE_REQUEST;
	//����������Ҫע���ֽ��򣬷�ֹͨ�ŶԶ˽���������δ�����ֽ���
	memcpy(arr + 1, &bodyLen, HEAD_SIZE - 1);

	//�������Э��head����evbuffer
	evbuffer_add(pOutBuf, arr, HEAD_SIZE);
	//�����л���������Э��body����evbuffer
	evbuffer_add(pOutBuf, strBuf.c_str(), bodyLen);

	SAFE_DELETE(pCall->pStrReq)
	pConn->mapCall[callId] = pCall;
}

void IOWorker::handleEnd()
{
	//������IOWorker�ظ�����
	if (m_bEnded)
		return;
	m_bEnded = true;

	//�˳��¼�ѭ��
	if (NULL != m_pEvBase)
		event_base_loopexit(m_pEvBase, NULL);

	//�ͷ�����
	for (auto it = m_mapConn.begin(); it != m_mapConn.end(); ++it)
	{
		unsigned int connId = it->first;
		m_connIdGen.back(connId);
		freeConn(it->second);
		SAFE_DELETE(it->second)
	}
	m_mapConn.clear();
}

void readCallback(bufferevent *pBufEv, void *pArg)
{
	if (NULL == pArg)
		return;
	Conn *pConn = (Conn *)pArg;
	IOWorker *pWorker = pConn->pWorker;
	if (NULL != pWorker)
		pWorker->handleRead(pConn);
}

void IOWorker::handleRead(Conn *pConn)
{
	if (NULL == pConn)
		return;
	bufferevent *pBufEv = pConn->pBufEv;
	if (NULL == pBufEv)
		return;
	//��ȡbufferevent�е�����evbufferָ��
	evbuffer *pInBuf = bufferevent_get_input(pBufEv);
	if (NULL == pInBuf)
		return;

	//���������յ������ݣ���������Ϊ������Ȼ������
	pConn->bConnectionMightLost = false;

	//����TCPЭ����һ���ֽ�����Э�飬���Ա�������Ӧ�ò�Э��Ĺ��������Э�����ݵı߽�
	while (true)
	{
		if (PROTOCOL_HEAD == pConn->inState)
		{
			//��ȡ����evbuffer����
			if (evbuffer_get_length(pInBuf) < HEAD_SIZE)
				break;
			//����evbuffer�е����ݿ��ܷ�ɢ�ڲ��������ڴ�飬��������Ҫ��ȡ�ֽ����飬�������evbuffer_pullup()���С����Ի���
			//��ȡЭ��head�ֽ�����
			unsigned char *pArr = evbuffer_pullup(pInBuf, HEAD_SIZE);
			if (NULL == pArr)
				break;
			//��Э��head��2-5�ֽ�ת��Э��body����
			pConn->inBodySize = *((bodySize_t *)(pArr + 1));
			//ͨ��Э��head��1�ֽ��ж���������
			//����������ΪPONG�������������evbuffer���Ƴ�Э������
			if (DATA_TYPE_HEARTBEAT_PONG == pArr[0])
				evbuffer_drain(pInBuf, HEAD_SIZE + pConn->inBodySize);
			//����������Ϊ�������ͣ�һ�������󣩣��������ȡЭ��body
			else
			{
				evbuffer_drain(pInBuf, HEAD_SIZE);
				pConn->inState = PROTOCOL_BODY;
			}
		}
		else if (PROTOCOL_BODY == pConn->inState)
		{
			//��ȡ����evbuffer����
			if (evbuffer_get_length(pInBuf) < pConn->inBodySize)
				break;

			unsigned char *pArr = evbuffer_pullup(pInBuf, HEAD_SIZE);
			if (NULL != pArr)
			{
				//�����л���Ӧ��Э��body
				ProtocolBodyResponse bodyResp;
				if (!bodyResp.ParseFromArray(pArr, pConn->inBodySize))
				{
					evbuffer_drain(pInBuf, pConn->inBodySize);
					pConn->inState = PROTOCOL_HEAD;
					continue;
				}
				//ͨ������id�ҵ�����ָ��
				auto it = pConn->mapCall.find(bodyResp.callid());
				if (it != pConn->mapCall.end())
				{
					Call *pCall = it->second;
					if (NULL != pCall)
					{
						google::protobuf::Message *pRespMessage = pCall->pRespMessage;
						if (NULL != pRespMessage)
						{
							//�����л���Ӧ�������û�����
							if (pRespMessage->ParseFromString(bodyResp.content()))
								rpcCallback(pCall->pClosure, pCall->sync_write_fd);
						}
						pConn->idGen.back(pCall->callId);
						pConn->mapCall.erase(it);
						SAFE_DELETE(pCall)
					}
				}
			}
			evbuffer_drain(pInBuf, pConn->inBodySize);
			pConn->inState = PROTOCOL_HEAD;
		}
		else
			break;
	}
}

void eventCallback(bufferevent *pBufEv, short events, void *pArg)
{
	//events��1�������ϣ�2��eof��3����ʱ��4�����ִ���
	auto del = [](Conn *pConn) {
		if (NULL == pConn)
			return;
		if (NULL != pConn->pBufEv)
		{
			bufferevent_free(pConn->pBufEv);
			pConn->pBufEv = NULL;
		}
		for (auto it = pConn->mapCall.begin(); it != pConn->mapCall.end(); ++it)
			SAFE_DELETE(it->second)
		delete pConn;
	};

	if (NULL == pArg)
		return;
	Conn *pConn = (Conn *)pArg;
	IOWorker *pWorker = pConn->pWorker;
	if (NULL == pWorker)
	{
		del(pConn);
		return;
	}
	if (0 != (events & BEV_EVENT_CONNECTED))
	{
		pWorker->handleConnected(pConn);
		return;
	}
	if (0 != (events & BEV_EVENT_TIMEOUT))
	{
		pWorker->handleTimeout(pConn);
		return;
	}
	pWorker->connect(pConn);
}

void IOWorker::handleConnected(Conn *pConn)
{
	if (NULL == pConn)
		return;

	pConn->bConnected = true;
	//��bufferevent�����ó�ʱʱ�䣬������ʱ����PING����
	if (NULL != pConn->pBufEv && m_heartbeatInterval.tv_sec >= 0)
		bufferevent_set_timeouts(pConn->pBufEv, &m_heartbeatInterval, NULL);

	//����֮ǰ����δ�����IO����
	handleIOTask();
}

void IOWorker::handleTimeout(Conn *pConn)
{
	if (NULL == pConn)
		return;
	bufferevent *pBufEv = pConn->pBufEv;
	if (NULL == pBufEv)
		return;
	
	if (pConn->bConnected && !pConn->bConnectionMightLost)
	{
		//��ȡbufferevent�е����evbufferָ��
		evbuffer *pOutBuf = bufferevent_get_output(pBufEv);
		if (NULL != pOutBuf)
		{
			//����PING������Э��head��Э��body����Ϊ0
			unsigned char arr[HEAD_SIZE] = {0};
			arr[0] = DATA_TYPE_HEARTBEAT_PING;
			//��Э�����ݷŽ����evbuffer
			evbuffer_add(pOutBuf, arr, sizeof(arr));

			cout << "IO thread " << boost::this_thread::get_id() << " finishes sending PING heartbeat." << endl;
		}
		//��ʱ������Ϊ�����Ѷ�ʧ�����´��յ�PONG�����ظ�����õ���Ӧ���ݣ�����false
		pConn->bConnectionMightLost = true;
	}
	else
		//���ӿ͹ۻ������Ѷ�ʧ����������
		connect(pConn);

	//����ʹ��bufferevent�ϵĿɶ��¼�
	bufferevent_enable(pConn->pBufEv, EV_READ);
}

bool IOWorker::connect(Conn *pConn)
{
	if (NULL == pConn)
		return false;

	//�ͷ�����
	freeConn(pConn);

	//����bufferevent
	bufferevent *pBufEv = bufferevent_socket_new(m_pEvBase, -1, BEV_OPT_CLOSE_ON_FREE);
	if (NULL == pBufEv)
		return false;
	pConn->pBufEv = pBufEv;
	bufferevent_setcb(pBufEv, readCallback, NULL, eventCallback, pConn);
	bufferevent_enable(pBufEv, EV_READ);

	if (bufferevent_socket_connect(pBufEv, (sockaddr *)(&pConn->serverAddr), sizeof(pConn->serverAddr)) < 0)
		return false;

	//��ȡ����������
	pConn->fd = bufferevent_getfd(pBufEv);
	//���������������óɷ�����
	evutil_make_socket_nonblocking(pConn->fd);

	return true;
}

void IOWorker::freeConn(Conn *pConn)
{
	if (NULL == pConn)
		return;
	pConn->bConnected = false;
	//�ͷ�bufferevent
	if (NULL != pConn->pBufEv)
	{
		bufferevent_free(pConn->pBufEv);
		pConn->pBufEv = NULL;
	}
	//���������ϵ����е���
	for (auto it = pConn->mapCall.begin(); it != pConn->mapCall.end(); ++it)
	{
		callId_t callId = it->first;
		pConn->idGen.back(callId);
		SAFE_DELETE(it->second)
	}
	pConn->mapCall.clear();
}

unsigned int IOWorker::getBusyLevel()
{
	return m_mapConn.size();
}

Conn *IOWorker::genConn()
{
	unsigned int connId;
	if (!m_connIdGen.generate(connId))
		return NULL;
	Conn *pConn = new Conn();
	pConn->connId = connId;
	m_mapConn[connId] = pConn;
	return pConn;
}

evutil_socket_t IOWorker::getNotify_fd()
{
	return m_notify_fd;
}

void IOWorker::rpcCallback(google::protobuf::Closure *pClosure, evutil_socket_t sync_write_fd)
{
	if (NULL != pClosure)
		//�첽�ص��û�����
		pClosure->Run();
	else
	{
		//����ͬ���ȴ��е��û������߳�
		char buf[1];
		send(sync_write_fd, buf, 1, 0);
	}
}