// DevCtrl.h: interface for the CDevCtrl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEVCTRL_H__E602FD87_0B64_4235_AFEC_67938EFAB936__INCLUDED_)
#define AFX_DEVCTRL_H__E602FD87_0B64_4235_AFEC_67938EFAB936__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MgwPublicData.h"
#ifdef WIN32
#include "../public/Function/AutoLock.h"
#include "../public/net/Udp2.h"
#include "SdkProxy.h"    //sdk代理
#else
#include "AutoLock/CAutoLock.h"
#endif
#include "MediaItem.h"
#include "DbOp.h"        //db op
#include <iostream>
#include <list>
#include <map>
#include <string>
using namespace std;

#define THREAD_OPEN_MAX        10     //打开设备最大处理线程数
#ifdef WIN32
typedef  CSdkProxy*  PSDKPROXY;       //对第三方SDK加载内存后的类接口
#endif

typedef struct _AutoZoomInPara_
{
	string  strDisplayFrameWide;
	string  strDisplayFrameHigh;
	string  strDestMatrixTopLeftX;
	string  strDestMatrixTopLeftY;
	string  strDestMatrixWide;
	string  strDestMatrixHigh;
	_AutoZoomInPara_()
	{
		strDisplayFrameWide = "";
		strDisplayFrameHigh = "";
		strDestMatrixTopLeftX = "";
		strDestMatrixTopLeftY = "";
		strDestMatrixWide = "";
		strDestMatrixHigh = "";	
	}
}AutoZoomInPara;
//设备控制模块		
class CDevCtrl  
{
public:
	CDevCtrl();
	virtual ~CDevCtrl();

	int  Start();      //开始
	int  Close();      //结束
	int  Close(CBSIPMSG stCbSipmsg,BOOL bReal = FALSE);               //close通道

	/////////////////
	//实时视频接口
	/////////////////
	int  FindDevice(string strDeviceID,CMediaItemDevice* &pDevice); //根据DeviceID查找Device对象
	int  OpenReal(STREAM_CON_INFO stStreamConInfo,UNITOPENTYPE eOpenType); //打开通道 added by chenyu 131015
    int  GetUnitState(string strLogicDeviceID,CHANNELITEM_STATE &eChannelState); //获取通道信息
    
	////////////////
	//回放接口
	////////////////
#ifdef WIN32
	int  PlayBackOpen(STREAM_CON_INFO stStreamConInfo,UNITOPENTYPE eOpenType); 
    int  PlayBackStop(CBSIPMSG stCbSipmsg);
	int  PlayBackCtrl(CBSIPMSG stCbSipmsg,RECORD_CTRL eRecordCtrl,int nCtrlData,int* nReturnData);
#endif
	///////////////
	//控制接口
	///////////////
	int  PTZCtrl(string strDeviceID,string strPTZCmd); //控球接口
	int  PTZCtrl(string strDeviceID,AutoZoomInPara &stAutoZoomInPara); //点击放大控制接口
	int  SetVideoParam(string strDeviceID,string strType, string strValue);
	int  GetVideoParam(string strDeviceID,unsigned int& nParam);
	int  MakeKeyFrame(string strDeviceID);
    int  UpdateAllUnitName();
	//int  GetUnitInfo(string strLogicDeviceID,UNITINFO &stUnitInfo);  //获取通道信息
#ifdef WIN32
	int  GetRecordInfo(string strDeviceID,
		               SYSTEMTIME sStartTime,SYSTEMTIME sStopTime,
					   EDVDVRRECORDTABLE* lpTable,int nTableCount,int& iTotalCount); //查询录像记录
#endif
	
	//PTZCmd命令解析 转换接口
    int  PTZCmdParser(string str28181PTZCmd,BYTE* pszOutWisCmd,DWORD dwOutBufSize);
    int  String2Bytes(unsigned char* szSrc, unsigned char* pDst, int nDstMaxLen);  
	int  AutoZoomInCmdParser(AutoZoomInPara &stAutoZoomInPara,BYTE* pszOutWisCmd,DWORD dwOutBufSize);

	//added by chenyu 131014 打开Dev线程
	virtual	int		OnThOpenDev();
	
#ifdef WIN32
	static DWORD WINAPI ThreadOpenDev(LPVOID pUser)
	{
		CDevCtrl* pThis = (CDevCtrl*)pUser;
		
		return pThis->OnThOpenDev();
	}
#else
	static void* ThreadOpenDev(LPVOID pUser)
	{
		CDevCtrl* pThis = (CDevCtrl*)pUser;
		pThis->OnThOpenDev();
		return NULL;
	}
#endif

	int  Init();        //初始化配置
	int  Fini();        //释放配置
	int  OpenAllDev();  //打开设备
	int  CloseAllDev(); //关闭设备
	int  CheckAllDev();  //检测设备
	int  OpenDev(UNGB_DEVCONFIGREC &stDevConfigRec);                 //返回m_lstMgwDev数组编号
#ifdef WIN32
	int  LoadSDK(int iDevType,PSDKPROXY &pSdkProxy);                 //动态加载设备SDK
#endif

#ifdef WIN32
	int  AddDev(UNGB_DEVCONFIGREC &stDevConfig,CSdkProxy* pSdkProxy); 
	int  DelDev(UNGB_DEVCONFIGREC &stDevConfig,CSdkProxy* pSdkProxy);
#else
	int  AddDev(UNGB_DEVCONFIGREC &stDevConfig);
	int  DelDev(UNGB_DEVCONFIGREC &stDevConfig);
#endif
	int  GetDevItem(map<int,CMediaItemDevice*> &mapDevItem){mapDevItem = m_mapDevItem; return 0;}; 

protected:
  	//打开文件线程
	BOOL			m_bThreadOpenDevFlag[THREAD_OPEN_MAX];
#ifdef WIN32
	HANDLE			m_hThreadOpenDev[THREAD_OPEN_MAX];
	DWORD			m_dwThreadOpenDevID[THREAD_OPEN_MAX];
	CCritSec                      m_mapDevCfgLock;                 //设备配置锁
#else
	pthread_t     m_threadOpenDevHandle[THREAD_OPEN_MAX];
	pthread_mutex_t	              m_mapDevCfgLock;                 //设备配置锁
#endif

	map<int,UNGB_DEVCONFIGREC>::iterator m_iterDevCfg;               //指向设备配置列表
    map<int,UNGB_DEVCONFIGREC>    m_mapDevCfg;          
	map<int,CMediaItemDevice*>    m_mapDevItem;                      //结构map->CMediaItemDevice->CMediaItemUnit
#ifdef WIN32
	CCritSec   m_mapDevItemLock; 
#else
	pthread_mutex_t	m_mapDevItemLock;
#endif
#ifdef WIN32
	list<CSdkProxy*>              m_lstSdkProxy;                      //sdk代理列表
	CCritSec                      m_lstSdkProxyLock;                  //sdk代理列表锁
	map<int,CString>              g_mapDllName;                       //sdk路径列表<设备类型,sdk路径>
#endif
};

#endif // !defined(AFX_DEVCTRL_H__E602FD87_0B64_4235_AFEC_67938EFAB936__INCLUDED_)
