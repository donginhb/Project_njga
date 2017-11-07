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
	//可根据配置文件读取
	g_mapDllName[DEVICE_TYPE_HIK]=".\\Netsdk\\HIK\\ComNetSdkHik.dll";   //Hik
	g_mapDllName[DEVICE_TYPE_DAH]=".\\Netsdk\\DAH\\ComNetSdkDah.dll";   //dah
	g_mapDllName[DEVICE_TYPE_EV8K]=".\\Netsdk\\EV8K\\ComNetSdkEv8k.dll";//EV8K
	g_mapDllName[DEVICE_TYPE_LB]=".\\Netsdk\\LB\\ComNetSdkLB.dll";//EV8K
	g_mapDllName[DEVICE_TYPE_BOSCH]=".\\Netsdk\\BOSCH\\ComNetSdkBosch.dll";//BOSCH
#endif
	return 0;
}

//释放配置
int CDevCtrl::Fini()
{
	return 0;
}

int CDevCtrl::Start()
{
	//初始化设备
	Init();
	OpenAllDev();
	//chenyu  暂时不获取通道名称
	//UpdateAllUnitName();                   //更新通道名称
	CSysCfg::Instance()->QueryDevChCfg();   //更新设备信息
	CSysCfg::m_bDevInited = TRUE;          //初始化完成
	return 0;
}

int CDevCtrl::Close()
{
	//关闭设备
    CloseAllDev();
	return 0;
}

//根据DeviceID查找Device对象
int CDevCtrl::FindDevice(string strDeviceID,CMediaItemDevice* &lpDevice)
{
	UNGB_CHCONFIGREC stChCfg;
	if(!CSysCfg::Instance()->GetOneChCfg(strDeviceID,stChCfg))
	{
		mgwlog("CDevCtrl::FindDevice 没有找到指定通道配置1：%s\n",strDeviceID.c_str());
		return -1;  //没有找到指定设备
	}
	int iDeviceIndex = stChCfg.iDeviceIndex;
	map<int,CMediaItemDevice*>::iterator iterDev = m_mapDevItem.find(iDeviceIndex);
	if((iterDev == m_mapDevItem.end()) || (!(iterDev->second)))
	{
		if((iterDev != m_mapDevItem.end()) && !(iterDev->second))
		{
			mgwlog("CDevCtrl::FindDevice 设备对象指针为空\n");
		}
		mgwlog("CDevCtrl::FindDevice 没有找到指定设备对象2：%s\n",strDeviceID.c_str());
		return -1;  //没有找到指定设备
	}
    lpDevice= iterDev->second;
	return 0;
}

int CDevCtrl::OpenReal(STREAM_CON_INFO stStreamConInfo,UNITOPENTYPE eOpenType)
{
	//查找设备
	CMediaItemDevice *lpDevice = NULL;
	if(0 == FindDevice(stStreamConInfo.strLogicDeviceID,lpDevice))
	{
		if(lpDevice)
		{
			return lpDevice->OpenReal(stStreamConInfo,eOpenType);
		}else{
			mgwlog("CDevCtrl::OpenReal没有找到对应设备2\n");
		}
	}else{
		mgwlog("CDevCtrl::OpenReal没有找到对应设备\n");
		return -1;
	}
	return 0;
}

int CDevCtrl::Close(CBSIPMSG stCbSipmsg,BOOL bReal)       //close通道
{
	CMediaItemDevice* lpDevice = NULL;
	if(0 !=FindDevice(stCbSipmsg.strcallee_id,lpDevice))
	{
		mgwlog("CDevCtrl::Close 视频 fail1\n");
		return -1;
	}
	if(lpDevice)
	{
		mgwlog("CDevCtrl::Close 视频 ID:%s\n",stCbSipmsg.strcallee_id.c_str());
		return lpDevice->Close(stCbSipmsg.idialog_index,bReal);
	}else{
		mgwlog("CDevCtrl::Close 视频 fail2\n");
		return -1;	
	}
	return -1;
}

//获取通道信息
int CDevCtrl::GetUnitState(string strLogicDeviceID,CHANNELITEM_STATE &eChannelState)
{
	eChannelState = CHANNELITEM_STATE_OPEN; //tmp for test chenyu
	CMediaItemDevice *lpDevice = NULL;
	if( 0 == FindDevice(strLogicDeviceID,lpDevice))  //找到
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
	//创建PlayBack对象
	CMediaItemDevice *lpDevice = NULL;
	if( 0 == FindDevice(stStreamConInfo.strLogicDeviceID,lpDevice))  //找到
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
	//创建PlayBack对象
	CMediaItemDevice *lpDevice = NULL;
	if( 0 == FindDevice(stCbSipmsg.strcallee_id,lpDevice))  //找到
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
	if( 0 == FindDevice(stCbSipmsg.strcallee_id,lpDevice))  //找到
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
	//xml ptzcmd 转为 wiscom标准命令
    BYTE szOutBuf[PTZCMD_WIS_LEN]={0};
	DWORD dwBufSize=PTZCMD_WIS_LEN;
    int nRet = PTZCmdParser((char *)strPTZCmd.c_str(),&(szOutBuf[0]),dwBufSize);
    if(nRet != 0)
	{
		mgwlog("PTZCmdParser() 解析失败\n");
		return -1;
	}
	CMediaItemDevice *lpDevice = NULL;
	if( 0 == FindDevice(strDeviceID,lpDevice))  //找到
	{
		if(lpDevice)
		{
			lpDevice->PTZCtrl(strDeviceID,(char *)&(szOutBuf[0]),dwBufSize);
			return 0;
		}
	}
	return -1;
}

//处理点击放大
int CDevCtrl::PTZCtrl(string strDeviceID,AutoZoomInPara &stAutoZoomInPara)
{
 	mgwlog("CDevCtrl::PTZCtrl 点击放大命令 DeviceID:%s %s %s %s %s %s %s\n",strDeviceID.c_str(),
		stAutoZoomInPara.strDisplayFrameWide.c_str(),
	    stAutoZoomInPara.strDisplayFrameHigh.c_str(),
	    stAutoZoomInPara.strDestMatrixTopLeftX.c_str(),
	    stAutoZoomInPara.strDestMatrixTopLeftY.c_str(),
	    stAutoZoomInPara.strDestMatrixWide.c_str(),
		stAutoZoomInPara.strDestMatrixHigh.c_str());
 	//xml ptzcmd 转为 wiscom标准命令
    BYTE szOutBuf[PTZCMD_WIS_AUTOZOOMIN_LEN]={0};
	DWORD dwBufSize=PTZCMD_WIS_AUTOZOOMIN_LEN;
    int nRet = AutoZoomInCmdParser(stAutoZoomInPara,&(szOutBuf[0]),dwBufSize);
    if(nRet != 0)
	{
		mgwlog("PTZCmdParser() 解析失败\n");
		return -1;
	}
	CMediaItemDevice *lpDevice = NULL;
	if( 0 == FindDevice(strDeviceID,lpDevice))  //找到
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
	if( 0 == FindDevice(strDeviceID,lpDevice))  //找到
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
	if( 0 == FindDevice(strDeviceID,lpDevice))  //找到
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
	if( 0 == FindDevice(strDeviceID,lpUnit))  //找到
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
//查询录像记录
int CDevCtrl::GetRecordInfo(string strDeviceID,
							SYSTEMTIME sStartTime,SYSTEMTIME sStopTime,
								EDVDVRRECORDTABLE* lpTable,int nTableCount,int& iTotalCount)
{
	//查找设备
	CMediaItemDevice *lpDevice = NULL;
	if( 0 == FindDevice(strDeviceID,lpDevice))  //找到
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

//输入28181Ptzcmd 转换为 wis Ptzcmd
int CDevCtrl::PTZCmdParser(string str28181PTZCmd,BYTE* pszOutWisCmd,DWORD dwOutBufSize)
{
	//字符串转换为8字节长的数据
    unsigned char szPtzcmd[PTZCMD_28181_LEN]={0};
	int iLen = String2Bytes((unsigned char* )str28181PTZCmd.c_str(),(unsigned char *)&szPtzcmd[0],PTZCMD_28181_LEN);
    if(iLen == -1)
	{
		mgwlog("PTZCmd字符串转换fail\n");
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
			mgwlog("ptzcmd 校验失败\n");
			return -1;
		}
		//控制类型 水平速度 垂直速度 变倍控制速度相对值 地址
		unsigned short usCtrlType=0,usHSpeed=0,usVSpeed=0,usCSpeed=0,usAddress=0;
		usCtrlType = b4;
		usHSpeed = b5;
		usVSpeed = b6;
		usCSpeed = b7>>4;
		usAddress = ((b7&0x0f)<<4)|b3;

		BYTE iSpeed = (usHSpeed>usVSpeed ? usHSpeed:usVSpeed);
		iSpeed = iSpeed/(255/63);  //28181速度转换为wis标准 待修改 chenyu
		BYTE* lpBuffer = pszOutWisCmd;
		*lpBuffer = 0xFF;
		*(lpBuffer+1) = (BYTE)usAddress;
		mgwlog("----CtrlType:%d--Speed:%d--\n",usCtrlType,iSpeed);
		//gb28181<---->wiscomsdk 控球命令映射关系
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
				mgwlog("----PTZCmdParser()解析错误，无对应28181指令----\n");
				return -1;
			}
			break;
		}
		*(lpBuffer+6) = (*(lpBuffer+1)+*(lpBuffer+2)+*(lpBuffer+3)+*(lpBuffer+4)+*(lpBuffer+5))%0x100;
	}

	return 0;
}

//输入28181点击放大指令 转换为 wis点击放大指令
int CDevCtrl::AutoZoomInCmdParser(AutoZoomInPara &stAutoZoomInPara,BYTE* pszOutWisCmd,DWORD dwOutBufSize)
{
	if(dwOutBufSize != PTZCMD_WIS_AUTOZOOMIN_LEN)
	{
		mgwlog("AutoZoomInCmdParser 输出缓冲大小失败\n");
		return -1;
	}

	BYTE* lpBuffer = pszOutWisCmd;
	memset(lpBuffer,0,PTZCMD_WIS_AUTOZOOMIN_LEN);
	//字符串转换为28字节长的数据
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

//打开文件线程
int CDevCtrl::OnThOpenDev()
{
	//创建设备
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
	mgwlog("退出打开设备线程\n");
	return 0;
}

//打开设备  修改为多线程
int CDevCtrl::OpenAllDev()
{
	DWORD now = GetTickCount();
	//创建设备
#ifdef MTOPENFILE    //多线程
	CSysCfg::Instance()->GetDevCfg(m_mapDevCfg);
	m_iterDevCfg = m_mapDevCfg.begin();
	//int n = m_mapDevCfg.size();
	//增加线程出来打开文件大小
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
	Sleep(3*1000);  //等待线程处理结束
#else
	sleep(3);
#endif
    //等待线程处理结束
	for (int iCount=0; iCount<THREAD_OPEN_MAX; iCount++)
	{
#ifdef WIN32
		if(m_hThreadOpenDev[iCount])
		{
			WaitForSingleObject(m_hThreadOpenDev[iCount], INFINITE);
			CLOSE_HANDLE(m_hThreadOpenDev[iCount]);
			m_dwThreadOpenDevID[iCount] = 0;
			m_bThreadOpenDevFlag[iCount] = FALSE;
			mgwlog("关闭打开设备线程,ID:[%d]\n",iCount);
		}
#else
		if(m_threadOpenDevHandle[iCount])
		{
			m_bThreadOpenDevFlag[iCount] = FALSE;
			pthread_cancel(m_threadOpenDevHandle[iCount]);
			pthread_join(m_threadOpenDevHandle[iCount], NULL);
			m_threadOpenDevHandle[iCount] = 0;
			mgwlog("关闭打开设备线程,ID:[%d]\n",iCount);
		}
#endif
	}
#else       //单线程
	map<int,UNGB_DEVCONFIGREC> mapDevConfig;
	CSysCfg::Instance()->GetDevCfg(mapDevConfig);
	map<int,UNGB_DEVCONFIGREC>::iterator iter = mapDevConfig.begin();
	while (iter != mapDevConfig.end())
	{
		OpenDev(iter->second);
		iter++;
	}
#endif

	mgwlog("打开设备耗时：%d 秒\n",(GetTickCount()-now)/1000);

	//创建通道
	CMediaItemDevice* lpDevice=NULL;
	map<string,UNGB_CHCONFIGREC>  mapChConfig;
	CSysCfg::Instance()->GetChCfg(mapChConfig);
	map<string,UNGB_CHCONFIGREC>::iterator iterch = mapChConfig.begin();
	while (iterch != mapChConfig.end())
	{
		map<int,CMediaItemDevice*>::iterator iterDev = m_mapDevItem.find((iterch->second).iDeviceIndex);
		if((iterDev == m_mapDevItem.end()) || (!(iterDev->second)))
		{
			mgwlog("没有找到对应索引(DeviceIndex)的设备,iDeviceIndex:%d 不创建通道节点\n",(iterch->second).iDeviceIndex);
			iterch++;
			continue;
		}
 		lpDevice = m_mapDevItem[(iterch->second).iDeviceIndex];
 		if(!lpDevice)
 		{
 			mgwlog("没有找到对应索引(DeviceIndex)的设备,iDeviceIndex:%d\n",(iterch->second).iDeviceIndex);
 			iterch++;
 			continue;
 		}
		//创建视频对象
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
		//根据类型加载DLL
		CSdkProxy *pSdkProxy=NULL;
		int nRet = LoadSDK(stDevInfo.iDeviceType,pSdkProxy);
		if(nRet)   //加载失败
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
//动态加载指定设备的SDK
int CDevCtrl::LoadSDK(int iDevType,PSDKPROXY &pSdkProxy)
{
	//先查找
	int nRet = -1;     //默认失败
	if(m_lstSdkProxy.size() > 0)
	{
		CAutoLock lock(&m_lstSdkProxyLock);
		list<CSdkProxy *>::iterator iter = m_lstSdkProxy.begin();
		while(iter != m_lstSdkProxy.end())  //找指定码流类型对应的解码库
		{
			if((*iter) && (*iter)->m_iDevType == iDevType)
			{
				nRet = 0;   //找到流对应的解码库
				pSdkProxy = *iter;    //返回解码库对象
				TRACE("CDevCtrl::LoadSDK 对应的sdk[%d]已加载\n",iDevType);
				break;
			}
			iter ++;
		}
	}

	if(0 == nRet)
	{
		return 0;
	}

	//没有找到则加载新的解码库
	CString strFilePath ="";
	map<int,CString>::iterator iter=g_mapDllName.find(iDevType);
	if(iter!=g_mapDllName.end())
	{
		strFilePath = iter->second;
	}else{
		return -1;  //没有找到指定解码库
	}

	CSdkProxy *pNewSdkProxy = NULL;  //加载新的dll解码库
	pNewSdkProxy = new CSdkProxy();
	if(pNewSdkProxy != NULL)
	{
		CAutoLock lock(&m_lstSdkProxyLock);
		char szCurWorkDir[1024] = {0};
		GetCurrentDirectory(1024-1,szCurWorkDir);  //得到当前工作路径
		char strModule[1024] = {0};
		GetModuleFileName(NULL,strModule, 1024-1); //得到当前模块路径
		CString strDllFullPath = strModule;
		strDllFullPath +="\\..\\"+strFilePath;
		if(!SetCurrentDirectory(strDllFullPath+"\\.."))  //disk:\xxx\dll.dll去除最后的dll.dll
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
			//SockObj::ConvertStringToIP(&dwIP, (char *)CSysCfg::Instance()->GetstrPara(SYS_CFG_LOCALIP).c_str());   //chIP本机IP
            SockObj::ConvertStringToIP(&dwIP,"127.0.0.1");   //chIP本机IP
			pNewSdkProxy->fEDV_DEVICE_Init(dwIP);      //第一次加载，调用初始化函数
			pNewSdkProxy->m_iDevType = iDevType;
			m_lstSdkProxy.push_back(pNewSdkProxy);
			pSdkProxy = pNewSdkProxy;   //返回解码库对象
			SetCurrentDirectory(szCurWorkDir);  //恢复旧的"当前工作路径"
			TRACE("CDevCtrl::LoadSDK success:%s\n",strFilePath);
			mgwlog("CDevCtrl::LoadSDK success:%s\n",strFilePath);
			return 0;
		}
		else
		{
			delete pNewSdkProxy;
			SetCurrentDirectory(szCurWorkDir);  //恢复旧的"当前工作路径"
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
//root 函数 移入
/////////////////
#ifdef WIN32
int CDevCtrl::AddDev(UNGB_DEVCONFIGREC &stDevConfig,CSdkProxy* pSdkProxy)
{
	CAutoLock lock(&m_mapDevItemLock);   //added by chenyu 131210
	//先查找added by chenyu 140606
	int iDeviceIndex = stDevConfig.iID;
	map<int,CMediaItemDevice*>::iterator iterDev = m_mapDevItem.find(iDeviceIndex);
	if(iterDev != m_mapDevItem.end())
	{
		mgwlog("CMediaItemRoot::AddDev 设备已打开%s\n",stDevConfig.strDeviceName.c_str());
		return 0;
	}
    //创建设备对象
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
	
	//链接到设备树
	//InsertChild(lpDevice);  //旧的链接方式
	//lpDevice->SetParent(this);
	
	//记录Dev的map映射
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
	//先查找added by chenyu 140606
	int iDeviceIndex = stDevConfig.iID;
	map<int,CMediaItemDevice*>::iterator iterDev = m_mapDevItem.find(iDeviceIndex);
	if(iterDev != m_mapDevItem.end())
	{
		mgwlog("CMediaItemRoot::AddDev 设备已打开%s,ID:%d\n",stDevConfig.strDeviceIP.c_str(),iDeviceIndex);
		return 0;
	}
	//创建设备对象
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

	//链接到设备树
	//InsertChild(lpDevice);  //旧的链接方式
	//lpDevice->SetParent(this);

	//记录Dev的map映射
	m_mapDevItem[stDevConfig.iID]=lpDevice;
	//mgwlog("----记录mapDevItem Id:%d \n----",stDevConfig.iID);
	return 0;
}

int CDevCtrl::DelDev(UNGB_DEVCONFIGREC &stDevConfig)
{
	return 0;
}
#endif

//关闭设备
int CDevCtrl::CloseAllDev()
{
	//关闭设备
	map<int,CMediaItemDevice*>::iterator iter = m_mapDevItem.begin(); 
	while (iter != m_mapDevItem.end())
	{
		MEMORY_DELETE(iter->second);
		iter++;
	}
	return 0;
}

//检测设备
int CDevCtrl::CheckAllDev()
{
	CAutoLock lock(&m_mapDevItemLock);
	UNGB_DEVCONFIGREC stDevInfo;
	//设备树和数据库比较上是否有已删除的设备或者删除的通道等
	map<int,CMediaItemDevice*>::iterator iterDev = m_mapDevItem.begin();
	while (iterDev != m_mapDevItem.end())
	{
		//获取对应设备信息
		if(!CSysCfg::Instance()->GetOneDevCfg(iterDev->first,stDevInfo))  //没找到
		{
			mgwlog("设备不存在，删除该设备节点，id:%d\n",iterDev->first);
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
				iterDev->second->SetDevCfg(stDevInfo);  //更新通道信息
				iterDev->second->CheckAllCh();   //检测下属通道情况
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