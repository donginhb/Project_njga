// ConfigOp.h: interface for the CConfigOp class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONFIGOP_H__4EA8D057_CEF1_48CC_88BF_408E92060095__INCLUDED_)
#define AFX_CONFIGOP_H__4EA8D057_CEF1_48CC_88BF_408E92060095__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MgwPublicData.h"
#ifdef WIN32
#include "Afxmt.h"
#include "../public/Function/AutoLock.h"
#else
//预置目录 OnvifProxy project path
#include "AutoLock/CAutoLock.h"
#endif
#include "DbOp.h"
#include <iostream>
#include <string>
#include <map>
using namespace std;

//系统信息模块
//Singleton
class CSysCfg  
{
public:
	static CSysCfg * Instance();
	virtual ~CSysCfg();
	int       Init();
    BOOL      Fini();
	DWORD     GetdwPara(SYS_CFG_TYPE eType);
    string    GetstrPara(SYS_CFG_TYPE eType);
    BOOL      QueryBaseCfg();  //查询基本配置信息
	BOOL      QueryDevChCfg();   //查询设备信息
	BOOL      UpdataChCfg(const UNGB_CHCONFIGREC stChCfg); //更新数据
    BOOL      GetDevCfg(map<int,UNGB_DEVCONFIGREC> &mapDevConfig);
	BOOL      GetChCfg(map<string,UNGB_CHCONFIGREC> &mapChConfig);
	BOOL      GetOneDevCfg(int iID,UNGB_DEVCONFIGREC &stDevCfg);
	BOOL      GetOneChCfg(string strDeviceID,UNGB_CHCONFIGREC &stChCfg);
    
public:
    static BOOL       m_bDevInited;   //设备是否初始化
	
private:
	
	//内存数据库操作锁  数据库在内存中只保留一份 保证数据库和设备树的同步
	//增加设备时同时操作设备树 内存数据库 数据库。
#ifdef WIN32
    static CCritSec   m_mapdbLock;
#else
    static pthread_mutex_t	m_mapdbLock;
#endif
	//设备信息 通道信息 对应设备表 通道表
	map<int,UNGB_DEVCONFIGREC>    m_mapDevConfig;        //<ID,UNGB_DEVCONFIGREC>
	map<string,UNGB_CHCONFIGREC>  m_mapChConfig;         //<LogicDeviceID,UNGB_CHCONFIGREC>
	   
	int       m_iRecID;           //记录ID
	string    m_strCmsIP;
	string    m_strServerID;
	DWORD     m_dwServerSipPort;
    string    m_strServerSipPort;
	string    m_strSipRefreshTime;   //sip刷新时间
	string    m_strDbIP;
	string    m_strDbPath;
	string    m_strDbPort;
	DWORD     m_dwDbPort;
	string    m_strLocalIP_in;
	string    m_strLocalIP_out;
	DWORD     m_dwLocalIP;
	string    m_strMgwDevID;   //设备ID 
	string    m_strUserName;
	string    m_strUserPwd;
	int       m_iConSendPackLen;    //每次发送包数

	CDbOp     *m_pDbOp;

private:
	CSysCfg();
#ifdef WIN32
	static CMutex mutex;
#else
	static pthread_mutex_t mutex;
#endif
	static CSysCfg * instance;
};

#endif // !defined(AFX_CONFIGOP_H__4EA8D057_CEF1_48CC_88BF_408E92060095__INCLUDED_)
