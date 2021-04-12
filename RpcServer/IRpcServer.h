#ifndef _IRPCSERVER_H_
#define _IRPCSERVER_H_

#ifdef WIN32
#ifdef RPCSERVER_EXPORTS
#define RPCSERVER_DLL_EXPORTS __declspec(dllexport)
#else
#define RPCSERVER_DLL_EXPORTS __declspec(dllimport)
#endif
#else
#define RPCSERVER_DLL_EXPORTS
#endif

#include <string>
#include <google/protobuf/service.h>

//RpcServer�ӿ�
class RPCSERVER_DLL_EXPORTS IRpcServer
{
public:
	virtual ~IRpcServer();

	/************************************************************************
	��  �ܣ�����RpcServerʵ��
	��  ����
		ip�����룬������������ip��ַ
		port�����룬�����������Ķ˿ں�
		IOWorkerNum�����룬IOWorker��Worker����
		IOWorkerAcceptQueueMaxSize�����룬IOWorker�ӹ����Ӷ��е���󳤶�
		IOWorkerWriteQueueMaxSize�����룬IOWorker���Ͷ��е���󳤶�
		businessWorkerNum�����룬ҵ��Worker��Worker����
		businessWorkerQueueMaxSize�����룬ҵ��Workerҵ��������е���󳤶�
	����ֵ��IRpcServerָ��
	************************************************************************/
	static IRpcServer *createRpcServer(const std::string &ip, int port, 
		unsigned int IOWorkerNum, unsigned int IOWorkerAcceptQueueMaxSize, unsigned int IOWorkerCompleteQueueMaxSize, 
		unsigned int businessWorkerNum, unsigned int businessWorkerQueueMaxSize);

	/************************************************************************
	��  �ܣ�����RpcServerʵ��
	��  ����
		pIRpcServer��IRpcServerָ��
	����ֵ����
	************************************************************************/
	static void releaseRpcServer(IRpcServer *pIRpcServer);

	/************************************************************************
	��  �ܣ�ע�����
	��  ����
		pService������ָ��
	����ֵ����
	************************************************************************/
	virtual void registerService(google::protobuf::Service *pService) = 0;

	/************************************************************************
	��  �ܣ�����RpcServer
	��  ������
	����ֵ����
	************************************************************************/
	virtual void start() = 0;

	/************************************************************************
	��  �ܣ�����RpcServer
	��  ������
	����ֵ����
	************************************************************************/
	virtual void end() = 0;
};

#endif