#pragma once

//#include <vector>
//#include <WinSock2.h>
//#include <afxmt.h>
#include <string>
#include <process.h>
#include "TCPDataOperator.h"
//#include <iostream>
//#include <array>
//#include <atlstr.h>


using namespace std;


#define DEFAULT_LISTEN_IP "127.0.0.1"
#define DEFAULT_LISTEN_PORT 500
#define DEFAULT_BUFF 256
#define SOCKADDR_LEN sizeof(SOCKADDR_IN)

//#define DEBUG  
//#define PR(x,y) cout <<#x<< " : " << (y) << endl;  

#define MSG_DATA_FROMMB_TOTCP WM_USER+100
#define MSG_DATA_FROMTCP_TOPRC WM_USER+101
#define MSG_SOCKETCLOSE WM_USER+102

#define EXITCODE_ACCEPT WM_USER+103
#define EXITCODE_RECEIVER WM_USER+104
#define EXITCODE_GETMSGER WM_USER+105
#define EXITCODE_RECEIVER_SOCKETCLOSE WM_USER+106


typedef struct _DataToAccepter
{
	CTCPDataOperator *pDataOP;
	SOCKET sockListen;
	unsigned uThreadPRCGetmsgerID;
}DATA_ACCEPTER,*PDATA_ACCEPTER;

typedef struct _DataToGetmsger_TCP
{
	SOCKET sockClient;
	unsigned uRcID;
}DATA_GETMSGER_TCP,*PDATA_GETMSGER_TCP;

typedef struct _DataToReceiver
{
	SOCKET sockClient;
	unsigned uRcID;
	unsigned uThreadPRCGetmsgerID;
	unsigned uThreadGetmsgerID;
	HANDLE hThreadGetmsger;
	CTCPDataOperator* pDataOP;
}DATA_RECEIVER,*PDATA_RECEIVER;


class CTCPOperator
{
public:

	CTCPOperator(const unsigned &uThreadID);
	~CTCPOperator(void);
	bool CreatAccepter(const unsigned &uPort=DEFAULT_LISTEN_PORT,const string &sIP=DEFAULT_LISTEN_IP);
	bool CloseAccepter();
	bool CreatConnect(const unsigned &uPort,const string &sIP,const unsigned &RcID);
	bool CloseConnect(const unsigned &RcID);
	bool IsAccepting();
	BOOL InitData(const string &sData,const unsigned &RcID);
	bool GetGetmsgerID(const unsigned &RcID,unsigned &GetmsgerID);
	
//	bool StartKeepAlive();
//	bool StopKeepAlive();
//	bool RestartKeepAlive();

private:

	bool							m_isWSAStartup;
	bool							m_isAccepting;
	SOCKET							m_sockListen;
    SOCKADDR_IN						m_addrListen;
	HANDLE							m_hThreadAccepter;
	HANDLE							m_hMutexInit;
	unsigned						m_threadPRCGetmsgerID;
	CTCPDataOperator				m_dataOP;

	static unsigned __stdcall ThreadAccepter(void* lpParam);
	static unsigned __stdcall ThreadReceiver(void* lpParam);
	static unsigned __stdcall ThreadGetmsger(void* lpParam);

	bool CloseAllSocket();
	bool CloseAllGetmsger();
	bool WaitAllReceiverThread();
	static bool SendData(const string &sData,const SOCKET &socket);
	static bool ColseReceiver(const SOCKET &socket,const HANDLE &hThread);
	static bool ColseGetmsger(const unsigned &uThreadID,const HANDLE &hThread);
	static bool CreateTwoThread(SOCKET &sockClient,const SOCKADDR_IN &addrClient,const unsigned &uThreadPRCGetmsgerID,const unsigned &RcID,CTCPDataOperator &dataOP);

	static unsigned __stdcall ThreadKeepAlive(void* lpParam);
	HANDLE m_hKeepAlive_Event;
	HANDLE m_hKeepAlive;
};

typedef struct _DataToKeepAlive
{
	CTCPDataOperator *pDataOP;
	CTCPOperator *pTCPOP;
	HANDLE *pEvent;
}DATA_KEEPALIVE,*PDATA_KEEPALIVE;