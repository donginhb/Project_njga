// MediaItem.cpp: implementation of the CMediaItem class.
//
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include "stdafx.h"
#include "EV9000Mgw.h"
#include "../public/net/SockObj.h"
#endif
#include "MediaItem.h"
#include <iostream>

#include "CtrlProOp.h"
#include "ConfigOp.h"
using namespace std;

#ifdef WIN32
//#pragma comment(lib,"lib/H264ToPs.lib")
#endif 

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

typedef struct _HIK_MEDIAINFO_    // modified by gb 080425
{
				unsigned int    media_fourcc;   // "HKMI": 0x484B4D49 Hikvision Media Information
				unsigned short  media_version;   // 版本号：指本信息结构版本号，目前为0x0101,即1.01版本，01：主版本号；01：子版本号。
				unsigned short  device_id;    // 设备ID，便于跟踪/分析   
				
				unsigned short  system_format;          // 系统封装层
				unsigned short  video_format;           // 视频编码类型
				
				unsigned short  audio_format;           // 音频编码类型
				unsigned char   audio_channels;         // 通道数  
				unsigned char   audio_bits_per_sample;  // 样位率
				unsigned int    audio_samplesrate;      // 采样率 
				unsigned int    audio_bitrate;          // 压缩音频码率,单位：bit
				
				unsigned int    reserved[4];            // 保留
}HIK_MEDIAINFO;

typedef struct PES_HEADER_tag
{
	unsigned char packet_start_code_prefix[3];
	unsigned char stream_id;
	unsigned char PES_packet_length[2];
	
	unsigned char original_or_copy:1;
	unsigned char copyright:1;
	unsigned char data_alignment_indicator:1;
	unsigned char PES_priority:1;
	unsigned char PES_scrambling_control:2;
	unsigned char fix_bit:2;
	
	unsigned char PES_extension_flag:1;
	unsigned char PES_CRC_flag:1;
	unsigned char additional_copy_info_flag:1;
	unsigned char DSM_trick_mode_flag:1;
	unsigned char ES_rate_flag:1;
	unsigned char ESCR_flag:1;
	unsigned char PTS_DTS_flags:2;
	
	unsigned char PES_header_data_length;
	PES_HEADER_tag()
	{
		packet_start_code_prefix[0] = 0x00;
		packet_start_code_prefix[1] = 0x00;
		packet_start_code_prefix[2] = 0x01;
		
		PES_packet_length[0] = 0x00;
		PES_packet_length[1] = 0x00;
		
		stream_id = 0xE0;
		fix_bit = 0x02;
	}
	
}*pPES_HEADER_tag;

#ifdef WIN32
extern CEV9000MgwApp theApp;
#endif
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMediaItem::CMediaItem(int nID)
{
	m_lpParent = NULL;
	m_lpNext = NULL;
	m_lpChild = NULL;
	ZeroMemory(m_sName, MAX_PATH);
	
	m_eMediaType = MEDIAITEM_TYPE_INVALID;
	m_nID = nID;
#ifdef WIN32
	//	m_lpLog = NULL;
	m_hMsgWnd = NULL;
#endif
	m_lpParent =NULL;
	m_lpNext  =NULL;
	m_lpChild =NULL;
}

CMediaItem::~CMediaItem()
{
	DeleteChild();

	//	MEMORY_DELETE(m_lpLog);
}

MEDIAITEM_TYPE CMediaItem::GetMediaType()
{
	return m_eMediaType;
}

int CMediaItem::GetID()
{
	return m_nID;
}

int CMediaItem::SetParent(CMediaItem* lpMediaItem)
{
	m_lpParent = lpMediaItem;
	
	return 0;
}

CMediaItem* CMediaItem::GetParent()
{
	return m_lpParent;
}

CMediaItem* CMediaItem::GetChild(int nID /* = -1 */, MEDIAITEM_TYPE eMediaType /* = MEDIAITEM_TYPE_INVALID */)
{
	if (-1 != nID)
	{
		CMediaItem* lpMediaItem = m_lpChild;
		while (lpMediaItem)
		{
			if (nID == lpMediaItem->GetID())
			{
				if (MEDIAITEM_TYPE_INVALID != eMediaType)
				{
					if (eMediaType == lpMediaItem->GetMediaType())
					{
						return lpMediaItem;
					}
				}	
				else
				{
					return lpMediaItem;
				}
			}
			
			lpMediaItem = lpMediaItem->GetNext();
		}
	}
	else
	{
		CMediaItem* lpMediaItem = m_lpChild;
		while (lpMediaItem)
		{
			if (MEDIAITEM_TYPE_INVALID != eMediaType)
			{
				if (eMediaType == lpMediaItem->GetMediaType())
				{
					return lpMediaItem;
				}
			}	
			else
			{
				return lpMediaItem;
			}
			
			lpMediaItem = lpMediaItem->GetNext();
		}		
	}
	
	return NULL;
}

CMediaItem* CMediaItem::GetLastChild()
{
	if (!m_lpChild)
	{
		return NULL;
	}
	
	CMediaItem* lpMediaItem = m_lpChild;
	while (lpMediaItem->GetNext())
	{
		lpMediaItem = lpMediaItem->GetNext();
	}
	
	return lpMediaItem;
}

int CMediaItem::SetNext(CMediaItem* lpMediaItem)
{
	m_lpNext = lpMediaItem;
	
	return 0;
}

CMediaItem* CMediaItem::GetNext()
{
	return m_lpNext;
}

int CMediaItem::GetChildNum(MEDIAITEM_TYPE eMediaType /* = MEDIAITEM_TYPE_INVALID */)
{
	int nMediaCount = 0;
	
	CMediaItem* lpMediaItem = m_lpChild;
	while (lpMediaItem)
	{
		if (MEDIAITEM_TYPE_INVALID != eMediaType)
		{
			if (lpMediaItem->GetMediaType() == eMediaType)
			{
				nMediaCount++;
			}
		}
		else
		{
			nMediaCount++;
		}		
		
		lpMediaItem = lpMediaItem->GetNext();
	}
	
	return nMediaCount;
}

int CMediaItem::InsertChild(CMediaItem* lpMediaItem)
{
	if (!lpMediaItem)
	{
		return -1;
	}
	
	if (m_lpChild)
	{
		CMediaItem* lpLastMediaItem = GetLastChild();
		if (lpLastMediaItem)
		{
			lpLastMediaItem->SetNext(lpMediaItem);
		}
	}
	else
	{
		m_lpChild = lpMediaItem;
	}	
	
	return 0;
}

void CMediaItem::DeleteChild(CMediaItem* lpMediaItem /* = NULL */)
{
	if (!lpMediaItem)
	{
		CMediaItem* lpMediaItem = m_lpChild;
		CMediaItem* lpTempMediaItem = NULL;
		while (lpMediaItem)
		{
			lpTempMediaItem = lpMediaItem->GetNext();
			MEMORY_DELETE(lpMediaItem);
			lpMediaItem = lpTempMediaItem;
		}
		
		m_lpChild = NULL;
	}
	else
	{
		CMediaItem* lpPreItem = NULL;
		CMediaItem* lpNextItem = lpMediaItem->GetNext();
		CMediaItem* lpItem = GetChild();
		CMediaItem* lpTempItem = NULL;
		while (lpItem)
		{
			if (lpItem == lpMediaItem)
			{
				lpPreItem = lpTempItem;
				
				break;
			}
			
			lpTempItem = lpItem;
			lpItem = lpItem->GetNext();
		}
		
		if (lpPreItem)
		{//不是第一个
			lpPreItem->SetNext(lpNextItem);
		}
		else
		{//第一个
			m_lpChild = lpNextItem;
		}
		
		MEMORY_DELETE(lpMediaItem);
	}
}

char* CMediaItem::GetName()
{
	return m_sName;
}

void CMediaItem::AddMsgWnd(HWND hMsgWnd)
{
	m_hMsgWnd = hMsgWnd;
}

void CMediaItem::SetData(BYTE* byData)
{
	
}

BOOL CMediaItem::GetData(BYTE* byData)
{
	return FALSE;
}

#ifdef WIN32
void CMediaItem::NotifyMessage(DWORD dwMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_lpParent)
	{
		m_lpParent->NotifyMessage(dwMsg, wParam, lParam);
		
		return;
	}
}
#endif

/////////////////////
//设备节点
/////////////////////
CMediaItemDevice::CMediaItemDevice(int nID) : CMediaItem(nID)
{
	m_eMediaType = MEDIAITEM_TYPE_DEVICE;
	m_hConHandle =-1;

	m_bThreadAliveStatusFlag=FALSE;
#ifdef WIN32
	m_pSdkProxy =NULL;
	m_hThreadAliveStatus=0;
	m_dwThreadAliveStatusID=0;
#else
    m_threadAliveStatusHandle =0;
	pthread_mutex_init(&m_csDevOpLock,NULL);
#endif
}

CMediaItemDevice::~CMediaItemDevice()
{
	Close();
}

#ifdef WIN32
int CMediaItemDevice::Start(UNGB_DEVCONFIGREC &stDevConfig,CSdkProxy* pSdkProxy)
{
	//启动状态线程
	if (!m_hThreadAliveStatus)
	{
		mgwlog("启动状态线程\n");
		m_bThreadAliveStatusFlag = TRUE;
		m_hThreadAliveStatus = CreateThread(NULL, 0, ThreadAliveStatus, (LPVOID)this, 0, &m_dwThreadAliveStatusID);
	}

	m_stDevConfig = stDevConfig;  //连接参数
	m_pSdkProxy = pSdkProxy;      //Sdk
	return ConnectDevice();
}
#else
int CMediaItemDevice::Start(UNGB_DEVCONFIGREC &stDevConfig)
{
	//启动状态线程
//	if (!m_hThreadAliveStatus)
//	{
//		mgwlog("启动状态线程\n");
//		m_bThreadAliveStatusFlag = TRUE;
//		m_hThreadAliveStatus = CreateThread(NULL, 0, ThreadAliveStatus, (LPVOID)this, 0, &m_dwThreadAliveStatusID);
//	}
	int err=0;
	if (!m_bThreadAliveStatusFlag)
	{
		m_bThreadAliveStatusFlag = TRUE;
		err = pthread_create(&m_threadAliveStatusHandle,NULL,ThreadAliveStatus,(void*)this);
		if(err!=0)
		{
			return FALSE;
		}
	}

	m_stDevConfig = stDevConfig;  //连接参数
	return ConnectDevice();
}
#endif

int CMediaItemDevice::Close()
{
	//关闭状态线程
#ifdef WIN32
	if (m_hThreadAliveStatus)
	{
		m_bThreadAliveStatusFlag = FALSE;
		WaitForSingleObject(m_hThreadAliveStatus, INFINITE);
		CLOSE_HANDLE(m_hThreadAliveStatus);
		m_dwThreadAliveStatusID = 0;
	}
#else
	if(m_threadAliveStatusHandle)
    {
		m_bThreadAliveStatusFlag = FALSE;
		pthread_cancel(m_threadAliveStatusHandle);
		pthread_join(m_threadAliveStatusHandle, NULL);
		m_threadAliveStatusHandle =0;
	}
#endif

	//关闭通道
	CMediaItemDevice* lpDevice=NULL;
	map<string,CMediaItemReal*>::iterator iter = m_mapRealPool.begin(); 
	while (iter != m_mapRealPool.end())
	{
		MEMORY_DELETE(iter->second);
		iter++;
	}

	//关闭录像
    mgwlog("补充关闭录像\n");

	return 0;
}

int CMediaItemDevice::Close(int idialog_index,BOOL bReal)
{
	int nRetStream =0,nRetPlayBack=0;
    nRetStream = StopReal(idialog_index,bReal);  //是否完全关闭
#ifdef SUP_PLAYBACK
	nRetPlayBack = StopPlayBack(idialog_index);
#endif
	if((nRetStream ==0)||(nRetPlayBack ==0))
	{
		return 0;
	}
	return -1;
}

//检测通道情况
int CMediaItemDevice::CheckAllCh()
{
	CAutoLock lock(&m_csDevOpLock);
	//遍列下级通道
	UNGB_CHCONFIGREC stChCfg;
	map<string,CMediaItemReal*>::iterator iterUnit =m_mapRealPool.begin();
	while(iterUnit != m_mapRealPool.end())
	{	
		if(!CSysCfg::Instance()->GetOneChCfg(iterUnit->first,stChCfg)) //未找到
		{
			mgwlog("通道不存在，删除该通道节点，id:%s\n",iterUnit->first.c_str());
			if (iterUnit->second)
			{
				MEMORY_DELETE(iterUnit->second);
			}
			//iterUnit = 
			m_mapRealPool.erase(iterUnit++);
			continue;
		}else{
			if (iterUnit->second)
			{
				iterUnit->second->SetChConfig(stChCfg); //更新通道信息
			}
			else
			{
				//iterUnit = 
				m_mapRealPool.erase(iterUnit++);
				continue;
			}
		}	
		iterUnit++;
	}
	return 0;
}  

//获取通道名称
int CMediaItemDevice::UpdateUnitName() 
{
	//写入数据库
	CDbOp  *pDbOp=NULL;
	if(!pDbOp) 
	{
		pDbOp = new CDbOp();
	}
	int nRet = pDbOp->DBOpen(NULL,0,NULL);
	char szSql[MAX_SQL_LEN]={0};
	map<string,CMediaItemReal*>::iterator iter = m_mapRealPool.begin();
	char szChName[MAX_CHNAME_LEN]={0};
	while(iter != m_mapRealPool.end())
	{
		memset(szChName,0,MAX_CHNAME_LEN);
		memset(szSql,0,MAX_SQL_LEN);
#ifdef WIN32
		if(m_pSdkProxy)
		{
			//m_pSdkProxy->fEDV_DEVICE_GetChannelName(m_hConHandle,iter->second->m_stChConfig.iMapChannel,szChName);
			m_pSdkProxy->fEDV_DEVICE_GetChannelName(m_hConHandle,ConvertMapChannel2int(iter->second->GetStrChannelID()),szChName);
		}else{
            mgwlog("m_pSdkProxy 为空\n");
			iter++;
			continue;
		}
#else

#endif
		if(szChName[0]!=0)   //查询到名称 
		{
           	//sprintf(szChName,"%s_%d",((CMediaItemDevice *)iter->second->GetParent())->m_stDevConfig.strDeviceIP.c_str(),iter->second->m_stChConfig.iMapChannel);
		}else  //获取不到则 ip+chno  
		{
            //sprintf(szChName,"name%s_%d",((CMediaItemDevice *)iter->second->GetParent())->m_stDevConfig.strDeviceIP.c_str(),iter->second->m_stChConfig.iMapChannel);
	        sprintf(szChName,"%s_%s",((CMediaItemDevice *)iter->second->GetParent())->m_stDevConfig.strDeviceIP.c_str(),(iter->second->GetStrChannelID()).c_str());
		}
// 		//修改设备 修改记录
// 		//查询记录，如果为空或有wis则更新
// 		sprintf(szSql,"select ChannelName from UnGBPhyDeviceChannelConfig where LogicDeviceID ='%s'",iter->second->m_stChConfig.strLogicDeviceID.c_str());
//         pDbOp->DBQuery(szSql);
// 		UNGB_CHCONFIGREC stChConfig; 
// 		char   szChannelName[256]={0};
// 		memset(szChannelName,0,256);
// 		pDbOp->GetFieldValue("ChannelName",szChannelName,255);
// 		stChConfig.strChannelName = szChannelName;
// 		size_t found = stChConfig.strChannelName.find("name");  
// 		if (stChConfig.strChannelName.empty() || found!=std::string::npos)
// 		{
// 			sprintf(szSql,"update UnGBPhyDeviceChannelConfig set ChannelName='%s'where LogicDeviceID ='%s'",
// 				szChName,iter->second->m_stChConfig.strLogicDeviceID.c_str());
// 			TRACE("sql:%s\n",szSql);
// 			if(pDbOp->DBExec(szSql)>0)
// 			{
// 				mgwlog("修改通道名成功：%s",szSql);
// 				TRACE("修改通道名成功：%s",szSql);
// 			}
// 		}
		sprintf(szSql,"update UnGBPhyDeviceChannelConfig set ChannelName='%s'where LogicDeviceID ='%s'",
			szChName,(iter->second->GetLogicDeviceID()).c_str());
		if(pDbOp->DBExec(szSql)>0)
		{
			TRACE("修改通道名成功：%s",szSql);
		}
		iter++;
	}
 	MEMORY_DELETE(pDbOp);
	return 0;
}  

//获取通道信息
int CMediaItemDevice::GetUnitState(string strLogicDeviceID,CHANNELITEM_STATE &eChannelState)
{
    //查找ID
	map<string,CMediaItemReal*>::iterator iterReal = m_mapRealPool.find(strLogicDeviceID);
	if((iterReal != m_mapRealPool.end()) && iterReal->second)
	{
		eChannelState = iterReal->second->GetOldChannelState();
// #ifdef WIN32
// 		eChannelState = iterReal->second->GetOldChannelState();
// #else
// 		eChannelState = CHANNELITEM_STATE_OPEN;  //tmp for test chenyu 140317
// #endif
		return 0;
	}else{
		mgwlog("CMediaItemDevice::GetUnitState 没有找到指定通道对象：%s\n",strLogicDeviceID.c_str());
		return -1;
	}
}    

int CMediaItemDevice::GetAllUnitInfo(list<UNITINFO> &lstUnitInfo)
{
	map<string,CMediaItemReal*>::iterator iterUnit =m_mapRealPool.begin();
	while(iterUnit != m_mapRealPool.end())
	{
		if(iterUnit->second)
		{
			UNITINFO stUnitInfo;
			iterUnit->second->GetUnitInfo(stUnitInfo);
			lstUnitInfo.push_back(stUnitInfo);
		}
		iterUnit++;
	}
	return 0;
}

int CMediaItemDevice::ConnectDevice()
{
	//加载成功,执行连接
	CONNECT_DEVICE_INFO DeviceInfo;
	ZeroMemory(&DeviceInfo, sizeof(CONNECT_DEVICE_INFO));
	DeviceInfo.eDeviceType = (DEVICE_TYPE)(m_stDevConfig.iDeviceType);
	DWORD  dwIP=0;
#ifdef WIN32
	SockObj::ConvertStringToIP(&dwIP,(char *)m_stDevConfig.strDeviceIP.c_str());
#else
	dwIP = iptoint((char *)m_stDevConfig.strDeviceIP.c_str());
#endif
	DeviceInfo.nDeviceIP = dwIP;
	DeviceInfo.nDevicePort = m_stDevConfig.iDevicePort;
	strcpy(DeviceInfo.sUserName, m_stDevConfig.strUseName.c_str());
	strcpy(DeviceInfo.sPassword, m_stDevConfig.strPassword.c_str());
#ifdef WIN32
	if(m_pSdkProxy)
	{
		int hConHandle = m_pSdkProxy->fEDV_DEVICE_ConnectDevice(&DeviceInfo);  //获取连接句柄
		if(hConHandle<0)
		{
			mgwlog("CDevCtrl::Open-->连接设备失败,原因[%d] fail\n",hConHandle);
			return -1;
		}
		m_hConHandle = hConHandle;
		mgwlog("CMediaItemDevice::ConnectDevice() 连接设备成功,连接句柄hConHandle[%d] DevID[%d]\n",hConHandle,m_stDevConfig.iID);
	}else{
		return -1;
	}
#else

#endif
	return 0;
}

void CMediaItemDevice::DisConnectDevice()
{
#ifdef WIN32
	m_pSdkProxy->fEDV_DEVICE_DisConnectDevice(m_hConHandle);
	m_hConHandle = -1;
#else

#endif
	return ;
}

int CMediaItemDevice::ConvertMapChannel2int(string strMapChannel)
{
	int iMapChannel = 0;
	if(DEVICE_TYPE_EV8K == m_stDevConfig.iDeviceType)
	{
		iMapChannel = ConvertEv8kCh2Int((char*)strMapChannel.c_str());	
	}
#ifdef WIN32
	else if (DEVICE_TYPE_BOSCH == m_stDevConfig.iDeviceType)
	{
		//IP->DWORD
		SockObj::ConvertStringToIP((unsigned long*)&iMapChannel,(char*)strMapChannel.c_str());
	}
#endif
	else
	{
		iMapChannel = atoi(strMapChannel.c_str());
	}
	return iMapChannel;
}
int CMediaItemDevice::ConvertEv8kCh2Int(char* sChID)  //转换Ev8k通道
{	
	int nID = 0;
	string strID = sChID;
	//分割字符串
	vector<string> vctID;
	StrSplit(strID,'.',vctID);
	if(vctID.size()<4)
	{
		mgwlog("ConvertEv8kCh2Int参数错误:%s\n",sChID);
		return -1;
	}
	nID = (atoi(vctID[1].c_str())<<24)|(atoi(vctID[2].c_str())<<16)|(atoi(vctID[3].c_str())&0xffff);
	//sprintf(sChID,"1.%d.%d.%d",nID>>24,(nID>>16)&0x000000ff,(nID&0xffff));	
	return nID;
}
void CMediaItemDevice::StrSplit(const string &str,const char delimter,vector<string> &strList)
{
	// 	string ss="111111";
	// 	int pos, bpos=0;
	// 	pos =3;
	//  string sss= ss.substr(bpos,pos-bpos);
	int pos, bpos=0;
	while((pos=str.find(delimter, bpos)) != string::npos){
		string strpara = str.substr(bpos, pos-bpos);
		strList.push_back(strpara);
		bpos = pos+1;
	}
	
	if(bpos < str.size())
		strList.push_back(str.substr(bpos, str.size()-bpos));
}

char* CMediaItemDevice::GetName()
{
	sprintf(m_sName, "前端设备 [%d个]", GetChildNum());
	return m_sName;
}

#ifdef WIN32
//查询录像记录
int CMediaItemDevice::GetRecordInfo(string strDeviceID,
									SYSTEMTIME sStartTime,SYSTEMTIME sStopTime,
									EDVDVRRECORDTABLE* lpTable,int nTableCount,int& iTotalCount) 
{
	iTotalCount = 0;
	if(m_pSdkProxy)
	{
		UNGB_CHCONFIGREC stChCfg;
		if(!CSysCfg::Instance()->GetOneChCfg(strDeviceID,stChCfg)) //未找到
		{
			mgwlog("没有找到指定通道:[%s]\n",strDeviceID.c_str());
			return -1;  //没有找到指定通道
		}
		int nChannelID = ConvertMapChannel2int(stChCfg.strMapChannel);
		iTotalCount = m_pSdkProxy->fEDV_DEVICE_FindRecord(m_hConHandle,nChannelID,sStartTime,sStopTime,lpTable,nTableCount);
		//mgwlog("查询到录像记录:%d 获取:%d\n",iTotalCount,nTableCount);
		return 0;
	}
	return -1;
}
#endif

//增加通道
int CMediaItemDevice::CreateReal(UNGB_CHCONFIGREC &stChConfig,BOOL bInit)
{
	CAutoLock lock(&m_csDevOpLock);
    if("00000000000000000000" == stChConfig.strLogicDeviceID)  //Onvif还未初始化数据
	{
		mgwlog("通道对象不需创建,Channel:%s LogicDeviceID:%s\n",stChConfig.strMapChannel.c_str(),stChConfig.strLogicDeviceID.c_str());
		return -1;
	}

	//先查找有没有 added by chenyu 140606
	//找视频池(m_mapRealPool) 
	map<string,CMediaItemReal*>::iterator iterReal = m_mapRealPool.find(stChConfig.strLogicDeviceID);
	if(iterReal != m_mapRealPool.end())
	{
		//mgwlog("CMediaItemDevice::CreateReal 视频对象：%s已打开\n",stChConfig.strLogicDeviceID.c_str());
		return 0;
	}

	//创建通道对象
	int iMapChannel = 0;
#ifdef WIN32
	iMapChannel = ConvertMapChannel2int(stChConfig.strMapChannel);
	CMediaItemReal* lpReal = new CMediaItemReal(iMapChannel);
	//mgwlog("CMediaItemDevice::CreateReal创建通道对象,mapChannel:%s\n",stChConfig.strMapChannel.c_str());
#else
	CMediaItemReal* lpReal = new CMediaItemRealOnvif(iMapChannel);
#endif
	if(!lpReal)
	{
		mgwlog("CMediaItemDevice::CreateReal创建通道对象失败,Channel:%s LogicDeviceID:%s\n",stChConfig.strMapChannel.c_str(),stChConfig.strLogicDeviceID.c_str());
		return -1;
	}
	lpReal->SetChConfig(stChConfig);   //设置通道配置信息
	lpReal->SetDeviceIP(m_stDevConfig.strDeviceIP);  //所属设备IP
	lpReal->SetParent(this); //链接到设备树上
	
	//记录Unit的map映射
	m_mapRealPool[stChConfig.strLogicDeviceID] = lpReal;
	if(lpReal)
	{
		STREAM_CON_INFO stStreamConInfo;
#ifdef WIN32
		//lpReal->StartStream(stStreamConInfo,UNIT_OPEN_EMPTY); //只创建视频对象，不打开
#else
		mgwlog("CMediaItemDevice::CreateReal to do\n");
		return 0;
#endif
	}
	return 0;
} 

//eOpenType 打开类型 空打开、正常打开
int CMediaItemDevice::OpenReal(STREAM_CON_INFO stStreamConInfo,UNITOPENTYPE eOpenType)
{
	mgwlog("CMediaItemDevice::OpenReal enter\n");
	CAutoLock lock(&m_csDevOpLock);
	mgwlog("CMediaItemDevice::OpenReal after lock\n");
	stStreamConInfo.iDeviceType = m_stDevConfig.iDeviceType;  //设备类型
	//先找工作队列(m_mapRealWorkQueue) 
	map<int,CMediaItemReal*>::iterator iterRealWork = m_mapRealWorkQueue.find(stStreamConInfo.stSipContext.idialog_index);
	if((iterRealWork != m_mapRealWorkQueue.end()) && iterRealWork->second)
	{
		mgwlog("CMediaItemDevice::OpenReal工作队列中找到指定视频对象[dialogindex:%d]:%s 先执行关闭\n",
			stStreamConInfo.stSipContext.idialog_index,stStreamConInfo.strLogicDeviceID.c_str());
		iterRealWork->second->StopStream(TRUE);
	}else{
		mgwlog("CMediaItemDevice::OpenReal工作队列中没有找到指定视频对象[dialogindex:%d]:%s\n",
			stStreamConInfo.stSipContext.idialog_index,stStreamConInfo.strLogicDeviceID.c_str());
	}
	//再找视频池(m_mapRealPool) 
	map<string,CMediaItemReal*>::iterator iterReal = m_mapRealPool.find(stStreamConInfo.strLogicDeviceID);
	if((iterReal != m_mapRealPool.end()) && iterReal->second)
	{
		m_mapRealWorkQueue[stStreamConInfo.stSipContext.idialog_index] = iterReal->second;
		mgwlog("Device::OpenReal记录视频对象[%s:%s]到m_mapRealWorkQueue中size:%d dialogindex:%d\n",
			m_stDevConfig.strDeviceIP.c_str(),m_stDevConfig.strDeviceName.c_str(),
			m_mapRealWorkQueue.size(),stStreamConInfo.stSipContext.idialog_index);
		return iterReal->second->StartStream(stStreamConInfo,eOpenType);
	}else{
		mgwlog("CMediaItemDevice::OpenReal 视频池中没有找到指定视频对象：%s\n",stStreamConInfo.strLogicDeviceID.c_str());
		return -1;
	}
	return 0;
}

//是否完全关闭
int CMediaItemDevice::StopReal(int idialog_index,BOOL bReal)
{
	mgwlog("CMediaItemDevice::StopReal enter\n");
	CAutoLock lock(&m_csDevOpLock);
	mgwlog("CMediaItemDevice::StopReal after lock\n");
#ifdef WIN32
	//在工作队列(m_mapRealWorkQueue)中查找
	map<int,CMediaItemReal*>::iterator iterIndex = m_mapRealWorkQueue.find(idialog_index);
	if((iterIndex != m_mapRealWorkQueue.end()) && iterIndex->second)
	{
	   /*
	    *需在编译命令行中加入 /EHa 参数,
	    *这样VC编译器就不会把没有throw的try_catch模块优化掉
		*/
		static int iExceptionCount=0;  //异常计数
		try
		{
			iterIndex->second->StopStream(bReal);
			m_mapRealWorkQueue.erase(iterIndex);  //删除工作队列中视频连接
			mgwlog("CMediaItemDevice::StopStream dlgindex:%d,m_mapRealWorkQueue size:%d\n",idialog_index,m_mapRealWorkQueue.size());
		}
		catch (...)
		{
			iExceptionCount++;  //计数
			if(iExceptionCount > MAX_EXCEPTION_NUM)
			{
				mgwlog("Exception: CMediaItemDevice::StopStream 异常超过50次，退出....\n");
				exit(-1);
			}
			mgwlog("Exception: CMediaItemDevice::StopStream 异常次数:%d\n",iExceptionCount);
		}
		return 0;
	}else{
		mgwlog("CMediaItemDevice::StopStream 没有找到指定视频对象,dlg_index:%d\n",idialog_index);
		return -1;
	}
#else
	mgwlog("onvif不做Stop码流处理\n");
#endif
}

#ifdef SUP_PLAYBACK
int CMediaItemDevice::FindPlayBack(int idialog_index,CPlayBack* &pPlayBack)
{
	map<int,CPlayBack*>::iterator iterPlayBack = m_mapPlayBack.find(idialog_index);
	if((iterPlayBack == m_mapPlayBack.end()) || (!(iterPlayBack->second)))
	{
		mgwlog("CMediaItemDevice::FindPlayBack 没有找到指定回放对象,index:%d\n",idialog_index);
		return -1;  //没有找到指定设备
	}
	pPlayBack = iterPlayBack->second;
	return 0;
}

//创建回放对象
int CMediaItemDevice::OpenPlayBack(STREAM_CON_INFO &stStreamConInfo,UNITOPENTYPE eOpenType)  
{
	mgwlog("CMediaItemDevice::OpenPlayBack enter\n");
	CAutoLock lock(&m_csDevOpLock);
	mgwlog("CMediaItemDevice::OpenPlayBack after lock\n");
	UNGB_CHCONFIGREC stChCfg;
	if(!CSysCfg::Instance()->GetOneChCfg(stStreamConInfo.strLogicDeviceID,stChCfg))
	{
		mgwlog("CMediaItemDevice::OpenPlayBack 没有找到指定通道配置1：%s\n",stStreamConInfo.strLogicDeviceID.c_str());
		return -1;  //没有找到指定设备
	}

	CPlayBack* lpPlayBack =NULL;
	if( 0 == FindPlayBack(stStreamConInfo.stSipContext.idialog_index,lpPlayBack))  //找到
	{
		mgwlog("CMediaItemDevice::OpenPlayBack success\n");
		return 0;
	}

	//创建新的回放对象
	lpPlayBack =NULL;
	int iMapChannel = 0;
	iMapChannel = ConvertMapChannel2int(stChCfg.strMapChannel);
	lpPlayBack = new CPlayBack(iMapChannel);
	if(!lpPlayBack)
	{
		mgwlog("CMediaItemDevice::OpenPlayBack new CPlayBack fail LogicID:%s\n",m_nID);
		return -1;
	}
	if(lpPlayBack->SetChConfig(stChCfg)<0)
	{
		mgwlog("CMediaItemDevice::OpenPlayBack setChConfig fail\n");
		return -1;
	}
	lpPlayBack->SetStreamConInfo(stStreamConInfo);
	lpPlayBack->SetParent(this);  //链接到设备树
	
	//记录Unit的map映射
	m_mapPlayBack[stStreamConInfo.stSipContext.idialog_index]=lpPlayBack;
	mgwlog("CMediaItemDevice::OpenPlayBack 创建回放对象success,idialog_index:%d\n",stStreamConInfo.stSipContext.idialog_index);
    lpPlayBack->StartByTime();  //开始播放
	return 0;
}

int CMediaItemDevice::StopPlayBack(int idialog_index)
{
	mgwlog("CMediaItemDevice::StopPlayBack enter\n");
	CAutoLock lock(&m_csDevOpLock);
	mgwlog("CMediaItemDevice::StopPlayBack after lock\n");
	map<int,CPlayBack*>::iterator iter = m_mapPlayBack.find(idialog_index);
	if((iter != m_mapPlayBack.end())&&(iter->second))
	{
		MEMORY_DELETE(iter->second);
		m_mapPlayBack.erase(iter);
		mgwlog("CMediaItemDevice::StopPlayBack回放对象 dlgindex:%d,m_mapPlayBack size:%d\n",idialog_index,m_mapPlayBack.size());
		return 0;
	}
	
	return -1;
}

int CMediaItemDevice::PlayBackCtrl(int idialog_index,RECORD_CTRL eRecordCtrl,int nCtrlData,int* nReturnData)
{
	CAutoLock lock(&m_csDevOpLock);
	map<int,CPlayBack*>::iterator iter = m_mapPlayBack.find(idialog_index);
	if((iter != m_mapPlayBack.end())&&(iter->second))
	{
		CPlayBack* lpPlayBack = NULL;
		lpPlayBack = iter->second;
		int nRet = -1;
		if(lpPlayBack)
		{
			if(RECORD_CTRL_PAUSE == eRecordCtrl)
			{
				nRet = lpPlayBack->CtrlRecordStream(RECORD_CTRL_PAUSE,0,NULL);
			}
			else if (RECORD_CTRL_RESUME == eRecordCtrl)
			{
				nRet = lpPlayBack->CtrlRecordStream(RECORD_CTRL_RESUME,0,NULL);
			}
			else if (RECORD_CTRL_SETPLAYPOS == eRecordCtrl)
			{
				nRet = lpPlayBack->CtrlRecordStream(RECORD_CTRL_SETPLAYPOS,nCtrlData,NULL);
			}
			else
			{
				mgwlog("录像回放无效指令\n");
				nRet =0;
			}
		}
        return nRet;
	}
	return -1;
}
#endif

int CMediaItemDevice::PTZCtrl(string strDeviceID,char* lpSendBuf, DWORD dwBufSize)
{
	//CAutoLock lock(&m_csDevOpLock);
	if (-1 == m_hConHandle)
	{
		//if (0>ConnectDevice())
		{
			return -1;
		}
	}
#ifdef WIN32
    //查找ID
	map<string,CMediaItemReal*>::iterator iterReal = m_mapRealPool.find(strDeviceID);
	if((iterReal != m_mapRealPool.end()) && iterReal->second)
	{
		int nID = iterReal->second->GetChannelID();
		return m_pSdkProxy->fEDV_DEVICE_PTZCtrl(m_hConHandle, nID, (BYTE*)lpSendBuf, dwBufSize);
	}else{
		mgwlog("CMediaItemDevice::PTZCtrl 没有找到指定通道对象：%s\n",strDeviceID.c_str());
		return -1;
	}
#else
	mgwlog("CMediaItemDevice::PTZCtrl to do\n");
	return 0;
#endif
}

int CMediaItemDevice::MakeKeyFrame(string strDeviceID)
{
	//CAutoLock lock(&m_csDevOpLock);
	if (-1 == m_hConHandle)
	{
		//if (0>ConnectDevice())
		{
			return -1;
		}
	}
	
#ifdef WIN32
    //查找ID
	map<string,CMediaItemReal*>::iterator iterReal = m_mapRealPool.find(strDeviceID);
	if((iterReal != m_mapRealPool.end()) && iterReal->second)
	{
		int nID = iterReal->second->GetChannelID();
		return m_pSdkProxy->fEDV_DEVICE_MakeKeyFrame(m_hConHandle, nID);
	}else{
		mgwlog("CMediaItemDevice::MakeKeyFrame 没有找到指定通道对象：%s\n",strDeviceID.c_str());
		return -1;
	}
#else
	mgwlog("CMediaItemDevice::MakeKeyFrame to do\n");
	return 0;
#endif
}

//设置参数
int CMediaItemDevice::SetVideoParam(string strDeviceID, BYTE byType, BYTE byParam)
{
	//CAutoLock lock(&m_csDevOpLock);
	if (-1 == m_hConHandle)
	{
		//if (0>ConnectDevice())
		{
			mgwlog("CMediaItemDevice::SetVideoParam m_hConHandle==-1 exit\n");
			return -1;
		}
	}
#ifdef WIN32
	//查找ID
	map<string,CMediaItemReal*>::iterator iterReal = m_mapRealPool.find(strDeviceID);
	if((iterReal != m_mapRealPool.end()) && iterReal->second)
	{
		int nID = iterReal->second->GetChannelID();
		return m_pSdkProxy->fEDV_DEVICE_SetVideoParam(m_hConHandle, nID,byType,byParam);
	}else{
		mgwlog("CMediaItemDevice::SetVideoParam 没有找到指定通道对象：%s\n",strDeviceID.c_str());
		return -1;
	}
#else
	mgwlog("CMediaItemDevice::SetVideoParam to do\n");
	return 0;
#endif
}

//获取参数
int CMediaItemDevice::GetVideoParam(string strDeviceID,unsigned int& nParam)
{
	//CAutoLock lock(&m_csDevOpLock);
	if (-1 == m_hConHandle)
	{
		//if (0>ConnectDevice())
		{
			mgwlog("CMediaItemDevice::GetVideoParam m_hConHandle==-1 exit\n");
			return -1;
		}
	}
#ifdef WIN32
	//查找ID
	map<string,CMediaItemReal*>::iterator iterReal = m_mapRealPool.find(strDeviceID);
	if((iterReal != m_mapRealPool.end()) && iterReal->second)
	{
		int nID = iterReal->second->GetChannelID();
		return m_pSdkProxy->fEDV_DEVICE_GetVideoParam(m_hConHandle, nID,nParam);
	}else{
		mgwlog("CMediaItemDevice::GetVideoParam 没有找到指定通道对象：%s\n",strDeviceID.c_str());
		return -1;
	}
#else
	mgwlog("CMediaItemDevice::GetVideoParam to do\n");
	return 0;
#endif
}

//状态线程函数
int	CMediaItemDevice::OnAliveStatus()
{
	while (m_bThreadAliveStatusFlag)
	{
		static DWORD dwTime = GetTickCount();
		if((GetTickCount()-dwTime)>PRINT_TIME_SPAN)  //30s 
		{
           //mgwlog("++++线程工作中CMediaItemDevice::OnAliveStatus()handle:[0x%x]\n",this);
		   dwTime = GetTickCount();
		}
        //遍列下级通道
		map<string,CMediaItemReal*>::iterator iterUnit =m_mapRealPool.begin();
		while(iterUnit != m_mapRealPool.end())
		{
			//mgwlog("上报状态\n");
			SendAliveStatusMsg(iterUnit->second);
			iterUnit++;
		}
// 		map<int,CMediaItemReal*>::iterator iterUnit2 =m_mapRealWorkQueue.begin();
// 		while(iterUnit2 != m_mapRealWorkQueue.end())
// 		{
// 			//mgwlog("上报状态\n");
// 			SendAliveStatusMsg(iterUnit2->second);
// 			iterUnit2++;
// 		}
#ifdef WIN32
		Sleep(1000*10);   //10s 检测一次
#else
		sleep(10);
#endif
	}
	mgwlog("exit CMediaItemDevice::OnAliveStatus\n");
	return 0;	
}

//发生变化则上报消息
// 	<?xml version="1.0"?>
// 		<Notify>
// 		<CmdType>DeviceStatus</CmdType>
// 		<SN>43</SN>
// 		<DeviceID>XXXX<DeviceID>
// 		<Status>OFFLINE</Status>
// 		</Notify>
//上报点位状态
int CMediaItemDevice::SendAliveStatusMsg(CMediaItemReal* pUnit)
{
	//CAutoLock lock(&m_csDevOpLock);
	if(!CSysCfg::m_bDevInited)
	{
        //mgwlog("设备尚未初始化完成\n");
		return -1;
	}
	if (!pUnit)
	{
		mgwlog("通道句柄为空\n");
		return -1;
	}

	string strChState="";
	if (-1 == m_hConHandle)   //设备离线则下面通道都离线
	{
		strChState = "OFF";
	}else{
        CHANNELITEM_STATE eChannelState;
#ifdef WIN32
        if(!m_pSdkProxy->fEDV_DEVICE_GetChannelState(m_hConHandle,pUnit->GetChannelID(),eChannelState))  //获取成功
		{
			pUnit->SetCurChannelState(eChannelState);
		}
#endif
		strChState = CovertState2str(eChannelState);
	}

	//发生变化则上报
	if(FALSE == pUnit->NeedSendState()) //不需要上报
	{
		//mgwlog("点位没有发生变化\n");
		return -1;
	}

	int iRet =-1;
	SIP_MSG_INFO msg_info;
	ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
	sprintf(msg_info.CallerID,"%s",CSysCfg::Instance()->GetstrPara(SYS_CFG_MGWDEVID).c_str());
	sprintf(msg_info.CalleedID,"%s",CSysCfg::Instance()->GetstrPara(SYS_CFG_SERVERID).c_str());
	
	//创建一个XML的文档对象。
	TiXmlDocument *myDocument = new TiXmlDocument();
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
	myDocument->LinkEndChild(decl);
	
	//创建一个根元素(消息类型)并连接。
	TiXmlElement *RootElement = new TiXmlElement("Notify");
	myDocument->LinkEndChild(RootElement);
	
	//创建一个CmdType元素并连接
	TiXmlElement *xmlNode = new TiXmlElement("CmdType");
	RootElement->LinkEndChild(xmlNode);
	TiXmlText *xmlContent = new TiXmlText("Status");
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("SN");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText("1");
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("DeviceID");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText((pUnit->GetLogicDeviceID()).c_str());
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("Status");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText(strChState.c_str());
	xmlNode->LinkEndChild(xmlContent);
	
	//xml文档内容输出为字符串
	TiXmlPrinter printer;
	myDocument->Accept(&printer);
	string strSipMsg ="";
	strSipMsg = printer.CStr();
	int iLen = strSipMsg.length();
	if(iLen<512)
	{
		//mgwlog("::::make xmlmsg:\n%s\n",strSipMsg.c_str());
	}	
	msg_info.pMsg=NULL;
	msg_info.pMsg = new char[iLen+1];    //1024
	memset(msg_info.pMsg,0,iLen+1);
	sprintf(msg_info.pMsg,"%s",printer.CStr());
	msg_info.nMsgLen=strlen(msg_info.pMsg);
	iRet = CCtrlProOp::Instance()->SendMsg(msg_info);
	mgwlog("Unit点位[%s]状态，旧的:%d 当前:%d\n",(pUnit->GetLogicDeviceID()).c_str(),pUnit->GetOldChannelState(),pUnit->GetCurChannelState());
	
	MEMORY_DELETE(myDocument);
	MEMORY_DELETE_EX(msg_info.pMsg);
	pUnit->RefreshState();  //更新状态
	pUnit->SetSendStateMark(TRUE);
	return iRet;
}

#ifndef WIN32
//onvif设备
int CMediaItemDeviceOnvif::ConnectDevice()
{
	//加载成功,执行连接
	CONNECT_DEVICE_INFO DeviceInfo;
	ZeroMemory(&DeviceInfo, sizeof(CONNECT_DEVICE_INFO));
	DeviceInfo.eDeviceType = (DEVICE_TYPE)(m_stDevConfig.iDeviceType);
	DWORD  dwIP=0;
#ifdef WIN32
	SockObj::ConvertStringToIP(&dwIP,(char *)m_stDevConfig.strDeviceIP.c_str());
#else
	dwIP = iptoint((char *)m_stDevConfig.strDeviceIP.c_str());
#endif
	DeviceInfo.nDeviceIP = dwIP;
	DeviceInfo.nDevicePort = m_stDevConfig.iDevicePort;
	strcpy(DeviceInfo.sUserName, m_stDevConfig.strUseName.c_str());
	strcpy(DeviceInfo.sPassword, m_stDevConfig.strPassword.c_str());
    if(!m_pOnvifOp)
    {
    	m_pOnvifOp = new OnvifOp();
    	if(!m_pOnvifOp)
    	{
    		return -1;
    	}
    }
    if(m_pOnvifOp)
    {
    	m_pOnvifOp->SetPara((char*)m_stDevConfig.strDeviceIP.c_str(),
    			(char*)m_stDevConfig.strUseName.c_str(),
    			(char*)m_stDevConfig.strPassword.c_str(),
    			m_stDevConfig.iDevicePort);
    	m_pOnvifOp->StartCheckAlive();
    }
	return 0;
}

void CMediaItemDeviceOnvif::DisConnectDevice()
{
	if(m_pOnvifOp)
	{
		mgwlog("StopCheckAlive() 停止OnvifDevice保活检测\n");
		m_pOnvifOp->StopCheckAlive();
	}
	return ;
}

//上报点位状态
int CMediaItemDeviceOnvif::SendAliveStatusMsg(CMediaItemReal* pUnit)
{
	mgwlog("SendAliveStatusMsg:%s\n",m_stDevConfig.strDeviceIP.c_str());
	//CAutoLock lock(&m_csDevOpLock);
// 	if(!CSysCfg::m_bDevInited)
// 	{
//         //mgwlog("设备尚未初始化完成\n");
// 		return -1;
// 	}
	if (!pUnit)
	{
		mgwlog("通道句柄为空\n");
		return -1;
	}
	
	string strChState="";
	CHANNELITEM_STATE eChannelState;
	//GetChannelState(eChannelState);
	if(m_pOnvifOp)
	{
		if(m_pOnvifOp->GetAliveStatus()==0)   
		{
			eChannelState = CHANNELITEM_STATE_OPEN;
		}else{
			eChannelState = CHANNELITEM_STATE_OFFLINE;
		}
		pUnit->SetCurChannelState(eChannelState);     //用设备状态表示下面每个通道的状态
		strChState = CovertState2str(eChannelState);
	}else{
		mgwlog("m_pOnvifOp is NULL\n");
		return -1;
	}
	
	//发生变化则上报
	if(FALSE == pUnit->NeedSendState()) //不需要上报
	{
		//mgwlog("点位没有发生变化\n");
		return -1;
	}
	
	int iRet =-1;
	SIP_MSG_INFO msg_info;
	ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
	sprintf(msg_info.CallerID,"%s",CSysCfg::Instance()->GetstrPara(SYS_CFG_MGWDEVID).c_str());
	sprintf(msg_info.CalleedID,"%s",CSysCfg::Instance()->GetstrPara(SYS_CFG_SERVERID).c_str());
	
	//创建一个XML的文档对象。
	TiXmlDocument *myDocument = new TiXmlDocument();
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
	myDocument->LinkEndChild(decl);
	
	//创建一个根元素(消息类型)并连接。
	TiXmlElement *RootElement = new TiXmlElement("Notify");
	myDocument->LinkEndChild(RootElement);
	
	//创建一个CmdType元素并连接
	TiXmlElement *xmlNode = new TiXmlElement("CmdType");
	RootElement->LinkEndChild(xmlNode);
	TiXmlText *xmlContent = new TiXmlText("Status");
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("SN");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText("1");
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("DeviceID");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText((pUnit->GetLogicDeviceID()).c_str());
	xmlNode->LinkEndChild(xmlContent);
	
	xmlNode = new TiXmlElement("Status");
	RootElement->LinkEndChild(xmlNode);
	xmlContent = new TiXmlText(strChState.c_str());
	xmlNode->LinkEndChild(xmlContent);
	
	//xml文档内容输出为字符串
	TiXmlPrinter printer;
	myDocument->Accept(&printer);
	string strSipMsg ="";
	strSipMsg = printer.CStr();
	int iLen = strSipMsg.length();
	if(iLen<512)
	{
		//mgwlog("::::make xmlmsg:\n%s\n",strSipMsg.c_str());
	}	
	msg_info.pMsg=NULL;
	msg_info.pMsg = new char[iLen+1];    //1024
	memset(msg_info.pMsg,0,iLen+1);
	sprintf(msg_info.pMsg,"%s",printer.CStr());
	msg_info.nMsgLen=strlen(msg_info.pMsg);
	iRet = CCtrlProOp::Instance()->SendMsg(msg_info);
	mgwlog("Unit点位[%s]状态，旧的:%d 当前:%d\n",(pUnit->GetLogicDeviceID()).c_str(),pUnit->GetOldChannelState(),pUnit->GetCurChannelState());
	
	MEMORY_DELETE(myDocument);
	MEMORY_DELETE_EX(msg_info.pMsg);
	pUnit->RefreshState();  //更新状态
	pUnit->SetSendStateMark(TRUE);
	return iRet;
}

int CMediaItemDeviceOnvif::PTZCtrl(string strDeviceID,char* lpSendBuf, DWORD dwBufSize)
{
	if(m_pOnvifOp)
	{
		mgwlog("__Onvif_ptzctrl__\n");
		m_pOnvifOp->PTZCtrl((BYTE*)lpSendBuf, dwBufSize);
	}
	return 0;
}

int CMediaItemDeviceOnvif::MakeKeyFrame(string strDeviceID)
{
	if (-1 == m_hConHandle)
	{
		//if (0>ConnectDevice())
		{
			return -1;
		}
	}

	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetParent();
#ifdef WIN32
	return lpDevice->m_pSdkProxy->fEDV_DEVICE_MakeKeyFrame(m_hConnectHandle, m_nID);
#else
	mgwlog("CMediaItemDeviceOnvif::MakeKeyFrame() to do\n");
	return 0;
#endif
}

//设置参数
int CMediaItemDeviceOnvif::SetVideoParam(string strDeviceID, BYTE byType, BYTE byParam)
{
	if(m_pOnvifOp)
	{
		mgwlog("__Onvif_SetVideoParam__\n");
		m_pOnvifOp->SetVideoParam(byType,byParam);
	}
	return 0;
}

//获取参数
int CMediaItemDeviceOnvif::GetVideoParam(string strDeviceID,unsigned int& nParam)
{
	if(m_pOnvifOp)
	{
		mgwlog("__Onvif_GetVideoParam__\n");
		m_pOnvifOp->GetVideoParam(nParam);
	}
	return 0;
}

int CMediaItemDeviceOnvif::SetDevCfg(UNGB_DEVCONFIGREC &stDevConfig)
{
	m_stDevConfig = stDevConfig;
	if(m_pOnvifOp)
	{
		m_pOnvifOp->SetPara((char*)m_stDevConfig.strDeviceIP.c_str(),
			(char*)m_stDevConfig.strUseName.c_str(),
			(char*)m_stDevConfig.strPassword.c_str(),
			m_stDevConfig.iDevicePort);
    }
	return 0;
}

/////////////////
//Onvif视频节点
/////////////////
CMediaItemRealOnvif::CMediaItemRealOnvif(int nID) : CMediaItemReal(nID)
{
	m_pOnvifOp = NULL;
}

CMediaItemRealOnvif::~CMediaItemRealOnvif()
{
	StopStream(TRUE);
	MEMORY_DELETE(m_pOnvifOp);
}

// int CMediaItemUnitOnvif::GetStreamConInfo(STREAM_CON_INFO& stStreamConInfo)
// {
// 	stStreamConInfo = m_stStreamConInfo;
// 	return 0;
// }

int CMediaItemRealOnvif::ConnectDevice()
{
	//设备的连接信息在CMediaItemDevice中
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetParent();
	if (lpDevice)
	{
		if( 0 == lpDevice->ConnectDevice())
		{
			m_hConnectHandle = lpDevice->m_hConHandle;
			mgwlog("连接设备成功[%d]", m_hConnectHandle);
		}else{
			mgwlog("连接设备失败");
            return -1;
		}
	}else{
		return -1;
	}
	return 0;  //成功
}

void CMediaItemRealOnvif::DisConnectDevice()
{
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetParent();
	lpDevice->DisConnectDevice();
	m_hConnectHandle = -1;
}

//added by chenyu 131016
//打开类型
//虚打开  : 不创建发送线程 不发送码流
//正常打开: 发送码流
int CMediaItemRealOnvif::StartStream(STREAM_CON_INFO &stStreamConInfo,UNITOPENTYPE eOpenType)
{
    CAutoLock lock(&m_csRealOpLock);        //打开前Lock
    mgwlog("onvif不做Start码流处理\n");
    mgwlog("创建OnvifOp对象\n");
	if(!m_pOnvifOp)
	{
		m_pOnvifOp = new OnvifOp();
		if(!m_pOnvifOp)
		{
			return -1;
		}
		CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetParent();
		if (lpDevice)
		{
			m_pOnvifOp->SetPara((char*)lpDevice->m_stDevConfig.strDeviceIP.c_str(),
								 (char*)lpDevice->m_stDevConfig.strUseName.c_str(),
								 (char*)lpDevice->m_stDevConfig.strPassword.c_str(),
								 lpDevice->m_stDevConfig.iDevicePort);
		}else{
			return -1;
		}
	}
	if(m_pOnvifOp)
	{
		m_pOnvifOp->MakeKeyFrame();
	}
	return 0;
}

//是否完全关闭
void CMediaItemRealOnvif::StopStream(BOOL bReal)
{
	mgwlog("onvif不做Stop码流处理:");
    return ;
}

#endif

/////////////////
//实时视频
/////////////////
#ifdef WIN32
BOOL  CMediaItemReal::m_bPsPackInited = FALSE;
#endif
CMediaItemReal::CMediaItemReal(int nID) : CMediaItem(nID)
{
	m_eMediaType = MEDIAITEM_TYPE_UNIT;
	ZeroMemory(&m_StreamInfo, sizeof(REALSTREAM_INFO));
	m_StreamInfo.eChannelState = CHANNELITEM_STATE_CLOSE;  
	m_hOpenHandle = -1;
//	m_eDeviceType = DEVICE_TYPE_INVALID;
	m_dwTime = 0;
	m_llRcvByteCount = 0;
	m_llSendByteCount = 0;
	m_llLastSendByteCount = 0;

#ifdef WIN32
    m_bThreadPsPackFlag = FALSE;   //ps打包线程
	m_hThreadPsPack = 0;
	m_dwThreadPsPackID = 0;
	m_hPsPack = -1;

	m_bThreadSendFlag=FALSE;
	m_hThreadSend=0;
	m_dwThreadSendID=0;
	m_lpDataSend =NULL;
	m_lpSendBuf =NULL;
    m_lpPlayUDP =NULL;             //udp发送
	
	m_lpMediaDataOp =NULL;         //转码模块
	m_bHeadInfoInited =FALSE;
	m_pCPSPackaging = NULL;
	m_pPsPackBuf = NULL;
	m_pH264Buf = NULL;
	m_dwDataSize = 0;
	m_bFinishCheckStream = FALSE;
	m_nCheckStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //默认未知
	m_nStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //默认未知
#else
	pthread_mutex_init(&m_csRealOpLock,NULL);
#endif
	m_bStartMark = FALSE;
	m_eOpenType = UNIT_OPEN_EMPTY;   //空打开

	//---------------
	m_eOldChannelState = CHANNELITEM_STATE_CLOSE;   //通道状态
    m_eCurChannelState = CHANNELITEM_STATE_CLOSE;   //当前通道状态
	m_bSend = FALSE;
	m_fw = NULL;
	m_bCBYUVData = FALSE;
}

CMediaItemReal::~CMediaItemReal()
{
#ifdef WIN32
	StopStream(TRUE);
#endif
}

CMediaItemDevice* CMediaItemReal::GetDevice()
{
    CMediaItemDevice* lpDevice = NULL;
	lpDevice = (CMediaItemDevice *)GetParent();
	return lpDevice;
}

int CMediaItemReal::GetStreamConInfo(STREAM_CON_INFO& stStreamConInfo)
{
	stStreamConInfo = m_stStreamConInfo;
	return 0;
}

int CMediaItemReal::ConnectDevice()
{
	//设备的连接信息在CMediaItemDevice中
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetParent();
	if (lpDevice)
	{
		if( 0 == lpDevice->ConnectDevice())
		{
			m_hConnectHandle = lpDevice->GetConnectHandle();
			mgwlog("连接设备成功[%d]", m_hConnectHandle);	
		}else{
			mgwlog("连接设备失败");
            return -1;
		}
	}else{
		return -1;
	}
	return 0;  //成功
}

void CMediaItemReal::DisConnectDevice()
{
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetParent();
	lpDevice->DisConnectDevice();
	m_hConnectHandle = -1;
}

//added by chenyu 131016
//打开类型 
//虚打开  : 不创建发送线程 不发送码流
//正常打开: 发送码流
int CMediaItemReal::StartStream(STREAM_CON_INFO &stStreamConInfo,UNITOPENTYPE eOpenType)
{
	mgwlog("CMediaItemReal::StartStream enter\n");
	//1. 先关闭
	StopStream(TRUE);
    CAutoLock lock(&m_csRealOpLock);
	mgwlog("CMediaItemReal::StartStream after lock\n");
#ifdef WIN32

	if (UNIT_OPEN_EMPTY == eOpenType)  //虚打开
	{
		mgwlog("虚打开:%s[%s]\n",m_stChConfig.strLogicDeviceID.c_str(),m_stChConfig.strChannelName.c_str()); 
		m_dwTime = 0; 
		m_llRcvByteCount = 0;
		m_llSendByteCount = 0;
		m_llLastSendByteCount = 0;
		return 0;
	}
	m_stStreamConInfo = stStreamConInfo;
	mgwlog("打开unit:%s[%s] 本地端口:%d TSU端口:%d\n",
		m_stChConfig.strLogicDeviceID.c_str(),m_stChConfig.strChannelName.c_str(),
		m_stStreamConInfo.iSrcPort,m_stStreamConInfo.iDestPort);
      
	if (!m_lpSendBuf)
	{
		m_lpSendBuf = new byte[MAX_SENDBUF_LEN];
	}
	
	m_hConnectHandle = ((CMediaItemDevice *)GetParent())->GetConnectHandle();
	if (-1 == m_hConnectHandle)
	{
		if ( ConnectDevice() !=0 )   //失败
		{
            mgwlog("重新连接设备 fail\n");  
			return -1;
		}
	}
	
	//2.启动编解码线程  流输入到MediaDataOp中处理后，再从MediaDataOp读出。
	if(!m_lpMediaDataOp)
	{
		m_stCodecinfo.iStreamType = m_stChConfig.iStreamType;
		m_stCodecinfo.bNeedCodec = m_stChConfig.iNeedCodec;
		m_stCodecinfo.strLogicID = m_stChConfig.strLogicDeviceID;
		CMediaDataOp *lpMediaDataOp = new CMediaDataOp(m_stCodecinfo);
		lpMediaDataOp->StartCodec();	  //启动编解码
		m_lpMediaDataOp = lpMediaDataOp;
	}
	
	//3.先关闭已有的发送线程，创建新的码流发送线程
	if(m_hThreadSend)
	{   
		mgwlog("关闭旧的码流发送线程\n");
		m_bThreadSendFlag = FALSE;
		WaitForSingleObject(m_hThreadSend, INFINITE);
		CLOSE_HANDLE(m_hThreadSend);
		m_dwThreadSendID = 0;
	}
	MEMORY_DELETE(m_lpDataSend);
	mgwlog("创建新的码流发送线程\n");
	m_dwTime = GetTickCount();    //正常打开则开始计算码率 
	m_llRcvByteCount = 0;
	m_llSendByteCount = 0;
	m_llLastSendByteCount = 0;
	if (!m_hThreadSend)
	{
		//mgwlog("启动发送线程\n");
		m_bThreadSendFlag = TRUE;
		m_hThreadSend = CreateThread(NULL, 0, ThreadSend, (LPVOID)this, 0, &m_dwThreadSendID); //启动发送线程
	}

	if(!m_pCPSPackaging)
	{
        m_pCPSPackaging = new CPSPackaging();
		if(!m_pCPSPackaging)
		{
			return 0;
		}
	}
	
	if (!m_pPsPackBuf)
	{
		m_pPsPackBuf = new byte[MAX_PSBUFFER_SIZE];
	}
	if (!m_pH264Buf)
	{
		m_pH264Buf = new byte[MAX_PSBUFFER_SIZE];
	}
	
#ifdef SUP_PS_PACK_THREAD
	//3.1如果不是ps流则创建打包ps线程
	if (m_stChConfig.iStreamType !=0)
	{
		if(!m_hThreadPsPack)
		{
			m_bThreadPsPackFlag = TRUE;
			m_hThreadPsPack = CreateThread(NULL, 0, ThreadPsPack, (LPVOID)this, 0, &m_dwThreadPsPackID); //启动发送线程
		}
	}
#endif

	//4.打开流创建实时流回调
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetDevice();
	if(!lpDevice)
	{
		mgwlog("CMediaItemReal::StartStream 未找到设备对象，退出\n");
		return -1;
	}

	EDVOPENHANDLE handle = -1;
// 	if((DEVICE_TYPE_EV8K == m_stStreamConInfo.iDeviceType) && (1 == m_stChConfig.iNeedCodec))
// 	{
// 		m_bCBYUVData = TRUE;
// 	}
//	if(m_bCBYUVData)
	if( EV9000APP_VIDEOENCODE_TYPE_YUV == m_stChConfig.iStreamType)  //流类型YUV
	{
        mgwlog("设置YUV回调\n");
		handle = lpDevice->m_pSdkProxy->fEDV_DEVICE_OpenRealStream(m_hConnectHandle, m_nID, STREAM_TYPE_MAIN,STREAM_DATATYPE_YUV);  //回调YUV数据
	}else{
		mgwlog("设置正常码流回调\n");
	    handle = lpDevice->m_pSdkProxy->fEDV_DEVICE_OpenRealStream(m_hConnectHandle, m_nID, STREAM_TYPE_MAIN,STREAM_DATATYPE_COM,FALSE);  //正常回调
	}
 
	if (0 > handle)
	{
		mgwlog("打开设备通道图像失败退出,原因 handle:[%d] m_hConnectHandle:%d\n", handle,m_hConnectHandle);
		FreeRealResource(TRUE);  //释放资源退出 
		return -1;
	}else{
		mgwlog("打开设备通道图像成功 handle:[%d] m_hConnectHandle:%d\n", handle,m_hConnectHandle);
	}
	LONG lRet = lpDevice->m_pSdkProxy->fEDV_DEVICE_SetRealCallBack(handle, StreamDataCallBack, this);
	if(lRet>=0)
	{
		mgwlog("设置回调成功[%d]\n", lRet);
	}else{
		mgwlog("设置回调失败[%d] handle:%d\n", lRet,handle);
		return -1;
	}
	
	//5.修改状态 触发I帧
	m_hOpenHandle = handle;
	m_StreamInfo.eChannelState = CHANNELITEM_STATE_OPEN;
	m_stStreamConInfo.bConStatus = TRUE;
    m_eOpenType = eOpenType;  //更新打开状态
	if(lpDevice)
	{
		lpDevice->MakeKeyFrame(m_stChConfig.strLogicDeviceID);  //强制产生I帧
	}

	m_bStartMark = TRUE;  //已打开过
#else
	mgwlog("CMediaItemReal::StartStream onvif不做Start码流处理\n");
#endif
	return 0;
}

//释放资源
void CMediaItemReal::FreeRealResource(BOOL bReal)
{
	mgwlog("CMediaItemReal::FreeRealResource 释放资源\n");
#ifdef WIN32
    //1.关闭码流	
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetDevice();
	if(lpDevice && (m_hOpenHandle>=0))
	{
		mgwlog("CMediaItemReal::StopStream() CloseStream关闭流句柄\n");
		lpDevice->m_pSdkProxy->fEDV_DEVICE_SetRealCallBack(m_hOpenHandle, NULL, this);
		lpDevice->m_pSdkProxy->fEDV_DEVICE_CloseStream(m_hOpenHandle);
		m_hOpenHandle = -1;
		//lpDevice->m_pSdkProxy->fEDV_DEVICE_DisConnectDevice(m_hConnectHandle);  //不断开连接
	}
	
#ifdef SUP_PS_PACK_THREAD
	if(m_hThreadPsPack)
    {
		m_bThreadPsPackFlag = FALSE;
		WaitForSingleObject(m_hThreadPsPack, INFINITE);
        CLOSE_HANDLE(m_hThreadPsPack);
		m_dwThreadPsPackID = 0;
	}
#endif
	
	if(m_hThreadSend)
    {
		m_bThreadSendFlag = FALSE;
		WaitForSingleObject(m_hThreadSend, INFINITE);
        CLOSE_HANDLE(m_hThreadSend);
		m_dwThreadSendID = 0;
	}
	
#ifdef PS_PACK_THREAD
	if(m_hPsPack>=0)  //关闭打包
	{
		EV9KPsPack_Close(m_hPsPack);
		m_hPsPack = -1;
	}
#endif
	
	//2.清空相关状态及内存
	MEMORY_DELETE_EX(m_lpSendBuf);
	MEMORY_DELETE(m_lpMediaDataOp);
	MEMORY_DELETE(m_lpDataSend);
	MEMORY_DELETE(m_pCPSPackaging);   //释放 PsPack
	MEMORY_DELETE_EX(m_pPsPackBuf);
	MEMORY_DELETE_EX(m_pH264Buf);
	m_dwDataSize = 0;
	m_bCBYUVData = FALSE;
    m_stStreamConInfo.bConStatus = FALSE;
	m_StreamInfo.eChannelState = CHANNELITEM_STATE_CLOSE;
	m_bHeadInfoInited = FALSE;
	m_eOpenType = UNIT_OPEN_EMPTY;
	m_bStartMark = FALSE;
	m_dwTime = 0;  //发送时间清空 
	m_llRcvByteCount = 0;
	m_llSendByteCount = 0;
	m_llLastSendByteCount = 0;
	m_bFinishCheckStream = FALSE;
	m_nCheckStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //默认未知
	m_nStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //默认未知
	
	//3.关闭旧的会话dialog_index
	if(m_stStreamConInfo.stSipContext.idialog_index>0)
	{
		mgwlog("关闭旧的sip通信,dialog_index:%d\n",m_stStreamConInfo.stSipContext.idialog_index);
        SIP_SendBye(m_stStreamConInfo.stSipContext.idialog_index);
    }
	m_stStreamConInfo.Clear();
#else
	mgwlog("CMediaItemReal::FreeRealResource onvif不做Stop码流处理\n");
#endif
}

//是否完全关闭
void CMediaItemReal::StopStream(BOOL bReal)
{
	mgwlog("CMediaItemReal::StopStream enter\n");
	CAutoLock lock(&m_csRealOpLock);
	mgwlog("CMediaItemReal::StopStream after lock\n");
	mgwlog("CMediaItemReal::StopStream 关闭码流操作\n");
	FreeRealResource(bReal);  //释放资源
}

#ifdef WIN32
//处理收到的回调数据
//调用MediaDataOp模块处理数据
int CMediaItemReal::OnStreamData(EDVOPENHANDLE handle, LPDATA_INFO lpDataInfo, unsigned char* lpBuf, DWORD dwBufSize)
{
	m_llRcvByteCount +=dwBufSize;  //接收数据
 	//海康需要设置头
    if(!m_bHeadInfoInited 
		&& (m_stCodecinfo.iStreamType >= EV9000_STREAMDATA_TYPE_VIDEO_HIK)
		&& (m_stCodecinfo.iStreamType < EV9000_STREAMDATA_TYPE_VIDEO_DAH))
	{
		//收到Hik NET_DVR_SYSHEAD头则启动数据处理
		//EV8000           14 byte
		//long              4 byte
		//NET_DVR_SYSHEAD  40 byte 
		if(58 == lpDataInfo->nFrameHeadLen )   //14+4+40    
		{
			if(m_lpMediaDataOp)
			{
                mgwlog("CMediaItemReal::OnStreamData SetDataHead 设置海康解码头\n");
				m_lpMediaDataOp->SetDataHead(lpDataInfo);
				//根据海康回调头数据更新数据库类型  //modified by chenyu 1030
				int nStreamSubType = 2;
				HIK_MEDIAINFO *pHikHead = (HIK_MEDIAINFO*)(lpDataInfo->lpFrameHead+18);
				if((pHikHead->media_fourcc == 1212893236) 
					&& (pHikHead->system_format == 776)
					&& (pHikHead->video_format == 8196))
				{
					nStreamSubType = EV9000APP_VIDEOENCODE_TYPE_HIK; //hik001
				}
				else if((pHikHead->media_fourcc == 1212894537)
					&&(pHikHead->system_format == 1)
					&& (pHikHead->video_format == 1))
				{
					nStreamSubType = EV9000APP_VIDEOENCODE_TYPE_HIK; //hik002
				}else{
					nStreamSubType = EV9000APP_VIDEOENCODE_TYPE_H264;   //h264
				}
				//mgwlog("CMediaItemReal::OnStreamData 海康码流类型:%d\n",nStreamSubType);
                //暂不启用 chenyu 131031
				//更新库，更新内存 
// 				if(m_stChConfig.iStreamSubType != nStreamSubType)
// 				{
// 					mgwlog("更新流类型[%s]:%d\n",m_stChConfig.strChannelName.c_str(),nStreamSubType);
// 					m_stChConfig.iStreamSubType = nStreamSubType;
// 					CSysCfg::Instance()->UpdataChInfo(m_stChConfig);
// 				}
				m_bHeadInfoInited = TRUE;
			}
		}
	}
    //1.检测流类型
	if(FALSE == m_bFinishCheckStream)
	{
		int nRet = CheckStreamType(lpBuf,dwBufSize);
		if(-1 == nRet)  //检测失败退出，继续检测
		{
			mgwlog("自动检测流类型失败退出，继续检测\n");
			return 0;
		}else{
			m_nCheckStreamType = nRet;  //检测得到流类型
			if (m_nCheckStreamType != EV9000APP_VIDEOENCODE_TYPE_UNKNOW) //检测到确定类型
			{
				m_nStreamType = m_nCheckStreamType;
			}else{
				m_nStreamType = m_stChConfig.iStreamType;   //否则用配置类型
			}
			mgwlog("配置类型:%d 自动检测类型:%d 流最终判定类型:%d\n",m_stChConfig.iStreamType,m_nCheckStreamType,m_nStreamType);
			m_bFinishCheckStream  = TRUE;
		}
	}

	//2.流处理 如果自动检测流类型为未知则依据配置进行处理
	if(m_lpMediaDataOp)
	{
		//mgwlog("CMediaItemReal::OnStreamData inputdata 回调数据");
		if(	UNIT_OPEN_EMPTY != m_eOpenType)  //非空打开
		{
			if(EV9000APP_VIDEOENCODE_TYPE_YUV == m_nStreamType)  //YUV数据
			{
				if(lpDataInfo)
				{
					m_Frame.nHeight = lpDataInfo->nHeight;
					m_Frame.nWidth = lpDataInfo->nWidth;
					m_Frame.pbuffer = lpBuf;
					m_Frame.lenth = dwBufSize;
					m_lpMediaDataOp->InputYUVData(&m_Frame,0); //输入待处理数据 YUV
				}	
			}
			else if((EV9000APP_VIDEOENCODE_TYPE_H264 == m_nStreamType)||(2==m_nStreamType)) //如果是h264则转化为ps
			{
				DealH264(lpBuf,dwBufSize);
			}
			else if((EV9000_STREAMDATA_TYPE_PS == m_nStreamType)||(0==m_nStreamType)) //非标ps则转化为标准ps
			{
                //DealUnGBPs(lpBuf,dwBufSize);    //新版本，不规则ps流客户端处理
				m_lpMediaDataOp->InputData(lpBuf, dwBufSize);
			}
			else  //其它流直接输入MediaDataOp中处理
			{
                m_lpMediaDataOp->InputData(lpBuf, dwBufSize);
			}
		}
		WriteRecFile(lpBuf, dwBufSize); //写录像文件
	}
	return 0;
}

//检测流类型
/*  
*ps   ---- EV9000_STREAMDATA_TYPE_PS 
*h264 ---- EV9000APP_VIDEOENCODE_TYPE_H264
*未知 ---- EV9000APP_VIDEOENCODE_TYPE_UNKNOW
*失败 ---- -1
*/
int CMediaItemReal::CheckStreamType(unsigned char* lpBuf, DWORD dwBufSize)
{
	if(dwBufSize < 6)  
	{
		return -1;  //太短检测失败
	}
	if(dwBufSize>10)
	{
		mgwlog("流特征值：0x%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			lpBuf[0],lpBuf[1],lpBuf[2],lpBuf[3],lpBuf[4],lpBuf[5],lpBuf[6],lpBuf[7],lpBuf[8],lpBuf[9]);
	}
	if(lpBuf[0]==0 && lpBuf[1]==0 && lpBuf[2]==1 
		&& ( lpBuf[3]==0xBA     //Program Pack
		|| lpBuf[3]==0xE0     //pes vedio头
		|| lpBuf[3]==0xC0    //pes audio头
		|| lpBuf[3]==0xBB    //system头
		))
	{
		mgwlog("自动检测流类型为:ps\n");
		return EV9000_STREAMDATA_TYPE_PS;
	}
	else if(lpBuf[0]==0 && lpBuf[1]==0 && lpBuf[2]==0 && lpBuf[3]==0x1)
	{
		mgwlog("自动检测流类型为:h264\n");
		return EV9000APP_VIDEOENCODE_TYPE_H264;
	}
	else if((dwBufSize > 45) &&(lpBuf[40+0]==0 && lpBuf[40+1]==0 && lpBuf[40+2]==0 && lpBuf[40+3]==0x1)) //(可能有40字节头)
	{
		mgwlog("自动检测流类型为:h264(包含40字节头)\n");
		return EV9000APP_VIDEOENCODE_TYPE_H264;
	}
	else
	{
		mgwlog("自动检测流类型为:UnKnow\n");
		return EV9000APP_VIDEOENCODE_TYPE_UNKNOW;
	}
	return -1;
}

//H264处理
int CMediaItemReal::DealH264(unsigned char* lpBuf, DWORD dwBufSize)
{
	if (dwBufSize<6)  //太短数据异常
	{
		return 0;
	}
	if(m_pCPSPackaging)
	{
		//大华枪机升级后每帧多出32个字节头，去掉 20141125 chenyu
	    if( (lpBuf[0]!=0x0) 
			|| (lpBuf[1]!=0x0)
			|| (lpBuf[2]!=0x0) 
			|| (lpBuf[3]!=0x1))
		{
			//去除dah私有数据
			if((dwBufSize > 37) &&(lpBuf[32+0]==0 && lpBuf[32+1]==0 && lpBuf[32+2]==0 && lpBuf[32+3]==0x1)) //(可能有32字节头)
			{
				lpBuf+=32;
				dwBufSize -=32;
			}
			else if((dwBufSize > 45) &&(lpBuf[40+0]==0 && lpBuf[40+1]==0 && lpBuf[40+2]==0 && lpBuf[40+3]==0x1)) //(可能有40字节头)
			{
				lpBuf+=40;
				dwBufSize -=40;
			}
		}
		
		int nDestLen = 0; 
		memset(m_pPsPackBuf,0,MAX_PSBUFFER_SIZE);
		if((0x67 == *(lpBuf+4)) || (0x27 == *(lpBuf+4)) || (0x47 == *(lpBuf+4)) || ((dwBufSize > 45) && (0x27 == *(lpBuf+40+4))))  //I帧，I帧帧头 (可能有40字节头)
		{
			m_pCPSPackaging->Packet_I_frame((const char*)lpBuf,dwBufSize,(char*)m_pPsPackBuf,nDestLen,25,0,0,0);
			//mgwlog("----make I frame:%d:%d---\n",dwBufSize,nDestLen);
		}else{
			m_pCPSPackaging->Packet_P_frame((const char*)lpBuf,dwBufSize,(char*)m_pPsPackBuf,nDestLen);
			//mgwlog("----make P frame:%d:%d---\n",dwBufSize,nDestLen);
		}
		m_lpMediaDataOp->InputData(m_pPsPackBuf, nDestLen); //输入待处理数据  h264
		WriteRecFilePs(m_pPsPackBuf, nDestLen); //写录像文件
	}else{
		mgwlog("m_pCPSPackaging is NULL\n");
	}
	return 0;
}

// 非标ps处理
// int CMediaItemReal::DealUnGBPs(unsigned char* lpBuf, DWORD dwBufSize)
// {
// 	unsigned char*pH264Data = NULL;
// 	int iHeadLen=0,iH264Size=0;
//     //去除pes头 只处理pes包
// 	if(lpBuf[0]==0 && lpBuf[1]==0 && lpBuf[2]==1 
// 		&& (lpBuf[3]==0xC0    //pes audio头
// 		|| lpBuf[3]==0xE0    //pes vedio头
// 		))
// 	{
// 		pPES_HEADER_tag pPesHead = (pPES_HEADER_tag )(lpBuf);
// 		int iPesHeadLen = pPesHead->PES_header_data_length;
// 		iHeadLen = 9 + iPesHeadLen;  //指向pes头后的H264原始数据
// 	   	pH264Data = lpBuf+iHeadLen;
// 		iH264Size = dwBufSize-iHeadLen;
// 	    if(iH264Size<=0)
// 		{
// 			return 0;
// 		}
// 	}
// 	else
// 	{
// 		return 0;
// 	}
// 
//     if(0x67 == *(pH264Data+4)) //0x67 Sequece  保存数据
// 	{
// 		if(iH264Size<MAX_PSBUFFER_SIZE)
// 		{
// 			memcpy(m_pH264Buf+0,pH264Data,iH264Size);
// 			m_dwDataSize = iH264Size;
// 		}
// 	}
// 	else if (0x68 == *(pH264Data+4)) //0x68 Picture 保存数据
// 	{
// 		if(m_dwDataSize + iH264Size<MAX_PSBUFFER_SIZE)
// 		{
// 			memcpy(m_pH264Buf+m_dwDataSize,pH264Data,iH264Size);
// 			m_dwDataSize += iH264Size;
// 		}
// 	}
// 	else //非 Sequece Picture
// 	{
// 		//打包
// 		if(m_pCPSPackaging)
// 		{
// 			int nDestLen = 0; 
// 			memset(m_pPsPackBuf,0,MAX_PSBUFFER_SIZE);
// 			//I帧 和 IDR帧
// 			if(0x65 == pH264Data[4])
// // 				if((0x21 == pH264Data[4] && 0x88 == pH264Data[5])
// // 					|| (0x61 == pH264Data[4] && 0x88 == pH264Data[5])
// // 				|| (0x65 == pH264Data[4]))
// 			{   //和前面的 Sequece Picture 一起打包
// 				if(m_dwDataSize + iH264Size <MAX_PSBUFFER_SIZE)
// 				{
// 					memcpy(m_pH264Buf+m_dwDataSize,pH264Data,iH264Size);
// 					m_dwDataSize += iH264Size;
// 				}else{
// 					mgwlog("I帧过大，超过缓冲：614400\n");
// 				}
// 				m_pCPSPackaging->Packet_I_frame((const char*)m_pH264Buf,m_dwDataSize,(char*)m_pPsPackBuf,nDestLen,25,0,0,0);
// 				m_dwDataSize = 0;  //归零
// 			}
// 			else
// 			{   //打当前数据的包
// 				m_pCPSPackaging->Packet_P_frame((const char*)pH264Data,iH264Size,(char*)m_pPsPackBuf,nDestLen);
// 			}
// 			m_lpMediaDataOp->InputData(m_pPsPackBuf, nDestLen); //输入待处理数据  h264
// 			WriteRecFilePs(m_pPsPackBuf, nDestLen); //写录像文件
// 		}else{
// 			mgwlog("m_pCPSPackaging is NULL\n");
// 		}
// 	}	
// 	return 0;
// }

//旧的处理方法
//非标ps处理
int CMediaItemReal::DealUnGBPs(unsigned char* lpBuf, DWORD dwBufSize)
{
	unsigned char*pH264Data = NULL;
	int iHeadLen=0,iH264Size=0;
    //去除pes头 只处理pes包
	if(lpBuf[0]==0 && lpBuf[1]==0 && lpBuf[2]==1 
		&& (lpBuf[3]==0xC0    //pes audio头
		|| lpBuf[3]==0xE0    //pes vedio头
		))
	{
		pPES_HEADER_tag pPesHead = (pPES_HEADER_tag )(lpBuf);
		int iPesHeadLen = pPesHead->PES_header_data_length;
		iHeadLen = 9 + iPesHeadLen;  //指向pes头后的H264原始数据
		pH264Data = lpBuf+iHeadLen;
		iH264Size = dwBufSize-iHeadLen;
		if(iH264Size<=0)
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
	//打包
	if(m_pCPSPackaging)
	{
		int nDestLen = 0; 
		memset(m_pPsPackBuf,0,MAX_PSBUFFER_SIZE);
		if(0x67 == *(pH264Data+4))  //I帧，I帧帧头
		{
			m_pCPSPackaging->Packet_I_frame((const char*)pH264Data,iH264Size,(char*)m_pPsPackBuf,nDestLen,25,0,0,0);
			//mgwlog("----make I frame:%d:%d---\n",dwBufSize,nDestLen);
		}else{
			m_pCPSPackaging->Packet_P_frame((const char*)pH264Data,iH264Size,(char*)m_pPsPackBuf,nDestLen);
			//mgwlog("----make P frame:%d:%d---\n",dwBufSize,nDestLen);
		}
		m_lpMediaDataOp->InputData(m_pPsPackBuf, nDestLen); //输入待处理数据  h264
		WriteRecFilePs(m_pPsPackBuf, nDestLen); //写录像文件
	}else{
		mgwlog("m_pCPSPackaging is NULL\n");
	}
	return 0;
}

int CMediaItemReal::WriteRecFile(void*pbuffer,int iLen)
{
    //static FILE *fw =NULL;
	if(m_stChConfig.strLogicDeviceID == theApp.m_strRecordLogicID)
	{
		//写成文件
		if(!m_fw)
		{
			char szPath[256]={0};
			sprintf(szPath,".\\%srec.264",m_stChConfig.strLogicDeviceID.c_str());
			m_fw = fopen(szPath,"wb+");
			if(NULL == m_fw)
			{
				mgwlog("创建录像文件失败：%s\n",szPath);
				theApp.m_strRecordLogicID = "";
			}
		}
		if(m_fw) fwrite(pbuffer,1,iLen,m_fw);
	}
	if(theApp.m_strRecordLogicID == "")
	{
		if(m_fw)
		{
			fclose(m_fw);
			m_fw = NULL;
		}
	}
	return 0;
}

int CMediaItemReal::WriteRecFilePs(void*pbuffer,int iLen)
{
    static FILE *fw1 =NULL;
	if(m_stChConfig.strLogicDeviceID == theApp.m_strRecordLogicID)
	{
		//写成文件
		if(!fw1)
		{
			char szPath[256]={0};
			sprintf(szPath,".\\%srec.ps",m_stChConfig.strLogicDeviceID.c_str());
			fw1 = fopen(szPath,"wb+");
			if(NULL == fw1)
			{
				mgwlog("创建录像文件失败：%s\n",szPath);
				theApp.m_strRecordLogicID = "";
			}
		}
		if(fw1) fwrite(pbuffer,1,iLen,fw1);
	}
	if(theApp.m_strRecordLogicID == "")
	{
		if(fw1)
		{
			fclose(fw1);
			fw1 = NULL;
		}
	}
	return 0;
}

int CMediaItemReal::WriteRecFile2(void*pbuffer,int iLen)
{
    static FILE *fw2 =NULL;
	if(m_stChConfig.strLogicDeviceID == theApp.m_strRecordLogicID)
	{
		//写成文件
		if(!fw2)
		{
			char szPath[256]={0};
			sprintf(szPath,".\\%srec.264S",m_stChConfig.strLogicDeviceID.c_str());
			fw2 = fopen(szPath,"wb+");
			if(NULL == fw2)
			{
				mgwlog("创建录像文件失败：%s\n",szPath);
				theApp.m_strRecordLogicID = "";
			}
		}
		if(fw2) fwrite(pbuffer,1,iLen,fw2);
	}
	if(theApp.m_strRecordLogicID == "")
	{
		if(fw2)
		{
			fclose(fw2);
			fw2 = NULL;
		}
	}
	return 0;
}

int CMediaItemReal::OnSend()
{
    //初始化发送线程
	//待修改 码流指定网卡 chenyu 131024
	if (!m_lpDataSend)
	{
		m_lpDataSend =new CDataSend();
		RTP_PARA stRtppara;
		stRtppara.destport = m_stStreamConInfo.iDestPort;
		stRtppara.portbase = m_stStreamConInfo.iSrcPort;
		stRtppara.strLogicDeviceID = m_stChConfig.strLogicDeviceID;  //added by chenyu 131024
		stRtppara.strChannelName = m_stChConfig.strChannelName;
		char szIP[20]={0};
		SockObj::ConvertIPToString(m_stStreamConInfo.dwDestIP,szIP);
		stRtppara.ipstr = szIP;
		m_stStreamConInfo.strDestIP = szIP;
		if(0 != m_lpDataSend->StartSend(stRtppara,PROT_TYPE_RTP))  //rtp发送
		{
			MEMORY_DELETE(m_lpDataSend);
			mgwlog("Warning:Create CDataSend fail \n");
			m_hThreadSend =0;
			return -1;
		}
	}
	while (m_bThreadSendFlag)
	{
// 		static DWORD dwTime = GetTickCount();
// 		if((GetTickCount()-dwTime)>PRINT_TIME_SPAN)  //30s 
// 		{
//            //mgwlog("++++线程工作中CMediaItemReal::OnSend()handle:[0x%x]\n",this);
// 		   dwTime = GetTickCount();
// 		}
		if(m_lpMediaDataOp)
		{
			int nLen = 0;
// 			if(0 == m_stChConfig.iStreamType)  //ps
// 			{
// 				nLen = m_lpMediaDataOp->OutputData(m_lpSendBuf,MAX_SENDBUF_LEN);  //读取H264数据
// 				static int iCountSend=0;
// 				iCountSend++;
// 				if(iCountSend>100)
// 				{
// 					iCountSend =0;
// 					mgwlog("do m_lpMediaDataOp->OutputData\n");
// 				}
// 			}else{
// 				if(m_hPsPack>=0)
// 				{
// 					nLen = EV9KPsPack_OutputData(m_hPsPack,m_lpSendBuf,MAX_SENDBUF_LEN);  //读取H264数据
// 					static int iCountSend=0;
// 					iCountSend++;
// 					if(iCountSend>100)
// 					{
// 						iCountSend =0;
// 						mgwlog("do EV9KPsPack_OutputData\n");
// 					}
// 				}
// 			}
			nLen = m_lpMediaDataOp->OutputData(m_lpSendBuf,MAX_SENDBUF_LEN);  //读取H264数据
			if(nLen>0)
			{
				//mgwlog("CMediaItemReal::OnSend() 发送数据到客户端，len:%d\n",nLen);
				int iLoopSendCount = theApp.m_iLoopSendCount; 
				if ((iLoopSendCount>1) && (iLoopSendCount<100))
				{
					int iSendCount = 0;
					while (iLoopSendCount>0)
					{
						m_lpDataSend->SendData(m_lpSendBuf, nLen); //发送
						//WriteRecFile2(m_lpSendBuf, nLen); //写录像文件
						m_llSendByteCount += nLen;  //统计发送数据
						iLoopSendCount --;
						iSendCount ++;
						if(iSendCount>20)
						{
							iSendCount =0;
                            //Sleep(1);
						}
					}
				}else{
					m_lpDataSend->SendData(m_lpSendBuf, nLen); //发送
					//WriteRecFile2(m_lpSendBuf, nLen); //写录像文件
					m_llSendByteCount += nLen;  //统计发送数据
				}
			}else{
                Sleep(1);
			}
		}else{
			Sleep(20);
		}
	}
	//MEMORY_DELETE(m_lpDataSend);
	mgwlog("退出线程CMediaItemReal::OnSend()\n");
	return 0;
}

int CMediaItemReal::OnPsPack()
{
//     //初始化ps打包器
// 	if(!m_bPsPackInited)
// 	{
// 		int nRet =EV9KPsPack_Init();
// 		if(!nRet)
// 		{
// 			CMediaItemReal::m_bPsPackInited = TRUE;
// 		}
// 	}
// 	m_hPsPack= EV9KPsPack_Open();
// 	if(m_hPsPack == -1)
// 	{
// 		mgwlog("CMediaItemReal::OnPsPack() EV9KPsPack_Open fail\n");
// 		return -1;
// 	}
// 	char* lpBuf = new char[MAX_SENDBUF_LEN];
// 	//读取数据 打包
// 	while (m_bThreadPsPackFlag)
// 	{
// 		if(m_lpMediaDataOp)
// 		{
// 			int nLen = m_lpMediaDataOp->OutputData((unsigned char*)lpBuf,MAX_SENDBUF_LEN);  //读取H264数据
// 			if(nLen>0)
// 			{
// 				static int iCount=0;
// 				iCount++;
// 				if(iCount>100)
// 				{
// 					iCount =0;
// 					mgwlog("do EV9KPsPack_InputData\n");
// 
// 				}
// 				EV9KPsPack_InputData(m_hPsPack,lpBuf,nLen);  //输入H264
// 			}else{
//                 Sleep(10);
// 			}
// 		}else{
// 			Sleep(20);
// 		}
// 	}
// 	TRACE("++++++++++++++++++++++++++退出OnPsPack()+++++++++++++++\n");
	return 0;
}
#endif

//是否需要上报
BOOL CMediaItemReal::NeedSendState()
{
	//发生变化则上报
	if((m_eOldChannelState == m_eCurChannelState) && m_bSend)  //没有发生变化 并且上报过则不上报
	{
		//mgwlog("点位没有发生变化\n");
		return FALSE;
	}else{
		return TRUE;
	}
}


//通道号
int CMediaItemReal::GetChannelID()
{
    return m_nID;
}

int CMediaItemReal::SetChConfig(UNGB_CHCONFIGREC &stChConfig)
{
	m_stChConfig = stChConfig;
	return  0;
}

//获取通道信息
int CMediaItemReal::GetUnitInfo(UNITINFO &stUnitInfo)
{
	stUnitInfo.strLogicDeviceID = m_stChConfig.strLogicDeviceID;
	stUnitInfo.strChannelName = m_stChConfig.strChannelName;
	stUnitInfo.strDeviceIP = m_strDeviceIP;
	stUnitInfo.dwTime = m_dwTime;
	stUnitInfo.llRcvByteCount = m_llRcvByteCount;
	stUnitInfo.llSendByteCount = m_llSendByteCount;
	stUnitInfo.llLastSendByteCount = m_llLastSendByteCount;
	m_llLastSendByteCount = m_llSendByteCount;   //把当前值赋给last
	stUnitInfo.strDestIP = m_stStreamConInfo.strDestIP;
	stUnitInfo.iDestPort = m_stStreamConInfo.iDestPort;
	stUnitInfo.iSrcPort = m_stStreamConInfo.iSrcPort;
	stUnitInfo.eChannelState = m_eCurChannelState;
	return 0;
}

#ifdef WIN32
/////////////////
//录像回放节点
/////////////////
CPlayBack::CPlayBack(int nID) : CMediaItem(nID)
{
	m_nID = nID;
	sprintf(m_sName, "录像回放[%d]", nID);
	
	//ZeroMemory(&m_RecTable, sizeof(EDVDVRRECORDTABLE));
	m_hConnectHandle = -1;
	m_hOpenHandle = -1;

	m_nKeepAliveCount = 0;
	m_bRate = 0;
	m_bPause = FALSE;
	m_bEmpty = FALSE;

	m_bThreadSendFlag=FALSE;
	m_hThreadSend=0;
	m_dwThreadSendID=0;
	m_lpDataSend =NULL;
	iCount = 0;
	iFrameFailCount =0;
	m_fw =NULL;
	m_lpReadBuffer = NULL;
	m_dwLastSendPause = 0;
	m_dwLastSendResume = 0;
	m_pCPSPackaging = NULL;
	m_pPsPackBuf = NULL;
	m_lpMediaDataOp =NULL;         //转码模块
	m_uiDataTime = 0;
	m_bFinishCheckStream = FALSE;
	m_nCheckStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //默认未知
	m_nStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //默认未知
#ifndef WIN32
	pthread_mutex_init(&m_csPlayBackOpLock,NULL);
#endif
}

CPlayBack::~CPlayBack()
{
	Stop();
}

CMediaItemDevice* CPlayBack::GetDevice()
{
    CMediaItemDevice* lpDevice = NULL;
	lpDevice = (CMediaItemDevice *)GetParent();
	return lpDevice;
}

//获取通道信息
int CPlayBack::GetUnitInfo(UNITINFO &stUnitInfo)
{
	return 0;
}

int CPlayBack::SetChConfig(UNGB_CHCONFIGREC &stChConfig)
{
	m_stChConfig = stChConfig;
	return  0;
}

SYSTEMTIME Time_tToSystemTime(time_t t)
{
    tm temptm = *localtime(&t);
    SYSTEMTIME st = {1900 + temptm.tm_year, 
		1 + temptm.tm_mon, 
		temptm.tm_wday, 
		temptm.tm_mday, 
		temptm.tm_hour, 
		temptm.tm_min, 
		temptm.tm_sec, 
		0};
    return st;
}

LONG CPlayBack::Start()
{
// 	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetDevice();
// 	if(!lpDevice)
// 	{
// 		mgwlog("CPlayBack::Start 未找到设备对象，退出\n");
// 		return -1;
// 	}
// 	ConnectDevice();
// 	if (-1 == m_hConnectHandle)
// 	{//返回失败
// //		AddListInfo(LOG_INFO_LEVEL_FATAL, "[%s]连接设备失败", GetName());
// 		
// 		return -2;
// 	}
// 
// // 	char sTemp[8] = {'\0'};
// // 	memcpy(sTemp, m_lpUnitTable->sResved2, 4);
// // 	int nRSChannelID = atoi(sTemp);
// // 	m_hOpenHandle = OpenRecordStreamByName(m_hConnectHandle, nRSChannelID, &m_RecTable);
// // 	if (0 > m_hOpenHandle)
// // 	{
// // 		AddListInfo(LOG_INFO_LEVEL_FATAL, "[%s] 打开录像失败", GetName());
// // 
// // 		return -3;
// // 	}
// 
// 	lpDevice->m_pSdkProxy->fEDV_DEVICE_SetRecordCallBack(m_hOpenHandle, StreamDataCallBack, this);
// 		
// //	AddListInfo(LOG_INFO_LEVEL_MAIN, "[%s]录像成功,对应播放句柄[%d]", GetName(), m_hOpenHandle);
	return 0;
}

LONG CPlayBack::StartByTime()
{
	mgwlog("CPlayBack::StartByTime enter\n");
	//1.先关闭
	Stop();
	CAutoLock lock(&m_csPlayBackOpLock);
	mgwlog("CPlayBack::StartByTime after lock\n");
	m_dwLastSendPause = 0;
	m_dwLastSendResume = 0;
	m_bEmpty = FALSE;
	m_sStart = Time_tToSystemTime(m_stStreamConInfo.iStartTime);
	m_sStop = Time_tToSystemTime(m_stStreamConInfo.iStopTime);
	m_sPlay = Time_tToSystemTime(m_stStreamConInfo.iPlayTime);
	int nRet = 0; //默认成功
	do 
	{
		//1.启动编解码线程  流输入到MediaDataOp中处理后，再从MediaDataOp读出。
		if(!m_lpMediaDataOp)
		{
			m_stCodecinfo.iStreamType = m_stChConfig.iStreamType;
			m_stCodecinfo.bNeedCodec = m_stChConfig.iNeedCodec;
			m_stCodecinfo.strLogicID = m_stChConfig.strLogicDeviceID;
			m_stCodecinfo.nDataType = 1;  //录像数据标志
			CMediaDataOp *lpMediaDataOp = new CMediaDataOp(m_stCodecinfo);
			lpMediaDataOp->StartCodec();	  //启动编解码
			m_lpMediaDataOp = lpMediaDataOp;
		}

		if (!m_lpReadBuffer)
		{
			m_lpReadBuffer = new byte[MAX_READ_LEN];
			if(!m_lpReadBuffer)
			{
				mgwlog("CPlayBack::StartByTime m_lpReadBuffer new fail\n");
				nRet = -1;
				break;
			}
		}

		if(!m_pCPSPackaging)
		{
			m_pCPSPackaging = new CPSPackaging();
			if(!m_pCPSPackaging)
			{
				nRet = -1;
				break;
			}
		}
		
		if (!m_pPsPackBuf)
		{
			m_pPsPackBuf = new byte[MAX_PSBUFFER_SIZE];
		}

		//2.建立发送线程
		mgwlog("创建录像码流发送线程\n");
		if (!m_hThreadSend)
		{
			//mgwlog("启动发送线程\n");
			m_bThreadSendFlag = TRUE;
			m_hThreadSend = CreateThread(NULL, 0, ThreadSend, (LPVOID)this, 0, &m_dwThreadSendID); //启动发送线程
		}

		//3.打开录像创建流回调
		CMediaItemDevice *lpDevice = NULL; 
		lpDevice = (CMediaItemDevice *)GetDevice();
		if (!lpDevice)
		{
			nRet = -1;
			break;
		}
		//获取流类型
		int iDeviceType = lpDevice->GetDeviceType();
		if (-1 == lpDevice->GetConnectHandle())
		{
			if( 0 == lpDevice->ConnectDevice())
			{
				m_hConnectHandle = lpDevice->GetConnectHandle();
				mgwlog("连接设备成功[%d]", m_hConnectHandle);	
			}else{
				mgwlog("连接设备失败");
				nRet = -1;
				break;
			}
		}
		m_hConnectHandle = lpDevice->GetConnectHandle();
		//m_eDeviceType = table.eDeviceType;
		EDVOPENHANDLE handle = -1;
		if (DEVICE_TYPE_EV8K == iDeviceType)
		{	
			mgwlog("CPlayBack::StartByTime() Ev8k sdk\n");
			//handle = lpDevice->m_pSdkProxy->fEDV_DEVICE_OpenRecordStreamEx(m_hConnectHandle, m_nID, m_sStart, m_sStop,m_sPlay);
			if( EV9000APP_VIDEOENCODE_TYPE_YUV == m_stChConfig.iStreamType)  //流类型YUV
			{
				mgwlog("CPlayBack::StartByTime() 设置YUV回调\n");
				handle = lpDevice->m_pSdkProxy->fEDV_DEVICE_OpenRecordStreamEx(m_hConnectHandle, m_nID, m_sStart, m_sStop,m_sPlay,STREAM_DATATYPE_YUV);  //回调YUV数据
			}else{
				mgwlog("CPlayBack::StartByTime() 设置正常码流回调\n");
				handle = lpDevice->m_pSdkProxy->fEDV_DEVICE_OpenRecordStreamEx(m_hConnectHandle, m_nID, m_sStart, m_sStop,m_sPlay,STREAM_DATATYPE_COM);  //正常回调
			}
		}else if(DEVICE_TYPE_BOSCH == iDeviceType)
		{
			mgwlog("CPlayBack::StartByTime() BOSCH 设置正常码流回调\n");
			handle = lpDevice->m_pSdkProxy->fEDV_DEVICE_OpenRecordStreamEx(m_hConnectHandle, m_nID, m_sStart, m_sStop,m_sPlay,STREAM_DATATYPE_COM);  //正常回调
		}else{
			mgwlog("CPlayBack::StartByTime() other sdk\n");
			handle = lpDevice->m_pSdkProxy->fEDV_DEVICE_OpenRecordStream(m_hConnectHandle, m_nID, m_sStart, m_sStop);  //正常回调  
		}
		if (0 > handle)
		{
			mgwlog("打开设备[%s],通道[%d]录像失败,原因[%d]", GetName(), m_nID, handle);
			nRet = -1;
			break;
		}
		
		int lTmpRet = lpDevice->m_pSdkProxy->fEDV_DEVICE_SetRecordCallBack(handle, StreamDataCallBack, this);
		if(lTmpRet<0)
		{
			mgwlog("设置录像回调失败[%d] handle:%d\n", lTmpRet,handle);
			nRet = -1;
			break;
		}
		m_hOpenHandle = handle;
		mgwlog("打开设备[%s],通道[%d]录像成功,对应播放句柄[%d]", GetName(), m_nID, handle);					
	} while (0);
	return nRet;
}

void CPlayBack::Stop()
{
	mgwlog("CPlayBack::Stop enter\n");
	CAutoLock lock(&m_csPlayBackOpLock);
	mgwlog("CPlayBack::Stop after lock\n");
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetDevice();
	if(!lpDevice)
	{
		mgwlog("CPlayBack::Stop 未找到设备对象，退出\n");
		return;
	}
	mgwlog("CPlayBack::Stop Start to SetRecordCallBack\n");
	if(m_hOpenHandle!=-1)
	{
		//lpDevice->m_pSdkProxy->fEDV_DEVICE_SetRecordCallBack(m_hOpenHandle, NULL, NULL);  //注销 chenyu 140703
        mgwlog("CPlayBack::Stop Start to CloseStream\n");
		lpDevice->m_pSdkProxy->fEDV_DEVICE_CloseStream(m_hOpenHandle);
		m_hOpenHandle = -1;
	}
    mgwlog("CPlayBack::Stop Start to Stop Send\n");
	if(m_hThreadSend)
    {
		m_bThreadSendFlag = FALSE;
		WaitForSingleObject(m_hThreadSend, INFINITE);
        CLOSE_HANDLE(m_hThreadSend);
		m_dwThreadSendID = 0;
	}
	MEMORY_DELETE(m_lpDataSend);
	MEMORY_DELETE_EX(m_lpReadBuffer);
	MEMORY_DELETE(m_lpMediaDataOp);
	MEMORY_DELETE(m_pCPSPackaging);   //释放 PsPack
	MEMORY_DELETE_EX(m_pPsPackBuf);
	m_uiDataTime = 0;
	m_bPause = FALSE;
	m_bEmpty = FALSE;
	m_bFinishCheckStream = FALSE;
	m_nCheckStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //默认未知
	m_nStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //默认未知

	if(m_fw)
	{
		fclose(m_fw);
		m_fw = 0;
	}
	mgwlog("leave CPlayBack::Stop()\n");
}

int CPlayBack::CtrlRecordStream(RECORD_CTRL eRecordCtrl, int nCtrlData, int* nReturnData)
{	
	CAutoLock lock(&m_csPlayBackOpLock);
	if(-1 == m_hOpenHandle)
	{
		mgwlog("CPlayBack::CtrlRecordStream m_hOpenHandle==-1 退出\n");
		return -1;
	}
	BOOL bCtrlFront = FALSE;
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetDevice();
	if(!lpDevice)
	{
		mgwlog("CPlayBack::CtrlRecordStream 未找到设备对象，退出\n");
		return -1;
	}
	if (RECORD_CTRL_SETPLAYPOS == eRecordCtrl)
	{
		bCtrlFront = TRUE;
		m_bPause = FALSE;
		m_bEmpty = TRUE;    //通知回调线程也清一下
	}
    else if (RECORD_CTRL_PAUSE == eRecordCtrl)
	{
		m_bPause = TRUE;
	}
	else if (RECORD_CTRL_RESUME == eRecordCtrl)
	{
		m_bPause = FALSE;
	}
	else
	{
		//m_bPause = FALSE;
	}

	if(bCtrlFront)
	{
		if (RECORD_CTRL_SETPLAYPOS == eRecordCtrl)
		{
	       mgwlog("执行seek命令\n");		
		}
		return lpDevice->m_pSdkProxy->fEDV_DEVICE_CtrlRecordStream(m_hOpenHandle, eRecordCtrl, nCtrlData, nReturnData);		
	}else{
		return 0;
	}
}

int CPlayBack::CtrlRecordStreamFront(RECORD_CTRL eRecordCtrl, int nCtrlData, int* nReturnData)
{	
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetDevice();
	if(!lpDevice)
	{
		mgwlog("CPlayBack::CtrlRecordStream 未找到设备对象，退出\n");
		return -1;
	}
	if(m_hOpenHandle!=-1)
	{
		return lpDevice->m_pSdkProxy->fEDV_DEVICE_CtrlRecordStream(m_hOpenHandle, eRecordCtrl, nCtrlData, nReturnData);	
	}else{
		return -1;
	}
}

int CPlayBack::WriteRecFile(void*pbuffer,int iLen)
{
    //static FILE *fw =NULL;
	if(m_stChConfig.strLogicDeviceID == theApp.m_strRecordLogicID)
	{
		//写成文件
		if(!m_fw)
		{
			char szPath[256]={0};
			sprintf(szPath,".\\%srec.264",m_stChConfig.strLogicDeviceID.c_str());
			m_fw = fopen(szPath,"wb+");
			if(NULL == m_fw)
			{
				mgwlog("创建录像文件失败：%s\n",szPath);
				theApp.m_strRecordLogicID = "";
			}
		}
		if(m_fw) fwrite(pbuffer,1,iLen,m_fw);
	}
	if(theApp.m_strRecordLogicID == "")
	{
		if(m_fw)
		{
			fclose(m_fw);
			m_fw = NULL;
		}
	}
	return 0;
}

int CPlayBack::OnStreamData(EDVOPENHANDLE handle, LPDATA_INFO lpDataInfo, unsigned char* lpBuf, DWORD dwBufSize)
{
	//Warning:回调函数不能暂停 否则会导致CChannelItemRecordEV8K::OnRecordData不能出来
	if((!lpBuf)||(dwBufSize <=0)||(!lpDataInfo)||(!m_lpReadBuffer))
	{
		return -1;
	}

	//1.检测流类型
	if(FALSE == m_bFinishCheckStream)
	{
		int nRet = CheckStreamType(lpBuf,dwBufSize);
		if(-1 == nRet)  //检测失败退出，继续检测
		{
			mgwlog("自动检测流类型失败退出，继续检测\n");
			return 0;
		}else{
			m_nCheckStreamType = nRet;  //检测得到流类型
			if (m_nCheckStreamType != EV9000APP_VIDEOENCODE_TYPE_UNKNOW) //检测到确定类型
			{
				m_nStreamType = m_nCheckStreamType;
			}else{
				m_nStreamType = m_stChConfig.iStreamType;   //否则用配置类型
			}
			mgwlog("PlayBack:配置类型:%d 自动检测类型:%d 流最终判定类型:%d\n",m_stChConfig.iStreamType,m_nCheckStreamType,m_nStreamType);
			m_bFinishCheckStream  = TRUE;
		}
	}

	if(m_lpMediaDataOp)  //回调录像码流处理
	{
		if (m_bEmpty)
		{
			m_lpMediaDataOp->ClearBuff();
			m_bEmpty = FALSE;
			mgwlog("清除录像缓冲\n");
		}
		//mgwlog("CMediaItemReal::OnStreamData inputdata 回调数据");
		if(EV9000APP_VIDEOENCODE_TYPE_YUV == m_nStreamType)  //YUV数据
		{
			if(lpDataInfo)
			{
				m_Frame.nHeight = lpDataInfo->nHeight;
				m_Frame.nWidth = lpDataInfo->nWidth;
				m_Frame.pbuffer = lpBuf;
				m_Frame.lenth = dwBufSize;
				m_lpMediaDataOp->InputYUVData(&m_Frame,0); //输入待处理数据 YUV
			}	
		}
		else if((EV9000APP_VIDEOENCODE_TYPE_H264 == m_nStreamType)||(2==m_nStreamType)) //如果是h264则转化为ps
		{
			DealH264(lpBuf,dwBufSize);
		}
		else if((EV9000_STREAMDATA_TYPE_PS == m_nStreamType)||(0==m_nStreamType)) //非标ps则转化为标准ps
		{
			//DealUnGBPs(lpBuf,dwBufSize);    //新版本，不规则ps流客户端处理
			m_lpMediaDataOp->InputData(lpBuf, dwBufSize);
		}
		else  //其它流直接输入MediaDataOp中处理
		{
			m_lpMediaDataOp->InputData(lpBuf, dwBufSize);
		}
		m_uiDataTime = lpDataInfo->uiDataTime;  //保存当前码流时间
	}
	//暂停
	int nOutBuffDataLen = m_lpMediaDataOp->GetOutBuffDataLen();
	if(nOutBuffDataLen>(REC_RINGBUF_LEN*2/3))  //>2/3
	{
		DWORD dwTime = GetTickCount();
		if((dwTime - m_dwLastSendPause)> 60)  //100ms
		{
			int nRet = CtrlRecordStreamFront(RECORD_CTRL_PAUSE,0,NULL);
			mgwlog("缓冲超过2/3 缓冲size:%d kb,发送暂停命令,执行结果:%d\n",nOutBuffDataLen/1024,nRet);
			m_dwLastSendPause = dwTime;
		}
	}
	return 0;
}

//检测流类型
/*  
*ps   ---- EV9000_STREAMDATA_TYPE_PS 
*h264 ---- EV9000APP_VIDEOENCODE_TYPE_H264
*未知 ---- EV9000APP_VIDEOENCODE_TYPE_UNKNOW
*失败 ---- -1
*/
int CPlayBack::CheckStreamType(unsigned char* lpBuf, DWORD dwBufSize)
{
	if(dwBufSize < 6)  
	{
		return -1;  //太短检测失败
	}
	if(lpBuf[0]==0 && lpBuf[1]==0 && lpBuf[2]==1 
		&& ( lpBuf[3]==0xBA     //Program Pack
		|| lpBuf[3]==0xE0     //pes vedio头
		|| lpBuf[3]==0xC0    //pes audio头
		|| lpBuf[3]==0xBB    //system头
		))
	{
		mgwlog("PlayBack:自动检测流类型为:ps\n");
		return EV9000_STREAMDATA_TYPE_PS;
	}
	else if(lpBuf[0]==0 && lpBuf[1]==0 && lpBuf[2]==0 && lpBuf[3]==0x1)
	{
		mgwlog("PlayBack:自动检测流类型为:h264\n");
		return EV9000APP_VIDEOENCODE_TYPE_H264;
	}
	else if((dwBufSize > 45) &&(lpBuf[40+0]==0 && lpBuf[40+1]==0 && lpBuf[40+2]==0 && lpBuf[40+3]==0x1)) //(可能有40字节头)
	{
		mgwlog("PlayBack:自动检测流类型为:h264(包含40字节头)\n");
		return EV9000APP_VIDEOENCODE_TYPE_H264;
	}
	else
	{
		mgwlog("PlayBack:自动检测流类型为:UnKnow\n");
		return EV9000APP_VIDEOENCODE_TYPE_UNKNOW;
	}
	return -1;
}

//H264处理
int CPlayBack::DealH264(unsigned char* lpBuf, DWORD dwBufSize)
{
	if(m_pCPSPackaging)
	{
		int nDestLen = 0; 
		memset(m_pPsPackBuf,0,MAX_PSBUFFER_SIZE);
		if((0x67 == *(lpBuf+4)) || (0x27 == *(lpBuf+4)) ||(0x47 == *(lpBuf+4)) || ((dwBufSize > 45) && (0x27 == *(lpBuf+40+4))))  //I帧，I帧帧头 (可能有40字节头)
		{
			m_pCPSPackaging->Packet_I_frame((const char*)lpBuf,dwBufSize,(char*)m_pPsPackBuf,nDestLen,25,0,0,0);
			//mgwlog("----make I frame:%d:%d---\n",dwBufSize,nDestLen);
		}else{
			m_pCPSPackaging->Packet_P_frame((const char*)lpBuf,dwBufSize,(char*)m_pPsPackBuf,nDestLen);
			//mgwlog("----make P frame:%d:%d---\n",dwBufSize,nDestLen);
		}
		m_lpMediaDataOp->InputData(m_pPsPackBuf, nDestLen); //输入待处理数据  h264
		//WriteRecFilePs(m_pPsPackBuf, nDestLen); //写录像文件
	}else{
		mgwlog("m_pCPSPackaging is NULL\n");
	}
	return 0;
}

//非标ps处理
int CPlayBack::DealUnGBPs(unsigned char* lpBuf, DWORD dwBufSize)
{
	unsigned char*pH264Data = NULL;
	int iHeadLen=0,iH264Size=0;
    //去除pes头 只处理pes包
	if(lpBuf[0]==0 && lpBuf[1]==0 && lpBuf[2]==1 
		&& (lpBuf[3]==0xC0    //pes audio头
		|| lpBuf[3]==0xE0    //pes vedio头
		))
	{
		pPES_HEADER_tag pPesHead = (pPES_HEADER_tag )(lpBuf);
		int iPesHeadLen = pPesHead->PES_header_data_length;
		iHeadLen = 9 + iPesHeadLen;  //指向pes头后的H264原始数据
	   	pH264Data = lpBuf+iHeadLen;
		iH264Size = dwBufSize-iHeadLen;
	    if(iH264Size<=0)
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
	//打包
	if(m_pCPSPackaging)
	{
		int nDestLen = 0; 
		memset(m_pPsPackBuf,0,MAX_PSBUFFER_SIZE);
		if(0x67 == *(pH264Data+4))  //I帧，I帧帧头
		{
			m_pCPSPackaging->Packet_I_frame((const char*)pH264Data,iH264Size,(char*)m_pPsPackBuf,nDestLen,25,0,0,0);
			//mgwlog("----make I frame:%d:%d---\n",dwBufSize,nDestLen);
		}else{
			m_pCPSPackaging->Packet_P_frame((const char*)pH264Data,iH264Size,(char*)m_pPsPackBuf,nDestLen);
			//mgwlog("----make P frame:%d:%d---\n",dwBufSize,nDestLen);
		}
		m_lpMediaDataOp->InputData(m_pPsPackBuf, nDestLen); //输入待处理数据  h264
		//WriteRecFilePs(m_pPsPackBuf, nDestLen); //写录像文件
	}else{
		mgwlog("m_pCPSPackaging is NULL\n");
	}
	return 0;
}

int CPlayBack::OnSend()
{
    //初始化发送线程
	//待修改 码流指定网卡 chenyu 131024
	mgwlog("CPlayBack::OnSend\n");
	if (!m_lpDataSend)
	{
		m_lpDataSend =new CDataSend();
		RTP_PARA stRtppara;
		stRtppara.destport = m_stStreamConInfo.iDestPort;
		stRtppara.portbase = m_stStreamConInfo.iSrcPort;
		stRtppara.strLogicDeviceID = m_stChConfig.strLogicDeviceID;  //added by chenyu 131024
		stRtppara.strChannelName = m_stChConfig.strChannelName;
		char szIP[20]={0};
		SockObj::ConvertIPToString(m_stStreamConInfo.dwDestIP,szIP);
		stRtppara.ipstr = szIP;
		m_stStreamConInfo.strDestIP = szIP;
		if(0 != m_lpDataSend->StartSend(stRtppara,PROT_TYPE_RTP))  //RTP 发送
		{
			MEMORY_DELETE(m_lpDataSend);
			mgwlog("Warning:Create CDataSend fail \n");
			m_hThreadSend =0;
			return -1;
		}
	}
	if(theApp.m_iConSendPackLen<=0) 
	{
		theApp.m_iConSendPackLen = 10;   //1000包/second
	}
	if(theApp.m_iConSendPackLen>=(MAX_READ_LEN/MAX_RTP_PAYLOAD_LEN))
	{
		theApp.m_iConSendPackLen = (MAX_READ_LEN/MAX_RTP_PAYLOAD_LEN);
	}
	while (m_bThreadSendFlag)
	{
		if(m_lpMediaDataOp)
		{
			//检测缓冲中数据
			int nOutBuffDataLen = m_lpMediaDataOp->GetOutBuffDataLen();
			if(nOutBuffDataLen<(REC_RINGBUF_LEN*1/4) )  //<1/4
			{
				DWORD dwTime = GetTickCount();
				if((dwTime - m_dwLastSendResume)> 60)  //60ms
				{
					int nRet = CtrlRecordStreamFront(RECORD_CTRL_RESUME,0,NULL);
					//mgwlog("缓冲低于1/4 缓冲size:%d,发送继续命令,执行结果:%d\n",m_lpFrameQue->GetCount(),nRet);
					m_dwLastSendResume = dwTime;
				}
			}
			while (m_bPause && m_bThreadSendFlag)
			{
				Sleep(10);
				continue;
			}
			int nLen = 0;
			nLen = m_lpMediaDataOp->OutputData(m_lpReadBuffer,theApp.m_iConSendPackLen*MAX_RTP_PAYLOAD_LEN);  //读取H264数据
			if(nLen>0)
			{
				m_lpDataSend->SendData(m_lpReadBuffer,nLen,1,m_uiDataTime); 
				Sleep(10);
				//WriteRecFile(m_lpReadBuffer,nLen);
			}else{
                Sleep(10);
			}
		}else{
			Sleep(10);
		}
	}
	mgwlog("退出线程CPlayBack::OnSend()\n");
	return 0;	
}

// int CPlayBack::OnStreamData(EDVOPENHANDLE handle, LPDATA_INFO lpDataInfo, unsigned char* lpBuf, DWORD dwBufSize)
// {
// 	//Warning:回调函数不能暂停 否则会导致CChannelItemRecordEV8K::OnRecordData不能出来
// 	if((!lpBuf)||(dwBufSize <=0)||(!lpDataInfo)||(!m_lpFrameQue)||(!m_lpInData)||(!m_lpReadBuffer))
// 	{
// 		return -1;
// 	}
// 	//写入缓冲
// 	if(m_lpInData)
// 	{
// 		int nLen = 0;
// 		int nDataLen=0;
// 		if((EV9000APP_VIDEOENCODE_TYPE_H264 == m_stChConfig.iStreamType)||(2==m_stChConfig.iStreamType)) //如果是h264则转化为ps
// 		{
// 			if(m_pCPSPackaging)
// 			{
// 				int nDestLen = 0; 
// 				memset(m_pPsPackBuf,0,MAX_PSBUFFER_SIZE);
// 				if(FRAME_DATA_TYPE_IFRAME == lpDataInfo->eDataType)  //I帧，I帧帧头
// 				{
// 					m_pCPSPackaging->Packet_I_frame((const char*)lpBuf,dwBufSize,(char*)m_pPsPackBuf,nDestLen,25,0,0,0);
// 					//mgwlog("----make I frame:%d:%d---\n",dwBufSize,nDestLen);
// 				}else{
// 					m_pCPSPackaging->Packet_P_frame((const char*)lpBuf,dwBufSize,(char*)m_pPsPackBuf,nDestLen);
// 					//mgwlog("----make P frame:%d:%d---\n",dwBufSize,nDestLen);
// 				}
// 				nLen = m_lpInData->Write(m_pPsPackBuf,nDestLen);  //输入ps
// 				nDataLen = nDestLen;
// 			}else{
// 				mgwlog("m_pCPSPackaging is NULL\n");
// 			}
// 		}else{
// 			nLen = m_lpInData->Write(lpBuf,dwBufSize); //输入ps
// 			nDataLen = dwBufSize;
// 		}
// 
// 		if(nLen < nDataLen)
// 		{
// 			mgwlog("CPlayBack::OnStreamData 写入数据溢出,本次写入:%d 成功:%d \n",dwBufSize,nLen);	
// // 			m_nOverflowNum++;
// // 			if(0 == m_nOverflowNum%1000)
// // 			{
// // 				mgwlog("CMediaDataOp::InputData 写入数据溢出,本次写入:%d 成功:%d 总溢出次数:%d\n",dwBufSize,nLen,m_nOverflowNum);
// // 			}
// 		}
// 	}
// 
// 	if (m_lpFrameQue)
// 	{
// 		if (m_bEmpty)
// 		{
// 			CAutoLock lock(&m_csFrameQueLock);
// 			m_lpFrameQue->FreeAll();
// 			m_bEmpty = FALSE;
// 			mgwlog("回调函数也清除一次缓冲\n");
// 		}
// 		//暂停
//         if(m_lpFrameQue->GetCount()>(MAX_RECORD_DATABUF_LEN/MAX_RTP_PAYLOAD_LEN*2/3))  //>2/3
// 		{
// 			DWORD dwTime = GetTickCount();
// 			if((dwTime - m_dwLastSendPause)> 60)  //100ms
// 			{
// 				int nRet = CtrlRecordStreamFront(RECORD_CTRL_PAUSE,0,NULL);
// 				mgwlog("缓冲超过2/3 缓冲size:%d,发送暂停命令,执行结果:%d\n",m_lpFrameQue->GetCount(),nRet);
// 				m_dwLastSendPause = dwTime;
// 			}
// 		}
// 		
// 		//读取数据组帧
// 		while(m_bThreadSendFlag)
// 		{
// 			CAutoLock lock(&m_csFrameQueLock);
// 			T_FrameBuf* pFreeBuf =NULL;
// 			pFreeBuf = m_lpFrameQue->getFreeBuf();
// 			if (pFreeBuf)
// 			{
// 				int nLen =0;
// 				if(m_lpInData)
// 				{
// 					if(m_lpInData->GetSize()<MAX_RTP_PAYLOAD_LEN)
// 					{
// 						m_lpFrameQue->InsertFreeBuf(pFreeBuf); 
// 						break;
// 					}
// 					nLen = m_lpInData->Read(m_lpReadBuffer,MAX_RTP_PAYLOAD_LEN);
// 					if(nLen)
// 					{
// 						memcpy((char *)pFreeBuf->pbuffer,m_lpReadBuffer,MAX_RTP_PAYLOAD_LEN);
// 						pFreeBuf->lenth = MAX_RTP_PAYLOAD_LEN;
// 						pFreeBuf->nFrameRate = 0;
// 						pFreeBuf->nFrameRate = lpDataInfo->uiDataTime;  //记录数据时间
// 						m_lpFrameQue->InsertUsedBuf(pFreeBuf);  
// 					}else{
// 						m_lpFrameQue->InsertFreeBuf(pFreeBuf); 
// 						break;
// 					}
// 				}else{
// 					m_lpFrameQue->InsertFreeBuf(pFreeBuf);
// 					break;
// 				}
// 			}else{
// 				break;
// 			}
// 		}
// 	}
// 	return 0;
// }
// 
// int CPlayBack::OnSend()
// {
//     //初始化发送线程
// 	//待修改 码流指定网卡 chenyu 131024
// 	mgwlog("CPlayBack::OnSend\n");
// 	if (!m_lpDataSend)
// 	{
// 		m_lpDataSend =new CDataSend();
// 		RTP_PARA stRtppara;
// 		stRtppara.destport = m_stStreamConInfo.iDestPort;
// 		stRtppara.portbase = m_stStreamConInfo.iSrcPort;
// 		stRtppara.strLogicDeviceID = m_stChConfig.strLogicDeviceID;  //added by chenyu 131024
// 		stRtppara.strChannelName = m_stChConfig.strChannelName;
// 		char szIP[20]={0};
// 		SockObj::ConvertIPToString(m_stStreamConInfo.dwDestIP,szIP);
// 		stRtppara.ipstr = szIP;
// 		m_stStreamConInfo.strDestIP = szIP;
// 		if(0 != m_lpDataSend->StartSend(stRtppara))
// 		{
// 			MEMORY_DELETE(m_lpDataSend);
// 			mgwlog("Warning:Create CDataSend fail \n");
// 			m_hThreadSend =0;
// 			return -1;
// 		}
// 	}
// 	m_uiSendCount = 0;
// 	BOOL bSleepMark = FALSE;  //sleep 标志
// 	BOOL bGetFrameSuccess = TRUE;
// 	while (m_bThreadSendFlag)
// 	{
// 		static DWORD dwTime = GetTickCount();
// 		if((GetTickCount()-dwTime)>PRINT_TIME_SPAN)  //30s 
// 		{
// 			//mgwlog("++++线程工作中CPlayBack::OnSend()handle:[0x%x]\n",this);
// 			dwTime = GetTickCount();
// 		}
// 		bSleepMark = FALSE;
// 	    bGetFrameSuccess = TRUE;
// 		//获取数据
// 		T_FrameBuf* lpTempBuf =NULL;
// 		if(!m_lpFrameQue)
// 		{
//             Sleep(10);
// 			continue;
// 		}
// 		//继续
//         if(m_lpFrameQue->GetCount()<(MAX_RECORD_DATABUF_LEN/MAX_RTP_PAYLOAD_LEN*1/4) )  //<1/4
// 		{
// 			DWORD dwTime = GetTickCount();
// 			if((dwTime - m_dwLastSendResume)> 60)  //60ms
// 			{
// 				int nRet = CtrlRecordStreamFront(RECORD_CTRL_RESUME,0,NULL);
// 				//mgwlog("缓冲低于1/4 缓冲size:%d,发送继续命令,执行结果:%d\n",m_lpFrameQue->GetCount(),nRet);
// 				m_dwLastSendResume = dwTime;
// 			}
// 		}
// 
// 		while (m_bPause && m_bThreadSendFlag)
// 		{
// 			Sleep(10);
// 			continue;
// 		}
// 
// 		if (TRUE)
// 		{
// 			CAutoLock lock(&m_csFrameQueLock);
// 			lpTempBuf = m_lpFrameQue->getUsedBuf();  
// 			if(lpTempBuf)
// 			{
// 				
// 				m_lpDataSend->SendData((unsigned char*)lpTempBuf->pbuffer,lpTempBuf->lenth,1,lpTempBuf->nFrameRate); 
// 				m_uiSendCount++;
// 				if(theApp.m_iConSendPackLen == m_uiSendCount)
// 				{
// 					bSleepMark = TRUE;
// 					m_uiSendCount = 0;
// 				}
// 				WriteRecFile(lpTempBuf->pbuffer,lpTempBuf->lenth);
// 				m_lpFrameQue->InsertFreeBuf(lpTempBuf); //释放
// 				bGetFrameSuccess = TRUE;
// 			}else{
// 				bGetFrameSuccess = FALSE;
// 			}
// 		}
// 		if(FALSE == bGetFrameSuccess)  //取帧失败Sleep
// 		{
//             Sleep(10);
// 		}
// 		if(TRUE == bSleepMark)
// 		{
//             Sleep(10);
// 		}
// 	}
// 	//MEMORY_DELETE(m_lpDataSend);
// 	mgwlog("退出线程CPlayBack::OnSend()\n");
// 	return 0;
// }

int CPlayBack::ConnectDevice()
{
// 	CMediaItemNet* lpRoot = GetRoot();
// 	if (lpRoot)
// 	{
// 		if (m_lpUnitTable)
// 		{
// 			CMediaItemDevice* lpDevice = (CMediaItemDevice*)lpRoot->GetChild(-1, MEDIAITEM_TYPE_DEVICE);
// 			if (lpDevice)
// 			{	
// 				int nRSDeviceID = atoi(m_lpUnitTable->sResved1);
// 				CMediaItemDeviceInfo* lpDeviceInfo = (CMediaItemDeviceInfo*)lpDevice->GetChild(nRSDeviceID, MEDIAITEM_TYPE_DEVICEINFO);
// 				if (lpDeviceInfo)
// 				{
// 					MEDIA_DEVICE_TABLE table;
// 					ZeroMemory(&table, sizeof(MEDIA_DEVICE_TABLE));
// 					
// 					lpDeviceInfo->GetData((BYTE*)&table);
// 					
// 					CONNECT_DEVICE_INFO DeviceInfo;
// 					ZeroMemory(&DeviceInfo, sizeof(CONNECT_DEVICE_INFO));
// 					DeviceInfo.eDeviceType = table.eDeviceType;
// 					DeviceInfo.nDeviceIP = table.nDeviceIP;
// 					DeviceInfo.nDevicePort = table.nDevicePort;
// 					strcpy(DeviceInfo.sUserName, table.sUserName);
// 					strcpy(DeviceInfo.sPassword, table.sPassword);
// 					int nRet = EDV_DEVICE_ConnectDevice(&DeviceInfo);
// 					if (0 > nRet)
// 					{
// 						AddListInfo(LOG_INFO_LEVEL_FATAL, "连接设备[%s]失败,原因[%d]", lpDeviceInfo->GetName(), nRet);
// 						
// 						return 1;
// 					}
// 					
// 					AddListInfo(LOG_INFO_LEVEL_MAIN, "连接设备[%s]成功", lpDeviceInfo->GetName());
// 					m_hConnectHandle = nRet;
// 				}
// 			}
// 		}
// 	}
	
	return 0;
}

void CPlayBack::DisConnectDevice()
{
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetDevice();
	if(!lpDevice)
	{
		mgwlog("CPlayBack::DisConnectDevice未找到设备对象，退出\n");
		return ;
	}
	lpDevice->m_pSdkProxy->fEDV_DEVICE_DisConnectDevice(m_hConnectHandle);
	m_hConnectHandle = -1;
}
#endif


