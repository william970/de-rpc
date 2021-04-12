#include <iostream>
#include <google/protobuf/stubs/common.h>
#include "RpcController.h"
#include "Test.pb.h"
#include "IRpcClient.h"
#include "IRpcChannel.h"

//�첽���õĻص�����
void callback(testNamespace::NumResponse *pResp, google::protobuf::RpcController *pController)
{
	if (NULL == pResp)
		return;

	if (NULL != pController)
	{
		if (pController->Failed())
			return;
	}

	std::cout << "async call result: 3 + 4 = " << pResp->output() << std::endl;
}

int main()
{
	//����RpcClientʵ��
	timeval t = {3, 0};
	IRpcClient *pIClient = IRpcClient::createRpcClient(3, 50, t);
	pIClient->start();
	//����RpcChannelʵ����һ��channel����һ������
	IRpcChannel *pIChannel = IRpcChannel::createRpcChannel(pIClient, "127.0.0.1", 8888);
	//��������Ŀͻ���stubʵ��
	testNamespace::NumService::Stub numServiceStub((google::protobuf::RpcChannel *)pIChannel);
	//����request������ֵ
	testNamespace::NumRequest req;
	req.set_input1(3);
	req.set_input2(4);
	//����response
	testNamespace::NumResponse resp;
	//����RpcControllerʵ����������ʾ�ɹ�����Լ�ʧ��ԭ��
	RpcController controller;

	//done��������NULL�Խ���ͬ������
	numServiceStub.minus(&controller, &req, &resp, NULL);
	if (controller.Failed())
		std::cout << "sync call error: " << controller.ErrorText() << std::endl;
	else
		std::cout << "sync call result: 3 - 4 = " << resp.output() << std::endl;

	//done��������ص���Ϣ�Խ����첽����
	controller.Reset();
	numServiceStub.add(&controller, &req, &resp, google::protobuf::NewCallback(callback, &resp, (google::protobuf::RpcController *)(&controller)));
	
	//����RpcChannelʵ��
	//IRpcChannel::releaseRpcChannel(pIChannel);
	//����RpcClientʵ��
	//IRpcClient::releaseRpcClient(pIClient);

	while (true) {}
	
	return 0;
}