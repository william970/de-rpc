#ifndef _UNIQUEIDGENERATOR_H_
#define _UNIQUEIDGENERATOR_H_

#include <set>

//Ψһid������
//����T�������޷�������
template<typename T>
class UniqueIdGenerator
{
public:
	/************************************************************************
	��  �ܣ����캯��
	��  ����
		maxId�����룬id�����ֵ��id�����ͱ������޷������ͣ�Ĭ��ֵΪ��Ӧ����ȡֵ��Χ�ڵ����ֵ
	����ֵ����
	************************************************************************/
	UniqueIdGenerator(T maxId = ~((T)0))
	{
		m_maxId = maxId;
		m_bAllDirty = false;
		m_curCleanUsableId = 0;
	}

	/************************************************************************
	��  �ܣ�����һ���µ�id
	��  ����
		id����������ɵ��µ�id
	����ֵ��
		true�����ɳɹ�
		false�������޿���id��������ʧ��
	************************************************************************/
	bool generate(T &id)
	{
		bool bRet = true;
		if (!m_bAllDirty)
		{
			id = m_curCleanUsableId;
			if (m_curCleanUsableId >= m_maxId)
				m_bAllDirty = true;
			else
				++m_curCleanUsableId;
		}

		else
		{
			if (m_setDirtyId.empty())
				bRet = false;
			else
			{
				auto it = m_setDirtyId.begin();
				id = *it;
				m_setDirtyId.erase(it);
			}
		}
		return bRet;
	}

	/************************************************************************
	��  �ܣ��黹id��id��Դ��
	��  ����
		id�����룬���黹��id
	����ֵ����
	************************************************************************/
	void back(T &id)
	{
		m_setDirtyId.insert(id);
	}

private:
	//id�����ֵ
	T m_maxId;
	//�Ƿ�����id������ģ��ࣺid�����ɹ����ɾ���idδ�����ɹ�
	bool m_bAllDirty;
	//��ǰ�ɾ��Ŀ���id
	T m_curCleanUsableId;
	//��id���ϣ����黹��id���ڴ˼���
	std::set<T> m_setDirtyId;
};

#endif