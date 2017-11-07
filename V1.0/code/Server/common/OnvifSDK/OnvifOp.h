/*
 * OnvifOp.h
 *
 *  Created on: 2014-3-10
 *      Author: root
 */

#ifndef ONVIFOP_H_
#define ONVIFOP_H_

#include "StdAfx.h"
#ifdef WIN32
#include "../OnvifProbe/OnvifDiscovery.h"  //设备发现
#include "../public/Function/EV9000Func.h"
#endif
#include "../../common/include/EV9000_InnerDef.h"
//#include "MgwPublicData.h"
#ifdef __linux__
#include "CLog.h"  //对应库libEV9000ComTool.so
#endif
/**********************Onvif云台控制命令 begin*************************/
#define LIGHT_PWRON		2	/* 接通灯光电源 */
#define WIPER_PWRON		3	/* 接通雨刷开关 */
#define FAN_PWRON		4	/* 接通风扇开关 */
#define HEATER_PWRON	5	/* 接通加热器开关 */
#define AUX_PWRON1		6	/* 接通辅助设备开关 */
#define AUX_PWRON2		7	/* 接通辅助设备开关 */
#define SET_PRESET		8	/* 设置预置点 */
#define CLE_PRESET		9	/* 清除预置点 */

#define ZOOM_IN			11	/* 焦距以速度SS变大(倍率变大) */
#define ZOOM_OUT		12	/* 焦距以速度SS变小(倍率变小) */
#define FOCUS_NEAR      13  /* 焦点以速度SS前调 */
#define FOCUS_FAR       14  /* 焦点以速度SS后调 */
#define IRIS_OPEN       15  /* 光圈以速度SS扩大 */
#define IRIS_CLOSE      16  /* 光圈以速度SS缩小 */

#define TILT_UP			21	/* 云台以SS的速度上仰 */
#define TILT_DOWN		22	/* 云台以SS的速度下俯 */
#define PAN_LEFT		23	/* 云台以SS的速度左转 */
#define PAN_RIGHT		24	/* 云台以SS的速度右转 */
#define UP_LEFT			25	/* 云台以SS的速度上仰和左转 */
#define UP_RIGHT		26	/* 云台以SS的速度上仰和右转 */
#define DOWN_LEFT		27	/* 云台以SS的速度下俯和左转 */
#define DOWN_RIGHT		28	/* 云台以SS的速度下俯和右转 */
#define PAN_AUTO		29	/* 云台以SS的速度左右自动扫描 */

#define FILL_PRE_SEQ	30	/* 将预置点加入巡航序列 */
#define SET_SEQ_DWELL	31	/* 设置巡航点停顿时间 */
#define SET_SEQ_SPEED	32	/* 设置巡航速度 */
#define CLE_PRE_SEQ		33	/* 将预置点从巡航序列中删除 */
#define STA_MEM_CRUISE	34	/* 开始记录轨迹 */
#define STO_MEM_CRUISE	35	/* 停止记录轨迹 */
#define RUN_CRUISE		36	/* 开始轨迹 */
#define RUN_SEQ			37	/* 开始巡航 */
#define STOP_SEQ		38	/* 停止巡航 */
#define GOTO_PRESET		39	/* 快球转到预置点 */

/**********************Onvif云台控制命令 end*************************/

#include <iostream>
#include <string>
using namespace std;

#define KEEPALIVE_FAILD_COUNT      3     //3*10秒
#define DEV_PASSWORD "12345"
#define MAX_HOSTNAME_LEN 128
//#include "OnvifAPIAll/DeviceBinding.nsmap"
#include "OnvifAPIAll/wsseapi.h"
#include "OnvifAPIAll/wsaapi.h"

#include "OnvifAPIAll/soapDeviceBindingProxy.h"
#include "OnvifAPIAll/soapMediaBindingProxy.h"
#include "OnvifAPIAll/soapPTZBindingProxy.h"

#include "OnvifAPIAll/soapEventBindingProxy.h"
#include "OnvifAPIAll/soapNotificationProducerBindingProxy.h"
#include "OnvifAPIAll/soapDiscoveryLookupBindingProxy.h"
#include "OnvifAPIAll/soapImagingBindingProxy.h"
#include "OnvifAPIAll/soapPullPointSubscriptionBindingProxy.h"

typedef struct _OnvifImageInfo_  
{
	int  nBrightness;                               //亮度
	int  nSaturation;                               //饱和度
	int  nContrast;                                 //对比度
	int  nColourDegree;                             //色度
	int  nResved1;                                  //保留1
	int  nResved2;                                  //保留1
	char          strResved1[64];                            //保留2 
	_OnvifImageInfo_()
	{
		nBrightness = -1;
		nSaturation = -1;
		nContrast = -1;
		nColourDegree = -1;
	}
}ONVIF_IMAGE_INFO;

typedef struct _OnvifOptions_
{
	int  nBrightnessMin;                               //亮度
	int  nSaturationMin;                               //饱和度
	int  nContrastMin;                                 //对比度
	int  nColourDegreeMin;                             //色度
	int  nBrightnessMax;                               //亮度
	int  nSaturationMax;                               //饱和度
	int  nContrastMax;                                 //对比度
	int  nColourDegreeMax;                             //色度
	int  nBrightnessRange;                               //亮度
	int  nSaturationRange;                               //饱和度
	int  nContrastRange;                                 //对比度
	int  nColourDegreeRange;                             //色度
	int  nResved1;                                  //保留1
	int  nResved2;                                  //保留1
	char          strResved1[64];                            //保留2 
}ONVIF_OPTIONS;

typedef struct _OnvifMoveOptions_
{
	int  nContinuousMin;                            
	int  nContinuousMax;                            
	int  nResved1;                                  //保留1
	int  nResved2;                                  //保留1
	char          strResved1[64];                   //保留2 
}ONVIF_MOVE_OPTIONS;

typedef struct _OnvifPtzMoveOptions_
{
	int  nPanMin;                            
	int  nPanMax; 
	int  nTiltMin;                            
	int  nTiltMax; 
	int  nZoomMin;                            
	int  nZoomMax; 
	int  nPanRange;
	int  nTiltRange;
	int  nZoomRange;
	int  nResved1;                                  //保留1
	int  nResved2;                                  //保留1
	char          strResved1[64];                   //保留2 
	_OnvifPtzMoveOptions_()
	{
		nPanMin = -1;                            
		nPanMax = 1; 
		nTiltMin = -1;                            
		nTiltMax = 1; 
		nZoomMin = -1;                            
		nZoomMax = 1;
		nPanRange = 2;
		nTiltRange = 2;
		nZoomRange = 2;
	}
}ONVIF_PTZMOVE_OPTIONS;

class  OnvifOp {
public:
	OnvifOp();
	virtual ~OnvifOp();
	int   Close();
	int   SetPara(char* szIP="172.168.1.1",char*szUsr="admin",char*szPwd="12345",int nPort=80)
	{
		if(szIP)
		m_strIP = szIP;
		if(szUsr)
		m_strUsr = szUsr;
		if(szPwd)
		m_strPwd = szPwd;
        m_nPort = nPort;
		//endpoint
		memset(szHostName,0,MAX_HOSTNAME_LEN);
		strcat(szHostName,"http://");
		strcat(szHostName,m_strIP.c_str());
		char szPort[10]={0};
		sprintf(szPort,":%d",m_nPort);
		strcat(szHostName,szPort);
		strcat(szHostName,"/onvif/device_service");

// 		proxyDevice.endpoint = szHostName;
// 	   //proxyDevice.endpoint = "http://172.18.13.153/onvif/device_service";
// 		soap_register_plugin(proxyDevice.soap,soap_wsse);
// 		soap_register_plugin(proxyMedia.soap,soap_wsse);
// 		soap_register_plugin(proxyImaging.soap,soap_wsa);
// 		soap_register_plugin(proxyPTZ.soap,soap_wsse);
		//soap_register_plugin(proxyEvent.soap,soap_wsse);
		//soap_register_plugin(proxyNP.soap,soap_wsse);
		//GetCapabilities();   //先获取一下能力集
		return 0;
	}
	int   VerifyPassword(int iTimeout=2);  //验证用户名密码
	int   GetLogicDevNum(int& iLogicDevNum,int iTimeout=2);
	int   GetLogicDevCfg(EV9000_OnvifLogicDeviceConfig* pLogicCfg,int iInDataLen,int iTimeout=2);
// #ifdef WIN32
// 	void  SetDevCfgCallBack(LPDEVCFGCALLBACK lpFunCall, LPVOID lpUserData);
// 	int   Discovery(int timeout=5,char* szLocalIP="127.0.0.1");
// 	int   StopDiscovery();
// #endif
	int   MakeKeyFrame();
	int   SetSystemDateAndTime();
	int   PTZCtrl(BYTE* lpBuffer, int nBufferLen);
	int   DoPTZControl(DWORD dwPTZCommand, DWORD dwStop, DWORD dwSpeed);
	int   DoContinuousMove(_tptz__ContinuousMove *tptz__ContinuousMove,
			                _tptz__ContinuousMoveResponse *tptz__ContinuousMoveResponse,
			                float px,float py,float zx,float zy,PTZBinding& proxyPTZ);
	int   PTZPreset(DWORD dwPTZCommand,DWORD dwPresetIndex);
	int   SetVideoParam( BYTE byType, BYTE byParam);
	int   SetVideoParam(ONVIF_IMAGE_INFO stParam);    //标准为[0,100] Onvif相机需做转换
	int   GetVideoParam(unsigned int& nParam);
	int   GetVideoParam(ONVIF_IMAGE_INFO &stParam);   //标准为[0,100] Onvif相机需做转换
	int   SetVideoFocusIris(DWORD dwPTZCommand,DWORD dwParam);  //设置光圈和聚焦
	int   SetVideoFocus(DWORD dwPTZCommand,DWORD dwParam);  //设置聚焦
	int   GetVideoIris(float &fIris);  //读取聚焦
	int   SetVideoIris(DWORD dwPTZCommand,DWORD dwParam,float fCurIris,int nSpan);  //设置聚焦 nSpan为调整值
	int   GetOptions();    //获取参数范围
	int   GetMoveOptions();
	int   SetFocusMode(tt__AutoFocusMode eFocusMode);
	int   GetPtzConfigurationOptions();
	int   GetEventProperties();
	int   CreatePullPoint(string& strSubscriptionEntry);
	int   PullMessages(string strEntry,string& strMsg,int& nAlarmType,int& nChannelMark);	

	
/*
//注销，用ping实现
	//保活接口
	int   StartCheckAlive(); //启动保活检测
	int   StopCheckAlive();  //停止保活检测
	int   GetAliveStatus();  //读取保活状态
	int   OnCheckAlive();
	static void* ThreadCheckAlive(LPVOID pUser)
	{
		OnvifOp* pThis = (OnvifOp*)pUser;
		pThis->OnCheckAlive();
		return NULL;
	}
	*/
	int   Set_soap_wsse_para(string strType,struct soap *soap); //设置web_service验证参数
	int   GetCapabilities();
	//int   InitProxy(int iTimeout=5);
	int   InitProxy(DeviceBinding*  pproxyDevice = NULL,
					   MediaBinding*   pproxyMedia = NULL,
					   ImagingBinding* pproxyImaging = NULL,
					   PTZBinding*     pproxyPTZ = NULL,
					   EventBinding*   pproxyEvent = NULL,     
					   int iTimeout = 4);

private:

// #ifdef WIN32  //设备发现linux不需要
// 	OnvifDiscovery * m_pOnvifDiscovery;
// #endif
// 	BOOL	      m_bThreadCheckAliveFlag;    //刷新注册
// 	pthread_t    m_threadCheckAliveHandle;
    //Onvif 保活计数 <
	int          m_nKeepAliveCount;

	//Onvif para
	string       m_strIP;
	string       m_strUsr;
	string       m_strPwd;
	int          m_nPort;
	char         szHostName[MAX_HOSTNAME_LEN];
	string       m_strVideoSourceToken;
	string       m_strProfileToken;
	//_tds__GetCapabilitiesResponse *tds__GetCapabilitiesResponse;  //通道参数
	bool         m_bGetCapabilitiesSuccess;     //成功获取参数
	bool         m_blSupportPTZ;

	string       m_proxyDeviceEntry;
	string       m_proxyMediaEntry;
	string       m_proxyImagingEntry;
	string       m_proxyPTZEntry;
	string       m_proxyEventEntry;
	string       m_proxyNPEntry;

	//Onvif操作对象
	struct soap     *soap;
// 	DeviceBinding   proxyDevice;
// 	MediaBinding    proxyMedia;
// 	ImagingBinding  proxyImaging;
// 	PTZBinding      proxyPTZ;
	//EventBinding    proxyEvent;
	//NotificationProducerBinding proxyNP;

	ONVIF_OPTIONS   m_stOptions;
	bool            m_bGetOptionsSuccess;     //成功获取参数
	bool            m_bGetMoveOptionsSuccess;     //成功获取参数
	ONVIF_MOVE_OPTIONS  m_stMoveOptions;
	bool            m_bGetPtzMoveOptionsSuccess;     //成功获取参数
	ONVIF_PTZMOVE_OPTIONS  m_stPtzMoveOptions;
	string          m_strPtzMoveOptionsNodeToken;      //GetPtzConfigurationOptions
	bool            m_bGetEventPropertiesSuccess;
	string          m_strFilterDialect;
	string          m_strTopicExpressionDialect;
	CMutexLock        g_OnvifOpLock;
};

#endif /* ONVIFOP_H_ */
