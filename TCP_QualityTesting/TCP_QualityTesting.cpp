// TCP_QualityTesting.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "TCPOperator.h"
#include <iostream>
#include <fstream>
#include "mmsystem.h"
#include <atlstr.h>
#pragma comment(lib,"winmm")

using namespace std;

#define MSG_QUIT WM_USER+600
#define MSG_SEND WM_USER+601

HANDLE hGetmsger=0;
HANDLE hLoop=0;
unsigned uGetmsgerID=0;
unsigned uLoopID=0;
CTCPOperator *pTCPoper=0;

DWORD time_send=0,time_recv=0,time_error,time_loopstart=0,time_loopend=0;
HANDLE hEvent_recv=0,hEvent_work=0,hEvent_resend=0;
unsigned long times_send=0,times_recv=0;

fstream file;

unsigned __stdcall ThreadGetmsger(void* lpParam)
{
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

	string text_recv;
	string *pstr=NULL;
	while(1)
	{
		if(GetMessage(&msg,0,0,0)) 
        {
            switch(msg.message)
			{
			case MSG_QUIT:
				return 1;
				break;
			case MSG_DATA_FROMTCP_TOPRC:
				text_recv=*(string*)msg.wParam;
				if(text_recv == _T("From R01：There is a testing text!"))
				{
					ResetEvent(hEvent_resend);
					SetEvent(hEvent_recv);
					times_recv++;
				}
				break;
			default:
				break;
			}
		}
	}
	return 1;
}

void CALLBACK TimeResend(UINT uID,UINT uMsg,DWORD dwUsers,DWORD dw1,DWORD dw2)
{
	if(WaitForSingleObject(hEvent_resend,0)==WAIT_OBJECT_0)
	{
		time_error=timeGetTime();
		SetEvent(hEvent_recv);
		ResetEvent(hEvent_resend);
	}
}

void CALLBACK TimeLoop(UINT uID,UINT uMsg,DWORD dwUsers,DWORD dw1,DWORD dw2)
{
	WaitForSingleObject(hEvent_work,INFINITE);
	PostThreadMessage(uLoopID,MSG_SEND,NULL,NULL);
}

unsigned __stdcall Loop(void* lpParam)
{
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

//	string senStr(_T("There is a testing text!"));
//	CString buffer;

//	timeBeginPeriod(1);
	UINT timerID=0; 
	WORD wVersionRequested= MAKEWORD(2,2);//2.2版本的套接字
	WSADATA wsaData;
	WSAStartup( wVersionRequested, &wsaData );

	string ip("192.168.21.101");
	unsigned port=60000;
	SOCKADDR_IN addrClient;
	addrClient.sin_family = AF_INET;
	addrClient.sin_addr.S_un.S_addr = inet_addr( ip.c_str() );
	addrClient.sin_port = htons( port );

	SOCKET sockClient;
	sockClient=socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

	connect(sockClient,(const sockaddr*)(&addrClient),SOCKADDR_LEN);

	//发送
	int errorNum=0;
	string sendData("There is a testing text!"),recvData("");
	char rData[25],key=0;
	ZeroMemory(rData,25);
	CString buffer("");

	file.open("..\\Data.info",ios::out);
	file.close();
	int iRes;

	while(1)
	{
		if(GetMessage(&msg,0,0,0)) 
        {
            switch(msg.message)
			{
			case MSG_QUIT:
				return 1;
				break;
			case MSG_SEND:
				iRes=send(sockClient, sendData.c_str(), sendData.size(), 0);
				if(iRes!=sendData.size())
				{
					Sleep(10);
					continue;
				}
				time_send=timeGetTime();
				times_send++;
				recv(sockClient,rData,DEFAULT_BUFF,0);
				time_recv=timeGetTime();
				recvData=rData;
				file.open("..\\Data.info",ios::app);
				if(file)
				{
					if(recvData==sendData)
						buffer.Format(_T("%lu %lu %lu"),times_send,time_send,time_recv);
					else
						buffer.Format(_T("%lu %d %d"),times_send,0,0);
					file<<buffer<<endl;
					file.close();
				}
				else
				{
					cout<<_T("打不开保存文件！")<<endl;
					system("pause");
					return -1;
				}
				/*
				time_send=timeGetTime();
				if(pTCPoper->InitData(senStr,1))
					times_send++;
				SetEvent(hEvent_resend);
				timeSetEvent(100,1,TimeResend,NULL,TIME_ONESHOT|TIME_KILL_SYNCHRONOUS);
				WaitForSingleObject(hEvent_recv,INFINITE);
				timeKillEvent(timerID);
				time_recv=timeGetTime();
				
				file.open("..\\Data.info",ios::app);
				if(file)
				{
					if(time_error)
					{
						buffer.Format(_T("ERROR:%010lu %010lu %010lu"),times_send,time_send,time_error);
						file<<buffer<<endl;
						time_error=0;
					}
					else
					{
						buffer.Format(_T("%010lu %010lu %010lu"),times_send,time_send,time_recv);
						file<<buffer<<endl;
					}
					file.close();
				}
				else
				{
					cout<<_T("打不开保存文件！")<<endl;
					system("pause");
					return -1;
				}
				*/
				break;
			default:
				break;
			}
		}
	}
	shutdown(sockClient,2);
//	timeEndPeriod(1);
	return 1;
}
/*
//下面是可用的代码段
//调用了tcp类
int _tmain(int argc, _TCHAR* argv[])
{
	hEvent_recv=CreateEvent(NULL,FALSE,FALSE,NULL);
	hEvent_work=CreateEvent(NULL,TRUE,FALSE,NULL);
	hEvent_resend=CreateEvent(NULL,TRUE,TRUE,NULL);
	hGetmsger=(HANDLE)_beginthreadex(NULL,NULL,ThreadGetmsger,NULL,0,&uGetmsgerID);
	pTCPoper=new CTCPOperator(uGetmsgerID);
//	pTCPoper->CreatConnect(60000,"127.0.0.1",1);
	if(!pTCPoper->CreatConnect(60000,"192.168.21.101",1))
	{
		cout<<_T("连接不了控制器。");
		system(_T("pause"));
		return -1;
	}
	hLoop=(HANDLE)_beginthreadex(NULL,NULL,Loop,NULL,0,&uLoopID);

	bool bWorking=false;
	char c[20]={};
	string str("");

	file.open("..\\Data.info",ios::out);
	file.close();

	timeBeginPeriod(1);
	UINT timerID=timeSetEvent(500,1,TimeLoop,NULL,TIME_PERIODIC); 

	time_loopstart=timeGetTime();
	while(1)
	{
		if(bWorking)
			cout<<_T("-输入“P”停止。")<<endl;
		else
			cout<<_T("-输入“A”运行。")<<endl;
		cout<<_T("-输入“Q”退出。")<<endl<<endl<<endl;
		cin.getline(c,20);
		str=c;
		if(str=="A"||str=="a")
		{
			if(bWorking)
				continue;
			SetEvent(hEvent_work);
			bWorking=true;
			cout<<_T("运行中......")<<endl;
			continue;
		}
		if(str=="P"||str=="p")
		{
			if(!bWorking)
				continue;
			ResetEvent(hEvent_work);
			bWorking=false;
			cout<<_T("已暂停。")<<endl;
			continue;
		}
		if(str=="Q"||str=="q")
		{
			break;
		}
	}
	time_loopend=timeGetTime();

	timeKillEvent(timerID);
	timeEndPeriod(1);

	SetEvent(hEvent_recv);
	CloseHandle(hEvent_recv);
	hEvent_recv=0;
	SetEvent(hEvent_work);
	CloseHandle(hEvent_work);
	hEvent_work=0;
	PostThreadMessage(uLoopID,MSG_QUIT,NULL,NULL);
	WaitForSingleObject(hLoop,INFINITE);
	PostThreadMessage(uGetmsgerID,MSG_QUIT,NULL,NULL);
	WaitForSingleObject(hGetmsger,INFINITE);
	delete pTCPoper;
	return 0;
}
*/

int _tmain(int argc, _TCHAR* argv[])
{
	//初始化socket
	hLoop=(HANDLE)_beginthreadex(NULL,NULL,Loop,NULL,0,&uLoopID);

	timeBeginPeriod(1);
	UINT timerID=timeSetEvent(30,1,TimeLoop,NULL,TIME_PERIODIC); 
	char key=0;
	while (cin>>key)
	{
		if(key=='Q')
			break;
	}
	timeKillEvent(timerID);
	timeEndPeriod(1);

	
	return 0;
}