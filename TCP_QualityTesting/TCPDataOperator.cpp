#include "StdAfx.h"
#include "TCPDataOperator.h"
#include <string>

CTCPDataOperator::CTCPDataOperator(void)
{
	m_hMutex=CreateMutex(NULL,FALSE,NULL);
	m_hMutex_lines=CreateMutex(NULL,FALSE,NULL);
}


CTCPDataOperator::~CTCPDataOperator(void)
{
	Destory();
	CloseHandle(m_hMutex);
	CloseHandle(m_hMutex_lines);
}

bool CTCPDataOperator::Add(const PTHDATA_CONTCP pData)
{
	WaitForSingleObject(m_hMutex,INFINITE);
	m_vector.push_back(pData);
	ReleaseMutex(m_hMutex);
	return true;
}

bool CTCPDataOperator::Del(const unsigned &uRcID)
{
	vector<PTHDATA_CONTCP>::iterator iter;
	WaitForSingleObject(m_hMutex,INFINITE);
	for(iter=m_vector.begin();iter!=m_vector.end();)
	{
		
		if((*iter)->RcID==uRcID)
		{
			delete *iter;
			iter=m_vector.erase(iter);
		}
		else
			++iter;
	}
	ReleaseMutex(m_hMutex);
	return true;
}

bool CTCPDataOperator::Destory()
{
	vector<PTHDATA_CONTCP>::iterator iter;
	WaitForSingleObject(m_hMutex,INFINITE);
	for(iter=m_vector.begin();iter!=m_vector.end();)
	{
		delete (*iter);
		iter=m_vector.erase(iter);
	}
	ReleaseMutex(m_hMutex);

	vector<PLINEDATA>::iterator iter2;
	WaitForSingleObject(m_hMutex_lines,INFINITE);
	for(iter2=m_vLines.begin();iter2!=m_vLines.end();)
	{
		delete (*iter);
		iter2=m_vLines.erase(iter2);
	}
	ReleaseMutex(m_hMutex_lines);

	return true;
}

bool CTCPDataOperator::Size(unsigned &size)
{
	WaitForSingleObject(m_hMutex,INFINITE);
	size=m_vector.size();
	ReleaseMutex(m_hMutex);
	return true;
}


bool operator==(SOCKADDR_IN &p1, SOCKADDR_IN &p2)
{
	string s1=inet_ntoa(p1.sin_addr);
	string s2=inet_ntoa(p2.sin_addr);
	return  s1==s2&&
			p1.sin_family==p2.sin_family&&
			p1.sin_port==p2.sin_port;
}


BOOL CTCPDataOperator::Check(SOCKADDR_IN &sockAddr,const unsigned &uRcID)
{
	vector<PTHDATA_CONTCP>::iterator iter;
	WaitForSingleObject(m_hMutex,INFINITE);
	for(iter=m_vector.begin();iter!=m_vector.end();++iter)
	{
		
		if((*iter)->addrCli==sockAddr)
		{
			if((*iter)->RcID==uRcID)
			{
				ReleaseMutex(m_hMutex);
				return 1;
			}
			else
				if((*iter)->RcID==0)
				{
					(*iter)->RcID=uRcID;
					ReleaseMutex(m_hMutex);
					return 1;
				}
				else
				{
					//Å×³öÒì³£
					ReleaseMutex(m_hMutex);
					return -1;
				}
		}
	}
	ReleaseMutex(m_hMutex);
	return 0;
}


bool CTCPDataOperator::GetDataToClose(const unsigned &uRcID,SOCKET &socket,HANDLE &hThreadReceiver,HANDLE &hThreadGetmsger,unsigned &uGetmsgerID)
{
	vector<PTHDATA_CONTCP>::iterator iter;
	WaitForSingleObject(m_hMutex,INFINITE);
	for(iter=m_vector.begin();iter!=m_vector.end();++iter)
	{
		if((*iter)->RcID==uRcID)
		{
			socket=(*iter)->sockClient;
			hThreadReceiver=(*iter)->hThreadReceiver;
			hThreadGetmsger=(*iter)->hThreadGetmsger;
			uGetmsgerID=(*iter)->uThreadGetmsgerID;
			ReleaseMutex(m_hMutex);
			return true;
		}
	}
	ReleaseMutex(m_hMutex);
	return false;
}


unsigned CTCPDataOperator::GetGetmsgerID(const unsigned &uRcID)
{
	vector<PTHDATA_CONTCP>::iterator iter;
	WaitForSingleObject(m_hMutex,INFINITE);
	for(iter=m_vector.begin();iter!=m_vector.end();++iter)
	{
		if((*iter)->RcID==uRcID)
		{
			ReleaseMutex(m_hMutex);
			return (*iter)->uThreadGetmsgerID;
		}
	}
	ReleaseMutex(m_hMutex);
	return 0;
}


bool CTCPDataOperator::GetAllSocket(vector<SOCKET> &sockets)
{
	vector<PTHDATA_CONTCP>::iterator iter;
	WaitForSingleObject(m_hMutex,INFINITE);
	for(iter=m_vector.begin();iter!=m_vector.end();++iter)
	{
		if((*iter)->sockClient!=NULL)
		{
			sockets.push_back((*iter)->sockClient);
		}
	}
	ReleaseMutex(m_hMutex);
	return true;
}

bool CTCPDataOperator::GetAllGetmsgerID(vector<unsigned> &ids)
{
	vector<PTHDATA_CONTCP>::iterator iter;
	WaitForSingleObject(m_hMutex,INFINITE);
	for(iter=m_vector.begin();iter!=m_vector.end();++iter)
	{
		ids.push_back((*iter)->uThreadGetmsgerID);
	}
	ReleaseMutex(m_hMutex);
	return true;
}

bool CTCPDataOperator::GetReceiverHandle(vector<HANDLE> &handles)
{
	vector<PTHDATA_CONTCP>::iterator iter;
	WaitForSingleObject(m_hMutex,INFINITE);
	for(iter=m_vector.begin();iter!=m_vector.end();++iter)
	{
		if((*iter)->hThreadReceiver!=NULL)
		{
			handles.push_back((*iter)->hThreadReceiver);
		}
	}
	ReleaseMutex(m_hMutex);
	return true;
}

bool CTCPDataOperator::AddLine(const unsigned &uRcID)
{
	PLINEDATA p=new LINEDATA();
	p->bAlive=true;
	p->uRcID=uRcID;
	p->dieTime=0;
	WaitForSingleObject(m_hMutex_lines,INFINITE);
	m_vLines.push_back(p);
	ReleaseMutex(m_hMutex_lines);
	return true;
}

bool CTCPDataOperator::SetAlive(const unsigned &uRcID)
{
	vector<PLINEDATA>::iterator iter;
	WaitForSingleObject(m_hMutex_lines,INFINITE);
	for(iter=m_vLines.begin();iter!=m_vLines.end();++iter)
	{
		if((*iter)->uRcID==uRcID)
		{
			(*iter)->bAlive=true;
			(*iter)->dieTime=0;
			break;
		}
	}
	ReleaseMutex(m_hMutex_lines);
	return true;
}

bool CTCPDataOperator::SetDead(const unsigned &uRcID)
{
	vector<PLINEDATA>::iterator iter;
	WaitForSingleObject(m_hMutex_lines,INFINITE);
	for(iter=m_vLines.begin();iter!=m_vLines.end();++iter)
	{
		if((*iter)->uRcID==uRcID)
		{
			(*iter)->dieTime++;
			break;
		}
	}
	ReleaseMutex(m_hMutex_lines);
	return true;
}

bool CTCPDataOperator::SetDead()
{
	vector<PLINEDATA>::iterator iter;
	WaitForSingleObject(m_hMutex_lines,INFINITE);
	for(iter=m_vLines.begin();iter!=m_vLines.end();++iter)
		(*iter)->bAlive=false;
	ReleaseMutex(m_hMutex_lines);
	return true;
}

bool CTCPDataOperator::DelLine(const unsigned &uRcID)
{
	vector<PLINEDATA>::iterator iter;
	WaitForSingleObject(m_hMutex_lines,INFINITE);
	for(iter=m_vLines.begin();iter!=m_vLines.end();++iter)
	{
		if((*iter)->uRcID==uRcID)
		{
			delete *iter;
			m_vLines.erase(iter);
			break;
		}
	}
	ReleaseMutex(m_hMutex_lines);
	return true;
}

bool CTCPDataOperator::GetDeadLines(deque<PLINESTATE> &qDeadLines)
{
	vector<PLINEDATA>::iterator iter;
	PLINESTATE p;
	WaitForSingleObject(m_hMutex_lines,INFINITE);
	for(iter=m_vLines.begin();iter!=m_vLines.end();++iter)
	{
		if((*iter)->bAlive)
			continue;
		p=new LINESTATE();
		p->uRcID=(*iter)->uRcID;
		(*iter)->dieTime++;
		if((*iter)->dieTime>3)
			p->bCertainlyDead=true;
		else
			p->bCertainlyDead=false;
		qDeadLines.push_back(p);
	}
	ReleaseMutex(m_hMutex_lines);
	return true;
}