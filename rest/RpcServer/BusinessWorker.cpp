#include "BusinessWorker.h"
#include "IOWorker.h"

BusinessWorker::BusinessWorker(unsigned int queueMaxSize)
{
	m_bEnded = false;
	m_pMapRegisteredService = NULL;
	m_queue.setMaxSize(queueMaxSize);
	m_queue.setWait(true);
}

BusinessWorker::~BusinessWorker()
{
	m_bEnded = true;
	m_queue.stop();
	//�ȴ��߳̽���
	m_thd.join();
}

void BusinessWorker::setRegisteredServices(const map<string, pair<google::protobuf::Service *, vector<const google::protobuf::MethodDescriptor *> > > *pMapRegisteredService)
{
	m_pMapRegisteredService = pMapRegisteredService;
}

void BusinessWorker::start()
{
	//�����߳�
	m_thd = std::move(boost::thread(&BusinessWorker::threadMain, this));
}

unsigned int BusinessWorker::getBusyLevel()
{
	return m_queue.getSize();
}

void BusinessWorker::threadMain()
{
	while (!m_bEnded)
	{
		//Ϊ����������������һ���ԴӶ�����ȡ������ҵ������ָ��
		list<BusinessTask *> queue;
		m_queue.takeAll(queue);

		for (auto it = queue.begin(); it != queue.end(); ++it)
		{
			cout << "business thread " << boost::this_thread::get_id() << " begins handling task..." << endl;

			BusinessTask *pTask = *it;
			//��ҵ������ָ�������ָ��󶨣�����֪ͨIOWorker�Զ�ִ��
			unique_ptr<BusinessTask, function<void(BusinessTask *)> > ptrMonitor(pTask, [](BusinessTask *pTask) {
				if (NULL == pTask)
					return;
				if (NULL == pTask->pWorker)
					return;
				//��ҵ������ָ�������ӦIOWorker��write���У��ҷŲ��ɹ���һֱ�ȴ�
				while (!pTask->pWorker->m_writeQueue.put(pTask)) {}
				//֪ͨ��ӦIOWorker
				char buf[1] = { NOTIFY_IOWORKER_WRITE };
				send(pTask->pWorker->getNotify_fd(), buf, 1, 0);
			});

			if (NULL == pTask)
				continue;
			if (NULL == pTask->pBuf)
				continue;

			if (NULL == m_pMapRegisteredService)
			{
				//�ͷ�evbuffer
				evbuffer_free(pTask->pBuf);
				pTask->pBuf = NULL;
				continue;
			}

			//��ȡ�����Э��body����
			bodySize_t len = evbuffer_get_length(pTask->pBuf);
			//�����л������Э��body
			ProtocolBodyRequest bodyReq;
			if (!bodyReq.ParseFromArray(evbuffer_pullup(pTask->pBuf, len), len))
			{
				evbuffer_free(pTask->pBuf);
				pTask->pBuf = NULL;
				continue;
			}
			//ͨ���������ҷ�����Ϣ
			auto itFind = m_pMapRegisteredService->find(bodyReq.servicename());
			if (itFind == m_pMapRegisteredService->end())
			{
				evbuffer_free(pTask->pBuf);
				pTask->pBuf = NULL;
				continue;
			}
			//��ȡ����ָ��
			google::protobuf::Service *pService = itFind->second.first;
			uint32_t index = bodyReq.methodindex();
			if (NULL == pService || index >= itFind->second.second.size())
			{
				evbuffer_free(pTask->pBuf);
				pTask->pBuf = NULL;
				continue;
			}
			//��ȡ��������ָ��
			const google::protobuf::MethodDescriptor *pMethodDescriptor = itFind->second.second[index];
			if (NULL == pMethodDescriptor)
			{
				evbuffer_free(pTask->pBuf);
				pTask->pBuf = NULL;
				continue;
			}
			//�����������Message
			google::protobuf::Message *pReq = pService->GetRequestPrototype(pMethodDescriptor).New();
			if (NULL == pReq)
			{
				evbuffer_free(pTask->pBuf);
				pTask->pBuf = NULL;
				continue;
			}
			//�������ķ������Message������ָ��󶨣����ô����ķ������Message�Զ�����
			unique_ptr<google::protobuf::Message> ptrReq(pReq);
			//������������Message
			google::protobuf::Message *pResp = pService->GetResponsePrototype(pMethodDescriptor).New();
			if (NULL == pResp)
			{
				evbuffer_free(pTask->pBuf);
				pTask->pBuf = NULL;
				continue;
			}
			//�������ķ�������Message������ָ��󶨣����ô����ķ�������Message�Զ�����
			unique_ptr<google::protobuf::Message> ptrResp(pResp);
			//������η����л�
			if (!pReq->ParseFromString(bodyReq.content()))
			{
				evbuffer_free(pTask->pBuf);
				pTask->pBuf = NULL;
				continue;
			}

			//��������
			RpcController controller;
			pService->CallMethod(pMethodDescriptor, &controller, pReq, pResp, NULL);

			//������Ӧ��Э��body
			ProtocolBodyResponse bodyResp;
			//���õ���id���˵���id�ɿͻ��˴�����ά���������ֻ��ԭ������
			bodyResp.set_callid(bodyReq.callid());
			//���л�����
			string content;
			if (!pResp->SerializeToString(&content))
			{
				evbuffer_free(pTask->pBuf);
				pTask->pBuf = NULL;
				continue;
			}
			//�������л���ĳ���
			bodyResp.set_content(content);
			//���л���Ӧ��Э��body
			string strRet;
			if (!bodyResp.SerializeToString(&strRet))
			{
				evbuffer_free(pTask->pBuf);
				pTask->pBuf = NULL;
				continue;
			}

			evbuffer_free(pTask->pBuf);
			//����evbuffer
			pTask->pBuf = evbuffer_new();
			if (NULL == pTask->pBuf)
				continue;
			//�����л������Ӧ��Э��body����evbuffer
			evbuffer_add(pTask->pBuf, strRet.c_str(), strRet.size());

			cout << "business thread " << boost::this_thread::get_id() << " finishes handling task." << endl;
		}
	}
}