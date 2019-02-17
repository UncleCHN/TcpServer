#ifndef _IOCPEXFUNCS_H_
#define _IOCPEXFUNCS_H_

#include<winsock2.h>
#include<MSWSock.h>

extern LPFN_CONNECTEX               lpfnConnectEx;
extern LPFN_DISCONNECTEX            lpfnDisConnectEx;
extern LPFN_ACCEPTEX                lpfnAcceptEx;               // AcceptEx �� GetAcceptExSockaddrs �ĺ���ָ�룬���ڵ�����������չ����
extern LPFN_GETACCEPTEXSOCKADDRS    lpfnGetAcceptExSockAddrs;

bool LoadIocpExFuncs();
bool LoadWsaFunc_AccpetEx(SOCKET tmpsock);
bool LoadWsaFunc_ConnectEx(SOCKET tmpsock);

#endif