// DbOp.h: interface for the CDbOp class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DBOP_H__73298B8E_23FD_4313_868E_B487817FE6D7__INCLUDED_)
#define AFX_DBOP_H__73298B8E_23FD_4313_868E_B487817FE6D7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#include "../public/Include/IEV9000DBOp.h"
//#pragma comment(lib,"../lib/EV9000DBOp.lib")

#ifdef WIN32
#include "CppSQLite3.h"
#pragma comment(lib,"./lib/sqlite3.lib")
#else
#include "../../common/db/DBOper.h"
#include "Log/CLog.h"
#endif

//数据库模块
class CDbOp  
{
public:
	CDbOp();
	virtual ~CDbOp();
	
	int  DBOpen(char* szIP,int nPort,char* szDbPath);
	void DBClose();
	int  DBQuery(char* strSql);
	int  DBExec(char* strSql);
	int  DBGetTotalNum();
	bool DBEof();     //判断是否结束
    int  DBMoveNext();
	int  GetFieldValue(const char * fd_name, int &nValue);	//int
    int  GetFieldValue( const char * fd_name,char* pszValue,int nSize); //char*

//protected:
    int m_nDbHandle;

private:
#ifdef WIN32
	//sqlite
	CppSQLite3DB myDB;
	CppSQLite3Query myQuery;
#else
	DBOperSon* m_pDb;
	bool    m_bCon;  //连接成功标志
#endif
};

#endif // !defined(AFX_DBOP_H__73298B8E_23FD_4313_868E_B487817FE6D7__INCLUDED_)
