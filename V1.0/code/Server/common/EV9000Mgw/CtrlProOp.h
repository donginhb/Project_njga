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

#include "sip/SipWrapper.h"   //sipЭ��
#include <iostream>
#include <string>
#include <list>
#include <map>
using namespace std;

#ifndef WIN32
#include <pthread.h>
#endif

//Э�����ģ��
//CCtrlProOp Singleton
class CCtrlProOp  
{
public:
	static CCtrlProOp * Instance();
	virtual ~CCtrlProOp();

	///////////////////////////
	//----SIPЭ�鴦��ӿ�------
	///////////////////////////
	int   Init();
	BOOL  Fini();
	int   SetLoginInfo(EV9000_LOGININFO &loginInfo);  //����ע����Ϣ
	int   Register();
	int   RecordDialogIndex(int dialog_index,string& callee_id);
	int    GetIDByDialogIndex(int dialog_index,string& callee_id);
    int    DelDialogIndex(int dialog_index);
	//int   Register(EV9000_LOGININFO &loginInfo);   
	int   RefreshReg();     //ˢ��ע��30s
	int   GetFreePort();
	int   SendMsg(SIP_MSG_INFO &msg_info);
    BOOL  GetMsg(CBSIPMSG &stCbSipmsg);  //���Ȳ��Ҽ�ʱ���У��ٲ�����ͨ����
	int   GetMsgLen();

	static int CBMessage(char* caller_id, char* callee_id, char* call_id, char* pMsg, int nMsgLen);  //Sip��Ϣ���ջص�
	static int CBInvite(char* caller_id, char* callee_id, char* call_id, int dialog_index); //Invite��Ϣ�ص�
	static int CBBye(char* caller_id, char* callee_id, char* call_id, int dialog_index,int status_code); //Bye��Ϣ�ص�
  
	static int CBCancel(char* caller_id, char* callee_id, char* call_id, int dialog_index, int userdata); //cancel��Ϣ�ص�
	static int CBinfo_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* call_id, int dialog_index, char* msg_body,int msg_len,int user_data); //info��Ϣ�ص�

	/* UA �Ự��ʱ�ص�����
         ����: dialog_index,�Ự�������
    */
    static int CBExpire(int dialog_index);

	///////////////////////////
	//----����Э�鴦��ӿ�------
	///////////////////////////

private:
	CCtrlProOp();

	static unsigned short  m_usLocalPort;   //���ض˿�

#ifdef WIN32
	static CCritSec        m_lstMsgLock;
#else
	static pthread_mutex_t	m_lstMsgLock;
#endif
	static list<CBSIPMSG>  m_lstMsg;         //�յ�����Ϣ����
	static list<CBSIPMSG>  m_lstIntimeMsg;   //��ʱ��Ϣ����
	static CBSIPMSG        m_DoingMsg;       //���ڴ������Ϣ
#ifdef WIN32
	CCritSec          m_csOpLock;        //Send��Ϣlock
#else
    pthread_mutex_t m_csOpLock;
#endif
	BOOL m_bInited;   //�Ƿ��ʼ��
    EV9000_LOGININFO  m_stLoginInfo;   //��¼��Ϣ

#ifdef WIN32
	CCritSec         m_csMapDialogLock; //mapdialog��lock
#else
    pthread_mutex_t  m_csMapDialogLock;
#endif
	map<int,string>  m_mapDialog;       //dialog��Ӧ��ϵ

	int m_hMsg;    //SipMsg Handle
	int m_hSip;    //SipЭ��ģ�������

#ifdef WIN32
	static CMutex mutex;
#else
	static pthread_mutex_t mutex;
#endif
	static CCtrlProOp * instance;
};


#endif // !defined(AFX_CTRLPROOP_H__F456EAD4_360E_4119_B8B4_87D82BF5EB4C__INCLUDED_)
