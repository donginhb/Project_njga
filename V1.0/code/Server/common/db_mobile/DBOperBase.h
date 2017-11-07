#ifndef  __DBBASE_H__
#define  __DBBASE_H__



//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DBOPERBASE_H__720058DD_AF8E_4251_B537_AFBB5E1CB9EA__INCLUDED_)
#define AFX_DBOPERBASE_H__720058DD_AF8E_4251_B537_AFBB5E1CB9EA__INCLUDED_

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include "DBOperMysql.h"
#include "cppmysql.h"



using namespace std;

typedef unsigned char BYTE;
typedef int           WBOOL;

#ifndef WIN32
#ifndef     TRUE
#define     TRUE    1
#endif
#ifndef     FALSE
#define     FALSE   0
#endif
#ifndef     NULL
#define     NULL    0
#endif
#endif

#define MAX_QUERY_NUM    100  //每批查询行数

//统一错误码
/* 0X F8XX XXXX	数据库 */
#define DB_ERR_OK                       0
#define DB_ERR_BASE                     0XF8000000     //基本值

#define DB_ERR_ICE                      0XF8010000     //ICE错误
#define DB_ERR_DATABUS                  0XF8020000     //databus返回错误
#define DB_ERR_BDB                      0XF8040000     //BDB错误
#define DB_ERR_MYSQL                    0XF8080000     //Mysql错误
#define DB_ERR_OTHER                    0XF8100000     //其它错误

//ICE
#define DB_ERR_ICE_EXCEPTION           (DB_ERR_ICE | 1)

//DATABUS
#define DB_ERR_DATABUS_DBNOTEXIST      (DB_ERR_DATABUS | 1)
#define DB_ERR_DATABUS_FIELDNOTEXIST   (DB_ERR_DATABUS | 2)
#define DB_ERR_DATABUS_NOTGETREADY     (DB_ERR_DATABUS | 4)
#define DB_ERR_DATABUS_SQLEXCEPTION    (DB_ERR_DATABUS | 8)

//BDB

//Mysql  来自cppmysql类的错误
#define DB_ERR_MYSQL_TRANSFAIL         (DB_ERR_MYSQL | 1)     //开始事务失败
#define DB_ERR_MYSQL_COMMITFAIL        (DB_ERR_MYSQL | 2)     //提交事务失败
#define DB_ERR_MYSQL_ROLLBACKFAIL      (DB_ERR_MYSQL | 3)     //回滚事务失败
#define DB_ERR_MYSQL_OTHER             (DB_ERR_MYSQL | 4)     //其它mysql失败

//OTHER
#define DB_ERR_OTHER_UNCONNECT         (DB_ERR_OTHER | 1)
#define DB_ERR_OTHER_STRISNULL         (DB_ERR_OTHER | 2)     //字符串为空
#define DB_ERR_OTHER_OVERMAXROW        (DB_ERR_OTHER | 3)    //行号超出范围
#define DB_ERR_OTHER_MOVENEXTFAIL      (DB_ERR_OTHER | 4)   //movenext 失败
#define DB_ERR_OTHER_GETVALUEFAIL      (DB_ERR_OTHER | 5)    //取值失败
#define DB_ERR_OTHER_FIELDNUMERROR     (DB_ERR_OTHER | 6)
#define DB_ERR_OTHER_PARA              (DB_ERR_MYSQL | 7)     //参数错误
#define DB_ERR_OTHER_CONFAIL           (DB_ERR_MYSQL | 8)     //连接失败
#define DB_ERR_OHTER_SQLNULL           (DB_ERR_MYSQL | 9)     //sql为空
#define DB_ERR_OHTER_FILENAMENOT       (DB_ERR_MYSQL | 11)     //sql为空
#define DB_ERR_OTHER_UNKNOWN           (DB_ERR_OTHER | 100)  

#define DB_PING_ERROR_RECON            -10000

//wangqichao mysql-errcode ===================================
#define MySQL_Err_Code_Catch_Select    -80000
#define MySQL_Err_Code_Catch_Insert    -80001
#define MySQL_Err_Code_Catch_Delete    -80002
#define MySQL_Err_Code_Catch_Update    -80003

#define MySQL_Err_Code_Ping            -1000

#define MySQL_Err_Code_Connect         -1

#define MySQL_Suss_Code                0


//wangqichao mysql-errcode ===================================



// ***BEGIN***  增加数据库异常保护机制 wangqichao 2013/6/17 add
#define DB_ERROR_CODE         DB_ERR_BASE-100
#define DB_WRETURN_CODE       DB_ERR_BASE-101
const int MAXTIMENUM = 5;
const int CharNum = 500;
const int MAXGetNum = 10;
const int MAXSetNum = 10;
const int MAXWITEMNUM = 1000;
// ***END***  增加数据库异常保护机制 wangqichao 2013/6/17 add

typedef struct _dbErrorInfo
{
    unsigned int   iErrorCode;    //错误码
    string         strErrorMsg;   //错误说明
} DBERRORINFO;

class DBOperBase  
{
public:
    DBOperBase()
    {
        m_pciceConPara =NULL;
        m_pcdbPath =NULL;
        // ***BEGIN***  增加数据库异常保护机制 wangqichao 2013/6/17 add
        m_pciceConPara = new char[CharNum + 1];
        memset(m_pciceConPara, 0, CharNum);
        m_pcdbPath = new char[CharNum + 1];
        memset(m_pcdbPath, 0, CharNum);
        // ***END***  增加数据库异常保护机制 wangqichao 2013/6/17 add
        m_iLastOpErrorCode = DB_ERR_OK;
    };
    virtual ~DBOperBase()
    {
        if (m_pciceConPara)
        {
            delete []m_pciceConPara;
            m_pciceConPara = NULL;
        }

        if (m_pcdbPath)
        {
            delete []m_pcdbPath;
            m_pcdbPath = NULL;
        }
    
        m_iLastOpErrorCode = DB_ERR_OK;
    };

    virtual int GetTotalNum()=0;
    virtual int GetMyDBExit()=0;
    virtual void SetMyDBExit()=0;
    virtual bool Ice_Ping()=0;
    virtual void SetTimeOut(int iIceTimeOut)=0;

    virtual int  Connect(const char* szConPara, const char* dbPath, int iIceTimeOut = 3)=0; //带超时设置，连接Ice::databus数据库服务器
    virtual int  DBExecute(const char* sql)=0;    // 执行操作 int DBExecute(const char * sql,int &nExecRet);
    virtual int  DBQuery(const char* sql)=0;  // 执行查询语句
    virtual int  DBQueryNext(){return 0;}  // 执行查询下一批

    virtual void FreeResult()=0; // 释放查询的结果集
    virtual void Close()=0; // 关闭连接
    virtual int  MoveNext()=0;  //下移一条记录

    //----事务操作接口--begin------
    virtual int  BeginTransaction()=0;  //开启事务
    virtual int  Commit()=0;            //提交事务
    virtual int  RollBack()=0;          //回滚事务
    //----事务操作接口--end------

    //-------类ADO接口---begin---------
    //  获得指定行和指定列的整形值
    //  行列都从0开始编号
    //  row  ：行号
    //  fd_name ：列名称
    //  fd_num  ：列字段号
    //unsigned int
    virtual int  GetFieldValue(const char* fd_name, unsigned int& nValue)=0;
    virtual int  GetFieldValue(int fd_num, unsigned int& nValue)=0;
    //int
    virtual int  GetFieldValue(const char* fd_name, int& nValue)=0;
    virtual int  GetFieldValue(int fd_num, int& nValue)=0;

    //float
    virtual int  GetFieldValue(const char* fd_name, float& fValue)=0;
    virtual int  GetFieldValue(int fd_num, float& fValue)=0;

    //string
    virtual int  GetFieldValue(const char* fd_name, string& strValue)=0;
    virtual int  GetFieldValue(int fd_num, string& strValue)=0;

    //char* with length
    virtual int  GetFieldValue(const char* fd_name, char* pBuf, int nSize, unsigned* length)=0;
    virtual int  GetFieldValue(int fd_num, char* pBuf, int nSize, unsigned* length)=0;

    //char*
    virtual int  GetFieldValue(const char* fd_name, char* pBuf, int nSize)=0;
    virtual int  GetFieldValue(int fd_num, char* pBuf, int nSize)=0;

    //bin
    virtual int  GetFieldValue(const char* fd_name, BYTE* pBuf, int nSize, unsigned* length)=0;
    virtual int  GetFieldValue(int fd_num, BYTE* pBuf, int nSize, unsigned* length)=0;
    //-------类ADO接口---end-----------
    virtual CppMySQLQuery& getMySQLQueryRes()=0;
    

    WBOOL GetDBStatus(){return Status;}

    virtual int          GetLastOpErrorCode(){return m_iLastOpErrorCode;};
    virtual const char*  GetLastOpErrorMsg()
    {
        unsigned int dwErrorType=0;
        dwErrorType =  ((m_iLastOpErrorCode & 0X00040000) ||(m_iLastOpErrorCode & 0X00080000)); //BDB/mysql数据库内部错误
        if(dwErrorType) //DBD/mysql类型
        {
            return m_tLastDbErrorInfo.strErrorMsg.c_str();
        }
        else
        {
            switch (m_iLastOpErrorCode)
            {
            case DB_ERR_OK               :
                return "数据库操作成功";

            case DB_ERR_DATABUS_DBNOTEXIST :
                return "数据库不存在";

            case DB_ERR_DATABUS_FIELDNOTEXIST:
                return "字段不存在";

            case DB_ERR_DATABUS_NOTGETREADY  :
                return "数据库没有准备好";

            case DB_ERR_DATABUS_SQLEXCEPTION :
                return "SQL执行异常";

            case DB_ERR_ICE_EXCEPTION        :
                return "ICE通信异常";

            case DB_ERR_OTHER_UNCONNECT      :
                return "还没有打开数据库连接";

            case DB_ERR_OTHER_STRISNULL      :
                return "输入字符串为空";

            case DB_ERR_OTHER_OVERMAXROW     :
                return "已超过记录集最大行数";

            case DB_ERR_OTHER_MOVENEXTFAIL   :
                return "下移记录异常";

            case DB_ERR_OTHER_GETVALUEFAIL   :
                return "取字段值失败";
                
            case DB_ERR_OTHER_FIELDNUMERROR  :
                return "字段编号错误";

            default:
                return "数据库执行失败";
          }
         }
        return "";
     }

    //数据库错误信息
    virtual int          GetLastDbErrorCode(){return GetLastOpErrorCode();};
    virtual const char*  GetLastDbErrorMsg(){return GetLastOpErrorMsg();};
    /*****************************************************************************
	Prototype    : safeCopyStr
	Description  : 数据库字符串拷贝函数
	Input        : char* dst, const char* src, int maxlen
	Output       : None
	Return Value : int
	Calls        :
	Called By    :
	History      :
	1.Date       : 2013/6/28
	Author       : wangqichao
	Modification : Created function
	*****************************************************************************/
    char* safeCopyStr(char* dst, const char* src, int maxlen)
    {
        if (maxlen < 0)
        {
            return NULL;
        }
        // 只有目标缓冲区指针不为空才能拷贝
        if (dst)
        {
            int len = 0;
            if (src)
            {
                int temp = strlen(src);
                len = (temp <= maxlen) ? temp : maxlen;
                memcpy(dst, src, len);
            }
            dst[len] = NULL;
        }
        return dst;
    }

public:
    WBOOL         Status;              //连接状态
    char*        m_pciceConPara;
    char*        m_pcdbPath;
    //int          m_iIceTimeOut;       //Ice 超时设定  单位s
    int                         m_iLastOpErrorCode;  //操作错误码，EV9000标准错误码
    int                         m_iIceTimeOut;       //Ice 超时设定  单位s
    DBERRORINFO                 m_tLastDbErrorInfo;  //数据库操作错误信息，DBD错误码
};

#endif // !defined(AFX_DBOPERBASE_H__720058DD_AF8E_4251_B537_AFBB5E1CB9EA__INCLUDED_)

#endif
