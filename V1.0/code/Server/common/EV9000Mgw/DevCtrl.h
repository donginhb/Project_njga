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
#include "SdkProxy.h"    //sdk����
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

#define THREAD_OPEN_MAX        10     //���豸������߳���
#ifdef WIN32
typedef  CSdkProxy*  PSDKPROXY;       //�Ե�����SDK�����ڴ�����ӿ�
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
//�豸����ģ��		
class CDevCtrl  
{
public:
	CDevCtrl();
	virtual ~CDevCtrl();

	int  Start();      //��ʼ
	int  Close();      //����
	int  Close(CBSIPMSG stCbSipmsg,BOOL bReal = FALSE);               //closeͨ��

	/////////////////
	//ʵʱ��Ƶ�ӿ�
	/////////////////
	int  FindDevice(string strDeviceID,CMediaItemDevice* &pDevice); //����DeviceID����Device����
	int  OpenReal(STREAM_CON_INFO stStreamConInfo,UNITOPENTYPE eOpenType); //��ͨ�� added by chenyu 131015
    int  GetUnitState(string strLogicDeviceID,CHANNELITEM_STATE &eChannelState); //��ȡͨ����Ϣ
    
	////////////////
	//�طŽӿ�
	////////////////
#ifdef WIN32
	int  PlayBackOpen(STREAM_CON_INFO stStreamConInfo,UNITOPENTYPE eOpenType); 
    int  PlayBackStop(CBSIPMSG stCbSipmsg);
	int  PlayBackCtrl(CBSIPMSG stCbSipmsg,RECORD_CTRL eRecordCtrl,int nCtrlData,int* nReturnData);
#endif
	///////////////
	//���ƽӿ�
	///////////////
	int  PTZCtrl(string strDeviceID,string strPTZCmd); //����ӿ�
	int  PTZCtrl(string strDeviceID,AutoZoomInPara &stAutoZoomInPara); //����Ŵ���ƽӿ�
	int  SetVideoParam(string strDeviceID,string strType, string strValue);
	int  GetVideoParam(string strDeviceID,unsigned int& nParam);
	int  MakeKeyFrame(string strDeviceID);
    int  UpdateAllUnitName();
	//int  GetUnitInfo(string strLogicDeviceID,UNITINFO &stUnitInfo);  //��ȡͨ����Ϣ
#ifdef WIN32
	int  GetRecordInfo(string strDeviceID,
		               SYSTEMTIME sStartTime,SYSTEMTIME sStopTime,
					   EDVDVRRECORDTABLE* lpTable,int nTableCount,int& iTotalCount); //��ѯ¼���¼
#endif
	
	//PTZCmd������� ת���ӿ�
    int  PTZCmdParser(string str28181PTZCmd,BYTE* pszOutWisCmd,DWORD dwOutBufSize);
    int  String2Bytes(unsigned char* szSrc, unsigned char* pDst, int nDstMaxLen);  
	int  AutoZoomInCmdParser(AutoZoomInPara &stAutoZoomInPara,BYTE* pszOutWisCmd,DWORD dwOutBufSize);

	//added by chenyu 131014 ��Dev�߳�
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

	int  Init();        //��ʼ������
	int  Fini();        //�ͷ�����
	int  OpenAllDev();  //���豸
	int  CloseAllDev(); //�ر��豸
	int  CheckAllDev();  //����豸
	int  OpenDev(UNGB_DEVCONFIGREC &stDevConfigRec);                 //����m_lstMgwDev������
#ifdef WIN32
	int  LoadSDK(int iDevType,PSDKPROXY &pSdkProxy);                 //��̬�����豸SDK
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
  	//���ļ��߳�
	BOOL			m_bThreadOpenDevFlag[THREAD_OPEN_MAX];
#ifdef WIN32
	HANDLE			m_hThreadOpenDev[THREAD_OPEN_MAX];
	DWORD			m_dwThreadOpenDevID[THREAD_OPEN_MAX];
	CCritSec                      m_mapDevCfgLock;                 //�豸������
#else
	pthread_t     m_threadOpenDevHandle[THREAD_OPEN_MAX];
	pthread_mutex_t	              m_mapDevCfgLock;                 //�豸������
#endif

	map<int,UNGB_DEVCONFIGREC>::iterator m_iterDevCfg;               //ָ���豸�����б�
    map<int,UNGB_DEVCONFIGREC>    m_mapDevCfg;          
	map<int,CMediaItemDevice*>    m_mapDevItem;                      //�ṹmap->CMediaItemDevice->CMediaItemUnit
#ifdef WIN32
	CCritSec   m_mapDevItemLock; 
#else
	pthread_mutex_t	m_mapDevItemLock;
#endif
#ifdef WIN32
	list<CSdkProxy*>              m_lstSdkProxy;                      //sdk�����б�
	CCritSec                      m_lstSdkProxyLock;                  //sdk�����б���
	map<int,CString>              g_mapDllName;                       //sdk·���б�<�豸����,sdk·��>
#endif
};

#endif // !defined(AFX_DEVCTRL_H__E602FD87_0B64_4235_AFEC_67938EFAB936__INCLUDED_)
