#include "IOWorker.h"

IOWorker::IOWorker(evutil_socket_t *fds, unsigned int acceptQueueMaxSize, unsigned int completeQueueMaxSize, const vector<BusinessWorker *> *pVecBusinessWorker)
{
	m_notified_fd = fds[0];
	m_notify_fd = fds[1];
	m_pVecBusinessWorker = pVecBusinessWorker;
	m_pEvBase = NULL;
	m_connNum = 0;
	m_bStarted = false;
	m_bEnded = false;
	m_acceptQueue.setMaxSize(acceptQueueMaxSize);
	m_writeQueue.setMaxSize(completeQueueMaxSize);
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

	if (NOTIFY_IOWORKER_ACCEPT == buf[0])
		pWorker->handleAccept();
	else if (NOTIFY_IOWORKER_WRITE == buf[0])
		pWorker->handleWrite();
	else if (NOTIFY_IOWORKER_END == buf[0])
		pWorker->handleEnd();
}

void IOWorker::handleAccept()
{
	if (NULL == m_pEvBase)
		return;

	//Ϊ����������������һ���Դ�accept������ȡ����������������
	list<evutil_socket_t> queue;
	m_acceptQueue.takeAll(queue);

	for (auto it = queue.begin(); it != queue.end(); ++it)
	{
		//����bufferevent
		bufferevent *pBufEv = bufferevent_socket_new(m_pEvBase, *it, BEV_OPT_CLOSE_ON_FREE);
		if (NULL == pBufEv)
			continue;

		//����������Ϣ
		Conn *pConn = new Conn();
		pConn->fd = *it;
		pConn->pBufEv = pBufEv;
		pConn->pWorker = this;

		m_mapConn[*it] = pConn;
		++m_connNum;

		//����bufferevent�Ļص�����
		bufferevent_setcb(pBufEv, readCallback, NULL, eventCallback, pConn);
		//ʹ��bufferevent�ϵĿɶ��¼�
		bufferevent_enable(pBufEv, EV_READ);

		cout << "IO thread " << boost::this_thread::get_id() << " begins serving connnection " << pConn->fd << "..." <<endl;
	}
}

void IOWorker::handleWrite()
{
	//Ϊ����������������һ���Դ�write������ȡ������ҵ������ָ��
	list<BusinessTask *> queue;
	m_writeQueue.takeAll(queue);

	for (auto it = queue.begin(); it != queue.end(); ++it)
	{
		BusinessTask *pTask = *it;
		Conn *pConn = NULL;
		//��ҵ������ָ�������ָ��󶨣����Զ��������ٺ���Դ�ͷ�
		unique_ptr<BusinessTask, function<void(BusinessTask *)> > ptrTask(pTask, [this, &pConn](BusinessTask *pTask) {
			if (NULL == pTask)
				return;
			if (NULL != pTask->pBuf)
			{
				evbuffer_free(pTask->pBuf);
				pTask->pBuf = NULL;
			}
			SAFE_DELETE(pTask)
			checkToFreeConn(pConn);
		});

		if (NULL == pTask)
			continue;
		//������������
		auto itFind = m_mapConn.find(pTask->conn_fd);
		if (itFind == m_mapConn.end())
			continue;
		//��ȡ������Ϣָ��
		pConn = itFind->second;
		if (NULL == pConn)
			continue;
		if (NULL == pTask->pBuf || !pConn->bValid || NULL == pConn->pBufEv)
		{
			--(pConn->todoCount);
			continue;
		}
		//��ȡbufferevent�ϵ����evbufferָ��
		evbuffer *pOutBuf = bufferevent_get_output(pConn->pBufEv);
		if (NULL == pOutBuf)
		{
			--(pConn->todoCount);
			continue;
		}
		//���Э��head
		unsigned char arr[HEAD_SIZE] = {0};
		arr[0] = DATA_TYPE_RESPONSE;
		bodySize_t bodyLen = evbuffer_get_length(pTask->pBuf);
		memcpy(arr + 1, &bodyLen, HEAD_SIZE - 1);
		//��Э��ͷ�Ž����evbuffer
		evbuffer_add(pOutBuf, arr, HEAD_SIZE);
		//�����л����Э��body�ƶ������evbuffer
		evbuffer_remove_buffer(pTask->pBuf, pOutBuf, evbuffer_get_length(pTask->pBuf));
		--(pConn->todoCount);
	}
}

void IOWorker::handleEnd()
{
	//������IOWorker�ظ�����
	if (m_bEnded)
		return;
	m_bEnded = true;

	//�����������˳��¼�ѭ��
	if (m_mapConn.empty() && NULL != m_pEvBase)
		event_base_loopexit(m_pEvBase, NULL);
}

void readCallback(bufferevent *pBufEv, void *pArg)
{
	if (NULL == pBufEv || NULL == pArg)
		return;
	IOWorker *pWorker = ((Conn *)pArg)->pWorker;
	if (NULL != pWorker)
		pWorker->handleRead((Conn *)pArg);
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
			//����������ΪPING��������ֱ�ӻظ��ͻ���PONG����
			if (DATA_TYPE_HEARTBEAT_PING == pArr[0])
			{
				//��ȡbufferevent�е����evbufferָ��
				evbuffer *pOutBuf = bufferevent_get_output(pBufEv);
				if (NULL != pOutBuf)
				{
					//����PONG������Э��head��Э��body����Ϊ0
					unsigned char arr[HEAD_SIZE] = {0};
					arr[0] = DATA_TYPE_HEARTBEAT_PONG;
					//��Э�����ݷŽ����evbuffer
					evbuffer_add(pOutBuf, arr, sizeof(arr));

					cout << "IO thread " << boost::this_thread::get_id() << " finishes replying PONG heartbeat." << endl;
				}
				//������evbuffer���Ƴ�Э������
				evbuffer_drain(pInBuf, HEAD_SIZE + pConn->inBodySize);
			}
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

			//����ҵ������
			BusinessTask *pTask = new BusinessTask();
			pTask->pWorker = this;
			pTask->conn_fd = pConn->fd;
			//����evbuffer�����ƶ�����evbuffer������
			pTask->pBuf = evbuffer_new();
			if (NULL != pTask->pBuf)
			{
				//����ҵ��Worker��������
				BusinessWorker *pSelectedWorker = schedule();
				if (NULL != pSelectedWorker)
				{
					evbuffer_remove_buffer(pInBuf, pTask->pBuf, pConn->inBodySize);
					//��ҵ������ָ�����ѡ�е�ҵ��Worker�еĶ���
					if (pSelectedWorker->m_queue.put(pTask))
					{
						++(pConn->todoCount);
						evbuffer_drain(pInBuf, pConn->inBodySize);
						pConn->inState = PROTOCOL_HEAD;
						continue;
					}
					//���·�֧����ʾδ�ɹ���ҵ�������ɷ���ҵ��Worker�������������ڴ桢�ͷ���Դ
					SAFE_DELETE(pTask)
				}
				else
				{
					evbuffer_free(pTask->pBuf);
					SAFE_DELETE(pTask)
				}
			}
			else
				SAFE_DELETE(pTask)
			
			evbuffer_drain(pInBuf, pConn->inBodySize);
			pConn->inState = PROTOCOL_HEAD;
		}
		else
			break;
	}
}

void eventCallback(struct bufferevent *pBufEv, short events, void *pArg)
{
	//events��1�������ϣ�2��eof��3����ʱ��4�����ִ���
	if (NULL == pBufEv || NULL == pArg)
		return;
	IOWorker *pWorker = ((Conn *)pArg)->pWorker;
	if (NULL != pWorker)
		pWorker->handleEvent((Conn *)pArg);
}

void IOWorker::handleEvent(Conn *pConn)
{
	if (NULL == pConn)
		return;

	pConn->bValid = false;
	checkToFreeConn(pConn);
}

BusinessWorker *IOWorker::schedule()
{
	BusinessWorker *pSelectedWorker = NULL;
	unsigned int minBusyLevel;

	auto it = m_pVecBusinessWorker->begin();
	for ( ; it != m_pVecBusinessWorker->end(); ++it)
	{
		BusinessWorker *pWorker = *it;
		if (NULL != pWorker)
		{
			pSelectedWorker = pWorker;
			//��ȡҵ��Worker�ķ�æ�̶ȣ�Խ��æ��ҵ��WorkerԽ���ױ�ѡ��
			minBusyLevel = pSelectedWorker->getBusyLevel();
			break;
		}
	}
	if (NULL == pSelectedWorker || minBusyLevel <= 0)
		return pSelectedWorker;

	for (++it; it != m_pVecBusinessWorker->end(); ++it)
	{
		BusinessWorker *pWorker = *it;
		if (NULL != pWorker)
		{
			unsigned int busyLevel = pWorker->getBusyLevel();
			if (busyLevel < minBusyLevel)
			{
				pSelectedWorker = pWorker;
				minBusyLevel = busyLevel;
				//��ҵ��Worker��ȫ���У���������ѡ��ҵ��Worker
				if (minBusyLevel <= 0)
					break;
			}
		}
	}
	return pSelectedWorker;
}

bool IOWorker::checkToFreeConn(Conn *pConn)
{
	int i;
	unique_ptr<int, function<void(int *)> > ptrMonitor(&i, [this](int *p) {
		//�����������˳��¼�ѭ��
		if (m_bEnded && m_mapConn.empty() && NULL != m_pEvBase)
			event_base_loopexit(m_pEvBase, NULL);
	});

	if (NULL == pConn)
		return false;

	//��������û����δ��������󣨲�����PING������������������
	if (!pConn->bValid && pConn->todoCount <= 0 && NULL != pConn->pBufEv)
	{
		auto it = m_mapConn.find(pConn->fd);
		if (it != m_mapConn.end())
			m_mapConn.erase(it);

		bufferevent_free(pConn->pBufEv);
		SAFE_DELETE(pConn)
		return true;
	}
	return false;
}

unsigned int IOWorker::getBusyLevel()
{
	return m_connNum;
}

evutil_socket_t IOWorker::getNotify_fd()
{
	return m_notify_fd;
}