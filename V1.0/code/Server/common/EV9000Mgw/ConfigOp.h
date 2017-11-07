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
//Ԥ��Ŀ¼ OnvifProxy project path
#include "AutoLock/CAutoLock.h"
#endif
#include "DbOp.h"
#include <iostream>
#include <string>
#include <map>
using namespace std;

//ϵͳ��Ϣģ��
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
    BOOL      QueryBaseCfg();  //��ѯ����������Ϣ
	BOOL      QueryDevChCfg();   //��ѯ�豸��Ϣ
	BOOL      UpdataChCfg(const UNGB_CHCONFIGREC stChCfg); //��������
    BOOL      GetDevCfg(map<int,UNGB_DEVCONFIGREC> &mapDevConfig);
	BOOL      GetChCfg(map<string,UNGB_CHCONFIGREC> &mapChConfig);
	BOOL      GetOneDevCfg(int iID,UNGB_DEVCONFIGREC &stDevCfg);
	BOOL      GetOneChCfg(string strDeviceID,UNGB_CHCONFIGREC &stChCfg);
    
public:
    static BOOL       m_bDevInited;   //�豸�Ƿ��ʼ��
	
private:
	
	//�ڴ����ݿ������  ���ݿ����ڴ���ֻ����һ�� ��֤���ݿ���豸����ͬ��
	//�����豸ʱͬʱ�����豸�� �ڴ����ݿ� ���ݿ⡣
#ifdef WIN32
    static CCritSec   m_mapdbLock;
#else
    static pthread_mutex_t	m_mapdbLock;
#endif
	//�豸��Ϣ ͨ����Ϣ ��Ӧ�豸�� ͨ����
	map<int,UNGB_DEVCONFIGREC>    m_mapDevConfig;        //<ID,UNGB_DEVCONFIGREC>
	map<string,UNGB_CHCONFIGREC>  m_mapChConfig;         //<LogicDeviceID,UNGB_CHCONFIGREC>
	   
	int       m_iRecID;           //��¼ID
	string    m_strCmsIP;
	string    m_strServerID;
	DWORD     m_dwServerSipPort;
    string    m_strServerSipPort;
	string    m_strSipRefreshTime;   //sipˢ��ʱ��
	string    m_strDbIP;
	string    m_strDbPath;
	string    m_strDbPort;
	DWORD     m_dwDbPort;
	string    m_strLocalIP_in;
	string    m_strLocalIP_out;
	DWORD     m_dwLocalIP;
	string    m_strMgwDevID;   //�豸ID 
	string    m_strUserName;
	string    m_strUserPwd;
	int       m_iConSendPackLen;    //ÿ�η��Ͱ���

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
