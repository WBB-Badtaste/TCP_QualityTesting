#pragma once

#include <vector>
#include <deque>
//#include <algorithm>

#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")

using namespace std;

typedef struct _ThDataConnectTCP
{
	SOCKET sockClient;
	SOCKADDR_IN addrCli;
	HANDLE hThreadReceiver;
	HANDLE hThreadGetmsger;
	unsigned uThreadReceiverID;
	unsigned uThreadGetmsgerID;
	unsigned RcID;
}THDATA_CONTCP,*PTHDATA_CONTCP;

typedef struct _LineData
{
	unsigned uRcID;
	bool bAlive;
	unsigned dieTime;
}LINEDATA,*PLINEDATA;

typedef struct _LineState
{
	unsigned uRcID;
	bool bCertainlyDead;
}LINESTATE,*PLINESTATE;

class CTCPDataOperator
{

public:
	CTCPDataOperator(void);
	~CTCPDataOperator(void);

	bool Add(const PTHDATA_CONTCP pData);
	bool Del(const unsigned &uRcID);
	bool Destory();
	bool Size(unsigned &size);
	BOOL Check(SOCKADDR_IN &sockAddr,const unsigned &uRcID);
	bool GetDataToClose(const unsigned &uRcID,SOCKET &socket,HANDLE &hThreadReceiver,HANDLE &hThreadGetmsger,unsigned &uGetmsgerID);
	bool GetAllSocket(vector<SOCKET> &sockets);
	bool GetAllGetmsgerID(vector<unsigned> &ids);
	bool GetReceiverHandle(vector<HANDLE> &handles);
	unsigned GetGetmsgerID(const unsigned &uRcID);

	bool AddLine(const unsigned &uRcID);
	bool SetAlive(const unsigned &uRcID);
	bool DelLine(const unsigned &uRcID);
	bool SetDead(const unsigned &uRcID);
	bool SetDead();
	bool GetDeadLines(deque<PLINESTATE> &qDeadLines);
private:

	friend bool operator==(SOCKADDR_IN &p1, SOCKADDR_IN &p2); 

	HANDLE m_hMutex;
	vector<PTHDATA_CONTCP> m_vector;
	
	HANDLE m_hMutex_lines;
	vector<PLINEDATA> m_vLines;
};

