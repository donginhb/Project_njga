// DbOp.cpp: implementation of the CDbOp class.
//
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include "stdafx.h"
#include "EV9000Mgw.h"
#endif
#include "DbOp.h"
#include <iostream>
#include <string>
#include "MgwPublicData.h"
using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifdef WIN32
extern CEV9000MgwApp theApp;
#endif
CDbOp::CDbOp()
{
#ifndef WIN32
	m_pDb = NULL;
	m_bCon = false;
#endif
	m_nDbHandle =-1;
}

CDbOp::~CDbOp()
{
    DBClose();
}

int  CDbOp::DBOpen(char* szIP,int nPort,char* szDbPath)
{
#ifdef WIN32
	//sqlite数据库	
	try
	{
		char strModule[1024] = {0};
		GetModuleFileName(NULL,strModule, 1024-1); //得到当前模块路径
		CString strAppPath = strModule;
		strAppPath +="\\..\\ev9000mgw.db";
		myDB.open(strAppPath.GetBuffer(0));
		//myDB.open(INNERDB) ;
		if (!myDB.tableExists("MgwLoginConfig"))
		{
			//AfxMessageBox("没有找到MgwLoginConfig表");
			myDB.execDML("CREATE TABLE [MgwLoginConfig] ("
				"[ID] INTEGER  NOT NULL PRIMARY KEY,"
				"[CmsIP] VARCHAR(100)  NULL,"
				"[ServerID] VARCHAR(100)  NULL,"
				"[ServerSipPort] VARCHAR(100)  NULL,"
				"[SipRefreshTime] VARCHAR(100)  NULL,"
				"[DbIP] VARCHAR(100)  NULL,"
				"[DbPath] VARCHAR(100)  NULL,"
				"[DbPort] VARCHAR(100)  NULL,"
				"[LocalIP_in] VARCHAR(100)  NULL,"
                "[LocalIP_out] VARCHAR(100)  NULL,"
				"[MgwDevID] VARCHAR(100)  NULL,"
				"[UserName] VARCHAR(100)  NULL,"
				"[UserPwd] VARCHAR(100)  NULL,"
				"[Resved1] INTEGER  NULL, "
                "[Resved2] VARCHAR(100)  NULL);");
			//myDB.close();
			//return -1;
		}

		if (!myDB.tableExists("UnGBPhyDeviceConfig"))
		{
			//AfxMessageBox("没有找到UnGBPhyDeviceConfig表");
			myDB.execDML("CREATE TABLE [UnGBPhyDeviceConfig] ("
			    "[ID] INTEGER  NOT NULL PRIMARY KEY,"
				"[DeviceName] VARCHAR(36)  NULL,"
				"[DeviceType] INTEGER  NOT NULL," 
				"[DeviceIP] VARCHAR(20)  NULL, "
				"[DevicePort] INTEGER  NULL, "
				"[UserName] VARCHAR(36)  NULL," 
				"[Password] VARCHAR(36)  NULL, "
				"[RecordType] INTEGER  NULL, "
				"[Resved1] INTEGER  NULL, "
				"[Resved2] VARCHAR(32)  NULL);");
			//myDB.close();
			//return -1;
		}
		
		if (!myDB.tableExists("UnGBPhyDeviceChannelConfig"))
		{
			//AfxMessageBox("没有找到UnGBPhyDeviceChannelConfig表");
			myDB.execDML("CREATE TABLE [UnGBPhyDeviceChannelConfig] ("
				"[ID] INTEGER  NOT NULL PRIMARY KEY," 
				"[DeviceIndex] INTEGER  NOT NULL, "
				"[LogicDeviceID] VARCHAR(24)  NOT NULL," 
				"[ChannelName] VARCHAR(68)  NOT NULL," 
				"[MapChannel] INTEGER  NULL," 
				"[StreamType] INTEGER DEFAULT 2,"
				"[NeedCodec] INTEGER DEFAULT 0,"
				"[Resved1] INTEGER  NULL," 
				"[Resved2] VARCHAR(32)  NULL,"
				"foreign key (DeviceIndex) references UnGBPhyDeviceConfig(ID) on delete cascade on update cascade );");

			//myDB.close();
			//return -1;
		}
		return 0;
	}
	catch (CppSQLite3Exception& e)
	{		
		mgwlog("CDbOp::DBOpen 数据库操作异常:%s\n",e.errorMessage());
		//AfxMessageBox("数据库操作异常");
		myDB.close();
		return -1;
	}
	return 0;
#else
	//mysql
	DBClose();
	if(!m_pDb)
	{
		m_pDb = new DBOperSon();
	}
	if(m_pDb)
	{
		string strConPara="";
		strConPara=strConPara+szIP+";root;wiscom;EV9000DB;3306";
		string strDbPath="";
		int nRet = m_pDb->Connect(strConPara.c_str(),strDbPath.c_str(),10);
		if(0==nRet)
		{
		    m_bCon=true;
		}
		return nRet;
	}
	return -1;
#endif
}

void CDbOp::DBClose()
{
#ifdef WIN32
	//sqlite数据库
    myDB.close();
#else
    //mysql
    if(m_pDb)
    {
    	m_pDb->Close();
		MEMORY_DELETE(m_pDb);
    }
    m_bCon = false;
    return ;
#endif
}

int  CDbOp::DBQuery(char* strSql)
{
#ifdef WIN32
    //sqlite数据库
	try
	{
		myQuery = myDB.execQuery(strSql);
	}
	catch (CppSQLite3Exception& e)
	{		
		mgwlog("CDbOp::DBQuery 数据库操作异常:%s\n",e.errorMessage());
		//AfxMessageBox("数据库操作异常");
		myDB.close();
		return -1;
	}
	return 0;
#else
	//mysql
	if(m_pDb && m_bCon)
	{
		return m_pDb->DBQuery(strSql);
	}else{
		return -1;
	}
#endif
}

int  CDbOp::DBExec(char* strSql)
{
#ifdef WIN32
	//sqlite数据库
	try
	{
		return myDB.execDML(strSql);
	}
	catch (CppSQLite3Exception& e)
	{		
		mgwlog("CDbOp::DBExec 数据库操作异常:%s\n",e.errorMessage());
		//AfxMessageBox("数据库操作异常");
		myDB.close();
		return -1;
	}
	return 0;
#else
	//mysql
	if(m_pDb && m_bCon)
	{
		return m_pDb->DBExecute(strSql);
	}else{
		return -1;
	}
#endif
}

int  CDbOp::DBGetTotalNum()
{
#ifdef WIN32
	mgwlog("不支持此接口CDbOp::DBGetTotalNum()\n");
	return 0;
#else
   	if(m_pDb && m_bCon)
	{
		return m_pDb->GetTotalNum();
	}else{
		return -1;
	}
#endif
}

bool CDbOp::DBEof()
{
#ifdef WIN32
	//sqlite数据库
	return myQuery.eof();
#else
	//mysql
	mgwlog("不支持此接口DBEof()\n");
	return false;
#endif
}

int  CDbOp::DBMoveNext()
{
#ifdef WIN32
	//sqlite数据库
	if(myQuery.eof()==true)
	{
		return -1;
	}else{
        myQuery.nextRow();
		if(myQuery.eof()==true)
		{
			return -1;
		}else{
			return 0;
		}
	}
#else
	//mysql
	if(m_pDb && m_bCon)
	{
		return m_pDb->MoveNext();
	}else{
		return -1;
	}
#endif
}

//int
int  CDbOp::GetFieldValue(const char * fd_name, int &nValue)
{
#ifdef WIN32
	try
	{
		//sqlite数据库
		nValue = myQuery.getIntField(fd_name);
	}
	catch (CppSQLite3Exception& e)
	{	
		mgwlog("CDbOp::GetFieldValue :%s 数据库操作异常:%s\n",fd_name,e.errorMessage());
		//AfxMessageBox("数据库操作异常");
		nValue =0;
		myDB.close();
		return -1;	
	}
	return 0;
#else
	//mysql
	if(m_pDb && m_bCon)
	{
		return m_pDb->GetFieldValue(fd_name,nValue);
	}else{
		return -1;
	}
#endif
}

//char*
int  CDbOp::GetFieldValue( const char * fd_name,char* pszValue,int nSize)
{
#ifdef WIN32
	try
	{
		//sqlite数据库
		string strValue ="";
		const char* szValue = NULL;
		szValue = myQuery.getStringField(fd_name);
		if(szValue !=NULL)
		{
			strValue = szValue;
		}
	    memcpy(pszValue,strValue.c_str(),strValue.length());
	}
	catch (CppSQLite3Exception& e)
	{	
		mgwlog("CDbOp::GetFieldValue :%s 数据库操作异常:%s\n",fd_name,e.errorMessage());
		//AfxMessageBox("数据库操作异常");
		pszValue[0]='\0';
		myDB.close();
		return -1;	
	}
	return 0;
#else
	//mysql
	if(m_pDb && m_bCon)
	{
		return m_pDb->GetFieldValue(fd_name,pszValue,nSize);
	}else{
		return -1;
	}
#endif
}
