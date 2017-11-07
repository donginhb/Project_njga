#ifdef WIN32
#include <afxwin.h>
#else
#include "AutoLock/CAutoLock.h"
#endif
#include "SipWrapper.h"

// 全局函数


CSipWrapper* GetSipByRegIndex(int nRegIndex)
{
	for (int i=0;i<MAX_SIPWRAPPER_COUNT;i++)
	{
		if (CSipWrapper::g_SipWrapperList[i])
		{
			if (nRegIndex==CSipWrapper::g_SipWrapperList[i]->GetRegIndex())
			{
				return CSipWrapper::g_SipWrapperList[i];
			}
		}
	}
	return NULL;
}

CSipInvite* GetInviteByDialogIndex(int dialog_index)
{
	for (int i=0;i<MAX_SIPWRAPPER_COUNT;i++)
	{
		if (CSipWrapper::g_SipWrapperList[i])
		{
			for (int j=0;j<MAX_INVITE_COUNT;j++)
			{
				if (CSipWrapper::g_SipWrapperList[i]->m_pSipInviteList[j])
				{
					if (dialog_index==CSipWrapper::g_SipWrapperList[i]->m_pSipInviteList[j]->GetDialogIndex())
					{
						return CSipWrapper::g_SipWrapperList[i]->m_pSipInviteList[j];
					}
				}
			}
		}
	}
	return NULL;
}

CSipMessage* GetMessageBySN(int nSN)
{
	for (int i=0;i<MAX_SIPWRAPPER_COUNT;i++)
	{
		if (CSipWrapper::g_SipWrapperList[i])
		{
			CSipMessage* pMessage=CSipWrapper::g_SipWrapperList[i]->GetMessageBySN(nSN);
			if (pMessage)
			{
				return pMessage;
			}			
		}
	}
	return NULL;
}

const char* GetAttributeByName( TiXmlElement* pRootEle, const char* strAttributeName )
{	
	
    TiXmlElement* pEle = pRootEle;    
    for (pEle = pRootEle->FirstChildElement(); pEle; pEle = pEle->NextSiblingElement())    
    {    
        // recursive find sub node return node pointer    
        if (pEle->Attribute(strAttributeName)>0)  
        {  
            return pEle->Attribute(strAttributeName);
        }  
        else  
        { 
			GetAttributeByName(pEle,strAttributeName);  
        }  
    }    
	
    return NULL;  
}

//全局变量初始化
int CSipWrapper::g_SipRecvPort=0;
CSipWrapper* CSipWrapper::g_SipWrapperList[MAX_SIPWRAPPER_COUNT]={0};


CSipMessage::CSipMessage( int nID )
{
	m_nRecvMsgLen=0;
	ZeroMemory(m_sCallID,EV9000_LONG_STRING_LEN);
#ifdef WIN32
	m_hResponseEvent=CreateEvent(NULL, FALSE, FALSE, NULL);
#else
    m_hResponseEvent= new CMyEvent(TRUE);
#endif
	for (int i=0;i<MAX_RECV_MSG_COUNT;i++)
	{
		m_pMsgRecvList[i]=NULL;
	}
	m_nToRecvItemCount=0;    
	m_nRecvItemCount=0;   
	ZeroMemory(&m_sipMsgInfo,sizeof(SIP_MSG_INFO));
	m_bError=FALSE;
	m_nErrCode=0;
	m_strErrorReason="";
}

int CSipMessage::SendMessage( SIP_MSG_INFO msgInfo,int nWaitTime/*=2*/ )
{
	char* sCallID=new_callid();
	sprintf(m_sCallID,"%s",sCallID);
	m_sipMsgInfo=msgInfo;
	int nRet=SIP_SendMessage(sCallID,msgInfo.CallerID,msgInfo.CalleedID,
		m_loginInfo.sLocalIP,CSipWrapper::g_SipRecvPort,m_loginInfo.sServerIP,m_loginInfo.nServerPort,msgInfo.pMsg,msgInfo.nMsgLen);
	if (0==nRet)
	{
		strcpy(m_sCallID,sCallID);
	}
	if (nWaitTime>0)
	{
#ifdef WIN32
		WaitForSingleObject(m_hResponseEvent,nWaitTime*1000);
#else
		if(m_hResponseEvent)
		{
			m_hResponseEvent->Wait(nWaitTime*1000);
		}
#endif
	}
	
	return nRet;
}

//added by chenyu 130812
//全局发送消息
int CSipMessage::gSendMessage( SIP_MSG_INFO msgInfo,int nWaitTime/*=2*/ )
{
	char* sCallID=new_callid();
	sprintf(m_sCallID,"%s",sCallID);
	//m_nMessageSN=msgInfo.nSN;  不记录SN消息标志
	int nRet=SIP_SendMessage(sCallID,msgInfo.CallerID,msgInfo.CalleedID,
		m_loginInfo.sLocalIP,CSipWrapper::g_SipRecvPort,m_loginInfo.sServerIP,m_loginInfo.nServerPort,msgInfo.pMsg,msgInfo.nMsgLen);
	if (0==nRet)
	{
		strcpy(m_sCallID,sCallID);
	}
// 	if (nWaitTime>0)
// 	{
// 		WaitForSingleObject(m_hResponseEvent,nWaitTime*1000);
// 	}
	
	return nRet;
}

MessageRecvCallBack CSipMessage::g_msgRecvCallBack=NULL;    // 全局回调初始化
void CSipMessage::Init()
{
	app_set_message_received_cb(&MessageReceiveCB,NULL);	
}

void CSipMessage::Fini()
{
	app_set_message_received_cb(NULL,NULL);
}
int CSipMessage::MessageReceiveCB( char* caller_id, char* sendIP,int nSendPort,char* callee_id, char* receiveIP,int nreceivePort, char* call_id, int dialog_index, char* pMsg, int nMsgLen, int nUserData )
{
	//  此回调中需要通过解析收到的xml 找到序列号SN  在根据SN找到是那个SipMessage实例发出的消息 
	int nret=SIP_AnswerToSipMessage(call_id,200,NULL);
	if (nMsgLen<=0)
	{
		return -1;
	}
	if (nMsgLen>SIP_MSG_RECV_LEN)
	{
		return -2;
	}
	
	TiXmlDocument *myDocument = new TiXmlDocument();
	if (!myDocument)
	{	
		return -3;
	}
#ifdef WIN32
	CString str=pMsg;
	ConvertGBKToUtf8(str);
	myDocument->Parse(str.GetBuffer(0));
#else
	string str=pMsg;
    myDocument->Parse(str.c_str());
#endif
	TiXmlElement *RootElement = myDocument->RootElement();
	if (!RootElement)
	{// 解析错误
		return -4;
	}
	int nSN=0;
	TiXmlElement* pSnElement=RootElement->FirstChildElement("SN");
	if (pSnElement)
	{
		if (!pSnElement->FirstChild())
		{
			MEMORY_DELETE(myDocument);
			return -5;
		}		
		nSN=atoi(pSnElement->FirstChild()->Value());
	}


// 	static int index=0;
// 	TiXmlElement* pCmdTypeElement=RootElement->FirstChildElement("CmdType");
// 	if (pCmdTypeElement)
// 	{
// 		if (!pCmdTypeElement->FirstChild())
// 		{
// 			return -5;
// 		}	
// 		
// 		char filename[MAX_PATH]={0};
// 		sprintf(filename,"d:\\test%d.xml",index);
// 		if (!strcmp("RecordInfo",pCmdTypeElement->FirstChild()->Value()))
// 		{
// 			index++;
// 			myDocument->SaveFile(filename);
// 		}
// 	}
	
	for (int i=0;i<MAX_SIPWRAPPER_COUNT;i++)
	{
		if (CSipWrapper::g_SipWrapperList[i])			
		{
			CAutoLock lock(&CSipWrapper::g_SipWrapperList[i]->m_MessageCs);   // 保证下面获取和使用sipmessage对象不一致
		}
	}
	CSipMessage* pSipMsg=GetMessageBySN(nSN);
	if (!pSipMsg)
	{
		MEMORY_DELETE(myDocument);
		if (g_msgRecvCallBack)
		{//作为服务端主动接受message消息，外层根据业务解析xml
#ifdef WIN32
			g_msgRecvCallBack(caller_id, callee_id,call_id,str.GetBuffer(0),str.GetLength());
#else
			g_msgRecvCallBack(caller_id, callee_id,call_id,(char*)str.c_str(),str.length());
#endif
		}
		return -6;
	}

	MEMORY_DELETE(myDocument);
#ifdef WIN32
	pSipMsg->OnResponse(caller_id, callee_id, call_id, str.GetBuffer(0), str.GetLength());
#else
	pSipMsg->OnResponse(caller_id, callee_id, call_id, (char*)str.c_str(),str.length());
#endif

	return 0;
}

int CSipMessage::OnResponse( char* caller_id, char* callee_id, char* call_id, char* pMsg, int nMsgLen)
{
	// 每回调一次 需要保存收到的xml数据 
	for (int i=0;i<MAX_RECV_MSG_COUNT;i++)
	{
		if (!m_pMsgRecvList[i])
		{
			m_pMsgRecvList[i]=new MessageRecvData();
			strcpy(m_pMsgRecvList[i]->recvMsg,pMsg);
			m_pMsgRecvList[i]->nMsgLen=nMsgLen;
			break;
		}
	}
	TiXmlDocument *myDocument = new TiXmlDocument();
	if (!myDocument)
	{	
		//theApp.WriteLog("创建 TiXmlDocument对象失败");
		return -1;
	}	
	myDocument->Parse(pMsg);
	//myDocument->SaveFile("d:\\error.xml");
	TiXmlElement *RootElement = myDocument->RootElement();	
	int nSumNum=0,ItemNum=0;
	TiXmlElement* pSumNumElement=RootElement->FirstChildElement("SumNum");
	if (pSumNumElement)
	{
		if (pSumNumElement->FirstChild())
		{
			nSumNum=atoi(pSumNumElement->FirstChild()->Value());
		}		
	}	
	TiXmlElement* pErrCodeElement=RootElement->FirstChildElement("ErrCode");
	if (pErrCodeElement)
	{
		if (pErrCodeElement->FirstChild())
		{
			m_nErrCode=atoi(pErrCodeElement->FirstChild()->Value());
		}		
	}

	if (m_nErrCode<0)
	{
		m_bError=TRUE;
		TiXmlElement* pReasonElement=RootElement->FirstChildElement("Reason");
		if (pReasonElement && pReasonElement->FirstChild())
		{
			m_strErrorReason=pReasonElement->FirstChild()->Value();
#ifdef WIN32
			TRACE("错误原因 %s\n",m_strErrorReason);
#endif
		}
	}

	//  总共多少条数据
	const char* pNumValue=GetAttributeByName(RootElement,"Num");	
	if (!pNumValue)
	{// 找不到此节点 认为 不需要等待多条数据返回
#ifdef WIN32
		SetEvent(m_hResponseEvent);
#else
		if(m_hResponseEvent)
		{
			m_hResponseEvent->Set();
		}
#endif
	}
	else
	{
		ItemNum=atoi(pNumValue);
		m_nToRecvItemCount=nSumNum;
		m_nRecvItemCount+=ItemNum;
		if (m_nRecvItemCount>=m_nToRecvItemCount)
		{// 认为接收完毕
#ifdef WIN32
			SetEvent(m_hResponseEvent);
#else
			if(m_hResponseEvent)
			{
				m_hResponseEvent->Set();
			}
#endif
		}
	}
	
	MEMORY_DELETE(myDocument);
	return 0;
}

CSipMessage::~CSipMessage()
{
	int i = 0;
	for (i=0;i<MAX_RECV_MSG_COUNT;i++)
	{
		MEMORY_DELETE(m_pMsgRecvList[i]);
	}
#ifdef WIN32
	CLOSE_HANDLE(m_hResponseEvent);
#else
	MEMORY_DELETE(m_hResponseEvent);
#endif
}

int CSipMessage::SetMessageCallBack( MessageRecvCallBack callback )
{
	g_msgRecvCallBack=callback;
	return 0;
}

int CSipMessage::GetMessageSN()
{
	return m_sipMsgInfo.nSN;
}

int CSipMessage::SendMessageEx( SIP_MSG_INFO msgInfo )
{
	char* sCallID=new_callid();
	sprintf(m_sCallID,"%s",sCallID);
	m_sipMsgInfo=msgInfo;
	m_sipMsgInfo.nSN=0;   // 置为0 让 全局回调 不进入具体实例

	int nRet=SIP_SendMessage(sCallID,msgInfo.CallerID,msgInfo.CalleedID,
		m_loginInfo.sLocalIP,CSipWrapper::g_SipRecvPort,m_loginInfo.sServerIP,m_loginInfo.nServerPort,msgInfo.pMsg,msgInfo.nMsgLen);
	if (0==nRet)
	{
		strcpy(m_sCallID,sCallID);
	}
	return nRet;
}
CSipWrapper::CSipWrapper( int nID )
{
	m_nID=nID;
	m_nRefreshFailedCount=0;
	int i = 0;
	for (i=0;i<MAX_INVITE_COUNT;i++)
	{
		m_pSipInviteList[i]=NULL;
	}
	for (i=0;i<MAX_MESSAGE_COUNT;i++)
	{
		m_pSipMessageList[i]=NULL;
	}
#ifdef WIN32
	m_hRegisterEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
	m_hRegisterEvent = new CMyEvent(TRUE);
#endif
	m_nRegIndex=-1;
	m_nRegStatusCode=-1;
	sprintf(m_sName,"CSipWrapper id[%d] ",m_nID);
	m_nTime=0;

}
CSipWrapper::~CSipWrapper()
{
	int i = 0;
	for (i=0;i<MAX_INVITE_COUNT;i++)
	{
		MEMORY_DELETE(m_pSipInviteList[i]);
	}
	for (i=0;i<MAX_MESSAGE_COUNT;i++)
	{
		MEMORY_DELETE(m_pSipMessageList[i]);
	}
#ifdef WIN32
	CLOSE_HANDLE(m_hRegisterEvent);
#else
	MEMORY_DELETE(m_hRegisterEvent);
#endif
}


int CSipWrapper::Init()
{
	int i = 0;
	for (i=0;i<MAX_SIPWRAPPER_COUNT;i++)
	{
		g_SipWrapperList[i]=NULL;
	}
	int nRet=SIP_Init();
	if (0!=nRet)
	{		
		return -1;
	}
	nRet=SIP_UACStartUdpReceive(&g_SipRecvPort);
	if (0!=nRet)
	{
		return -2;
	}
	app_set_uac_register_response_received_cb(&RegisterCB,NULL);
	CSipInvite::Init();
	CSipMessage::Init();
	return 0;
}

void CSipWrapper::Free()
{
	for (int i=0;i<MAX_SIPWRAPPER_COUNT;i++)
	{
		MEMORY_DELETE(g_SipWrapperList[i]);
	}
	app_set_uac_register_response_received_cb(NULL,NULL);
	CSipInvite::Fini();
	CSipMessage::Fini();	
	SIP_Free();
}

int CSipWrapper::SendBye( int nInviteHandle )
{
	if (m_pSipInviteList[nInviteHandle])
	{
		m_pSipInviteList[nInviteHandle]->SendBye();
	}
	return 0;
}

int CSipWrapper::SendRegister( int nTimeOut/*=60*/,int nWaitTime/*=2*/ )
{
	m_nRegIndex=SIP_SendRegister(m_loginInfo.sServerID,m_loginInfo.sUserID,m_loginInfo.sLocalIP,g_SipRecvPort,m_loginInfo.sServerIP,m_loginInfo.nServerPort,
		m_loginInfo.sUserName,m_loginInfo.sUserPwd,nTimeOut);
	if (m_nRegIndex<0)
	{		
		return -1;
	}
	
#ifdef WIN32
	WaitForSingleObject(m_hRegisterEvent,nWaitTime*1000);
#else
	if(m_hRegisterEvent)
	{
		m_hRegisterEvent->Wait(nWaitTime*1000);
	}
#endif

	if (200==m_nRegStatusCode)
	{
		return 0;
	}	
	return -1;
}
int CSipWrapper::OnRegisterCallBack( int nRegIndex,UINT nStatusCode,unsigned int nTime)
{	
	char str[MAX_PATH];
	sprintf(str,"regindex:%d StatusCode:%d",nRegIndex,nStatusCode);
	m_nRegStatusCode=nStatusCode;
	if (200==nStatusCode)
	{
		m_nRefreshFailedCount=0;
	}
	else
	{
		m_nRefreshFailedCount++;
	}
	if (200==nStatusCode)
	{
		m_nTime=nTime;
	}
#ifdef WIN32
	SetEvent(m_hRegisterEvent);
#else
	if(m_hRegisterEvent)
	{
		m_hRegisterEvent->Set();
	}
#endif
	return 0;
}

void CSipWrapper::SetLoginInfo( EV9000_LOGININFO loginInfo )
{
	m_loginInfo=loginInfo;
}

int CSipWrapper::GetFreeInvite()
{
	CAutoLock lock(&m_InviteCS);
	for (int i=0;i<MAX_INVITE_COUNT;i++)
	{
		if (!m_pSipInviteList[i])
		{
			m_pSipInviteList[i]=new CSipInvite(i);
			m_pSipInviteList[i]->SetParent(this);
			m_pSipInviteList[i]->SetLoginInfo(m_loginInfo);
			return i;
		}
	}
	return -1;
}

int CSipWrapper::SendInvite( int nInviteHandle,SIP_INVITE_INFO inviteInfo,int nWaitTime)
{
	if (nInviteHandle<0 || nInviteHandle>=MAX_INVITE_COUNT)
	{
		return -1;
	}
	if (m_pSipInviteList[nInviteHandle])
	{
		return m_pSipInviteList[nInviteHandle]->SendInvite(inviteInfo,nWaitTime);
	}
	return -1;
}

CSipInvite* CSipWrapper::GetInviteByHandle( int nInviteHandle )
{
	CAutoLock lock(&m_InviteCS);
	if (nInviteHandle<0 || nInviteHandle>=MAX_INVITE_COUNT)
	{
		return NULL;
	}
	return m_pSipInviteList[nInviteHandle];
}

int CSipWrapper::SendMessage( int nMessageHandle,SIP_MSG_INFO msgInfo,int nWaitTime/*=2*/ )
{
	if (nMessageHandle<0 || nMessageHandle>=MAX_MESSAGE_COUNT)
	{
		return -1;
	}
	if (m_pSipMessageList[nMessageHandle])
	{
		return m_pSipMessageList[nMessageHandle]->SendMessage(msgInfo,nWaitTime);
	}
	return -1;
}

//added by chenyu 130812
//全局发送消息
int CSipWrapper::gSendMessage( int nMessageHandle,SIP_MSG_INFO msgInfo,int nWaitTime/*=2*/ )
{
	if (nMessageHandle<0 || nMessageHandle>=MAX_MESSAGE_COUNT)
	{
		return -1;
	}
	if (m_pSipMessageList[nMessageHandle])
	{
		return m_pSipMessageList[nMessageHandle]->gSendMessage(msgInfo,nWaitTime);
	}
	return -1;
}

int CSipWrapper::SendInfo( SIP_INFO& info )
{
	return 0;
}

int CSipWrapper::RegisterCB( int nRegIndex,int expires,int nStatusCode,unsigned int nTime,int nUserData )
{
	CSipWrapper* pSipWrapper=GetSipByRegIndex(nRegIndex);
	if (pSipWrapper)
	{
		pSipWrapper->OnRegisterCallBack(nRegIndex,nStatusCode,nTime);
	}
	return 0;
}

int CSipWrapper::FreeInvite( int nInviteHandle )
{
	CAutoLock lock(&m_InviteCS);
	if (nInviteHandle>=0 && nInviteHandle<MAX_INVITE_COUNT)
	{
		MEMORY_DELETE(m_pSipInviteList[nInviteHandle]);
	}
	return 0;
}

CSipInvite* CSipWrapper::GetInviteByDialogIndex( int nDialogIndex )
{
	for (int i=0;i<MAX_INVITE_COUNT;i++)
	{
		if (m_pSipInviteList[i])
		{
			if (nDialogIndex==m_pSipInviteList[i]->GetDialogIndex())
			{
				return m_pSipInviteList[i];
			}
		}
	}
	return NULL;
}

int CSipWrapper::GetFreeMessage()
{
	CAutoLock lock(&m_MessageCs);
	for (int i=0;i<MAX_MESSAGE_COUNT;i++)
	{
		if (!m_pSipMessageList[i])
		{
			m_pSipMessageList[i]=new CSipMessage(i);
			m_pSipMessageList[i]->SetParent(this);
			m_pSipMessageList[i]->SetLoginInfo(m_loginInfo);
			return i;
		}
	}
	return -1;
}

int CSipWrapper::FreeMessage( int nMessageHandle )
{
	CAutoLock lock(&m_MessageCs);
	if (nMessageHandle>=0 && nMessageHandle<MAX_MESSAGE_COUNT)
	{
		MEMORY_DELETE(m_pSipMessageList[nMessageHandle]);
	}
	return 0;
}

int CSipWrapper::RefreshRegister()
{
	if (m_nRegIndex>=0)
	{
		m_nRegStatusCode=-1;      // 重置
		int nRet=SIP_SendRegisterForRefresh(m_nRegIndex);
		if (0!=nRet)
		{
			return -1;
		}
#ifdef WIN32
		WaitForSingleObject(m_hRegisterEvent,2*1000);
#else
		if(m_hRegisterEvent)
		{
			m_hRegisterEvent->Wait(2*1000);
		}
#endif
		if (200==m_nRegStatusCode)
		{
			return 0;
		}
	}
	return -1;
}

CSipMessage* CSipWrapper::GetMessageByHandle( int nMessageHandle )
{
	if (nMessageHandle<0 || nMessageHandle>=MAX_MESSAGE_COUNT)
	{
		return NULL;
	}
	if (m_pSipMessageList[nMessageHandle])
	{
		return m_pSipMessageList[nMessageHandle];
	}
	return NULL;
}

int CSipWrapper::SendAck( int nInviteHandle )
{
	if (nInviteHandle<0 || nInviteHandle>=MAX_INVITE_COUNT)
	{
		return -1;
	}
	if (m_pSipInviteList[nInviteHandle])
	{
		return m_pSipInviteList[nInviteHandle]->SendACK();
	}
	return -1;
}

int CSipWrapper::SendInfoWithinDialog( int nInviteHandle,char* body,int body_len)
{
	if (nInviteHandle<0 || nInviteHandle>=MAX_INVITE_COUNT)
	{
		return -1;
	}
	if (m_pSipInviteList[nInviteHandle])
	{
		return m_pSipInviteList[nInviteHandle]->SendInfo(body,body_len);
	}
	return -1;
	
}

int CSipWrapper::CreateSipWrapper()
{
	for (int i=0;i<MAX_SIPWRAPPER_COUNT;i++)
	{
		if (!g_SipWrapperList[i])
		{
			g_SipWrapperList[i]=new CSipWrapper(i);
			if (g_SipWrapperList[i])
			{
				return i;
			}			
		}
	}
	return -1;
}

CSipMessage* CSipWrapper::GetMessageBySN( int nSN )
{	
	for (int i=0;i<MAX_MESSAGE_COUNT;i++)
	{
		if (m_pSipMessageList[i] && nSN==m_pSipMessageList[i]->GetMessageSN())
		{
			return m_pSipMessageList[i];
		}
	}
	return NULL;
}

void CSipWrapper::DeleteSipWrapper( int nHandle )
{
	if (nHandle>=0 && nHandle<MAX_SIPWRAPPER_COUNT)
	{
		MEMORY_DELETE(g_SipWrapperList[nHandle]);
	}
}

CSipWrapper* CSipWrapper::GetSipWrapperByHandle( int nHandle )
{
	if (nHandle>=0 && nHandle<MAX_SIPWRAPPER_COUNT)
	{
		return g_SipWrapperList[nHandle];
	}	
}

int CSipWrapper::SendMessageEx( int nMessageHandle,SIP_MSG_INFO msgInfo )
{
	if (nMessageHandle<0 || nMessageHandle>=MAX_MESSAGE_COUNT)
	{
		return -1;
	}
	if (m_pSipMessageList[nMessageHandle])
	{
		return m_pSipMessageList[nMessageHandle]->SendMessageEx(msgInfo);
	}
	return -1;
}

int CSipWrapper::GetStatusCode( int nInviteHandle )
{
	if (nInviteHandle<0 || nInviteHandle>=MAX_INVITE_COUNT)
	{
		return -1;
	}
	if (m_pSipInviteList[nInviteHandle])
	{
		return m_pSipInviteList[nInviteHandle]->GetStatusCode();
	}
	return -1;
}

char* CSipWrapper::GetInviteFailedReason( int nInviteHandle )
{
	if (nInviteHandle<0 || nInviteHandle>=MAX_INVITE_COUNT)
	{
		return NULL;
	}
	if (m_pSipInviteList[nInviteHandle])
	{
		return m_pSipInviteList[nInviteHandle]->GetInviteFailedReason();
	}
	return NULL;
}

BOOL CSipWrapper::IsSessionExpire( int nInviteHandle )
{
	if (nInviteHandle<0 || nInviteHandle>=MAX_INVITE_COUNT)
	{
		return NULL;
	}
	if (m_pSipInviteList[nInviteHandle])
	{
		return m_pSipInviteList[nInviteHandle]->IsSessionExpire();
	}
	return FALSE;
}
CSipInvite::CSipInvite( int nID )
{
	m_nID=nID;
	m_nDialogIndex=-1;
	sprintf(m_sName,"CSipInvite id[%d] ",m_nID);
	m_hInviteEvent = NULL;
	m_nStatusCode=-1;
	ZeroMemory(m_sInviteFailedReason,EV9000_NORMAL_STRING_LEN);
	m_bSessionExpired=FALSE;
}

int CSipInvite::SendInvite( SIP_INVITE_INFO inviteInfo,int nWaitTime )
{	
#ifdef SIP_NEW
	sdp_message_t* pSdp=NULL;
#else
	sdp_t* pSdp=NULL;
#endif
	if (!m_hInviteEvent)
	{
#ifdef WIN32
		m_hInviteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
		m_hInviteEvent = new CMyEvent(TRUE);
#endif
	}
	
	sdp_build_offer(NULL,&pSdp,NULL,inviteInfo.sVideoPort,m_loginInfo.sLocalIP,inviteInfo.sType,inviteInfo.nStartTime,inviteInfo.nStopTime,inviteInfo.nPlayTime,2,-1,-1);

	int nRet=SIP_SendInvite(inviteInfo.CallerID,inviteInfo.CalleedID,m_loginInfo.sLocalIP,
		CSipWrapper::g_SipRecvPort,m_loginInfo.sServerIP,m_loginInfo.nServerPort,
		m_loginInfo.sUserName,m_loginInfo.sUserPwd,pSdp);
	if (nRet<0)
	{
		return -1;
	}
	m_nDialogIndex=nRet;
#ifdef WIN32
	WaitForSingleObject(m_hInviteEvent,nWaitTime*1000);
#else
	if(m_hInviteEvent)
	{
		m_hInviteEvent->Wait(nWaitTime*1000);
	}
#endif
	if (200==m_nStatusCode)
	{
		//AddListInfo("invite返回成功");
		m_nDialogIndex=nRet;
		return 0;
	}
	if (-1==m_nStatusCode)
	{
		//AddListInfo("未收到Invite消息响应 status code:%d",GetName());
	}
	else
	{
	//	AddListInfo("Invite失败 status code:%d",m_nStatusCode);
	}
	
	return -1;
}
int CSipInvite::SendBye()
{
	return SIP_SendBye(m_nDialogIndex);
}

int CSipInvite::OnInviteResponse( char* caller_id,char* callee_id,char* call_id,int dialog_index,int status_code,char* reasonphrase)
{	
	m_nStatusCode=status_code;	
	if (reasonphrase)
	{
		if (-1==m_nStatusCode)
		{
			sprintf(m_sInviteFailedReason,"%s","未等到Invite响应");
		}
		else
		{
			sprintf(m_sInviteFailedReason,"%s 错误码 %d",m_sInviteFailedReason,m_nStatusCode);
		}
	}
	
#ifdef WIN32
	SetEvent(m_hInviteEvent);
#else
	if(m_hInviteEvent)
	{
		m_hInviteEvent->Set();
	}
#endif
	m_bInviteSuccess=TRUE;
	return 0;
}

int CSipInvite::SendACK()
{
	if (m_nDialogIndex>=0)
	{
		int nret=SIP_SendAck(m_nDialogIndex);
		return nret;
	}
	return -1;
}

#ifdef SIP_NEW
int CSipInvite::Invite_response_receive_cb( char* caller_id,char* callee_id,char* call_id,int dialog_index,int status_code,char* reasonphrase,char *body,int body_len,int nUserData)
#else
int CSipInvite::Invite_response_receive_cb( char* caller_id,char* callee_id,char* call_id,int dialog_index,int status_code,char* reasonphrase,int nUserData )
#endif
{
	//TRACE("Invite_response_receive_cb status code:%d\n",status_code);
	if (status_code==100)
	{
		return 0;
	}
	CAutoLock lock(&CSipWrapper::g_SipWrapperList[0]->m_InviteCS);

	CSipInvite* pSipInvite=GetInviteByDialogIndex(dialog_index);
	if (pSipInvite)
	{
		return pSipInvite->OnInviteResponse(caller_id,callee_id,call_id,dialog_index,status_code,reasonphrase);
	}
	return 0;
}
InviteRecvCallBack CSipInvite::g_inviteRecvCallBack=NULL;
ByeRecvCallBack    CSipInvite::g_byeRecvCallBack=NULL;
void CSipInvite::Init()
{
	app_set_invite_response_received_cb(&Invite_response_receive_cb,NULL);
	app_set_invite_received_cb(&Invite_recedive_cb,NULL);
	app_set_bye_received_cb(&Bye_receive_cb,NULL);
	app_set_ua_session_expires_cb(&session_expires_cb);  // 会话超时回调
}

void CSipInvite::Fini()
{
	app_set_invite_response_received_cb(NULL,NULL);
	app_set_invite_received_cb(NULL,NULL);
	app_set_bye_received_cb(NULL,NULL);
	app_set_ua_session_expires_cb(NULL);
}

#ifdef SIP_NEW
sdp_message_t* CSipInvite::GetRemoteSDP()
#else
sdp_t* CSipInvite::GetRemoteSDP()
#endif
{
	return SIP_GetInviteDialogRemoteSDP(m_nDialogIndex);	
}

#ifdef SIP_NEW
int CSipInvite::GetSDPVideoInfo( sdp_message_t* sdp, unsigned long* addr, int* port, int* codetype, int* flag )
#else
int CSipInvite::GetSDPVideoInfo( sdp_t* sdp, unsigned long* addr, int* port, int* codetype, int* flag )
#endif
{
	return SIP_GetSDPVideoInfo(sdp, addr, port, codetype,flag);
}

int CSipInvite::SendInfo( char* body, int body_len )
{
	return SIP_SendInfoWithinDialog(m_nDialogIndex,body,body_len);
}

#ifdef SIP_NEW
int CSipInvite::Invite_recedive_cb( char* caller_id, char* callee_id, char* call_id, int dialog_index,char *body,int body_len, int nUserData)
#else
int CSipInvite::Invite_recedive_cb( char* caller_id, char* callee_id, char* call_id, int dialog_index, int nUserData )
#endif
{
	if (g_inviteRecvCallBack)
	{
		g_inviteRecvCallBack(caller_id,callee_id,call_id,dialog_index);
	}
	return 0;
}

int CSipInvite::SetInviteRecvCallBack( InviteRecvCallBack callBack )
{
	g_inviteRecvCallBack=callBack;
	return 0;
}

int CSipInvite::Bye_receive_cb( char* caller_id, char* callee_id, char* call_id, int dialog_index, int nUserData )
{
	if (g_byeRecvCallBack)
	{
		g_byeRecvCallBack(caller_id,callee_id,call_id,dialog_index);
	}
	return 0;
}

int CSipInvite::SetByeRecvCallBack(ByeRecvCallBack callBack)
{
	g_byeRecvCallBack=callBack;
	return 0;
}

int CSipInvite::session_expires_cb( int nDialogIndex )
{
	CSipInvite* pInvite=GetInviteByDialogIndex(nDialogIndex);
	if (pInvite)
	{
		pInvite->OnSessionExpire();
	}
	return 0;
}

int CSipInvite::OnSessionExpire()
{
	m_bSessionExpired=TRUE;
	return 0;
}

CSipInvite::~CSipInvite()
{
#ifdef WIN32
	CLOSE_HANDLE(m_hInviteEvent);
#else
	MEMORY_DELETE(m_hInviteEvent);
#endif
}
CSipBaseItem::CSipBaseItem()
{
	m_nID=0;
	ZeroMemory(m_sName,MAX_PATH);
	m_pParent=NULL;
}

#ifdef WIN32
void CSipBaseItem::ConvertUtf8ToGBK(CString& strUtf8) 
{
	int len=MultiByteToWideChar(CP_UTF8, 0, (LPCTSTR)strUtf8, -1, NULL,0);
	unsigned short * wszGBK = new unsigned short[len+1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, (LPCTSTR)strUtf8, -1, wszGBK, len);
	
	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char *szGBK=new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte (CP_ACP, 0, wszGBK, -1, szGBK, len, NULL,NULL);
	
	strUtf8 = szGBK;
	delete[] szGBK;
	delete[] wszGBK;
}
void CSipBaseItem::ConvertGBKToUtf8(CString& strGBK)
{
	int len=MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)strGBK, -1, NULL,0);
	unsigned short * wszUtf8 = new unsigned short[len+1];
	memset(wszUtf8, 0, len * 2 + 2);
	MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)strGBK, -1, wszUtf8, len);
	
	len = WideCharToMultiByte(CP_UTF8, 0, wszUtf8, -1, NULL, 0, NULL, NULL);
	char *szUtf8=new char[len + 1];
	memset(szUtf8, 0, len + 1);
	WideCharToMultiByte (CP_UTF8, 0, wszUtf8, -1, szUtf8, len, NULL,NULL);
	
	strGBK = szUtf8;
	delete[] szUtf8;
	delete[] wszUtf8;
}
#endif
