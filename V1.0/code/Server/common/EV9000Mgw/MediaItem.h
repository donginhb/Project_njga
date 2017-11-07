// MediaItem.h: interface for the CMediaItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MEDIAITEM_H__4F0E695E_E2B6_42F5_B0CA_CFE464A02DA2__INCLUDED_)
#define AFX_MEDIAITEM_H__4F0E695E_E2B6_42F5_B0CA_CFE464A02DA2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MgwPublicData.h"
#ifdef WIN32
#include "../public/Include/IComNetSdk.h"
#include "../public/buf/RingBuffer.h"
#include "../public/Function/AutoLock.h"
#include "../public/net/Udp2.h"
#include "MediaDataOp.h"
#include "DataSend.h"
#include "SdkProxy.h"    //sdk����
#include "../public/Include/IEV9kPsPack.h"
#include "PS_Packet_Packaging.h"
#include "PS_Define.h"
#else
#include "../OnvifSDK/OnvifOp.h"
#include "AutoLock/CAutoLock.h"
#endif
#include "CtrlProOp.h"
#include <iostream>
#include <list>
#include <vector>
#include <map>
#include <string>
using namespace std;


class CMediaItem;
class CMediaItemDevice;         //�豸
class CMediaItemReal;           //ʵʱ
// class CMediaItemPlayBack;
// class CFindData;
#ifdef WIN32
class CPlayBack;                //¼��
#endif

////////////////////////////////////////////////
//�豸��
////////////////////////////////////////////////
class CMediaItem  
{
public:
	CMediaItem(int nID);
	virtual ~CMediaItem();

public:
	MEDIAITEM_TYPE  GetMediaType();
	int             GetID();
	int             SetParent(CMediaItem* lpMediaItem);
	CMediaItem*     GetParent();
	CMediaItem*     GetChild(int nID = -1, MEDIAITEM_TYPE eMediaType = MEDIAITEM_TYPE_INVALID);
	CMediaItem*     GetLastChild();
	int             SetNext(CMediaItem* lpMediaItem);
	CMediaItem*     GetNext();
	int             GetChildNum(MEDIAITEM_TYPE eMediaType = MEDIAITEM_TYPE_INVALID);
	int             InsertChild(CMediaItem* lpMediaItem);
	void            DeleteChild(CMediaItem* lpMediaItem = NULL);	
	virtual char*   GetName();

public:	
	void            AddMsgWnd(HWND hMsgWnd);
	virtual void    SetData(BYTE* byData);           //�������е��������
	virtual BOOL    GetData(BYTE* byData);           //��ȡ���е��������

public:
#ifdef WIN32
	virtual void    NotifyMessage(DWORD dwMsg, WPARAM wParam, LPARAM lParam);	
//	void 			AddListInfo(LOG_INFO_LEVEL eLogInfoLevel, char* param, ...);
#endif
	
protected:
	CMediaItem*     m_lpParent;
	CMediaItem*     m_lpNext;
	CMediaItem*     m_lpChild;
	char            m_sName[MAX_PATH];

protected:
	MEDIAITEM_TYPE  m_eMediaType;
	int             m_nID;
//	CLog*           m_lpLog;
	HWND            m_hMsgWnd;

};

typedef struct RecvDataInfo
{
	EDVHEAD edvHead;
	DWORD   dwRemoteIP;
	WORD    wRemotePort;
	RecvDataInfo()
	{
		ZeroMemory(&edvHead,sizeof(EDVHEAD));
		dwRemoteIP=0;
		wRemotePort=0;
	}
}RecvDataInfo,*LPRecvDataInfo;

class CMediaItemDevice : public CMediaItem
{
public:
	CMediaItemDevice(int nID);
	virtual ~CMediaItemDevice();

#ifdef WIN32
	virtual int    Start(UNGB_DEVCONFIGREC &stDevConfig,CSdkProxy* pSdkProxy); //0--�ɹ� -1--ʧ��
#else
	virtual int    Start(UNGB_DEVCONFIGREC &stDevConfig); //0--�ɹ� -1--ʧ��
#endif
	int            Close();
	int            Close(int idialog_index,BOOL bReal = FALSE);  //�Ƿ���ȫ�ر�
    virtual int    CheckAllCh();  //���ͨ�����
	//��ȡ�ڲ�����
	int            UpdateUnitName();   //����ͨ������
	string         GetDeviceName(){return m_stDevConfig.strDeviceName;};
	int            GetDeviceType(){return m_stDevConfig.iDeviceType;}; 
	int            GetConnectHandle(){return m_hConHandle;};
	int            GetUnitState(string strLogicDeviceID,CHANNELITEM_STATE &eChannelState);     //��ȡͨ����Ϣ
	int            GetAllUnitInfo(list<UNITINFO> &lstUnitInfo);
    virtual char*  GetName();
    string         GetDevUserName(){return m_stDevConfig.strUseName;};
    string         GetDevPwd(){return m_stDevConfig.strPassword;};
    string         GetDevIP(){return m_stDevConfig.strDeviceIP;};
	int            GetMapRealInfo(map<string,CMediaItemReal*> &mapReal){mapReal = m_mapRealPool;return 0;};
	virtual int    SetDevCfg(UNGB_DEVCONFIGREC &stDevConfig){m_stDevConfig = stDevConfig;return 0;};
#ifdef WIN32
	int            GetRecordInfo(string strDeviceID,
		                         SYSTEMTIME sStartTime,SYSTEMTIME sStopTime,
								 EDVDVRRECORDTABLE* lpTable,int nTableCount,int& iTotalCount); //��ѯ¼���¼
#endif
	virtual int    ConnectDevice();     //0--�ɹ� -1--ʧ��
	virtual void   DisConnectDevice();
	int            ConvertMapChannel2int(string strMapChannel);
	int            ConvertEv8kCh2Int(char* sChID);  //ת��Ev8kͨ��
	void           StrSplit(const string &str,const char delimter,vector<string> &strList);

    //�ϱ���������״̬
	virtual int    SendAliveStatusMsg(CMediaItemReal* pUnit);
	int	   OnAliveStatus();
#ifdef WIN32
	static DWORD WINAPI ThreadAliveStatus(LPVOID pUser)
	{
		CMediaItemDevice* pThis = (CMediaItemDevice*)pUser;
		return pThis->OnAliveStatus();
	}
#else
	static void* ThreadAliveStatus(LPVOID pUser)
	{
		CMediaItemDevice* pThis = (CMediaItemDevice*)pUser;
		pThis->OnAliveStatus();
		return NULL;
	}
#endif

	///////////////
	//ʵʱ��Ƶ�ӿ�
	///////////////
	int             CreateReal(UNGB_CHCONFIGREC &stChConfig,BOOL bInit = FALSE);       //����ͨ��(ϵͳ��ʼ��ʱ)
	int             OpenReal(STREAM_CON_INFO stStreamConInfo,UNITOPENTYPE eOpenType);  //��ͨ��
	virtual int     StopReal(int idialog_index,BOOL bReal = FALSE);  //�Ƿ���ȫ�ر�
	
	//////////////
	//�طŽӿ�
	//////////////
#ifdef SUP_PLAYBACK
	int             FindPlayBack(int idialog_index,CPlayBack* &pPlayBack);
	virtual int     OpenPlayBack(STREAM_CON_INFO &stStreamConInfo,UNITOPENTYPE eOpenType);               
	virtual int     StopPlayBack(int idialog_index);
	int             PlayBackCtrl(int idialog_index,RECORD_CTRL eRecordCtrl,int nCtrlData,int* nReturnData);
#endif

	/////////////
	//���ƽӿ�
	/////////////
	virtual int     MakeKeyFrame(string strDeviceID);
	virtual int     PTZCtrl(string strDeviceID,char* lpSendBuf, DWORD dwBufSize);      //��������
	virtual int     SetVideoParam(string strDeviceID, BYTE byType, BYTE byParam);      //���ò���
	/********************************************************************
    ���ܣ���ȡͼ�����
    ������dwParam����ȡ���Ĳ���  ���ֽ����Σ�ɫ�� ���� �Աȶ� ���Ͷ�
    ���أ�0:�ɹ�
    ˵�����ڴ�ʵʱ��Ƶ��������ģ���ڲ���������ʱ��Ч��
    *********************************************************************/
	virtual int     GetVideoParam(string strDeviceID,unsigned int& nParam);    //��ȡ����

public:
#ifdef WIN32
	CSdkProxy                   *m_pSdkProxy;       //ͨ��Sdk������(����ָ����SDK)
#endif

#ifdef WIN32
private:
#else
public:
#endif
	int                         m_hConHandle;       //���Ӿ��
	UNGB_DEVCONFIGREC           m_stDevConfig;      //�豸��Ϣ(UNGBPhyDeviceConfig) 
	map<string,CMediaItemReal*> m_mapRealPool;      //ȫ��ͨ����ɵ�ʵʱ��Ƶ��map<LogicDeviceID, ��Ƶ����ָ��>
	map<int,CMediaItemReal*>    m_mapRealWorkQueue; //ʵʱ��Ƶ��������map<dlg_index, ��Ƶ����ָ��>
#ifdef SUP_PLAYBACK
	map<int,CPlayBack*>         m_mapPlayBack;      //�ط�map<Sip_Dlgindex, �طŶ���ָ��>
	CCritSec                    m_csDevOpLock;
#else
	pthread_mutex_t	          m_csDevOpLock;
#endif
	BOOL	    m_bThreadAliveStatusFlag;           //����״̬�߳�  
#ifdef WIN32
	HANDLE	    m_hThreadAliveStatus;
	DWORD	    m_dwThreadAliveStatusID;
#else
	pthread_t   m_threadAliveStatusHandle;
#endif
};

#ifndef WIN32
class CMediaItemDeviceOnvif : public CMediaItemDevice
{
public:
	CMediaItemDeviceOnvif(int nID):CMediaItemDevice(nID)
	{
		m_pOnvifOp = NULL;
	}
	virtual ~CMediaItemDeviceOnvif()
	{
        Close();
		DisConnectDevice();  //�Ͽ����Ӻͱ�����
		if(m_pOnvifOp)
		{
			delete m_pOnvifOp;
            m_pOnvifOp = NULL;
		}
	}
	int ConnectDevice();     //0--�ɹ� -1--ʧ��
	void DisConnectDevice();
	//�ϱ���������״̬
	int SendAliveStatusMsg(CMediaItemReal* pReal);
	int            MakeKeyFrame(string strDeviceID);
	int            GetUnitInfo(UNITINFO &stUnitInfo);              //��ȡͨ����Ϣ
	int            SetChConfig(UNGB_CHCONFIGREC &stChConfig);
    int            PTZCtrl(string strDeviceID,char* lpSendBuf, DWORD dwBufSize);      //��������
    int            SetVideoParam(string strDeviceID, BYTE byType, BYTE byParam);      //���ò���
	int            SetDevCfg(UNGB_DEVCONFIGREC &stDevConfig);
	/********************************************************************
    ���ܣ���ȡͼ�����
    ������dwParam����ȡ���Ĳ���  ���ֽ����Σ�ɫ�� ���� �Աȶ� ���Ͷ�
    ���أ�0:�ɹ�
    ˵�����ڴ�ʵʱ��Ƶ��������ģ���ڲ���������ʱ��Ч��
    *********************************************************************/
	int            GetVideoParam(string strDeviceID,unsigned int& nParam);    //��ȡ����
	int            GetChannelID();
private:
	OnvifOp*  m_pOnvifOp;
};
#endif

//ʵʱ��Ƶ
class CMediaItemReal : public CMediaItem
{
public:
	CMediaItemReal(int nID);
	virtual ~CMediaItemReal();
	
public:
	virtual int    ConnectDevice();    //0--�ɹ� -1--ʧ��
	virtual void   DisConnectDevice();

	virtual int    StartStream(STREAM_CON_INFO &stStreamConInfo,UNITOPENTYPE eOpenType);
	virtual void   StopStream(BOOL bReal = FALSE);  //�Ƿ���ȫ�ر�
	virtual void   FreeRealResource(BOOL bReal);  //�ͷ���Դ
	
	BOOL           NeedSendState();  //�Ƿ���Ҫ�ϱ�״̬
	void           RefreshState(){m_eOldChannelState = m_eCurChannelState;};  //ˢ��״̬
	virtual int    SetChConfig(UNGB_CHCONFIGREC &stChConfig);
	int            SetDeviceIP(string strDeviceIP){m_strDeviceIP = strDeviceIP;return 0;};
	int            SetSendStateMark(BOOL bSend){m_bSend = bSend;return 0;};
	int            SetOldChannelState(CHANNELITEM_STATE ChannelState){ m_eOldChannelState = ChannelState;return 0;};
	int            SetCurChannelState(CHANNELITEM_STATE ChannelState){ m_eCurChannelState = ChannelState;return 0;};
	CHANNELITEM_STATE  GetOldChannelState(){return m_eOldChannelState;};
	CHANNELITEM_STATE  GetCurChannelState(){return m_eCurChannelState;};
	virtual int    GetChannelID();     //��Ӧ���ݿ��е�MapChannelͨ����(����int)
	string         GetStrChannelID(){return m_stChConfig.strMapChannel;return 0;};  //��Ӧ���ݿ��е�MapChannelͨ����(����string)
	string         GetLogicDeviceID(){return m_stChConfig.strLogicDeviceID;};  //��Ӧͳһ21λ���
	string         GetChannelName(){return m_stChConfig.strChannelName;};
	virtual int    GetUnitInfo(UNITINFO &stUnitInfo);              //��ȡͨ����Ϣ
	virtual int    GetStreamConInfo(STREAM_CON_INFO& stStreamConInfo);
	DEVICE_TYPE    GetDeviceType(){return m_eDeviceType;}; 
	EDVOPENHANDLE  GetOpenHandle(){return m_hOpenHandle;};
 	EDVCONHANDLE   GetConnectHandle(){return m_hConnectHandle;};
	CMediaItemDevice* GetDevice();
	
#ifdef WIN32
	int            OnStreamData(EDVOPENHANDLE handle, LPDATA_INFO lpDataInfo, unsigned char* lpBuf, DWORD dwBufSize);
	static int CALLBACK StreamDataCallBack(EDVOPENHANDLE handle, LPDATA_INFO lpDataInfo, unsigned char* lpBuf, DWORD dwBufSize, LPVOID lpUser)
	{
		CMediaItemReal* lpRealPlay = (CMediaItemReal*)lpUser;		
		return lpRealPlay->OnStreamData(handle, lpDataInfo, lpBuf, dwBufSize);
	}

	///////////////////
	//��������
	///////////////////
	int	 OnSend(); 
	static DWORD WINAPI ThreadSend(LPVOID pUser)
	{
		CMediaItemReal* pThis = (CMediaItemReal*)pUser;
		return pThis->OnSend();
	}
    int            WriteRecFile(void*pbuffer,int iLen);   //д¼���ļ�
	int            WriteRecFilePs(void*pbuffer,int iLen);
	int            WriteRecFile2(void*pbuffer,int iLen);
	int            DealH264(unsigned char* lpBuf, DWORD dwBufSize);
	int            DealUnGBPs(unsigned char* lpBuf, DWORD dwBufSize);
	int            CheckStreamType(unsigned char* lpBuf, DWORD dwBufSize);

	//////////////////
	//���ps
	//////////////////
	int	 OnPsPack(); 
	static DWORD WINAPI ThreadPsPack(LPVOID pUser)
	{
		CMediaItemReal* pThis = (CMediaItemReal*)pUser;
		return pThis->OnPsPack();
	}
#endif

	FILE                 *m_fw;                   //¼����
	REALSTREAM_INFO      m_StreamInfo;          //����Ϣ
	EDVCONHANDLE         m_hConnectHandle;      //���Ӿ��
	EDVOPENHANDLE        m_hOpenHandle;	        //ʵʱ��Ƶ���
	DEVICE_TYPE          m_eDeviceType;         //��¼�豸����

	DWORD                m_dwTime;                  //�����ʾ��ʼ���͵�ʱ�� 
	LONGLONG             m_llRcvByteCount;          //���յ��ֽ���
	LONGLONG             m_llSendByteCount;         //���͵��ֽ���
	LONGLONG             m_llLastSendByteCount;     //��һ�λ�ȡ״̬ʱ�ֽ���
	
	UNGB_CHCONFIGREC     m_stChConfig;            //ͨ��������Ϣ
	string               m_strDeviceIP;           //��¼ͨ�������豸IP
	STREAM_CON_INFO      m_stStreamConInfo;       //����������Ϣ SrcIP  DestIP SrcPort DestPort
    CODECINFO            m_stCodecinfo;           //������������ ���� �Ƿ�����

#ifdef WIN32
	BOOL	             m_bThreadPsPackFlag;       //�����߳�
	HANDLE	             m_hThreadPsPack;
	DWORD	             m_dwThreadPsPackID;
	int                  m_hPsPack;               //ps���
	static  BOOL         m_bPsPackInited;      //��ʼ�������   false-δ��ʼ��
	CPSPackaging         *m_pCPSPackaging;
	BYTE                 *m_pPsPackBuf;
	BYTE                 *m_pH264Buf; 
    int                  m_dwDataSize;

	BOOL	             m_bThreadSendFlag;       //�����߳�
	HANDLE	             m_hThreadSend;
	DWORD	             m_dwThreadSendID;
	BYTE*                m_lpSendBuf;
	CDataSend            *m_lpDataSend;           //����ģ��
    CUdp2*               m_lpPlayUDP;             //udp����

	CMediaDataOp         *m_lpMediaDataOp;        //ת��ģ��
	BOOL                 m_bHeadInfoInited;       //ͷ��Ϣ��ʼ����־
	BOOL                 m_bFinishCheckStream;    //��������ͼ��
	int                  m_nCheckStreamType;      //�������������
	int                  m_nStreamType;           //������
#endif

	BOOL                 m_bStartMark;            //ִ�й�StartStream��־
	UNITOPENTYPE         m_eOpenType;             //������
#ifdef WIN32
	CCritSec             m_csRealOpLock;              //����Lock
#else
	pthread_mutex_t      m_csRealOpLock;            //����Lock
#endif

	CHANNELITEM_STATE    m_eOldChannelState;      //�ɵ�ͨ��״̬
    CHANNELITEM_STATE    m_eCurChannelState;      //��ǰͨ��״̬
	BOOL                 m_bSend;                 //�Ƿ��ϱ���  true -- �ϱ���  false -- δ�ϱ���
#ifdef WIN32
	T_FrameBuf           m_Frame;
#endif
	BOOL                 m_bCBYUVData;            //�Ƿ�ص�YUV����
};

#ifndef WIN32
//Onvif�豸
class CMediaItemRealOnvif : public CMediaItemReal
{
public:
	CMediaItemRealOnvif(int nID);
	virtual ~CMediaItemRealOnvif();

public:
	int            ConnectDevice();    //0--�ɹ� -1--ʧ��
	void           DisConnectDevice();
	int            StartStream(STREAM_CON_INFO &stStreamConInfo,UNITOPENTYPE eOpenType);
	void           StopStream(BOOL bReal = FALSE);  //�Ƿ���ȫ�ر�
private:
	OnvifOp*          m_pOnvifOp;
	STREAM_CON_INFO      m_stStreamConInfo;       //����������Ϣ SrcIP  DestIP SrcPort DestPort
};
#endif

#ifdef WIN32
//¼��طŽڵ�
class CPlayBack : public CMediaItem
{
public:
	CPlayBack(int nID);
	virtual ~CPlayBack();
	
public:
	LONG			Start();
	LONG            StartByTime();
	void			Stop();
	
	int             SetStreamConInfo(STREAM_CON_INFO &stStreamConInfo){m_stStreamConInfo = stStreamConInfo;return 0;};
	int             CtrlRecordStream(RECORD_CTRL eRecordCtrl, int nCtrlData, int* nReturnData);
	int             CtrlRecordStreamFront(RECORD_CTRL eRecordCtrl, int nCtrlData, int* nReturnData);
	void            SetKeepAlive(){m_nKeepAliveCount = 0;};
	int             GetKeepAlive(){m_nKeepAliveCount++;return m_nKeepAliveCount;};
	int             SetChConfig(UNGB_CHCONFIGREC &stChConfig);
	CMediaItemDevice* GetDevice();
	int             GetUnitInfo(UNITINFO &stUnitInfo);     //��ȡͨ����Ϣ
    void            Resume(){m_bPause=FALSE;};
	int             WriteRecFile(void*pbuffer,int iLen);   //д¼���ļ�

	int      OnStreamData(EDVOPENHANDLE handle, LPDATA_INFO lpDataInfo, unsigned char* lpBuf, DWORD dwBufSize);
	static int CALLBACK StreamDataCallBack(EDVOPENHANDLE handle, LPDATA_INFO lpDataInfo, unsigned char* lpBuf, DWORD dwBufSize, LPVOID lpUser)
	{
		CPlayBack* lpPlayBack= (CPlayBack*)lpUser;
		return lpPlayBack->OnStreamData(handle, lpDataInfo, lpBuf, dwBufSize);
	}
	
	int            DealH264(unsigned char* lpBuf, DWORD dwBufSize);
	int            DealUnGBPs(unsigned char* lpBuf, DWORD dwBufSize);
	int            CheckStreamType(unsigned char* lpBuf, DWORD dwBufSize);

	///////////////////
	//��������
	///////////////////
	int	 OnSend(); 
	static DWORD WINAPI ThreadSend(LPVOID pUser)
	{
		CPlayBack* pThis = (CPlayBack*)pUser;
		return pThis->OnSend();
	}

protected:
	int             ConnectDevice();
	void            DisConnectDevice();

private:
	CMediaDataOp         *m_lpMediaDataOp;        //ת��ģ��
	CPSPackaging         *m_pCPSPackaging;
	BYTE                 *m_pPsPackBuf;
    byte*                m_lpReadBuffer;          //readbuf
	FILE                 *m_fw;                   //¼����
	BOOL	             m_bThreadSendFlag;       //�����߳�
	HANDLE	             m_hThreadSend;
	DWORD	             m_dwThreadSendID;
	CDataSend            *m_lpDataSend;           //����ģ��
    int                  iCount;
	int                  iFrameFailCount;         //��ȡ֡ʧ�ܼ���
	BOOL                 m_bFinishCheckStream;    //��������ͼ��
	int                  m_nCheckStreamType;      //�������������
	int                  m_nStreamType;           //������

	UNGB_CHCONFIGREC     m_stChConfig;            //ͨ��������Ϣ
    STREAM_CON_INFO      m_stStreamConInfo;       //��������Ϣ
	CODECINFO            m_stCodecinfo;           //������������ ���� �Ƿ�����
	unsigned int         m_uiDataTime;            //¼������ʱ�� 
	
	SYSTEMTIME      m_sStart;
	SYSTEMTIME      m_sStop;
	SYSTEMTIME      m_sPlay;
	
	//LPMEDIA_UNIT_TABLE m_lpUnitTable;
	EDVCONHANDLE    m_hConnectHandle;
	EDVOPENHANDLE   m_hOpenHandle;
	int             m_nKeepAliveCount;
	
	BYTE            m_bRate;
	BOOL            m_bPause;
	BOOL            m_bEmpty;             //��ջ���
	CCritSec        m_csFrameQueLock;    //m_lpFramQue Lock
	unsigned int    m_uiSendCount;         //���ͼ���  
	DWORD           m_dwLastSendPause;     //�ϴη�����ͣ����ʱ��
	DWORD           m_dwLastSendResume;    //�ϴη��ͼ�������ʱ��
#ifdef WIN32
	T_FrameBuf      m_Frame;
#endif
#ifdef WIN32
	CCritSec        m_csPlayBackOpLock;            //����Lock
#else
	pthread_mutex_t m_csPlayBackOpLock;            //����Lock
#endif
};
#endif

#endif // !defined(AFX_MEDIAITEM_H__4F0E695E_E2B6_42F5_B0CA_CFE464A02DA2__INCLUDED_)
