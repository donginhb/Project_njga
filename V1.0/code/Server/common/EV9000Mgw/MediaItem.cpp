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
				unsigned short  media_version;   // �汾�ţ�ָ����Ϣ�ṹ�汾�ţ�ĿǰΪ0x0101,��1.01�汾��01�����汾�ţ�01���Ӱ汾�š�
				unsigned short  device_id;    // �豸ID�����ڸ���/����   
				
				unsigned short  system_format;          // ϵͳ��װ��
				unsigned short  video_format;           // ��Ƶ��������
				
				unsigned short  audio_format;           // ��Ƶ��������
				unsigned char   audio_channels;         // ͨ����  
				unsigned char   audio_bits_per_sample;  // ��λ��
				unsigned int    audio_samplesrate;      // ������ 
				unsigned int    audio_bitrate;          // ѹ����Ƶ����,��λ��bit
				
				unsigned int    reserved[4];            // ����
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
		{//���ǵ�һ��
			lpPreItem->SetNext(lpNextItem);
		}
		else
		{//��һ��
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
//�豸�ڵ�
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
	//����״̬�߳�
	if (!m_hThreadAliveStatus)
	{
		mgwlog("����״̬�߳�\n");
		m_bThreadAliveStatusFlag = TRUE;
		m_hThreadAliveStatus = CreateThread(NULL, 0, ThreadAliveStatus, (LPVOID)this, 0, &m_dwThreadAliveStatusID);
	}

	m_stDevConfig = stDevConfig;  //���Ӳ���
	m_pSdkProxy = pSdkProxy;      //Sdk
	return ConnectDevice();
}
#else
int CMediaItemDevice::Start(UNGB_DEVCONFIGREC &stDevConfig)
{
	//����״̬�߳�
//	if (!m_hThreadAliveStatus)
//	{
//		mgwlog("����״̬�߳�\n");
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

	m_stDevConfig = stDevConfig;  //���Ӳ���
	return ConnectDevice();
}
#endif

int CMediaItemDevice::Close()
{
	//�ر�״̬�߳�
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

	//�ر�ͨ��
	CMediaItemDevice* lpDevice=NULL;
	map<string,CMediaItemReal*>::iterator iter = m_mapRealPool.begin(); 
	while (iter != m_mapRealPool.end())
	{
		MEMORY_DELETE(iter->second);
		iter++;
	}

	//�ر�¼��
    mgwlog("����ر�¼��\n");

	return 0;
}

int CMediaItemDevice::Close(int idialog_index,BOOL bReal)
{
	int nRetStream =0,nRetPlayBack=0;
    nRetStream = StopReal(idialog_index,bReal);  //�Ƿ���ȫ�ر�
#ifdef SUP_PLAYBACK
	nRetPlayBack = StopPlayBack(idialog_index);
#endif
	if((nRetStream ==0)||(nRetPlayBack ==0))
	{
		return 0;
	}
	return -1;
}

//���ͨ�����
int CMediaItemDevice::CheckAllCh()
{
	CAutoLock lock(&m_csDevOpLock);
	//�����¼�ͨ��
	UNGB_CHCONFIGREC stChCfg;
	map<string,CMediaItemReal*>::iterator iterUnit =m_mapRealPool.begin();
	while(iterUnit != m_mapRealPool.end())
	{	
		if(!CSysCfg::Instance()->GetOneChCfg(iterUnit->first,stChCfg)) //δ�ҵ�
		{
			mgwlog("ͨ�������ڣ�ɾ����ͨ���ڵ㣬id:%s\n",iterUnit->first.c_str());
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
				iterUnit->second->SetChConfig(stChCfg); //����ͨ����Ϣ
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

//��ȡͨ������
int CMediaItemDevice::UpdateUnitName() 
{
	//д�����ݿ�
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
            mgwlog("m_pSdkProxy Ϊ��\n");
			iter++;
			continue;
		}
#else

#endif
		if(szChName[0]!=0)   //��ѯ������ 
		{
           	//sprintf(szChName,"%s_%d",((CMediaItemDevice *)iter->second->GetParent())->m_stDevConfig.strDeviceIP.c_str(),iter->second->m_stChConfig.iMapChannel);
		}else  //��ȡ������ ip+chno  
		{
            //sprintf(szChName,"name%s_%d",((CMediaItemDevice *)iter->second->GetParent())->m_stDevConfig.strDeviceIP.c_str(),iter->second->m_stChConfig.iMapChannel);
	        sprintf(szChName,"%s_%s",((CMediaItemDevice *)iter->second->GetParent())->m_stDevConfig.strDeviceIP.c_str(),(iter->second->GetStrChannelID()).c_str());
		}
// 		//�޸��豸 �޸ļ�¼
// 		//��ѯ��¼�����Ϊ�ջ���wis�����
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
// 				mgwlog("�޸�ͨ�����ɹ���%s",szSql);
// 				TRACE("�޸�ͨ�����ɹ���%s",szSql);
// 			}
// 		}
		sprintf(szSql,"update UnGBPhyDeviceChannelConfig set ChannelName='%s'where LogicDeviceID ='%s'",
			szChName,(iter->second->GetLogicDeviceID()).c_str());
		if(pDbOp->DBExec(szSql)>0)
		{
			TRACE("�޸�ͨ�����ɹ���%s",szSql);
		}
		iter++;
	}
 	MEMORY_DELETE(pDbOp);
	return 0;
}  

//��ȡͨ����Ϣ
int CMediaItemDevice::GetUnitState(string strLogicDeviceID,CHANNELITEM_STATE &eChannelState)
{
    //����ID
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
		mgwlog("CMediaItemDevice::GetUnitState û���ҵ�ָ��ͨ������%s\n",strLogicDeviceID.c_str());
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
	//���سɹ�,ִ������
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
		int hConHandle = m_pSdkProxy->fEDV_DEVICE_ConnectDevice(&DeviceInfo);  //��ȡ���Ӿ��
		if(hConHandle<0)
		{
			mgwlog("CDevCtrl::Open-->�����豸ʧ��,ԭ��[%d] fail\n",hConHandle);
			return -1;
		}
		m_hConHandle = hConHandle;
		mgwlog("CMediaItemDevice::ConnectDevice() �����豸�ɹ�,���Ӿ��hConHandle[%d] DevID[%d]\n",hConHandle,m_stDevConfig.iID);
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
int CMediaItemDevice::ConvertEv8kCh2Int(char* sChID)  //ת��Ev8kͨ��
{	
	int nID = 0;
	string strID = sChID;
	//�ָ��ַ���
	vector<string> vctID;
	StrSplit(strID,'.',vctID);
	if(vctID.size()<4)
	{
		mgwlog("ConvertEv8kCh2Int��������:%s\n",sChID);
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
	sprintf(m_sName, "ǰ���豸 [%d��]", GetChildNum());
	return m_sName;
}

#ifdef WIN32
//��ѯ¼���¼
int CMediaItemDevice::GetRecordInfo(string strDeviceID,
									SYSTEMTIME sStartTime,SYSTEMTIME sStopTime,
									EDVDVRRECORDTABLE* lpTable,int nTableCount,int& iTotalCount) 
{
	iTotalCount = 0;
	if(m_pSdkProxy)
	{
		UNGB_CHCONFIGREC stChCfg;
		if(!CSysCfg::Instance()->GetOneChCfg(strDeviceID,stChCfg)) //δ�ҵ�
		{
			mgwlog("û���ҵ�ָ��ͨ��:[%s]\n",strDeviceID.c_str());
			return -1;  //û���ҵ�ָ��ͨ��
		}
		int nChannelID = ConvertMapChannel2int(stChCfg.strMapChannel);
		iTotalCount = m_pSdkProxy->fEDV_DEVICE_FindRecord(m_hConHandle,nChannelID,sStartTime,sStopTime,lpTable,nTableCount);
		//mgwlog("��ѯ��¼���¼:%d ��ȡ:%d\n",iTotalCount,nTableCount);
		return 0;
	}
	return -1;
}
#endif

//����ͨ��
int CMediaItemDevice::CreateReal(UNGB_CHCONFIGREC &stChConfig,BOOL bInit)
{
	CAutoLock lock(&m_csDevOpLock);
    if("00000000000000000000" == stChConfig.strLogicDeviceID)  //Onvif��δ��ʼ������
	{
		mgwlog("ͨ�������贴��,Channel:%s LogicDeviceID:%s\n",stChConfig.strMapChannel.c_str(),stChConfig.strLogicDeviceID.c_str());
		return -1;
	}

	//�Ȳ�����û�� added by chenyu 140606
	//����Ƶ��(m_mapRealPool) 
	map<string,CMediaItemReal*>::iterator iterReal = m_mapRealPool.find(stChConfig.strLogicDeviceID);
	if(iterReal != m_mapRealPool.end())
	{
		//mgwlog("CMediaItemDevice::CreateReal ��Ƶ����%s�Ѵ�\n",stChConfig.strLogicDeviceID.c_str());
		return 0;
	}

	//����ͨ������
	int iMapChannel = 0;
#ifdef WIN32
	iMapChannel = ConvertMapChannel2int(stChConfig.strMapChannel);
	CMediaItemReal* lpReal = new CMediaItemReal(iMapChannel);
	//mgwlog("CMediaItemDevice::CreateReal����ͨ������,mapChannel:%s\n",stChConfig.strMapChannel.c_str());
#else
	CMediaItemReal* lpReal = new CMediaItemRealOnvif(iMapChannel);
#endif
	if(!lpReal)
	{
		mgwlog("CMediaItemDevice::CreateReal����ͨ������ʧ��,Channel:%s LogicDeviceID:%s\n",stChConfig.strMapChannel.c_str(),stChConfig.strLogicDeviceID.c_str());
		return -1;
	}
	lpReal->SetChConfig(stChConfig);   //����ͨ��������Ϣ
	lpReal->SetDeviceIP(m_stDevConfig.strDeviceIP);  //�����豸IP
	lpReal->SetParent(this); //���ӵ��豸����
	
	//��¼Unit��mapӳ��
	m_mapRealPool[stChConfig.strLogicDeviceID] = lpReal;
	if(lpReal)
	{
		STREAM_CON_INFO stStreamConInfo;
#ifdef WIN32
		//lpReal->StartStream(stStreamConInfo,UNIT_OPEN_EMPTY); //ֻ������Ƶ���󣬲���
#else
		mgwlog("CMediaItemDevice::CreateReal to do\n");
		return 0;
#endif
	}
	return 0;
} 

//eOpenType ������ �մ򿪡�������
int CMediaItemDevice::OpenReal(STREAM_CON_INFO stStreamConInfo,UNITOPENTYPE eOpenType)
{
	mgwlog("CMediaItemDevice::OpenReal enter\n");
	CAutoLock lock(&m_csDevOpLock);
	mgwlog("CMediaItemDevice::OpenReal after lock\n");
	stStreamConInfo.iDeviceType = m_stDevConfig.iDeviceType;  //�豸����
	//���ҹ�������(m_mapRealWorkQueue) 
	map<int,CMediaItemReal*>::iterator iterRealWork = m_mapRealWorkQueue.find(stStreamConInfo.stSipContext.idialog_index);
	if((iterRealWork != m_mapRealWorkQueue.end()) && iterRealWork->second)
	{
		mgwlog("CMediaItemDevice::OpenReal�����������ҵ�ָ����Ƶ����[dialogindex:%d]:%s ��ִ�йر�\n",
			stStreamConInfo.stSipContext.idialog_index,stStreamConInfo.strLogicDeviceID.c_str());
		iterRealWork->second->StopStream(TRUE);
	}else{
		mgwlog("CMediaItemDevice::OpenReal����������û���ҵ�ָ����Ƶ����[dialogindex:%d]:%s\n",
			stStreamConInfo.stSipContext.idialog_index,stStreamConInfo.strLogicDeviceID.c_str());
	}
	//������Ƶ��(m_mapRealPool) 
	map<string,CMediaItemReal*>::iterator iterReal = m_mapRealPool.find(stStreamConInfo.strLogicDeviceID);
	if((iterReal != m_mapRealPool.end()) && iterReal->second)
	{
		m_mapRealWorkQueue[stStreamConInfo.stSipContext.idialog_index] = iterReal->second;
		mgwlog("Device::OpenReal��¼��Ƶ����[%s:%s]��m_mapRealWorkQueue��size:%d dialogindex:%d\n",
			m_stDevConfig.strDeviceIP.c_str(),m_stDevConfig.strDeviceName.c_str(),
			m_mapRealWorkQueue.size(),stStreamConInfo.stSipContext.idialog_index);
		return iterReal->second->StartStream(stStreamConInfo,eOpenType);
	}else{
		mgwlog("CMediaItemDevice::OpenReal ��Ƶ����û���ҵ�ָ����Ƶ����%s\n",stStreamConInfo.strLogicDeviceID.c_str());
		return -1;
	}
	return 0;
}

//�Ƿ���ȫ�ر�
int CMediaItemDevice::StopReal(int idialog_index,BOOL bReal)
{
	mgwlog("CMediaItemDevice::StopReal enter\n");
	CAutoLock lock(&m_csDevOpLock);
	mgwlog("CMediaItemDevice::StopReal after lock\n");
#ifdef WIN32
	//�ڹ�������(m_mapRealWorkQueue)�в���
	map<int,CMediaItemReal*>::iterator iterIndex = m_mapRealWorkQueue.find(idialog_index);
	if((iterIndex != m_mapRealWorkQueue.end()) && iterIndex->second)
	{
	   /*
	    *���ڱ����������м��� /EHa ����,
	    *����VC�������Ͳ����û��throw��try_catchģ���Ż���
		*/
		static int iExceptionCount=0;  //�쳣����
		try
		{
			iterIndex->second->StopStream(bReal);
			m_mapRealWorkQueue.erase(iterIndex);  //ɾ��������������Ƶ����
			mgwlog("CMediaItemDevice::StopStream dlgindex:%d,m_mapRealWorkQueue size:%d\n",idialog_index,m_mapRealWorkQueue.size());
		}
		catch (...)
		{
			iExceptionCount++;  //����
			if(iExceptionCount > MAX_EXCEPTION_NUM)
			{
				mgwlog("Exception: CMediaItemDevice::StopStream �쳣����50�Σ��˳�....\n");
				exit(-1);
			}
			mgwlog("Exception: CMediaItemDevice::StopStream �쳣����:%d\n",iExceptionCount);
		}
		return 0;
	}else{
		mgwlog("CMediaItemDevice::StopStream û���ҵ�ָ����Ƶ����,dlg_index:%d\n",idialog_index);
		return -1;
	}
#else
	mgwlog("onvif����Stop��������\n");
#endif
}

#ifdef SUP_PLAYBACK
int CMediaItemDevice::FindPlayBack(int idialog_index,CPlayBack* &pPlayBack)
{
	map<int,CPlayBack*>::iterator iterPlayBack = m_mapPlayBack.find(idialog_index);
	if((iterPlayBack == m_mapPlayBack.end()) || (!(iterPlayBack->second)))
	{
		mgwlog("CMediaItemDevice::FindPlayBack û���ҵ�ָ���طŶ���,index:%d\n",idialog_index);
		return -1;  //û���ҵ�ָ���豸
	}
	pPlayBack = iterPlayBack->second;
	return 0;
}

//�����طŶ���
int CMediaItemDevice::OpenPlayBack(STREAM_CON_INFO &stStreamConInfo,UNITOPENTYPE eOpenType)  
{
	mgwlog("CMediaItemDevice::OpenPlayBack enter\n");
	CAutoLock lock(&m_csDevOpLock);
	mgwlog("CMediaItemDevice::OpenPlayBack after lock\n");
	UNGB_CHCONFIGREC stChCfg;
	if(!CSysCfg::Instance()->GetOneChCfg(stStreamConInfo.strLogicDeviceID,stChCfg))
	{
		mgwlog("CMediaItemDevice::OpenPlayBack û���ҵ�ָ��ͨ������1��%s\n",stStreamConInfo.strLogicDeviceID.c_str());
		return -1;  //û���ҵ�ָ���豸
	}

	CPlayBack* lpPlayBack =NULL;
	if( 0 == FindPlayBack(stStreamConInfo.stSipContext.idialog_index,lpPlayBack))  //�ҵ�
	{
		mgwlog("CMediaItemDevice::OpenPlayBack success\n");
		return 0;
	}

	//�����µĻطŶ���
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
	lpPlayBack->SetParent(this);  //���ӵ��豸��
	
	//��¼Unit��mapӳ��
	m_mapPlayBack[stStreamConInfo.stSipContext.idialog_index]=lpPlayBack;
	mgwlog("CMediaItemDevice::OpenPlayBack �����طŶ���success,idialog_index:%d\n",stStreamConInfo.stSipContext.idialog_index);
    lpPlayBack->StartByTime();  //��ʼ����
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
		mgwlog("CMediaItemDevice::StopPlayBack�طŶ��� dlgindex:%d,m_mapPlayBack size:%d\n",idialog_index,m_mapPlayBack.size());
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
				mgwlog("¼��ط���Чָ��\n");
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
    //����ID
	map<string,CMediaItemReal*>::iterator iterReal = m_mapRealPool.find(strDeviceID);
	if((iterReal != m_mapRealPool.end()) && iterReal->second)
	{
		int nID = iterReal->second->GetChannelID();
		return m_pSdkProxy->fEDV_DEVICE_PTZCtrl(m_hConHandle, nID, (BYTE*)lpSendBuf, dwBufSize);
	}else{
		mgwlog("CMediaItemDevice::PTZCtrl û���ҵ�ָ��ͨ������%s\n",strDeviceID.c_str());
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
    //����ID
	map<string,CMediaItemReal*>::iterator iterReal = m_mapRealPool.find(strDeviceID);
	if((iterReal != m_mapRealPool.end()) && iterReal->second)
	{
		int nID = iterReal->second->GetChannelID();
		return m_pSdkProxy->fEDV_DEVICE_MakeKeyFrame(m_hConHandle, nID);
	}else{
		mgwlog("CMediaItemDevice::MakeKeyFrame û���ҵ�ָ��ͨ������%s\n",strDeviceID.c_str());
		return -1;
	}
#else
	mgwlog("CMediaItemDevice::MakeKeyFrame to do\n");
	return 0;
#endif
}

//���ò���
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
	//����ID
	map<string,CMediaItemReal*>::iterator iterReal = m_mapRealPool.find(strDeviceID);
	if((iterReal != m_mapRealPool.end()) && iterReal->second)
	{
		int nID = iterReal->second->GetChannelID();
		return m_pSdkProxy->fEDV_DEVICE_SetVideoParam(m_hConHandle, nID,byType,byParam);
	}else{
		mgwlog("CMediaItemDevice::SetVideoParam û���ҵ�ָ��ͨ������%s\n",strDeviceID.c_str());
		return -1;
	}
#else
	mgwlog("CMediaItemDevice::SetVideoParam to do\n");
	return 0;
#endif
}

//��ȡ����
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
	//����ID
	map<string,CMediaItemReal*>::iterator iterReal = m_mapRealPool.find(strDeviceID);
	if((iterReal != m_mapRealPool.end()) && iterReal->second)
	{
		int nID = iterReal->second->GetChannelID();
		return m_pSdkProxy->fEDV_DEVICE_GetVideoParam(m_hConHandle, nID,nParam);
	}else{
		mgwlog("CMediaItemDevice::GetVideoParam û���ҵ�ָ��ͨ������%s\n",strDeviceID.c_str());
		return -1;
	}
#else
	mgwlog("CMediaItemDevice::GetVideoParam to do\n");
	return 0;
#endif
}

//״̬�̺߳���
int	CMediaItemDevice::OnAliveStatus()
{
	while (m_bThreadAliveStatusFlag)
	{
		static DWORD dwTime = GetTickCount();
		if((GetTickCount()-dwTime)>PRINT_TIME_SPAN)  //30s 
		{
           //mgwlog("++++�̹߳�����CMediaItemDevice::OnAliveStatus()handle:[0x%x]\n",this);
		   dwTime = GetTickCount();
		}
        //�����¼�ͨ��
		map<string,CMediaItemReal*>::iterator iterUnit =m_mapRealPool.begin();
		while(iterUnit != m_mapRealPool.end())
		{
			//mgwlog("�ϱ�״̬\n");
			SendAliveStatusMsg(iterUnit->second);
			iterUnit++;
		}
// 		map<int,CMediaItemReal*>::iterator iterUnit2 =m_mapRealWorkQueue.begin();
// 		while(iterUnit2 != m_mapRealWorkQueue.end())
// 		{
// 			//mgwlog("�ϱ�״̬\n");
// 			SendAliveStatusMsg(iterUnit2->second);
// 			iterUnit2++;
// 		}
#ifdef WIN32
		Sleep(1000*10);   //10s ���һ��
#else
		sleep(10);
#endif
	}
	mgwlog("exit CMediaItemDevice::OnAliveStatus\n");
	return 0;	
}

//�����仯���ϱ���Ϣ
// 	<?xml version="1.0"?>
// 		<Notify>
// 		<CmdType>DeviceStatus</CmdType>
// 		<SN>43</SN>
// 		<DeviceID>XXXX<DeviceID>
// 		<Status>OFFLINE</Status>
// 		</Notify>
//�ϱ���λ״̬
int CMediaItemDevice::SendAliveStatusMsg(CMediaItemReal* pUnit)
{
	//CAutoLock lock(&m_csDevOpLock);
	if(!CSysCfg::m_bDevInited)
	{
        //mgwlog("�豸��δ��ʼ�����\n");
		return -1;
	}
	if (!pUnit)
	{
		mgwlog("ͨ�����Ϊ��\n");
		return -1;
	}

	string strChState="";
	if (-1 == m_hConHandle)   //�豸����������ͨ��������
	{
		strChState = "OFF";
	}else{
        CHANNELITEM_STATE eChannelState;
#ifdef WIN32
        if(!m_pSdkProxy->fEDV_DEVICE_GetChannelState(m_hConHandle,pUnit->GetChannelID(),eChannelState))  //��ȡ�ɹ�
		{
			pUnit->SetCurChannelState(eChannelState);
		}
#endif
		strChState = CovertState2str(eChannelState);
	}

	//�����仯���ϱ�
	if(FALSE == pUnit->NeedSendState()) //����Ҫ�ϱ�
	{
		//mgwlog("��λû�з����仯\n");
		return -1;
	}

	int iRet =-1;
	SIP_MSG_INFO msg_info;
	ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
	sprintf(msg_info.CallerID,"%s",CSysCfg::Instance()->GetstrPara(SYS_CFG_MGWDEVID).c_str());
	sprintf(msg_info.CalleedID,"%s",CSysCfg::Instance()->GetstrPara(SYS_CFG_SERVERID).c_str());
	
	//����һ��XML���ĵ�����
	TiXmlDocument *myDocument = new TiXmlDocument();
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
	myDocument->LinkEndChild(decl);
	
	//����һ����Ԫ��(��Ϣ����)�����ӡ�
	TiXmlElement *RootElement = new TiXmlElement("Notify");
	myDocument->LinkEndChild(RootElement);
	
	//����һ��CmdTypeԪ�ز�����
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
	
	//xml�ĵ��������Ϊ�ַ���
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
	mgwlog("Unit��λ[%s]״̬���ɵ�:%d ��ǰ:%d\n",(pUnit->GetLogicDeviceID()).c_str(),pUnit->GetOldChannelState(),pUnit->GetCurChannelState());
	
	MEMORY_DELETE(myDocument);
	MEMORY_DELETE_EX(msg_info.pMsg);
	pUnit->RefreshState();  //����״̬
	pUnit->SetSendStateMark(TRUE);
	return iRet;
}

#ifndef WIN32
//onvif�豸
int CMediaItemDeviceOnvif::ConnectDevice()
{
	//���سɹ�,ִ������
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
		mgwlog("StopCheckAlive() ֹͣOnvifDevice������\n");
		m_pOnvifOp->StopCheckAlive();
	}
	return ;
}

//�ϱ���λ״̬
int CMediaItemDeviceOnvif::SendAliveStatusMsg(CMediaItemReal* pUnit)
{
	mgwlog("SendAliveStatusMsg:%s\n",m_stDevConfig.strDeviceIP.c_str());
	//CAutoLock lock(&m_csDevOpLock);
// 	if(!CSysCfg::m_bDevInited)
// 	{
//         //mgwlog("�豸��δ��ʼ�����\n");
// 		return -1;
// 	}
	if (!pUnit)
	{
		mgwlog("ͨ�����Ϊ��\n");
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
		pUnit->SetCurChannelState(eChannelState);     //���豸״̬��ʾ����ÿ��ͨ����״̬
		strChState = CovertState2str(eChannelState);
	}else{
		mgwlog("m_pOnvifOp is NULL\n");
		return -1;
	}
	
	//�����仯���ϱ�
	if(FALSE == pUnit->NeedSendState()) //����Ҫ�ϱ�
	{
		//mgwlog("��λû�з����仯\n");
		return -1;
	}
	
	int iRet =-1;
	SIP_MSG_INFO msg_info;
	ZeroMemory(&msg_info,sizeof(SIP_MSG_INFO));
	sprintf(msg_info.CallerID,"%s",CSysCfg::Instance()->GetstrPara(SYS_CFG_MGWDEVID).c_str());
	sprintf(msg_info.CalleedID,"%s",CSysCfg::Instance()->GetstrPara(SYS_CFG_SERVERID).c_str());
	
	//����һ��XML���ĵ�����
	TiXmlDocument *myDocument = new TiXmlDocument();
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
	myDocument->LinkEndChild(decl);
	
	//����һ����Ԫ��(��Ϣ����)�����ӡ�
	TiXmlElement *RootElement = new TiXmlElement("Notify");
	myDocument->LinkEndChild(RootElement);
	
	//����һ��CmdTypeԪ�ز�����
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
	
	//xml�ĵ��������Ϊ�ַ���
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
	mgwlog("Unit��λ[%s]״̬���ɵ�:%d ��ǰ:%d\n",(pUnit->GetLogicDeviceID()).c_str(),pUnit->GetOldChannelState(),pUnit->GetCurChannelState());
	
	MEMORY_DELETE(myDocument);
	MEMORY_DELETE_EX(msg_info.pMsg);
	pUnit->RefreshState();  //����״̬
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

//���ò���
int CMediaItemDeviceOnvif::SetVideoParam(string strDeviceID, BYTE byType, BYTE byParam)
{
	if(m_pOnvifOp)
	{
		mgwlog("__Onvif_SetVideoParam__\n");
		m_pOnvifOp->SetVideoParam(byType,byParam);
	}
	return 0;
}

//��ȡ����
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
//Onvif��Ƶ�ڵ�
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
	//�豸��������Ϣ��CMediaItemDevice��
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetParent();
	if (lpDevice)
	{
		if( 0 == lpDevice->ConnectDevice())
		{
			m_hConnectHandle = lpDevice->m_hConHandle;
			mgwlog("�����豸�ɹ�[%d]", m_hConnectHandle);
		}else{
			mgwlog("�����豸ʧ��");
            return -1;
		}
	}else{
		return -1;
	}
	return 0;  //�ɹ�
}

void CMediaItemRealOnvif::DisConnectDevice()
{
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetParent();
	lpDevice->DisConnectDevice();
	m_hConnectHandle = -1;
}

//added by chenyu 131016
//������
//���  : �����������߳� ����������
//������: ��������
int CMediaItemRealOnvif::StartStream(STREAM_CON_INFO &stStreamConInfo,UNITOPENTYPE eOpenType)
{
    CAutoLock lock(&m_csRealOpLock);        //��ǰLock
    mgwlog("onvif����Start��������\n");
    mgwlog("����OnvifOp����\n");
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

//�Ƿ���ȫ�ر�
void CMediaItemRealOnvif::StopStream(BOOL bReal)
{
	mgwlog("onvif����Stop��������:");
    return ;
}

#endif

/////////////////
//ʵʱ��Ƶ
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
    m_bThreadPsPackFlag = FALSE;   //ps����߳�
	m_hThreadPsPack = 0;
	m_dwThreadPsPackID = 0;
	m_hPsPack = -1;

	m_bThreadSendFlag=FALSE;
	m_hThreadSend=0;
	m_dwThreadSendID=0;
	m_lpDataSend =NULL;
	m_lpSendBuf =NULL;
    m_lpPlayUDP =NULL;             //udp����
	
	m_lpMediaDataOp =NULL;         //ת��ģ��
	m_bHeadInfoInited =FALSE;
	m_pCPSPackaging = NULL;
	m_pPsPackBuf = NULL;
	m_pH264Buf = NULL;
	m_dwDataSize = 0;
	m_bFinishCheckStream = FALSE;
	m_nCheckStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //Ĭ��δ֪
	m_nStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //Ĭ��δ֪
#else
	pthread_mutex_init(&m_csRealOpLock,NULL);
#endif
	m_bStartMark = FALSE;
	m_eOpenType = UNIT_OPEN_EMPTY;   //�մ�

	//---------------
	m_eOldChannelState = CHANNELITEM_STATE_CLOSE;   //ͨ��״̬
    m_eCurChannelState = CHANNELITEM_STATE_CLOSE;   //��ǰͨ��״̬
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
	//�豸��������Ϣ��CMediaItemDevice��
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetParent();
	if (lpDevice)
	{
		if( 0 == lpDevice->ConnectDevice())
		{
			m_hConnectHandle = lpDevice->GetConnectHandle();
			mgwlog("�����豸�ɹ�[%d]", m_hConnectHandle);	
		}else{
			mgwlog("�����豸ʧ��");
            return -1;
		}
	}else{
		return -1;
	}
	return 0;  //�ɹ�
}

void CMediaItemReal::DisConnectDevice()
{
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetParent();
	lpDevice->DisConnectDevice();
	m_hConnectHandle = -1;
}

//added by chenyu 131016
//������ 
//���  : �����������߳� ����������
//������: ��������
int CMediaItemReal::StartStream(STREAM_CON_INFO &stStreamConInfo,UNITOPENTYPE eOpenType)
{
	mgwlog("CMediaItemReal::StartStream enter\n");
	//1. �ȹر�
	StopStream(TRUE);
    CAutoLock lock(&m_csRealOpLock);
	mgwlog("CMediaItemReal::StartStream after lock\n");
#ifdef WIN32

	if (UNIT_OPEN_EMPTY == eOpenType)  //���
	{
		mgwlog("���:%s[%s]\n",m_stChConfig.strLogicDeviceID.c_str(),m_stChConfig.strChannelName.c_str()); 
		m_dwTime = 0; 
		m_llRcvByteCount = 0;
		m_llSendByteCount = 0;
		m_llLastSendByteCount = 0;
		return 0;
	}
	m_stStreamConInfo = stStreamConInfo;
	mgwlog("��unit:%s[%s] ���ض˿�:%d TSU�˿�:%d\n",
		m_stChConfig.strLogicDeviceID.c_str(),m_stChConfig.strChannelName.c_str(),
		m_stStreamConInfo.iSrcPort,m_stStreamConInfo.iDestPort);
      
	if (!m_lpSendBuf)
	{
		m_lpSendBuf = new byte[MAX_SENDBUF_LEN];
	}
	
	m_hConnectHandle = ((CMediaItemDevice *)GetParent())->GetConnectHandle();
	if (-1 == m_hConnectHandle)
	{
		if ( ConnectDevice() !=0 )   //ʧ��
		{
            mgwlog("���������豸 fail\n");  
			return -1;
		}
	}
	
	//2.����������߳�  �����뵽MediaDataOp�д�����ٴ�MediaDataOp������
	if(!m_lpMediaDataOp)
	{
		m_stCodecinfo.iStreamType = m_stChConfig.iStreamType;
		m_stCodecinfo.bNeedCodec = m_stChConfig.iNeedCodec;
		m_stCodecinfo.strLogicID = m_stChConfig.strLogicDeviceID;
		CMediaDataOp *lpMediaDataOp = new CMediaDataOp(m_stCodecinfo);
		lpMediaDataOp->StartCodec();	  //���������
		m_lpMediaDataOp = lpMediaDataOp;
	}
	
	//3.�ȹر����еķ����̣߳������µ����������߳�
	if(m_hThreadSend)
	{   
		mgwlog("�رվɵ����������߳�\n");
		m_bThreadSendFlag = FALSE;
		WaitForSingleObject(m_hThreadSend, INFINITE);
		CLOSE_HANDLE(m_hThreadSend);
		m_dwThreadSendID = 0;
	}
	MEMORY_DELETE(m_lpDataSend);
	mgwlog("�����µ����������߳�\n");
	m_dwTime = GetTickCount();    //��������ʼ�������� 
	m_llRcvByteCount = 0;
	m_llSendByteCount = 0;
	m_llLastSendByteCount = 0;
	if (!m_hThreadSend)
	{
		//mgwlog("���������߳�\n");
		m_bThreadSendFlag = TRUE;
		m_hThreadSend = CreateThread(NULL, 0, ThreadSend, (LPVOID)this, 0, &m_dwThreadSendID); //���������߳�
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
	//3.1�������ps���򴴽����ps�߳�
	if (m_stChConfig.iStreamType !=0)
	{
		if(!m_hThreadPsPack)
		{
			m_bThreadPsPackFlag = TRUE;
			m_hThreadPsPack = CreateThread(NULL, 0, ThreadPsPack, (LPVOID)this, 0, &m_dwThreadPsPackID); //���������߳�
		}
	}
#endif

	//4.��������ʵʱ���ص�
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetDevice();
	if(!lpDevice)
	{
		mgwlog("CMediaItemReal::StartStream δ�ҵ��豸�����˳�\n");
		return -1;
	}

	EDVOPENHANDLE handle = -1;
// 	if((DEVICE_TYPE_EV8K == m_stStreamConInfo.iDeviceType) && (1 == m_stChConfig.iNeedCodec))
// 	{
// 		m_bCBYUVData = TRUE;
// 	}
//	if(m_bCBYUVData)
	if( EV9000APP_VIDEOENCODE_TYPE_YUV == m_stChConfig.iStreamType)  //������YUV
	{
        mgwlog("����YUV�ص�\n");
		handle = lpDevice->m_pSdkProxy->fEDV_DEVICE_OpenRealStream(m_hConnectHandle, m_nID, STREAM_TYPE_MAIN,STREAM_DATATYPE_YUV);  //�ص�YUV����
	}else{
		mgwlog("�������������ص�\n");
	    handle = lpDevice->m_pSdkProxy->fEDV_DEVICE_OpenRealStream(m_hConnectHandle, m_nID, STREAM_TYPE_MAIN,STREAM_DATATYPE_COM,FALSE);  //�����ص�
	}
 
	if (0 > handle)
	{
		mgwlog("���豸ͨ��ͼ��ʧ���˳�,ԭ�� handle:[%d] m_hConnectHandle:%d\n", handle,m_hConnectHandle);
		FreeRealResource(TRUE);  //�ͷ���Դ�˳� 
		return -1;
	}else{
		mgwlog("���豸ͨ��ͼ��ɹ� handle:[%d] m_hConnectHandle:%d\n", handle,m_hConnectHandle);
	}
	LONG lRet = lpDevice->m_pSdkProxy->fEDV_DEVICE_SetRealCallBack(handle, StreamDataCallBack, this);
	if(lRet>=0)
	{
		mgwlog("���ûص��ɹ�[%d]\n", lRet);
	}else{
		mgwlog("���ûص�ʧ��[%d] handle:%d\n", lRet,handle);
		return -1;
	}
	
	//5.�޸�״̬ ����I֡
	m_hOpenHandle = handle;
	m_StreamInfo.eChannelState = CHANNELITEM_STATE_OPEN;
	m_stStreamConInfo.bConStatus = TRUE;
    m_eOpenType = eOpenType;  //���´�״̬
	if(lpDevice)
	{
		lpDevice->MakeKeyFrame(m_stChConfig.strLogicDeviceID);  //ǿ�Ʋ���I֡
	}

	m_bStartMark = TRUE;  //�Ѵ򿪹�
#else
	mgwlog("CMediaItemReal::StartStream onvif����Start��������\n");
#endif
	return 0;
}

//�ͷ���Դ
void CMediaItemReal::FreeRealResource(BOOL bReal)
{
	mgwlog("CMediaItemReal::FreeRealResource �ͷ���Դ\n");
#ifdef WIN32
    //1.�ر�����	
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetDevice();
	if(lpDevice && (m_hOpenHandle>=0))
	{
		mgwlog("CMediaItemReal::StopStream() CloseStream�ر������\n");
		lpDevice->m_pSdkProxy->fEDV_DEVICE_SetRealCallBack(m_hOpenHandle, NULL, this);
		lpDevice->m_pSdkProxy->fEDV_DEVICE_CloseStream(m_hOpenHandle);
		m_hOpenHandle = -1;
		//lpDevice->m_pSdkProxy->fEDV_DEVICE_DisConnectDevice(m_hConnectHandle);  //���Ͽ�����
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
	if(m_hPsPack>=0)  //�رմ��
	{
		EV9KPsPack_Close(m_hPsPack);
		m_hPsPack = -1;
	}
#endif
	
	//2.������״̬���ڴ�
	MEMORY_DELETE_EX(m_lpSendBuf);
	MEMORY_DELETE(m_lpMediaDataOp);
	MEMORY_DELETE(m_lpDataSend);
	MEMORY_DELETE(m_pCPSPackaging);   //�ͷ� PsPack
	MEMORY_DELETE_EX(m_pPsPackBuf);
	MEMORY_DELETE_EX(m_pH264Buf);
	m_dwDataSize = 0;
	m_bCBYUVData = FALSE;
    m_stStreamConInfo.bConStatus = FALSE;
	m_StreamInfo.eChannelState = CHANNELITEM_STATE_CLOSE;
	m_bHeadInfoInited = FALSE;
	m_eOpenType = UNIT_OPEN_EMPTY;
	m_bStartMark = FALSE;
	m_dwTime = 0;  //����ʱ����� 
	m_llRcvByteCount = 0;
	m_llSendByteCount = 0;
	m_llLastSendByteCount = 0;
	m_bFinishCheckStream = FALSE;
	m_nCheckStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //Ĭ��δ֪
	m_nStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //Ĭ��δ֪
	
	//3.�رվɵĻỰdialog_index
	if(m_stStreamConInfo.stSipContext.idialog_index>0)
	{
		mgwlog("�رվɵ�sipͨ��,dialog_index:%d\n",m_stStreamConInfo.stSipContext.idialog_index);
        SIP_SendBye(m_stStreamConInfo.stSipContext.idialog_index);
    }
	m_stStreamConInfo.Clear();
#else
	mgwlog("CMediaItemReal::FreeRealResource onvif����Stop��������\n");
#endif
}

//�Ƿ���ȫ�ر�
void CMediaItemReal::StopStream(BOOL bReal)
{
	mgwlog("CMediaItemReal::StopStream enter\n");
	CAutoLock lock(&m_csRealOpLock);
	mgwlog("CMediaItemReal::StopStream after lock\n");
	mgwlog("CMediaItemReal::StopStream �ر���������\n");
	FreeRealResource(bReal);  //�ͷ���Դ
}

#ifdef WIN32
//�����յ��Ļص�����
//����MediaDataOpģ�鴦������
int CMediaItemReal::OnStreamData(EDVOPENHANDLE handle, LPDATA_INFO lpDataInfo, unsigned char* lpBuf, DWORD dwBufSize)
{
	m_llRcvByteCount +=dwBufSize;  //��������
 	//������Ҫ����ͷ
    if(!m_bHeadInfoInited 
		&& (m_stCodecinfo.iStreamType >= EV9000_STREAMDATA_TYPE_VIDEO_HIK)
		&& (m_stCodecinfo.iStreamType < EV9000_STREAMDATA_TYPE_VIDEO_DAH))
	{
		//�յ�Hik NET_DVR_SYSHEADͷ���������ݴ���
		//EV8000           14 byte
		//long              4 byte
		//NET_DVR_SYSHEAD  40 byte 
		if(58 == lpDataInfo->nFrameHeadLen )   //14+4+40    
		{
			if(m_lpMediaDataOp)
			{
                mgwlog("CMediaItemReal::OnStreamData SetDataHead ���ú�������ͷ\n");
				m_lpMediaDataOp->SetDataHead(lpDataInfo);
				//���ݺ����ص�ͷ���ݸ������ݿ�����  //modified by chenyu 1030
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
				//mgwlog("CMediaItemReal::OnStreamData ������������:%d\n",nStreamSubType);
                //�ݲ����� chenyu 131031
				//���¿⣬�����ڴ� 
// 				if(m_stChConfig.iStreamSubType != nStreamSubType)
// 				{
// 					mgwlog("����������[%s]:%d\n",m_stChConfig.strChannelName.c_str(),nStreamSubType);
// 					m_stChConfig.iStreamSubType = nStreamSubType;
// 					CSysCfg::Instance()->UpdataChInfo(m_stChConfig);
// 				}
				m_bHeadInfoInited = TRUE;
			}
		}
	}
    //1.���������
	if(FALSE == m_bFinishCheckStream)
	{
		int nRet = CheckStreamType(lpBuf,dwBufSize);
		if(-1 == nRet)  //���ʧ���˳����������
		{
			mgwlog("�Զ����������ʧ���˳����������\n");
			return 0;
		}else{
			m_nCheckStreamType = nRet;  //���õ�������
			if (m_nCheckStreamType != EV9000APP_VIDEOENCODE_TYPE_UNKNOW) //��⵽ȷ������
			{
				m_nStreamType = m_nCheckStreamType;
			}else{
				m_nStreamType = m_stChConfig.iStreamType;   //��������������
			}
			mgwlog("��������:%d �Զ��������:%d �������ж�����:%d\n",m_stChConfig.iStreamType,m_nCheckStreamType,m_nStreamType);
			m_bFinishCheckStream  = TRUE;
		}
	}

	//2.������ ����Զ����������Ϊδ֪���������ý��д���
	if(m_lpMediaDataOp)
	{
		//mgwlog("CMediaItemReal::OnStreamData inputdata �ص�����");
		if(	UNIT_OPEN_EMPTY != m_eOpenType)  //�ǿմ�
		{
			if(EV9000APP_VIDEOENCODE_TYPE_YUV == m_nStreamType)  //YUV����
			{
				if(lpDataInfo)
				{
					m_Frame.nHeight = lpDataInfo->nHeight;
					m_Frame.nWidth = lpDataInfo->nWidth;
					m_Frame.pbuffer = lpBuf;
					m_Frame.lenth = dwBufSize;
					m_lpMediaDataOp->InputYUVData(&m_Frame,0); //������������� YUV
				}	
			}
			else if((EV9000APP_VIDEOENCODE_TYPE_H264 == m_nStreamType)||(2==m_nStreamType)) //�����h264��ת��Ϊps
			{
				DealH264(lpBuf,dwBufSize);
			}
			else if((EV9000_STREAMDATA_TYPE_PS == m_nStreamType)||(0==m_nStreamType)) //�Ǳ�ps��ת��Ϊ��׼ps
			{
                //DealUnGBPs(lpBuf,dwBufSize);    //�°汾��������ps���ͻ��˴���
				m_lpMediaDataOp->InputData(lpBuf, dwBufSize);
			}
			else  //������ֱ������MediaDataOp�д���
			{
                m_lpMediaDataOp->InputData(lpBuf, dwBufSize);
			}
		}
		WriteRecFile(lpBuf, dwBufSize); //д¼���ļ�
	}
	return 0;
}

//���������
/*  
*ps   ---- EV9000_STREAMDATA_TYPE_PS 
*h264 ---- EV9000APP_VIDEOENCODE_TYPE_H264
*δ֪ ---- EV9000APP_VIDEOENCODE_TYPE_UNKNOW
*ʧ�� ---- -1
*/
int CMediaItemReal::CheckStreamType(unsigned char* lpBuf, DWORD dwBufSize)
{
	if(dwBufSize < 6)  
	{
		return -1;  //̫�̼��ʧ��
	}
	if(dwBufSize>10)
	{
		mgwlog("������ֵ��0x%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			lpBuf[0],lpBuf[1],lpBuf[2],lpBuf[3],lpBuf[4],lpBuf[5],lpBuf[6],lpBuf[7],lpBuf[8],lpBuf[9]);
	}
	if(lpBuf[0]==0 && lpBuf[1]==0 && lpBuf[2]==1 
		&& ( lpBuf[3]==0xBA     //Program Pack
		|| lpBuf[3]==0xE0     //pes vedioͷ
		|| lpBuf[3]==0xC0    //pes audioͷ
		|| lpBuf[3]==0xBB    //systemͷ
		))
	{
		mgwlog("�Զ����������Ϊ:ps\n");
		return EV9000_STREAMDATA_TYPE_PS;
	}
	else if(lpBuf[0]==0 && lpBuf[1]==0 && lpBuf[2]==0 && lpBuf[3]==0x1)
	{
		mgwlog("�Զ����������Ϊ:h264\n");
		return EV9000APP_VIDEOENCODE_TYPE_H264;
	}
	else if((dwBufSize > 45) &&(lpBuf[40+0]==0 && lpBuf[40+1]==0 && lpBuf[40+2]==0 && lpBuf[40+3]==0x1)) //(������40�ֽ�ͷ)
	{
		mgwlog("�Զ����������Ϊ:h264(����40�ֽ�ͷ)\n");
		return EV9000APP_VIDEOENCODE_TYPE_H264;
	}
	else
	{
		mgwlog("�Զ����������Ϊ:UnKnow\n");
		return EV9000APP_VIDEOENCODE_TYPE_UNKNOW;
	}
	return -1;
}

//H264����
int CMediaItemReal::DealH264(unsigned char* lpBuf, DWORD dwBufSize)
{
	if (dwBufSize<6)  //̫�������쳣
	{
		return 0;
	}
	if(m_pCPSPackaging)
	{
		//��ǹ��������ÿ֡���32���ֽ�ͷ��ȥ�� 20141125 chenyu
	    if( (lpBuf[0]!=0x0) 
			|| (lpBuf[1]!=0x0)
			|| (lpBuf[2]!=0x0) 
			|| (lpBuf[3]!=0x1))
		{
			//ȥ��dah˽������
			if((dwBufSize > 37) &&(lpBuf[32+0]==0 && lpBuf[32+1]==0 && lpBuf[32+2]==0 && lpBuf[32+3]==0x1)) //(������32�ֽ�ͷ)
			{
				lpBuf+=32;
				dwBufSize -=32;
			}
			else if((dwBufSize > 45) &&(lpBuf[40+0]==0 && lpBuf[40+1]==0 && lpBuf[40+2]==0 && lpBuf[40+3]==0x1)) //(������40�ֽ�ͷ)
			{
				lpBuf+=40;
				dwBufSize -=40;
			}
		}
		
		int nDestLen = 0; 
		memset(m_pPsPackBuf,0,MAX_PSBUFFER_SIZE);
		if((0x67 == *(lpBuf+4)) || (0x27 == *(lpBuf+4)) || (0x47 == *(lpBuf+4)) || ((dwBufSize > 45) && (0x27 == *(lpBuf+40+4))))  //I֡��I֡֡ͷ (������40�ֽ�ͷ)
		{
			m_pCPSPackaging->Packet_I_frame((const char*)lpBuf,dwBufSize,(char*)m_pPsPackBuf,nDestLen,25,0,0,0);
			//mgwlog("----make I frame:%d:%d---\n",dwBufSize,nDestLen);
		}else{
			m_pCPSPackaging->Packet_P_frame((const char*)lpBuf,dwBufSize,(char*)m_pPsPackBuf,nDestLen);
			//mgwlog("----make P frame:%d:%d---\n",dwBufSize,nDestLen);
		}
		m_lpMediaDataOp->InputData(m_pPsPackBuf, nDestLen); //�������������  h264
		WriteRecFilePs(m_pPsPackBuf, nDestLen); //д¼���ļ�
	}else{
		mgwlog("m_pCPSPackaging is NULL\n");
	}
	return 0;
}

// �Ǳ�ps����
// int CMediaItemReal::DealUnGBPs(unsigned char* lpBuf, DWORD dwBufSize)
// {
// 	unsigned char*pH264Data = NULL;
// 	int iHeadLen=0,iH264Size=0;
//     //ȥ��pesͷ ֻ����pes��
// 	if(lpBuf[0]==0 && lpBuf[1]==0 && lpBuf[2]==1 
// 		&& (lpBuf[3]==0xC0    //pes audioͷ
// 		|| lpBuf[3]==0xE0    //pes vedioͷ
// 		))
// 	{
// 		pPES_HEADER_tag pPesHead = (pPES_HEADER_tag )(lpBuf);
// 		int iPesHeadLen = pPesHead->PES_header_data_length;
// 		iHeadLen = 9 + iPesHeadLen;  //ָ��pesͷ���H264ԭʼ����
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
//     if(0x67 == *(pH264Data+4)) //0x67 Sequece  ��������
// 	{
// 		if(iH264Size<MAX_PSBUFFER_SIZE)
// 		{
// 			memcpy(m_pH264Buf+0,pH264Data,iH264Size);
// 			m_dwDataSize = iH264Size;
// 		}
// 	}
// 	else if (0x68 == *(pH264Data+4)) //0x68 Picture ��������
// 	{
// 		if(m_dwDataSize + iH264Size<MAX_PSBUFFER_SIZE)
// 		{
// 			memcpy(m_pH264Buf+m_dwDataSize,pH264Data,iH264Size);
// 			m_dwDataSize += iH264Size;
// 		}
// 	}
// 	else //�� Sequece Picture
// 	{
// 		//���
// 		if(m_pCPSPackaging)
// 		{
// 			int nDestLen = 0; 
// 			memset(m_pPsPackBuf,0,MAX_PSBUFFER_SIZE);
// 			//I֡ �� IDR֡
// 			if(0x65 == pH264Data[4])
// // 				if((0x21 == pH264Data[4] && 0x88 == pH264Data[5])
// // 					|| (0x61 == pH264Data[4] && 0x88 == pH264Data[5])
// // 				|| (0x65 == pH264Data[4]))
// 			{   //��ǰ��� Sequece Picture һ����
// 				if(m_dwDataSize + iH264Size <MAX_PSBUFFER_SIZE)
// 				{
// 					memcpy(m_pH264Buf+m_dwDataSize,pH264Data,iH264Size);
// 					m_dwDataSize += iH264Size;
// 				}else{
// 					mgwlog("I֡���󣬳������壺614400\n");
// 				}
// 				m_pCPSPackaging->Packet_I_frame((const char*)m_pH264Buf,m_dwDataSize,(char*)m_pPsPackBuf,nDestLen,25,0,0,0);
// 				m_dwDataSize = 0;  //����
// 			}
// 			else
// 			{   //��ǰ���ݵİ�
// 				m_pCPSPackaging->Packet_P_frame((const char*)pH264Data,iH264Size,(char*)m_pPsPackBuf,nDestLen);
// 			}
// 			m_lpMediaDataOp->InputData(m_pPsPackBuf, nDestLen); //�������������  h264
// 			WriteRecFilePs(m_pPsPackBuf, nDestLen); //д¼���ļ�
// 		}else{
// 			mgwlog("m_pCPSPackaging is NULL\n");
// 		}
// 	}	
// 	return 0;
// }

//�ɵĴ�����
//�Ǳ�ps����
int CMediaItemReal::DealUnGBPs(unsigned char* lpBuf, DWORD dwBufSize)
{
	unsigned char*pH264Data = NULL;
	int iHeadLen=0,iH264Size=0;
    //ȥ��pesͷ ֻ����pes��
	if(lpBuf[0]==0 && lpBuf[1]==0 && lpBuf[2]==1 
		&& (lpBuf[3]==0xC0    //pes audioͷ
		|| lpBuf[3]==0xE0    //pes vedioͷ
		))
	{
		pPES_HEADER_tag pPesHead = (pPES_HEADER_tag )(lpBuf);
		int iPesHeadLen = pPesHead->PES_header_data_length;
		iHeadLen = 9 + iPesHeadLen;  //ָ��pesͷ���H264ԭʼ����
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
	//���
	if(m_pCPSPackaging)
	{
		int nDestLen = 0; 
		memset(m_pPsPackBuf,0,MAX_PSBUFFER_SIZE);
		if(0x67 == *(pH264Data+4))  //I֡��I֡֡ͷ
		{
			m_pCPSPackaging->Packet_I_frame((const char*)pH264Data,iH264Size,(char*)m_pPsPackBuf,nDestLen,25,0,0,0);
			//mgwlog("----make I frame:%d:%d---\n",dwBufSize,nDestLen);
		}else{
			m_pCPSPackaging->Packet_P_frame((const char*)pH264Data,iH264Size,(char*)m_pPsPackBuf,nDestLen);
			//mgwlog("----make P frame:%d:%d---\n",dwBufSize,nDestLen);
		}
		m_lpMediaDataOp->InputData(m_pPsPackBuf, nDestLen); //�������������  h264
		WriteRecFilePs(m_pPsPackBuf, nDestLen); //д¼���ļ�
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
		//д���ļ�
		if(!m_fw)
		{
			char szPath[256]={0};
			sprintf(szPath,".\\%srec.264",m_stChConfig.strLogicDeviceID.c_str());
			m_fw = fopen(szPath,"wb+");
			if(NULL == m_fw)
			{
				mgwlog("����¼���ļ�ʧ�ܣ�%s\n",szPath);
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
		//д���ļ�
		if(!fw1)
		{
			char szPath[256]={0};
			sprintf(szPath,".\\%srec.ps",m_stChConfig.strLogicDeviceID.c_str());
			fw1 = fopen(szPath,"wb+");
			if(NULL == fw1)
			{
				mgwlog("����¼���ļ�ʧ�ܣ�%s\n",szPath);
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
		//д���ļ�
		if(!fw2)
		{
			char szPath[256]={0};
			sprintf(szPath,".\\%srec.264S",m_stChConfig.strLogicDeviceID.c_str());
			fw2 = fopen(szPath,"wb+");
			if(NULL == fw2)
			{
				mgwlog("����¼���ļ�ʧ�ܣ�%s\n",szPath);
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
    //��ʼ�������߳�
	//���޸� ����ָ������ chenyu 131024
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
		if(0 != m_lpDataSend->StartSend(stRtppara,PROT_TYPE_RTP))  //rtp����
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
//            //mgwlog("++++�̹߳�����CMediaItemReal::OnSend()handle:[0x%x]\n",this);
// 		   dwTime = GetTickCount();
// 		}
		if(m_lpMediaDataOp)
		{
			int nLen = 0;
// 			if(0 == m_stChConfig.iStreamType)  //ps
// 			{
// 				nLen = m_lpMediaDataOp->OutputData(m_lpSendBuf,MAX_SENDBUF_LEN);  //��ȡH264����
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
// 					nLen = EV9KPsPack_OutputData(m_hPsPack,m_lpSendBuf,MAX_SENDBUF_LEN);  //��ȡH264����
// 					static int iCountSend=0;
// 					iCountSend++;
// 					if(iCountSend>100)
// 					{
// 						iCountSend =0;
// 						mgwlog("do EV9KPsPack_OutputData\n");
// 					}
// 				}
// 			}
			nLen = m_lpMediaDataOp->OutputData(m_lpSendBuf,MAX_SENDBUF_LEN);  //��ȡH264����
			if(nLen>0)
			{
				//mgwlog("CMediaItemReal::OnSend() �������ݵ��ͻ��ˣ�len:%d\n",nLen);
				int iLoopSendCount = theApp.m_iLoopSendCount; 
				if ((iLoopSendCount>1) && (iLoopSendCount<100))
				{
					int iSendCount = 0;
					while (iLoopSendCount>0)
					{
						m_lpDataSend->SendData(m_lpSendBuf, nLen); //����
						//WriteRecFile2(m_lpSendBuf, nLen); //д¼���ļ�
						m_llSendByteCount += nLen;  //ͳ�Ʒ�������
						iLoopSendCount --;
						iSendCount ++;
						if(iSendCount>20)
						{
							iSendCount =0;
                            //Sleep(1);
						}
					}
				}else{
					m_lpDataSend->SendData(m_lpSendBuf, nLen); //����
					//WriteRecFile2(m_lpSendBuf, nLen); //д¼���ļ�
					m_llSendByteCount += nLen;  //ͳ�Ʒ�������
				}
			}else{
                Sleep(1);
			}
		}else{
			Sleep(20);
		}
	}
	//MEMORY_DELETE(m_lpDataSend);
	mgwlog("�˳��߳�CMediaItemReal::OnSend()\n");
	return 0;
}

int CMediaItemReal::OnPsPack()
{
//     //��ʼ��ps�����
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
// 	//��ȡ���� ���
// 	while (m_bThreadPsPackFlag)
// 	{
// 		if(m_lpMediaDataOp)
// 		{
// 			int nLen = m_lpMediaDataOp->OutputData((unsigned char*)lpBuf,MAX_SENDBUF_LEN);  //��ȡH264����
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
// 				EV9KPsPack_InputData(m_hPsPack,lpBuf,nLen);  //����H264
// 			}else{
//                 Sleep(10);
// 			}
// 		}else{
// 			Sleep(20);
// 		}
// 	}
// 	TRACE("++++++++++++++++++++++++++�˳�OnPsPack()+++++++++++++++\n");
	return 0;
}
#endif

//�Ƿ���Ҫ�ϱ�
BOOL CMediaItemReal::NeedSendState()
{
	//�����仯���ϱ�
	if((m_eOldChannelState == m_eCurChannelState) && m_bSend)  //û�з����仯 �����ϱ������ϱ�
	{
		//mgwlog("��λû�з����仯\n");
		return FALSE;
	}else{
		return TRUE;
	}
}


//ͨ����
int CMediaItemReal::GetChannelID()
{
    return m_nID;
}

int CMediaItemReal::SetChConfig(UNGB_CHCONFIGREC &stChConfig)
{
	m_stChConfig = stChConfig;
	return  0;
}

//��ȡͨ����Ϣ
int CMediaItemReal::GetUnitInfo(UNITINFO &stUnitInfo)
{
	stUnitInfo.strLogicDeviceID = m_stChConfig.strLogicDeviceID;
	stUnitInfo.strChannelName = m_stChConfig.strChannelName;
	stUnitInfo.strDeviceIP = m_strDeviceIP;
	stUnitInfo.dwTime = m_dwTime;
	stUnitInfo.llRcvByteCount = m_llRcvByteCount;
	stUnitInfo.llSendByteCount = m_llSendByteCount;
	stUnitInfo.llLastSendByteCount = m_llLastSendByteCount;
	m_llLastSendByteCount = m_llSendByteCount;   //�ѵ�ǰֵ����last
	stUnitInfo.strDestIP = m_stStreamConInfo.strDestIP;
	stUnitInfo.iDestPort = m_stStreamConInfo.iDestPort;
	stUnitInfo.iSrcPort = m_stStreamConInfo.iSrcPort;
	stUnitInfo.eChannelState = m_eCurChannelState;
	return 0;
}

#ifdef WIN32
/////////////////
//¼��طŽڵ�
/////////////////
CPlayBack::CPlayBack(int nID) : CMediaItem(nID)
{
	m_nID = nID;
	sprintf(m_sName, "¼��ط�[%d]", nID);
	
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
	m_lpMediaDataOp =NULL;         //ת��ģ��
	m_uiDataTime = 0;
	m_bFinishCheckStream = FALSE;
	m_nCheckStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //Ĭ��δ֪
	m_nStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //Ĭ��δ֪
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

//��ȡͨ����Ϣ
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
// 		mgwlog("CPlayBack::Start δ�ҵ��豸�����˳�\n");
// 		return -1;
// 	}
// 	ConnectDevice();
// 	if (-1 == m_hConnectHandle)
// 	{//����ʧ��
// //		AddListInfo(LOG_INFO_LEVEL_FATAL, "[%s]�����豸ʧ��", GetName());
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
// // 		AddListInfo(LOG_INFO_LEVEL_FATAL, "[%s] ��¼��ʧ��", GetName());
// // 
// // 		return -3;
// // 	}
// 
// 	lpDevice->m_pSdkProxy->fEDV_DEVICE_SetRecordCallBack(m_hOpenHandle, StreamDataCallBack, this);
// 		
// //	AddListInfo(LOG_INFO_LEVEL_MAIN, "[%s]¼��ɹ�,��Ӧ���ž��[%d]", GetName(), m_hOpenHandle);
	return 0;
}

LONG CPlayBack::StartByTime()
{
	mgwlog("CPlayBack::StartByTime enter\n");
	//1.�ȹر�
	Stop();
	CAutoLock lock(&m_csPlayBackOpLock);
	mgwlog("CPlayBack::StartByTime after lock\n");
	m_dwLastSendPause = 0;
	m_dwLastSendResume = 0;
	m_bEmpty = FALSE;
	m_sStart = Time_tToSystemTime(m_stStreamConInfo.iStartTime);
	m_sStop = Time_tToSystemTime(m_stStreamConInfo.iStopTime);
	m_sPlay = Time_tToSystemTime(m_stStreamConInfo.iPlayTime);
	int nRet = 0; //Ĭ�ϳɹ�
	do 
	{
		//1.����������߳�  �����뵽MediaDataOp�д�����ٴ�MediaDataOp������
		if(!m_lpMediaDataOp)
		{
			m_stCodecinfo.iStreamType = m_stChConfig.iStreamType;
			m_stCodecinfo.bNeedCodec = m_stChConfig.iNeedCodec;
			m_stCodecinfo.strLogicID = m_stChConfig.strLogicDeviceID;
			m_stCodecinfo.nDataType = 1;  //¼�����ݱ�־
			CMediaDataOp *lpMediaDataOp = new CMediaDataOp(m_stCodecinfo);
			lpMediaDataOp->StartCodec();	  //���������
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

		//2.���������߳�
		mgwlog("����¼�����������߳�\n");
		if (!m_hThreadSend)
		{
			//mgwlog("���������߳�\n");
			m_bThreadSendFlag = TRUE;
			m_hThreadSend = CreateThread(NULL, 0, ThreadSend, (LPVOID)this, 0, &m_dwThreadSendID); //���������߳�
		}

		//3.��¼�񴴽����ص�
		CMediaItemDevice *lpDevice = NULL; 
		lpDevice = (CMediaItemDevice *)GetDevice();
		if (!lpDevice)
		{
			nRet = -1;
			break;
		}
		//��ȡ������
		int iDeviceType = lpDevice->GetDeviceType();
		if (-1 == lpDevice->GetConnectHandle())
		{
			if( 0 == lpDevice->ConnectDevice())
			{
				m_hConnectHandle = lpDevice->GetConnectHandle();
				mgwlog("�����豸�ɹ�[%d]", m_hConnectHandle);	
			}else{
				mgwlog("�����豸ʧ��");
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
			if( EV9000APP_VIDEOENCODE_TYPE_YUV == m_stChConfig.iStreamType)  //������YUV
			{
				mgwlog("CPlayBack::StartByTime() ����YUV�ص�\n");
				handle = lpDevice->m_pSdkProxy->fEDV_DEVICE_OpenRecordStreamEx(m_hConnectHandle, m_nID, m_sStart, m_sStop,m_sPlay,STREAM_DATATYPE_YUV);  //�ص�YUV����
			}else{
				mgwlog("CPlayBack::StartByTime() �������������ص�\n");
				handle = lpDevice->m_pSdkProxy->fEDV_DEVICE_OpenRecordStreamEx(m_hConnectHandle, m_nID, m_sStart, m_sStop,m_sPlay,STREAM_DATATYPE_COM);  //�����ص�
			}
		}else if(DEVICE_TYPE_BOSCH == iDeviceType)
		{
			mgwlog("CPlayBack::StartByTime() BOSCH �������������ص�\n");
			handle = lpDevice->m_pSdkProxy->fEDV_DEVICE_OpenRecordStreamEx(m_hConnectHandle, m_nID, m_sStart, m_sStop,m_sPlay,STREAM_DATATYPE_COM);  //�����ص�
		}else{
			mgwlog("CPlayBack::StartByTime() other sdk\n");
			handle = lpDevice->m_pSdkProxy->fEDV_DEVICE_OpenRecordStream(m_hConnectHandle, m_nID, m_sStart, m_sStop);  //�����ص�  
		}
		if (0 > handle)
		{
			mgwlog("���豸[%s],ͨ��[%d]¼��ʧ��,ԭ��[%d]", GetName(), m_nID, handle);
			nRet = -1;
			break;
		}
		
		int lTmpRet = lpDevice->m_pSdkProxy->fEDV_DEVICE_SetRecordCallBack(handle, StreamDataCallBack, this);
		if(lTmpRet<0)
		{
			mgwlog("����¼��ص�ʧ��[%d] handle:%d\n", lTmpRet,handle);
			nRet = -1;
			break;
		}
		m_hOpenHandle = handle;
		mgwlog("���豸[%s],ͨ��[%d]¼��ɹ�,��Ӧ���ž��[%d]", GetName(), m_nID, handle);					
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
		mgwlog("CPlayBack::Stop δ�ҵ��豸�����˳�\n");
		return;
	}
	mgwlog("CPlayBack::Stop Start to SetRecordCallBack\n");
	if(m_hOpenHandle!=-1)
	{
		//lpDevice->m_pSdkProxy->fEDV_DEVICE_SetRecordCallBack(m_hOpenHandle, NULL, NULL);  //ע�� chenyu 140703
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
	MEMORY_DELETE(m_pCPSPackaging);   //�ͷ� PsPack
	MEMORY_DELETE_EX(m_pPsPackBuf);
	m_uiDataTime = 0;
	m_bPause = FALSE;
	m_bEmpty = FALSE;
	m_bFinishCheckStream = FALSE;
	m_nCheckStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //Ĭ��δ֪
	m_nStreamType = EV9000APP_VIDEOENCODE_TYPE_UNKNOW;  //Ĭ��δ֪

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
		mgwlog("CPlayBack::CtrlRecordStream m_hOpenHandle==-1 �˳�\n");
		return -1;
	}
	BOOL bCtrlFront = FALSE;
	CMediaItemDevice *lpDevice = (CMediaItemDevice *)GetDevice();
	if(!lpDevice)
	{
		mgwlog("CPlayBack::CtrlRecordStream δ�ҵ��豸�����˳�\n");
		return -1;
	}
	if (RECORD_CTRL_SETPLAYPOS == eRecordCtrl)
	{
		bCtrlFront = TRUE;
		m_bPause = FALSE;
		m_bEmpty = TRUE;    //֪ͨ�ص��߳�Ҳ��һ��
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
	       mgwlog("ִ��seek����\n");		
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
		mgwlog("CPlayBack::CtrlRecordStream δ�ҵ��豸�����˳�\n");
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
		//д���ļ�
		if(!m_fw)
		{
			char szPath[256]={0};
			sprintf(szPath,".\\%srec.264",m_stChConfig.strLogicDeviceID.c_str());
			m_fw = fopen(szPath,"wb+");
			if(NULL == m_fw)
			{
				mgwlog("����¼���ļ�ʧ�ܣ�%s\n",szPath);
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
	//Warning:�ص�����������ͣ ����ᵼ��CChannelItemRecordEV8K::OnRecordData���ܳ���
	if((!lpBuf)||(dwBufSize <=0)||(!lpDataInfo)||(!m_lpReadBuffer))
	{
		return -1;
	}

	//1.���������
	if(FALSE == m_bFinishCheckStream)
	{
		int nRet = CheckStreamType(lpBuf,dwBufSize);
		if(-1 == nRet)  //���ʧ���˳����������
		{
			mgwlog("�Զ����������ʧ���˳����������\n");
			return 0;
		}else{
			m_nCheckStreamType = nRet;  //���õ�������
			if (m_nCheckStreamType != EV9000APP_VIDEOENCODE_TYPE_UNKNOW) //��⵽ȷ������
			{
				m_nStreamType = m_nCheckStreamType;
			}else{
				m_nStreamType = m_stChConfig.iStreamType;   //��������������
			}
			mgwlog("PlayBack:��������:%d �Զ��������:%d �������ж�����:%d\n",m_stChConfig.iStreamType,m_nCheckStreamType,m_nStreamType);
			m_bFinishCheckStream  = TRUE;
		}
	}

	if(m_lpMediaDataOp)  //�ص�¼����������
	{
		if (m_bEmpty)
		{
			m_lpMediaDataOp->ClearBuff();
			m_bEmpty = FALSE;
			mgwlog("���¼�񻺳�\n");
		}
		//mgwlog("CMediaItemReal::OnStreamData inputdata �ص�����");
		if(EV9000APP_VIDEOENCODE_TYPE_YUV == m_nStreamType)  //YUV����
		{
			if(lpDataInfo)
			{
				m_Frame.nHeight = lpDataInfo->nHeight;
				m_Frame.nWidth = lpDataInfo->nWidth;
				m_Frame.pbuffer = lpBuf;
				m_Frame.lenth = dwBufSize;
				m_lpMediaDataOp->InputYUVData(&m_Frame,0); //������������� YUV
			}	
		}
		else if((EV9000APP_VIDEOENCODE_TYPE_H264 == m_nStreamType)||(2==m_nStreamType)) //�����h264��ת��Ϊps
		{
			DealH264(lpBuf,dwBufSize);
		}
		else if((EV9000_STREAMDATA_TYPE_PS == m_nStreamType)||(0==m_nStreamType)) //�Ǳ�ps��ת��Ϊ��׼ps
		{
			//DealUnGBPs(lpBuf,dwBufSize);    //�°汾��������ps���ͻ��˴���
			m_lpMediaDataOp->InputData(lpBuf, dwBufSize);
		}
		else  //������ֱ������MediaDataOp�д���
		{
			m_lpMediaDataOp->InputData(lpBuf, dwBufSize);
		}
		m_uiDataTime = lpDataInfo->uiDataTime;  //���浱ǰ����ʱ��
	}
	//��ͣ
	int nOutBuffDataLen = m_lpMediaDataOp->GetOutBuffDataLen();
	if(nOutBuffDataLen>(REC_RINGBUF_LEN*2/3))  //>2/3
	{
		DWORD dwTime = GetTickCount();
		if((dwTime - m_dwLastSendPause)> 60)  //100ms
		{
			int nRet = CtrlRecordStreamFront(RECORD_CTRL_PAUSE,0,NULL);
			mgwlog("���峬��2/3 ����size:%d kb,������ͣ����,ִ�н��:%d\n",nOutBuffDataLen/1024,nRet);
			m_dwLastSendPause = dwTime;
		}
	}
	return 0;
}

//���������
/*  
*ps   ---- EV9000_STREAMDATA_TYPE_PS 
*h264 ---- EV9000APP_VIDEOENCODE_TYPE_H264
*δ֪ ---- EV9000APP_VIDEOENCODE_TYPE_UNKNOW
*ʧ�� ---- -1
*/
int CPlayBack::CheckStreamType(unsigned char* lpBuf, DWORD dwBufSize)
{
	if(dwBufSize < 6)  
	{
		return -1;  //̫�̼��ʧ��
	}
	if(lpBuf[0]==0 && lpBuf[1]==0 && lpBuf[2]==1 
		&& ( lpBuf[3]==0xBA     //Program Pack
		|| lpBuf[3]==0xE0     //pes vedioͷ
		|| lpBuf[3]==0xC0    //pes audioͷ
		|| lpBuf[3]==0xBB    //systemͷ
		))
	{
		mgwlog("PlayBack:�Զ����������Ϊ:ps\n");
		return EV9000_STREAMDATA_TYPE_PS;
	}
	else if(lpBuf[0]==0 && lpBuf[1]==0 && lpBuf[2]==0 && lpBuf[3]==0x1)
	{
		mgwlog("PlayBack:�Զ����������Ϊ:h264\n");
		return EV9000APP_VIDEOENCODE_TYPE_H264;
	}
	else if((dwBufSize > 45) &&(lpBuf[40+0]==0 && lpBuf[40+1]==0 && lpBuf[40+2]==0 && lpBuf[40+3]==0x1)) //(������40�ֽ�ͷ)
	{
		mgwlog("PlayBack:�Զ����������Ϊ:h264(����40�ֽ�ͷ)\n");
		return EV9000APP_VIDEOENCODE_TYPE_H264;
	}
	else
	{
		mgwlog("PlayBack:�Զ����������Ϊ:UnKnow\n");
		return EV9000APP_VIDEOENCODE_TYPE_UNKNOW;
	}
	return -1;
}

//H264����
int CPlayBack::DealH264(unsigned char* lpBuf, DWORD dwBufSize)
{
	if(m_pCPSPackaging)
	{
		int nDestLen = 0; 
		memset(m_pPsPackBuf,0,MAX_PSBUFFER_SIZE);
		if((0x67 == *(lpBuf+4)) || (0x27 == *(lpBuf+4)) ||(0x47 == *(lpBuf+4)) || ((dwBufSize > 45) && (0x27 == *(lpBuf+40+4))))  //I֡��I֡֡ͷ (������40�ֽ�ͷ)
		{
			m_pCPSPackaging->Packet_I_frame((const char*)lpBuf,dwBufSize,(char*)m_pPsPackBuf,nDestLen,25,0,0,0);
			//mgwlog("----make I frame:%d:%d---\n",dwBufSize,nDestLen);
		}else{
			m_pCPSPackaging->Packet_P_frame((const char*)lpBuf,dwBufSize,(char*)m_pPsPackBuf,nDestLen);
			//mgwlog("----make P frame:%d:%d---\n",dwBufSize,nDestLen);
		}
		m_lpMediaDataOp->InputData(m_pPsPackBuf, nDestLen); //�������������  h264
		//WriteRecFilePs(m_pPsPackBuf, nDestLen); //д¼���ļ�
	}else{
		mgwlog("m_pCPSPackaging is NULL\n");
	}
	return 0;
}

//�Ǳ�ps����
int CPlayBack::DealUnGBPs(unsigned char* lpBuf, DWORD dwBufSize)
{
	unsigned char*pH264Data = NULL;
	int iHeadLen=0,iH264Size=0;
    //ȥ��pesͷ ֻ����pes��
	if(lpBuf[0]==0 && lpBuf[1]==0 && lpBuf[2]==1 
		&& (lpBuf[3]==0xC0    //pes audioͷ
		|| lpBuf[3]==0xE0    //pes vedioͷ
		))
	{
		pPES_HEADER_tag pPesHead = (pPES_HEADER_tag )(lpBuf);
		int iPesHeadLen = pPesHead->PES_header_data_length;
		iHeadLen = 9 + iPesHeadLen;  //ָ��pesͷ���H264ԭʼ����
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
	//���
	if(m_pCPSPackaging)
	{
		int nDestLen = 0; 
		memset(m_pPsPackBuf,0,MAX_PSBUFFER_SIZE);
		if(0x67 == *(pH264Data+4))  //I֡��I֡֡ͷ
		{
			m_pCPSPackaging->Packet_I_frame((const char*)pH264Data,iH264Size,(char*)m_pPsPackBuf,nDestLen,25,0,0,0);
			//mgwlog("----make I frame:%d:%d---\n",dwBufSize,nDestLen);
		}else{
			m_pCPSPackaging->Packet_P_frame((const char*)pH264Data,iH264Size,(char*)m_pPsPackBuf,nDestLen);
			//mgwlog("----make P frame:%d:%d---\n",dwBufSize,nDestLen);
		}
		m_lpMediaDataOp->InputData(m_pPsPackBuf, nDestLen); //�������������  h264
		//WriteRecFilePs(m_pPsPackBuf, nDestLen); //д¼���ļ�
	}else{
		mgwlog("m_pCPSPackaging is NULL\n");
	}
	return 0;
}

int CPlayBack::OnSend()
{
    //��ʼ�������߳�
	//���޸� ����ָ������ chenyu 131024
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
		if(0 != m_lpDataSend->StartSend(stRtppara,PROT_TYPE_RTP))  //RTP ����
		{
			MEMORY_DELETE(m_lpDataSend);
			mgwlog("Warning:Create CDataSend fail \n");
			m_hThreadSend =0;
			return -1;
		}
	}
	if(theApp.m_iConSendPackLen<=0) 
	{
		theApp.m_iConSendPackLen = 10;   //1000��/second
	}
	if(theApp.m_iConSendPackLen>=(MAX_READ_LEN/MAX_RTP_PAYLOAD_LEN))
	{
		theApp.m_iConSendPackLen = (MAX_READ_LEN/MAX_RTP_PAYLOAD_LEN);
	}
	while (m_bThreadSendFlag)
	{
		if(m_lpMediaDataOp)
		{
			//��⻺��������
			int nOutBuffDataLen = m_lpMediaDataOp->GetOutBuffDataLen();
			if(nOutBuffDataLen<(REC_RINGBUF_LEN*1/4) )  //<1/4
			{
				DWORD dwTime = GetTickCount();
				if((dwTime - m_dwLastSendResume)> 60)  //60ms
				{
					int nRet = CtrlRecordStreamFront(RECORD_CTRL_RESUME,0,NULL);
					//mgwlog("�������1/4 ����size:%d,���ͼ�������,ִ�н��:%d\n",m_lpFrameQue->GetCount(),nRet);
					m_dwLastSendResume = dwTime;
				}
			}
			while (m_bPause && m_bThreadSendFlag)
			{
				Sleep(10);
				continue;
			}
			int nLen = 0;
			nLen = m_lpMediaDataOp->OutputData(m_lpReadBuffer,theApp.m_iConSendPackLen*MAX_RTP_PAYLOAD_LEN);  //��ȡH264����
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
	mgwlog("�˳��߳�CPlayBack::OnSend()\n");
	return 0;	
}

// int CPlayBack::OnStreamData(EDVOPENHANDLE handle, LPDATA_INFO lpDataInfo, unsigned char* lpBuf, DWORD dwBufSize)
// {
// 	//Warning:�ص�����������ͣ ����ᵼ��CChannelItemRecordEV8K::OnRecordData���ܳ���
// 	if((!lpBuf)||(dwBufSize <=0)||(!lpDataInfo)||(!m_lpFrameQue)||(!m_lpInData)||(!m_lpReadBuffer))
// 	{
// 		return -1;
// 	}
// 	//д�뻺��
// 	if(m_lpInData)
// 	{
// 		int nLen = 0;
// 		int nDataLen=0;
// 		if((EV9000APP_VIDEOENCODE_TYPE_H264 == m_stChConfig.iStreamType)||(2==m_stChConfig.iStreamType)) //�����h264��ת��Ϊps
// 		{
// 			if(m_pCPSPackaging)
// 			{
// 				int nDestLen = 0; 
// 				memset(m_pPsPackBuf,0,MAX_PSBUFFER_SIZE);
// 				if(FRAME_DATA_TYPE_IFRAME == lpDataInfo->eDataType)  //I֡��I֡֡ͷ
// 				{
// 					m_pCPSPackaging->Packet_I_frame((const char*)lpBuf,dwBufSize,(char*)m_pPsPackBuf,nDestLen,25,0,0,0);
// 					//mgwlog("----make I frame:%d:%d---\n",dwBufSize,nDestLen);
// 				}else{
// 					m_pCPSPackaging->Packet_P_frame((const char*)lpBuf,dwBufSize,(char*)m_pPsPackBuf,nDestLen);
// 					//mgwlog("----make P frame:%d:%d---\n",dwBufSize,nDestLen);
// 				}
// 				nLen = m_lpInData->Write(m_pPsPackBuf,nDestLen);  //����ps
// 				nDataLen = nDestLen;
// 			}else{
// 				mgwlog("m_pCPSPackaging is NULL\n");
// 			}
// 		}else{
// 			nLen = m_lpInData->Write(lpBuf,dwBufSize); //����ps
// 			nDataLen = dwBufSize;
// 		}
// 
// 		if(nLen < nDataLen)
// 		{
// 			mgwlog("CPlayBack::OnStreamData д���������,����д��:%d �ɹ�:%d \n",dwBufSize,nLen);	
// // 			m_nOverflowNum++;
// // 			if(0 == m_nOverflowNum%1000)
// // 			{
// // 				mgwlog("CMediaDataOp::InputData д���������,����д��:%d �ɹ�:%d ���������:%d\n",dwBufSize,nLen,m_nOverflowNum);
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
// 			mgwlog("�ص�����Ҳ���һ�λ���\n");
// 		}
// 		//��ͣ
//         if(m_lpFrameQue->GetCount()>(MAX_RECORD_DATABUF_LEN/MAX_RTP_PAYLOAD_LEN*2/3))  //>2/3
// 		{
// 			DWORD dwTime = GetTickCount();
// 			if((dwTime - m_dwLastSendPause)> 60)  //100ms
// 			{
// 				int nRet = CtrlRecordStreamFront(RECORD_CTRL_PAUSE,0,NULL);
// 				mgwlog("���峬��2/3 ����size:%d,������ͣ����,ִ�н��:%d\n",m_lpFrameQue->GetCount(),nRet);
// 				m_dwLastSendPause = dwTime;
// 			}
// 		}
// 		
// 		//��ȡ������֡
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
// 						pFreeBuf->nFrameRate = lpDataInfo->uiDataTime;  //��¼����ʱ��
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
//     //��ʼ�������߳�
// 	//���޸� ����ָ������ chenyu 131024
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
// 	BOOL bSleepMark = FALSE;  //sleep ��־
// 	BOOL bGetFrameSuccess = TRUE;
// 	while (m_bThreadSendFlag)
// 	{
// 		static DWORD dwTime = GetTickCount();
// 		if((GetTickCount()-dwTime)>PRINT_TIME_SPAN)  //30s 
// 		{
// 			//mgwlog("++++�̹߳�����CPlayBack::OnSend()handle:[0x%x]\n",this);
// 			dwTime = GetTickCount();
// 		}
// 		bSleepMark = FALSE;
// 	    bGetFrameSuccess = TRUE;
// 		//��ȡ����
// 		T_FrameBuf* lpTempBuf =NULL;
// 		if(!m_lpFrameQue)
// 		{
//             Sleep(10);
// 			continue;
// 		}
// 		//����
//         if(m_lpFrameQue->GetCount()<(MAX_RECORD_DATABUF_LEN/MAX_RTP_PAYLOAD_LEN*1/4) )  //<1/4
// 		{
// 			DWORD dwTime = GetTickCount();
// 			if((dwTime - m_dwLastSendResume)> 60)  //60ms
// 			{
// 				int nRet = CtrlRecordStreamFront(RECORD_CTRL_RESUME,0,NULL);
// 				//mgwlog("�������1/4 ����size:%d,���ͼ�������,ִ�н��:%d\n",m_lpFrameQue->GetCount(),nRet);
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
// 				m_lpFrameQue->InsertFreeBuf(lpTempBuf); //�ͷ�
// 				bGetFrameSuccess = TRUE;
// 			}else{
// 				bGetFrameSuccess = FALSE;
// 			}
// 		}
// 		if(FALSE == bGetFrameSuccess)  //ȡ֡ʧ��Sleep
// 		{
//             Sleep(10);
// 		}
// 		if(TRUE == bSleepMark)
// 		{
//             Sleep(10);
// 		}
// 	}
// 	//MEMORY_DELETE(m_lpDataSend);
// 	mgwlog("�˳��߳�CPlayBack::OnSend()\n");
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
// 						AddListInfo(LOG_INFO_LEVEL_FATAL, "�����豸[%s]ʧ��,ԭ��[%d]", lpDeviceInfo->GetName(), nRet);
// 						
// 						return 1;
// 					}
// 					
// 					AddListInfo(LOG_INFO_LEVEL_MAIN, "�����豸[%s]�ɹ�", lpDeviceInfo->GetName());
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
		mgwlog("CPlayBack::DisConnectDeviceδ�ҵ��豸�����˳�\n");
		return ;
	}
	lpDevice->m_pSdkProxy->fEDV_DEVICE_DisConnectDevice(m_hConnectHandle);
	m_hConnectHandle = -1;
}
#endif


