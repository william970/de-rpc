#include <boost/thread/thread.hpp>
#include "Test.pb.h"
#include "IRpcServer.h"
#include "RpcController.h"

//����ʵ����
class NumServiceImpl : public testNamespace::NumService
{
public:
	virtual void add(::google::protobuf::RpcController* controller,
		const ::testNamespace::NumRequest* request,
		::testNamespace::NumResponse* response,
		::google::protobuf::Closure* done)
	{
		response->set_output(request->input1() + request->input2());
	}

	virtual void minus(::google::protobuf::RpcController* controller,
		const ::testNamespace::NumRequest* request,
		::testNamespace::NumResponse* response,
		::google::protobuf::Closure* done)
	{
		response->set_output(request->input1() - request->input2());
	}
};

//����RpcServerʵ�����ԣ�ʵ�ʿ����в�Ҫ��ô������Ϊserver��������������ֹͣ
void releaseServer(IRpcServer *pIServer)
{
	//�߳����߼��룬��server������������
	boost::this_thread::sleep_for(boost::chrono::seconds(3));
	//����RpcServerʵ��
	IRpcServer::releaseRpcServer(pIServer);
}

int main()
{
	//����RpcServerʵ��
	IRpcServer *pIServer = IRpcServer::createRpcServer("127.0.0.1", 8888, 3, 50, 50, 3, 50);
	//��������ʵ����ʵ��
	NumServiceImpl numService;
	//ע�����
	pIServer->registerService(&numService);

	//����RpcServerʵ�����ԣ�ʵ�ʿ����в�Ҫ��ô������Ϊserver��������������ֹͣ
	//boost::thread thd(releaseServer, pIServer);

	//����RpcServer
	pIServer->start();

	//�ȴ��߳��˳�
	//thd.join();

	return 0;
}