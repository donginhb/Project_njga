// CenterCtrl.h: interface for the CCenterCtrl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CENTERCTRL_H__27EF54C0_9D0A_42E5_9437_76F3D168B449__INCLUDED_)
#define AFX_CENTERCTRL_H__27EF54C0_9D0A_42E5_9437_76F3D168B449__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CtrlProOp.h"     //Protocol
#include "ConfigOp.h"
#include "DevCtrl.h"       //device op
#include "MgwPublicData.h"

#define THREAD_DEALMSG_MAX        2   //消息最大处理线程数

typedef struct _thpara
{
	LPVOID p;
	int    iHandle;
}THPARA;

//控制模块
class CCenterCtrl  
{
public:
	CCenterCtrl();
	virtual ~CCenterCtrl();
	
	BOOL Start();
	BOOL Close();
	
	//命令处理
	int  DealMsg(CBSIPMSG &stCbSipmsg);
	int  DoDeviceInfoCmd(CBSIPMSG &stCbSipmsg);
	int  DoCatalogCmd(CBSIPMSG &stCbSipmsg);
	int  DoRecordQuery(CBSIPMSG &stCbSipmsg);
	int  DoInviteCmd(CBSIPMSG &stCbSipmsg);
	int  DoInform(CBSIPMSG &stCbSipmsg);    //inform
	int  DoByeCmd(CBSIPMSG &stCbSipmsg);
	int  DoDeviceControlCmd(CBSIPMSG &stCbSipmsg);
    
	int  DoSetDeviceVideoParamCmd(CBSIPMSG &stCbSipmsg);
	int  DoGetDeviceVideoParamCmd(CBSIPMSG &stCbSipmsg);
	int  DoRequestIFrameDataCmd(CBSIPMSG &stCbSipmsg);
	int  DoAutoZoomInCmd(CBSIPMSG &stCbSipmsg);
#ifdef WIN32
	string CovertSystime2ISO(SYSTEMTIME systime);
	int  SendEmptyRecordMsg(CBSIPMSG &stCbSipmsg);
#endif
private:

	BOOL InitDevice();          //设备初始化
	BOOL FiniDevice();          //设备关闭
    BOOL Register();            //注册
	BOOL StartDealMsg();    	//创建指令处理线程         
	int	 OnDealMsg(int iHandle);//Sipmsg消息处理线程
    int	 OnRefresh();           //刷新线程
#ifdef WIN32
	static DWORD WINAPI ThreadDealMsg(LPVOID pPara)
	{
		THPARA *pThpara = (THPARA *) pPara;
		CCenterCtrl* pThis = (CCenterCtrl*)(pThpara->p);
		return pThis->OnDealMsg(pThpara->iHandle);
	}

	static DWORD WINAPI ThreadRefresh(LPVOID pUser)
	{
		CCenterCtrl* pThis = (CCenterCtrl*)pUser;
		return pThis->OnRefresh();
	}
	
	int	 OnInfoRecv();   //消息接收线程
	static DWORD WINAPI ThreadInfoRecv(LPVOID pUser)
	{
		CCenterCtrl* pThis = (CCenterCtrl*)pUser;
		return pThis->OnInfoRecv();
	}
#else
	static void* ThreadDealMsg(LPVOID pPara)
	{
		THPARA *pThpara = (THPARA *) pPara;
		CCenterCtrl* pThis = (CCenterCtrl*)(pThpara->p);
		pThis->OnDealMsg(pThpara->iHandle);
		return NULL;
	}

	static void* ThreadRefresh(LPVOID pUser)
	{
		CCenterCtrl* pThis = (CCenterCtrl*)pUser;
		pThis->OnRefresh();
		return NULL;
	}
#endif

public:
	string        m_strMgwDeviceID;        //媒体网关ID
	CDevCtrl      *m_pDevCtrl;
	THPARA        m_stThpara[THREAD_DEALMSG_MAX];       

	//打开文件线程
	BOOL		  m_bThreadDealMsgFlag[THREAD_DEALMSG_MAX];
	BOOL	      m_bThreadRefreshFlag;    //刷新注册 
#ifdef WIN32
	HANDLE		  m_hThreadDealMsg[THREAD_DEALMSG_MAX];
	DWORD		  m_dwThreadDealMsgID[THREAD_DEALMSG_MAX];

	HANDLE	      m_hThreadRefresh;
	DWORD	      m_dwThreadRefreshID;
	
	CUdp2*        m_lpInfoUdp;   //日记接收线程
	BOOL		  m_bThreadInfoRecvFlag;
	HANDLE		  m_hThreadInfoRecv;
	DWORD		  m_dwThreadInfoRecvID;
#else
	pthread_t     m_threadDealMsgHandle[THREAD_DEALMSG_MAX];
	pthread_t     m_threadRefreshHandle;
#endif
};

#endif // !defined(AFX_CENTERCTRL_H__27EF54C0_9D0A_42E5_9437_76F3D168B449__INCLUDED_)
