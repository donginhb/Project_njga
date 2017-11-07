// CtrlProOp.h: interface for the CCtrlProOp class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CTRLPROOP_H__F456EAD4_360E_4119_B8B4_87D82BF5EB4C__INCLUDED_)
#define AFX_CTRLPROOP_H__F456EAD4_360E_4119_B8B4_87D82BF5EB4C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MgwPublicData.h"
#ifdef WIN32
#include "Afxmt.h"
#else
#include "AutoLock/CAutoLock.h"
#endif

#include "sip/SipWrapper.h"   //sip协议
#include <iostream>
#include <string>
#include <list>
#include <map>
using namespace std;

#ifndef WIN32
#include <pthread.h>
#endif

//协议操作模块
//CCtrlProOp Singleton
class CCtrlProOp  
{
public:
	static CCtrlProOp * Instance();
	virtual ~CCtrlProOp();

	///////////////////////////
	//----SIP协议处理接口------
	///////////////////////////
	int   Init();
	BOOL  Fini();
	int   SetLoginInfo(EV9000_LOGININFO &loginInfo);  //设置注册信息
	int   Register();
	int   RecordDialogIndex(int dialog_index,string& callee_id);
	int    GetIDByDialogIndex(int dialog_index,string& callee_id);
    int    DelDialogIndex(int dialog_index);
	//int   Register(EV9000_LOGININFO &loginInfo);   
	int   RefreshReg();     //刷新注册30s
	int   GetFreePort();
	int   SendMsg(SIP_MSG_INFO &msg_info);
    BOOL  GetMsg(CBSIPMSG &stCbSipmsg);  //优先查找及时队列，再查找普通队列
	int   GetMsgLen();

	static int CBMessage(char* caller_id, char* callee_id, char* call_id, char* pMsg, int nMsgLen);  //Sip消息接收回调
	static int CBInvite(char* caller_id, char* callee_id, char* call_id, int dialog_index); //Invite消息回调
	static int CBBye(char* caller_id, char* callee_id, char* call_id, int dialog_index,int status_code); //Bye消息回调
  
	static int CBCancel(char* caller_id, char* callee_id, char* call_id, int dialog_index, int userdata); //cancel消息回调
	static int CBinfo_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* call_id, int dialog_index, char* msg_body,int msg_len,int user_data); //info消息回调

	/* UA 会话超时回调函数
         参数: dialog_index,会话句柄索引
    */
    static int CBExpire(int dialog_index);

	///////////////////////////
	//----其它协议处理接口------
	///////////////////////////

private:
	CCtrlProOp();

	static unsigned short  m_usLocalPort;   //本地端口

#ifdef WIN32
	static CCritSec        m_lstMsgLock;
#else
	static pthread_mutex_t	m_lstMsgLock;
#endif
	static list<CBSIPMSG>  m_lstMsg;         //收到的消息队列
	static list<CBSIPMSG>  m_lstIntimeMsg;   //及时消息队列
	static CBSIPMSG        m_DoingMsg;       //正在处理的消息
#ifdef WIN32
	CCritSec          m_csOpLock;        //Send消息lock
#else
    pthread_mutex_t m_csOpLock;
#endif
	BOOL m_bInited;   //是否初始化
    EV9000_LOGININFO  m_stLoginInfo;   //登录信息

#ifdef WIN32
	CCritSec         m_csMapDialogLock; //mapdialog的lock
#else
    pthread_mutex_t  m_csMapDialogLock;
#endif
	map<int,string>  m_mapDialog;       //dialog对应关系

	int m_hMsg;    //SipMsg Handle
	int m_hSip;    //Sip协议模块对象句柄

#ifdef WIN32
	static CMutex mutex;
#else
	static pthread_mutex_t mutex;
#endif
	static CCtrlProOp * instance;
};


#endif // !defined(AFX_CTRLPROOP_H__F456EAD4_360E_4119_B8B4_87D82BF5EB4C__INCLUDED_)
