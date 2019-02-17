#pragma once

#include"utils/include/IocpExFuncs.h"
#include"utils/include/utils.h"
#include"utils/include/CmiThreadLock.h"
#include"utils/include/CmiWaitSem.h"
#include"utils/include/CmiAlloc.h"
#include"utils/include/CmiNewMemory.h"
#include<string>
#include<map>
#include<set>
#include <iostream>
#include<list>
#include <unordered_map>  
#include"utils/include/Singleton.h"
#include"utils/include/UniqueID.h"
#include"utils/include/CmiLog.h"
using namespace std;

	
// ÿһ���������ϲ������ٸ��߳�
#define IO_THREADS_PER_PROCESSOR 2
// ͬʱͶ�ݵ�Accept���������
#define MAX_POST_ACCEPT              20

#define ONE_DAY_MS        86400000

#define MAX_PATH          260
// ���������� (1024*4)
#define MAX_BUFFER_LEN        2048
#define MIDDLE_BUFFER_LEN     1024
// Ĭ�϶˿�
#define DEFAULT_PORT          8888
// Ĭ��IP��ַ
#define DEFAULT_IP            "127.0.0.1"

// ���ݸ�Worker�̵߳��˳��ź�
#define EXIT_CODE                    NULL


// �ͷ�Socket��
//#define RELEASE_SOCKET(x)               {if(x !=INVALID_SOCKET) { lpfnDisConnectEx(x,NULL,TF_REUSE_SOCKET,0);x=INVALID_SOCKET;}}
#define RELEASE_SOCKET(x)               {if(x !=INVALID_SOCKET) { shutdown(x,SD_SEND); closesocket(x);x=INVALID_SOCKET;}}

typedef uint64_t socket_id_t;

	typedef struct _Packet Packet;
	class DePacketor;
	class ServerInfo;
	class Server;
	class Socket;
	class ServerTask;
	class ServerTaskMgr;
	class BaseMsgProcesser;
	class BaseMsgCoroutine;

	typedef basic_string<char, char_traits<char>, CmiAlloc<char> > CmiString;

	typedef set<Socket*, less<Socket*>, CmiAlloc<Socket*>> SocketCtxSet;
	typedef list<Packet*, CmiAlloc<Packet*>> PacketList;
	typedef list<Socket*, CmiAlloc<Socket*>> SocketCtxList;
	typedef list<ServerInfo*, CmiAlloc<ServerInfo*>> RemoteServerInfoList;

	//typedef unordered_map<uint64_t, Socket*, hash_compare<uint64_t, less<uint64_t>>, CmiAlloc<pair<uint64_t, Socket*>>> SocketHashMap;
	typedef unordered_map<socket_id_t, Socket*, hash<socket_id_t>, equal_to<socket_id_t>, CmiAlloc<pair<socket_id_t, Socket*>>> SocketHashMap;

	typedef unordered_map<uint64_t, BaseMsgCoroutine*, hash<uint64_t>, equal_to<uint64_t>, CmiAlloc<pair<uint64_t, BaseMsgCoroutine*>>> MsgCoroutineHashMap;
	//typedef unordered_map<int32_t, vector<BaseMsgCoroutine*, CmiAlloc<BaseMsgCoroutine*>>, hash<int32_t>, equal_to<int32_t>, CmiAlloc<pair<int32_t, vector<BaseMsgCoroutine*, CmiAlloc<BaseMsgCoroutine*>>>>> MsgCoroutinesHashMap;

	typedef struct _WSABUF PackBuf;

	struct PackHeader
	{
		uint16_t type;
		uint16_t len;
	};

	enum ExtractState
	{
		ES_PACKET_HEADLEN_NOT_GET,  //��ͷ����δ��ȡ
		ES_PACKET_HEAD_FULL,     //�ѻ�ȡ��ͷ��Ϣ	
		ES_PACKET_HEAD_NOT_FULL, //��ͷ��Ϣ��ȫ
	};

	enum SocketEvent
	{
		EV_SOCKET_OFFLINE,
		EV_SOCKET_PORT_BEUSED,
		EV_SOCKET_CONNECTED,
		EV_SOCKET_ACCEPTED,
		EV_SOCKET_SEND,
		EV_SOCKET_RECV
	};

	enum DataTransMode
	{
		MODE_STREAM,
		MODE_PACK
	};

	typedef void(*UnPackCallBack)(SocketEvent ev, Socket& socketCtx, void* param);
	typedef int32_t(*GetPackDataLenCB)(DePacketor* dePacket, uint8_t* pack, size_t packLen, int32_t* realPackHeadLen);
	typedef void(*SetDataLengthToPackHeadCallBack)(uint8_t* pack, size_t dataSize);

	// Ͷ�ݵ�I/O����
	enum PostIoType
	{
		POST_IO_CONNECT = 0,
		POST_IO_ACCEPT,                     // Accept����
		POST_IO_RECV,                       // ��������
		POST_IO_SEND,                       // ��������
		POST_IO_NULL

	};

	enum SocketType
	{
		UNKNOWN_SOCKET,                  //δ��SOCKET����
		LISTENER_SOCKET,                 //��������SOCKET
		LISTEN_CLIENT_SOCKET,            //������Զ�̿ͻ���֮��ͨ����SOCKET
		CONNECT_SERVER_SOCKET            //������Զ��Զ�̷�����֮�������ӵ�SOCKET
	};

	enum SocketState
	{
		INIT_FAILD,             //��ʼ��ʧ��
		LISTEN_FAILD,           //����ʧ��
		LISTENING,              //�������й���������
		CONNECTED_CLIENT,       //����ͻ�������
		CONNECTTING_SERVER,     //�������������
		CONNECTED_SERVER,       //�����ӷ�����
		WAIT_REUSED,            //�ȴ�����SOCKET
		NEW_CREATE,             //�����ɵ�SOCKET
		ASSOCIATE_FAILD,        //�󶨵���ɶ˿�ʧ��
		BIND_FAILD,             //�󶨶˿�ʧ��
		LOCALIP_INVALID,        //��Ч�ı��ص�ַ    
		PORT_BEUSED,            //�˿ڱ�ռ��
		RESPONSE_TIMEOUT,       //��Ӧ��ʱ
		RECV_DATA_TOO_BIG,      //���յ����ݹ���
		NORMAL_CLOSE,           //�����ر�
	};


	enum ServerMessage
	{
		SMSG_LISTENER_CREATE_FINISH,    //�������������
		SMSG_ACCEPT_CLIENT,             //���յ�һ���ͻ������� 
		SMSG_REQUEST_CONNECT_SERVER     //�������ӷ�����
	};

	// �������̵߳��̲߳���
	typedef struct _tagThreadParams_WORKER
	{
		Server* server;                                   // ��ָ�룬���ڵ������еĺ���
		int         nThreadNo;                                    // �̱߳��
	}THREADPARAMS_WORKER, *PTHREADPARAM_WORKER;


	struct ServerTaskState
	{
		int state;
		int clientSize;
	};

	struct TaskData
	{
		Socket* socketCtx;
		void* dataPtr;
	};

	struct ServerMsgTaskData
	{
		ServerMessage msg;
		ServerTaskMgr* serverTaskMgr;
		void* dataPtr;
	};
