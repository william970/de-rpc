#ifndef _IRPCCHANNEL_H_
#define _IRPCCHANNEL_H_

#include <string>
#include <google/protobuf/service.h>
#include "IRpcClient.h"

#ifdef WIN32
#ifdef RPCCLIENT_EXPORTS
#define RPCCLIENT_DLL_EXPORTS __declspec(dllexport)
#else
#define RPCCLIENT_DLL_EXPORTS __declspec(dllimport)
#endif
#else
#define RPCCLIENT_DLL_EXPORTS
#endif

//RpcChannel�ӿ�
class RPCCLIENT_DLL_EXPORTS IRpcChannel : public google::protobuf::RpcChannel
{
public:
	virtual ~IRpcChannel();

	/************************************************************************
	��  �ܣ�����RpcChannelʵ��
	��  ����
		pIClient�����룬IRpcClientָ��
		ip�����룬������������ip��ַ
		port�����룬�����������Ķ˿ں�
	����ֵ��IRpcChannelָ��
	************************************************************************/
	static IRpcChannel *createRpcChannel(IRpcClient *pIClient, const std::string &ip, int port);

	/************************************************************************
	��  �ܣ�����RpcChannelʵ��
	��  ����
		pIRpcChannel��pIRpcChannelָ��
	����ֵ����
	************************************************************************/
	static void releaseRpcChannel(IRpcChannel *pIRpcChannel);

	/************************************************************************
	��  �ܣ����÷���
	��  ������google::protobuf::RpcChannel CallMethod()����
	����ֵ����
	************************************************************************/
	virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
		google::protobuf::RpcController* controller,
		const google::protobuf::Message* request,
		google::protobuf::Message* response,
		google::protobuf::Closure* done) = 0;
};

#endif