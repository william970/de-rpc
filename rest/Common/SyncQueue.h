#ifndef _SYNCQUEUE_H_
#define _SYNCQUEUE_H_

#include <list>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

template<typename T>
class SyncQueue
{
public:
	/************************************************************************
	��  �ܣ����캯��
	��  ����
		bWait�����룬�Ƿ���������ʽ���ʶ���
			true����������ʽ���ʶ���
			false���Է�������ʽ���ʶ��У�����Ĭ��ֵ
	����ֵ����
	************************************************************************/
	SyncQueue(bool bWait = false)
	{
		m_maxSize = 0;
		m_bWait = bWait;
		m_bStopped = false;
	}

	/************************************************************************
	��  �ܣ����캯��
	��  ����
		maxSize�����룬���е���󳤶�
		bWait�����룬�Ƿ���������ʽ���ʶ���
			true����������ʽ���ʶ���
			false���Է�������ʽ���ʶ��У�����Ĭ��ֵ
	����ֵ����
	************************************************************************/
	SyncQueue(unsigned int maxSize, bool bWait = false)
	{
		m_maxSize = maxSize;
		m_bWait = bWait;
		m_bStopped = false;
	}

	/************************************************************************
	��  �ܣ�����з�һ����ֵ
	��  ����
		x�����룬��������е���ֵ
	����ֵ��
		true���ųɹ�
		false����ʧ��
	************************************************************************/
	bool put(T &x)
	{
		return add(x, true);
	}

	/************************************************************************
	��  �ܣ�����з�һ����ֵ
	��  ����
		x�����룬��������е���ֵ
	����ֵ��
		true���ųɹ�
		false����ʧ��
	************************************************************************/
	bool put(T &&x)
	{
		return add(x, false);
	}

	/************************************************************************
	��  �ܣ��Ӷ���ȡֵ
	��  ����
		x����������Ӷ���ȡ����ֵ
	����ֵ��
		true��ȡ�ɹ�
		false��ȡʧ��
	************************************************************************/
	bool take(T &x)
	{
		//�����з��ʷ�ʽΪ���������Ҷ���Ϊ�գ�����������
		if (!bWait && m_queue.empty())
			return false;

		//���²�����������
		boost::unique_lock<boost::mutex> lock(m_mutex);
		if (bWait)
		{
			//���ⲿδ���ù�stop()ʱ��ͬ���ȴ����еķǿ�����
			while (!m_bStopped && m_queue.empty())
				m_notEmpty.wait(lock);
			//������ʼʱ�жϹ������ڶ��̻߳����£����ڱ��������ж�
			if (m_queue.empty())
				return false;
		}
		else
		{
			//������ʼʱ�жϹ������ڶ��̻߳����£����ڱ��������ж�
			if (m_queue.empty())
				return false;
		}

		//�Ӷ���ȡֵ
		x = m_queue.front();
		m_queue.pop_front();
		if (m_bWait)
			//���д�ʱ���������ѵȴ����еķ����������߳�
			m_notFull.notify_one();

		return true;
	}

	/************************************************************************
	��  �ܣ��Ӷ���ȡ����ֵ
	��  ����
		x����������Ӷ���ȡ��������ֵ
	����ֵ��
		true��ȡ�ɹ�
		false��ȡʧ��
	************************************************************************/
	bool takeAll(std::list<T> &queue)
	{
		if (!m_bWait && m_queue.empty())
			return false;

		boost::unique_lock<boost::mutex> lock(m_mutex);
		if (m_bWait)
		{
			while (!m_bStopped && m_queue.empty())
				m_notEmpty.wait(lock);
			if (m_queue.empty())
				return false;
		}
		else
		{
			if (m_queue.empty())
				return false;
		}

		queue = m_queue;
		m_queue.clear();
		if (m_bWait)
			m_notFull.notify_one();

		return true;
	}

	/************************************************************************
	��  �ܣ������з��ʷ�ʽΪ����ʱ���������еȴ��ŵ��̣߳��˺����ֻ���Է������ķ�ʽ������
	��  ������
	����ֵ����
	************************************************************************/
	void stop()
	{
		if (m_bWait)
		{
			m_bStopped = true;
			//�������еȴ����еķ����������߳�
			m_notFull.notify_all();
			//�������еȴ����еķǿ��������߳�
			m_notEmpty.notify_all();
		}
	}

	/************************************************************************
	��  �ܣ����ö�����󳤶�
	��  ����
		maxSize�����룬������󳤶�
	����ֵ����
	************************************************************************/
	void setMaxSize(unsigned int maxSize)
	{
		//���²�����������
		boost::unique_lock<boost::mutex> lock(m_mutex);
		m_maxSize = maxSize;
	}

	/************************************************************************
	��  �ܣ����ö�����������������ʷ�ʽ
	��  ����
		bWait�����룬�Ƿ���������ʽ���ʶ���
			true����������ʽ���ʶ���
			false���Է�������ʽ���ʶ���
	����ֵ����
	************************************************************************/
	void setWait(bool bWait)
	{
		//���²�����������
		boost::unique_lock<boost::mutex> lock(m_mutex);
		m_bWait = bWait;
	}

	/************************************************************************
	��  �ܣ���ȡ���е�ǰ����
	��  ������
	����ֵ�����е�ǰ����
	************************************************************************/
	unsigned int getSize()
	{
		return m_queue.size();
	}

private:
	//����з�һ�����ųɹ�����true����ʧ�ܷ���false
	/************************************************************************
	��  �ܣ�����з�һ����ֵ����ֵ
	��  ����
		x�����룬��������е���ֵ
		bLeftValue�����룬����ֵ������ֵ
			true������ֵ
			false������ֵ
	����ֵ��
		true���ųɹ�
		false����ʧ��
	************************************************************************/
	bool add(T &x, bool bLeftValue)
	{
		//�����з��ʷ�ʽΪ���������Ҷ���Ϊ��������������
		if (!m_bWait && m_maxSize <= m_queue.size())
			return false;

		//���²�����������
		boost::unique_lock<boost::mutex> lock(m_mutex);
		if (m_bWait)
		{
			//���ⲿδ���ù�stop()ʱ��ͬ���ȴ����еķ�������
			while (!m_bStopped && m_maxSize <= m_queue.size())
				m_notFull.wait(lock);
			//������ʼʱ�жϹ������ڶ��̻߳����£����ڱ��������ж�
			if (m_maxSize <= m_queue.size())
				return false;
		}
		else
		{
			//������ʼʱ�жϹ������ڶ��̻߳����£����ڱ��������ж�
			if (m_maxSize <= m_queue.size())
				return false;
		}

		//����з�һ����ֵ����ֵ
		if (bLeftValue)
			m_queue.push_back(x);
		else
			m_queue.push_back(move(x));
		if (m_bWait)
			//���д�ʱ�ǿգ����ѵȴ����еķǿ��������߳�
			m_notEmpty.notify_one();

		return true;
	}

	//����
	std::list<T> m_queue;
	//������󳤶�
	unsigned int m_maxSize;
	//�Ƿ���������ʽ���ʶ��У����ӿն���ȡ���ݣ����������з�����ʱ���Ƿ������������߳�ֱ�����зǿգ�����з���
	bool m_bWait;
	//��stop()������
	bool m_bStopped;
	//�����������ڶ��зǿա����з�������������������������Ҫ��д�ٽ����Ĳ���
	boost::mutex m_mutex;
	//���зǿ���������
	boost::condition_variable m_notEmpty;
	//���з�����������
	boost::condition_variable m_notFull;
};

#endif