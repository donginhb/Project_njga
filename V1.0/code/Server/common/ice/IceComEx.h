// IceComEx.h: interface for the CIceComEx class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ICECOMEX_H__AA54AA1D_C3A5_4BBC_B946_207470E4EF59__INCLUDED_)
#define AFX_ICECOMEX_H__AA54AA1D_C3A5_4BBC_B946_207470E4EF59__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <iostream>
#include <string>
#include <Ice/Ice.h> 
#include <Ice/Communicator.h>  
#include "CSysRunState.h"
#ifdef WIN32
#include "../../app/public/Include/IEV9000IceEx.h"
#endif

#ifndef WIN32
#include <pthread.h>
//#define     EV9000ICE_API   extern "C"
#define     EV9000ICE_API   
typedef     unsigned short  USHORT;
typedef     int             LONG;
typedef     unsigned char   BYTE;
#define     BOOL            int
typedef     unsigned int    UINT;
typedef     void*           LPVOID;
typedef     void*           HANDLE;
typedef     unsigned long long    UINT64;

#ifndef     TRUE
#define     TRUE    1
#endif
#ifndef     FALSE
#define     FALSE   0
#endif
#ifndef     NULL
#define	    NULL    0
#endif

#define __stdcall
#define CALLBACK

#ifndef __HWND_defined
#define __HWND_defined
typedef void* HWND; 
#else 
typedef void* HWND; 
#endif
#endif

#include <CSysRunState.h>
#include <Ice/Application.h>
#include <IceUtil/Monitor.h>
#include <IceUtil/IceUtil.h>
#include <iostream>
#include <list>
#include "MsgOpEx.h"

#define OUT
#define IN

using namespace std;
using namespace WiscomEV9000;
// 
// //���������
// class WorkQueue : public IceUtil::Thread  
// {
// public:
// 	WorkQueue();
// 	virtual ~WorkQueue();
// 	
//     virtual void run();
// 	void add(const AMD_EV9000MSI_OMMPToCMSInfoPtr& cb, 
// 		     const ::Ice::Int nSendType,
// 			 const BYTEBUFFER& sInData);
//     void destroy();
// 	
// private:
// 	
//     struct CallbackEntry
//     {
//         AMD_EV9000MSI_OMMPToCMSInfoPtr cb;
//         ::Ice::Int nSendType;
// 		BYTEBUFFER sInData;
//     };
// 	
//     IceUtil::Monitor<IceUtil::Mutex> _monitor;  //��
//     std::list<CallbackEntry> _callbacks;  //�������
//     bool _bIsRun;   //�Ƿ����б�־	
// };
// 
// typedef IceUtil::Handle<WorkQueue> WorkQueuePtr;

//����������ӿ���
class ServerIf : virtual public CSysRunState
{
public:
	
	//ServerI(const WorkQueuePtr& workQueue);
	ServerIf(){};
	~ServerIf(){};
	virtual ::Ice::Int SysRunState(const ::std::string& strInData, ::std::string& strOutData, const ::Ice::Current& cur = ::Ice::Current())
	//virtual ::Ice::Int SysRunState(::Ice::Int iOperatorType,const StructBuffer& inBuff, StructBuffer& outBuff, const ::Ice::Current& cur= ::Ice::Current())
	{
		cout <<endl<< "�յ��ͻ���ͬ�����ݵ�������" << endl;
        CMsgOpEx::GetSysRunState(strInData,strOutData);
		//return 0;
	}
private:
	//WorkQueuePtr _workQueue;
};

//Iceͨ����
class CIceComEx  
{
public:
	CIceComEx();
	virtual ~CIceComEx();

	BOOL Start(string strServId,int iServPort);  //��������Ice����
	BOOL Close();     //�رձ���Ice����
	
	//Ice�����߳�
	int	 OnIceServ();
#ifdef WIN32
	static DWORD WINAPI ThreadIce(LPVOID pUser)
	{
		CIceComEx* pThis = (CIceComEx*)pUser;
		return pThis->OnIceServ();
	}
#else
    static void * ThreadIce(void *pUser)
	{
		CIceComEx* pThis = (CIceComEx*)pUser;
		pThis->OnIceServ();
		return NULL; 
	}
#endif

	void SetTimeOut(int iTimeOut)
	{
		m_iTimeOut = iTimeOut;
		return ;
	}

	BOOL SetConPara(string strRmtIP,int iRmtPort, string strProxy);      //����Զ��Ice���Ӳ���
	BOOL SendData(string strInData,string& strOutData);
	BOOL SendData(char *pInBuf,int iInBufLen,char *pOutBuf,int iOutBufLen,int &iOutDataLen);

private:
	
	//BOOL ConverData2Ice(IN char *pInBuf,IN int iInBufLen,OUT string& strInData);
	//BOOL ConverIce2Data(IN string& strOutData,OUT char *pOutBuf,OUT int iOutBufLen,OUT int &iOutDataLen);
	
	BOOL	     m_bThreadIceFlag;

#ifdef WIN32
	HANDLE	     m_hThreadIce;
	DWORD	     m_dwThreadIceID;
#else	
	pthread_t    m_threadHandle;
#endif

	//���ط���
	string       m_strServId;
	int          m_iServPort;
    //WorkQueuePtr _workQueue;  //�������

	//Զ�˷���
	string       m_strProxy;
	string       m_strRmtIP;
	int          m_iRmtPort;

	int          m_iTimeOut;  //Ice ��ʱ�趨  ��λs
	//Ice::ObjectPrx m_base;

private:

	//�ͻ������Ӳ���
	Ice::CommunicatorPtr icClient;
	WiscomEV9000::CSysRunStatePrx prxClient;

};

#endif // !defined(AFX_ICECOMEX_H__AA54AA1D_C3A5_4BBC_B946_207470E4EF59__INCLUDED_)
