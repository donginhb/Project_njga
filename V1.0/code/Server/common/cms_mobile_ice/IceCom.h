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
	unsigned short wVersions;   /*��ʶ��ͬ�汾���ӵ�λ����λ��ÿλ��1��֧�ָð汾��0����֧��*/
	unsigned short wPayLoadLen; /*�غɳ���*/
	unsigned int   dwEvent;     /*Word32  �¼�����*/
	unsigned short wResved1;    /*����*/
	unsigned short wResved2;    /*����*/
	unsigned int   dwComonID;   /*����ID*/
	unsigned int   dwDestIP;    /*IP*/
	char           dummy[16];   /*���*/
	unsigned char  ucPayLoad[MAX_PAYLOAD_LEN]; /*BYTE[1400]  Payload����*/
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

//Iceͨ����
class CIceCom  
{
public:
	CIceCom();
	virtual ~CIceCom(); 

	BOOL Start(string strServId,int iServPort);  //��������Ice����
	BOOL Close();     //�رձ���Ice����
	
	//Ice�����߳�
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

	BOOL SetConPara(string strRmtIP,int iRmtPort, string strProxy);      //����Զ��Ice���Ӳ���
	BOOL SendData(int nSendType, EDVHEAD_EX &tInData,EDVHEAD_EX & tOutData); //nSendType 0--ͬ�� 1--�첽

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

	//���ط���
	string       m_strServId;
	int          m_iServPort;
    WorkQueuePtr _workQueue;  //�������

	//Զ�˷���
	string       m_strProxy;
	string       m_strRmtIP;
	int          m_iRmtPort;

	int          m_iTimeOut;  //Ice ��ʱ�趨  ��λs
	//Ice::ObjectPrx m_base;

private:

	//�ͻ������Ӳ���
	Ice::CommunicatorPtr icClient;
	EV9000MS::EV9000MSIPrx prxClient;

};


#endif // !defined(AFX_ICECOM_H__E5AB0B82_B6B8_41A5_BF05_31C605C64349__INCLUDED_)
