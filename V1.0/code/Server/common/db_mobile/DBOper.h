
/////////////////////////////begin/////////////////////////

#ifndef  __DBOPER_H__
#define  __DBOPER_H__

#ifdef WIN32
	#include "StdAfx.h"
#else

#endif
//#define USE_BDB      //bdb数据库
#include "DBOperMysql.h"



//extern void osip_usleep(int useconds);


class DBOperSon
{
public:
    DBOperBase* m_pDb;    //数据库操作类
   
public:
    string m_iceConPara;
    string m_dbPath;
    int GetmyDBTableExit();
    void SetmyDBTableExit();
    int GetTotalNum()
    {
        if(m_pDb)
        {
            return m_pDb->GetTotalNum();
        }
        return 0;
    }

    DBOperSon();
    ~DBOperSon();
    //static DBOper* DBinstance();

    bool Ice_Ping();
    void SetTimeOut(int iIceTimeOut)
    {
        if(m_pDb)
        {
            m_pDb->SetTimeOut(iIceTimeOut);
        }
        return ;
    }

    int  Connect(const char* iceConPara, const char* dbPath, int iIceTimeOut = 3); //带超时设置，连接Ice::databus数据库服务器
    int  DBExecute(const char* sql);    // 执行操作 int DBExecute(const char * sql,int &nExecRet);
    int  DBQuery(const char* sql);  // 执行查询语句
    int  DBQueryNext(); // 执行查询下一批
    void FreeResult(); // 释放查询的结果集
    void Close(); // 关闭连接
    int  MoveNext();  //下移一条记录

    //----事务操作接口--begin------
    int  BeginTransaction();  //开启事务
    int  Commit();            //提交事务
    int  RollBack();          //回滚事务
    //----事务操作接口--end------

    //CppMySQLQuery m_oQueryRes;            //查询结果类

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

    //操作错误信息
    int  GetLastOpErrorCode();
    const char* GetLastOpErrorMsg();

    //数据库错误信息
    int  GetLastDbErrorCode();
    const char* GetLastDbErrorMsg();

    // ***BEGIN***  增加数据库异常保护机制 wangqichao 2013/6/17 add
    bool GetDBStatus()
    {
        if(m_pDb)
        {
            return m_pDb->GetDBStatus();
        }
        return false;
    }
    int DB_Abnormity_Manage_Select(const char* sql);
    int DB_Abnormity_Manage_Insert(const char* sql_Select, const char* sql_Update, const char* sql_Insert);
    int DB_Abnormity_Manage_Update(const char* sql);
    int DB_Abnormity_Manage_Delete(const char* sql);

    int DB_Select(const char* sql, int Flag = 1);
    int DB_Insert(const char* sql_Select, const char* sql_Update, const char* sql_Insert, int Flag = 1);
    int DB_Update(const char* sql, int Flag = 1);
    int DB_Delete(const char* sql, int Flag = 1);
    char* safeCopyStr(char* dst, const char* src, int maxlen);
    int DB_WSelect(const char* sql, CppMySQLQuery& m_oQueryRes, int Flag = 1);
    // ***END***  增加数据库异常保护机制 wangqichao 2013/6/17 add

};


class DBOper
{
public:
    DBOperSon *pDBOperSon_EV9000DB;
    DBOperSon *pDBOperSon_MyDB;
    string m_MyTable;
public:
    DBOper();
    ~DBOper();
    
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

    int  Connect(char iceConPara[2][100], const char* dbPath); 
    //int  Connect(const char* iceConPara, const char* dbPath); 

    //操作错误信息
    int  GetLastOpErrorCode();
    const char* GetLastOpErrorMsg();

    //数据库错误信息
    int  GetLastDbErrorCode();
    const char* GetLastDbErrorMsg();

    void Close(); 

    void FreeResult()
    {
        if(pDBOperSon_EV9000DB)
        {
            pDBOperSon_EV9000DB->FreeResult();
        }
        return;
    }

    int GetmyDBTableExit()
    {
        return pDBOperSon_EV9000DB->GetmyDBTableExit();
    }

    void SetmyDBTableExit()
    {
        pDBOperSon_EV9000DB->SetmyDBTableExit();
    }

    int  MoveNext()
    {
        if(pDBOperSon_EV9000DB)
        {
            return pDBOperSon_EV9000DB->MoveNext();
        }
        return DB_ERR_OTHER_UNKNOWN;
    }


    int DB_Select(const char* sql, int Flag = 1, int Level=0);
    int DB_Insert(const char* sql_Select, const char* sql_Update, const char* sql_Insert, int Flag = 1, int Level=0);
    int DB_Update(const char* sql, int Flag = 1, int Level=0);
    int DB_Delete(const char* sql, int Flag = 1, int Level=0); 
    int DBExecute(const char* sql, int Flag = 1);

};


#ifndef WIN32


//异步处理线程
class DB_Thread
{
public:
    DB_Thread(char StrCon[4][100], const char* StrDb)
    {
        m_pEV9000DBOper = NULL;  
        m_pEV9000DBOper = new DBOperSon();
        while(NULL == m_pEV9000DBOper)
        {
            m_pEV9000DBOper = new DBOperSon();
        }
        m_pEV9000DBOper->Connect(StrCon[0],StrDb,3);

        m_pEV9000TSUOper = NULL;  
        m_pEV9000TSUOper = new DBOperSon();
        while(NULL == m_pEV9000TSUOper)
        {
            m_pEV9000TSUOper = new DBOperSon();
        }
        m_pEV9000TSUOper->Connect(StrCon[1],StrDb,3);

        m_pEV9000LOGOper = NULL;  
        m_pEV9000LOGOper = new DBOperSon();
        while(NULL == m_pEV9000LOGOper)
        {
            m_pEV9000LOGOper = new DBOperSon();
        }
        m_pEV9000LOGOper->Connect(StrCon[2],StrDb,3);

        m_pEV9000MyOper = NULL;  
        m_pEV9000MyOper = new DBOperSon();
        while(NULL == m_pEV9000MyOper)
        {
            m_pEV9000MyOper = new DBOperSon();
        }
        m_pEV9000MyOper->Connect(StrCon[3],StrDb,3);
        
        Thread_exit = 0;
        //启动线程
        (void)pthread_create(&m_DBthreadId, NULL, Thread_run, (void*)this);
    }
    ~DB_Thread()
    {
        m_pEV9000DBOper->Close();
        m_pEV9000TSUOper->Close();
        m_pEV9000LOGOper->Close();
        m_pEV9000MyOper->Close();
        Thread_exit = 1;
    }
    static void * Thread_run(void* p);
    //void SqlWrite(string sqltest);
    void SqlRead();
    DBOperSon *m_pEV9000DBOper;
    DBOperSon *m_pEV9000TSUOper;
    DBOperSon *m_pEV9000LOGOper;
    DBOperSon *m_pEV9000MyOper;
    
    int Thread_exit;
    pthread_t m_DBthreadId;
};
#endif

#endif   /* #ifndef  __DBOPER_H__  */

