#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "RpcController.h"
#include "ProtocolBody.pb.h"

#define SAFE_DELETE(p) { if (NULL != (p)) { delete (p); (p) = NULL; } }

/************************************************************************
Rpcͨ��Э��ṹ��head + body
1��head������Ϊ5�ֽ�
	��1����1�ֽڣ��������ͣ�������DATA_TYPE�Ķ���
	��2����2-5�ֽڣ�body�ĳ���
2��body
	��1������������Ϊ������PING��PONG����body����Ϊ0�ֽ�
	��2������������Ϊ�������Ӧ��body���ݼ�ProtocolBody.proto
************************************************************************/

//Э��head����
#define HEAD_SIZE 5
//Э��body��������
typedef uint32_t bodySize_t;
//����Id����
typedef uint32_t callId_t;

//Э�鲿λ
enum PROTOCOL_PART
{
	//head
	PROTOCOL_HEAD = 0, 
	//body
	PROTOCOL_BODY = 1
};

//Э����������
enum DATA_TYPE
{
	//�����ɿͻ��˷��͸�������
	DATA_TYPE_REQUEST = 0, 
	//��Ӧ���ɷ������ظ����ͻ���
	DATA_TYPE_RESPONSE = 1, 
	//PING�������ɿͻ��˷��͸�������
	DATA_TYPE_HEARTBEAT_PING = 2, 
	//PONG�������ɷ������ظ����ͻ���
	DATA_TYPE_HEARTBEAT_PONG = 3
};

#endif