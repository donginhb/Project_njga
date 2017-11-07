// CtrlProOp.cpp: implementation of the CCtrlProOp class.
//
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include "stdafx.h"
#include "EV9000Mgw.h"
#include "MsgOp.h"
#endif
#include "ConfigOp.h"
#include "CtrlProOp.h"
#include <iostream>
#include <fstream>
using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifdef WIN32
extern CEV9000MgwApp theApp;
#endif

#define  BASE_PORT   10000

//初始化
unsigned short CCtrlProOp::m_usLocalPort = BASE_PORT;

#ifdef WIN32
CCritSec   CCtrlProOp::m_lstMsgLock;
CMutex CCtrlProOp::mutex;
#else
pthread_mutex_t	CCtrlProOp::m_lstMsgLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t CCtrlProOp::mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
list<CBSIPMSG> CCtrlProOp::m_lstMsg;
list<CBSIPMSG> CCtrlProOp::m_lstIntimeMsg;

CCtrlProOp * CCtrlProOp::instance = 0;
CBSIPMSG  CCtrlProOp::m_DoingMsg;

CCtrlProOp::CCtrlProOp()
{
	memset(&m_stLoginInfo,0,sizeof(m_stLoginInfo));
	m_hMsg =-1;
	m_hSip =-1;
	m_bInited = FALSE;
#ifndef WIN32
	pthread_mutex_init(&m_csOpLock,NULL);
	pthread_mutex_init(&m_csMapDialogLock,NULL);
#endif
}

CCtrlProOp::~CCtrlProOp()
{
	Fini();
}

CCtrlProOp * CCtrlProOp::Instance()
{
#ifdef WIN32
	mutex.Lock();
	if (instance == 0) {
		//Sleep(1000);
        instance = new CCtrlProOp();
		instance->Init();
	}
	mutex.Unlock();
	return instance;
#else
	CCtrlProOp * instanceTmp =NULL;
	if(NULL == instance)
	{
		CAutoLock theLock(&mutex);  //?????
		if(NULL == instance)
		{
			instanceTmp=new CCtrlProOp();
			if(NULL == instanceTmp)
			{
				mgwlog("CtrlProOp new fail\n");
				exit(1);
			}
			instanceTmp->Init();
		    instance = instanceTmp;
		}
	}
	return instance;
#endif
}

//对应SIPWrapper 接口
int CCtrlProOp::Init()
{
	int nRet = CSipWrapper::Init();   // 全局初始化
	CSipMessage::SetMessageCallBack(CBMessage); //设置被动接收message消息回调函数
	CSipInvite::SetInviteRecvCallBack(CBInvite);  // 设置被动接收invite 消息回调函数
   	app_set_bye_received_cb(&CBBye,NULL);  
	app_set_cancel_received_cb(&CBCancel, NULL);
	app_set_ua_session_expires_cb(&CBExpire);
	app_set_info_received_cb(&CBinfo_received_proc,NULL);
	m_hSip = CSipWrapper::CreateSipWrapper();
	m_bInited = TRUE;
	return 0;
}

BOOL CCtrlProOp::Fini()
{
	if(m_bInited)
	{
		m_hMsg =-1;
		m_hSip =-1;
		CSipWrapper::Free();
		m_bInited = FALSE;
	}
	return TRUE;
}
 
int  CCtrlProOp::SendMsg(SIP_MSG_INFO &msg_info)
{
    CAutoLock lock(&m_csOpLock);
	if(m_hSip <0)
	{
		m_hSip = CSipWrapper::CreateSipWrapper();
		if(m_hSip<0)
		{
			mgwlog("CCtrlProOp::SendMsg() m_hSip 错误\n");
			return -1;
		}
	}
	// 主动发送message信息
	if(m_hMsg < 0 )
	{
		m_hMsg=CSipWrapper::g_SipWrapperList[m_hSip]->GetFreeMessage();
		if(m_hMsg<0)
		{
			mgwlog("CCtrlProOp::SendMsg() m_hMsg错误\n");
			return -1;
		}	
	}
	
	int nRet=CSipWrapper::g_SipWrapperList[m_hSip]->SendMessageEx(m_hMsg,msg_info);
	if (nRet==0)
	{
		//发送成功，如果要访问接收到的数据可以这样访问
		//	CSipMessage* pMessage=CSipWrapper::g_SipWrapperList[m_hSip]->GetMessageByHandle(nMessageHadle);
	//	mgwlog("SendMessage success\n");
		// 数据存储在 pMessage->m_pMsgRecvList  列表中 根据具体业务用xml解析
		//   mgwlog("msg:::%s\n",pMessage->m_pMsgRecvList[0]->recvMsg);
	}
	//CSipWrapper::g_SipWrapperList[m_hSip]->FreeMessage(nMessageHandle);    // 删除message对象
	return nRet;
}

int CCtrlProOp::GetFreePort()
{
	if(CCtrlProOp::m_usLocalPort<BASE_PORT || CCtrlProOp::m_usLocalPort > BASE_PORT+10000)
	{
		CCtrlProOp::m_usLocalPort=BASE_PORT;
	}
   return (CCtrlProOp::m_usLocalPort+=2);  //jrtp 不支持奇数端口
}

//优先查找及时队列，再查找普通队列
BOOL CCtrlProOp::GetMsg(CBSIPMSG &stCbSipmsg)
{
	CAutoLock lock(&m_lstMsgLock);
	//优先查找及时队列
	list<CBSIPMSG>::iterator iter= m_lstIntimeMsg.begin();
	if(iter != m_lstIntimeMsg.end())
	{
		stCbSipmsg = *iter;
		CCtrlProOp::m_DoingMsg = stCbSipmsg;  //保存正在处理的消息
		m_lstIntimeMsg.erase(iter);
		return TRUE;
	}
	//及时队列没有找到，再查找普通队列
	iter= m_lstMsg.begin();
	if(iter != m_lstMsg.end())
	{
		stCbSipmsg = *iter;
		CCtrlProOp::m_DoingMsg = stCbSipmsg;  //保存正在处理的消息
		m_lstMsg.erase(iter);
		return TRUE;
	}else{
		return  FALSE;
	}
}

int CCtrlProOp::GetMsgLen()
{
	CAutoLock lock(&m_lstMsgLock);
	return m_lstMsg.size();
}

//设置注册信息
int CCtrlProOp::SetLoginInfo(EV9000_LOGININFO &loginInfo)
{
	memcpy(&(m_stLoginInfo.sServerIP[0]),&(loginInfo.sServerIP[0]),EV9000_NORMAL_STRING_LEN);  // 服务器IP
	m_stLoginInfo.nServerPort = loginInfo.nServerPort ;                                        // 服务器端口
	memcpy(&(m_stLoginInfo.sUserName[0]),&(loginInfo.sUserName[0]),EV9000_NORMAL_STRING_LEN);
	memcpy(&(m_stLoginInfo.sUserPwd[0]),&(loginInfo.sUserPwd[0]),EV9000_NORMAL_STRING_LEN);
	memcpy(&(m_stLoginInfo.sServerID[0]),&(loginInfo.sServerID[0]),EV9000_NORMAL_STRING_LEN);  // 服务器编号
	memcpy(&(m_stLoginInfo.sUserID[0]),&(loginInfo.sUserID[0]),EV9000_NORMAL_STRING_LEN);
	memcpy(&(m_stLoginInfo.sLocalIP[0]),&(loginInfo.sLocalIP[0]),EV9000_NORMAL_STRING_LEN);
	m_stLoginInfo.nDigital = loginInfo.nDigital;                                               // 是否使用数字证书登录 0 不使用 1 使用
	memcpy(&(m_stLoginInfo.sReserved[0]),&(loginInfo.sReserved[0]),EV9000_NORMAL_STRING_LEN);
	m_stLoginInfo.nReserved = loginInfo.nReserved;
	
	return 0;
} 

//向CMS注册
//int CCtrlProOp::Register(EV9000_LOGININFO &loginInfo)
int CCtrlProOp::Register()
{
	CAutoLock lock(&m_csOpLock);
	//申请一个Sip对象
//	if(m_hSip<0)
//	{
		Fini();
	    Init();   //初始化服务监听端口
		m_hSip = CSipWrapper::CreateSipWrapper();
		if(m_hSip<0)
		{
			mgwlog("CCtrlProOp::Register() m_hSip 错误\n");
			return -1;
		}	
//	}
	mgwlog("CCtrlProOp::Register() m_hSip:%d\n",m_hSip);
	// 注册
    CSipWrapper::g_SipWrapperList[m_hSip]->SetLoginInfo(m_stLoginInfo);

	int iTimeOut = MAX_SIP_TIMEOUT;
	string strTimeOut = CSysCfg::Instance()->GetstrPara(SYS_CFG_SIPREFRESHTIME);
	iTimeOut = atoi(strTimeOut.c_str())*2;  //刷新时间的2倍
	return CSipWrapper::g_SipWrapperList[m_hSip]->SendRegister(iTimeOut,2);
}

int CCtrlProOp::RecordDialogIndex(int dialog_index,string& callee_id)
{
	CAutoLock lock(&m_csMapDialogLock);
	m_mapDialog[dialog_index] = callee_id;
	int nLen = m_mapDialog.size();
	mgwlog("增加会话，会话队列size:%d\n",nLen);
	if(nLen>1024)
	{
		mgwlog("会话队列size:%d 超过1024,重启程序\n",nLen);
		exit(-1);
	}
	return 0;
}

int CCtrlProOp::GetIDByDialogIndex(int dialog_index,string& callee_id)
{
	CAutoLock lock(&m_csMapDialogLock);
	map<int,string>::iterator iter=m_mapDialog.find(dialog_index);
	if(iter!=m_mapDialog.end())
	{
		callee_id = iter->second;
	}else{
		callee_id = ""; 
	}
	return 0;
}
int CCtrlProOp::DelDialogIndex(int dialog_index)
{
	CAutoLock lock(&m_csMapDialogLock);
	map<int,string>::iterator iter=m_mapDialog.find(dialog_index);
	if(iter!=m_mapDialog.end())
	{
		m_mapDialog.erase(iter);
		mgwlog("删除会话，会话队列size:%d\n",m_mapDialog.size());
	}
	return 0;
}

#ifdef WIN32
void TimetToSystemTime( time_t t, LPSYSTEMTIME pst )
{
	FILETIME ft;
	LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;
	ft.dwLowDateTime = (DWORD) ll;
	ft.dwHighDateTime = (DWORD)(ll >> 32);
	
	FileTimeToSystemTime( &ft, pst );
}
#endif

int CCtrlProOp::RefreshReg()
{
	if(m_hSip<0)
	{
		mgwlog("CCtrlProOp::RefreshReg() m_hSip<0 错误\n");
		return -1;
	}
	int nRet = CSipWrapper::g_SipWrapperList[m_hSip]->RefreshRegister();
    if(0 == nRet)  //刷新成功 更新本地时间  //added by chenyu
	{
		mgwlog("CCtrlProOp::RefreshReg() 刷新成功 更新本地时间\n");
#ifdef WIN32
		SYSTEMTIME stTime;
        unsigned int uiTime = CSipWrapper::g_SipWrapperList[m_hSip]->GetTime();
		mgwlog("m_hSip:%d 更新本地时间:%u",m_hSip,uiTime);
        if(uiTime>0)
		{
			uiTime +=8*3600;   //linux utc时间转换为当地时间（相差8小时）
			TimetToSystemTime(uiTime,&stTime);
			SetLocalTime(&stTime);
		}
#endif
	}else{
		mgwlog("CCtrlProOp::RefreshReg() 刷新失败\n");
	}
	return nRet;
}

//message消息接收回调
int CCtrlProOp::CBMessage(char* caller_id, char* callee_id, char* call_id, char* pMsg, int nMsgLen)  
{
	//mgwlog("receive message msg caller_id:%s callee_id:%s call_id:%s\n",caller_id,callee_id,call_id);
	CBSIPMSG stSipMsg;
	stSipMsg.eSipMsgType = SIP_MSG_MSG;
	stSipMsg.strcaller_id = caller_id;
	stSipMsg.strcallee_id = callee_id;
	stSipMsg.strcall_id = call_id;
	stSipMsg.strMsg = pMsg;
	stSipMsg.nMsgLen = nMsgLen;

// 	if(stSipMsg.strMsg.find("Catalog")!=string::npos)
// 	{
// 		CAutoLock lock(&m_lstMsgLock);
// 		mgwlog("消息队列size:%d\n",CCtrlProOp::m_lstMsg.size());
// 		//mgwlog("Doing msg is:%s\n",CCtrlProOp::m_DoingMsg.strMsg.c_str());
// 	}

#ifdef MTDEALMSG
	//解析XML指令
	MsgOp oMsgOp;
	oMsgOp.InputMsg((char *)stSipMsg.strMsg.c_str());
	string  strMsgType,strCmdType;
	strMsgType = oMsgOp.GetMsgType();
	strCmdType = oMsgOp.GetMsgText("CmdType");
	
	CAutoLock lock(&m_lstMsgLock);
	//标识消息等级
	if( ("Control" == strMsgType) && (strCmdType == "DeviceControl"))  //控球命令
	{
		CCtrlProOp::m_lstIntimeMsg.push_back(stSipMsg);  //及时消息队列中
	}else{   //其它命令
		CCtrlProOp::m_lstMsg.push_back(stSipMsg);        //普通消息队列
	}
#else
	CAutoLock lock(&m_lstMsgLock);
	CCtrlProOp::m_lstMsg.push_back(stSipMsg);            //压到普通消息队列中
#endif
	//1.临时处理 判断消息积压数 2.对关闭加监视线程CStartupGuard，超时重启   //Todo for chenyu 20140830
	int nCount = CCtrlProOp::m_lstMsg.size();
	if(nCount>40)
	{
		mgwlog("sip消息队列size:%d 积压超过40个,可能操作阻塞，退出重启进程\n",nCount);
		exit(1);  //退出重启
	}
	mgwlog("sip消息队列size:%d\n",nCount);
	return 0;
}

//Invite消息回调
int CCtrlProOp::CBInvite(char* caller_id, char* callee_id, char* call_id, int dialog_index)
{
	//mgwlog("receive Invite msg caller_id:%s callee_id:%s call_id:%s\n",caller_id,callee_id,call_id);
	CBSIPMSG stSipMsg;
	stSipMsg.eSipMsgType = SIP_MSG_INVITE;
	stSipMsg.strcaller_id = caller_id;
	stSipMsg.strcallee_id=callee_id;
	stSipMsg.strcall_id=call_id;
    stSipMsg.idialog_index = dialog_index;
	CAutoLock lock(&m_lstMsgLock);
	CCtrlProOp::m_lstMsg.push_back(stSipMsg);  //压到消息队列中
	return 0;
}

//Bye消息回调
int CCtrlProOp::CBBye(char* caller_id, char* callee_id, char* call_id, int dialog_index,int status_code)
{
	//mgwlog("receive Bye msg caller_id:%s callee_id:%s call_id:%s\n",caller_id,callee_id,call_id);
	CBSIPMSG stSipMsg;
	stSipMsg.eSipMsgType = SIP_MSG_BYE;
	stSipMsg.strcaller_id = caller_id;
	stSipMsg.strcallee_id=callee_id;
	stSipMsg.strcall_id=call_id;
    stSipMsg.idialog_index = dialog_index;
	CAutoLock lock(&m_lstMsgLock);
	CCtrlProOp::m_lstMsg.push_back(stSipMsg);  //压到消息队列中
	return 0;
} 

//cancel消息回调
int CCtrlProOp::CBCancel(char* caller_id, char* callee_id, char* call_id, int dialog_index, int userdata)
{
	//mgwlog("receive cancel msg caller_id:%s callee_id:%s call_id:%s\n",caller_id,callee_id,call_id);
	CBSIPMSG stSipMsg;
	stSipMsg.eSipMsgType = SIP_MSG_CANCEL;
	stSipMsg.strcaller_id = caller_id;
	stSipMsg.strcallee_id=callee_id;
	stSipMsg.strcall_id=call_id;
    stSipMsg.idialog_index = dialog_index;
	CAutoLock lock(&m_lstMsgLock);
	CCtrlProOp::m_lstMsg.push_back(stSipMsg);  //压到消息队列中
	return 0;
} 

//info消息回调
int CCtrlProOp::CBinfo_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* call_id, int dialog_index, char* msg_body,int msg_len,int user_data)
{
	//mgwlog("receive cancel msg caller_id:%s callee_id:%s call_id:%s\n",caller_id,callee_id,call_id);
	CBSIPMSG stSipMsg;
	stSipMsg.eSipMsgType = SIP_MSG_INFORM;
	stSipMsg.strcaller_id = caller_id;
	stSipMsg.strcallee_id=callee_id;
	stSipMsg.strcall_id=call_id;
    stSipMsg.idialog_index = dialog_index;
	stSipMsg.strMsg = msg_body;
	stSipMsg.nMsgLen = msg_len; 
	CAutoLock lock(&m_lstMsgLock);
	CCtrlProOp::m_lstMsg.push_back(stSipMsg);  //压到消息队列中
	return 0;
}

//超时回调
int CCtrlProOp::CBExpire(int dialog_index)
{
	//mgwlog("receive cancel msg caller_id:%s callee_id:%s call_id:%s\n",caller_id,callee_id,call_id);
	CBSIPMSG stSipMsg;
	stSipMsg.eSipMsgType = SIP_MSG_EXPIRE;
	stSipMsg.strcaller_id = "";
	stSipMsg.strcallee_id= "";
	stSipMsg.strcall_id= "";
    stSipMsg.idialog_index = dialog_index;
	CAutoLock lock(&m_lstMsgLock);
	CCtrlProOp::m_lstMsg.push_back(stSipMsg);  //压到消息队列中
	return 0;
} 
