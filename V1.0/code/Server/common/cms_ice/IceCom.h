// IceCom.h: interface for the CIceCom class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ICECOM_H__E5AB0B82_B6B8_41A5_BF05_31C605C64349__INCLUDED_)
#define AFX_ICECOM_H__E5AB0B82_B6B8_41A5_BF05_31C605C64349__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <iostream>
#include <string>
#include <Ice/Ice.h> 
#include <Ice/Communicator.h>  
#include "EV9000_ManageService.h"
#ifdef WIN32
#include "../../app/public/Include/IEV9000Ice.h"
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

#define  MAX_PAYLOAD_LEN    (32*1024)

typedef struct tag_EDVHEAD
{
	unsigned short wVersions;   /*标识不同版本，从低位到高位，每位置1：支持该版本；0：不支持*/
	unsigned short wPayLoadLen; /*载荷长度*/
	unsigned int   dwEvent;     /*Word32  事件类型*/
	unsigned short wResved1;    /*保留*/
	unsigned short wResved2;    /*保留*/
	unsigned int   dwComonID;   /*命令ID*/
	unsigned int   dwDestIP;    /*IP*/
	char           dummy[16];   /*填充*/
	unsigned char  ucPayLoad[MAX_PAYLOAD_LEN]; /*BYTE[1400]  Payload部分*/
	tag_EDVHEAD()
	{
		memset(this,0,sizeof(tag_EDVHEAD));
		//ZeroMemory(this,sizeof(tag_EDVHEAD));
		wPayLoadLen =0;
	}
}EDVHEAD_EX,*PEDVHEAD_EX;

#endif

#include "ServerI.h"
#include "ClientI.h"

using namespace std;
using namespace EV9000MS;

#define OUT
#define IN

//Ice通信类
class CIceCom  
{
public:
	CIceCom();
	virtual ~CIceCom(); 

	BOOL Start(string strServId,int iServPort);  //启动本机Ice服务
	BOOL Close();     //关闭本机Ice服务
	
	//Ice服务线程
	int	 OnIceServ();
#ifdef WIN32
	static DWORD WINAPI ThreadIce(LPVOID pUser)
	{
		CIceCom* pThis = (CIceCom*)pUser;
		return pThis->OnIceServ();
	}
#else
    static void * ThreadIce(void *pUser)
	{
		CIceCom* pThis = (CIceCom*)pUser;
		pThis->OnIceServ();
		return NULL; 
	}
#endif

	void SetTimeOut(int iTimeOut)
	{
		m_iTimeOut = iTimeOut;
		return ;
	}

	BOOL SetConPara(string strRmtIP,int iRmtPort, string strProxy);      //设置远端Ice连接参数
	BOOL SendData(int nSendType, EDVHEAD_EX &tInData,EDVHEAD_EX & tOutData); //nSendType 0--同步 1--异步

private:
	
	BOOL ConverData2Ice( IN const EDVHEAD_EX &tInData,OUT COMMNMSGHEAD &tComMsg,OUT BYTEBUFFER& sInData);
	BOOL ConverIce2Data( IN COMMNMSGHEAD &tOutComMsg,IN BYTEBUFFER& sOutData,OUT EDVHEAD_EX &tOutData);

	BOOL	     m_bThreadIceFlag;

#ifdef WIN32
	HANDLE	     m_hThreadIce;
	DWORD	     m_dwThreadIceID;
#else	
	pthread_t    m_threadHandle;
#endif

	//本地服务
	string       m_strServId;
	int          m_iServPort;
    WorkQueuePtr _workQueue;  //任务队列

	//远端服务
	string       m_strProxy;
	string       m_strRmtIP;
	int          m_iRmtPort;

	int          m_iTimeOut;  //Ice 超时设定  单位s
	//Ice::ObjectPrx m_base;

private:

	//客户端连接参数
	Ice::CommunicatorPtr icClient;
	EV9000MS::EV9000MSIPrx prxClient;

};


#endif // !defined(AFX_ICECOM_H__E5AB0B82_B6B8_41A5_BF05_31C605C64349__INCLUDED_)
