#include "StdAfx.h"
#include "TCPOperator.h"

//CTCPOperator::CTCPOperator(void)
CTCPOperator::CTCPOperator(const unsigned &uThreadID)
{
	////////////////////////////////////////////////
	//
	//			��ʼ����Ա����
	//
	////////////////////////////////////////////////
	m_sockListen=0;
	memset(&m_addrListen, 0 ,SOCKADDR_LEN );
	m_isWSAStartup=false;
	m_isAccepting=false;
	m_hThreadAccepter=NULL;
	m_threadPRCGetmsgerID=uThreadID;
	m_hMutexInit=CreateMutex(NULL,FALSE,NULL);
	m_hKeepAlive_Event=CreateEvent(NULL, TRUE, TRUE, NULL); 
	m_hKeepAlive=0;
	////////////////////////////////////////////////
	//
	//			ָ��Windows Sockets API �汾
	//
	////////////////////////////////////////////////
	int iRes=0;

	WORD wVersionRequested= MAKEWORD(2,2);//2.2�汾���׽���
	WSADATA wsaData;
	iRes = WSAStartup( wVersionRequested, &wsaData );
	if ( iRes != 0 )
	{
		////////////////////////////////////////////////
		//
		//			����쳣����
		//
		////////////////////////////////////////////////
		return;
	}//�����׽��ֿ⣬�Ӳ�ʧ���򷵻�
	if ( LOBYTE( wsaData.wVersion ) != 2 ||HIBYTE( wsaData.wVersion ) != 2 )
	{
		////////////////////////////////////////////////
		//
		//			����쳣����
		//
		////////////////////////////////////////////////
		WSACleanup( );
		return; 
	}//�������2.2�����˳�
	
	m_isWSAStartup=true;

}


CTCPOperator::~CTCPOperator(void)
{
	CloseAccepter();
	CloseHandle(m_hMutexInit);
	CloseHandle(m_hKeepAlive_Event);
	WaitForSingleObject(m_hKeepAlive,INFINITE);
	CloseHandle(m_hKeepAlive);
	////////////////////////////////////
	//
	//			ע�⣺
	//    ��������������ִ��ʱ������
	//	 �����߳����ݵļ������ɴ���
	//
	///////////////////////////////////
	CloseAllSocket();
	WaitAllReceiverThread();

	m_dataOP.Destory();
	WSACleanup();
}


bool CTCPOperator::CreatAccepter(const unsigned &uPort,const string &sIP)
{
	if(m_isAccepting)
	{
		if(!CloseAccepter())
			return false;
	}

	if(!m_isWSAStartup)
	{
		return false;
	}
	////////////////////////////////////////////////
	//
	//			׼��������
	//					��socket����addr��bing��listen
	//					���÷�����ģʽ
	//
	////////////////////////////////////////////////
	int iRes=0;
	//*************************************************************************************����m_sockListen
	m_sockListen=socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if(m_sockListen==INVALID_SOCKET)
    {
		////////////////////////////////////////////////
		//
		//			����쳣����
		//
		////////////////////////////////////////////////
        return false;
    }

	//*************************************************************************************��дm_addrListen
	m_addrListen.sin_family = AF_INET;
	m_addrListen.sin_addr.S_un.S_addr = inet_addr(sIP.c_str());
    m_addrListen.sin_port = htons(uPort);

	//*************************************************************************************bind
	iRes = bind( m_sockListen, (const sockaddr*)&m_addrListen, SOCKADDR_LEN );
	if( iRes == SOCKET_ERROR )
    {
        ////////////////////////////////////////////////
		//
		//			����쳣����
		//
		////////////////////////////////////////////////
        closesocket( m_sockListen );
        return false;
    }

	//*************************************************************************************����m_sockListen
	iRes = listen( m_sockListen, 5 );
    if( iRes == SOCKET_ERROR )
    {
        ////////////////////////////////////////////////
		//
		//			����쳣����
		//
		////////////////////////////////////////////////
        closesocket( m_sockListen );
        return false;
    }
	/*
	//*************************************************************************************����m_sockListenΪ������ģʽ
	int iMode = 1;
	iRes=ioctlsocket(m_sockListen,FIONBIO, (u_long FAR*) &iMode);
	if( iRes == SOCKET_ERROR )
    {
		////////////////////////////////////////////////
		//
		//			����쳣����
		//
		////////////////////////////////////////////////
        closesocket( m_sockListen );
        return false;
    }
	*/
	//*************************************************************************************����ACCEPT�߳�
	PDATA_ACCEPTER pthDataAccept = new DATA_ACCEPTER();
	pthDataAccept->sockListen=m_sockListen;
	pthDataAccept->uThreadPRCGetmsgerID=m_threadPRCGetmsgerID;
	pthDataAccept->pDataOP=&m_dataOP;
	m_hThreadAccepter=(HANDLE)_beginthreadex(NULL,NULL,ThreadAccepter,pthDataAccept,0,NULL);
	if(m_hThreadAccepter == NULL)
	{
		////////////////////////////////////////////////
		//
		//			����쳣����
		//
		////////////////////////////////////////////////
	//	closesocket(pThDataConTCP->sockClient);
		delete pthDataAccept;
	}
	

	m_isAccepting=true;
	return true;
}//CSrvListen::SrvCreat


bool CTCPOperator::CloseAccepter()
{
	if(!m_isAccepting)
		return true;
	closesocket(m_sockListen);
	shutdown(m_sockListen,2);
	closesocket(m_sockListen);
	WaitForSingleObject(m_hThreadAccepter,INFINITE);
	CloseHandle(m_hThreadAccepter);
	m_isAccepting=false;
	return true;
}


unsigned __stdcall CTCPOperator::ThreadAccepter(void* lpParam)
{
	
	////////////////////////////////////////////////
	//
	//		����Ҫ��ѭ����������ӡ����߳̽���
	//				select(m_sockListen)10��
	//
	////////////////////////////////////////////////
	PDATA_ACCEPTER pthDataAccept=(PDATA_ACCEPTER)lpParam;
	SOCKET sockListen=pthDataAccept->sockListen;
	unsigned uThreadPRCGetmsgerID=pthDataAccept->uThreadPRCGetmsgerID;
	CTCPDataOperator *pDataOP=pthDataAccept->pDataOP;
	delete pthDataAccept;

	
	SOCKET sockClient=NULL;
	SOCKADDR_IN addrClient;
	memset( &addrClient, 0, SOCKADDR_LEN);
	int addrLenClient=SOCKADDR_LEN;//ע�⣺����ֱ����int*ǿ��ת��SOCKADDR_LEN��Ϊaccept()����
	
	while(1)
	{	
		sockClient = accept( sockListen, (sockaddr*)&addrClient, &addrLenClient );
		if( sockClient == INVALID_SOCKET  )
		{
			//continue;
			return EXITCODE_ACCEPT;
		}

//		if(!CreateTwoThread(sockClient,addrClient,0,*pDataOP))
		if(!CreateTwoThread(sockClient,addrClient,uThreadPRCGetmsgerID,0,*pDataOP))
		{
			//����쳣����
			shutdown(sockClient,2);
			continue;
		}
	}//while(1)
	
	return EXITCODE_ACCEPT;
}

unsigned __stdcall CTCPOperator::ThreadGetmsger(void* lpParam)
{
	
	PDATA_GETMSGER_TCP pInfo=(PDATA_GETMSGER_TCP)lpParam;
	const SOCKET socket=pInfo->sockClient;
	unsigned uRcID=pInfo->uRcID;
	delete pInfo;

	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	string *p=0;
	string sData;
	while(1)
	{
		if(GetMessage(&msg,0,0,0)) 
        {
            switch(msg.message)
            {
			case MSG_DATA_FROMMB_TOTCP:
				p=(string*)msg.wParam;
				sData=*p;
				delete p;

				if(!SendData(sData,socket))
				{
				}
				else
				{
				}
				break;
			case MSG_SOCKETCLOSE:
				return EXITCODE_GETMSGER;
				break;
			default:
				break;
			}
		}
	}

	return EXITCODE_GETMSGER;
}


unsigned __stdcall CTCPOperator::ThreadReceiver(void* lpParam)
{

	PDATA_RECEIVER pInfo=(PDATA_RECEIVER)lpParam;
	const SOCKET socket=pInfo->sockClient;
	const unsigned uThreadPRCID =pInfo->uThreadPRCGetmsgerID;
	const unsigned uThreadGMID =pInfo->uThreadGetmsgerID;
	const HANDLE hThreadGetmsger =pInfo->hThreadGetmsger;
	const unsigned uRcID = pInfo->uRcID;
	CTCPDataOperator* pDataOP=pInfo->pDataOP;
	delete pInfo;

	int iRes=0;//��������ֵ
	char cRecvBuff[DEFAULT_BUFF];
	memset(&cRecvBuff, 0 ,DEFAULT_BUFF );

	//************************************************************************ѭ����������
	unsigned errorNum=0;
	while(1)
	{		

		memset(&cRecvBuff, 0 ,DEFAULT_BUFF );
		iRes = recv(socket,cRecvBuff,DEFAULT_BUFF,0);
		if( iRes <= 0 )
		{

			if(errorNum=3)
			{

				closesocket(socket);
				iRes=PostThreadMessage(uThreadGMID,MSG_SOCKETCLOSE,0,0);
				if(iRes==0)
				{
//					PR(ErrorCode,GetLastError());
				}	
				WaitForSingleObject(hThreadGetmsger,INFINITE);
				CloseHandle(hThreadGetmsger);
				pDataOP->Del(uRcID);

				return EXITCODE_RECEIVER_SOCKETCLOSE;
			}
			++errorNum;
			////////////////////////////////////////////////
			//
			//			����쳣����
			//
			////////////////////////////////////////////////

		}
		else
		{
			errorNum=0;
			string sData;
			sData.assign(cRecvBuff);

			char buffer[100];
			sprintf_s(buffer,"%d%d",uRcID/10,uRcID%10);
			sData=buffer+sData;
			string *pData=new string(sData);
			PostThreadMessage(uThreadPRCID,MSG_DATA_FROMTCP_TOPRC,(WPARAM)pData,0);
		}
	}
	return EXITCODE_RECEIVER;
}


bool CTCPOperator::CreatConnect(const unsigned &uPort,const string &sIP,const unsigned &RcID)
{
	if(!m_isWSAStartup)
	{
		////////////////////////////////////////////////
		//
		//			����쳣����
		//
		////////////////////////////////////////////////
		return false;
	}
	//*************************************************************************************��дm_addrListen
	SOCKADDR_IN addrClient;
	addrClient.sin_family = AF_INET;
	addrClient.sin_addr.S_un.S_addr = inet_addr( sIP.c_str() );
    addrClient.sin_port = htons( uPort );

	int iRes;
	iRes=m_dataOP.Check(addrClient,RcID);
	switch(iRes)
	{
	case 1://�Ѵ���
		return true;
		break;
	case 0:
		break;
	default:
		//����쳣�����Ѵ�����·����RcID��ͬ
		break;
	}

	//*************************************************************************************����sockClient
	SOCKET sockClient;
	sockClient=socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if(sockClient==INVALID_SOCKET)
    {
		////////////////////////////////////////////////
		//
		//			����쳣����
		//
		////////////////////////////////////////////////
        return false;
    }

	
	//*************************************************************************************��������

	iRes=connect(sockClient,(const sockaddr*)(&addrClient),SOCKADDR_LEN);//ע��ڶ���������Ҫ����ǿ��ת����
	if(iRes!=0)
	{

		//����쳣����
		return false;
	}


	//*************************************************************************************�������߳�
//	if(!CreateTwoThread(sockClient,addrClient,RcID,m_dataOP))
	if(!CreateTwoThread(sockClient,addrClient,m_threadPRCGetmsgerID,RcID,m_dataOP))
		return false;
	return true;
}


bool CTCPOperator::CloseConnect(const unsigned &RcID)
{
	SOCKET socket;
	HANDLE hThreadReceiver,hThreadGetmsger;
	unsigned uGetmsgerID;
	if(!m_dataOP.GetDataToClose(RcID,socket,hThreadReceiver,hThreadGetmsger,uGetmsgerID))
		return true;
	int iRes=shutdown(socket,2);
	if(iRes==SOCKET_ERROR)
		return false;
	WaitForSingleObject(hThreadReceiver,INFINITE);
	CloseHandle(hThreadReceiver);
	m_dataOP.Del(RcID);
	return true;
}


bool CTCPOperator::IsAccepting()
{
	return m_isAccepting;
}

bool CTCPOperator::GetGetmsgerID(const unsigned &RcID,unsigned &GetmsgerID)
{
	for(int i=0;i<3;i++)
		GetmsgerID=m_dataOP.GetGetmsgerID(RcID);
		if(GetmsgerID!=0)
			return true;
	return false;
}

BOOL CTCPOperator::InitData(const string &sData,const unsigned &RcID)
{
	string *pStr=new string;
	*pStr=sData;
	WaitForSingleObject(m_hMutexInit,INFINITE);
	unsigned uGetmsgerID=m_dataOP.GetGetmsgerID(RcID);
	if(uGetmsgerID==0)
	{
		ReleaseMutex(m_hMutexInit);
		delete pStr;
		return -1;
	}
	int iRes=PostThreadMessage(uGetmsgerID,MSG_DATA_FROMMB_TOTCP,(WPARAM)pStr,NULL);
	if(iRes==0)
	{
		ReleaseMutex(m_hMutexInit);
		return 0;
	}
	ReleaseMutex(m_hMutexInit);
	return 1;
}

bool CTCPOperator::ColseReceiver(const SOCKET &socket,const HANDLE &hThread)
{
	closesocket(socket);
	WaitForSingleObject(hThread,INFINITE);
	CloseHandle(hThread);
	return true;
}

bool CTCPOperator::ColseGetmsger(const unsigned &uThreadID,const HANDLE &hThread)
{
	PostThreadMessage(uThreadID,WM_QUIT,0,0);
	WaitForSingleObject(hThread,INFINITE);
	CloseHandle(hThread);
	return true;
}

bool CTCPOperator::CloseAllSocket()
{
	vector<SOCKET> sockets;
	m_dataOP.GetAllSocket(sockets);
	vector<SOCKET>::iterator iter;
	for(iter=sockets.begin();iter!=sockets.end();++iter)
	{
		int i=0;
		while(shutdown(*iter,2)==0&&i<3) ++i;
	}
	sockets.clear();
	return true;
}

bool CTCPOperator::CloseAllGetmsger()
{
	vector<unsigned> ids;
	m_dataOP.GetAllGetmsgerID(ids);
	vector<unsigned>::iterator iter;
	for(iter=ids.begin();iter!=ids.end();++iter)
	{
		int i=0;
		while(PostThreadMessage(*iter,WM_QUIT,0,0)!=0&&i<3) ++i;
	}
	ids.clear();
	return true;
}

bool CTCPOperator::WaitAllReceiverThread()
{
	vector<HANDLE> handles;
	m_dataOP.GetReceiverHandle(handles);
	vector<HANDLE>::iterator iter;
	for(iter=handles.begin();iter!=handles.end();++iter)
	{
		WaitForSingleObject(*iter,INFINITE);
		CloseHandle(*iter);
	}
	handles.clear();
	return true;
}

//bool CTCPOperator::CreateTwoThread(SOCKET &sockClient,const SOCKADDR_IN &addrClient,const unsigned &RcID,CTCPDataOperator &dataOP)
bool CTCPOperator::CreateTwoThread(SOCKET &sockClient,const SOCKADDR_IN &addrClient,const unsigned &uThreadPRCGetmsgerID,const unsigned &RcID,CTCPDataOperator &dataOP)
{
	HANDLE hThreadReceiver=NULL;
	HANDLE hThreadGetmsger=NULL;
	unsigned uThreadReceiverID=0;
	unsigned uThreadGetmsgerID=0;
	int iErrorNum=0;

	PDATA_RECEIVER pDataToReceiver = new DATA_RECEIVER();
	pDataToReceiver->sockClient=sockClient;
	pDataToReceiver->uThreadPRCGetmsgerID=uThreadPRCGetmsgerID;
	pDataToReceiver->pDataOP=&dataOP;
	pDataToReceiver->uRcID=RcID;

	PDATA_GETMSGER_TCP pDataToGetmsger = new DATA_GETMSGER_TCP;
	pDataToGetmsger->sockClient=sockClient;
	pDataToGetmsger->uRcID=RcID;
	while(iErrorNum<3)
	{
		if(hThreadReceiver==NULL)
			hThreadReceiver=(HANDLE)_beginthreadex(NULL,NULL,ThreadReceiver,pDataToReceiver,CREATE_SUSPENDED,&uThreadReceiverID);
		if(hThreadGetmsger==NULL)
			hThreadGetmsger=(HANDLE)_beginthreadex(NULL,NULL,ThreadGetmsger,pDataToGetmsger,CREATE_SUSPENDED,&uThreadGetmsgerID);
		if(hThreadReceiver && hThreadGetmsger)
			break;
		++iErrorNum;
	}

	if(iErrorNum==3)
	{
		if(hThreadReceiver==NULL)
			ColseReceiver(sockClient,hThreadReceiver);
		if(hThreadGetmsger==NULL)
			ColseGetmsger(uThreadGetmsgerID,hThreadGetmsger);
		delete pDataToReceiver;
		delete pDataToGetmsger;
		return false;
	}

	//****************************************************�½��̺߳����������
	pDataToReceiver->uThreadGetmsgerID=uThreadGetmsgerID;
	pDataToReceiver->hThreadGetmsger=hThreadGetmsger;
	ResumeThread(hThreadGetmsger);
	ResumeThread(hThreadReceiver);

	//****************************************************��������
	PTHDATA_CONTCP pData = new THDATA_CONTCP();
	pData->addrCli = addrClient;
	pData->hThreadGetmsger = hThreadGetmsger;
	pData->hThreadReceiver = hThreadReceiver;
	pData->RcID = RcID;
	pData->sockClient = sockClient;
	pData->uThreadGetmsgerID = uThreadGetmsgerID;
	pData->uThreadReceiverID = uThreadReceiverID;
	dataOP.Add(pData);//��������

	HANDLE handles[2];
	handles[0]=hThreadGetmsger;
	handles[1]=hThreadReceiver;
	do
	{
		Sleep(10);
	}while(WaitForMultipleObjects(2,handles,false,0)!=WAIT_TIMEOUT);

	return true;
}

bool CTCPOperator::SendData(const string &sData,const SOCKET &socket)
{
	int iRes;
	unsigned errorNum=0;

	while(errorNum<3)
	{
		iRes=send(socket, sData.c_str(), sData.size(), 0);
		if(iRes!=sData.size())
		{
			++errorNum;
			Sleep(10);
		}
		else
			return true;
	}
	//����쳣����
	return false;
}

/*�����
unsigned __stdcall CTCPOperator::ThreadKeepAlive(void* lpParam)
{
	PDATA_KEEPALIVE pInfo =(PDATA_KEEPALIVE)lpParam;
	CTCPDataOperator *pDataOP=pInfo->pDataOP;
	CTCPOperator *pTCPOP=pInfo->pTCPOP;
	HANDLE *pEvent=pInfo->pEvent;
	delete pInfo;

	deque<PLINESTATE> qDeadLines;
	PLINESTATE p;
	string sData("\x02\xFF\xFF\xFF\xFF\x30\x31\x48\xFF\xFF\x03");
	while(WaitForSingleObject(*pEvent,INFINITE)==WAIT_OBJECT_0)
	{
		pDataOP->GetDeadLines(qDeadLines);
		pDataOP->SetDead();
		Sleep(50000);
		while(qDeadLines.size()>0)
		{
			p=qDeadLines.front();
			if(p->bCertainlyDead)
			{
				pTCPOP->CloseConnect(p->uRcID);
				pDataOP->Del(p->uRcID);
				pDataOP->DelLine(p->uRcID);
			}
			else
				pTCPOP->InitData(sData,p->uRcID);
			delete p;
			qDeadLines.pop_front();
		}
	}
	return 1;
}

bool CTCPOperator::StartKeepAlive()
{
	PDATA_KEEPALIVE pInfo=new DATA_KEEPALIVE();
	pInfo->pDataOP=&m_dataOP;
	pInfo->pEvent=&m_hKeepAlive_Event;
	pInfo->pTCPOP=this;
	m_hKeepAlive=(HANDLE)_beginthreadex(NULL,NULL,ThreadKeepAlive,pInfo,0,0);
	
	return 1;
}

bool CTCPOperator::StopKeepAlive()
{
	ResetEvent(m_hKeepAlive_Event);
	return 1;
}

bool CTCPOperator::RestartKeepAlive()
{
	SetEvent(m_hKeepAlive_Event);
	return 1;
}
*/