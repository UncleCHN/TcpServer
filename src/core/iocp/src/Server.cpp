#include"core/iocp/include/Server.h"
#include"core/iocp/include/ServerInfo.h"
#include"core/iocp/include/ServerTaskMgr.h"
#include"core/iocp/include/ServerTask.h"


extern LPFN_CONNECTEX               lpfnConnectEx;
extern LPFN_DISCONNECTEX            lpfnDisConnectEx;
extern LPFN_ACCEPTEX                lpfnAcceptEx;               // AcceptEx �� GetAcceptExSockaddrs �ĺ���ָ�룬���ڵ�����������չ����
extern LPFN_GETACCEPTEXSOCKADDRS    lpfnGetAcceptExSockAddrs;

Server::Server()
	: nThreads(0)
	, listenCtx(NULL)
	, ioThreads(NULL)
	, useDefTaskProcesser(false)
	, isCheckHeartBeat(false)
	, isStop(true)
	, isReusedSocket(false)
	, childLifeDelayTime(11000)             //10s
	, iocp(NULL)                                
	, localListenPort(DEFAULT_PORT)
	, exitIoThreads(0)
	, serverTaskCount(1)
	, useSingleSendTaskProcesser(true)
	, dePacketor(nullptr)
{
	LoadSocketLib();
	GetLocalIPEx(localIP); 
	serverTaskMgr = new ServerTaskMgr(this);

	SetUseSingleSendTaskProcesser(useSingleSendTaskProcesser);
	UniqueID::GetInstance();
}

Server::~Server(void)
{
	Stop();

	RELEASE(serverTaskMgr);
	RELEASE_REF(dePacketor);

	UnloadSocketLib();
}

//	����������
bool Server::Start()
{
	if (!isStop)
		return true;

	// ��ʼ��IOCP
	if (false == InitializeIOCP())
		return false;

	serverTaskMgr->Start();

	isStop = false;

	return true;
}


//	��ʼ����ϵͳ�˳���Ϣ���˳���ɶ˿ں��߳���Դ
void Server::Stop()
{
	if (isStop)
		return;

	InterlockedExchange(&exitIoThreads, TRUE);

	for (int i = 0; i < nThreads; i++) {
		// ֪ͨ���е���ɶ˿ڲ����˳�
		PostQueuedCompletionStatus(iocp, 0, (DWORD)EXIT_CODE, NULL);
	}

	// �ر�IOCP���
	RELEASE_HANDLE(iocp);

	serverTaskMgr->Stop();


	//
	for each(auto item in msgCoroutineMap)
	{
		RELEASE(item.second);
	}

	isStop = true;
}


// ��ʼ��WinSock 2.2
bool Server::LoadSocketLib()
{
	WORD wVersionRequested;
	WSADATA wsaData;    // ��ṹ�����ڽ���Windows Socket�Ľṹ��Ϣ��
	int nResult;

	wVersionRequested = MAKEWORD(2, 2);   // ����2.2�汾��WinSock��
	nResult = WSAStartup(wVersionRequested, &wsaData);

	if (NO_ERROR != nResult)
	{
		return false;          // ����ֵΪ���ʱ���Ǳ�ʾ�ɹ�����WSAStartup
	}

	LoadIocpExFuncs();
	return true;
}


void Server::SetServerTaskProcess(int serverTaskIdx, TaskProcesser* taskProcesser)
{
	if (!isStop)
		return;

	serverTaskMgr->CreateServerTaskProcess(serverTaskIdx, taskProcesser);
}

Timer* Server::CreateTimer(int serverTaskIdx, TimerCallBack timerCB, void* param, int durationMS)
{
	if (isStop)
		return nullptr;

	ServerTask* serverTask = serverTaskMgr->GetServerTask(serverTaskIdx);
	TaskProcesser* taskProcesser = serverTask->GetMainTaskProcesser();
	Timer* timer = new Timer(taskProcesser, durationMS, timerCB, param);
	return timer;
}


void Server::SetUnPackCallBack(UnPackCallBack _unPackCallBack, void* param)
{
	if (isStop && dePacketor != NULL)
		dePacketor->SetUnPackCallBack(_unPackCallBack, param);
}

inline void Server::SetDePacketor(DePacketor* _dePacketor)
{
	if (!isStop || _dePacketor == nullptr)
		return;

	if (_dePacketor == dePacketor)
		return;

	if (dePacketor != nullptr)
		DEBUG("%s:%I64u,��������Ϊ:%d", dePacketor->GetName().c_str(), (uint64_t)dePacketor, dePacketor->getRef() - 1);

	RELEASE_REF(dePacketor);
	dePacketor = _dePacketor;
	dePacketor->addRef();

	if (dePacketor != nullptr)
		DEBUG("%s:%I64u,��������Ϊ:%d", dePacketor->GetName().c_str(), (uint64_t)dePacketor, dePacketor->getRef());
}

void Server::SetServerMachineID(int machineID)
{
	UniqueID::GetInstance().set_workid(machineID);
}

void Server::SetUseSingleSendTaskProcesser(bool isUse)
{
	if (!isStop)
		return;

	useSingleSendTaskProcesser = isUse;
	
	if (serverTaskMgr != NULL)
		serverTaskMgr->SetUseSingleSendTaskProcesser(isUse);
}

Socket* Server::GetSocket(uint64_t socketID, int searchInServerTaskIdx)
{
	if (searchInServerTaskIdx != -1)
	{
		ServerTask* serverTask = serverTaskMgr->GetServerTask(searchInServerTaskIdx);
		return serverTask->GetSocket(socketID);
	}
	
	ServerTask* serverTask;
	Socket* socket;
	for (int i = 0; i < serverTaskMgr->GetServerTaskCount(); i++)
	{
		serverTask = serverTaskMgr->GetServerTask(i);
		socket = serverTask->GetSocket(socketID);	
		if (socket != nullptr)
			return socket;
	}

	return nullptr;
}

void Server::SetUseCoroutine(int serverTaskIdx, bool isUse)
{
	if (!isStop)
		return;

	ServerTask* serverTask;
	if (serverTaskIdx != -1)
	{
		serverTask = serverTaskMgr->GetServerTask(serverTaskIdx);
		serverTask->SetUseCoroutine(isUse);
		return;
	}

	for (int i = 0; i < serverTaskMgr->GetServerTaskCount(); i++)
	{
		serverTask = serverTaskMgr->GetServerTask(i);
		serverTask->SetUseCoroutine(isUse);
	}
}

LPVOID Server::GetMainFiberHandle(int serverTaskIdx)
{
	ServerTask* serverTask;
	LPVOID mainFiberHandle;

	if (serverTaskIdx >= 0 && serverTaskIdx < serverTaskMgr->GetServerTaskCount())
	{
		serverTask = serverTaskMgr->GetServerTask(serverTaskIdx);
		mainFiberHandle = serverTask->GetMainFiberHandle();
		return mainFiberHandle;
	}

	return nullptr;
}

void Server::SaveMsgCoroutine(BaseMsgCoroutine* co)
{
	LPVOID coHandle = co->GetHandle();
	uint64_t key = (uint64_t)coHandle;
	msgCoroutineMap[key] = co;
}

void Server::DelMsgCoroutine(BaseMsgCoroutine* co)
{
	LPVOID coHandle = co->GetHandle();
	msgCoroutineMap.erase((uint64_t)coHandle);
	RELEASE(co);
}


BaseMsgCoroutine* Server::GetMsgCoroutine(LPVOID coHandle)
{
	auto it = msgCoroutineMap.find((uint64_t)coHandle);
	if (it != msgCoroutineMap.end())
		return it->second;
	return nullptr;
}

void Server::MsgCoroutineYield()
{
	LPVOID mainCoHandle = GetMainFiberHandle();
	BaseMsgCoroutine::CoYield(mainCoHandle);
}

int Server::GetTotalSocketCount()
{
	ServerTask* serverTask;
	SocketHashMap* socketMap;
	int totalCount = 0;
	for (int i = 0; i < serverTaskMgr->GetServerTaskCount(); i++)
	{
		serverTask = serverTaskMgr->GetServerTask(i);
		socketMap = serverTask->GetSocketMap();
		totalCount += socketMap->size();
	}

	return totalCount;
}

int Server::StartListener()
{
	Start();	
	return serverTaskMgr->StartListener();
}

int Server::ConnectServer(ServerInfo* serverInfo, int delay)
{
	Start();
	return serverTaskMgr->ConnectServer(serverInfo, delay);
}


//�������߳�:ΪIOCP�������Ĺ������߳�
//Ҳ����ÿ����ɶ˿��ϳ�����������ݰ����ͽ�֮ȡ�������д�����߳�
DWORD WINAPI Server::IoThread(LPVOID pParam)
{
	THREADPARAMS_WORKER* param = (THREADPARAMS_WORKER*)pParam;
	Server* server = (Server*)param->server;
	int nThreadNo = (int)param->nThreadNo;

	////LOG4CPLUS_INFO(log.GetInst(), "�������߳�������ID:" << nThreadNo);

	OVERLAPPED      *pOverlapped = NULL;
	Socket   *socketCtx = NULL;
	DWORD            dwBytesTransfered = 0;

	// ѭ����������ֱ�����յ�Shutdown��ϢΪֹ
	while (server->exitIoThreads != TRUE)
	{
		BOOL bReturn = GetQueuedCompletionStatus(
			server->iocp,
			&dwBytesTransfered,
			(PULONG_PTR)&socketCtx,
			&pOverlapped,
			INFINITE);

		// ����յ������˳���־����ֱ���˳�
		if (EXIT_CODE == socketCtx)
		{
			break;
		}
		// �ж��Ƿ�����˴���
		else if (!bReturn)
		{
			// ��ȡ����Ĳ���
			Packet* packet = CONTAINING_RECORD(pOverlapped, Packet, overlapped);

			//LOG4CPLUS_ERROR(log.GetInst(), "GetQueuedCompletionStatus()����������! ���������ر�����SOCKET:"<<socketCtx);
			//LOG4CPLUS_ERROR(log.GetInst(), "�������������"<<socketCtx<<"�������ر���ɵ�.�������:" << WSAGetLastError());

			packet->serverTask->PostErrorTask(packet);
			continue;
		}
		else
		{
			// ��ȡ����Ĳ���
			Packet* packet = CONTAINING_RECORD(pOverlapped, Packet, overlapped);
			ServerTask* serverTask = packet->serverTask;


			// �ж��Ƿ��пͻ��˶Ͽ���
			if (dwBytesTransfered == 0 &&
				packet->postIoType != POST_IO_CONNECT &&
				packet->postIoType != POST_IO_ACCEPT)
			{
				serverTask->PostErrorTask(packet);
			}
			else
			{
				packet->transferedBytes = dwBytesTransfered;

				switch (packet->postIoType)
				{
				case POST_IO_CONNECT:
					serverTask->PostConnectedServerTask(packet);
					break;

				case POST_IO_ACCEPT:
					server->DoAccept(packet);
					break;

				case POST_IO_RECV:
					serverTask->PostRecvedTask(packet);
					break;

				case POST_IO_SEND:
					serverTask->PostSendedTask(packet);
					break;
				}
			}
		}

	}

	//LOG4CPLUS_TRACE(log.GetInst(), "�������߳�" <<nThreadNo<< "���˳�.");
	// �ͷ��̲߳���
	RELEASE(pParam);

	return 0;
}

// ��ʼ����ɶ˿�
bool Server::InitializeIOCP()
{
	// ������һ����ɶ˿�
	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	if (NULL == iocp)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "������ɶ˿�ʧ�ܣ��������:"<<WSAGetLastError());
		return false;
	}

	// ���ݱ����еĴ�����������������Ӧ���߳���
	nThreads = 5; // IO_THREADS_PER_PROCESSOR * GetNumProcessors();

	// Ϊ�������̳߳�ʼ�����
	ioThreads = new HANDLE[nThreads];

	// ���ݼ�����������������������߳�
	for (int i = 0; i < nThreads; i++)
	{
		THREADPARAMS_WORKER* pThreadParams = new THREADPARAMS_WORKER;
		pThreadParams->server = this;
		pThreadParams->nThreadNo = i + 1;

		ioThreads[i] = CreateThread(NULL, 0, IoThread, (void*)pThreadParams, 0, NULL);
	}

	return true;
}



//Ͷ��Connect����
bool Server::IocpPostConnect(Packet* packet)
{
	// ��ʼ������
	DWORD dwBytes = 0;
	OVERLAPPED *p_ol = &packet->overlapped;
	PackBuf *p_wbuf = &packet->packBuf;
	Socket* socket = packet->socketCtx;
	ServerInfo* serverInfo = socket->GetRemoteServerInfo();

	// ����Զ�̷�������ַ��Ϣ
	SOCKADDR_IN serverAddress;
	if (false == CreateAddressInfoEx(serverInfo->serverIP, serverInfo->serverPort, &serverAddress))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "��Ч��Զ�̷�������ַ["<<remoteServerInfo.serverIP.c_str()<<":"<<remoteServerInfo.serverPort<<"]");
		return false;
	}

	packet->postIoType = POST_IO_CONNECT;

	int rc = lpfnConnectEx(
		packet->socketCtx->sock,
		(sockaddr*)&(serverAddress),
		sizeof(serverAddress),
		p_wbuf->buf,
		p_wbuf->len,
		&dwBytes,
		p_ol);

	if (rc == FALSE)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			//LOG4CPLUS_ERROR(log.GetInst(), "Ͷ�� ConnectEx ����ʧ�ܣ��������:" << WSAGetLastError());
			return false;
		}
	}

	return true;
}

// ��iocpͶ��Accept����
bool Server::IocpPostAccept(Packet* packet)
{
	// ׼������
	DWORD dwBytes = 0;
	packet->postIoType = POST_IO_ACCEPT;
	PackBuf *p_wbuf = &packet->packBuf;
	OVERLAPPED *p_ol = &packet->overlapped;

	if (INVALID_SOCKET == packet->socketCtx->sock)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "��������Accept��Socketʧ�ܣ��������:"<< WSAGetLastError());
		return false;
	}

	// Ͷ��AcceptEx
	if (FALSE == lpfnAcceptEx(
		listenCtx->sock,
		packet->socketCtx->sock,
		p_wbuf->buf,
		0,                              //p_wbuf->len - ((sizeof(SOCKADDR_IN)+16) * 2)
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&dwBytes,
		p_ol))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			//LOG4CPLUS_ERROR(log.GetInst(), "Ͷ�� AcceptEx ����ʧ�ܣ��������:" << WSAGetLastError());
			return false;
		}
	}

	return true;
}


// Ͷ�ݽ�����������
bool Server::IocpPostRecv(Packet* packet)
{
	// ��ʼ������
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	//PackBuf p_wbuf ;
	PackBuf *p_wbuf = &(packet->packBuf);
	OVERLAPPED *p_ol = &packet->overlapped;

	//Packet::ClearBuffer(packet);
	packet->postIoType = POST_IO_RECV;

	// ��ʼ����ɺ�Ͷ��WSARecv����
	int nBytesRecv = WSARecv(packet->socketCtx->sock, p_wbuf, 1, &dwBytes, &dwFlags, p_ol, NULL);

	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "Ͷ��WSARecvʧ��!");
		return false;
	}

	return true;
}


// Ͷ�ݷ�����������
bool Server::IocpPostSend(Packet* packet)
{
	// ��ʼ������
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	PackBuf *p_wbuf = &(packet->packBuf);
	OVERLAPPED *p_ol = &packet->overlapped;

	packet->postIoType = POST_IO_SEND;

	// ��ʼ����ɺ�Ͷ��WSARecv����
	int nBytesRecv = WSASend(packet->socketCtx->sock, p_wbuf, 1, &dwBytes, dwFlags, p_ol, NULL);

	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "Ͷ��WSASendʧ��!");
		return false;
	}

	return true;
}

// �����(Socket)�󶨵���ɶ˿���
bool Server::AssociateSocketContext(Socket *socketCtx)
{
	// �����ںͿͻ���ͨ�ŵ�SOCKET�󶨵���ɶ˿���
	HANDLE hTemp = CreateIoCompletionPort((HANDLE)socketCtx->sock, iocp, (ULONG_PTR)socketCtx, 0);

	if (NULL == hTemp)
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "ִ��CreateIoCompletionPort()���ִ���.������룺"<< GetLastError());
		return false;
	}

	return true;
}

void Server::DoAccept(Packet* packet)
{
	Socket* newSocketCtx = packet->socketCtx;
	int err = 0;

	SOCKADDR_IN* ClientAddr = NULL;
	SOCKADDR_IN* LocalAddr = NULL;
	int remoteLen = sizeof(SOCKADDR_IN), localLen = sizeof(SOCKADDR_IN);
	std::string* value = 0;

	//ȡ������ͻ��˵ĵ�ַ��Ϣ,ͨ�� lpfnGetAcceptExSockAddrs
	//��������ȡ�ÿͻ��˺ͱ��ض˵ĵ�ַ��Ϣ������˳��ȡ���ͻ��˷����ĵ�һ������
	lpfnGetAcceptExSockAddrs(
		packet->packBuf.buf,
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		(LPSOCKADDR*)&LocalAddr,
		&localLen,
		(LPSOCKADDR*)&ClientAddr,
		&remoteLen);


	err = setsockopt(
		packet->socketCtx->sock,
		SOL_SOCKET,
		SO_UPDATE_ACCEPT_CONTEXT,
		(char *)&(listenCtx->sock),
		sizeof(listenCtx->sock));


	//LOG4CPLUS_INFO(log.GetInst(),"�������Ե�ַ"<<inet_ntoa(ClientAddr->sin_addr)<<"������.");
	uint64_t timeStamp = CmiGetTickCount64();
	bool ret = true;

	// ����������ϣ������Socket����ɶ˿ڰ�(��Ҳ��һ���ؼ�����)
	if (newSocketCtx->GetSocketState() != WAIT_REUSED)
		ret = AssociateSocketContext(newSocketCtx);

	newSocketCtx->SetSocketState(CONNECTED_CLIENT);
	newSocketCtx->UpdataTimeStamp(timeStamp);

	// ȡ�ÿͻ���ip�Ͷ˿ں�
	inet_ntop(AF_INET, (void *)&(ClientAddr->sin_addr), newSocketCtx->remoteIP, 16);
	newSocketCtx->remotePort = ClientAddr->sin_port;


	if (ret == true)
	{	
		Packet* packet = newSocketCtx->CreatePacket((int)dePacketor->GetMaxBufferSize());
		serverTaskMgr->PostServerMessage(SMSG_ACCEPT_CLIENT, packet);
	}
	else
	{
		//LOG4CPLUS_ERROR(log.GetInst(), "�󶨲�������ʧ��.����Ͷ��Accept");
		newSocketCtx->SetSocketState(ASSOCIATE_FAILD);
		RELEASE(newSocketCtx);
		return;
	}

	serverTaskMgr->PostSingleIocpAccpetTask(packet);
}
