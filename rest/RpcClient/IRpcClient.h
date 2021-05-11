#ifndef _IRPCCLIENT_H_
#define _IRPCCLIENT_H_

#ifdef WIN32
#ifdef RPCCLIENT_EXPORTS
#define RPCCLIENT_DLL_EXPORTS __declspec(dllexport)
#else
#define RPCCLIENT_DLL_EXPORTS __declspec(dllimport)
#endif
#else
#define RPCCLIENT_DLL_EXPORTS
#endif

#ifdef WIN32
#include <winsock2.h>
#endif

//RpcClient�ӿ�
class RPCCLIENT_DLL_EXPORTS IRpcClient
{
public:
	virtual ~IRpcClient();

	/************************************************************************
	��  �ܣ�����RpcClientʵ��
	��  ����
		IOWorkerNum�����룬IOWorker��Worker����
		IOWorkerQueueMaxSize�����룬IOWorker���е���󳤶�
		heartbeatInterval�����룬������PING�����ķ�������
	����ֵ��IRpcClientָ��
	************************************************************************/
	static IRpcClient *createRpcClient(unsigned int IOWorkerNum, unsigned int IOWorkerQueueMaxSize, timeval heartbeatInterval);

	/************************************************************************
	��  �ܣ�����RpcClientʵ��
	��  ����
		pIRpcClient��IRpcClientָ��
	����ֵ����
	************************************************************************/
	static void releaseRpcClient(IRpcClient *pIRpcClient);

	/************************************************************************
	��  �ܣ�����RpcClient
	��  ������
	����ֵ����
	************************************************************************/
	virtual void start() = 0;

	/************************************************************************
	��  �ܣ�����RpcClient
	��  ������
	����ֵ����
	************************************************************************/
	virtual void end() = 0;
};

#endif