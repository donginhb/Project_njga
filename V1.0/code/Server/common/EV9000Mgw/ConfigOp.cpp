// ConfigOp.cpp: implementation of the CConfigOp class.
//
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include "stdafx.h"
#include "EV9000Mgw.h"
#include "../public/net/SockObj.h"
#else
#include "IniOper/ConfOper.h"
#endif
#include "ConfigOp.h"
#include "DbOp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifdef WIN32
extern CEV9000MgwApp theApp;
#endif
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CSysCfg * CSysCfg::instance = 0;
BOOL CSysCfg::m_bDevInited = FALSE;
#ifdef WIN32
CMutex CSysCfg::mutex;
CCritSec CSysCfg::m_mapdbLock; 
#else
pthread_mutex_t CSysCfg::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t CSysCfg::m_mapdbLock = PTHREAD_MUTEX_INITIALIZER;
#endif

CSysCfg::CSysCfg()
{
	m_iRecID = 0;
	m_strCmsIP = "";
	m_strServerID = "";
	m_dwServerSipPort = 0;
    m_strServerSipPort = "";
	m_strSipRefreshTime ="30";   //默认30s
	m_strDbIP = "";
	m_strDbPath = "";
	m_strDbPort = "";
	m_dwDbPort = 0;
	m_strLocalIP_in = "";
	m_strLocalIP_out = "";
	m_dwLocalIP = 0;
	m_strMgwDevID = "";   //设备ID 
	m_strUserName = "";
	m_strUserPwd = "";
    m_pDbOp=NULL;
	m_iConSendPackLen = 64;
}

CSysCfg::~CSysCfg()
{
	Fini();
}

CSysCfg * CSysCfg::Instance()
{
#ifdef WIN32
	mutex.Lock();
	if (instance == 0) {
        instance = new CSysCfg();
		instance->Init();
	}
	mutex.Unlock();
	return instance;
#else
	CSysCfg * instanceTmp =NULL;
	if(NULL == instance)
	{
		CAutoLock theLock(&mutex);  //?????
		if(NULL == instance)
		{
			instanceTmp=new CSysCfg();
			if(NULL == instanceTmp)
			{
				mgwlog("CSysCfg new fail\n");
				exit(1);
			}
			instanceTmp->Init();
			instance = instanceTmp;
		}
	}
	return instance;
#endif
}

//初始化配置信息 从数据库中读取数据
int CSysCfg::Init()
{
	if(QueryBaseCfg() && QueryDevChCfg()) //查询设备信息
	{
		return 0;
	}else{
		return -1;
	}
	return 0;
}

BOOL CSysCfg::Fini()
{
	return TRUE;
}

DWORD CSysCfg::GetdwPara(SYS_CFG_TYPE eType)
{
	DWORD dwPara=0;
	switch (eType) 
	{  
	case SYS_CFG_DBPORT:
		{
			dwPara = m_dwDbPort;
			break;
		}
	case SYS_CFG_SERVERSIPPORT:
		{
			dwPara = m_dwServerSipPort;
			break;
		}
	case SYS_CFG_RECID:
		{
			
			dwPara= m_iRecID;
			break;
		}
	case SYS_CFG_CONSENDPACKLEN:
		{
			
			dwPara= m_iConSendPackLen;
			break;
		}
	default:
		{
			dwPara =0;
			break;
		}
	}
    return dwPara;
}

string CSysCfg::GetstrPara(SYS_CFG_TYPE eType)
{
	string strPara="";
	switch (eType) 
	{  
	case SYS_CFG_CMSIP:
		{
			strPara= m_strCmsIP;
			break;
		}
	case SYS_CFG_SERVERID: 
		{
			strPara = m_strServerID;
			break;
		}
	case SYS_CFG_SERVERSIPPORT: 
		{
			strPara = m_strServerSipPort;
			m_dwServerSipPort = atoi(m_strServerSipPort.c_str());
			break;
		}
	case SYS_CFG_SIPREFRESHTIME: 
		{
			strPara = m_strSipRefreshTime;    //刷新时间
			break;
		}
	case SYS_CFG_DBIP:
		{
			strPara= m_strDbIP;
			break;
		}
	case SYS_CFG_DBPATH:
		{
			strPara = m_strDbPath;
			break;
		}
	case SYS_CFG_DBPORT:
		{
			strPara = m_strDbPort;
			break;
		}
	case SYS_CFG_LOCALIP_IN:  
		{
			strPara = m_strLocalIP_in;
			break; 
		}
	case SYS_CFG_LOCALIP_OUT:  
		{
			strPara = m_strLocalIP_out;
			break; 
		}
	case SYS_CFG_LOCALIP_INTERCOM:  
		{
			if(m_strLocalIP_in !="")  //设备网不空则取设备网，否则取视频网,用于内部通讯
			{
				strPara = m_strLocalIP_in;
			}else{
				strPara = m_strLocalIP_out;
			}
			break; 
		}
	case SYS_CFG_MGWDEVID:  
		{
			strPara = m_strMgwDevID;
			break; 
		}
	case SYS_CFG_USERNAME:
		{
			strPara= m_strUserName;
			break;
		}
	case SYS_CFG_USERPWD:
		{
			strPara= m_strUserPwd;
			break;
		}
	default:
		{
			strPara ="";
			break;
		}
	}
	return strPara;
}

// 获取设备信息
BOOL CSysCfg::GetDevCfg(map<int,UNGB_DEVCONFIGREC> &mapDevConfig)
{
	try
	{
		CAutoLock lock(&CSysCfg::m_mapdbLock);
		mapDevConfig = m_mapDevConfig;
		return TRUE;
	}
	catch (...)
	{
		mgwlog("CSysCfg::GetDevCfg 发生异常\n");
		return FALSE;
	}
} 

// 获取通道信息
BOOL CSysCfg::GetChCfg(map<string,UNGB_CHCONFIGREC> &mapChConfig)
{
	try
	{
		CAutoLock lock(&CSysCfg::m_mapdbLock);
		mapChConfig = m_mapChConfig;
		return TRUE;
	}
	catch (...)
	{
		mgwlog("CSysCfg::GetChCfg 发生异常\n");
		return FALSE;
	}	
}

BOOL CSysCfg::GetOneDevCfg(int iID,UNGB_DEVCONFIGREC &stDevCfg)
{
	CAutoLock lock(&CSysCfg::m_mapdbLock);
	map<int,UNGB_DEVCONFIGREC>::iterator iter=m_mapDevConfig.find(iID);
	if(iter != m_mapDevConfig.end())
	{
		stDevCfg = iter->second;
		return TRUE;  //没有找到指定设备
	}
	mgwlog("CSysCfg::GetOneDevCfg 没有找到指定设备配置\n");
	return FALSE;
}

BOOL CSysCfg::GetOneChCfg(string strDeviceID,UNGB_CHCONFIGREC &stChCfg)
{
	CAutoLock lock(&CSysCfg::m_mapdbLock);
	map<string,UNGB_CHCONFIGREC>::iterator iter = m_mapChConfig.find(strDeviceID);
	if(iter != m_mapChConfig.end())
	{
		stChCfg = iter->second;
		return TRUE;  //找到指定设备
	}
	mgwlog("CSysCfg::GetOneChCfg 没有找到指定设备配置\n");
	return FALSE;
}

//查询基本配置信息
BOOL CSysCfg::QueryBaseCfg()
{
#ifdef WIN32
	//查找内外网地址手动配置
	DWORD dwOuterIP=0;
	char chIP[32] = {'\0'};
	int nRet =0;
	if(!m_pDbOp) 
	{
		m_pDbOp = new CDbOp();
		nRet = m_pDbOp->DBOpen(NULL,0,NULL);
		if(nRet !=0)
		{
			TRACE("pDbOp->DBOpen fail\n");
			MEMORY_DELETE(m_pDbOp);
			return FALSE;
		}
	}
		
    char szCmsIP[256]={0};
	char szServerID[256]={0};
	char szServerSipPort[256]={0};
	char szSipRefreshTime[256]={0};
	char szDbIP[256]={0};
	char szDbPath[256]={0};
	char szDbPort[256]={0};
	char szLocalIP_in[256]={0};
	char szLocalIP_out[256]={0};
	char szMgwDevID[256]={0};
	char szUserName[256]={0};
	char szUserPwd[256]={0};
//	char szUserID[256]={0};

    nRet = m_pDbOp->DBQuery("select * from MgwLoginConfig");
	if(nRet!=0)
	{
		MEMORY_DELETE(m_pDbOp);
		return FALSE;
	}	
    if(false == m_pDbOp->DBEof())
	{
		mgwlog("查询基本配置信息成功\n");
        m_pDbOp->GetFieldValue("ID",m_iRecID);
		m_pDbOp->GetFieldValue("CmsIP",szCmsIP,256);
		m_pDbOp->GetFieldValue("ServerID",szServerID,256);
		m_pDbOp->GetFieldValue("ServerSipPort",szServerSipPort,256);
		m_pDbOp->GetFieldValue("SipRefreshTime",szSipRefreshTime,256);
		m_pDbOp->GetFieldValue("DbIP",szDbIP,256);
		m_pDbOp->GetFieldValue("DbPath",szDbPath,256);
		m_pDbOp->GetFieldValue("DbPort",szDbPort,256);
		m_pDbOp->GetFieldValue("LocalIP_in",szLocalIP_in,256);
		m_pDbOp->GetFieldValue("LocalIP_out",szLocalIP_out,256);
		m_pDbOp->GetFieldValue("MgwDevID",szMgwDevID,256);
		m_pDbOp->GetFieldValue("UserName",szUserName,256);
		m_pDbOp->GetFieldValue("UserPwd",szUserPwd,256);
		m_pDbOp->GetFieldValue("Resved1",m_iConSendPackLen);
		if (0 == m_iConSendPackLen)
		{
            m_iConSendPackLen = 10;
		}
		theApp.m_iConSendPackLen = m_iConSendPackLen;
	}
   
	m_strCmsIP =  szCmsIP;
	m_strServerID = szServerID;
	m_strServerSipPort = szServerSipPort;
	m_strSipRefreshTime = szSipRefreshTime;
	m_strDbIP = szDbIP;
	m_strDbPath = szDbPath;
	m_strDbPort = szDbPort;
	m_strLocalIP_in = szLocalIP_in;
	m_strLocalIP_out = szLocalIP_out;
	m_strMgwDevID =szMgwDevID;
	m_strUserName =szUserName;
	m_strUserPwd = szUserPwd;

// 	m_dwLocalIP = dwOuterIP;
// 	m_strLocalIP_in = chIP;
	m_dwDbPort  = atoi(m_strDbPort.c_str());
	m_dwServerSipPort= atoi(m_strServerSipPort.c_str()) ;
    return TRUE;
#else
    //从配置文件读取
	string cfgpath = "/config/cmscof.cfg";
	string vidip,vidflag,vidport,devip,devflag,devport;
	vidip=ConfOper::ReadString("cms","vidip","172.18.13.100",cfgpath.c_str());
	vidflag=ConfOper::ReadString("cms","vidflag","0",cfgpath.c_str());
	vidport=ConfOper::ReadString("cms","vidport","5060",cfgpath.c_str());
	devip=ConfOper::ReadString("cms","devip","192.168.1.100",cfgpath.c_str());
	devflag=ConfOper::ReadString("cms","devflag","0",cfgpath.c_str());
	devport=ConfOper::ReadString("cms","devport","5060",cfgpath.c_str());
    if("1" == devflag)  //启用设备网
    {
    	m_dwServerSipPort=atoi(devport.c_str());
    	m_strCmsIP=devip;
		m_strLocalIP_in = devip;
    }else{
    	m_dwServerSipPort=atoi(vidport.c_str());
    	m_strCmsIP=vidip;
		m_strLocalIP_in = "";
    }
	m_strServerID=ConfOper::ReadString("cms","cmsid","32011501002000000001",cfgpath.c_str());
	m_strSipRefreshTime=ConfOper::ReadString("cms","SipRefreshTime","30",cfgpath.c_str());
	m_strDbIP=ConfOper::ReadString("cms","dbip","172.18.13.100",cfgpath.c_str());
	m_strDbPath="/data/EV9000DB";
	m_strDbPort=ConfOper::ReadString("cms","DbPort","3306",cfgpath.c_str());
	m_dwDbPort=atoi(m_strDbPort.c_str());
	m_dwLocalIP= iptoint(m_strLocalIP_in.c_str());
	m_strLocalIP_out = vidip;
	m_strMgwDevID=ConfOper::ReadString("cms","MgwDevID","32011501002090000001",cfgpath.c_str());
	m_strUserName=ConfOper::ReadString("cms","UserName","32011501002090000001",cfgpath.c_str());
	m_strUserPwd=ConfOper::ReadString("cms","UserPwd","12345678",cfgpath.c_str());
	return TRUE;
#endif
}

//查询设备信息
BOOL CSysCfg::QueryDevChCfg()
{
	CAutoLock lock(&CSysCfg::m_mapdbLock);
	int nRet =0;
	if(!m_pDbOp) 
	{
		m_pDbOp = new CDbOp();
		nRet = m_pDbOp->DBOpen((char*)m_strDbIP.c_str(),m_dwDbPort,(char*)m_strDbPath.c_str());
		if(nRet !=0)
		{
			TRACE("pDbOp->DBOpen fail\n");
			MEMORY_DELETE(m_pDbOp);
			return FALSE;
		}
	}
	m_mapDevConfig.clear(); //清空信息
#ifdef WIN32
	nRet = m_pDbOp->DBQuery("select * from UnGBPhyDeviceConfig");
#else
	nRet = m_pDbOp->DBQuery("select * from OnvifPhyDeviceConfig");
#endif
	if(nRet !=0)
	{
		TRACE("pDbOp->DBQuery OnvifPhyDeviceConfig fail\n");
		MEMORY_DELETE(m_pDbOp);
		return FALSE;
	}
    UNGB_DEVCONFIGREC stDevConfig; 
	int    iEnable = 0;
	int    iID =0;
	int    iDeviceType =0;
	//string strDeviceIP="";
	char   szDeviceName[256]={0};
	char   szDeviceIP[256]={0};
	int    iDevicePort =0;
	//string strUseName="";
	//string strPassword="";
	char   szUserName[256]={0};
	char   szPassword[256]={0};
	int    iStreamType=0;
	int    iRecordType=0;
	int    iReserved1=0;
	string strReserved2="";

#ifdef WIN32
	while(false == m_pDbOp->DBEof())
#else
	while(m_pDbOp->DBGetTotalNum()>0)
#endif
	{
		memset(szDeviceName,0,256);
		memset(szDeviceIP,0,256);
		memset(szUserName,0,256);
		memset(szPassword,0,256);

		m_pDbOp->GetFieldValue("ID",iID);
#ifdef WIN32
		m_pDbOp->GetFieldValue("DeviceName",szDeviceName,255);
		m_pDbOp->GetFieldValue("DevicePort",iDevicePort);
#else
		m_pDbOp->GetFieldValue("DeviceID",szDeviceName,255);
		m_pDbOp->GetFieldValue("Resved1",iDevicePort);
        m_pDbOp->GetFieldValue("Enable",iEnable);
#endif
		m_pDbOp->GetFieldValue("DeviceType",iDeviceType);
		m_pDbOp->GetFieldValue("DeviceIP",szDeviceIP,255);
		m_pDbOp->GetFieldValue("UserName",szUserName,255);
		m_pDbOp->GetFieldValue("Password",szPassword,255);

		stDevConfig.iID=iID;
		stDevConfig.strDeviceName = szDeviceName;
		stDevConfig.iDeviceType = iDeviceType;
		stDevConfig.strDeviceIP = szDeviceIP;
		stDevConfig.iDevicePort = iDevicePort;
		stDevConfig.strUseName = szUserName;
		stDevConfig.strPassword= szPassword;
#ifdef WIN32			
		m_mapDevConfig[iID]=stDevConfig;
#else
		if(1==iEnable)
		{
			m_mapDevConfig[iID]=stDevConfig;
		}
#endif
// 		mgwlog("devinfo:%d %d %s %s %s ID: %d\n",
// 			stDevConfig.iDeviceType,stDevConfig.iDevicePort,
// 			stDevConfig.strDeviceIP.c_str(),stDevConfig.strUseName.c_str(),
// 			stDevConfig.strPassword.c_str(),stDevConfig.iID);
		if(m_pDbOp->DBMoveNext()!=0)  //失败退出
		{
			mgwlog("物理表MoveNext fail\n");
			break;
		}
	}
	mgwlog("DevConfig 记录数:%d\n",m_mapDevConfig.size());

#ifdef WIN32
	nRet = m_pDbOp->DBQuery("select * from UnGBPhyDeviceChannelConfig");
#else
	nRet = m_pDbOp->DBQuery("select * from OnvifLogicDeviceConfig");
#endif
	if(nRet !=0)
	{
		TRACE("pDbOp->DBQuery OnvifLogicDeviceConfig fail\n");
		MEMORY_DELETE(m_pDbOp);
		return FALSE;
	}
	m_mapChConfig.clear(); //清空信息
    UNGB_CHCONFIGREC stChConfig; 
	//int    iID=0;
	int    iDeviceIndex=0;        //所属设备索引
	//string strLogicDeviceID="";    //通道ID
	//string strDeviceChannel="";
	char   szLogicDeviceID[256]={0};    //通道ID
	char   szChannelName[256]={0};
	char   szChannelIP[256]={0};
	int    iChannelPort=0;
	int    iMapChannel=0;         //通道号
	char   szMapChannel[256]={0};      //通道标志
	int    iNeedCodec=0;
	int    iStreamSubType =0;
	int    iCamType =0;
	//int    iReserved1=0;
	//string strReserved2="";
	
#ifdef WIN32
	while(false == m_pDbOp->DBEof())
#else
	while(m_pDbOp->DBGetTotalNum()>0)
#endif
	{
		memset(szLogicDeviceID,0,256);
	    memset(szChannelName,0,256);
	    memset(szMapChannel,0,256);

		m_pDbOp->GetFieldValue("ID",iID);
#ifdef WIN32
		m_pDbOp->GetFieldValue("DeviceIndex",iDeviceIndex);
		m_pDbOp->GetFieldValue("LogicDeviceID",szLogicDeviceID,255);
		m_pDbOp->GetFieldValue("ChannelName",szChannelName,255);
		m_pDbOp->GetFieldValue("MapChannel",szMapChannel,255);
#else
		m_pDbOp->GetFieldValue("PhyDeviceIndex",iDeviceIndex);
		m_pDbOp->GetFieldValue("DeviceID",szLogicDeviceID,255);
		m_pDbOp->GetFieldValue("DeviceName",szChannelName,255);
		m_pDbOp->GetFieldValue("PhyDeviceChannelMark",szMapChannel,255);
		m_pDbOp->GetFieldValue("IPAddress",szChannelIP,255);
        m_pDbOp->GetFieldValue("Port",iChannelPort);
		m_pDbOp->GetFieldValue("Enable",iEnable);
#endif
		m_pDbOp->GetFieldValue("StreamType",iStreamType);
		m_pDbOp->GetFieldValue("NeedCodec",iNeedCodec);
	//	m_pDbOp->GetFieldValue("CamType",iCamType);
		
		stChConfig.iID=iID;
		stChConfig.iDeviceIndex = iDeviceIndex;
		stChConfig.strLogicDeviceID = szLogicDeviceID;
		stChConfig.strChannelName = szChannelName;
		stChConfig.strMapChannel = szMapChannel;
		stChConfig.iStreamType = iStreamType;
		stChConfig.iNeedCodec  = iNeedCodec;
		stChConfig.iCamType = iCamType;
		stChConfig.strChannelIp = szChannelIP;
		stChConfig.iChannelPort = iChannelPort;
		
#ifdef WIN32
		m_mapChConfig[stChConfig.strLogicDeviceID] = stChConfig;
#else
		if((1 == iEnable) && (stChConfig.strLogicDeviceID != "00000000000000000000"))
		{
			m_mapChConfig[stChConfig.strLogicDeviceID] = stChConfig;
		}
#endif
// 		mgwlog("Channelinfo Index:%d  ChannelName:%s ID%d iNeedCodec:%d\n",
// 		stChConfig.iDeviceIndex,stChConfig.strChannelName.c_str(),stChConfig.iID,stChConfig.iNeedCodec);
		if(m_pDbOp->DBMoveNext()!=0)  //失败退出
		{
			mgwlog("逻辑通道表MoveNext fail\n");
			break;
		}
	}
	mgwlog("ChConfig 记录数:%d\n",m_mapChConfig.size());
	return TRUE;
}

//更新数据
BOOL CSysCfg::UpdataChCfg(const UNGB_CHCONFIGREC stChCfg)
{
	CAutoLock lock(&CSysCfg::m_mapdbLock);
	//更新内存
	map<string,UNGB_CHCONFIGREC>::iterator iter=m_mapChConfig.find(stChCfg.strLogicDeviceID);
	if(iter == m_mapChConfig.end())
	{
		mgwlog("CSysCfg::UpdataChInfo 没有找到指定通道配置\n");  //没有找到指定设备
	}else{
		m_mapChConfig[stChCfg.strLogicDeviceID]=stChCfg;
	}
	//更新数据库
	char szSql[MAX_SQL_LEN]={0};
	sprintf(szSql,"update UnGBPhyDeviceChannelConfig set DeviceIndex = %d,StreamType =%d where ID=%d",
		stChCfg.iDeviceIndex,stChCfg.iStreamType,stChCfg.iID);
	TRACE("sql:%s\n",szSql);
	int nRet =0;
	if(!m_pDbOp) 
	{
		m_pDbOp = new CDbOp();
		nRet = m_pDbOp->DBOpen(NULL,0,NULL);
		if(nRet <0)
		{
			TRACE("pDbOp->DBOpen fail\n");
			MEMORY_DELETE(m_pDbOp);
			return FALSE;
		}
	}
	if(m_pDbOp->DBExec(szSql)>=0)
	{
		//mgwlog("CSysCfg::UpdataChInfo 更新成功\n");
	}else{
		mgwlog("CSysCfg::UpdataChInfo 更新失败\n");
	}
	return TRUE;
} 
