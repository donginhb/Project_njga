/*
 * OnvifOp.cpp
 *
 *  Created on: 2014-3-10
 *      Author: root
 */

#include "OnvifOp.h"
#include "OnvifAPIAll/DeviceBinding.nsmap"
#include <time.h>

#ifdef WIN32
#pragma comment(lib,"lib/libeay32.lib")
#pragma comment(lib,"lib/ssleay32.lib")
#pragma comment(lib,"../lib/OnvifProbe.lib")
#endif

/*int __stdcall DevInfo(EV9000_OnvifPhyDeviceConfig* pDevInfo,long nUser,long nReserved1)
{
	cout<<pDevInfo->strDeviceID<<endl;
	cout<<pDevInfo->strDeviceIP<<endl;
	cout<<"++++++++++++++++"<<endl;
	return 0;
}	

int test()
{
	//OnvifDiscovery oOnvifDiscovery;
	//oOnvifDiscovery.Discovery(5);
	OnvifOp oOnvifOp;
	oOnvifOp.SetDevCfgCallBack(DevInfo,0);
	oOnvifOp.Discovery(5);
	cout<<"Sleep"<<endl;
	return 0;
}
*/

OnvifOp::OnvifOp()
{
	m_strIP.clear();
	m_strUsr.clear();
	m_strPwd.clear();
	m_nPort = 80;
	memset(szHostName,0,MAX_HOSTNAME_LEN);
	m_strVideoSourceToken = "";
	m_strProfileToken = "";
	m_bGetCapabilitiesSuccess = false;
	m_blSupportPTZ = false;
	//tds__GetCapabilitiesResponse = NULL;
	m_nKeepAliveCount = 0;   //默认保活成功
// #ifdef WIN32
// 	m_pOnvifDiscovery = NULL;
// 	if(!m_pOnvifDiscovery)
// 	{
// 		m_pOnvifDiscovery = new OnvifDiscovery();
// 		if(!m_pOnvifDiscovery)
// 		{
// 			printf("new OnvifDiscovery fail\n");
// 			return ;
// 		}
// 	}
// #endif
	soap = NULL;
	if(!soap)
	{
		soap = soap_new();
	}else{
		printf("soap_new() fail");
	}
	m_proxyDeviceEntry = "";
	m_proxyMediaEntry = "";
	m_proxyImagingEntry = "";
	m_proxyPTZEntry = "";
	m_proxyEventEntry = "";
	m_proxyNPEntry = "";
	m_bGetOptionsSuccess = false;
	m_bGetMoveOptionsSuccess = false;
	m_bGetPtzMoveOptionsSuccess = false;
	m_bGetEventPropertiesSuccess = false;
}

OnvifOp::~OnvifOp()
{
	CInteLock lock(&g_OnvifOpLock);
	Close();
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	//最后销毁
	soap_free(soap);//detach and free runtime context
	soap = NULL;
}

int OnvifOp::Close()
{
// #ifdef WIN32
//     StopDiscovery();
// 	MEMORY_DELETE(m_pOnvifDiscovery);
// #endif
	return 0;
}

void PrintErr(struct soap* _psoap)
{
	return ;  //注销信息输出
	fflush(stdout);
	fprintf(stdout,"error:%d faultstring:%s faultcode:%s faultsubcode:%s faultdetail:%s\r\n",
        _psoap->error,*soap_faultstring(_psoap),*soap_faultcode(_psoap),
		*soap_faultsubcode(_psoap),*soap_faultdetail(_psoap));
	printf("error:%d faultstring:%s faultcode:%s faultsubcode:%s faultdetail:%s\r\n",
        _psoap->error,*soap_faultstring(_psoap),*soap_faultcode(_psoap),
		*soap_faultsubcode(_psoap),*soap_faultdetail(_psoap));
}

int OnvifOp::VerifyPassword(int iTimeout)
{
	CInteLock lock(&g_OnvifOpLock);
	//struct soap *soap = soap_new();
	int nRet =0;
	MediaBinding    proxyMedia;
	do 
	{
		if(GetCapabilities()!=0)
		{
			nRet = -1;
			break;
		}

		InitProxy(NULL,&proxyMedia);
		//getprofiles
		if(Set_soap_wsse_para("Digest",proxyMedia.soap)!=0)
		{
			nRet = -1;
			break;
		}

		_trt__GetProfiles *trt__GetProfiles = soap_new__trt__GetProfiles(soap,-1);
		_trt__GetProfilesResponse *trt__GetProfilesResponse = soap_new__trt__GetProfilesResponse(soap,-1);

		if (SOAP_OK == proxyMedia.__trt__GetProfiles(trt__GetProfiles,trt__GetProfilesResponse))
		{
			_trt__GetStreamUri *trt__GetStreamUri = soap_new__trt__GetStreamUri(soap,-1);
			trt__GetStreamUri->StreamSetup = soap_new_tt__StreamSetup(soap,-1);
			trt__GetStreamUri->StreamSetup->Stream = tt__StreamType__RTP_Unicast;
			trt__GetStreamUri->StreamSetup->Transport = soap_new_tt__Transport(soap,-1);
			trt__GetStreamUri->StreamSetup->Transport->Protocol = tt__TransportProtocol__RTSP;

			_trt__GetStreamUriResponse *trt__GetStreamUriResponse = soap_new__trt__GetStreamUriResponse(soap,-1);
			//fprintf(stdout,"\r\n-------------------MediaProfiles-------------------\r\n");
			printf("VerifyPassword success,channelnum:%d\n",trt__GetProfilesResponse->Profiles.size()); //通道数
		}
		else
		{
			AddLogInfo("error mark line:%d\n",__LINE__);
			PrintErr(proxyMedia.soap);
			//return -1;
			nRet = -1;
			break;
		}
	}while(0);
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

int OnvifOp::GetLogicDevNum(int& iLogicDevNum,int iTimeout)
{
	CInteLock  lock(&g_OnvifOpLock);
	//struct soap *soap = soap_new();
	int nRet =0;
	MediaBinding    proxyMedia;
	do 
	{
		if(GetCapabilities()!=0)
		{
			nRet = -1;
			break;
		}

		InitProxy(NULL,&proxyMedia);
		//getprofiles
		if(Set_soap_wsse_para("Digest",proxyMedia.soap)!=0)
		{
			nRet = -1;
			break;
		}

		_trt__GetProfiles *trt__GetProfiles = soap_new__trt__GetProfiles(soap,-1);
		_trt__GetProfilesResponse *trt__GetProfilesResponse = soap_new__trt__GetProfilesResponse(soap,-1);

		if (SOAP_OK == proxyMedia.__trt__GetProfiles(trt__GetProfiles,trt__GetProfilesResponse))
		{
			_trt__GetStreamUri *trt__GetStreamUri = soap_new__trt__GetStreamUri(soap,-1);
			trt__GetStreamUri->StreamSetup = soap_new_tt__StreamSetup(soap,-1);
			trt__GetStreamUri->StreamSetup->Stream = tt__StreamType__RTP_Unicast;
			trt__GetStreamUri->StreamSetup->Transport = soap_new_tt__Transport(soap,-1);
			trt__GetStreamUri->StreamSetup->Transport->Protocol = tt__TransportProtocol__RTSP;

			_trt__GetStreamUriResponse *trt__GetStreamUriResponse = soap_new__trt__GetStreamUriResponse(soap,-1);
			//fprintf(stdout,"\r\n-------------------MediaProfiles-------------------\r\n");
			iLogicDevNum = trt__GetProfilesResponse->Profiles.size(); //通道数
			iLogicDevNum = 1;
		}
		else
		{
			AddLogInfo("error mark line:%d\n",__LINE__);
			PrintErr(proxyMedia.soap);
			//return -1;
			nRet = -1;
			break;
		}
	}while(0);
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

//int OnvifOp::testmain(int argc, char* argv[])
int OnvifOp::GetLogicDevCfg(EV9000_OnvifLogicDeviceConfig* pLogicCfg,int iInDataLen,int iTimeout)
{
	CInteLock lock(&g_OnvifOpLock);
	//struct soap *soap = soap_new();
    int nRet =0;
	MediaBinding    proxyMedia;
	do 
	{
		if(GetCapabilities()!=0)
		{
			nRet = -1;
			break;
		}

		InitProxy(NULL,&proxyMedia);
		//getprofiles
		if(Set_soap_wsse_para("Digest",proxyMedia.soap)!=0)
		{
			nRet = -1;
			break;
		}

		_trt__GetProfiles *trt__GetProfiles = soap_new__trt__GetProfiles(soap,-1);
		_trt__GetProfilesResponse *trt__GetProfilesResponse = soap_new__trt__GetProfilesResponse(soap,-1);

		if (SOAP_OK == proxyMedia.__trt__GetProfiles(trt__GetProfiles,trt__GetProfilesResponse))
		{
			_trt__GetStreamUri *trt__GetStreamUri = soap_new__trt__GetStreamUri(soap,-1);
			trt__GetStreamUri->StreamSetup = soap_new_tt__StreamSetup(soap,-1);
			trt__GetStreamUri->StreamSetup->Stream = tt__StreamType__RTP_Unicast;
			trt__GetStreamUri->StreamSetup->Transport = soap_new_tt__Transport(soap,-1);
			trt__GetStreamUri->StreamSetup->Transport->Protocol = tt__TransportProtocol__RTSP;

			_trt__GetStreamUriResponse *trt__GetStreamUriResponse = soap_new__trt__GetStreamUriResponse(soap,-1);
			//fprintf(stdout,"\r\n-------------------MediaProfiles-------------------\r\n");
			//if(iInDataLen < trt__GetProfilesResponse->Profiles.size()*sizeof(EV9000_OnvifLogicDeviceConfig))
			if(iInDataLen < 1*sizeof(EV9000_OnvifLogicDeviceConfig))
			{
				//soap_destroy(soap); // remove deserialized class instances (C++ only)
				//soap_end(soap); // clean up and remove deserialized data
				//soap_free(soap);//detach and free runtime context
				printf("申请结果空间不足\n");
				AddLogInfo("error mark line:%d\n",__LINE__);
				//return -1;
				nRet = -1;
				break;
			}
			memset(pLogicCfg,0,iInDataLen);
			EV9000_OnvifLogicDeviceConfig* pTmpLogicCfg =NULL;
			//for (int i = 0; i < trt__GetProfilesResponse->Profiles.size(); i++)
			for (int i = 0; i < 1; i++)
			{
				//fprintf(stdout,"profile%d:%s Token:%s\r\n",i,trt__GetProfilesResponse->Profiles[i]->Name.c_str(),trt__GetProfilesResponse->Profiles[i]->token.c_str());
				trt__GetStreamUri->ProfileToken = trt__GetProfilesResponse->Profiles[i]->token;
				pTmpLogicCfg = pLogicCfg+i;
				pTmpLogicCfg->nDeviceType = EV9000_DEVICETYPE_IPC;
				pTmpLogicCfg->nPhyDeviceChannel = i+1;
				if(Set_soap_wsse_para("Digest",proxyMedia.soap)!=0)
				{
					nRet = -1;
					break;
				}
				if (SOAP_OK == proxyMedia.__trt__GetStreamUri(trt__GetStreamUri,trt__GetStreamUriResponse))
				{
					fprintf(stdout,"RTSP URI:%s\r\n",trt__GetStreamUriResponse->MediaUri->Uri.c_str());
					string strTmp = trt__GetStreamUriResponse->MediaUri->Uri;
					size_t pos, bpos=0;
					bpos =strTmp.find("rtsp://");
					if( bpos != string::npos)
					{
						bpos+=7;
						pos=strTmp.find('/', bpos);
						if (pos != string::npos)
						{
							//提取IP Port
							string strIpPort="",strIp="",strPort="554";
							strIpPort =  strTmp.substr(bpos,pos-bpos);
							printf("strIpPort:%s\n",strIpPort.c_str());
							int poscolon=0;    //冒号位置
                            //先查找@
							int posQ = strIpPort.find('@');
							if(posQ!=string::npos) //有@ 去除
							{
                                strIpPort = strIpPort.substr(posQ+1);
								printf("new strIpPort:%s\n",strIpPort.c_str());
							}

							poscolon = strIpPort.find(':');
							if(poscolon!=string::npos)
							{
                                strIp = strIpPort.substr(0,poscolon);
                                strPort = strIpPort.substr(poscolon+1);
							}else{
                                strIp = strIpPort;
								strPort = "554";
							}
							memcpy(&(pTmpLogicCfg->strIPAddress),strIp.c_str(),strIp.length());
							pTmpLogicCfg->nPort = atoi(strPort.c_str());
							string strPhyDeviceChannelMark = strTmp.substr(pos); 
							//cout<<"strPhyDeviceChannelMark:"<<strPhyDeviceChannelMark<<endl;
							memcpy(&(pTmpLogicCfg->strPhyDeviceChannelMark),strPhyDeviceChannelMark.c_str(),strPhyDeviceChannelMark.length());
						}
					}
				}
				else
				{
					AddLogInfo("error mark line:%d\n",__LINE__);
					PrintErr(proxyMedia.soap);
					//return -1;
					nRet = -1;
					break;
				}
			}
		}
		else
		{
			AddLogInfo("error mark line:%d\n",__LINE__);
			PrintErr(proxyMedia.soap);
			//return -1;
			nRet = -1;
			break;
		}
	} while (0);
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	if(-1 == nRet)
	{
		return nRet;
	}

	do 
	{
		if(Set_soap_wsse_para("all",proxyMedia.soap)!=0)
		{
			nRet = -1;
			break;
		}

		_trt__GetVideoEncoderConfigurations *trt__GetVideoEncoderConfigurations = soap_new__trt__GetVideoEncoderConfigurations(soap,-1);
		_trt__GetVideoEncoderConfigurationsResponse *trt__GetVideoEncoderConfigurationsResponse = soap_new__trt__GetVideoEncoderConfigurationsResponse(soap,-1);

		if(SOAP_OK == proxyMedia.__trt__GetVideoEncoderConfigurations(trt__GetVideoEncoderConfigurations,trt__GetVideoEncoderConfigurationsResponse))
		{
			//fprintf(stdout,"\r\n-------------------VideoEncoderConfigurations-------------------\r\n");
			EV9000_OnvifLogicDeviceConfig* pTmpLogicCfg =NULL;
			//for (int i = 0; i < trt__GetVideoEncoderConfigurationsResponse->Configurations.size();i++)
			for (int i = 0; i < 1;i++)
			{
				if(Set_soap_wsse_para("all",proxyMedia.soap)!=0)
				{
					nRet = -1;
					break;
				}

				_trt__GetVideoEncoderConfiguration *trt__GetVideoEncoderConfiguration = soap_new__trt__GetVideoEncoderConfiguration(soap,-1);
				_trt__GetVideoEncoderConfigurationResponse *trt__GetVideoEncoderConfigurationResponse = soap_new__trt__GetVideoEncoderConfigurationResponse(soap,-1);

				trt__GetVideoEncoderConfiguration->ConfigurationToken = trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->token;

				if(SOAP_OK ==proxyMedia.__trt__GetVideoEncoderConfiguration(trt__GetVideoEncoderConfiguration,trt__GetVideoEncoderConfigurationResponse))
				{

				}
				else
				{
					AddLogInfo("error mark line:%d\n",__LINE__);
					PrintErr(proxyMedia.soap);
					//return -1;
					nRet = -1;
					break;
				}

				fprintf(stdout,"Encoding:%s\r\n",(trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->Encoding == tt__VideoEncoding__JPEG)?"tt__VideoEncoding__JPEG":(trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->Encoding == tt__VideoEncoding__MPEG4)?"tt__VideoEncoding__MPEG4":(trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->Encoding == tt__VideoEncoding__H264)?"tt__VideoEncoding__H264":"Error VideoEncoding");
				fprintf(stdout,"name:%s UseCount:%d token:%s\r\n",trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->Name.c_str(),
					trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->UseCount,trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->token.c_str());
				fprintf(stdout,"Width:%d Height:%d\r\n",trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->Resolution->Width,trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->Resolution->Height);

				pTmpLogicCfg = pLogicCfg+i;
				int nStreamType = trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->Encoding;
				if (nStreamType == tt__VideoEncoding__JPEG)
				{
					pTmpLogicCfg->nStreamType =3;
				}
				else if ( nStreamType == tt__VideoEncoding__MPEG4)
				{
					pTmpLogicCfg->nStreamType =2;
				}
				else if (nStreamType == tt__VideoEncoding__H264)
				{
					pTmpLogicCfg->nStreamType =9;
				}
				//cout<<"pTmpLogicCfg->nStreamType:"<<pTmpLogicCfg->nStreamType<<endl;
				if(Set_soap_wsse_para("Digest",proxyMedia.soap)!=0)
				{
					nRet = -1;
					break;
				}

				_trt__GetVideoEncoderConfigurationOptions *trt__GetVideoEncoderConfigurationOptions = soap_new__trt__GetVideoEncoderConfigurationOptions(soap,-1);
				_trt__GetVideoEncoderConfigurationOptionsResponse *trt__GetVideoEncoderConfigurationOptionsResponse= soap_new__trt__GetVideoEncoderConfigurationOptionsResponse(soap,-1);

				trt__GetVideoEncoderConfigurationOptions->ConfigurationToken = &trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->token;

				if (SOAP_OK == proxyMedia.__trt__GetVideoEncoderConfigurationOptions(trt__GetVideoEncoderConfigurationOptions,trt__GetVideoEncoderConfigurationOptionsResponse))
				{
				}
				else
				{
					AddLogInfo("error mark line:%d\n",__LINE__);
					PrintErr(proxyMedia.soap);
					//return -1;
					nRet = -1;
					break;
				}
			}
		}
		else
		{
			AddLogInfo("error mark line:%d\n",__LINE__);
			PrintErr(proxyMedia.soap);
			//return -1;
			nRet = -1;
			break;
		}
	} while (0);
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

/*
注销设备发现
#ifdef WIN32
int OnvifOp::Discovery(int timeout,char* szLocalIP)
{
    if(!m_pOnvifDiscovery)
	{
 		m_pOnvifDiscovery = new OnvifDiscovery();
		if(!m_pOnvifDiscovery)
 		{
			return -1;
		}
 	}
 	m_pOnvifDiscovery->Discovery(timeout,szLocalIP);

	return 0;
}

int OnvifOp::StopDiscovery()
{
	if(m_pOnvifDiscovery)
	{
		m_pOnvifDiscovery->StopDiscovery();
	}
	//MEMORY_DELETE(m_pOnvifDiscovery);
	return 0;
}

void OnvifOp::SetDevCfgCallBack(LPDEVCFGCALLBACK lpFunCall, LPVOID lpUserData)
{
	if(m_pOnvifDiscovery)
	{
		m_pOnvifDiscovery->SetDevCfgCallBack(lpFunCall,lpUserData);
	}
}
#endif
*/
/*
//注销用ping 实现
//保活接口
//启动保活检测
int OnvifOp::StartCheckAlive()
{
	int err=0;
	if (!m_bThreadCheckAliveFlag)
	{
		m_bThreadCheckAliveFlag = TRUE;
		err = pthread_create(&m_threadCheckAliveHandle,NULL,ThreadCheckAlive,(void*)this);
		if(err!=0)
		{
			return FALSE;
		}
	}
    return 0;
}

//停止保活检测
int OnvifOp::StopCheckAlive()
{
	if(m_threadCheckAliveHandle)
    {
		m_bThreadCheckAliveFlag = FALSE;
		pthread_cancel(m_threadCheckAliveHandle);
		pthread_join(m_threadCheckAliveHandle, NULL);
		m_threadCheckAliveHandle =0;
	}
	return 0;
}

//读取保活状态
int OnvifOp::GetAliveStatus()
{
	if(m_nKeepAliveCount <= KEEPALIVE_FAILD_COUNT)
	{
		//printf("GetStatus: Alive\n");
		return 0;
	}else{
		printf("IP[%s]GetStatus: Not Alive\n",m_strIP.c_str());
		return -1;
	}
}

int OnvifOp::OnCheckAlive()
{
	printf("进入 OnvifOp::OnCheckAlive\n");
	int nRegFailNum=0;
	DWORD dwTime = GetTickCount();
	int iTimeOut = 30;  //单位 s
// 	string strTimeOut = CSysCfg::Instance()->GetstrPara(SYS_CFG_SIPREFRESHTIME);
// 	int iTimeOutDefault = atoi(strTimeOut.c_str());
// 	printf("前端保活时间iTimeOutDefault:%d\n",iTimeOutDefault);
// 	if(iTimeOutDefault < MAX_SIP_TIMEOUT)
// 	{
// 		iTimeOutDefault = MAX_SIP_TIMEOUT; 
// 	}
// 	iTimeOut = iTimeOutDefault;  //单位 s
	while (m_bThreadCheckAliveFlag)
	{
		if((GetTickCount()-dwTime)< iTimeOut*1000)
		{
			usleep(iTimeOut*1000/4*1000);
			continue;
		}

		printf("----连接前端设备,IP:%s,timeSpan:%ld s----\n",m_strIP.c_str(),iTimeOut);
		//todo Connect IPC
		//if(SetSystemDateAndTime()== 0)
		if(0==VerifyPassword())
		{
			//printf("验证密码成功，设备在线,IP:%s\n",m_strIP.c_str());
			m_nKeepAliveCount = 0;  //验证密码成功
		}else{
			m_nKeepAliveCount++;    //验证密码失败
			printf("IP[%s]验证密码失败,次数:%d\n",m_strIP.c_str(),m_nKeepAliveCount);
		}
		dwTime = GetTickCount();  //更新刷新时间
	}
	printf("退出 OnvifOp::OnCheckAlive()\n");
	return 0;
}
*/
#ifdef WIN32
struct tm Seconds2UTC(time_t ntime)
{
	struct tm stime;
	ZeroMemory(&stime, sizeof(stime));
	ntime -=8*3600;  //转换为utc时间

	tm* ts = localtime(&ntime);
	if (!ts)
	{
		return stime;
	}

	return *ts;
}
#endif

int OnvifOp::SetSystemDateAndTime()
{
	CInteLock lock(&g_OnvifOpLock);
	//printf("thread:[%lu] %ld\n",pthread_self(),(long)&g_OnvifOpLock);
	//return 0;   //for test chenyu
	int nRet =0;
	DeviceBinding   proxyDevice;
	do
	{
		//校时不需要token参数
// 		if((nRet = GetCapabilities())!=0)  //获取参数
// 		{
// 			AddLogInfo("SetSystemDateAndTime GetCapabilities ret:%d devip:%s\n",nRet,m_strIP.c_str());
// 			nRet = -1;
// 			break;
// 		}
		InitProxy(&proxyDevice);
		if((nRet = Set_soap_wsse_para("all",proxyDevice.soap))!=0)
		{
			AddLogInfo("SetSystemDateAndTime Set_soap_wsse_para ret:%d devip:%s\n",nRet,m_strIP.c_str());
			nRet = -2;
			break;
		}
		//SetSystemDateAndTime
		_tds__SetSystemDateAndTime *tds__SetSystemDateAndTime = soap_new__tds__SetSystemDateAndTime(soap,-1);
		_tds__SetSystemDateAndTimeResponse *tds__SetSystemDateAndTimeResponse = soap_new__tds__SetSystemDateAndTimeResponse(soap,-1);
		
		//加上时区 chenyu 20161102
		tds__SetSystemDateAndTime->TimeZone = soap_new_tt__TimeZone(soap,-1);
		tds__SetSystemDateAndTime->TimeZone->TZ = "CST-8";  //北京时区

		tds__SetSystemDateAndTime->UTCDateTime = soap_new_tt__DateTime(soap,-1);
		tds__SetSystemDateAndTime->UTCDateTime->Date = soap_new_tt__Date(soap,-1);
		tds__SetSystemDateAndTime->UTCDateTime->Time = soap_new_tt__Time(soap,-1);

		time_t iDt =time(NULL);
		struct tm *ptm;
		struct tm local_tm;
		//ptm = localtime_r(&iDt,&local_tm); //本地时间 ＝ UTC时间 ＋8
#ifdef WIN32
		//ptm = _gmtime32_s(&local_tm,&iDt);   //UTC时间
		local_tm = Seconds2UTC(iDt);
#else
		ptm = gmtime_r(&iDt,&local_tm);   //UTC时间
#endif

		tds__SetSystemDateAndTime->UTCDateTime->Date->Year = local_tm.tm_year+1900;
		tds__SetSystemDateAndTime->UTCDateTime->Date->Month = local_tm.tm_mon+1;
		tds__SetSystemDateAndTime->UTCDateTime->Date->Day = local_tm.tm_mday;
		tds__SetSystemDateAndTime->UTCDateTime->Time->Hour = local_tm.tm_hour;
		tds__SetSystemDateAndTime->UTCDateTime->Time->Minute = local_tm.tm_min;
		tds__SetSystemDateAndTime->UTCDateTime->Time->Second = local_tm.tm_sec;
		if(SOAP_OK == proxyDevice.__tds__SetSystemDateAndTime(tds__SetSystemDateAndTime, tds__SetSystemDateAndTimeResponse))
		{
 			//AddLogInfo("__tds__SetSystemDateAndTime success:%d-%d-%d %d:%d:%d %s\n",
 			//tds__SetSystemDateAndTime->UTCDateTime->Date->Year,
 			//tds__SetSystemDateAndTime->UTCDateTime->Date->Month,
 			//tds__SetSystemDateAndTime->UTCDateTime->Date->Day,
 			//tds__SetSystemDateAndTime->UTCDateTime->Time->Hour,
 			//tds__SetSystemDateAndTime->UTCDateTime->Time->Minute,
 			//tds__SetSystemDateAndTime->UTCDateTime->Time->Second,
 			//m_strIP.c_str());
		}
		else
		{
			PrintErr(proxyDevice.soap);
			AddLogInfo("SetSystemDateAndTime__tds__SetSystemDateAndTime error devip:%s\n",m_strIP.c_str());
			nRet = -3;
			break;
		}
	} while (0);
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

int OnvifOp::MakeKeyFrame()
{
	CInteLock lock(&g_OnvifOpLock);
	//return 0; //for test chenyu
	int nRet =0;
	MediaBinding    proxyMedia;
	do
	{
		if((nRet = GetCapabilities())!=0)  //获取参数
		{
			AddLogInfo("MakeKeyFrame GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
			nRet = -1;
			break;
		}
		InitProxy(NULL,&proxyMedia);
		if((nRet = Set_soap_wsse_para("all",proxyMedia.soap))!=0)
		{
			AddLogInfo("MakeKeyFrame Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
			nRet = -2;
			break;
		}
		//SetSynchronizationPoint
		_trt__SetSynchronizationPoint *trt__SetSynchronizationPoint = soap_new__trt__SetSynchronizationPoint(soap,-1);
		_trt__SetSynchronizationPointResponse *trt__SetSynchronizationPointResponse = soap_new__trt__SetSynchronizationPointResponse(soap,-1);
		trt__SetSynchronizationPoint->ProfileToken = m_strProfileToken;
		if(SOAP_OK == proxyMedia.__trt__SetSynchronizationPoint(trt__SetSynchronizationPoint, trt__SetSynchronizationPointResponse))
		{
			AddLogInfo("__trt__SetSynchronizationPoint success ip:%s\n",m_strIP.c_str());
		}
		else
		{
			PrintErr(proxyMedia.soap);
			AddLogInfo("MakeKeyFrame__trt__SetSynchronizationPoint error,devip:%s\n",m_strIP.c_str());
			nRet = -3;
			break;
		}
	} while (0);
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

int OnvifOp::PTZCtrl(BYTE* lpBuffer, int nBufferLen)
{
	//解析命令，调用相应函数
	if (nBufferLen == 7)
	{
		BOOL bSucceed = FALSE;
		BYTE b1 = *lpBuffer;
		BYTE b2 = *(lpBuffer+1);
		BYTE b3 = *(lpBuffer+2);
		BYTE b4 = *(lpBuffer+3);
		BYTE b5 = *(lpBuffer+4);
		BYTE b6 = *(lpBuffer+5);
		BYTE b7 = *(lpBuffer+6);
		if (b1!=0xFF||b7 !=((b2+b3+b4+b5+b6)%0x100))
		{
			return 0;
		}
		DWORD dwPTZCommand = 0;
		DWORD dwSpeed = 0;
		DWORD dwStop = 0;
		DWORD dwPresetIndex = b6;
		UINT uiCtrl=0;
		uiCtrl = b3;
		uiCtrl=(uiCtrl<<8)+b4;
		if (b5 == 1||b5 == 2||b5 == 3||b5==4||b5==5||b5==6||b5==7||b5==8)
		{
			b5 = 1;
		}
		else
		{
			b5 = b5/9;
		}
		if (b6 == 1||b6 == 2||b6 == 3||b6==4||b6==5||b6==6||b6==7||b6==8)
		{
			b6 = 1;
		}
		else
		{
			b6 = b6/9;
		}
		switch (uiCtrl)
		{
		case 0x03:
			{
				dwPTZCommand = SET_PRESET;
				//bSucceed = NET_DVR_PTZPreset_Other(m_lLoginHandle,nChannelID,dwPTZCommand,dwPresetIndex);
				PTZPreset(dwPTZCommand,dwPresetIndex);
			}
			break;
		case 0x05:
			{
				dwPTZCommand = CLE_PRESET;
				//bSucceed = NET_DVR_PTZPreset_Other(m_lLoginHandle,nChannelID,dwPTZCommand,dwPresetIndex);
				PTZPreset(dwPTZCommand,dwPresetIndex);
			}
			break;
		case 0x07:
			{
				dwPTZCommand = GOTO_PRESET;
				//bSucceed = NET_DVR_PTZPreset_Other(m_lLoginHandle,nChannelID,dwPTZCommand,dwPresetIndex);
				PTZPreset(dwPTZCommand,dwPresetIndex);
			}
			break;
		default:
			{
				if (uiCtrl&0x400)
				{
					dwPTZCommand = IRIS_CLOSE;
				}
				if (uiCtrl&0x200)
				{
					dwPTZCommand = IRIS_OPEN;
				}
				if (uiCtrl&0x100)
				{
					dwPTZCommand = FOCUS_NEAR;
				}
				if (uiCtrl&0x80)
				{
					dwPTZCommand = FOCUS_FAR;
				}
				if (uiCtrl&0x40)
				{
					dwPTZCommand = ZOOM_OUT;
				}
				if (uiCtrl&0x20)
				{
					dwPTZCommand = ZOOM_IN;
				}
				if (uiCtrl&0x10)
				{
					//AddListInfo(LOG_INFO_LEVEL_FATAL, "hik 收到 向下 控球命令");
					dwPTZCommand = TILT_DOWN;
					dwSpeed = b6;
				}
				if (uiCtrl&0x08)
				{
					//AddListInfo(LOG_INFO_LEVEL_FATAL, "hik 收到 向上 控球命令");
					dwPTZCommand = TILT_UP;
					dwSpeed = b6;
				}
				if (uiCtrl&0x04)
				{
					//AddListInfo(LOG_INFO_LEVEL_FATAL, "hik 收到 向左 控球命令");
					dwPTZCommand = PAN_LEFT;
					dwSpeed = b5;
				}
				if (uiCtrl&0x02)
				{
					//AddListInfo(LOG_INFO_LEVEL_FATAL, "hik 收到 向右 控球命令");
					dwPTZCommand = PAN_RIGHT;
					dwSpeed = b5;
				}
				if ((uiCtrl&0x10)&&(uiCtrl&0x04))
				{
					dwPTZCommand = DOWN_LEFT;
					dwSpeed = (b5>b6)?b5:b6;
				}
				else if ((uiCtrl&0x10)&&(uiCtrl&0x02))
				{
					dwPTZCommand = DOWN_RIGHT;
					dwSpeed = (b5>b6)?b5:b6;
				}
				else if ((uiCtrl&0x08)&&(uiCtrl&0x04))
				{
					dwPTZCommand = UP_LEFT;
					dwSpeed = (b5>b6)?b5:b6;
				}
				else if ((uiCtrl&0x08)&&(uiCtrl&0x02))
				{
					dwPTZCommand = UP_RIGHT;
					dwSpeed = (b5>b6)?b5:b6;
				}
				if (uiCtrl==0x0)
				{
					dwStop = 1;
					//dwPTZCommand = m_LastCtrl;
					//AddListInfo(LOG_INFO_LEVEL_FATAL, "停止命令");
				}
				//m_LastCtrl = dwPTZCommand;
				DoPTZControl(dwPTZCommand, dwStop,dwSpeed);
			}
			break;
		}
		return bSucceed;
	}
	else if (nBufferLen == 28)
	{
	}
	else
	{
		return 0;
	}
}

int OnvifOp::PTZPreset(DWORD dwPTZCommand,DWORD dwPresetIndex)
{
	CInteLock lock(&g_OnvifOpLock);
	int nRet =0;
	PTZBinding      proxyPTZ;
	try{
	do
	{
		if((nRet = GetCapabilities())!=0)  //获取参数
		{
			AddLogInfo("PTZPreset GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
			nRet = -1;
			break;
		}
		InitProxy(NULL,NULL,NULL,&proxyPTZ);
		if((nRet = Set_soap_wsse_para("Digest",proxyPTZ.soap))!=0)
		{
			AddLogInfo("PTZPreset Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
			nRet = -2;
			break;
		}
		printf("dwPresetIndex:%d\n",dwPresetIndex);
		switch(dwPTZCommand)
		{
		case SET_PRESET:
			{
				_tptz__SetPreset *tptz__SetPreset = soap_new__tptz__SetPreset(soap,-1);
				_tptz__SetPresetResponse *tptz__SetPresetResponse = soap_new__tptz__SetPresetResponse(soap,-1);
				tptz__SetPreset->ProfileToken = m_strProfileToken;
				char szPresetName[20]={0};
				//sprintf(szPresetName,"yzw_%d",dwPresetIndex);
				sprintf(szPresetName,"%d",dwPresetIndex);
				string strPresetName = szPresetName;
				tptz__SetPreset->PresetName = &strPresetName;
				if (proxyPTZ.__tptz__SetPreset(tptz__SetPreset, tptz__SetPresetResponse) == SOAP_OK)
				{
					printf("tptz__SetPreset success,token:%s\n",tptz__SetPresetResponse->PresetToken.c_str());
				}
				else
				{
					PrintErr(proxyPTZ.soap);
					AddLogInfo("PTZPreset __tptz__SetPreset error devip:%s\n",m_strIP.c_str());
					//return -1;
					nRet = -3;
				}
			}
			break;
		case CLE_PRESET:
			{
				//getToken by PresetName
				_tptz__GetPresets *tptz__GetPresets = soap_new__tptz__GetPresets(soap,-1);
				_tptz__GetPresetsResponse *tptz__GetPresetsResponse = soap_new__tptz__GetPresetsResponse(soap,-1);
				tptz__GetPresets->ProfileToken = m_strProfileToken;
				string strPresetToken = "0";
				char szPresetName[20]={0};
				//sprintf(szPresetName,"yzw_%d",dwPresetIndex);
				sprintf(szPresetName,"%d",dwPresetIndex);
				string strPresetName = szPresetName;
				strPresetToken = strPresetName;
				if (proxyPTZ.__tptz__GetPresets(tptz__GetPresets, tptz__GetPresetsResponse) == SOAP_OK)
				{
					printf("tptz__GetPresets success,preset total:%d\n",tptz__GetPresetsResponse->Preset.size());
					for(int i=0; i<tptz__GetPresetsResponse->Preset.size();i++)
					{
						if(tptz__GetPresetsResponse->Preset[i]->Name 
							&& (*tptz__GetPresetsResponse->Preset[i]->Name == strPresetName))
						{
							strPresetToken = *tptz__GetPresetsResponse->Preset[i]->token;
							printf("find PresetToken:%s\n",strPresetToken.c_str());
							break;
						}
					}
				}
				else
				{
					PrintErr(proxyPTZ.soap);
					AddLogInfo("PTZPreset __tptz__GetPresets error devip:%s\n",m_strIP.c_str());
					//return -1;
					nRet = -4;
				}
				//soap_destroy(soap); // remove deserialized class instances (C++ only)
				//soap_end(soap); // clean up and remove deserialized data
				if(Set_soap_wsse_para("Digest",proxyPTZ.soap)!=0)
				{
					nRet = -5;
					break;
				}
				_tptz__RemovePreset *tptz__RemovePreset = soap_new__tptz__RemovePreset(soap,-1);
				_tptz__RemovePresetResponse *tptz__RemovePresetResponse = soap_new__tptz__RemovePresetResponse(soap,-1);
				tptz__RemovePreset->ProfileToken = m_strProfileToken;
				tptz__RemovePreset->PresetToken = strPresetToken;
				if (proxyPTZ.__tptz__RemovePreset(tptz__RemovePreset, tptz__RemovePresetResponse) == SOAP_OK)
				{
					printf("tptz__RemovePreset success\n");
				}
				else
				{
					PrintErr(proxyPTZ.soap);
					AddLogInfo("PTZPreset __tptz__RemovePreset error devip:%s\n",m_strIP.c_str());
					//return -1;
					nRet = -6;
				}
			}
			break;
		case GOTO_PRESET:
			{
			    //<Name,token> name 会有重复的值 待处理 <1,001> <2,002> <1,003><3,004> to do chenyu
				//getToken by PresetName
				_tptz__GetPresets *tptz__GetPresets = soap_new__tptz__GetPresets(soap,-1);
				_tptz__GetPresetsResponse *tptz__GetPresetsResponse = soap_new__tptz__GetPresetsResponse(soap,-1);
				tptz__GetPresets->ProfileToken = m_strProfileToken;
				string strPresetToken = "0";
				char szPresetName[20]={0};
				//sprintf(szPresetName,"yzw_%d",dwPresetIndex);
				sprintf(szPresetName,"%d",dwPresetIndex);
				string strPresetName = szPresetName;
				strPresetToken = strPresetName;
				if (proxyPTZ.__tptz__GetPresets(tptz__GetPresets, tptz__GetPresetsResponse) == SOAP_OK)
				{
					printf("tptz__GetPresets success,preset total:%d\n",tptz__GetPresetsResponse->Preset.size());
					for(int i=0; i<tptz__GetPresetsResponse->Preset.size();i++)
					{
						if(tptz__GetPresetsResponse->Preset[i]->Name 
							&& (*tptz__GetPresetsResponse->Preset[i]->Name == strPresetName))
						{
							strPresetToken = *tptz__GetPresetsResponse->Preset[i]->token;
							printf("find PresetToken:%s\n",strPresetToken.c_str());
							break;
						}
					}
				}
				else
				{
					PrintErr(proxyPTZ.soap);
					AddLogInfo("PTZPreset __tptz__GetPresets2 error devip:%s\n",m_strIP.c_str());
					//return -1;
					nRet = -7;
				}
				//soap_destroy(soap); // remove deserialized class instances (C++ only)
				//soap_end(soap); // clean up and remove deserialized data
				if(Set_soap_wsse_para("Digest",proxyPTZ.soap)!=0)
				{
					nRet = -8;
					break;
				}
				_tptz__GotoPreset *tptz__GotoPreset = soap_new__tptz__GotoPreset(soap,-1);
				_tptz__GotoPresetResponse *tptz__GotoPresetResponse = soap_new__tptz__GotoPresetResponse(soap,-1);
				tptz__GotoPreset->ProfileToken = m_strProfileToken;
				tptz__GotoPreset->PresetToken = strPresetToken;
				if (proxyPTZ.__tptz__GotoPreset(tptz__GotoPreset, tptz__GotoPresetResponse) == SOAP_OK)
				{
					printf("tptz__GotoPreset success\n");
				}
				else
				{
					PrintErr(proxyPTZ.soap);
					AddLogInfo("PTZPreset __tptz__GotoPreset error devip:%s\n",m_strIP.c_str());
					//return -1;
					nRet = -9;
				}
			}
			break;
		default:
			{
               printf("PTZPreset default\n");
			}
			break;
		}
	} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::PTZPreset do while error\n");
		nRet = -10;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
	return 0;
}

int OnvifOp::DoPTZControl(DWORD dwPTZCommand, DWORD dwStop, DWORD dwSpeed)
{
	int nRet =0;
	if((nRet = GetPtzConfigurationOptions())!=0)  //获取参数范围
	{
		AddLogInfo("DoPTZControl GetPtzConfigurationOptions error ret:%d devip:%s\n",nRet,m_strIP.c_str());
		return -1;
	}
	CInteLock lock(&g_OnvifOpLock);
	bool blSupportPTZ = false;
	//struct soap *soap = soap_new();
	PTZBinding      proxyPTZ;
	try{
	do
	{
		if((nRet = GetCapabilities())!=0)  //获取参数
		{
			AddLogInfo("DoPTZControl GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
			nRet = -2;
			break;
		}

		InitProxy(NULL,NULL,NULL,&proxyPTZ);
		if(m_blSupportPTZ)
		{
			if((nRet = Set_soap_wsse_para("Digest",proxyPTZ.soap))!=0)
			{
				AddLogInfo("DoPTZControl Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -3;
				break;
			}

			if(1 == dwStop)//s 停止
			{
				_tptz__Stop *tptz__Stop = soap_new__tptz__Stop(soap,-1);
				_tptz__StopResponse *tptz__StopResponse = soap_new__tptz__StopResponse(soap,-1);
				
				tptz__Stop->ProfileToken = m_strProfileToken;
				tptz__Stop->PanTilt = (bool *)malloc(sizeof(bool));
				tptz__Stop->Zoom = (bool *)malloc(sizeof(bool));
				*tptz__Stop->PanTilt = true;
				*tptz__Stop->Zoom = true;
				if (proxyPTZ.__tptz__Stop(tptz__Stop,tptz__StopResponse) == SOAP_OK)
				{
				}
				else
				{
					PrintErr(proxyPTZ.soap);
					AddLogInfo("DoPTZControl __tptz__Stop devip:%s\n",m_strIP.c_str());
					nRet = -4;
					break;
				}
				AddLogInfo("__ptz_stop__\n");
				break;
			}

			_tptz__ContinuousMove *tptz__ContinuousMove = soap_new__tptz__ContinuousMove(soap,-1);
			_tptz__ContinuousMoveResponse *tptz__ContinuousMoveResponse = soap_new__tptz__ContinuousMoveResponse(soap,-1);

			//tptz__ContinuousMove->ProfileToken = tptz__GetConfiguration->PTZConfigurationToken;
			tptz__ContinuousMove->ProfileToken = m_strProfileToken;
			tptz__ContinuousMove->Velocity = soap_new_tt__PTZSpeed(soap,-1);
			tptz__ContinuousMove->Velocity->PanTilt = soap_new_tt__Vector2D(soap,-1);
			tptz__ContinuousMove->Velocity->Zoom = soap_new_tt__Vector1D(soap,-1);

			DWORD   dwResult   =   0;
			//dwSpeed = 4; //暂时写死 chenyu
		
			int nSpeed1 = 0;
			int nSpeed2 = 0;
			nSpeed1 = (dwSpeed & 0xffff0000)>>16;
		    nSpeed2 = (dwSpeed & 0x0000ffff);
			if (nSpeed1 >= nSpeed2)
			{
				dwSpeed = nSpeed1;
			}else{
				dwSpeed = nSpeed2;
			}
			char szTmp[100]={0};
			sprintf(szTmp,"--ptz Client speed:%d %d %d--",nSpeed1,nSpeed2,dwSpeed);
			AddLogInfo(szTmp);
			if(PAN_RIGHT == dwPTZCommand)//a   右
			{
				dwSpeed = dwSpeed * m_stPtzMoveOptions.nPanRange*100/(2*255);
				if(DoContinuousMove(tptz__ContinuousMove,tptz__ContinuousMoveResponse,0.01*dwSpeed,0,0,0,proxyPTZ)!=0)
				{
					printf("OnvifOp::DoPTZControl DoContinuousMove fail\n");
					nRet = -5;
					break;
				}
				printf("__ptz_right__\n");
			}
			else if(PAN_LEFT == dwPTZCommand)//d   左
			{
				dwSpeed = dwSpeed * m_stPtzMoveOptions.nPanRange*100/(2*255);
				if(DoContinuousMove(tptz__ContinuousMove,tptz__ContinuousMoveResponse,-0.01*dwSpeed,0,0,0,proxyPTZ)!=0)
				{
					printf("OnvifOp::DoPTZControl DoContinuousMove fail\n");
					nRet = -6;
					break;
				}
				printf("__ptz_left__\n");
			}
			else if(TILT_UP == dwPTZCommand)//a   上
			{
				dwSpeed = dwSpeed * m_stPtzMoveOptions.nPanRange*100/(2*255);
				if(DoContinuousMove(tptz__ContinuousMove,tptz__ContinuousMoveResponse,0,0.01*dwSpeed,0,0,proxyPTZ)!=0)
				{
					printf("OnvifOp::DoPTZControl DoContinuousMove fail\n");
					nRet = -7;
					break;
				}
				printf("__ptz_up__\n");
			}
			else if(TILT_DOWN == dwPTZCommand)//d   下
			{
				dwSpeed = dwSpeed * m_stPtzMoveOptions.nPanRange*100/(2*255);
				if(DoContinuousMove(tptz__ContinuousMove,tptz__ContinuousMoveResponse,0,-0.01*dwSpeed,0,0,proxyPTZ)!=0)
				{
					printf("OnvifOp::DoPTZControl DoContinuousMove fail\n");
					nRet = -8;
					break;
				}
				printf("__ptz_down__\n");
			}
			else if(UP_LEFT == dwPTZCommand)//a   左上
			{
				dwSpeed = dwSpeed * m_stPtzMoveOptions.nPanRange*100/(2*255);
				if(DoContinuousMove(tptz__ContinuousMove,tptz__ContinuousMoveResponse,-0.01*dwSpeed,0.01*dwSpeed,0,0,proxyPTZ)!=0)
				{
					printf("OnvifOp::DoPTZControl DoContinuousMove fail\n");
					nRet = -9;
					break;
				}
				printf("__ptz_upleft__\n");
			}
			else if(UP_RIGHT == dwPTZCommand)//d   右上
			{
				dwSpeed = dwSpeed * m_stPtzMoveOptions.nPanRange*100/(2*255);
				if(DoContinuousMove(tptz__ContinuousMove,tptz__ContinuousMoveResponse,0.01*dwSpeed,0.01*dwSpeed,0,0,proxyPTZ)!=0)
				{
					printf("OnvifOp::DoPTZControl DoContinuousMove fail\n");
					nRet = -10;
					break;
				}
				printf("__ptz_upright__\n");
			}
			else if(DOWN_LEFT == dwPTZCommand)//a   左下
			{
				dwSpeed = dwSpeed * m_stPtzMoveOptions.nPanRange*100/(2*255);
				if(DoContinuousMove(tptz__ContinuousMove,tptz__ContinuousMoveResponse,-0.01*dwSpeed,-0.01*dwSpeed,0,0,proxyPTZ)!=0)
				{
					printf("OnvifOp::DoPTZControl DoContinuousMove fail\n");
					nRet = -11;
					break;
				}
				printf("__ptz_downleft__\n");
			}
			else if(DOWN_RIGHT == dwPTZCommand)//d   右下
			{
				dwSpeed = dwSpeed * m_stPtzMoveOptions.nPanRange*100/(2*255);
				if(DoContinuousMove(tptz__ContinuousMove,tptz__ContinuousMoveResponse,0.01*dwSpeed,-0.01*dwSpeed,0,0,proxyPTZ)!=0)
				{
					printf("OnvifOp::DoPTZControl DoContinuousMove fail\n");
					nRet = -12;
					break;
				}
				printf("__ptz_downright__\n");
			}
			else if(ZOOM_OUT == dwPTZCommand)//a   zoomin
			{
				dwSpeed = 4; 
				if(DoContinuousMove(tptz__ContinuousMove,tptz__ContinuousMoveResponse,0,0,0.1*dwSpeed,0,proxyPTZ)!=0)
				{
					printf("OnvifOp::DoPTZControl DoContinuousMove fail\n");
					nRet = -13;
					break;
				}
				printf("__ptz_zoomin__\n");
			}
			else if( ZOOM_IN == dwPTZCommand)//a   zoomout
			{
				dwSpeed = 4; 
				if(DoContinuousMove(tptz__ContinuousMove,tptz__ContinuousMoveResponse,0,0,-0.1*dwSpeed,0,proxyPTZ)!=0)
				{
					printf("OnvifOp::DoPTZControl DoContinuousMove fail\n");
					nRet = -14;
					break;
				}
				printf("__ptz_zoomout__\n");
			}
			//char szTmp[100]={0};
			sprintf(szTmp,"--ptz Onvif speed:%d--",dwSpeed);
			AddLogInfo(szTmp);
		}
	}while(0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::DoPTZControl do while error\n");
		nRet = -15;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

int OnvifOp::DoContinuousMove(_tptz__ContinuousMove *tptz__ContinuousMove,
		                       _tptz__ContinuousMoveResponse *tptz__ContinuousMoveResponse,
		                       float px,float py,float zx,float zy,PTZBinding& proxyPTZ)
{
	int nRet = 0;
	do
	{
		if((nRet = Set_soap_wsse_para("Digest",proxyPTZ.soap))!=0)
		{
			AddLogInfo("DoContinuousMove Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
			nRet = -1;
			break;
		}

		tptz__ContinuousMove->Velocity->PanTilt->x = px;
		tptz__ContinuousMove->Velocity->PanTilt->y = py;
		tptz__ContinuousMove->Velocity->Zoom->x = zx;
		//tptz__ContinuousMove->Velocity->Zoom->y = zy;
		//printf("ptz token:%s\n",tptz__ContinuousMove->ProfileToken.c_str());
		if (proxyPTZ.__tptz__ContinuousMove(tptz__ContinuousMove,tptz__ContinuousMoveResponse) == SOAP_OK)
		{
		}
		else
		{
			PrintErr(proxyPTZ.soap);
			AddLogInfo("DoContinuousMove __tptz__ContinuousMove error devip:%s\n",m_strIP.c_str());
			//return -1;
			nRet = -2;
			break;
		}
	}while(0);
	return nRet;
}

int OnvifOp::SetVideoParam( BYTE byType, BYTE byParam)
{
	CInteLock lock(&g_OnvifOpLock);
	int nRet =0;
	ImagingBinding  proxyImaging;
	try{
	do
	{
		if((nRet = GetCapabilities())!=0)  //获取参数
		{
			AddLogInfo("SetVideoParam GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
			nRet = -1;
			break;
		}
		InitProxy(NULL,NULL,&proxyImaging);
		if((nRet= Set_soap_wsse_para("all",proxyImaging.soap))!=0)
		{
			AddLogInfo("SetVideoParam Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
			nRet = -2;
			break;
		}
		//SetImagingSettings
		_timg__SetImagingSettings *timg__SetImagingSettings = soap_new__timg__SetImagingSettings(soap,-1);
		_timg__SetImagingSettingsResponse *timg__SetImagingSettingsResponse = soap_new__timg__SetImagingSettingsResponse(soap,-1);
		timg__SetImagingSettings->VideoSourceToken = m_strVideoSourceToken;
		timg__SetImagingSettings->ImagingSettings = soap_new_tt__ImagingSettings20(soap,-1);
		if(byType == 0)//色度不处理
		{
			;
		}
		else if(byType == 1)
		{
			timg__SetImagingSettings->ImagingSettings->Brightness = (float *)malloc(sizeof(float));
			*timg__SetImagingSettings->ImagingSettings->Brightness = byParam;
		}
		else if(byType == 2)
		{
			timg__SetImagingSettings->ImagingSettings->Contrast = (float *)malloc(sizeof(float));
			*timg__SetImagingSettings->ImagingSettings->Contrast = byParam;
		}
		else if(byType == 3)
		{
			timg__SetImagingSettings->ImagingSettings->ColorSaturation = (float *)malloc(sizeof(float));
			*timg__SetImagingSettings->ImagingSettings->ColorSaturation = byParam;
		}
		if(SOAP_OK == proxyImaging.__timg__SetImagingSettings(timg__SetImagingSettings,timg__SetImagingSettingsResponse))
		{

		}
		else
		{
			PrintErr(proxyImaging.soap);
			AddLogInfo("SetVideoParam __timg__SetImagingSettings error devip:%s\n",m_strIP.c_str());
			//return -1;
			nRet = -3;
			break;
		}
	} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::SetVideoParam do while error\n");
		nRet = -4;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

int OnvifOp::SetVideoParam(ONVIF_IMAGE_INFO stParam)
{
	CInteLock lock(&g_OnvifOpLock);
	int nRet =0;
	ImagingBinding  proxyImaging;
	try{
		do
		{
			if((nRet=GetCapabilities())!=0)  //获取参数
			{
				AddLogInfo("SetVideoParam GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -1;
				break;
			}
			InitProxy(NULL,NULL,&proxyImaging);
			if((nRet=Set_soap_wsse_para("all",proxyImaging.soap))!=0)
			{
				AddLogInfo("SetVideoParam Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -2;
				break;
			}
			//SetImagingSettings
			_timg__SetImagingSettings *timg__SetImagingSettings = soap_new__timg__SetImagingSettings(soap,-1);
			_timg__SetImagingSettingsResponse *timg__SetImagingSettingsResponse = soap_new__timg__SetImagingSettingsResponse(soap,-1);
			timg__SetImagingSettings->VideoSourceToken = m_strVideoSourceToken;
			timg__SetImagingSettings->ImagingSettings = soap_new_tt__ImagingSettings20(soap,-1);
			//标准[0,100] =>转换为Onvif相机定义的
			if(stParam.nBrightness>=0)
			{
				float fValue = stParam.nBrightness*m_stOptions.nBrightnessRange/100;
				timg__SetImagingSettings->ImagingSettings->Brightness = (float *)malloc(sizeof(float));
				*timg__SetImagingSettings->ImagingSettings->Brightness = fValue;
				char szTmp[1024]={0};
				sprintf(szTmp,"SetVideoParam Brightness:%f",fValue);
				AddLogInfo(szTmp);
			}
			if(stParam.nContrast>=0)
			{
				float fValue = stParam.nContrast*m_stOptions.nContrastRange/100;
				timg__SetImagingSettings->ImagingSettings->Contrast = (float *)malloc(sizeof(float));
				*timg__SetImagingSettings->ImagingSettings->Contrast = fValue;
				char szTmp[1024]={0};
				sprintf(szTmp,"SetVideoParam Contrast:%f",fValue);
				AddLogInfo(szTmp);
			}
			if(stParam.nSaturation>=0)
			{
				float fValue = stParam.nSaturation*m_stOptions.nSaturationRange/100;
				timg__SetImagingSettings->ImagingSettings->ColorSaturation = (float *)malloc(sizeof(float));
				*timg__SetImagingSettings->ImagingSettings->ColorSaturation = fValue;
				char szTmp[1024]={0};
				sprintf(szTmp,"SetVideoParam ColorSaturation:%f",fValue);
				AddLogInfo(szTmp);
			}
			if(stParam.nColourDegree>=0)
			{
				//Sharpness  暂不处理
// 				float fValue = stParam.nColourDegree*m_stOptions.nColourDegreeRange/100;
// 				timg__SetImagingSettings->ImagingSettings->Sharpness = (float *)malloc(sizeof(float));
// 				*timg__SetImagingSettings->ImagingSettings->Sharpness = fValue;
// 				char szTmp[1024]={0};
// 				sprintf(szTmp,"SetVideoParam Sharpness:%f",fValue);
// 				AddLogInfo(szTmp);
			}

			if(SOAP_OK == proxyImaging.__timg__SetImagingSettings(timg__SetImagingSettings,timg__SetImagingSettingsResponse))
			{
				AddLogInfo("SetVideoParam success");
			}
			else
			{
				PrintErr(proxyImaging.soap);
				AddLogInfo("SetVideoParam __timg__SetImagingSettings error devip:%s\n",m_strIP.c_str());
				//return -1;
				nRet = -3;
				break;
			}
		} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::SetVideoParam do while error\n");
		nRet = -4;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

int OnvifOp::GetVideoParam(unsigned int& nParam)
{
	CInteLock lock(&g_OnvifOpLock);
	int nRet =0;
	ImagingBinding  proxyImaging;
	try{
	do
	{
		if((nRet=GetCapabilities())!=0)  //获取参数
		{
			AddLogInfo("GetVideoParam GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
			nRet = -1;
			break;
		}
		InitProxy(NULL,NULL,&proxyImaging);
		if((nRet=Set_soap_wsse_para("all",proxyImaging.soap))!=0)
		{
			AddLogInfo("GetVideoParam Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
			nRet = -2;
			break;
		}
		//GetImagingSettings
		_timg__GetImagingSettings *timg__GetImagingSettings = soap_new__timg__GetImagingSettings(soap,-1);
		_timg__GetImagingSettingsResponse *timg__GetImagingSettingsResponse = soap_new__timg__GetImagingSettingsResponse(soap,-1);
		timg__GetImagingSettings->VideoSourceToken = m_strVideoSourceToken;
		if(SOAP_OK == proxyImaging.__timg__GetImagingSettings(timg__GetImagingSettings,timg__GetImagingSettingsResponse))
		{
			printf("ColorSaturation:%f Brightness:%f Contrast:%f",
					*timg__GetImagingSettingsResponse->ImagingSettings->ColorSaturation,
			       *timg__GetImagingSettingsResponse->ImagingSettings->Brightness,
			       *timg__GetImagingSettingsResponse->ImagingSettings->Contrast);

			unsigned int nTempParam = (0);
			nTempParam = (nTempParam<<8)|(unsigned int)((*timg__GetImagingSettingsResponse->ImagingSettings->Brightness));
			nTempParam = (nTempParam<<8)|(unsigned int)((*timg__GetImagingSettingsResponse->ImagingSettings->Contrast));
			nTempParam = (nTempParam<<8)|(unsigned int)((*timg__GetImagingSettingsResponse->ImagingSettings->ColorSaturation));
			nParam = nTempParam;
		}
		else
		{
			PrintErr(proxyImaging.soap);
			AddLogInfo("GetVideoParam __timg__GetImagingSettings error devip:%s\n",m_strIP.c_str());
			//return -1;
			nRet = -3;
			break;
		}
	} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::GetVideoParam do while error\n");
		nRet = -4;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

int OnvifOp::GetVideoParam(ONVIF_IMAGE_INFO &stParam)
{
	int nRet =0;
	if((nRet=GetOptions())!=0)  //获取参数范围
	{
		AddLogInfo("GetVideoParam GetOptions error ret:%d devip:%s\n",nRet,m_strIP.c_str());
		return -1;
	}
	CInteLock lock(&g_OnvifOpLock);
	ImagingBinding  proxyImaging;
	try{
		do
		{
			if((nRet=GetCapabilities())!=0)  //获取参数
			{
				AddLogInfo("GetVideoParam GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -2;
				break;
			}
			InitProxy(NULL,NULL,&proxyImaging);
			if((nRet=Set_soap_wsse_para("all",proxyImaging.soap))!=0)
			{
				AddLogInfo("GetVideoParam Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -3;
				break;
			}
			//GetImagingSettings
			_timg__GetImagingSettings *timg__GetImagingSettings = soap_new__timg__GetImagingSettings(soap,-1);
			_timg__GetImagingSettingsResponse *timg__GetImagingSettingsResponse = soap_new__timg__GetImagingSettingsResponse(soap,-1);
			timg__GetImagingSettings->VideoSourceToken = m_strVideoSourceToken;
			if(SOAP_OK == proxyImaging.__timg__GetImagingSettings(timg__GetImagingSettings,timg__GetImagingSettingsResponse))
			{
				float fBrightness,fContrast,fSaturation,fColourDegree;
				fBrightness = ((*timg__GetImagingSettingsResponse->ImagingSettings->Brightness));
				fContrast = ((*timg__GetImagingSettingsResponse->ImagingSettings->Contrast));
				fSaturation = ((*timg__GetImagingSettingsResponse->ImagingSettings->ColorSaturation));
				/*stParam.nColourDegree = (unsigned int)((*timg__GetImagingSettingsResponse->ImagingSettings->Sharpness));*/
				AddLogInfo("Onvif ColorSaturation:%f Brightness:%f Contrast:%f Sharpness:%f ",
					fSaturation,fBrightness,fContrast,fColourDegree);
				
				//转化为标准[0,100]
				if (m_stOptions.nBrightnessRange>0)
				{
					stParam.nBrightness = (fBrightness*100 / m_stOptions.nBrightnessRange +0.5);
				}else{
					stParam.nBrightness = 0;
					m_stOptions.nBrightnessRange = 255;
				}
				if (m_stOptions.nColourDegreeRange>0)
				{
					stParam.nColourDegree = (fColourDegree*100 / m_stOptions.nColourDegreeRange+0.5);
				}else{
					stParam.nColourDegree = 0;
					m_stOptions.nColourDegreeRange = 255;
				}
				if (m_stOptions.nContrastRange>0)
				{
					stParam.nContrast = (fContrast*100 / m_stOptions.nContrastRange+0.5);
				}else{
					stParam.nContrast = 0;
					m_stOptions.nContrastRange = 255;
				}
				if (m_stOptions.nSaturationRange>0)
				{
					stParam.nSaturation = (fSaturation*100 / m_stOptions.nSaturationRange+0.5);
				}else{
					stParam.nSaturation = 0;
					m_stOptions.nSaturationRange = 255;
				}
				AddLogInfo("[0~100]Client ColorSaturation:%d Brightness:%d Contrast:%d Sharpness:%d",
					stParam.nSaturation,
					stParam.nBrightness,
					stParam.nContrast,
					stParam.nColourDegree);
			}
			else
			{
				PrintErr(proxyImaging.soap);
				AddLogInfo("GetVideoParam __timg__GetImagingSettings error, devip:%s\n",m_strIP.c_str());
				//return -1;
				nRet = -4;
				break;
			}
		} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::GetVideoParam do while error\n");
		nRet = -5;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

//设置光圈和聚焦
int OnvifOp::SetVideoFocusIris(DWORD dwPTZCommand,DWORD dwParam)
{
	int nRet =0;
	do 
	{
		if ((FOCUS_NEAR == dwPTZCommand) || (FOCUS_FAR == dwPTZCommand))   //move focus
		{   //nRet = SetFocusMode(tt__AutoFocusMode__MANUAL);  //先设为手动
			nRet = SetVideoFocus(dwPTZCommand,dwParam);
			if (nRet < 0)
			{
				break;
			}
		}
		else if ((IRIS_OPEN == dwPTZCommand) || (IRIS_CLOSE == dwPTZCommand))  //光圈
		{
			float fIris = -24;
			nRet = GetVideoIris(fIris);
			if (nRet < 0)
			{
				break;
			}
			//不知道光圈数据的规律，只能测试多次
			for (int nSpan = 1; nSpan<=4;nSpan++)
			{
				//1.设置
				nRet = SetVideoIris(dwPTZCommand,dwParam,fIris,nSpan);
				if (nRet < 0)
				{
#ifdef LANG_EN
#else
					AddLogInfo("设置光圈，第[%d]次设置失败\n",nSpan);
#endif
					break;
				}
				//2.读取
				float fNewIris = -24;
				nRet = GetVideoIris(fNewIris);
				if (nRet < 0)
				{
#ifdef LANG_EN
#else
					AddLogInfo("设置光圈，第[%d]次读取失败\n",nSpan);
#endif
					break;
				}
				if(fNewIris != fIris) //发生变化，设置成功
				{
#ifdef LANG_EN
#else
					AddLogInfo("光圈发生变化，第[%d]次设置成功,原始值:%f 新值:%f\n",nSpan,fIris,fNewIris);
#endif
					break;
				}				
			}
		}
	} while (0);
	return nRet;
}

int OnvifOp::SetVideoFocus(DWORD dwPTZCommand,DWORD dwParam)  //设置聚焦
{
	int nRet =0;
	if((nRet = GetMoveOptions())!=0)  //获取参数范围
	{
		AddLogInfo("SetVideoFocus GetMoveOptions error ret:%d devip:%s\n",nRet,m_strIP.c_str());
		return -1;
	}
	CInteLock lock(&g_OnvifOpLock);
	ImagingBinding  proxyImaging;
	try{
		do
		{
			if((nRet=GetCapabilities())!=0)  //获取参数
			{
				AddLogInfo("SetVideoFocus GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -2;
				break;
			}
			InitProxy(NULL,NULL,&proxyImaging);
			if((nRet=Set_soap_wsse_para("all",proxyImaging.soap))!=0)
			{
				AddLogInfo("SetVideoFocus Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -3;
				break;
			}
			
			_timg__Move *timg__Move = soap_new__timg__Move(soap,-1);
			_timg__MoveResponse *timg__MoveResponse = soap_new__timg__MoveResponse(soap,-1);
			timg__Move->VideoSourceToken = m_strVideoSourceToken;
			timg__Move->Focus = soap_new_tt__FocusMove(soap,-1);
			timg__Move->Focus->Continuous = soap_new_tt__ContinuousFocus(soap,-1);
			
			int dwSpeed = 4; //暂时写死 chenyu
			if (FOCUS_NEAR == dwPTZCommand)
			{
				timg__Move->Focus->Continuous->Speed = (float)m_stMoveOptions.nContinuousMax/3; //0.1*dwSpeed;
			}
			else if(FOCUS_FAR == dwPTZCommand)
			{
				timg__Move->Focus->Continuous->Speed = (float)m_stMoveOptions.nContinuousMin/3; //-0.1*dwSpeed;
			}
			
			if(SOAP_OK == proxyImaging.__timg__Move(timg__Move,timg__MoveResponse))
			{
				AddLogInfo("__timg__Move success,Speed%f [%d,%d]",timg__Move->Focus->Continuous->Speed,
					m_stMoveOptions.nContinuousMin,m_stMoveOptions.nContinuousMax);
			}
			else
			{
				PrintErr(proxyImaging.soap);
				AddLogInfo("SetVideoFocus __timg__Move error devip:%s\n",m_strIP.c_str());
				nRet = -4;
				break;
			}
		} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::SetVideoFocusIris do while error\n");
		nRet = -5;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}  

int OnvifOp::GetVideoIris(float &fIris) //读取聚焦
{
	CInteLock lock(&g_OnvifOpLock);
	int nRet =0;
	ImagingBinding  proxyImaging;
	try{
		do
		{
			if((nRet=GetCapabilities())!=0)  //获取参数
			{
				AddLogInfo("GetVideoIris GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -1;
				break;
			}
			InitProxy(NULL,NULL,&proxyImaging);
			if((nRet=Set_soap_wsse_para("all",proxyImaging.soap))!=0)
			{
				AddLogInfo("GetVideoIris Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -2;
				break;
			}
			fIris = -24;   //默认 全黑
			//GetImagingSettings
			_timg__GetImagingSettings *timg__GetImagingSettings = soap_new__timg__GetImagingSettings(soap,-1);
			_timg__GetImagingSettingsResponse *timg__GetImagingSettingsResponse = soap_new__timg__GetImagingSettingsResponse(soap,-1);
			timg__GetImagingSettings->VideoSourceToken = m_strVideoSourceToken;
			if(SOAP_OK == proxyImaging.__timg__GetImagingSettings(timg__GetImagingSettings,timg__GetImagingSettingsResponse))
			{
				if(timg__GetImagingSettingsResponse->ImagingSettings
					&&timg__GetImagingSettingsResponse->ImagingSettings->Exposure)
				{
					if (tt__ExposureMode__AUTO == timg__GetImagingSettingsResponse->ImagingSettings->Exposure->Mode)
					{
						char szTmp[100]={0};
						sprintf(szTmp,"Iris Mode :Auto,quit",fIris);
						AddLogInfo(szTmp);
						nRet = -3;
						break;
					}
					//手动获取光圈参数
					if(timg__GetImagingSettingsResponse->ImagingSettings->Exposure->Iris)
					{
						fIris = *timg__GetImagingSettingsResponse->ImagingSettings->Exposure->Iris;
						char szTmp[100]={0};
						sprintf(szTmp,"Iris:%f",fIris);
						AddLogInfo(szTmp);
					}
				}else{				
					nRet = -4;
					break;
				}
			}
			else
			{
				PrintErr(proxyImaging.soap);
				AddLogInfo("GetVideoIris __timg__GetImagingSettings error devip:%s\n",m_strIP.c_str());
				//return -1;
				nRet = -5;
				break;
			}
		} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::SetVideoFocusIris do while error\n");
		nRet = -6;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}  

int OnvifOp::SetVideoIris(DWORD dwPTZCommand,DWORD dwParam,float fCurIris,int nSpan)  //设置聚焦
{
	CInteLock lock(&g_OnvifOpLock);
	int nRet =0;
	ImagingBinding  proxyImaging;
	try{
		do
		{
			if((nRet=GetCapabilities())!=0)  //获取参数
			{
				AddLogInfo("SetVideoIris GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -1;
				break;
			}
			InitProxy(NULL,NULL,&proxyImaging);
			if((nRet=Set_soap_wsse_para("all",proxyImaging.soap))!=0)
			{
				AddLogInfo("SetVideoIris Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -2;
				break;
			}
			
			//SetImagingSettings 设置光圈
			_timg__SetImagingSettings *timg__SetImagingSettings = soap_new__timg__SetImagingSettings(soap,-1);
			_timg__SetImagingSettingsResponse *timg__SetImagingSettingsResponse = soap_new__timg__SetImagingSettingsResponse(soap,-1);
			timg__SetImagingSettings->VideoSourceToken = m_strVideoSourceToken;
			timg__SetImagingSettings->ImagingSettings = soap_new_tt__ImagingSettings20(soap,-1);
			timg__SetImagingSettings->ImagingSettings->Exposure = soap_new_tt__Exposure20(soap,-1);
			timg__SetImagingSettings->ImagingSettings->Exposure->Mode = tt__ExposureMode__MANUAL;
			timg__SetImagingSettings->ImagingSettings->Exposure->Iris = (float *)malloc(sizeof(float));
			//hik Iirs value:0 -1 _ -3 -4 _ -6 -7 _ -9 -10 _ -12 -13 _ -15 -16 _ -18 -19 _ -21 -22 _ -24
			if(IRIS_OPEN == dwPTZCommand)  
			{
				int fIris = fCurIris + nSpan;  //新值
				*timg__SetImagingSettings->ImagingSettings->Exposure->Iris =  fIris;
			}
			else
			{
				int fIris = fCurIris - nSpan;  //新值
				*timg__SetImagingSettings->ImagingSettings->Exposure->Iris = fIris;
			}
			char szTmp[100]={0};
			sprintf(szTmp,"--set NewIris:%f",*timg__SetImagingSettings->ImagingSettings->Exposure->Iris);
			AddLogInfo(szTmp);
			if(SOAP_OK == proxyImaging.__timg__SetImagingSettings(timg__SetImagingSettings,timg__SetImagingSettingsResponse))
			{
			}
			else
			{
				PrintErr(proxyImaging.soap);
				AddLogInfo("SetVideoIris __timg__SetImagingSettings error devip:%s\n",m_strIP.c_str());
				nRet = -3;
				break;
			}
		} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::SetVideoFocusIris do while error\n");
		nRet = -4;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}  

int OnvifOp::Set_soap_wsse_para(string strType,struct soap *soap)
{
	int nRet = 0;
	try{
	do
	{
		if(("all"!= strType)&&("Digest"!=strType)&&("stamp"!=strType))
		{
			AddLogInfo("Set_soap_wsse_para error ");
			nRet = -1;
			break;
		}
		if(("all"== strType)||("Digest"==strType))
		{
			if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(soap, NULL, m_strUsr.c_str(), m_strPwd.c_str()))
			{
				AddLogInfo("soap_wsse_add_UsernameTokenDigest error ");
				//return -1;
				nRet = -2;
				break;
			}
		}
		if(("all"== strType)||("stamp"==strType))
		{
			if(SOAP_OK != soap_wsse_add_Timestamp(soap, "Time", 10)) // 10 seconds lifetime
			{
				AddLogInfo("soap_wsse_add_Timestamp error ");
				//return -1;
				nRet = -3;
				break;
			}
		}
	}while(0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::Set_soap_wsse_para do while error\n");
		nRet = -4;
	}
	return nRet;
}

int OnvifOp::GetCapabilities()
{
	if(m_bGetCapabilitiesSuccess)
	{
		return 0;
	}
	int nRet = 0;
	DeviceBinding   proxyDevice;
	MediaBinding    proxyMedia;
	ImagingBinding  proxyImaging;
	PTZBinding      proxyPTZ;
	InitProxy(&proxyDevice,&proxyMedia,&proxyImaging,&proxyPTZ);
	try{
	do
	{
		if((nRet=Set_soap_wsse_para("all",proxyDevice.soap))!=0)
		{
			AddLogInfo("GetCapabilities Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
			nRet = -1;
			break;
		}
		//Capability exchange
		_tds__GetCapabilities *tds__GetCapabilities = soap_new__tds__GetCapabilities(soap,-1);
		tds__GetCapabilities->Category.push_back(tt__CapabilityCategory__All);
		_tds__GetCapabilitiesResponse *tds__GetCapabilitiesResponse = soap_new__tds__GetCapabilitiesResponse(soap,-1);
		if(!tds__GetCapabilitiesResponse)
		{
			AddLogInfo("GetCapabilities tds__GetCapabilitiesResponse NULL devip:%s\n",m_strIP.c_str());
			nRet = -2;
			break;
			//tds__GetCapabilitiesResponse = soap_new__tds__GetCapabilitiesResponse(soap,-1);
		}
		if(SOAP_OK == proxyDevice.__tds__GetCapabilities(tds__GetCapabilities,tds__GetCapabilitiesResponse))
		{
			//fflush(stdout);
			if (tds__GetCapabilitiesResponse->Capabilities->Device != NULL)
			{
				int iSize = tds__GetCapabilitiesResponse->Capabilities->Device->System->SupportedVersions.size();
			}
			if (tds__GetCapabilitiesResponse->Capabilities->Media != NULL)
			{
				//fprintf(stdout,"\r\n-------------------Media-------------------\r\n");
				//fprintf(stdout,"XAddr:%s\r\n",tds__GetCapabilitiesResponse->Capabilities->Media->XAddr.c_str());

				//fprintf(stdout,"\r\n-------------------streaming-------------------\r\n");
				//fprintf(stdout,"RTPMulticast:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTPMulticast)?"Y":"N");
				//fprintf(stdout,"RTP_TCP:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTP_USCORETCP)?"Y":"N");
				//fprintf(stdout,"RTP_RTSP_TCP:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTP_USCORERTSP_USCORETCP)?"Y":"N");
				//fprintf(stdout,"\r\n-------------------profile-------------------\r\n");
				//fprintf(stdout,"RTPMulticast:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Media->profile->RTPMulticast)?"Y":"N");
				proxyMedia.endpoint = tds__GetCapabilitiesResponse->Capabilities->Media->XAddr.c_str();
				m_proxyMediaEntry = tds__GetCapabilitiesResponse->Capabilities->Media->XAddr.c_str();
				
			}else{
				AddLogInfo("not support Media\n");
			}
			if (tds__GetCapabilitiesResponse->Capabilities->PTZ != NULL)
			{
				//fprintf(stdout,"\r\n-------------------PTZ-------------------\r\n");
				//fprintf(stdout,"XAddr:%s\r\n",tds__GetCapabilitiesResponse->Capabilities->PTZ->XAddr.c_str());
				proxyPTZ.endpoint = tds__GetCapabilitiesResponse->Capabilities->PTZ->XAddr.c_str();
				m_proxyPTZEntry = tds__GetCapabilitiesResponse->Capabilities->PTZ->XAddr.c_str();
				m_blSupportPTZ = true;
			}else{
				AddLogInfo("not support PTZ\n");
			}

			if (tds__GetCapabilitiesResponse->Capabilities->Events != NULL)
			{
				//fprintf(stdout,"\r\n-------------------Events-------------------\r\n");
				//fprintf(stdout,"XAddr:%s\r\n",tds__GetCapabilitiesResponse->Capabilities->Events->XAddr.c_str());
				//fprintf(stdout,"WSSubscriptionPolicySupport:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Events->WSSubscriptionPolicySupport)?"Y":"N");
				//fprintf(stdout,"WSPullPointSupport:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Events->WSPullPointSupport)?"Y":"N");
				//fprintf(stdout,"WSPausableSubscriptionManagerInterfaceSupport:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Events->WSPausableSubscriptionManagerInterfaceSupport)?"Y":"N");
				m_proxyEventEntry = tds__GetCapabilitiesResponse->Capabilities->Events->XAddr.c_str();
			}else{
				AddLogInfo("not support Event\n");
			}

			if (tds__GetCapabilitiesResponse->Capabilities->Imaging != NULL)
			{
				//fprintf(stdout,"\r\n-------------------Imaging-------------------\r\n");
				//fprintf(stdout,"XAddr:%s\r\n",tds__GetCapabilitiesResponse->Capabilities->PTZ->XAddr.c_str());
				proxyImaging.endpoint = tds__GetCapabilitiesResponse->Capabilities->Imaging->XAddr.c_str();
				m_proxyImagingEntry = tds__GetCapabilitiesResponse->Capabilities->Imaging->XAddr.c_str();
			}else{
				AddLogInfo("not support Imaging\n");
			}
		}
		else
		{
			PrintErr(proxyDevice.soap);
			AddLogInfo("GetCapabilities __tds__GetCapabilities error devip:%s\n",m_strIP.c_str());
			//return -1;
			nRet = -3;
			break;
		}

		//GetVideoSources
		if((nRet = Set_soap_wsse_para("Digest",proxyMedia.soap))!=0)
		{
			AddLogInfo("GetCapabilities Set_soap_wsse_para2 error ret:%d devip:%s\n",nRet,m_strIP.c_str());
			nRet = -4;
			break;
		}
		_trt__GetVideoSources *trt__GetVideoSources = soap_new__trt__GetVideoSources(soap,-1);
		_trt__GetVideoSourcesResponse *trt__GetVideoSourcesResponse = soap_new__trt__GetVideoSourcesResponse(soap,-1);
		if (SOAP_OK == proxyMedia.__trt__GetVideoSources(trt__GetVideoSources,trt__GetVideoSourcesResponse))
		{
			m_strVideoSourceToken = trt__GetVideoSourcesResponse->VideoSources[0]->token;
			printf("m_strVideoSourceToken:%s\n",m_strVideoSourceToken.c_str());
		}
		else
		{
			AddLogInfo("GetCapabilities __trt__GetVideoSources error devip:%s\n",m_strIP.c_str());
			PrintErr(proxyMedia.soap);
			//return -1;
			//nRet = -5;   //for景阳，此错误不处理 20161102
			//break;
		}

		//GetProfiles
		if((nRet=Set_soap_wsse_para("all",proxyMedia.soap))!=0)
		{
			AddLogInfo("GetCapabilities Set_soap_wsse_para3 error ret:%d devip:%s\n",nRet,m_strIP.c_str());
			nRet = -6;
			break;
		}
		_trt__GetProfiles *trt__GetProfiles = soap_new__trt__GetProfiles(soap,-1);
		_trt__GetProfilesResponse *trt__GetProfilesResponse = soap_new__trt__GetProfilesResponse(soap,-1);
		if (SOAP_OK == proxyMedia.__trt__GetProfiles(trt__GetProfiles,trt__GetProfilesResponse))
		{
			//fprintf(stdout,"\r\n-------------------MediaProfiles-------------------\r\n");
			for (int i = 0; i < trt__GetProfilesResponse->Profiles.size(); i++)
			{
				//fprintf(stdout,"profile%d:%s Token:%s\r\n",i,trt__GetProfilesResponse->Profiles[i]->Name.c_str(),trt__GetProfilesResponse->Profiles[i]->token.c_str());
				if(i==0)
				{
					m_strProfileToken = trt__GetProfilesResponse->Profiles[i]->token;
					printf("m_strProfileToken:%s\n",m_strProfileToken.c_str());
					if (trt__GetProfilesResponse->Profiles[i]->PTZConfiguration)
					{
						m_strPtzMoveOptionsNodeToken = trt__GetProfilesResponse->Profiles[i]->PTZConfiguration->NodeToken;
						AddLogInfo("m_strPtzMoveOptionsNodeToken:%s\n",m_strPtzMoveOptionsNodeToken.c_str());
					}
				}
			}
		}
		else
		{
			PrintErr(proxyMedia.soap);
			AddLogInfo("GetCapabilities __trt__GetProfiles error devip:%s\n",m_strIP.c_str());
			//return -1;
			//nRet = -7;
			//break;
		}
	} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::GetCapabilities do while error\n");
		nRet = -8;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	if(0 == nRet)
	{
		m_bGetCapabilitiesSuccess = true;
		printf("----GetCapabilities Success----\n");
	}
	return nRet;
}

//获取参数
int OnvifOp::GetOptions()
{
	if(m_bGetOptionsSuccess)
	{
		return 0;
	}
	CInteLock lock(&g_OnvifOpLock);
	int nRet =0;
	ImagingBinding  proxyImaging;
	try{
		do
		{
			if((nRet=GetCapabilities())!=0)  //获取参数
			{
				AddLogInfo("GetOptions GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -1;
				break;
			}
			InitProxy(NULL,NULL,&proxyImaging);
			if((nRet=Set_soap_wsse_para("all",proxyImaging.soap))!=0)
			{
				AddLogInfo("GetOptions Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -2;
				break;
			}
			//GetOptions
			_timg__GetOptions *timg__GetOptions = soap_new__timg__GetOptions(soap,-1);
			_timg__GetOptionsResponse *timg__GetOptionsResponse = soap_new__timg__GetOptionsResponse(soap,-1);
			timg__GetOptions->VideoSourceToken = m_strVideoSourceToken;
			if(SOAP_OK == proxyImaging.__timg__GetOptions(timg__GetOptions,timg__GetOptionsResponse))
			{
				if (!timg__GetOptionsResponse->ImagingOptions)
				{
					AddLogInfo("GetOptions::ImagingOptions NULL ip:%s\n",m_strIP.c_str());
					nRet = -3;
					break;
				}
				//色度暂不处理 chenyu 20151221
				if (!timg__GetOptionsResponse->ImagingOptions->Brightness 
					/*||!timg__GetOptionsResponse->ImagingOptions->Sharpness */
					||!timg__GetOptionsResponse->ImagingOptions->Contrast
					||!timg__GetOptionsResponse->ImagingOptions->ColorSaturation)
				{
					AddLogInfo("GetOptions::Video param NULL ip:%s\n",m_strIP.c_str());
					nRet = -4;
					break;
				}
				m_stOptions.nBrightnessMax = timg__GetOptionsResponse->ImagingOptions->Brightness->Max;
				m_stOptions.nBrightnessMin = timg__GetOptionsResponse->ImagingOptions->Brightness->Min;
// 				m_stOptions.nColourDegreeMax = timg__GetOptionsResponse->ImagingOptions->Sharpness->Max;
// 				m_stOptions.nColourDegreeMin = timg__GetOptionsResponse->ImagingOptions->Sharpness->Min;
				m_stOptions.nContrastMax = timg__GetOptionsResponse->ImagingOptions->Contrast->Max;
				m_stOptions.nContrastMin = timg__GetOptionsResponse->ImagingOptions->Contrast->Min;
				m_stOptions.nSaturationMax = timg__GetOptionsResponse->ImagingOptions->ColorSaturation->Max;
				m_stOptions.nSaturationMin = timg__GetOptionsResponse->ImagingOptions->ColorSaturation->Min;
				m_stOptions.nBrightnessRange = m_stOptions.nBrightnessMax - m_stOptions.nBrightnessMin;
				/*m_stOptions.nColourDegreeRange = m_stOptions.nColourDegreeMax - m_stOptions.nColourDegreeMin;*/
				m_stOptions.nContrastRange = m_stOptions.nContrastMax - m_stOptions.nContrastMin;
				m_stOptions.nSaturationRange = m_stOptions.nSaturationMax - m_stOptions.nSaturationMin;
				char szTmp[1024]={0};
				sprintf(szTmp,"nBrightnessMax:%d %d nContrastMax:%d %d nSaturationMax:%d %d",
					m_stOptions.nBrightnessMax, m_stOptions.nBrightnessMin, 
					/*m_stOptions.nColourDegreeMax,  m_stOptions.nColourDegreeMin,*/
					m_stOptions.nContrastMax, m_stOptions.nContrastMin,
					m_stOptions.nSaturationMax,  m_stOptions.nSaturationMin);
				AddLogInfo(szTmp);
				m_bGetOptionsSuccess = true;  //获取成功
			}
			else
			{
				PrintErr(proxyImaging.soap);
				AddLogInfo("GetOptions __timg__GetImagingSettings error devip:%s\n",m_strIP.c_str());
				nRet = -5;
				break;
			}
		} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::GetVideoParam do while error\n");
		nRet = -6;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

int OnvifOp::GetMoveOptions()
{
	if(m_bGetMoveOptionsSuccess)
	{
		return 0;
	}
	CInteLock lock(&g_OnvifOpLock);
	int nRet =0;
	ImagingBinding  proxyImaging;
	try{
		do
		{
			if((nRet=GetCapabilities())!=0)  //获取参数
			{
				AddLogInfo("GetMoveOptions GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -1;
				break;
			}
			InitProxy(NULL,NULL,&proxyImaging);
			if((nRet=Set_soap_wsse_para("all",proxyImaging.soap))!=0)
			{
				AddLogInfo("GetMoveOptions Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -2;
				break;
			}
			//GetOptions
			_timg__GetMoveOptions *timg__GetMoveOptions = soap_new__timg__GetMoveOptions(soap,-1);
			_timg__GetMoveOptionsResponse *timg__GetMoveOptionsResponse = soap_new__timg__GetMoveOptionsResponse(soap,-1);
			timg__GetMoveOptions->VideoSourceToken = m_strVideoSourceToken;
			if(SOAP_OK == proxyImaging.__timg__GetMoveOptions(timg__GetMoveOptions,timg__GetMoveOptionsResponse))
			{
				if (!timg__GetMoveOptionsResponse->MoveOptions)
				{
					nRet = -3;
					break;
				}
				if (!timg__GetMoveOptionsResponse->MoveOptions->Continuous
					||!timg__GetMoveOptionsResponse->MoveOptions->Continuous->Speed)
				{
					nRet = -4;
					break;
				}
				m_stMoveOptions.nContinuousMax = timg__GetMoveOptionsResponse->MoveOptions->Continuous->Speed->Max;
				m_stMoveOptions.nContinuousMin = timg__GetMoveOptionsResponse->MoveOptions->Continuous->Speed->Min;
				char szTmp[1024]={0};
				sprintf(szTmp,"nContinuousSpeed:%d %d",m_stMoveOptions.nContinuousMax,m_stMoveOptions.nContinuousMin);
				AddLogInfo(szTmp);
				m_bGetMoveOptionsSuccess = true;  //获取成功
			}
			else
			{
				PrintErr(proxyImaging.soap);
				AddLogInfo("GetMoveOptions __timg__GetMoveOptions error devip:%s\n",m_strIP.c_str());
				nRet = -5;
				break;
			}
		} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::GetVideoParam do while error\n");
		nRet = -6;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

int OnvifOp::SetFocusMode(tt__AutoFocusMode eFocusMode)
{
	CInteLock lock(&g_OnvifOpLock);
	int nRet =0;
	ImagingBinding  proxyImaging;
	try{
		do
		{
			if((nRet=GetCapabilities())!=0)  //获取参数
			{
				AddLogInfo("SetFocusMode GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -1;
				break;
			}
			InitProxy(NULL,NULL,&proxyImaging);
			if((nRet=Set_soap_wsse_para("all",proxyImaging.soap))!=0)
			{
				AddLogInfo("SetFocusMode Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -2;
				break;
			}
			//SetImagingSettings
			_timg__SetImagingSettings *timg__SetImagingSettings = soap_new__timg__SetImagingSettings(soap,-1);
			_timg__SetImagingSettingsResponse *timg__SetImagingSettingsResponse = soap_new__timg__SetImagingSettingsResponse(soap,-1);
			timg__SetImagingSettings->VideoSourceToken = m_strVideoSourceToken;
			timg__SetImagingSettings->ImagingSettings = soap_new_tt__ImagingSettings20(soap,-1);
			timg__SetImagingSettings->ImagingSettings->Focus = soap_new_tt__FocusConfiguration20(soap,-1);
		    timg__SetImagingSettings->ImagingSettings->Focus->AutoFocusMode = eFocusMode;
			if(SOAP_OK == proxyImaging.__timg__SetImagingSettings(timg__SetImagingSettings,timg__SetImagingSettingsResponse))
			{
				AddLogInfo("__timg__SetImagingSettings FocusMode success:%d",eFocusMode);
			}
			else
			{
				PrintErr(proxyImaging.soap);
				AddLogInfo("SetFocusMode __timg__SetImagingSettings error, devip:%s\n",m_strIP.c_str());
				//return -1;
				nRet = -3;
				break;
			}
		} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::SetVideoParam do while error\n");
		nRet = -4;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

int OnvifOp::GetPtzConfigurationOptions()
{
	if(m_bGetPtzMoveOptionsSuccess)
	{
		return 0;
	}
	CInteLock lock(&g_OnvifOpLock);
	int nRet =0;
	ImagingBinding  proxyImaging;
	PTZBinding      proxyPTZ;
	try{
		do
		{
			if((nRet=GetCapabilities())!=0)  //获取参数
			{
				AddLogInfo("GetPtzConfigurationOptions GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -1;
				break;
			}
			InitProxy(NULL,NULL,&proxyImaging,&proxyPTZ);
			if((nRet=Set_soap_wsse_para("all",proxyImaging.soap))!=0)
			{
				AddLogInfo("GetPtzConfigurationOptions Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -2;
				break;
			}
			//GetOptions
			_tptz__GetConfigurationOptions *tptz__GetConfigurationOptions = soap_new__tptz__GetConfigurationOptions(soap,-1);
			_tptz__GetConfigurationOptionsResponse *tptz__GetConfigurationOptionsResponse = soap_new__tptz__GetConfigurationOptionsResponse(soap,-1);
			tptz__GetConfigurationOptions->ConfigurationToken = m_strPtzMoveOptionsNodeToken;
			if(SOAP_OK == proxyPTZ.__tptz__GetConfigurationOptions(tptz__GetConfigurationOptions,tptz__GetConfigurationOptionsResponse))
			{
				if (tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions
					&& tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces
					&& tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace[0]
					&& tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousZoomVelocitySpace[0])
				{
					tt__Space2DDescription * pSpace2D = tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousPanTiltVelocitySpace[0];  //取第一个
					tt__Space1DDescription * pSpace1D = tptz__GetConfigurationOptionsResponse->PTZConfigurationOptions->Spaces->ContinuousZoomVelocitySpace[0];

					if (pSpace2D->XRange
						&& pSpace2D->YRange
						&& pSpace1D->XRange)
					{
						m_stPtzMoveOptions.nPanMax = pSpace2D->XRange->Max;
					    m_stPtzMoveOptions.nPanMin = pSpace2D->XRange->Min;
						m_stPtzMoveOptions.nTiltMax = pSpace2D->YRange->Max;
					    m_stPtzMoveOptions.nTiltMin = pSpace2D->YRange->Min;
						m_stPtzMoveOptions.nZoomMax = pSpace1D->XRange->Max;
					    m_stPtzMoveOptions.nZoomMin = pSpace1D->XRange->Min;
						char szTmp[1024]={0};
						sprintf(szTmp,"get PtzMoveOptions success:%d %d %d %d %d %d %d %d %d",
							m_stPtzMoveOptions.nPanMax,m_stPtzMoveOptions.nPanMin,
							m_stPtzMoveOptions.nTiltMax,m_stPtzMoveOptions.nTiltMin,
							m_stPtzMoveOptions.nZoomMax, m_stPtzMoveOptions.nZoomMin,
							m_stPtzMoveOptions.nPanRange,m_stPtzMoveOptions.nTiltRange,m_stPtzMoveOptions.nZoomRange);
						AddLogInfo(szTmp);
					}
				}
				m_stPtzMoveOptions.nPanRange = m_stPtzMoveOptions.nPanMax - m_stPtzMoveOptions.nPanMin;
				m_stPtzMoveOptions.nTiltRange = m_stPtzMoveOptions.nTiltMax - m_stPtzMoveOptions.nTiltMin;
				m_stPtzMoveOptions.nZoomRange = m_stPtzMoveOptions.nZoomMax - m_stPtzMoveOptions.nZoomMin;
				if (m_stPtzMoveOptions.nPanRange<=0)
				{
					m_stPtzMoveOptions.nPanRange = 2;
				}
				if (m_stPtzMoveOptions.nTiltRange<=0)
				{
					m_stPtzMoveOptions.nTiltRange = 2;
				}
				if (m_stPtzMoveOptions.nZoomRange<=0)
				{
					m_stPtzMoveOptions.nZoomRange = 2;
				}
				char szTmp[1024]={0};
				sprintf(szTmp,"m_stPtzMoveOptions:%d %d %d %d %d %d %d %d %d",
					m_stPtzMoveOptions.nPanMax,m_stPtzMoveOptions.nPanMin,
					m_stPtzMoveOptions.nTiltMax,m_stPtzMoveOptions.nTiltMin,
					m_stPtzMoveOptions.nZoomMax, m_stPtzMoveOptions.nZoomMin,
					m_stPtzMoveOptions.nPanRange,m_stPtzMoveOptions.nTiltRange,m_stPtzMoveOptions.nZoomRange);
				AddLogInfo(szTmp);
				m_bGetPtzMoveOptionsSuccess = true;  //获取成功
			}
			else
			{
				PrintErr(proxyPTZ.soap);
#ifdef LANG_EN
#else
				AddLogInfo("使用默认值 __tptz__GetConfigurationOptions error,line:%d devip:%s\n",__LINE__,m_strIP.c_str());
#endif
				//nRet = -1;
				nRet = 0;
				m_bGetPtzMoveOptionsSuccess = true;  //获取成功 失败用默认值 因此也标志为成功
				break;
			}
		} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::GetPtzConfigurationOptions do while error\n");
		nRet = -3;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

int OnvifOp::GetEventProperties()
{
	if(m_bGetEventPropertiesSuccess)
	{
		return 0;
	}
	CInteLock lock(&g_OnvifOpLock);
	int nRet =0;
	EventBinding  proxyEvent;
	try{
		do
		{
// 			if((nRet=GetCapabilities())!=0)  //获取参数
// 			{
// 				AddLogInfo("GetEventProperties GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
// 				nRet = -1;
// 				break;
// 			}
			InitProxy(NULL,NULL,NULL,NULL,&proxyEvent);
			if((nRet=Set_soap_wsse_para("all",proxyEvent.soap))!=0)
			{
				AddLogInfo("GetEventProperties Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -2;
				break;
			}
			//GetEventProperties
			_tev__GetEventProperties *tev__GetEventProperties = soap_new__tev__GetEventProperties(soap,-1);
			_tev__GetEventPropertiesResponse *tev__GetEventPropertiesResponse = soap_new__tev__GetEventPropertiesResponse(soap,-1);
			if(proxyEvent.__ns11__GetEventProperties(tev__GetEventProperties,tev__GetEventPropertiesResponse) == SOAP_OK)
			{
				//fprintf(stdout,"\r\n-------------------EventProperties-------------------\r\n");
				for (int i = 0;i < tev__GetEventPropertiesResponse->TopicNamespaceLocation.size();i++)
				{
					//fprintf(stdout,"TopicNamespaceLocation[%d]:%s\r\n",i,tev__GetEventPropertiesResponse->TopicNamespaceLocation[i].c_str());
				}
				
				//fprintf(stdout,"FixedTopicSet:%s\r\n",(tev__GetEventPropertiesResponse->ns1__FixedTopicSet)?"Y":"N");
				for (int i = 0;i < tev__GetEventPropertiesResponse->ns1__TopicExpressionDialect.size();i++)
				{
					//fprintf(stdout,"TopicExpressionDialect[%d]:%s\r\n",i,tev__GetEventPropertiesResponse->ns1__TopicExpressionDialect[i].c_str());
					m_strTopicExpressionDialect = tev__GetEventPropertiesResponse->ns1__TopicExpressionDialect[0];
					break;
				}
				
				for (int i = 0;i < tev__GetEventPropertiesResponse->MessageContentFilterDialect.size();i++)
				{
					//fprintf(stdout,"MessageContentFilterDialect[%d]:%s\r\n",i,tev__GetEventPropertiesResponse->MessageContentFilterDialect[i].c_str());
					m_strFilterDialect = tev__GetEventPropertiesResponse->MessageContentFilterDialect[0];
					break;
				}
				
				for (int i = 0;i < tev__GetEventPropertiesResponse->MessageContentSchemaLocation.size();i++)
				{
					//fprintf(stdout,"MessageContentSchemaLocation[%d]:%s\r\n",i,tev__GetEventPropertiesResponse->MessageContentSchemaLocation[i].c_str());
				}
				m_bGetEventPropertiesSuccess = true;  //获取成功
			}
			else
			{
				//PrintErr(proxyEvent.soap);
				AddLogInfo("GetEventProperties __ns11__GetEventProperties error devip:%s\n",m_strIP.c_str());
				nRet = -3;
				break;
			}
		} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::GetEventProperties do while error\n");
		nRet = -4;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

int OnvifOp::CreatePullPoint(string& strSubscriptionEntry)
{
	int nRet =0;
// 	if((nRet=GetEventProperties())!=0)  //获取参数
// 	{
// 		AddLogInfo("CreatePullPoint GetEventProperties error ret:%d devip:%s\n",nRet,m_strIP.c_str());
// 		return -1;
// 	}

	CInteLock lock(&g_OnvifOpLock);
	EventBinding proxyEvent;
	try{
		do
		{
			if((nRet=GetCapabilities())!=0)  //获取参数
			{
				AddLogInfo("CreatePullPoint GetCapabilities error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -2;
				break;
			}
			InitProxy(NULL,NULL,NULL,NULL,&proxyEvent);
			if((nRet=Set_soap_wsse_para("all",proxyEvent.soap))!=0)
			{
				AddLogInfo("CreatePullPoint Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -3;
				break;
			}
			_tev__CreatePullPointSubscription *tev__CreatePullPointSubscription = soap_new__tev__CreatePullPointSubscription(soap,-1);
			string *p = new string;
			*p = "PT1000S";
			tev__CreatePullPointSubscription->InitialTerminationTime = p;
//			tev__CreatePullPointSubscription->Filter = soap_new_ns1__FilterType(soap,-1);
// 			char *pszExp = new char[100];
// 			sprintf(pszExp,"boolean(//tt:SimpleItem[@Name=\"IsMotion\" and @Value=\"true\"] )");
// 			tev__CreatePullPointSubscription->Filter->__any.push_back(pszExp);
			_tev__CreatePullPointSubscriptionResponse *tev__CreatePullPointSubscriptionResponse = soap_new__tev__CreatePullPointSubscriptionResponse(soap,-1);
			if (SOAP_OK == proxyEvent.__ns11__CreatePullPointSubscription(tev__CreatePullPointSubscription,tev__CreatePullPointSubscriptionResponse))
			{
				AddLogInfo("SubScription:%s",tev__CreatePullPointSubscriptionResponse->SubscriptionReference.Address);
				strSubscriptionEntry =  tev__CreatePullPointSubscriptionResponse->SubscriptionReference.Address;
			}
			else
			{
				PrintErr(proxyEvent.soap);
				AddLogInfo("CreatePullPoint __ns11__CreatePullPointSubscription error devip:%s\n",m_strIP.c_str());
				nRet = -4;
				break;
			}
		} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::CreatePullPoint do while error\n");
		nRet = -5;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	return nRet;
}

int OnvifOp::PullMessages(string strEntry,string& strMsg,int& nAlarmType,int& nChannelMark)
{
	struct soap *soap=NULL;
	soap = soap_new();
	if (!soap)
	{
		return -1;
	}
	strMsg = "";
	int nRet =0;
	PullPointSubscriptionBinding proxyPS;
	proxyPS.endpoint = strEntry.c_str();
	try{
		do
		{
			if((nRet=Set_soap_wsse_para("all",proxyPS.soap))!=0)
			{
				AddLogInfo("PullMessages Set_soap_wsse_para error ret:%d devip:%s\n",nRet,m_strIP.c_str());
				nRet = -2;
				break;
			}
			
			_tev__PullMessages *tev__PullMessages = soap_new__tev__PullMessages(soap,-1);  
			_tev__PullMessagesResponse *tev__PullMessagesResponse = soap_new__tev__PullMessagesResponse(soap,-1);
			tev__PullMessages->MessageLimit = 1;
			tev__PullMessages->Timeout = 40*1000;
			if(proxyPS.__ns10__PullMessages(tev__PullMessages,tev__PullMessagesResponse) == SOAP_OK)
			{
				if(tev__PullMessagesResponse->ns1__NotificationMessage.size()>0)
				{
					strMsg = tev__PullMessagesResponse->ns1__NotificationMessage[0]->Message.__any;
					int nPos = strMsg.find("Name=\"IsMotion\" Value=\"true\"");
					//AddLogInfo("\n\r-----:::::::\n\r%s",strMsg.c_str());
					if(nPos !=-1)
					{
						//AddLogInfo("-----------IsMotion");
						strMsg = "Motion_true";
					}else{
						//AddLogInfo("-----no Motion---");
					}
				}	
			}
			else
			{
				PrintErr(proxyPS.soap);
				AddLogInfo("PullMessages __ns10__PullMessages error,devip:%s\n",m_strIP.c_str());
				nRet = -3;
				break;
			}
		} while (0);
	}catch(...)
	{
		AddLogInfo("OnvifOp::PullMessages do while error\n");
		nRet = -4;
	}
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data
	soap_free(soap);//detach and free runtime context
	return nRet;
}

int OnvifOp::InitProxy(DeviceBinding*  pproxyDevice,
					   MediaBinding*   pproxyMedia,
					   ImagingBinding* pproxyImaging,
					   PTZBinding*     pproxyPTZ,
					   EventBinding*   pproxyEvent,  
					   int iTimeout)
{
	//清空soap
	// 	soap_destroy(soap); // remove deserialized class instances (C++ only)
	// 	soap_end(soap); // clean up and remove deserialized data
	// 
	// 	soap_destroy(proxyDevice.soap); // remove deserialized class instances (C++ only)
	// 	soap_end(proxyDevice.soap); // clean up and remove deserialized data
	// 	soap_destroy(proxyMedia.soap); // remove deserialized class instances (C++ only)
	// 	soap_end(proxyMedia.soap); // clean up and remove deserialized data
	// 	soap_destroy(proxyImaging.soap); // remove deserialized class instances (C++ only)
	// 	soap_end(proxyImaging.soap); // clean up and remove deserialized data
	// 	soap_destroy(proxyPTZ.soap); // remove deserialized class instances (C++ only)
	// 	soap_end(proxyPTZ.soap); // clean up and remove deserialized data
	//soap_destroy(proxyEvent.soap); // remove deserialized class instances (C++ only)
	//soap_end(proxyEvent.soap); // clean up and remove deserialized data
	//soap_destroy(proxyNP.soap); // remove deserialized class instances (C++ only)
	//soap_end(proxyNP.soap); // clean up and remove deserialized data
	if (pproxyDevice)
	{
		pproxyDevice->endpoint = szHostName;
		//proxyDevice.endpoint = "http://172.18.13.153/onvif/device_service";
		soap_register_plugin(pproxyDevice->soap,soap_wsse);
	}
	if (pproxyMedia)
	{
		soap_register_plugin(pproxyMedia->soap,soap_wsse);
		pproxyMedia->endpoint = m_proxyMediaEntry.c_str();
	}
	if (pproxyImaging)
	{
		soap_register_plugin(pproxyImaging->soap,soap_wsa);
		pproxyImaging->endpoint = m_proxyImagingEntry.c_str();
	}
	if (pproxyPTZ)
	{
		soap_register_plugin(pproxyPTZ->soap,soap_wsse);
		pproxyPTZ->endpoint = m_proxyPTZEntry.c_str();
	}
	if (pproxyEvent)
	{
		soap_register_plugin(pproxyEvent->soap,soap_wsse);
		pproxyEvent->endpoint = m_proxyEventEntry.c_str();
	}
	//proxyEvent.endpoint = m_proxyEventEntry.c_str();
	//proxyNP.endpoint = m_proxyNPEntry.c_str();
	
	if (iTimeout > 0)
	{
		if (pproxyDevice)
		{
			pproxyDevice->soap->recv_timeout = iTimeout;
			pproxyDevice->soap->send_timeout = iTimeout;
			pproxyDevice->soap->connect_timeout = iTimeout;
		}
		
		if (pproxyMedia)
		{
			pproxyMedia->soap->recv_timeout = iTimeout;
			pproxyMedia->soap->send_timeout = iTimeout;
			pproxyMedia->soap->connect_timeout = iTimeout;
		}
		
		if (pproxyImaging)
		{
			pproxyImaging->soap->recv_timeout = iTimeout;
			pproxyImaging->soap->send_timeout = iTimeout;
			pproxyImaging->soap->connect_timeout = iTimeout;
		}
		
		if (pproxyPTZ)
		{
			pproxyPTZ->soap->recv_timeout = iTimeout;
			pproxyPTZ->soap->send_timeout = iTimeout;
			pproxyPTZ->soap->connect_timeout = iTimeout;
		}
		
		if (pproxyEvent)
		{
			pproxyEvent->soap->recv_timeout = iTimeout;
			pproxyEvent->soap->send_timeout = iTimeout;
		    pproxyEvent->soap->connect_timeout = iTimeout;
		}
	
		//proxyNP.soap->recv_timeout = iTimeout;
		//proxyNP.soap->send_timeout = iTimeout;
		//proxyNP.soap->connect_timeout = iTimeout;
	}
	else
	{
		//如果外部接口没有设备默认超时时间的话，我这里给了一个默认值10s
		if (pproxyDevice)
		{
			pproxyDevice->soap->recv_timeout    = 4;
			pproxyDevice->soap->send_timeout    = 4;
			pproxyDevice->soap->connect_timeout = 4;
		}
		
		if (pproxyMedia)
		{
			pproxyMedia->soap->recv_timeout    = 4;
			pproxyMedia->soap->send_timeout    = 4;
			pproxyMedia->soap->connect_timeout = 4;
		}
		
		if (pproxyImaging)
		{
			pproxyImaging->soap->recv_timeout    = 4;
			pproxyImaging->soap->send_timeout    = 4;
			pproxyImaging->soap->connect_timeout = 4;
		}
		
		if (pproxyPTZ)
		{
			pproxyPTZ->soap->recv_timeout    = 4;
			pproxyPTZ->soap->send_timeout    = 4;
			pproxyPTZ->soap->connect_timeout = 4;
		}
		
		if (pproxyEvent)
		{
			pproxyEvent->soap->recv_timeout = 4;
			pproxyEvent->soap->send_timeout = 4;
			pproxyEvent->soap->connect_timeout = 4;
		}
		//proxyNP.soap->recv_timeout    = 4;
		//proxyNP.soap->send_timeout    = 4;
		//proxyNP.soap->connect_timeout = 4;
	}
	
	return 0;
}

//完整例子程序
int full_example_test(char*szUsr,char*szPwd)
{
	string m_strUsr = szUsr;
	string m_strPwd = szPwd;
	
	//cout<<SIP_Init()<<endl;

	bool blSupportPTZ = false;
	char szHostName[MAX_HOSTNAME_LEN] = {0};

	DeviceBinding proxyDevice;

	MediaBinding proxyMedia;

	PTZBinding proxyPTZ;

	EventBinding proxyEvent;
	NotificationProducerBinding proxyNP;

//	if (argc > 1)
//	{
//		strcat(szHostName,"http://");
//		strcat(szHostName,argv[1]);
//		strcat(szHostName,"/onvif/device_service");
//
//		proxyDevice.endpoint = szHostName;
//	}
//	else
//	{
//		proxyDevice.endpoint = "http://172.18.13.22/onvif/device_service";
//	}

	proxyDevice.endpoint = "http://172.18.13.197/onvif/device_service";

	soap_register_plugin(proxyDevice.soap,soap_wsse);
	soap_register_plugin(proxyMedia.soap,soap_wsse);
	soap_register_plugin(proxyPTZ.soap,soap_wsse);
	soap_register_plugin(proxyEvent.soap,soap_wsse);
	soap_register_plugin(proxyNP.soap,soap_wsse);

	soap_register_plugin(proxyEvent.soap,soap_wsa);
	soap_register_plugin(proxyNP.soap,soap_wsa);

	struct soap *soap = soap_new();

	//if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyDevice.soap, NULL, "admin", DEV_PASSWORD))
	if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyDevice.soap, NULL, m_strUsr.c_str(), m_strPwd.c_str()))
	{
		return -1;
	}

	if(SOAP_OK != soap_wsse_add_Timestamp(proxyDevice.soap, "Time", 10)) // 10 seconds lifetime
	{
		return -1;
	}

	//Get WSDL URL
	_tds__GetWsdlUrl *tds__GetWsdlUrl = soap_new__tds__GetWsdlUrl(soap,-1);
	_tds__GetWsdlUrlResponse *tds__GetWsdlUrlResponse = soap_new__tds__GetWsdlUrlResponse(soap,-1);

	if(SOAP_OK == proxyDevice.__tds__GetWsdlUrl(tds__GetWsdlUrl,tds__GetWsdlUrlResponse))
	{
		//fflush(stdout);
		fprintf(stdout,"-------------------WsdlUrl-------------------\r\n");
		fprintf(stdout,"WsdlUrl:%s\r\n ",tds__GetWsdlUrlResponse->WsdlUrl.c_str());
	}
	else
	{
		PrintErr(proxyDevice.soap);
	}

	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data

 	if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyDevice.soap, NULL, "admin", DEV_PASSWORD))
 	{
 		return -1;
 	}

	//Capability exchange
	_tds__GetCapabilities *tds__GetCapabilities = soap_new__tds__GetCapabilities(soap,-1);
	tds__GetCapabilities->Category.push_back(tt__CapabilityCategory__All);

	_tds__GetCapabilitiesResponse *tds__GetCapabilitiesResponse = soap_new__tds__GetCapabilitiesResponse(soap,-1);

	if(SOAP_OK == proxyDevice.__tds__GetCapabilities(tds__GetCapabilities,tds__GetCapabilitiesResponse))
	{
		//fflush(stdout);

		if(tds__GetCapabilitiesResponse->Capabilities->Analytics != NULL)
		{
			fprintf(stdout,"\r\n-------------------Analytics-------------------\r\n");
			fprintf(stdout,"XAddr:%s\r\n",tds__GetCapabilitiesResponse->Capabilities->Analytics->XAddr.c_str());
			fprintf(stdout,"RuleSupport:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Analytics->RuleSupport)?"Y":"N");
			fprintf(stdout,"AnalyticsModuleSupport:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Analytics->AnalyticsModuleSupport)?"Y":"N");
		}

		if (tds__GetCapabilitiesResponse->Capabilities->Device != NULL)
		{
			fprintf(stdout,"\r\n-------------------Device-------------------\r\n");
			fprintf(stdout,"XAddr:%s\r\n",tds__GetCapabilitiesResponse->Capabilities->Device->XAddr.c_str());

			fprintf(stdout,"\r\n-------------------Network-------------------\r\n");
			fprintf(stdout,"IPFilter:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->Network->IPFilter)?"Y":"N");
			fprintf(stdout,"ZeroConfiguration:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->Network->ZeroConfiguration)?"Y":"N");
			fprintf(stdout,"IPVersion6:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->Network->IPVersion6)?"Y":"N");
			fprintf(stdout,"DynDNS:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->Network->DynDNS)?"Y":"N");

			fprintf(stdout,"\r\n-------------------System-------------------\r\n");
			fprintf(stdout,"DiscoveryResolve:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->System->DiscoveryResolve)?"Y":"N");
			fprintf(stdout,"DiscoveryBye:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->System->DiscoveryBye)?"Y":"N");
			fprintf(stdout,"RemoteDiscovery:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->System->RemoteDiscovery)?"Y":"N");

			int iSize = tds__GetCapabilitiesResponse->Capabilities->Device->System->SupportedVersions.size();

			if(iSize > 0)
			{
				fprintf(stdout,"SupportedVersions:");

				for(int i = 0;i < iSize; i++)
				{
					fprintf(stdout,"%d.%d ",tds__GetCapabilitiesResponse->Capabilities->Device->System->SupportedVersions[i]->Major,tds__GetCapabilitiesResponse->Capabilities->Device->System->SupportedVersions[i]->Minor);
				}

				fprintf(stdout,"\r\n");
			}

			fprintf(stdout,"SystemBackup:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->System->SystemBackup)?"Y":"N");
			fprintf(stdout,"FirmwareUpgrade:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->System->FirmwareUpgrade)?"Y":"N");
			fprintf(stdout,"SystemLogging:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->System->SystemLogging)?"Y":"N");

			fprintf(stdout,"\r\n-------------------IO-------------------\r\n");
			fprintf(stdout,"InputConnectors:%d\r\n",tds__GetCapabilitiesResponse->Capabilities->Device->IO->InputConnectors);
			fprintf(stdout,"RelayOutputs:%d\r\n",tds__GetCapabilitiesResponse->Capabilities->Device->IO->RelayOutputs);

			fprintf(stdout,"\r\n-------------------Security-------------------\r\n");
			fprintf(stdout,"TLS1.1:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->Security->TLS1_x002e1)?"Y":"N");
			fprintf(stdout,"TLS1.2:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->Security->TLS1_x002e2)?"Y":"N");
			fprintf(stdout,"OnboardKeyGeneration:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->Security->OnboardKeyGeneration)?"Y":"N");
			fprintf(stdout,"AccessPolicyConfig:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->Security->AccessPolicyConfig)?"Y":"N");
			fprintf(stdout,"X.509Token:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->Security->X_x002e509Token)?"Y":"N");
			fprintf(stdout,"SAMLToken:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->Security->SAMLToken)?"Y":"N");
			fprintf(stdout,"KerberosToken:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->Security->KerberosToken)?"Y":"N");
			fprintf(stdout,"RELToken:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Device->Security->RELToken)?"Y":"N");
		}

		if (tds__GetCapabilitiesResponse->Capabilities->Events != NULL)
		{
			fprintf(stdout,"\r\n-------------------Events-------------------\r\n");
			fprintf(stdout,"XAddr:%s\r\n",tds__GetCapabilitiesResponse->Capabilities->Events->XAddr.c_str());
			fprintf(stdout,"WSSubscriptionPolicySupport:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Events->WSSubscriptionPolicySupport)?"Y":"N");
			fprintf(stdout,"WSPullPointSupport:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Events->WSPullPointSupport)?"Y":"N");
			fprintf(stdout,"WSPausableSubscriptionManagerInterfaceSupport:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Events->WSPausableSubscriptionManagerInterfaceSupport)?"Y":"N");

			proxyEvent.endpoint = tds__GetCapabilitiesResponse->Capabilities->Events->XAddr.c_str();
		}

		if (tds__GetCapabilitiesResponse->Capabilities->Imaging != NULL)
		{
			fprintf(stdout,"\r\n-------------------Imaging-------------------\r\n");
			fprintf(stdout,"XAddr:%s\r\n",tds__GetCapabilitiesResponse->Capabilities->Imaging->XAddr.c_str());
		}

		if (tds__GetCapabilitiesResponse->Capabilities->Media != NULL)
		{
			fprintf(stdout,"\r\n-------------------Media-------------------\r\n");
			fprintf(stdout,"XAddr:%s\r\n",tds__GetCapabilitiesResponse->Capabilities->Media->XAddr.c_str());

			fprintf(stdout,"\r\n-------------------streaming-------------------\r\n");
			fprintf(stdout,"RTPMulticast:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTPMulticast)?"Y":"N");
			fprintf(stdout,"RTP_TCP:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTP_USCORETCP)?"Y":"N");
			fprintf(stdout,"RTP_RTSP_TCP:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Media->StreamingCapabilities->RTP_USCORERTSP_USCORETCP)?"Y":"N");

			//fprintf(stdout,"\r\n-------------------profile-------------------\r\n");
			//fprintf(stdout,"RTPMulticast:%s\r\n",(tds__GetCapabilitiesResponse->Capabilities->Media->profile->RTPMulticast)?"Y":"N");
			proxyMedia.endpoint = tds__GetCapabilitiesResponse->Capabilities->Media->XAddr.c_str();
		}

		if (tds__GetCapabilitiesResponse->Capabilities->PTZ != NULL)
		{
			fprintf(stdout,"\r\n-------------------PTZ-------------------\r\n");
			fprintf(stdout,"XAddr:%s\r\n",tds__GetCapabilitiesResponse->Capabilities->PTZ->XAddr.c_str());

			proxyPTZ.endpoint = tds__GetCapabilitiesResponse->Capabilities->PTZ->XAddr.c_str();
			blSupportPTZ = true;
		}
	}
	else
	{
		PrintErr(proxyDevice.soap);
	}

	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data

	//Get Device Information
	_tds__GetDeviceInformation *tds__GetDeviceInformation = soap_new__tds__GetDeviceInformation(soap,-1);
	_tds__GetDeviceInformationResponse *tds__GetDeviceInformationResponse = soap_new__tds__GetDeviceInformationResponse(soap,-1);

	if(SOAP_OK == proxyDevice.__tds__GetDeviceInformation(tds__GetDeviceInformation,tds__GetDeviceInformationResponse))
	{
		//fflush(stdout);
		fprintf(stdout,"\r\n-------------------DeviceInformation-------------------\r\n");
		fprintf(stdout,"Manufacturer:%s\r\nModel:%s\r\nFirmwareVersion:%s\r\nSerialNumber:%s\r\nHardwareId:%s\r\n",tds__GetDeviceInformationResponse->Manufacturer.c_str(),
			tds__GetDeviceInformationResponse->Model.c_str(),tds__GetDeviceInformationResponse->FirmwareVersion.c_str(),
			tds__GetDeviceInformationResponse->SerialNumber.c_str(),tds__GetDeviceInformationResponse->HardwareId.c_str());
	}
	else
	{
		PrintErr(proxyDevice.soap);
	}

	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data

	if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyDevice.soap, NULL,m_strUsr.c_str(), m_strPwd.c_str()))
	{
		return -1;
	}

	//GetNetworkInterfaces
	_tds__GetNetworkInterfaces *tds__GetNetworkInterfaces = soap_new__tds__GetNetworkInterfaces(soap,-1);
	_tds__GetNetworkInterfacesResponse *tds__GetNetworkInterfacesResponse = soap_new__tds__GetNetworkInterfacesResponse(soap,-1);

	if (SOAP_OK == proxyDevice.__tds__GetNetworkInterfaces(tds__GetNetworkInterfaces,tds__GetNetworkInterfacesResponse))
	{
		fprintf(stdout,"\r\n-------------------NetworkInterfaces-------------------\r\n");
		fprintf(stdout,"token:%s\r\nMAC:%s\r\nIPv4:%s\r\n",tds__GetNetworkInterfacesResponse->NetworkInterfaces[0]->token.c_str(),
			tds__GetNetworkInterfacesResponse->NetworkInterfaces[0]->Info->HwAddress.c_str(),tds__GetNetworkInterfacesResponse->NetworkInterfaces[0]->IPv4->Config->Manual[0]->Address.c_str());
	}
	else
	{
		PrintErr(proxyDevice.soap);
	}

	soap_destroy(soap);
	soap_end(soap);

	//SetNetworkInterfaces
	/*_tds__SetNetworkInterfaces *tds__SetNetworkInterfaces = soap_new__tds__SetNetworkInterfaces(soap,-1);
	_tds__SetNetworkInterfacesResponse *tds__SetNetworkInterfacesResponse = soap_new__tds__SetNetworkInterfacesResponse(soap,-1);

	tds__SetNetworkInterfaces->InterfaceToken = tds__GetNetworkInterfacesResponse->NetworkInterfaces[0]->token;
	tds__SetNetworkInterfaces->NetworkInterface = soap_new_tt__NetworkInterfaceSetConfiguration(soap,-1);
	tds__SetNetworkInterfaces->NetworkInterface->IPv4 = soap_new_tt__IPv4NetworkInterfaceSetConfiguration(soap,-1);
	tds__SetNetworkInterfaces->NetworkInterface->IPv4->Manual.push_back(tds__GetNetworkInterfacesResponse->NetworkInterfaces[0]->IPv4->Config->Manual[0]);
	tds__SetNetworkInterfaces->NetworkInterface->IPv4->Manual[0]->Address = "192.168.1.109";
	tds__SetNetworkInterfaces->NetworkInterface->IPv4->Enabled = new bool(true);
	tds__SetNetworkInterfaces->NetworkInterface->IPv4->DHCP = new bool(false);

	if (SOAP_OK == proxyDevice.__tds__SetNetworkInterfaces(tds__SetNetworkInterfaces,tds__SetNetworkInterfacesResponse))
	{

	}
	else
	{
		PrintErr(proxyDevice.soap);
	}

	soap_destroy(soap);
	soap_end(soap);*/

	if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyMedia.soap, NULL, m_strUsr.c_str(), m_strPwd.c_str()))
	{
		return -1;
	}

	if(SOAP_OK != soap_wsse_add_Timestamp(proxyMedia.soap, "Time", 10)) // 10 seconds lifetime
	{
		return -1;
	}

	_trt__GetProfiles *trt__GetProfiles = soap_new__trt__GetProfiles(soap,-1);
	_trt__GetProfilesResponse *trt__GetProfilesResponse = soap_new__trt__GetProfilesResponse(soap,-1);

	if (SOAP_OK == proxyMedia.__trt__GetProfiles(trt__GetProfiles,trt__GetProfilesResponse))
	{
		_trt__GetStreamUri *trt__GetStreamUri = soap_new__trt__GetStreamUri(soap,-1);
		trt__GetStreamUri->StreamSetup = soap_new_tt__StreamSetup(soap,-1);
		trt__GetStreamUri->StreamSetup->Stream = tt__StreamType__RTP_Unicast;
		trt__GetStreamUri->StreamSetup->Transport = soap_new_tt__Transport(soap,-1);
		trt__GetStreamUri->StreamSetup->Transport->Protocol = tt__TransportProtocol__RTSP;

		_trt__GetStreamUriResponse *trt__GetStreamUriResponse = soap_new__trt__GetStreamUriResponse(soap,-1);

		fprintf(stdout,"\r\n-------------------MediaProfiles-------------------\r\n");
		for (int i = 0; i < trt__GetProfilesResponse->Profiles.size(); i++)
		{
			fprintf(stdout,"profile%d:%s Token:%s\r\n",i,trt__GetProfilesResponse->Profiles[i]->Name.c_str(),trt__GetProfilesResponse->Profiles[i]->token.c_str());
			trt__GetStreamUri->ProfileToken = trt__GetProfilesResponse->Profiles[i]->token;

			if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyMedia.soap, NULL, m_strUsr.c_str(), m_strPwd.c_str()))
			{
				return -1;
			}

			if (SOAP_OK == proxyMedia.__trt__GetStreamUri(trt__GetStreamUri,trt__GetStreamUriResponse))
			{
				fprintf(stdout,"RTSP URI:%s\r\n",trt__GetStreamUriResponse->MediaUri->Uri.c_str());
			}
			else
			{
				PrintErr(proxyMedia.soap);
			}

			/*_trt__GetCompatibleVideoEncoderConfigurations *trt__GetCompatibleVideoEncoderConfigurations = soap_new__trt__GetCompatibleVideoEncoderConfigurations(soap,-1);
			_trt__GetCompatibleVideoEncoderConfigurationsResponse *trt__GetCompatibleVideoEncoderConfigurationsResponse = soap_new__trt__GetCompatibleVideoEncoderConfigurationsResponse(soap,-1);

			trt__GetCompatibleVideoEncoderConfigurations->ProfileToken = trt__GetProfilesResponse->Profiles[i]->token;

			if(SOAP_OK == proxyMedia.__trt__GetCompatibleVideoEncoderConfigurations(trt__GetCompatibleVideoEncoderConfigurations,trt__GetCompatibleVideoEncoderConfigurationsResponse))
			{

			}
			else
			{
				PrintErr(proxyMedia.soap);
			}*/
		}
	}
	else
	{
		PrintErr(proxyMedia.soap);
	}

	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data

	if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyMedia.soap, NULL,  m_strUsr.c_str(), m_strPwd.c_str()))
	{
		return -1;
	}

	_trt__GetVideoEncoderConfigurations *trt__GetVideoEncoderConfigurations = soap_new__trt__GetVideoEncoderConfigurations(soap,-1);
	_trt__GetVideoEncoderConfigurationsResponse *trt__GetVideoEncoderConfigurationsResponse = soap_new__trt__GetVideoEncoderConfigurationsResponse(soap,-1);

	if(SOAP_OK == proxyMedia.__trt__GetVideoEncoderConfigurations(trt__GetVideoEncoderConfigurations,trt__GetVideoEncoderConfigurationsResponse))
	{
		fprintf(stdout,"\r\n-------------------VideoEncoderConfigurations-------------------\r\n");

		for (int i = 0; i < trt__GetVideoEncoderConfigurationsResponse->Configurations.size();i++)
		{
			_trt__GetVideoEncoderConfiguration *trt__GetVideoEncoderConfiguration = soap_new__trt__GetVideoEncoderConfiguration(soap,-1);
			_trt__GetVideoEncoderConfigurationResponse *trt__GetVideoEncoderConfigurationResponse = soap_new__trt__GetVideoEncoderConfigurationResponse(soap,-1);

			trt__GetVideoEncoderConfiguration->ConfigurationToken = trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->token;

			if(SOAP_OK ==proxyMedia.__trt__GetVideoEncoderConfiguration(trt__GetVideoEncoderConfiguration,trt__GetVideoEncoderConfigurationResponse))
			{

			}
			else
			{
				PrintErr(proxyMedia.soap);
			}

			fprintf(stdout,"Encoding:%s\r\n",(trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->Encoding == tt__VideoEncoding__JPEG)?"tt__VideoEncoding__JPEG":(trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->Encoding == tt__VideoEncoding__MPEG4)?"tt__VideoEncoding__MPEG4":(trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->Encoding == tt__VideoEncoding__H264)?"tt__VideoEncoding__H264":"Error VideoEncoding");
			fprintf(stdout,"name:%s UseCount:%d token:%s\r\n",trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->Name.c_str(),
				trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->UseCount,trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->token.c_str());
			fprintf(stdout,"Width:%d Height:%d\r\n",trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->Resolution->Width,trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->Resolution->Height);

			if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyMedia.soap, NULL, m_strUsr.c_str(), m_strPwd.c_str()))
			{
				return -1;
			}

			_trt__GetVideoEncoderConfigurationOptions *trt__GetVideoEncoderConfigurationOptions = soap_new__trt__GetVideoEncoderConfigurationOptions(soap,-1);
			_trt__GetVideoEncoderConfigurationOptionsResponse *trt__GetVideoEncoderConfigurationOptionsResponse= soap_new__trt__GetVideoEncoderConfigurationOptionsResponse(soap,-1);

			trt__GetVideoEncoderConfigurationOptions->ConfigurationToken = &trt__GetVideoEncoderConfigurationsResponse->Configurations[i]->token;

			if (SOAP_OK == proxyMedia.__trt__GetVideoEncoderConfigurationOptions(trt__GetVideoEncoderConfigurationOptions,trt__GetVideoEncoderConfigurationOptionsResponse))
			{
			}
			else
			{
				PrintErr(proxyMedia.soap);
			}
		}
	}
	else
	{
		PrintErr(proxyMedia.soap);
	}

	//soap_destroy(soap); // remove deserialized class instances (C++ only)
	//soap_end(soap); // clean up and remove deserialized data

	/*_trt__SetVideoEncoderConfigurationResponse *trt__SetVideoEncoderConfigurationResponse = soap_new__trt__SetVideoEncoderConfigurationResponse(soap,-1);

	if(SOAP_OK == proxyMedia.__trt__SetVideoEncoderConfiguration(trt__SetVideoEncoderConfiguration,trt__SetVideoEncoderConfigurationResponse))
	{

	}
	else
	{
		PrintErr(proxyMedia.soap);
	}*/

	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data

	if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyEvent.soap, NULL,  m_strUsr.c_str(), m_strPwd.c_str()))
	{
		return -1;
	}

	if(SOAP_OK != soap_wsse_add_Timestamp(proxyEvent.soap, "Time", 10)) // 10 seconds lifetime
	{
		return -1;
	}

	_tev__GetEventProperties *tev__GetEventProperties = soap_new__tev__GetEventProperties(soap,-1);
	_tev__GetEventPropertiesResponse *tev__GetEventPropertiesResponse = soap_new__tev__GetEventPropertiesResponse(soap,-1);

	if(SOAP_OK != soap_wsa_request(proxyEvent.soap, NULL, NULL, "http://www.onvif.org/ver10/events/wsdl/EventPortType/GetEventPropertiesRequest"))
	{
		return -1;
	}

    if(proxyEvent.__ns11__GetEventProperties(tev__GetEventProperties,tev__GetEventPropertiesResponse) == SOAP_OK)
	{
		fprintf(stdout,"\r\n-------------------EventProperties-------------------\r\n");
        int i =0;
		for (i = 0;i < tev__GetEventPropertiesResponse->TopicNamespaceLocation.size();i++)
		{
			fprintf(stdout,"TopicNamespaceLocation[%d]:%s\r\n",i,tev__GetEventPropertiesResponse->TopicNamespaceLocation[i].c_str());
		}

		fprintf(stdout,"FixedTopicSet:%s\r\n",(tev__GetEventPropertiesResponse->ns1__FixedTopicSet)?"Y":"N");

		for (i = 0;i < tev__GetEventPropertiesResponse->ns1__TopicExpressionDialect.size();i++)
		{
			fprintf(stdout,"TopicExpressionDialect[%d]:%s\r\n",i,tev__GetEventPropertiesResponse->ns1__TopicExpressionDialect[i].c_str());
		}

		for (i = 0;i < tev__GetEventPropertiesResponse->MessageContentFilterDialect.size();i++)
		{
			fprintf(stdout,"MessageContentFilterDialect[%d]:%s\r\n",i,tev__GetEventPropertiesResponse->MessageContentFilterDialect[i].c_str());
		}

		for (i = 0;i < tev__GetEventPropertiesResponse->MessageContentSchemaLocation.size();i++)
		{
			fprintf(stdout,"MessageContentSchemaLocation[%d]:%s\r\n",i,tev__GetEventPropertiesResponse->MessageContentSchemaLocation[i].c_str());
		}
	}
	else
	{
		PrintErr(proxyEvent.soap);
	}

	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data

	//_tev__CreatePullPointSubscription *tev__CreatePullPointSubscription = soap_new__tev__CreatePullPointSubscription(soap,-1);
	//_tev__CreatePullPointSubscriptionResponse *tev__CreatePullPointSubscriptionResponse = soap_new__tev__CreatePullPointSubscriptionResponse(soap,-1);

	//if (SOAP_OK == proxyEvent.__ns11__CreatePullPointSubscription(tev__CreatePullPointSubscription,tev__CreatePullPointSubscriptionResponse))
	//{
	//	fprintf(stdout,"%s",tev__CreatePullPointSubscriptionResponse->SubscriptionReference.Address);
	//}
	//else
	//{
	//	PrintErr(proxyEvent.soap);
	//}

	//soap_destroy(soap); // remove deserialized class instances (C++ only)
	//soap_end(soap); // clean up and remove deserialized data

	proxyNP.endpoint = proxyEvent.endpoint;
	if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyNP.soap, NULL, m_strUsr.c_str(), m_strPwd.c_str()))
	{
		return -1;
	}

	if(SOAP_OK != soap_wsse_add_Timestamp(proxyNP.soap, "Time", 10)) // 10 seconds lifetime
	{
		return -1;
	}

	_ns1__Subscribe *ns1__Subscribe = soap_new__ns1__Subscribe(soap,-1);
	_ns1__SubscribeResponse *ns1__SubscribeResponse = soap_new__ns1__SubscribeResponse(soap,-1);
	ns1__Subscribe->ConsumerReference.Address = "http://172.18.13.22:8022/service";


	/*if(SOAP_OK != soap_wsa_request(proxyNP.soap, NULL, NULL, "http://www.onvif.org/ver10/events/wsdl/EventPortType/CreatePullPointSubscriptionRequest"))
	{
		return -1;
	}*/

	if(proxyNP.__ns13__Subscribe(ns1__Subscribe,ns1__SubscribeResponse) == SOAP_OK)
	{

	}
	else
	{
		PrintErr(proxyNP.soap);
	}

	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data

	if(blSupportPTZ)
	{
		if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyPTZ.soap, NULL,  m_strUsr.c_str(), m_strPwd.c_str()))
		{
			return -1;
		}

		if(SOAP_OK != soap_wsse_add_Timestamp(proxyPTZ.soap, "Time", 10)) // 10 seconds lifetime
		{
			return -1;
		}

		_tptz__GetNodes *tptz__GetNodes = soap_new__tptz__GetNodes(soap,-1);
		_tptz__GetNodesResponse *tptz__GetNodesResponse = soap_new__tptz__GetNodesResponse(soap,-1);
		if(proxyPTZ.__tptz__GetNodes(tptz__GetNodes,tptz__GetNodesResponse) == SOAP_OK)
		{

		}
		else
		{
			PrintErr(proxyPTZ.soap);
		}

		soap_destroy(soap); // remove deserialized class instances (C++ only)
		soap_end(soap); // clean up and remove deserialized data

		if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyPTZ.soap, NULL, m_strUsr.c_str(), m_strPwd.c_str()))
		{
			return -1;
		}

		_tptz__GetConfigurations *tptz__GetConfigurations = soap_new__tptz__GetConfigurations(soap,-1);
		_tptz__GetConfigurationsResponse *tptz__GetConfigurationsResponse = soap_new__tptz__GetConfigurationsResponse(soap,-1);

		if (proxyPTZ.__tptz__GetConfigurations(tptz__GetConfigurations,tptz__GetConfigurationsResponse) == SOAP_OK)
		{
			if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyPTZ.soap, NULL,  m_strUsr.c_str(), m_strPwd.c_str()))
			{
				return -1;
			}

			_tptz__GetConfiguration *tptz__GetConfiguration = soap_new__tptz__GetConfiguration(soap,-1);
			_tptz__GetConfigurationResponse *tptz__GetConfigurationResponse = soap_new__tptz__GetConfigurationResponse(soap,-1);

			tptz__GetConfiguration->PTZConfigurationToken = tptz__GetConfigurationsResponse->PTZConfiguration[0]->token;
			if (proxyPTZ.__tptz__GetConfiguration(tptz__GetConfiguration,tptz__GetConfigurationResponse) == SOAP_OK)
			{

			}
			else
			{
				PrintErr(proxyPTZ.soap);
			}

			_tptz__ContinuousMove *tptz__ContinuousMove = soap_new__tptz__ContinuousMove(soap,-1);
			_tptz__ContinuousMoveResponse *tptz__ContinuousMoveResponse = soap_new__tptz__ContinuousMoveResponse(soap,-1);

			tptz__ContinuousMove->ProfileToken = tptz__GetConfiguration->PTZConfigurationToken;
			tptz__ContinuousMove->Velocity = soap_new_tt__PTZSpeed(soap,-1);
			tptz__ContinuousMove->Velocity->PanTilt = soap_new_tt__Vector2D(soap,-1);

//			HANDLE   g_hIn   =   GetStdHandle(STD_INPUT_HANDLE);
//			INPUT_RECORD   iBuffer;
//			DWORD   dwResult   =   0;
//
	//------------------------Control PTZ------------------------//
		fprintf(stdout,"\r\n-------------------Control PTZ-------------------\r\n");
		fprintf(stdout,"\r\nUP:w DOWN:x LEFT:a RIGHT:d STOP:s\r\n");
//			while(1)
//			{
//				ReadConsoleInput(g_hIn,   &iBuffer,   1,   &dwResult);
//				//   some   a   key   was   pressed
//				if(iBuffer.EventType   ==   KEY_EVENT   &&   iBuffer.Event.KeyEvent.bKeyDown)
//				{
//					if(iBuffer.Event.KeyEvent.wVirtualKeyCode   ==   VK_ESCAPE)
//					{
//						break;
//					}
//					else if(87 == iBuffer.Event.KeyEvent.wVirtualKeyCode)//w
//					{
//						if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyPTZ.soap, NULL, "admin", DEV_PASSWORD))
//						{
//							return -1;
//						}
//
//						tptz__ContinuousMove->Velocity->PanTilt->x = 0;
//						tptz__ContinuousMove->Velocity->PanTilt->y = 1;
//
//						if (proxyPTZ.__tptz__ContinuousMove(tptz__ContinuousMove,tptz__ContinuousMoveResponse) == SOAP_OK)
//						{
//						}
//						else
//						{
//							PrintErr(proxyPTZ.soap);
//						}
//					}
//					else if(65 == iBuffer.Event.KeyEvent.wVirtualKeyCode)//a
//					{
//						if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyPTZ.soap, NULL, "admin", DEV_PASSWORD))
//						{
//							return -1;
//						}
//
//						tptz__ContinuousMove->Velocity->PanTilt->x = 1;
//						tptz__ContinuousMove->Velocity->PanTilt->y = 0;
//
//						if (proxyPTZ.__tptz__ContinuousMove(tptz__ContinuousMove,tptz__ContinuousMoveResponse) == SOAP_OK)
//						{
//						}
//						else
//						{
//							PrintErr(proxyPTZ.soap);
//						}
//
//					}
//					else if(68 == iBuffer.Event.KeyEvent.wVirtualKeyCode)//d
//					{
//						if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyPTZ.soap, NULL, "admin", DEV_PASSWORD))
//						{
//							return -1;
//						}
//
//						tptz__ContinuousMove->Velocity->PanTilt->x = -1;
//						tptz__ContinuousMove->Velocity->PanTilt->y = 0;
//
//						if (proxyPTZ.__tptz__ContinuousMove(tptz__ContinuousMove,tptz__ContinuousMoveResponse) == SOAP_OK)
//						{
//						}
//						else
//						{
//							PrintErr(proxyPTZ.soap);
//						}
//					}
//					else if(83 == iBuffer.Event.KeyEvent.wVirtualKeyCode)//s
//					{
//						if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyPTZ.soap, NULL, "admin", DEV_PASSWORD))
//						{
//							return -1;
//						}
//
//						_tptz__Stop *tptz__Stop = soap_new__tptz__Stop(soap,-1);
//						_tptz__StopResponse *tptz__StopResponse = soap_new__tptz__StopResponse(soap,-1);
//
//						if (proxyPTZ.__tptz__Stop(tptz__Stop,tptz__StopResponse) == SOAP_OK)
//						{
//						}
//						else
//						{
//							PrintErr(proxyPTZ.soap);
//						}
//
//					}
//					else if(88 == iBuffer.Event.KeyEvent.wVirtualKeyCode)//x
//					{
//						if(SOAP_OK != soap_wsse_add_UsernameTokenDigest(proxyPTZ.soap, NULL, "admin", DEV_PASSWORD))
//						{
//							return -1;
//						}
//
//						tptz__ContinuousMove->Velocity->PanTilt->x = 0;
//						tptz__ContinuousMove->Velocity->PanTilt->y = -1;
//
//						if (proxyPTZ.__tptz__ContinuousMove(tptz__ContinuousMove,tptz__ContinuousMoveResponse) == SOAP_OK)
//						{
//						}
//						else
//						{
//							PrintErr(proxyPTZ.soap);
//						}
//					}
//				}
//				Sleep(1);
//			}
		}
		else
		{
			PrintErr(proxyPTZ.soap);
		}

		soap_destroy(soap); // remove deserialized class instances (C++ only)
		soap_end(soap); // clean up and remove deserialized data
	}

	getchar();
	soap_free(soap);//detach and free runtime context
	soap_done(soap); // detach context (last use and no longer in scope)


	return 0;
}




