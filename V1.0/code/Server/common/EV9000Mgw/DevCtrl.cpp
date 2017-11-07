// DevCtrl.cpp: implementation of the CDevCtrl class.
//
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include "stdafx.h"
#include "EV9000Mgw.h"
#endif
#include "DevCtrl.h"
#include "ConfigOp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifdef WIN32
extern CEV9000MgwApp theApp;
#endif

CDevCtrl::CDevCtrl()
{
#ifndef WIN32
	pthread_mutex_init(&m_mapDevCfgLock,NULL);
#endif
#ifdef MTOPENFILE
	for (int i=0; i<THREAD_OPEN_MAX; i++)
	{
		m_bThreadOpenDevFlag[i] = FALSE;
#ifdef WIN32
		m_hThreadOpenDev[i] = 0;
		m_dwThreadOpenDevID[i] = 0;
#else
		m_threadOpenDevHandle[i] = 0;
#endif
	}
#endif
#ifndef WIN32
	pthread_mutex_init(&m_mapDevItemLock,NULL);
#endif
}

CDevCtrl::~CDevCtrl()
{
	Close();
}

int CDevCtrl::Init()
{
#ifdef WIN32
	//�ɸ��������ļ���ȡ
	g_mapDllName[DEVICE_TYPE_HIK]=".\\Netsdk\\HIK\\ComNetSdkHik.dll";   //Hik
	g_mapDllName[DEVICE_TYPE_DAH]=".\\Netsdk\\DAH\\ComNetSdkDah.dll";   //dah
	g_mapDllName[DEVICE_TYPE_EV8K]=".\\Netsdk\\EV8K\\ComNetSdkEv8k.dll";//EV8K
	g_mapDllName[DEVICE_TYPE_LB]=".\\Netsdk\\LB\\ComNetSdkLB.dll";//EV8K
	g_mapDllName[DEVICE_TYPE_BOSCH]=".\\Netsdk\\BOSCH\\ComNetSdkBosch.dll";//BOSCH
#endif
	return 0;
}

//�ͷ�����
int CDevCtrl::Fini()
{
	return 0;
}

int CDevCtrl::Start()
{
	//��ʼ���豸
	Init();
	OpenAllDev();
	//chenyu  ��ʱ����ȡͨ������
	//UpdateAllUnitName();                   //����ͨ������
	CSysCfg::Instance()->QueryDevChCfg();   //�����豸��Ϣ
	CSysCfg::m_bDevInited = TRUE;          //��ʼ�����
	return 0;
}

int CDevCtrl::Close()
{
	//�ر��豸
    CloseAllDev();
	return 0;
}

//����DeviceID����Device����
int CDevCtrl::FindDevice(string strDeviceID,CMediaItemDevice* &lpDevice)
{
	UNGB_CHCONFIGREC stChCfg;
	if(!CSysCfg::Instance()->GetOneChCfg(strDeviceID,stChCfg))
	{
		mgwlog("CDevCtrl::FindDevice û���ҵ�ָ��ͨ������1��%s\n",strDeviceID.c_str());
		return -1;  //û���ҵ�ָ���豸
	}
	int iDeviceIndex = stChCfg.iDeviceIndex;
	map<int,CMediaItemDevice*>::iterator iterDev = m_mapDevItem.find(iDeviceIndex);
	if((iterDev == m_mapDevItem.end()) || (!(iterDev->second)))
	{
		if((iterDev != m_mapDevItem.end()) && !(iterDev->second))
		{
			mgwlog("CDevCtrl::FindDevice �豸����ָ��Ϊ��\n");
		}
		mgwlog("CDevCtrl::FindDevice û���ҵ�ָ���豸����2��%s\n",strDeviceID.c_str());
		return -1;  //û���ҵ�ָ���豸
	}
    lpDevice= iterDev->second;
	return 0;
}

int CDevCtrl::OpenReal(STREAM_CON_INFO stStreamConInfo,UNITOPENTYPE eOpenType)
{
	//�����豸
	CMediaItemDevice *lpDevice = NULL;
	if(0 == FindDevice(stStreamConInfo.strLogicDeviceID,lpDevice))
	{
		if(lpDevice)
		{
			return lpDevice->OpenReal(stStreamConInfo,eOpenType);
		}else{
			mgwlog("CDevCtrl::OpenRealû���ҵ���Ӧ�豸2\n");
		}
	}else{
		mgwlog("CDevCtrl::OpenRealû���ҵ���Ӧ�豸\n");
		return -1;
	}
	return 0;
}

int CDevCtrl::Close(CBSIPMSG stCbSipmsg,BOOL bReal)       //closeͨ��
{
	CMediaItemDevice* lpDevice = NULL;
	if(0 !=FindDevice(stCbSipmsg.strcallee_id,lpDevice))
	{
		mgwlog("CDevCtrl::Close ��Ƶ fail1\n");
		return -1;
	}
	if(lpDevice)
	{
		mgwlog("CDevCtrl::Close ��Ƶ ID:%s\n",stCbSipmsg.strcallee_id.c_str());
		return lpDevice->Close(stCbSipmsg.idialog_index,bReal);
	}else{
		mgwlog("CDevCtrl::Close ��Ƶ fail2\n");
		return -1;	
	}
	return -1;
}

//��ȡͨ����Ϣ
int CDevCtrl::GetUnitState(string strLogicDeviceID,CHANNELITEM_STATE &eChannelState)
{
	eChannelState = CHANNELITEM_STATE_OPEN; //tmp for test chenyu
	CMediaItemDevice *lpDevice = NULL;
	if( 0 == FindDevice(strLogicDeviceID,lpDevice))  //�ҵ�
	{
		if(lpDevice)
		{
			int nRet =lpDevice->GetUnitState(strLogicDeviceID,eChannelState);
			if(0 == nRet)
			{
				//mgwlog("CDevCtrl::GetVideoParam success\n");
			}else{
				mgwlog("CDevCtrl::GetUnitState fail\n");
			}
			return nRet;
		}
	}
	return -1;
}

#ifdef SUP_PLAYBACK
int CDevCtrl::PlayBackOpen(STREAM_CON_INFO stStreamConInfo,UNITOPENTYPE eOpenType) 
{
	mgwlog("enter CDevCtrl::PlayBackOpen\n");
	//����PlayBack����
	CMediaItemDevice *lpDevice = NULL;
	if( 0 == FindDevice(stStreamConInfo.strLogicDeviceID,lpDevice))  //�ҵ�
	{
		if(lpDevice)
		{
            return lpDevice->OpenPlayBack(stStreamConInfo,eOpenType);
		}
	}
	return -1;	
}
 
int CDevCtrl::PlayBackStop(CBSIPMSG stCbSipmsg)
{
	mgwlog("enter CDevCtrl::PlayBackStop\n");
	//����PlayBack����
	CMediaItemDevice *lpDevice = NULL;
	if( 0 == FindDevice(stCbSipmsg.strcallee_id,lpDevice))  //�ҵ�
	{
		if(lpDevice)
		{
            return lpDevice->StopPlayBack(stCbSipmsg.idialog_index);
		}
	}
	return -1;	
} 

int CDevCtrl::PlayBackCtrl(CBSIPMSG stCbSipmsg,RECORD_CTRL eRecordCtrl,int nCtrlData,int* nReturnData)
{
	//mgwlog("enter CDevCtrl::PlayBackCtrl\n");
	CMediaItemDevice *lpDevice = NULL;
	if( 0 == FindDevice(stCbSipmsg.strcallee_id,lpDevice))  //�ҵ�
	{
		if(lpDevice)
		{
			int nReturnData =0;
            return lpDevice->PlayBackCtrl(stCbSipmsg.idialog_index,eRecordCtrl,nCtrlData,&nReturnData);
		}
	}
	return -1;
}

#endif

int CDevCtrl::PTZCtrl(string strDeviceID,string strPTZCmd)
{
	mgwlog("CDevCtrl::PTZCtrl DeviceID:%s PTZCmd:%s\n",strDeviceID.c_str(),strPTZCmd.c_str());
	//xml ptzcmd תΪ wiscom��׼����
    BYTE szOutBuf[PTZCMD_WIS_LEN]={0};
	DWORD dwBufSize=PTZCMD_WIS_LEN;
    int nRet = PTZCmdParser((char *)strPTZCmd.c_str(),&(szOutBuf[0]),dwBufSize);
    if(nRet != 0)
	{
		mgwlog("PTZCmdParser() ����ʧ��\n");
		return -1;
	}
	CMediaItemDevice *lpDevice = NULL;
	if( 0 == FindDevice(strDeviceID,lpDevice))  //�ҵ�
	{
		if(lpDevice)
		{
			lpDevice->PTZCtrl(strDeviceID,(char *)&(szOutBuf[0]),dwBufSize);
			return 0;
		}
	}
	return -1;
}

//�������Ŵ�
int CDevCtrl::PTZCtrl(string strDeviceID,AutoZoomInPara &stAutoZoomInPara)
{
 	mgwlog("CDevCtrl::PTZCtrl ����Ŵ����� DeviceID:%s %s %s %s %s %s %s\n",strDeviceID.c_str(),
		stAutoZoomInPara.strDisplayFrameWide.c_str(),
	    stAutoZoomInPara.strDisplayFrameHigh.c_str(),
	    stAutoZoomInPara.strDestMatrixTopLeftX.c_str(),
	    stAutoZoomInPara.strDestMatrixTopLeftY.c_str(),
	    stAutoZoomInPara.strDestMatrixWide.c_str(),
		stAutoZoomInPara.strDestMatrixHigh.c_str());
 	//xml ptzcmd תΪ wiscom��׼����
    BYTE szOutBuf[PTZCMD_WIS_AUTOZOOMIN_LEN]={0};
	DWORD dwBufSize=PTZCMD_WIS_AUTOZOOMIN_LEN;
    int nRet = AutoZoomInCmdParser(stAutoZoomInPara,&(szOutBuf[0]),dwBufSize);
    if(nRet != 0)
	{
		mgwlog("PTZCmdParser() ����ʧ��\n");
		return -1;
	}
	CMediaItemDevice *lpDevice = NULL;
	if( 0 == FindDevice(strDeviceID,lpDevice))  //�ҵ�
	{
		if(lpDevice)
		{
			lpDevice->PTZCtrl(strDeviceID,(char *)&(szOutBuf[0]),dwBufSize);
			return 0;
		}
	}
	return -1;
}

int CDevCtrl::SetVideoParam(string strDeviceID,string strType, string strValue)
{
	//mgwlog("CDevCtrl::SetVideoParam DeviceID:%s\n",strDeviceID.c_str());
	BYTE byType=0;
	BYTE byParam=0;
	byParam = atoi(strValue.c_str());
	if("ColourDegree"==strType)
	{
		byType =0;
	}
	else if("Brightness"==strType)
	{
        byType =1;
	}
	else if("Contrast"==strType)
	{
		byType =2;
	}
	else if("Saturation"==strType)
	{
		byType =3;
	}

	CMediaItemDevice *lpDevice = NULL;
	if( 0 == FindDevice(strDeviceID,lpDevice))  //�ҵ�
	{
		if(lpDevice)
		{
			int nRet = lpDevice->SetVideoParam(strDeviceID,byType,byParam);
			if(0 == nRet)
			{
				//mgwlog("CDevCtrl::SetVideoParam success\n");
			}else{
				mgwlog("CDevCtrl::SetVideoParam fail\n");
			}
			return nRet;
		}
	}
	return -1;
}

int CDevCtrl::GetVideoParam(string strDeviceID,unsigned int& nParam)
{
	//mgwlog("CDevCtrl::GetVideoParam DeviceID:%s\n",strDeviceID.c_str());
	CMediaItemDevice *lpDevice = NULL;
	if( 0 == FindDevice(strDeviceID,lpDevice))  //�ҵ�
	{
		if(lpDevice)
		{
			int nRet =lpDevice->GetVideoParam(strDeviceID,nParam);
			if(0 == nRet)
			{
				//mgwlog("CDevCtrl::GetVideoParam success\n");
			}else{
				mgwlog("CDevCtrl::GetVideoParam fail\n");
			}
			return nRet;
		}
	}
	return -1;
}

int CDevCtrl::MakeKeyFrame(string strDeviceID)
{
	mgwlog("CDevCtrl::MakeKeyFrame DeviceID:%s\n",strDeviceID.c_str());
	CMediaItemDevice *lpUnit = NULL;
	if( 0 == FindDevice(strDeviceID,lpUnit))  //�ҵ�
	{
		if(lpUnit)
		{
			lpUnit->MakeKeyFrame(strDeviceID);
			return 0;
		}
	}
	return -1;
}

#ifdef WIN32
//��ѯ¼���¼
int CDevCtrl::GetRecordInfo(string strDeviceID,
							SYSTEMTIME sStartTime,SYSTEMTIME sStopTime,
								EDVDVRRECORDTABLE* lpTable,int nTableCount,int& iTotalCount)
{
	//�����豸
	CMediaItemDevice *lpDevice = NULL;
	if( 0 == FindDevice(strDeviceID,lpDevice))  //�ҵ�
	{
		if(lpDevice)
		{
			int nRet = lpDevice->GetRecordInfo(strDeviceID,sStartTime,sStopTime,lpTable,nTableCount,iTotalCount);
			if(0 == nRet)
			{
				//mgwlog("CDevCtrl::SetVideoParam success\n");
			}else{
				mgwlog("CDevCtrl::GetRecordInfo fail\n");
			}
			return nRet;
		}
	}
	return -1;
}
#endif

int CDevCtrl::String2Bytes(unsigned char* szSrc, unsigned char* pDst, int nDstMaxLen)
{
	int iLen;
	int i;
	if(szSrc == NULL)
	{
		return -1;
	}
	iLen = strlen((char *)szSrc);
	if (iLen <= 0 || iLen%2 != 0 || pDst == NULL || nDstMaxLen < iLen/2)
	{
		return -1;
	}
	iLen /= 2;
	strupr((char *)szSrc);
	for (i=0; i<iLen; i++)
	{
		int iVal = 0;
		unsigned char *pSrcTemp = szSrc + i*2;
		sscanf((char *)pSrcTemp, "%02x", &iVal);
		pDst[i] = (unsigned char)iVal;
	}
	return iLen;
}

//����28181Ptzcmd ת��Ϊ wis Ptzcmd
int CDevCtrl::PTZCmdParser(string str28181PTZCmd,BYTE* pszOutWisCmd,DWORD dwOutBufSize)
{
	//�ַ���ת��Ϊ8�ֽڳ�������
    unsigned char szPtzcmd[PTZCMD_28181_LEN]={0};
	int iLen = String2Bytes((unsigned char* )str28181PTZCmd.c_str(),(unsigned char *)&szPtzcmd[0],PTZCMD_28181_LEN);
    if(iLen == -1)
	{
		mgwlog("PTZCmd�ַ���ת��fail\n");
		return -1;
	}

	if (iLen == 8)
	{
		BOOL bSucceed = FALSE;
		BYTE b1 = *szPtzcmd;
		BYTE b2 = *(szPtzcmd+1);
		BYTE b3 = *(szPtzcmd+2);
		BYTE b4 = *(szPtzcmd+3);
		BYTE b5 = *(szPtzcmd+4);
		BYTE b6 = *(szPtzcmd+5);
		BYTE b7 = *(szPtzcmd+6);
        BYTE b8 = *(szPtzcmd+7);
		if (b1!=0xA5||b8 !=((b1+b2+b3+b4+b5+b6+b7)%256))
		{
			mgwlog("ptzcmd У��ʧ��\n");
			return -1;
		}
		//�������� ˮƽ�ٶ� ��ֱ�ٶ� �䱶�����ٶ����ֵ ��ַ
		unsigned short usCtrlType=0,usHSpeed=0,usVSpeed=0,usCSpeed=0,usAddress=0;
		usCtrlType = b4;
		usHSpeed = b5;
		usVSpeed = b6;
		usCSpeed = b7>>4;
		usAddress = ((b7&0x0f)<<4)|b3;

		BYTE iSpeed = (usHSpeed>usVSpeed ? usHSpeed:usVSpeed);
		iSpeed = iSpeed/(255/63);  //28181�ٶ�ת��Ϊwis��׼ ���޸� chenyu
		BYTE* lpBuffer = pszOutWisCmd;
		*lpBuffer = 0xFF;
		*(lpBuffer+1) = (BYTE)usAddress;
		mgwlog("----CtrlType:%d--Speed:%d--\n",usCtrlType,iSpeed);
		//gb28181<---->wiscomsdk ��������ӳ���ϵ
		switch (usCtrlType)
		{
		case EV9K_UP:
			{
				*(lpBuffer+2) = 0x00;
				*(lpBuffer+3) = 0x08;
				*(lpBuffer+4) = 0x00;
				*(lpBuffer+5) = (BYTE)iSpeed;
			}
			break;
		case EV9K_DOWN:
			{
				*(lpBuffer+2) = 0x00;
				*(lpBuffer+3) = 0x10;
				*(lpBuffer+4) = 0x00;
				*(lpBuffer+5) = (BYTE)iSpeed;
			}
			break;
		case EV9K_LEFT:
			{
				*(lpBuffer+2) = 0x00;
				*(lpBuffer+3) = 0x04;
				*(lpBuffer+4) = (BYTE)iSpeed;
				*(lpBuffer+5) = 0x00;
			}
			break;
		case EV9K_RIGHT:
			{
				*(lpBuffer+2) = 0x00;
				*(lpBuffer+3) = 0x02;
				*(lpBuffer+4) = (BYTE)iSpeed;
				*(lpBuffer+5) = 0x00;
			}
			break;
		case EV9K_UP_LEFT:
			{
				*(lpBuffer+2) = 0x00;
				*(lpBuffer+3) = 0x0C;
				*(lpBuffer+4) = (BYTE)iSpeed;
				*(lpBuffer+5) = (BYTE)iSpeed;
			}
			break;
		case EV9K_UP_RIGHT:
			{
				*(lpBuffer+2) = 0x00;
				*(lpBuffer+3) = 0x0A;
				*(lpBuffer+4) = (BYTE)iSpeed;
				*(lpBuffer+5) = (BYTE)iSpeed;
			}
			break;
		case EV9K_DOWN_LEFT:
			{
				*(lpBuffer+2) = 0x00;
				*(lpBuffer+3) = 0x14;
				*(lpBuffer+4) = (BYTE)iSpeed;
				*(lpBuffer+5) = (BYTE)iSpeed;
			}
			break;
		case EV9K_DOWN_RIGHT:
			{
				*(lpBuffer+2) = 0x00;
				*(lpBuffer+3) = 0x12;
				*(lpBuffer+4) = (BYTE)iSpeed;
				*(lpBuffer+5) = (BYTE)iSpeed;
			}
			break;
		case EV9K_ZOOMIN:
			{
				*(lpBuffer+2) = 0x00;
				*(lpBuffer+3) = 0x20;
				*(lpBuffer+4) = (BYTE)iSpeed;
				*(lpBuffer+5) = (BYTE)iSpeed;
			}
			break;
		case EV9K_ZOOMOUT:
			{
				*(lpBuffer+2) = 0x00;
				*(lpBuffer+3) = 0x40;
				*(lpBuffer+4) = (BYTE)iSpeed;
				*(lpBuffer+5) = (BYTE)iSpeed;
			}
			break;
		case EV9K_IRIS_OPEN:
			{
				*(lpBuffer+2) = 0x02;
				*(lpBuffer+3) = 0x00;
				*(lpBuffer+4) = (BYTE)iSpeed;
				*(lpBuffer+5) = (BYTE)iSpeed;
			}
			break;
		case EV9K_IRIS_CLOSE:
			{
				*(lpBuffer+2) = 0x04;
				*(lpBuffer+3) = 0x00;
				*(lpBuffer+4) = (BYTE)iSpeed;
				*(lpBuffer+5) = (BYTE)iSpeed;
			}
			break;
		case EV9K_FOCUS_NEAR:
			{
				*(lpBuffer+2) = 0x01;
				*(lpBuffer+3) = 0x00;
				*(lpBuffer+4) = (BYTE)iSpeed;
				*(lpBuffer+5) = (BYTE)iSpeed;
			}
			break;
		case EV9K_FOCUS_FAR:
			{
				*(lpBuffer+2) = 0x00;
				*(lpBuffer+3) = 0x80;
				*(lpBuffer+4) = (BYTE)iSpeed;
				*(lpBuffer+5) = (BYTE)iSpeed;
			}
			break;
		case EV9K_SET_PRESET:
			{
				*(lpBuffer+2) = 0x00;
				*(lpBuffer+3) = 0x03;
				*(lpBuffer+4) = 0x00;
				*(lpBuffer+5) = b6;
			}
			break;
		case EV9K_GOTO_PRESET:
			{
				*(lpBuffer+2) = 0x00;
				*(lpBuffer+3) = 0x07;
				*(lpBuffer+4) = 0x00;
				*(lpBuffer+5) = b6;
			}
			break;
		case EV9K_CLE_PRESET:
			{
				*(lpBuffer+2) = 0x00;
				*(lpBuffer+3) = 0x05;
				*(lpBuffer+4) = 0x00;
				*(lpBuffer+5) = b6;
			}
			break;
		case EV9K_STOP:
			{
				*(lpBuffer+2) = 0x00;
				*(lpBuffer+3) = 0x00;
				*(lpBuffer+4) = 0;
				*(lpBuffer+5) = 0;
			}
			break;
		default:
			{
				mgwlog("----PTZCmdParser()���������޶�Ӧ28181ָ��----\n");
				return -1;
			}
			break;
		}
		*(lpBuffer+6) = (*(lpBuffer+1)+*(lpBuffer+2)+*(lpBuffer+3)+*(lpBuffer+4)+*(lpBuffer+5))%0x100;
	}

	return 0;
}

//����28181����Ŵ�ָ�� ת��Ϊ wis����Ŵ�ָ��
int CDevCtrl::AutoZoomInCmdParser(AutoZoomInPara &stAutoZoomInPara,BYTE* pszOutWisCmd,DWORD dwOutBufSize)
{
	if(dwOutBufSize != PTZCMD_WIS_AUTOZOOMIN_LEN)
	{
		mgwlog("AutoZoomInCmdParser ��������Сʧ��\n");
		return -1;
	}

	BYTE* lpBuffer = pszOutWisCmd;
	memset(lpBuffer,0,PTZCMD_WIS_AUTOZOOMIN_LEN);
	//�ַ���ת��Ϊ28�ֽڳ�������
    unsigned int uiPara;
	*lpBuffer = 0xFF;
	uiPara = 0;
	uiPara = atoi(stAutoZoomInPara.strDestMatrixTopLeftX.c_str());
    *(lpBuffer+2) = (uiPara & 0xff00)>>8;
	*(lpBuffer+3) = (uiPara & 0x00ff);
	uiPara = 0;
	uiPara = atoi(stAutoZoomInPara.strDestMatrixTopLeftY.c_str());
	*(lpBuffer+4) = (uiPara & 0xff00)>>8;
	*(lpBuffer+5) = (uiPara & 0x00ff);
	*(lpBuffer+6) = (*(lpBuffer+1)+*(lpBuffer+2)+*(lpBuffer+3)+*(lpBuffer+4)+*(lpBuffer+5))%0x100;
	
	lpBuffer+=7;
	*lpBuffer = 0xFF;
	uiPara = 0;
	uiPara = atoi(stAutoZoomInPara.strDestMatrixWide.c_str());
	*(lpBuffer+2) = (uiPara & 0xff00)>>8;
	*(lpBuffer+3) = (uiPara & 0x00ff);
	uiPara = 0;
	uiPara = atoi(stAutoZoomInPara.strDestMatrixHigh.c_str());
	*(lpBuffer+4) = (uiPara & 0xff00)>>8;
	*(lpBuffer+5) = (uiPara & 0x00ff);
	*(lpBuffer+6) = (*(lpBuffer+1)+*(lpBuffer+2)+*(lpBuffer+3)+*(lpBuffer+4)+*(lpBuffer+5))%0x100;
	
	lpBuffer+=7;
	*lpBuffer = 0xFF;
	uiPara = 0;
	uiPara = atoi(stAutoZoomInPara.strDisplayFrameWide.c_str());
	*(lpBuffer+2) = (uiPara & 0xff00)>>8;
	*(lpBuffer+3) = (uiPara & 0x00ff);
	uiPara = 0;
	uiPara = atoi(stAutoZoomInPara.strDisplayFrameHigh.c_str());
	*(lpBuffer+4) = (uiPara & 0xff00)>>8;
	*(lpBuffer+5) = (uiPara & 0x00ff);
	*(lpBuffer+6) = (*(lpBuffer+1)+*(lpBuffer+2)+*(lpBuffer+3)+*(lpBuffer+4)+*(lpBuffer+5))%0x100;
	
	lpBuffer+=7;
	*lpBuffer = 0xFF;
	*(lpBuffer+6) = (*(lpBuffer+1)+*(lpBuffer+2)+*(lpBuffer+3)+*(lpBuffer+4)+*(lpBuffer+5))%0x100;
	
	return 0;
}

//���ļ��߳�
int CDevCtrl::OnThOpenDev()
{
	//�����豸
	UNGB_DEVCONFIGREC stDevCfg;
	while (TRUE)
	{
		if(TRUE)
		{
			CAutoLock lock(&m_mapDevCfgLock);  //added by chenyu 131102
			if (m_iterDevCfg != m_mapDevCfg.end())
			{
				stDevCfg = m_iterDevCfg->second;
				m_iterDevCfg++;
			}else{
				break;
			}
		}
		OpenDev(stDevCfg);
	}
	mgwlog("�˳����豸�߳�\n");
	return 0;
}

//���豸  �޸�Ϊ���߳�
int CDevCtrl::OpenAllDev()
{
	DWORD now = GetTickCount();
	//�����豸
#ifdef MTOPENFILE    //���߳�
	CSysCfg::Instance()->GetDevCfg(m_mapDevCfg);
	m_iterDevCfg = m_mapDevCfg.begin();
	//int n = m_mapDevCfg.size();
	//�����̳߳������ļ���С
	for (int i=0; i<THREAD_OPEN_MAX; i++)
	{
#ifdef WIN32
		if (!m_bThreadOpenDevFlag[i])
		{
			m_bThreadOpenDevFlag[i] = TRUE;
			m_hThreadOpenDev[i] = CreateThread(NULL, 0, ThreadOpenDev, (LPVOID)this, 0, &m_dwThreadOpenDevID[i]);
		}
#else
		int err=0;
		if (!m_bThreadOpenDevFlag[i])
		{
			m_bThreadOpenDevFlag[i] = TRUE;
			err = pthread_create(&m_threadOpenDevHandle[i],NULL,ThreadOpenDev,(void*)this);
			if(err!=0)
			{
				return FALSE;
			}
		}
#endif
	}

#ifdef WIN32
	Sleep(3*1000);  //�ȴ��̴߳������
#else
	sleep(3);
#endif
    //�ȴ��̴߳������
	for (int iCount=0; iCount<THREAD_OPEN_MAX; iCount++)
	{
#ifdef WIN32
		if(m_hThreadOpenDev[iCount])
		{
			WaitForSingleObject(m_hThreadOpenDev[iCount], INFINITE);
			CLOSE_HANDLE(m_hThreadOpenDev[iCount]);
			m_dwThreadOpenDevID[iCount] = 0;
			m_bThreadOpenDevFlag[iCount] = FALSE;
			mgwlog("�رմ��豸�߳�,ID:[%d]\n",iCount);
		}
#else
		if(m_threadOpenDevHandle[iCount])
		{
			m_bThreadOpenDevFlag[iCount] = FALSE;
			pthread_cancel(m_threadOpenDevHandle[iCount]);
			pthread_join(m_threadOpenDevHandle[iCount], NULL);
			m_threadOpenDevHandle[iCount] = 0;
			mgwlog("�رմ��豸�߳�,ID:[%d]\n",iCount);
		}
#endif
	}
#else       //���߳�
	map<int,UNGB_DEVCONFIGREC> mapDevConfig;
	CSysCfg::Instance()->GetDevCfg(mapDevConfig);
	map<int,UNGB_DEVCONFIGREC>::iterator iter = mapDevConfig.begin();
	while (iter != mapDevConfig.end())
	{
		OpenDev(iter->second);
		iter++;
	}
#endif

	mgwlog("���豸��ʱ��%d ��\n",(GetTickCount()-now)/1000);

	//����ͨ��
	CMediaItemDevice* lpDevice=NULL;
	map<string,UNGB_CHCONFIGREC>  mapChConfig;
	CSysCfg::Instance()->GetChCfg(mapChConfig);
	map<string,UNGB_CHCONFIGREC>::iterator iterch = mapChConfig.begin();
	while (iterch != mapChConfig.end())
	{
		map<int,CMediaItemDevice*>::iterator iterDev = m_mapDevItem.find((iterch->second).iDeviceIndex);
		if((iterDev == m_mapDevItem.end()) || (!(iterDev->second)))
		{
			mgwlog("û���ҵ���Ӧ����(DeviceIndex)���豸,iDeviceIndex:%d ������ͨ���ڵ�\n",(iterch->second).iDeviceIndex);
			iterch++;
			continue;
		}
 		lpDevice = m_mapDevItem[(iterch->second).iDeviceIndex];
 		if(!lpDevice)
 		{
 			mgwlog("û���ҵ���Ӧ����(DeviceIndex)���豸,iDeviceIndex:%d\n",(iterch->second).iDeviceIndex);
 			iterch++;
 			continue;
 		}
		//������Ƶ����
		lpDevice->CreateReal(iterch->second);
		iterch++;
	}

	return 0;
} 

int CDevCtrl::OpenDev(UNGB_DEVCONFIGREC &stDevInfo)
{
#ifdef WIN32
	mgwlog("enter CDevCtrl::OpenDev\n");
	int iRet = -1;
	do
	{
		//�������ͼ���DLL
		CSdkProxy *pSdkProxy=NULL;
		int nRet = LoadSDK(stDevInfo.iDeviceType,pSdkProxy);
		if(nRet)   //����ʧ��
		{
			mgwlog("LoadSDK fail,DeviceType:%d\n",stDevInfo.iDeviceType);
			iRet = -1;
			break;
		}
		mgwlog("LoadSDK success,DeviceType:%d\n",stDevInfo.iDeviceType);

		iRet = AddDev(stDevInfo,pSdkProxy);
	} while (0);

	return iRet;
#else
	mgwlog("enter CDevCtrl::OpenDev\n");
	int iRet = -1;
	iRet = AddDev(stDevInfo);
	return iRet;
#endif
}

#ifdef WIN32
//��̬����ָ���豸��SDK
int CDevCtrl::LoadSDK(int iDevType,PSDKPROXY &pSdkProxy)
{
	//�Ȳ���
	int nRet = -1;     //Ĭ��ʧ��
	if(m_lstSdkProxy.size() > 0)
	{
		CAutoLock lock(&m_lstSdkProxyLock);
		list<CSdkProxy *>::iterator iter = m_lstSdkProxy.begin();
		while(iter != m_lstSdkProxy.end())  //��ָ���������Ͷ�Ӧ�Ľ����
		{
			if((*iter) && (*iter)->m_iDevType == iDevType)
			{
				nRet = 0;   //�ҵ�����Ӧ�Ľ����
				pSdkProxy = *iter;    //���ؽ�������
				TRACE("CDevCtrl::LoadSDK ��Ӧ��sdk[%d]�Ѽ���\n",iDevType);
				break;
			}
			iter ++;
		}
	}

	if(0 == nRet)
	{
		return 0;
	}

	//û���ҵ�������µĽ����
	CString strFilePath ="";
	map<int,CString>::iterator iter=g_mapDllName.find(iDevType);
	if(iter!=g_mapDllName.end())
	{
		strFilePath = iter->second;
	}else{
		return -1;  //û���ҵ�ָ�������
	}

	CSdkProxy *pNewSdkProxy = NULL;  //�����µ�dll�����
	pNewSdkProxy = new CSdkProxy();
	if(pNewSdkProxy != NULL)
	{
		CAutoLock lock(&m_lstSdkProxyLock);
		char szCurWorkDir[1024] = {0};
		GetCurrentDirectory(1024-1,szCurWorkDir);  //�õ���ǰ����·��
		char strModule[1024] = {0};
		GetModuleFileName(NULL,strModule, 1024-1); //�õ���ǰģ��·��
		CString strDllFullPath = strModule;
		strDllFullPath +="\\..\\"+strFilePath;
		if(!SetCurrentDirectory(strDllFullPath+"\\.."))  //disk:\xxx\dll.dllȥ������dll.dll
		{
			TRACE("set CurDir fail: %s\n",strDllFullPath.GetBuffer(0));
			mgwlog("set CurDir fail: %s\n",strDllFullPath.GetBuffer(0));
		}else{
			TRACE("set CurDir success: %s\n",strDllFullPath.GetBuffer(0));
			mgwlog("set CurDir success: %s\n",strDllFullPath.GetBuffer(0));
		}

		if(pNewSdkProxy->LoadDLLPlugin(strDllFullPath))
		{
			//CAutoLock lock(&m_listSdkProxyLock);
			DWORD dwIP=0;
			//SockObj::ConvertStringToIP(&dwIP, (char *)CSysCfg::Instance()->GetstrPara(SYS_CFG_LOCALIP).c_str());   //chIP����IP
            SockObj::ConvertStringToIP(&dwIP,"127.0.0.1");   //chIP����IP
			pNewSdkProxy->fEDV_DEVICE_Init(dwIP);      //��һ�μ��أ����ó�ʼ������
			pNewSdkProxy->m_iDevType = iDevType;
			m_lstSdkProxy.push_back(pNewSdkProxy);
			pSdkProxy = pNewSdkProxy;   //���ؽ�������
			SetCurrentDirectory(szCurWorkDir);  //�ָ��ɵ�"��ǰ����·��"
			TRACE("CDevCtrl::LoadSDK success:%s\n",strFilePath);
			mgwlog("CDevCtrl::LoadSDK success:%s\n",strFilePath);
			return 0;
		}
		else
		{
			delete pNewSdkProxy;
			SetCurrentDirectory(szCurWorkDir);  //�ָ��ɵ�"��ǰ����·��"
			TRACE("CDevCtrl::LoadSDK fail:%s\n",strFilePath);
			mgwlog("CDevCtrl::LoadSDK fail:%s\n",strFilePath);
			return -1;
		}
	}
	else
	{
		return -1;
	}
	return 0;
}
#endif

/////////////////
//root ���� ����
/////////////////
#ifdef WIN32
int CDevCtrl::AddDev(UNGB_DEVCONFIGREC &stDevConfig,CSdkProxy* pSdkProxy)
{
	CAutoLock lock(&m_mapDevItemLock);   //added by chenyu 131210
	//�Ȳ���added by chenyu 140606
	int iDeviceIndex = stDevConfig.iID;
	map<int,CMediaItemDevice*>::iterator iterDev = m_mapDevItem.find(iDeviceIndex);
	if(iterDev != m_mapDevItem.end())
	{
		mgwlog("CMediaItemRoot::AddDev �豸�Ѵ�%s\n",stDevConfig.strDeviceName.c_str());
		return 0;
	}
    //�����豸����
	CMediaItemDevice* lpDevice =NULL;
	lpDevice = new CMediaItemDevice(stDevConfig.iID);
	if(!lpDevice)
	{
		mgwlog("CDevCtrl::AddDev new CMediaItemDevice fail LogicID:%s\n",stDevConfig.iID);
		return -1;
	}
	if(lpDevice->Start(stDevConfig,pSdkProxy)<0)
	{
       mgwlog("CDevCtrl::AddDev fail LogicID:%s\n",stDevConfig.iID);
	   return -1;
	}
	
	//���ӵ��豸��
	//InsertChild(lpDevice);  //�ɵ����ӷ�ʽ
	//lpDevice->SetParent(this);
	
	//��¼Dev��mapӳ��
	m_mapDevItem[stDevConfig.iID]=lpDevice;
	return 0;
}

int CDevCtrl::DelDev(UNGB_DEVCONFIGREC &stDevConfig,CSdkProxy* pSdkProxy)
{
	return 0;
}
#else
int CDevCtrl::AddDev(UNGB_DEVCONFIGREC &stDevConfig)
{
	CAutoLock lock(&m_mapDevItemLock);   //added by chenyu 131210
	//�Ȳ���added by chenyu 140606
	int iDeviceIndex = stDevConfig.iID;
	map<int,CMediaItemDevice*>::iterator iterDev = m_mapDevItem.find(iDeviceIndex);
	if(iterDev != m_mapDevItem.end())
	{
		mgwlog("CMediaItemRoot::AddDev �豸�Ѵ�%s,ID:%d\n",stDevConfig.strDeviceIP.c_str(),iDeviceIndex);
		return 0;
	}
	//�����豸����
	CMediaItemDevice* lpDevice =NULL;
	lpDevice = new CMediaItemDeviceOnvif(stDevConfig.iID);
	if(!lpDevice)
	{
		mgwlog("CDevCtrl::AddDev new CMediaItemDevice fail LogicID:%s\n",stDevConfig.iID);
		return -1;
	}
	if(lpDevice->Start(stDevConfig)<0)
	{
       mgwlog("CDevCtrl::AddDev fail LogicID:%s\n",stDevConfig.iID);
	   return -1;
	}

	//���ӵ��豸��
	//InsertChild(lpDevice);  //�ɵ����ӷ�ʽ
	//lpDevice->SetParent(this);

	//��¼Dev��mapӳ��
	m_mapDevItem[stDevConfig.iID]=lpDevice;
	//mgwlog("----��¼mapDevItem Id:%d \n----",stDevConfig.iID);
	return 0;
}

int CDevCtrl::DelDev(UNGB_DEVCONFIGREC &stDevConfig)
{
	return 0;
}
#endif

//�ر��豸
int CDevCtrl::CloseAllDev()
{
	//�ر��豸
	map<int,CMediaItemDevice*>::iterator iter = m_mapDevItem.begin(); 
	while (iter != m_mapDevItem.end())
	{
		MEMORY_DELETE(iter->second);
		iter++;
	}
	return 0;
}

//����豸
int CDevCtrl::CheckAllDev()
{
	CAutoLock lock(&m_mapDevItemLock);
	UNGB_DEVCONFIGREC stDevInfo;
	//�豸�������ݿ�Ƚ����Ƿ�����ɾ�����豸����ɾ����ͨ����
	map<int,CMediaItemDevice*>::iterator iterDev = m_mapDevItem.begin();
	while (iterDev != m_mapDevItem.end())
	{
		//��ȡ��Ӧ�豸��Ϣ
		if(!CSysCfg::Instance()->GetOneDevCfg(iterDev->first,stDevInfo))  //û�ҵ�
		{
			mgwlog("�豸�����ڣ�ɾ�����豸�ڵ㣬id:%d\n",iterDev->first);
            if(iterDev->second)
			{
				MEMORY_DELETE(iterDev->second);
			}
			//iterDev = 
				m_mapDevItem.erase(iterDev++);
			continue;
		}
		else
		{
			if(iterDev->second)
			{
				iterDev->second->SetDevCfg(stDevInfo);  //����ͨ����Ϣ
				iterDev->second->CheckAllCh();   //�������ͨ�����
			}else{
				//iterDev = 
				m_mapDevItem.erase(iterDev++);
			    continue;
			}
		}
		iterDev++;
	}
	return 0;
}

int CDevCtrl::UpdateAllUnitName()
{
	map<int,CMediaItemDevice*>::iterator iter= m_mapDevItem.begin();
	while(iter!=m_mapDevItem.end())
	{
		if(iter->second)
		{
			iter->second->UpdateUnitName();
		}
		iter++;
	}
	return 0;
}