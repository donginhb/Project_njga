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
#include "SdkProxy.h"    //sdk代理
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
class CMediaItemDevice;         //设备
class CMediaItemReal;           //实时
// class CMediaItemPlayBack;
// class CFindData;
#ifdef WIN32
class CPlayBack;                //录像
#endif

////////////////////////////////////////////////
//设备树
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
	virtual void    SetData(BYTE* byData);           //设置运行的相关数据
	virtual BOOL    GetData(BYTE* byData);           //获取运行的相关数据

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
	virtual int    Start(UNGB_DEVCONFIGREC &stDevConfig,CSdkProxy* pSdkProxy); //0--成功 -1--失败
#else
	virtual int    Start(UNGB_DEVCONFIGREC &stDevConfig); //0--成功 -1--失败
#endif
	int            Close();
	int            Close(int idialog_index,BOOL bReal = FALSE);  //是否完全关闭
    virtual int    CheckAllCh();  //检测通道情况
	//获取内部参数
	int            UpdateUnitName();   //更新通道名称
	string         GetDeviceName(){return m_stDevConfig.strDeviceName;};
	int            GetDeviceType(){return m_stDevConfig.iDeviceType;}; 
	int            GetConnectHandle(){return m_hConHandle;};
	int            GetUnitState(string strLogicDeviceID,CHANNELITEM_STATE &eChannelState);     //获取通道信息
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
								 EDVDVRRECORDTABLE* lpTable,int nTableCount,int& iTotalCount); //查询录像记录
#endif
	virtual int    ConnectDevice();     //0--成功 -1--失败
	virtual void   DisConnectDevice();
	int            ConvertMapChannel2int(string strMapChannel);
	int            ConvertEv8kCh2Int(char* sChID);  //转换Ev8k通道
	void           StrSplit(const string &str,const char delimter,vector<string> &strList);

    //上报离线在线状态
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
	//实时视频接口
	///////////////
	int             CreateReal(UNGB_CHCONFIGREC &stChConfig,BOOL bInit = FALSE);       //创建通道(系统初始化时)
	int             OpenReal(STREAM_CON_INFO stStreamConInfo,UNITOPENTYPE eOpenType);  //打开通道
	virtual int     StopReal(int idialog_index,BOOL bReal = FALSE);  //是否完全关闭
	
	//////////////
	//回放接口
	//////////////
#ifdef SUP_PLAYBACK
	int             FindPlayBack(int idialog_index,CPlayBack* &pPlayBack);
	virtual int     OpenPlayBack(STREAM_CON_INFO &stStreamConInfo,UNITOPENTYPE eOpenType);               
	virtual int     StopPlayBack(int idialog_index);
	int             PlayBackCtrl(int idialog_index,RECORD_CTRL eRecordCtrl,int nCtrlData,int* nReturnData);
#endif

	/////////////
	//控制接口
	/////////////
	virtual int     MakeKeyFrame(string strDeviceID);
	virtual int     PTZCtrl(string strDeviceID,char* lpSendBuf, DWORD dwBufSize);      //控球命令
	virtual int     SetVideoParam(string strDeviceID, BYTE byType, BYTE byParam);      //设置参数
	/********************************************************************
    功能：获取图像参数
    参数：dwParam：获取到的参数  四字节依次：色度 亮度 对比度 饱和度
    返回：0:成功
    说明：在打开实时视频，且设置模块内部发送码流时有效。
    *********************************************************************/
	virtual int     GetVideoParam(string strDeviceID,unsigned int& nParam);    //获取参数

public:
#ifdef WIN32
	CSdkProxy                   *m_pSdkProxy;       //通用Sdk代理类(代表指定的SDK)
#endif

#ifdef WIN32
private:
#else
public:
#endif
	int                         m_hConHandle;       //连接句柄
	UNGB_DEVCONFIGREC           m_stDevConfig;      //设备信息(UNGBPhyDeviceConfig) 
	map<string,CMediaItemReal*> m_mapRealPool;      //全部通道组成的实时视频池map<LogicDeviceID, 视频对象指针>
	map<int,CMediaItemReal*>    m_mapRealWorkQueue; //实时视频工作队列map<dlg_index, 视频对象指针>
#ifdef SUP_PLAYBACK
	map<int,CPlayBack*>         m_mapPlayBack;      //回放map<Sip_Dlgindex, 回放对象指针>
	CCritSec                    m_csDevOpLock;
#else
	pthread_mutex_t	          m_csDevOpLock;
#endif
	BOOL	    m_bThreadAliveStatusFlag;           //发送状态线程  
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
		DisConnectDevice();  //断开连接和保活检测
		if(m_pOnvifOp)
		{
			delete m_pOnvifOp;
            m_pOnvifOp = NULL;
		}
	}
	int ConnectDevice();     //0--成功 -1--失败
	void DisConnectDevice();
	//上报离线在线状态
	int SendAliveStatusMsg(CMediaItemReal* pReal);
	int            MakeKeyFrame(string strDeviceID);
	int            GetUnitInfo(UNITINFO &stUnitInfo);              //获取通道信息
	int            SetChConfig(UNGB_CHCONFIGREC &stChConfig);
    int            PTZCtrl(string strDeviceID,char* lpSendBuf, DWORD dwBufSize);      //控球命令
    int            SetVideoParam(string strDeviceID, BYTE byType, BYTE byParam);      //设置参数
	int            SetDevCfg(UNGB_DEVCONFIGREC &stDevConfig);
	/********************************************************************
    功能：获取图像参数
    参数：dwParam：获取到的参数  四字节依次：色度 亮度 对比度 饱和度
    返回：0:成功
    说明：在打开实时视频，且设置模块内部发送码流时有效。
    *********************************************************************/
	int            GetVideoParam(string strDeviceID,unsigned int& nParam);    //获取参数
	int            GetChannelID();
private:
	OnvifOp*  m_pOnvifOp;
};
#endif

//实时视频
class CMediaItemReal : public CMediaItem
{
public:
	CMediaItemReal(int nID);
	virtual ~CMediaItemReal();
	
public:
	virtual int    ConnectDevice();    //0--成功 -1--失败
	virtual void   DisConnectDevice();

	virtual int    StartStream(STREAM_CON_INFO &stStreamConInfo,UNITOPENTYPE eOpenType);
	virtual void   StopStream(BOOL bReal = FALSE);  //是否完全关闭
	virtual void   FreeRealResource(BOOL bReal);  //释放资源
	
	BOOL           NeedSendState();  //是否需要上报状态
	void           RefreshState(){m_eOldChannelState = m_eCurChannelState;};  //刷新状态
	virtual int    SetChConfig(UNGB_CHCONFIGREC &stChConfig);
	int            SetDeviceIP(string strDeviceIP){m_strDeviceIP = strDeviceIP;return 0;};
	int            SetSendStateMark(BOOL bSend){m_bSend = bSend;return 0;};
	int            SetOldChannelState(CHANNELITEM_STATE ChannelState){ m_eOldChannelState = ChannelState;return 0;};
	int            SetCurChannelState(CHANNELITEM_STATE ChannelState){ m_eCurChannelState = ChannelState;return 0;};
	CHANNELITEM_STATE  GetOldChannelState(){return m_eOldChannelState;};
	CHANNELITEM_STATE  GetCurChannelState(){return m_eCurChannelState;};
	virtual int    GetChannelID();     //对应数据库中的MapChannel通道号(类型int)
	string         GetStrChannelID(){return m_stChConfig.strMapChannel;return 0;};  //对应数据库中的MapChannel通道号(类型string)
	string         GetLogicDeviceID(){return m_stChConfig.strLogicDeviceID;};  //对应统一21位编号
	string         GetChannelName(){return m_stChConfig.strChannelName;};
	virtual int    GetUnitInfo(UNITINFO &stUnitInfo);              //获取通道信息
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
	//发送数据
	///////////////////
	int	 OnSend(); 
	static DWORD WINAPI ThreadSend(LPVOID pUser)
	{
		CMediaItemReal* pThis = (CMediaItemReal*)pUser;
		return pThis->OnSend();
	}
    int            WriteRecFile(void*pbuffer,int iLen);   //写录像文件
	int            WriteRecFilePs(void*pbuffer,int iLen);
	int            WriteRecFile2(void*pbuffer,int iLen);
	int            DealH264(unsigned char* lpBuf, DWORD dwBufSize);
	int            DealUnGBPs(unsigned char* lpBuf, DWORD dwBufSize);
	int            CheckStreamType(unsigned char* lpBuf, DWORD dwBufSize);

	//////////////////
	//打包ps
	//////////////////
	int	 OnPsPack(); 
	static DWORD WINAPI ThreadPsPack(LPVOID pUser)
	{
		CMediaItemReal* pThis = (CMediaItemReal*)pUser;
		return pThis->OnPsPack();
	}
#endif

	FILE                 *m_fw;                   //录像句柄
	REALSTREAM_INFO      m_StreamInfo;          //流信息
	EDVCONHANDLE         m_hConnectHandle;      //连接句柄
	EDVOPENHANDLE        m_hOpenHandle;	        //实时视频句柄
	DEVICE_TYPE          m_eDeviceType;         //记录设备类型

	DWORD                m_dwTime;                  //非零表示开始发送的时间 
	LONGLONG             m_llRcvByteCount;          //接收的字节数
	LONGLONG             m_llSendByteCount;         //发送的字节数
	LONGLONG             m_llLastSendByteCount;     //上一次获取状态时字节数
	
	UNGB_CHCONFIGREC     m_stChConfig;            //通道配置信息
	string               m_strDeviceIP;           //记录通道所属设备IP
	STREAM_CON_INFO      m_stStreamConInfo;       //码流连接信息 SrcIP  DestIP SrcPort DestPort
    CODECINFO            m_stCodecinfo;           //码流类型现象 类型 是否编解码

#ifdef WIN32
	BOOL	             m_bThreadPsPackFlag;       //发送线程
	HANDLE	             m_hThreadPsPack;
	DWORD	             m_dwThreadPsPackID;
	int                  m_hPsPack;               //ps打包
	static  BOOL         m_bPsPackInited;      //初始化打包器   false-未初始化
	CPSPackaging         *m_pCPSPackaging;
	BYTE                 *m_pPsPackBuf;
	BYTE                 *m_pH264Buf; 
    int                  m_dwDataSize;

	BOOL	             m_bThreadSendFlag;       //发送线程
	HANDLE	             m_hThreadSend;
	DWORD	             m_dwThreadSendID;
	BYTE*                m_lpSendBuf;
	CDataSend            *m_lpDataSend;           //发送模块
    CUdp2*               m_lpPlayUDP;             //udp发送

	CMediaDataOp         *m_lpMediaDataOp;        //转码模块
	BOOL                 m_bHeadInfoInited;       //头信息初始化标志
	BOOL                 m_bFinishCheckStream;    //完成流类型检测
	int                  m_nCheckStreamType;      //检测所得流类型
	int                  m_nStreamType;           //流类型
#endif

	BOOL                 m_bStartMark;            //执行过StartStream标志
	UNITOPENTYPE         m_eOpenType;             //打开类型
#ifdef WIN32
	CCritSec             m_csRealOpLock;              //操作Lock
#else
	pthread_mutex_t      m_csRealOpLock;            //操作Lock
#endif

	CHANNELITEM_STATE    m_eOldChannelState;      //旧的通道状态
    CHANNELITEM_STATE    m_eCurChannelState;      //当前通道状态
	BOOL                 m_bSend;                 //是否上报过  true -- 上报过  false -- 未上报过
#ifdef WIN32
	T_FrameBuf           m_Frame;
#endif
	BOOL                 m_bCBYUVData;            //是否回调YUV数据
};

#ifndef WIN32
//Onvif设备
class CMediaItemRealOnvif : public CMediaItemReal
{
public:
	CMediaItemRealOnvif(int nID);
	virtual ~CMediaItemRealOnvif();

public:
	int            ConnectDevice();    //0--成功 -1--失败
	void           DisConnectDevice();
	int            StartStream(STREAM_CON_INFO &stStreamConInfo,UNITOPENTYPE eOpenType);
	void           StopStream(BOOL bReal = FALSE);  //是否完全关闭
private:
	OnvifOp*          m_pOnvifOp;
	STREAM_CON_INFO      m_stStreamConInfo;       //码流连接信息 SrcIP  DestIP SrcPort DestPort
};
#endif

#ifdef WIN32
//录像回放节点
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
	int             GetUnitInfo(UNITINFO &stUnitInfo);     //获取通道信息
    void            Resume(){m_bPause=FALSE;};
	int             WriteRecFile(void*pbuffer,int iLen);   //写录像文件

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
	//发送数据
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
	CMediaDataOp         *m_lpMediaDataOp;        //转码模块
	CPSPackaging         *m_pCPSPackaging;
	BYTE                 *m_pPsPackBuf;
    byte*                m_lpReadBuffer;          //readbuf
	FILE                 *m_fw;                   //录像句柄
	BOOL	             m_bThreadSendFlag;       //发送线程
	HANDLE	             m_hThreadSend;
	DWORD	             m_dwThreadSendID;
	CDataSend            *m_lpDataSend;           //发送模块
    int                  iCount;
	int                  iFrameFailCount;         //获取帧失败计数
	BOOL                 m_bFinishCheckStream;    //完成流类型检测
	int                  m_nCheckStreamType;      //检测所得流类型
	int                  m_nStreamType;           //流类型

	UNGB_CHCONFIGREC     m_stChConfig;            //通道配置信息
    STREAM_CON_INFO      m_stStreamConInfo;       //流连接信息
	CODECINFO            m_stCodecinfo;           //码流类型现象 类型 是否编解码
	unsigned int         m_uiDataTime;            //录像数据时间 
	
	SYSTEMTIME      m_sStart;
	SYSTEMTIME      m_sStop;
	SYSTEMTIME      m_sPlay;
	
	//LPMEDIA_UNIT_TABLE m_lpUnitTable;
	EDVCONHANDLE    m_hConnectHandle;
	EDVOPENHANDLE   m_hOpenHandle;
	int             m_nKeepAliveCount;
	
	BYTE            m_bRate;
	BOOL            m_bPause;
	BOOL            m_bEmpty;             //清空缓冲
	CCritSec        m_csFrameQueLock;    //m_lpFramQue Lock
	unsigned int    m_uiSendCount;         //发送计数  
	DWORD           m_dwLastSendPause;     //上次发送暂停命令时间
	DWORD           m_dwLastSendResume;    //上次发送继续命令时间
#ifdef WIN32
	T_FrameBuf      m_Frame;
#endif
#ifdef WIN32
	CCritSec        m_csPlayBackOpLock;            //操作Lock
#else
	pthread_mutex_t m_csPlayBackOpLock;            //操作Lock
#endif
};
#endif

#endif // !defined(AFX_MEDIAITEM_H__4F0E695E_E2B6_42F5_B0CA_CFE464A02DA2__INCLUDED_)
