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
#include "../OnvifProbe/OnvifDiscovery.h"  //�豸����
#include "../public/Function/EV9000Func.h"
#endif
#include "../../common/include/EV9000_InnerDef.h"
//#include "MgwPublicData.h"
#ifdef __linux__
#include "CLog.h"  //��Ӧ��libEV9000ComTool.so
#endif
/**********************Onvif��̨�������� begin*************************/
#define LIGHT_PWRON		2	/* ��ͨ�ƹ��Դ */
#define WIPER_PWRON		3	/* ��ͨ��ˢ���� */
#define FAN_PWRON		4	/* ��ͨ���ȿ��� */
#define HEATER_PWRON	5	/* ��ͨ���������� */
#define AUX_PWRON1		6	/* ��ͨ�����豸���� */
#define AUX_PWRON2		7	/* ��ͨ�����豸���� */
#define SET_PRESET		8	/* ����Ԥ�õ� */
#define CLE_PRESET		9	/* ���Ԥ�õ� */

#define ZOOM_IN			11	/* �������ٶ�SS���(���ʱ��) */
#define ZOOM_OUT		12	/* �������ٶ�SS��С(���ʱ�С) */
#define FOCUS_NEAR      13  /* �������ٶ�SSǰ�� */
#define FOCUS_FAR       14  /* �������ٶ�SS��� */
#define IRIS_OPEN       15  /* ��Ȧ���ٶ�SS���� */
#define IRIS_CLOSE      16  /* ��Ȧ���ٶ�SS��С */

#define TILT_UP			21	/* ��̨��SS���ٶ����� */
#define TILT_DOWN		22	/* ��̨��SS���ٶ��¸� */
#define PAN_LEFT		23	/* ��̨��SS���ٶ���ת */
#define PAN_RIGHT		24	/* ��̨��SS���ٶ���ת */
#define UP_LEFT			25	/* ��̨��SS���ٶ���������ת */
#define UP_RIGHT		26	/* ��̨��SS���ٶ���������ת */
#define DOWN_LEFT		27	/* ��̨��SS���ٶ��¸�����ת */
#define DOWN_RIGHT		28	/* ��̨��SS���ٶ��¸�����ת */
#define PAN_AUTO		29	/* ��̨��SS���ٶ������Զ�ɨ�� */

#define FILL_PRE_SEQ	30	/* ��Ԥ�õ����Ѳ������ */
#define SET_SEQ_DWELL	31	/* ����Ѳ����ͣ��ʱ�� */
#define SET_SEQ_SPEED	32	/* ����Ѳ���ٶ� */
#define CLE_PRE_SEQ		33	/* ��Ԥ�õ��Ѳ��������ɾ�� */
#define STA_MEM_CRUISE	34	/* ��ʼ��¼�켣 */
#define STO_MEM_CRUISE	35	/* ֹͣ��¼�켣 */
#define RUN_CRUISE		36	/* ��ʼ�켣 */
#define RUN_SEQ			37	/* ��ʼѲ�� */
#define STOP_SEQ		38	/* ֹͣѲ�� */
#define GOTO_PRESET		39	/* ����ת��Ԥ�õ� */

/**********************Onvif��̨�������� end*************************/

#include <iostream>
#include <string>
using namespace std;

#define KEEPALIVE_FAILD_COUNT      3     //3*10��
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
	int  nBrightness;                               //����
	int  nSaturation;                               //���Ͷ�
	int  nContrast;                                 //�Աȶ�
	int  nColourDegree;                             //ɫ��
	int  nResved1;                                  //����1
	int  nResved2;                                  //����1
	char          strResved1[64];                            //����2 
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
	int  nBrightnessMin;                               //����
	int  nSaturationMin;                               //���Ͷ�
	int  nContrastMin;                                 //�Աȶ�
	int  nColourDegreeMin;                             //ɫ��
	int  nBrightnessMax;                               //����
	int  nSaturationMax;                               //���Ͷ�
	int  nContrastMax;                                 //�Աȶ�
	int  nColourDegreeMax;                             //ɫ��
	int  nBrightnessRange;                               //����
	int  nSaturationRange;                               //���Ͷ�
	int  nContrastRange;                                 //�Աȶ�
	int  nColourDegreeRange;                             //ɫ��
	int  nResved1;                                  //����1
	int  nResved2;                                  //����1
	char          strResved1[64];                            //����2 
}ONVIF_OPTIONS;

typedef struct _OnvifMoveOptions_
{
	int  nContinuousMin;                            
	int  nContinuousMax;                            
	int  nResved1;                                  //����1
	int  nResved2;                                  //����1
	char          strResved1[64];                   //����2 
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
	int  nResved1;                                  //����1
	int  nResved2;                                  //����1
	char          strResved1[64];                   //����2 
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
		//GetCapabilities();   //�Ȼ�ȡһ��������
		return 0;
	}
	int   VerifyPassword(int iTimeout=2);  //��֤�û�������
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
	int   SetVideoParam(ONVIF_IMAGE_INFO stParam);    //��׼Ϊ[0,100] Onvif�������ת��
	int   GetVideoParam(unsigned int& nParam);
	int   GetVideoParam(ONVIF_IMAGE_INFO &stParam);   //��׼Ϊ[0,100] Onvif�������ת��
	int   SetVideoFocusIris(DWORD dwPTZCommand,DWORD dwParam);  //���ù�Ȧ�;۽�
	int   SetVideoFocus(DWORD dwPTZCommand,DWORD dwParam);  //���þ۽�
	int   GetVideoIris(float &fIris);  //��ȡ�۽�
	int   SetVideoIris(DWORD dwPTZCommand,DWORD dwParam,float fCurIris,int nSpan);  //���þ۽� nSpanΪ����ֵ
	int   GetOptions();    //��ȡ������Χ
	int   GetMoveOptions();
	int   SetFocusMode(tt__AutoFocusMode eFocusMode);
	int   GetPtzConfigurationOptions();
	int   GetEventProperties();
	int   CreatePullPoint(string& strSubscriptionEntry);
	int   PullMessages(string strEntry,string& strMsg,int& nAlarmType,int& nChannelMark);	

	
/*
//ע������pingʵ��
	//����ӿ�
	int   StartCheckAlive(); //����������
	int   StopCheckAlive();  //ֹͣ������
	int   GetAliveStatus();  //��ȡ����״̬
	int   OnCheckAlive();
	static void* ThreadCheckAlive(LPVOID pUser)
	{
		OnvifOp* pThis = (OnvifOp*)pUser;
		pThis->OnCheckAlive();
		return NULL;
	}
	*/
	int   Set_soap_wsse_para(string strType,struct soap *soap); //����web_service��֤����
	int   GetCapabilities();
	//int   InitProxy(int iTimeout=5);
	int   InitProxy(DeviceBinding*  pproxyDevice = NULL,
					   MediaBinding*   pproxyMedia = NULL,
					   ImagingBinding* pproxyImaging = NULL,
					   PTZBinding*     pproxyPTZ = NULL,
					   EventBinding*   pproxyEvent = NULL,     
					   int iTimeout = 4);

private:

// #ifdef WIN32  //�豸����linux����Ҫ
// 	OnvifDiscovery * m_pOnvifDiscovery;
// #endif
// 	BOOL	      m_bThreadCheckAliveFlag;    //ˢ��ע��
// 	pthread_t    m_threadCheckAliveHandle;
    //Onvif ������� <
	int          m_nKeepAliveCount;

	//Onvif para
	string       m_strIP;
	string       m_strUsr;
	string       m_strPwd;
	int          m_nPort;
	char         szHostName[MAX_HOSTNAME_LEN];
	string       m_strVideoSourceToken;
	string       m_strProfileToken;
	//_tds__GetCapabilitiesResponse *tds__GetCapabilitiesResponse;  //ͨ������
	bool         m_bGetCapabilitiesSuccess;     //�ɹ���ȡ����
	bool         m_blSupportPTZ;

	string       m_proxyDeviceEntry;
	string       m_proxyMediaEntry;
	string       m_proxyImagingEntry;
	string       m_proxyPTZEntry;
	string       m_proxyEventEntry;
	string       m_proxyNPEntry;

	//Onvif��������
	struct soap     *soap;
// 	DeviceBinding   proxyDevice;
// 	MediaBinding    proxyMedia;
// 	ImagingBinding  proxyImaging;
// 	PTZBinding      proxyPTZ;
	//EventBinding    proxyEvent;
	//NotificationProducerBinding proxyNP;

	ONVIF_OPTIONS   m_stOptions;
	bool            m_bGetOptionsSuccess;     //�ɹ���ȡ����
	bool            m_bGetMoveOptionsSuccess;     //�ɹ���ȡ����
	ONVIF_MOVE_OPTIONS  m_stMoveOptions;
	bool            m_bGetPtzMoveOptionsSuccess;     //�ɹ���ȡ����
	ONVIF_PTZMOVE_OPTIONS  m_stPtzMoveOptions;
	string          m_strPtzMoveOptionsNodeToken;      //GetPtzConfigurationOptions
	bool            m_bGetEventPropertiesSuccess;
	string          m_strFilterDialect;
	string          m_strTopicExpressionDialect;
	CMutexLock        g_OnvifOpLock;
};

#endif /* ONVIFOP_H_ */
