// DBOperMysql.h: interface for the DBOperMysql class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DBOPERMYSQL_H__DE8FE54F_0392_4A77_B167_0E92961753EC__INCLUDED_)
#define AFX_DBOPERMYSQL_H__DE8FE54F_0392_4A77_B167_0E92961753EC__INCLUDED_

#ifdef WIN32
    #include "StdAfx.h"
    #pragma comment(lib,"lib/libmysql.lib")
#endif

#include "DBOperBase.h"
#include <iostream>
#include <vector>
#include <list>
#include "cppmysql.h"
using namespace std;

class DBOperMysql :public DBOperBase 
{
public:
    int GetMyDBExit()
    {
        return m_pdb->myDBTableExit;
    }
    void SetMyDBExit()
    {
        m_pdb->myDBTableExit = 0;
    }

    DBOperMysql();
    virtual ~DBOperMysql();

    bool Ice_Ping()
    {
        if(m_pdb)
        {
            //return (0 == m_pdb->ping());
            //if(ReConnect())
            if(0 == m_pdb->ping())
            {
                return true;
            }
            else
            {
                if(!ReConnect())
                {
                    return false;
                }
                return true;
            }
        }
        return false;
    }
    
    int GetTotalNum()
    {
        if(m_pdb)
        {
            m_nTotalNum = m_oQueryRes.numRow();
        }
        
        return m_nTotalNum;
    }

    void SetTimeOut(int iIceTimeOut)
    {
        if(m_pdb)
        {
            m_pdb->setBusyTimeout(iIceTimeOut);
        }
        //m_iIceTimeOut = iIceTimeOut;
        return ;
    }

    int  Connect(const char* szConPara, const char* dbPath, int iTimeOut = 3); //连接MySql服务器
    int  DBExecute(const char* sql);      //执行操作 int DBExecute(const char * sql,int &nExecRet);
    int  DBQuery(const char* sql);  // 执行查询语句
    //int  DBQueryNext(); // 执行查询下一批
    void FreeResult(); // 释放查询的结果集
    void Close(); // 关闭连接
    int  MoveNext();  //下移一条记录

    int  GetAffectedRows();
    //----------------------
    //----事务操作接口--begin------
    int  BeginTransaction();  //开启事务
    int  Commit();            //提交事务
    int  RollBack();          //回滚事务
    //----事务操作接口--end------

    //-------类ADO接口---begin---------
    //  获得指定行和指定列的整形值
    //  行列都从0开始编号
    //  row  ：行号
    //  fd_name ：列名称
    //  fd_num  ：列字段号
    //unsigned int 
    int  GetFieldValue(const char* fd_name, unsigned int& nValue);
    int  GetFieldValue(int fd_num, unsigned int& nValue);
    //int
    int  GetFieldValue(const char* fd_name, int& nValue);
    int  GetFieldValue(int fd_num, int& nValue);

    //float
    int  GetFieldValue(const char* fd_name, float& fValue);
    int  GetFieldValue(int fd_num, float& fValue);

    //string
    int  GetFieldValue(const char* fd_name, string& strValue);
    int  GetFieldValue(int fd_num, string& strValue);

    //char* with length
    int  GetFieldValue(const char* fd_name, char* pBuf, int nSize, unsigned* length);
    int  GetFieldValue(int fd_num, char* pBuf, int nSize, unsigned* length);

    //char*
    int  GetFieldValue(const char* fd_name, char* pBuf, int nSize);
    int  GetFieldValue(int fd_num, char* pBuf, int nSize);

    //bin
    int  GetFieldValue(const char* fd_name, BYTE* pBuf, int nSize, unsigned* length);
    int  GetFieldValue(int fd_num, BYTE* pBuf, int nSize, unsigned* length);
    //-------类ADO接口---end-----------
    CppMySQLQuery& getMySQLQueryRes();

private:
    bool ReConnect();       //重连数据库
    void StrSplit(const string &str,const char delimter, vector<string> &strList);
private:

    int           m_iAffectedRows;    // DBExecute执行时，受影响行数

    //连接参数
    string        m_strHost;
    string        m_strUser;
    string        m_strPwd;
    string        m_strDBName;
    int           m_iPort;
    int           m_iTimeOut;
    int           m_nTotalNum;         //记录总行数
    int           m_nCurPos;           //当前记录位置
    
    CppMySQL3DB   *m_pdb;              //数据库操作类
    CppMySQLQuery m_oQueryRes;            //查询结果类
    //wangqichao add 分号
    char m_sql_str[4048];
};

#endif // !defined(AFX_DBOPERMYSQL_H__DE8FE54F_0392_4A77_B167_0E92961753EC__INCLUDED_)
