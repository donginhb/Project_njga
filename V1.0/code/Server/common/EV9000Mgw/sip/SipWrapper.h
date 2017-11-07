#ifndef _SIPWRAPPER_H_DEFINE
#define _SIPWRAPPER_H_DEFINE

#ifdef WIN32
#define SIP_NEW
#ifdef SIP_NEW
#include "../../../embedded/cms/libsip_new/include/libsip.h"
#else
#include "../../../embedded/cms/libsip/include/libsip.h"
#endif
#include "../../../Common/include/EV9000_InnerDef.h"
#include "../../../Common/include/EV9000_ExtDef.h"
#include "../public/Include/IEV9000App.h"
#include "../public/log/Log.h"
#else  
//for linux  add searchpath of OnvifProxy project
#define  SIP_NEW
#define  ZeroMemory(Destination,Length) memset((Destination),0,(Length))
#define  TRACE printf 
#include "include/libsip.h"
#include "MgwPublicData.h"
#include "Event/MyEvent.h"
#endif

#include "../xml/tinyxml/tinystr.h"
#include "../xml/tinyxml/tinyxml.h"

#ifndef CLOSE_HANDLE
#define CLOSE_HANDLE(handle) if(handle) { CloseHandle(handle);handle=NULL;}
#endif
#define MEMORY_DELETE(x)  if(x) {delete x; x=NULL;}              //delete �ڴ�
#define MEMORY_DELETE_EX(x)  if(x){ delete[] x; x=NULL;}         //delete �ڴ�����

#define  MAX_INVITE_COUNT 64
#define  MAX_MESSAGE_COUNT 32
#define  MAX_SENDMSG_SIZE  512       // message��Ϣ����ַ�������
#define  MAX_SIPWRAPPER_COUNT 32 
#define  MAX_RECV_MSG_COUNT  2048     // ÿ��sip����Զ���յ�sip msg ����
#define  SIP_MSG_RECV_LEN  10000       // SIP message��Ϣ������Ϣ����
                  // sip ���ն˿�

class CSipWrapper;
// ��װ Invite ��Ϣ���ͽṹ
typedef struct SIP_INVITE_INFO
{
	char CallerID[EV9000_NORMAL_STRING_LEN];
	char CalleedID[EV9000_NORMAL_STRING_LEN];
#ifdef SIP_NEW   //modified by chenyu 131024
	sdp_message_t* pSdp;
#else
	sdp_t* pSdp;
#endif
	char sVideoPort[EV9000_SHORT_STRING_LEN];
	char sAudioPort[EV9000_SHORT_STRING_LEN];
	char sType[EV9000_SHORT_STRING_LEN];          // "PLAY"
	int  nStartTime;
	int  nStopTime;
	int  nPlayTime;
	SIP_INVITE_INFO()
	{
		ZeroMemory(CallerID,EV9000_NORMAL_STRING_LEN);
		ZeroMemory(CalleedID,EV9000_NORMAL_STRING_LEN);
		pSdp=NULL;
		ZeroMemory(sType,EV9000_SHORT_STRING_LEN);
		nStartTime=0;
		nStopTime=0;
		nPlayTime=0;
	}
}SIP_INVITE_INFO,*LPSIP_INVITE_INFO;

#ifdef WIN32
// ��װ message ��Ϣ���ͽṹ
typedef struct SIP_MSG_INFO
{
	char CallerID[EV9000_NORMAL_STRING_LEN];
	char CalleedID[EV9000_NORMAL_STRING_LEN];
	char* pMsg;
	int   nMsgLen;
	int   nSN;                 // message ��Ϣ���к� 
	SIP_MSG_INFO()
	{
		ZeroMemory(CallerID,EV9000_NORMAL_STRING_LEN);
		ZeroMemory(CalleedID,EV9000_NORMAL_STRING_LEN);
		pMsg=NULL;
		nMsgLen=0;
		nSN=0;
	}
}SIP_MSG_INFO,*LPSIP_MSG_INFO;
#endif

// ��װ info ��Ϣ���ͽṹ
typedef struct SIP_INFO
{
	char sMsgID[EV9000_IDCODE_LEN];
	char CallerID[EV9000_NORMAL_STRING_LEN];
	char CalleedID[EV9000_NORMAL_STRING_LEN];
	char* pBody;
	int   nBodyLen;
	SIP_INFO()
	{
		ZeroMemory(sMsgID,EV9000_NORMAL_STRING_LEN);
		ZeroMemory(CallerID,EV9000_NORMAL_STRING_LEN);
		ZeroMemory(CalleedID,EV9000_NORMAL_STRING_LEN);
		pBody=NULL;
		nBodyLen=0;
	}
}SIP_INFO,*LPSIP_INFO;

class CSipBaseItem  
{
public:
	CSipBaseItem();
	virtual ~CSipBaseItem(){};
	int  GetID(){return m_nID;};
	char* GetName(){return m_sName;};
	void SetParent(CSipBaseItem* pParent){m_pParent=pParent;};
#ifdef WIN32
	static void ConvertUtf8ToGBK(CString& strUtf8);
	static void ConvertGBKToUtf8(CString& strGBK);
#endif
protected:
	char     m_sName[MAX_PATH];
	int      m_nID;
	CSipBaseItem* m_pParent;
};
// �ṩ������õ�ȫ�ֻص�
typedef int (*InviteRecvCallBack)(char* caller_id, char* callee_id, char* call_id, int dialog_index);
typedef int (*ByeRecvCallBack)(char* caller_id, char* callee_id, char* call_id, int dialog_index);

// sip invite ��Ϣ��װ
class CSipInvite:public CSipBaseItem
{
public:
	CSipInvite(int nID);
	~CSipInvite();
	void SetLoginInfo(EV9000_LOGININFO loginInfo){m_loginInfo=loginInfo;};
	int  GetDialogIndex(){return m_nDialogIndex;};
	int  GetStatusCode(){return m_nStatusCode;};
	BOOL IsSessionExpire(){return m_bSessionExpired;};
	char* GetInviteFailedReason(){return m_sInviteFailedReason;};
	int  SendInvite(SIP_INVITE_INFO inviteInfo,int nWaitTime=2);
	int  SendACK();
	int  SendBye();
	int  SendInfo(char* body, int body_len);           // �Ự��info��Ϣ
#ifdef SIP_NEW  //modified by chenyu 131025
	sdp_message_t* GetRemoteSDP();
	int  GetSDPVideoInfo(sdp_message_t* sdp, unsigned long* addr, int* port, int* codetype, int* flag);
#else
    sdp_t* GetRemoteSDP();
	int  GetSDPVideoInfo(sdp_t* sdp, unsigned long* addr, int* port, int* codetype, int* flag);
#endif
	int  OnInviteResponse(char* caller_id,char* callee_id,char* call_id,int dialog_index,int status_code,char* reasonphrase);
	int  OnSessionExpire();
public:
	static void Init();
	static void Fini();
	// ����invite��Ϣ�� ��Ӧ�ص�����
#ifdef SIP_NEW
	static int Invite_response_receive_cb(char* caller_id,char* callee_id,char* call_id,int dialog_index,int status_code,char* reasonphrase,char *body,int body_len,int nUserData);
#else
	static int Invite_response_receive_cb(char* caller_id,char* callee_id,char* call_id,int dialog_index,int status_code,char* reasonphrase,int nUserData);
#endif
	// ����invite��Ϣ�ص�
#ifdef SIP_NEW
	static int Invite_recedive_cb(char* caller_id, char* callee_id, char* call_id, int dialog_index,char *body,int body_len, int nUserData);
#else
	static int Invite_recedive_cb(char* caller_id, char* callee_id, char* call_id, int dialog_index, int nUserData);
#endif
	static InviteRecvCallBack g_inviteRecvCallBack;
	static int session_expires_cb(int nDialogIndex);  // �Ự��ʱ�ص�
	static int SetInviteRecvCallBack(InviteRecvCallBack callBack);
	// ����Bye��Ϣ�ص�
	static int Bye_receive_cb(char* caller_id, char* callee_id, char* call_id, int dialog_index, int nUserData);
	static ByeRecvCallBack g_byeRecvCallBack;
	static int SetByeRecvCallBack(ByeRecvCallBack callBack);

private:
	int m_nDialogIndex;
	int m_nStatusCode;
	EV9000_LOGININFO m_loginInfo;
#ifdef WIN32
	HANDLE m_hInviteEvent;
#else
    CMyEvent *m_hInviteEvent;
#endif
	char m_sInviteFailedReason[EV9000_NORMAL_STRING_LEN];
	BOOL m_bInviteSuccess;
	BOOL m_bSessionExpired;   //  �Ự���ʱ
};
// ����msg��Ϣ�ṹ��ͨ��new ����
typedef struct MessageRecvData 
{
	char recvMsg[SIP_MSG_RECV_LEN];
	int  nMsgLen;
	MessageRecvData()
	{
		ZeroMemory(recvMsg,SIP_MSG_RECV_LEN);
		nMsgLen=0;
	}
}MessageRecvData,*LPMessageRecvData;

// ����Ļص�
typedef int (*MessageRecvCallBack) (char* caller_id, char* callee_id, char* call_id, char* pMsg, int nMsgLen);
// ��װ sip message
class CSipMessage:public CSipBaseItem
{
public:
	CSipMessage(int nID);
	~CSipMessage();
	void SetLoginInfo(EV9000_LOGININFO loginInfo){m_loginInfo=loginInfo;};
	int GetMessageSN();
	int SendMessage(SIP_MSG_INFO msgInfo,int nWaitTime=2); // Ĭ�ϵȴ�ʱ��2s
	int SendMessageEx(SIP_MSG_INFO msgInfo);               // �����ظ�message��Ϣ ����ȴ����ؽ��
    int gSendMessage( SIP_MSG_INFO msgInfo,int nWaitTime=2 ); //ȫ�ַ�����Ϣ //added by chenyu 130812
	int OnResponse(char* caller_id, char* callee_id, char* call_id, char* pMsg, int nMsgLen);
	BOOL IsError(){return m_bError;};
	int  GetErrCode(){return m_nErrCode;};
#ifdef WIN32
	CString GetReason(){return m_strErrorReason;};
#else
    string GetReason(){return m_strErrorReason;};
#endif
	static void Init();
	static void Fini();
	static int MessageReceiveCB(char* caller_id, char* sendIP,int nSendPort,char* callee_id, char* receiveIP,int nreceivePort, char* call_id, int dialog_index, char* pMsg, int nMsgLen, int nUserData);
	static int SetMessageCallBack(MessageRecvCallBack callback);
	static MessageRecvCallBack g_msgRecvCallBack;
protected:
#ifdef WIN32
	HANDLE m_hResponseEvent;
#else
	CMyEvent *m_hResponseEvent;
#endif
public:
	char   m_sCallID[EV9000_IDCODE_LEN];
	EV9000_LOGININFO m_loginInfo;
	SIP_MSG_INFO m_sipMsgInfo;
	char m_sRecvMsg[512];
	int  m_nRecvMsgLen;
	MessageRecvData* m_pMsgRecvList[MAX_RECV_MSG_COUNT];   
	int m_nToRecvItemCount;    // ��Ҫ���յļ�¼����
	int m_nRecvItemCount;      // �ۼ��յ���¼����
	BOOL  m_bError;
	int   m_nErrCode;
#ifdef WIN32
	CString m_strErrorReason;
#else
	string m_strErrorReason;
#endif
};
// sip ��װ

class CSipWrapper:public CSipBaseItem
{
public:	
	virtual ~CSipWrapper();
	static int Init();
	static void Free();
	static int  CreateSipWrapper();                  // ����sipWrapper ʵ�� ���ؾ��
	static void DeleteSipWrapper(int nHandle);       // ɾ��sipʵ��  nHandle ��ȫ�������±�
	CSipWrapper* GetSipWrapperByHandle(int nHandle);
	void SetLoginInfo(EV9000_LOGININFO loginInfo); 
	BOOL IsSessionExpire(int nInviteHandle);
	int SendRegister(int nTimeOut=60,int nWaitTime=2);	   // ��ʱʱ��Ĭ��60s �ȴ�����Ĭ��2s
	int RefreshRegister();
	int GetFreeInvite();                   // ���� invite ��Դ	
	int FreeInvite(int nInviteHandle);	
	int SendInvite(int nInviteHandle,SIP_INVITE_INFO inviteInfo,int nWaitTime=2);
	int GetStatusCode(int nInviteHandle);
	char* GetInviteFailedReason(int nInviteHandle);
	int SendBye(int nInviteHandle);
	int SendAck(int nInviteHandle);
	
	int SendInfo(SIP_INFO& info);
	int SendInfoWithinDialog(int nInviteHandle,char* body,int body_len);
	int GetFreeMessage();
	int FreeMessage(int nMessageHandle);
	CSipMessage* GetMessageByHandle(int nMessageHandle);
	CSipMessage* GetMessageBySN(int nSN);
	int SendMessage(int nMessageHandle,SIP_MSG_INFO msgInfo,int nWaitTime=2);
	int SendMessageEx(int nMessageHandle,SIP_MSG_INFO msgInfo); 
    int gSendMessage( int nMessageHandle,SIP_MSG_INFO msgInfo,int nWaitTime=2 );//ȫ�ַ�����Ϣ //added by chenyu 130812

	CSipInvite* GetInviteByHandle(int nInviteHandle);
	CSipInvite* GetInviteByDialogIndex(int nDialogIndex);
	int GetRegIndex(){return m_nRegIndex;};
	int GetRegStatusCode(){return m_nRegStatusCode;};

	int OnRegisterCallBack(int nRegIndex,UINT nStatusCode,unsigned int nTime);	
	unsigned int GetTime(){return m_nTime;};
protected:
	CSipWrapper(int nID);      //������ֱ��ʵ����
	static int RegisterCB(int nRegIndex,int expires,int nStatusCode,unsigned int nTime,int nUserData);	
public:
	CSipInvite* m_pSipInviteList[MAX_INVITE_COUNT];	
	CSipMessage* m_pSipMessageList[MAX_MESSAGE_COUNT];
	EV9000_LOGININFO m_loginInfo;
#ifdef WIN32
	HANDLE m_hRegisterEvent;      // ͬ���ȴ��ص��¼�
#else
	CMyEvent *m_hRegisterEvent;      // ͬ���ȴ��ص��¼�
#endif
	int    m_nRegIndex;
	int    m_nRegStatusCode;
	int    m_nRefreshFailedCount;
#ifdef WIN32
	CCritSec m_InviteCS;         //  �Ự��Դ��
	CCritSec m_MessageCs;
#else
	pthread_mutex_t m_InviteCS;         //  �Ự��Դ��
	pthread_mutex_t m_MessageCs;
#endif
	unsigned int m_nTime;        //  ע����Ӧ��������ʱ��
public:
	static int g_SipRecvPort; 
	static CSipWrapper* g_SipWrapperList[MAX_SIPWRAPPER_COUNT];	
};

#endif
