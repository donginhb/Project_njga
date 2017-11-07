// DBOperMysql.cpp: implementation of the DBOperMysql class.
//
//////////////////////////////////////////////////////////////////////

#include "DBOperMysql.h"
#ifndef WIN32
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define MAX_FIELD_DATA_LEN 200

DBOperMysql::DBOperMysql()
{
    memset(m_sql_str,0,4048);
    m_strHost = "";
    m_strUser = "";
    m_strPwd = "";
    m_strDBName = "";
    m_iPort = 0;
    m_iTimeOut = 0;
    m_pdb = NULL;
    m_pdb = new CppMySQL3DB();
    int tmpnum = 0;
    while((NULL == m_pdb)&&(tmpnum < 100))
    {
        m_pdb = new CppMySQL3DB();
    }
}

DBOperMysql::~DBOperMysql()
{
    Close();    // 释放数据库连接
}

void DBOperMysql::StrSplit(const string &str,const char delimter,vector<string> &strList)
{
    int pos, bpos=0;
    while((pos=str.find(delimter, bpos)) != string::npos)
        {
        string strpara = str.substr(bpos, pos-bpos);
        strList.push_back(strpara);
        bpos = pos+1;
        }

    if(bpos < str.size())
        strList.push_back(str.substr(bpos, str.size()-bpos));
}

int DBOperMysql::Connect(const char* szConPara, const char* dbPath, int iTimeOut)
{
    if(strlen(szConPara) <= CharNum && strlen(dbPath) <= CharNum)
    {
        memcpy(m_pciceConPara, szConPara, strlen(szConPara));
        memcpy(m_pcdbPath, dbPath, strlen(dbPath));
    }
    //解析字符串
    string strConPara = szConPara;
    vector<string> vctConPara;
    StrSplit(strConPara,';',vctConPara);
    if(vctConPara.size()<5)
    {
        printf("参数错误\n");
        return DB_ERR_OTHER_PARA; //参数错误
    }
    m_strHost = vctConPara[0];
    m_strUser = vctConPara[1];
    m_strPwd = vctConPara[2];
    m_strDBName = vctConPara[3];
    m_iPort = atoi(vctConPara[4].c_str());
    m_iTimeOut = iTimeOut;
    if(m_pdb)
    {
        #ifndef WIN32
        m_pdb->m_host.clear();
        m_pdb->m_host = m_strHost;
        m_pdb->m_user.clear();
        m_pdb->m_user = m_strUser;
        m_pdb->m_dbname.clear();
        m_pdb->m_dbname = m_strDBName;
        m_pdb->m_password.clear();
        m_pdb->m_password = m_strPwd;
        m_pdb->m_port = m_iPort;
        #else
        m_pdb->m_host.empty();
        m_pdb->m_host = m_strHost;
        m_pdb->m_user.empty();
        m_pdb->m_user = m_strUser;
        m_pdb->m_dbname.empty();
        m_pdb->m_dbname = m_strDBName;
        m_pdb->m_password.empty();
        m_pdb->m_password = m_strPwd;
        m_pdb->m_port = m_iPort;
        #endif
    }


    
    cout << "DB para: " << m_strHost << ", " << m_strUser << ", " << m_strPwd << ", " << m_strDBName << endl;
    bool bRet = ReConnect();
    if(bRet)
    {
        string sqlset;
        #ifndef WIN32
        sqlset.clear();
        sqlset = "SET character_set_client='gbk'";
        DBExecute(sqlset.c_str());
        sqlset.clear();
        sqlset = "SET character_set_connection='gbk'";
        DBExecute(sqlset.c_str());
        sqlset.clear();
        sqlset = "SET character_set_results='gbk'";
        DBExecute(sqlset.c_str());
        sqlset.clear();
        sqlset = "SET interactive_timeout=604800";
        DBExecute(sqlset.c_str());
        sqlset.clear();
        sqlset = "SET wait_timeout=604800";
        DBExecute(sqlset.c_str());
        sqlset.clear();
        sqlset = "SET GLOBAL max_connections=100000";
        DBExecute(sqlset.c_str());
        #else
        sqlset.empty();
        sqlset = "SET character_set_client='gbk'";
        DBExecute(sqlset.c_str());
        sqlset.empty();
        sqlset = "SET character_set_connection='gbk'";
        DBExecute(sqlset.c_str());
        sqlset.empty();
        sqlset = "SET character_set_results='gbk'";
        DBExecute(sqlset.c_str());

        #endif
    
        return DB_ERR_OK;
      }
      else
      {
        if(m_pdb)
        {
            m_tLastDbErrorInfo.strErrorMsg = m_pdb->strErrorMsg; 
        }
        return DB_ERR_MYSQL_OTHER;
     }
}

//重新连接
bool DBOperMysql::ReConnect()
{
    if(m_pdb)
    {
        try
        {
            m_pdb->close();
            m_pdb->_db_ptr = NULL;
        }
        catch(...)
        {
           m_pdb->_db_ptr = NULL;
        }
        
        printf("\r\n EV9000DB m_strHost = %s,m_strUser = %s m_strPwd = %s m_strDBName = %s m_iPort = %d \r\n",m_strHost.c_str(),m_strUser.c_str(),m_strPwd.c_str(),m_strDBName.c_str(),m_iPort);
        
        if(0 == m_pdb->open(m_strHost.c_str(),m_strUser.c_str(),m_strPwd.c_str(),m_strDBName.c_str(),m_iPort,0))
        {
            string sqlset;
            #ifndef WIN32
            sqlset.clear();
            sqlset = "SET character_set_client='gbk'";
            DBExecute(sqlset.c_str());
            sqlset.clear();
            sqlset = "SET character_set_connection='gbk'";
            DBExecute(sqlset.c_str());
            sqlset.clear();
            sqlset = "SET character_set_results='gbk'";
            DBExecute(sqlset.c_str());
            sqlset.clear();
            sqlset = "SET interactive_timeout=604800";
            DBExecute(sqlset.c_str());
            sqlset.clear();
            sqlset = "SET wait_timeout=604800";
            DBExecute(sqlset.c_str());
            sqlset.clear();
            sqlset = "SET GLOBAL max_connections=100000";
            DBExecute(sqlset.c_str());
            #else
            sqlset.empty();
            sqlset = "SET character_set_client='gbk'";
            DBExecute(sqlset.c_str());
            sqlset.empty();
            sqlset = "SET character_set_connection='gbk'";
            DBExecute(sqlset.c_str());
            sqlset.empty();
            sqlset = "SET character_set_results='gbk'";
            DBExecute(sqlset.c_str());

            #endif
            return true;
        }
        else
        {
            return false;
        }
    }

    return false;
}

// 关闭连接
void DBOperMysql::Close()
{
    if(m_pdb)
    {
        delete m_pdb;
    }
    m_pdb = NULL;
    return;
}

// execute database sql sentence
int DBOperMysql::DBExecute(const char * sql)
{
    // 参数合法性校验
    if (NULL == sql)
    {
        printf("DBOper::DBExecute: invalid parameter! sql is null!\n" );
        m_tLastDbErrorInfo.strErrorMsg = "sql为空";
        return DB_ERR_OTHER_PARA;
    }

    if(m_pdb)
    {
        string sqlset;
        #ifndef WIN32
        
        #else
        sqlset.empty();
        sqlset = "SET character_set_client='gbk'";
        m_pdb->execSQL(sqlset.c_str());
        sqlset.empty();
        sqlset = "SET character_set_connection='gbk'";
        m_pdb->execSQL(sqlset.c_str());
        sqlset.empty();
        sqlset = "SET character_set_results='gbk'";
        m_pdb->execSQL(sqlset.c_str());
        #endif
        

        
        memset(m_sql_str,0,4048);
        memcpy(m_sql_str,sql,strlen(sql));
        strcat(m_sql_str,";");
        int nRet = m_pdb->execSQL(m_sql_str);
        
        //if(strncmp(sql+strlen(sql)-1,";",1)!=0)
        //{
        //    strcat(sql,";");
        //}
        
        //int nRet = m_pdb->execSQL(sql);
        m_iLastOpErrorCode = nRet;
        if(nRet!=0)
        {
            m_tLastDbErrorInfo.strErrorMsg = m_pdb->strErrorMsg;
        }
        return nRet;
    }
    return -1;
}

/*
 * //数据库表 查询
 *返回 查询结果条数
 * const char * sql sql查询语句
 */
int DBOperMysql::DBQuery(const char * sql)
{
    // 参数合法性校验
    if (NULL == sql)
    {
        printf("DBOper::DBExecute: invalid parameter! sql is null!\n" );
        m_tLastDbErrorInfo.strErrorMsg = "sql为空";
        return DB_ERR_OTHER_PARA;
    }
    try
    {
        if(m_pdb)
        {
        string sqlset;
        #ifndef WIN32
        
        #else
        sqlset.empty();
        sqlset = "SET character_set_client='gbk'";
        m_pdb->execSQL(sqlset.c_str());
        sqlset.empty();
        sqlset = "SET character_set_connection='gbk'";
        m_pdb->execSQL(sqlset.c_str());
        sqlset.empty();
        sqlset = "SET character_set_results='gbk'";
        m_pdb->execSQL(sqlset.c_str());
        #endif
        
        #ifndef WIN32
        FreeResult();
        #endif
        int flag = 0;
        m_oQueryRes = m_pdb->querySQL(sql,flag);

        if(-1000 == flag)
        {
            return -1000;
        }

        return DB_ERR_OK;
        }
    }
    catch (...)
    {
        return -1;	
    }
    return 0;
}

int DBOperMysql::MoveNext()
{
    m_oQueryRes.nextRow();
    if (m_oQueryRes.eof())
    {
        return DB_ERR_OTHER_MOVENEXTFAIL;
    }

    return DB_ERR_OK;
}

// start transaction
int DBOperMysql::BeginTransaction()
{
    if (m_pdb)
    {
        WBOOL bRet = ( 0==m_pdb->startTransaction());
        if(!bRet)
        {
            m_tLastDbErrorInfo.strErrorMsg = m_pdb->strErrorMsg;
            return DB_ERR_MYSQL_TRANSFAIL;
        }
        else
        {
            return DB_ERR_OK;
        } 
    }
    return DB_ERR_OTHER_UNKNOWN;
}

// close transaction
int DBOperMysql::Commit()
{
    if (m_pdb)
    {
        WBOOL bRet = ( 0==m_pdb->commit());
        if(!bRet)
        {
            m_tLastDbErrorInfo.strErrorMsg = m_pdb->strErrorMsg;
            return DB_ERR_MYSQL_COMMITFAIL;
        }
        else
        {
        return DB_ERR_OK;
        } 
    }
    return DB_ERR_OTHER_UNKNOWN;
}
// roll back
int DBOperMysql::RollBack()
{
    if (m_pdb)
    {
        WBOOL bRet = ( 0==m_pdb->rollback());
        if(!bRet)
        {
            m_tLastDbErrorInfo.strErrorMsg = m_pdb->strErrorMsg;
            return DB_ERR_MYSQL_ROLLBACKFAIL;
        }
        else
        {
            return DB_ERR_OK;
        } 
    }
    return DB_ERR_OTHER_UNKNOWN;
}

// 释放查询的结果集
void DBOperMysql::FreeResult()
{
    m_oQueryRes.finalize();
    return;
}

// 获取DBExecute执行时受影响行数
int DBOperMysql::GetAffectedRows()
{
    return m_iAffectedRows;
}



//unsigned int
int DBOperMysql::GetFieldValue(const char* fd_name, unsigned int& nValue)
{
    int len = 0;
    int iRet   = 0;


    iRet = DB_ERR_OTHER_GETVALUEFAIL;
    if (m_pdb)
    {
        nValue = m_oQueryRes.getunsignedField(fd_name,0);
        iRet = DB_ERR_OK;
    }
    return iRet;
}

int DBOperMysql::GetFieldValue(int fd_num, unsigned int& nValue)
{
    int len = 0;
    int iRet   = 0;

    iRet = DB_ERR_OTHER_GETVALUEFAIL;
    if (m_pdb)
    {
        nValue = m_oQueryRes.getunsignedField(fd_num,0);
        iRet = DB_ERR_OK;
    }
    return iRet;
}

// 获取整型的值
//int
int DBOperMysql::GetFieldValue(const char* fd_name, int& nValue)
{
    int len = 0;
    int iRet   = 0;


    iRet = DB_ERR_OTHER_GETVALUEFAIL;
    if (m_pdb)
    {
        nValue = m_oQueryRes.getIntField(fd_name,0);
        iRet = DB_ERR_OK;
    }
    return iRet;
}

int DBOperMysql::GetFieldValue(int fd_num, int& nValue)
{
    int len = 0;
    int iRet   = 0;

    iRet = DB_ERR_OTHER_GETVALUEFAIL;
    if (m_pdb)
    {
        nValue = m_oQueryRes.getIntField(fd_num,0);
        iRet = DB_ERR_OK;
    }
    return iRet;
}

//float
int DBOperMysql::GetFieldValue(const char* fd_name, float& fValue)
{
    int len = 0;
    int iRet   = 0;

    iRet = DB_ERR_OTHER_GETVALUEFAIL;
    if (m_pdb)
    {
        fValue = m_oQueryRes.getFloatField(fd_name,0);
        iRet = DB_ERR_OK;
    }
    return iRet;
}

int  DBOperMysql::GetFieldValue(int fd_num, float& fValue)
{
    int len = 0;
    int iRet   = 0;

    iRet = DB_ERR_OTHER_GETVALUEFAIL;
    if (m_pdb)
    {
        fValue = m_oQueryRes.getFloatField(fd_num,0);
        iRet = DB_ERR_OK;
    }
    return iRet;
}

//string
int DBOperMysql::GetFieldValue(const char* fd_name, string& strValue)
{
    int len = 0;
    int iRet   = 0;

    iRet = DB_ERR_OTHER_GETVALUEFAIL;
    if (m_pdb)
    {
        const char * tmpstr = m_oQueryRes.getStringField(fd_name,"");
        if(NULL == tmpstr)
        {
            iRet = DB_ERR_OTHER_STRISNULL;
            strValue = "";
            return iRet;        
        }   
        strValue = tmpstr;
        #ifndef WIN32
        if(strValue == "MysqlDB_cou_not_fount")
        {
            iRet = DB_ERR_OHTER_FILENAMENOT;
            return iRet;
        }
        #endif
        //strValue = m_oQueryRes.getStringField(fd_name,"");
        iRet = DB_ERR_OK;
    }
    return iRet;
}

int DBOperMysql::GetFieldValue(int fd_num, string& strValue)
{
    int len = 0;
    int iRet   = 0;


    iRet = DB_ERR_OTHER_GETVALUEFAIL;
    if (m_pdb)
    {
        const char * tmpstr = m_oQueryRes.getStringField(fd_num,"");
        if(NULL == tmpstr)
        {
            iRet = DB_ERR_OTHER_STRISNULL;
            strValue = "";
            return iRet;        
        }   
        strValue = tmpstr;

        //strValue = m_oQueryRes.getStringField(fd_num,"");
        iRet = DB_ERR_OK;
    }
    return iRet;
}

//char* with length
int DBOperMysql::GetFieldValue(const char* fd_name, char* pBuf, int nSize, unsigned* length)
{
    int len = 0;
    int iRet   = 0;

    iRet = DB_ERR_OTHER_GETVALUEFAIL;
    if (m_pdb)
    {
        const char * tmpstr = m_oQueryRes.getStringField(fd_name,"");
        if(NULL == tmpstr)
        {
            iRet = DB_ERR_OTHER_STRISNULL;
            return iRet;        
        }   
        string strValue = tmpstr;
        #ifndef WIN32
        if(strValue == "MysqlDB_cou_not_fount")
        {
            iRet = DB_ERR_OHTER_FILENAMENOT;
            return iRet;
        }
        #endif
        //string strValue = m_oQueryRes.getStringField(fd_name,"");
        int len = strValue.length();
        if(len <= nSize-1)
        {
            memcpy(pBuf,strValue.c_str(),len);
            *length = len;
            iRet = DB_ERR_OK;
        }
    }
    return iRet;
}

int DBOperMysql::GetFieldValue(int fd_num, char* pBuf, int nSize, unsigned* length)
{
    int len = 0;
    int iRet   = 0;

    iRet = DB_ERR_OTHER_GETVALUEFAIL;
    if (m_pdb)
    {
        const char * tmpstr = m_oQueryRes.getStringField(fd_num,"");
        if(NULL == tmpstr)
        {
            iRet = DB_ERR_OTHER_STRISNULL;
            return iRet;        
        }   
        string strValue = tmpstr;
        //string strValue = m_oQueryRes.getStringField(fd_num,"");
        int len = strValue.length();
        if(len <= nSize-1)
        {
            memcpy(pBuf,strValue.c_str(),len);
            *length = len;
            iRet = DB_ERR_OK;
        }
    }
    return iRet;
}

//char*
int DBOperMysql::GetFieldValue(const char* fd_name, char* pBuf, int nSize)
{
    unsigned  length;
    return GetFieldValue(fd_name,pBuf,nSize, &length);
}

int DBOperMysql::GetFieldValue(int fd_num, char* pBuf, int nSize)
{
    unsigned  length;
    return GetFieldValue(fd_num,pBuf,nSize, &length);
}

//bin
//unsigned long* ulLen = mysql_fetch_lengths(res); 
int DBOperMysql::GetFieldValue(const char* fd_name, BYTE* pBuf, int nSize, unsigned* length)
{
    return -1;
    return DB_ERR_OK;
}

int DBOperMysql::GetFieldValue(int fd_num, BYTE* pBuf, int nSize, unsigned* length)
{
    return -1;
    return DB_ERR_OK;
}

CppMySQLQuery& DBOperMysql::getMySQLQueryRes()
{
    return m_oQueryRes;
}


