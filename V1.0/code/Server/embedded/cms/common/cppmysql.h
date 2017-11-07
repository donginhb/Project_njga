////////////////////////////////////////////////////////////////////////////////
// CppMysql - A C++ wrapper around the mysql database library.
//
// Copyright (c) 2009 Rob Groves. All Rights Reserved. lizp.net@gmail.com
// 
// Permission to use, copy, modify, and distribute this software and its
// documentation for any purpose, without fee, and without a written
// agreement, is hereby granted, provided that the above copyright notice, 
// this paragraph and the following two paragraphs appear in all copies, 
// modifications, and distributions.
//
// IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT,
// INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST
// PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
// EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF
// ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS". THE AUTHOR HAS NO OBLIGATION
// TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
//
// u can use it for anything, but u must show the source
// frome http://rainfish.cublog.cn
// by ben
// if u find some questions, please tell me with email
//
// V1.0		18/09/2009	-Initial Version for cppmysql
////////////////////////////////////////////////////////////////////////////////
#ifndef _CPP_MYSQL_1_H_
#define _CPP_MYSQL_1_H_

#ifdef WIN32
#include "StdAfx.h"
#include <winsock2.h>
#endif

#include <string>
#include "mysql.h"
#include "errmsg.h"
using namespace std;

typedef unsigned int u_int;
typedef unsigned long u_long;
typedef MYSQL*   DB_HANDLE;

class CppMySQL3DB;

class CppMySQLQuery
{
      friend class CppMySQL3DB;
public:

    CppMySQLQuery();

    //当执行拷贝构造函数后，括号里的类已经无效，不能再使用
    CppMySQLQuery(CppMySQLQuery& rQuery);

    //当执行赋值构造函数后，=右边的类已经无效，不能再使用
    CppMySQLQuery& operator=(CppMySQLQuery& rQuery);

    virtual ~CppMySQLQuery();

    int numRow();//多少行
    int numFields();//多少列

    void setRow()
    {
        _row_count = 1001;
    }

    int fieldIndex(const char* szField);
    //0...n-1列
    const char* fieldName(int nCol);

 //   const char* fieldDeclType(int nCol);
 //   int fieldDataType(int nCol);

    u_long seekRow(u_long offerset);

    int getIntField(int nField, int nNullValue=0);
    int getIntField(const char* szField, int nNullValue=0);

    unsigned int getunsignedField(int nField, unsigned int nNullValue=0);
    unsigned int getunsignedField(const char* szField, unsigned int nNullValue=0);

    double getFloatField(int nField, double fNullValue=0.0);
    double getFloatField(const char* szField, double fNullValue=0.0);

    //0...n-1列
    const char* getStringField(int nField, const char* szNullValue="");
    const char* getStringField(const char* szField, const char* szNullValue="");

    const unsigned char* getBlobField(int nField, int& nLen);
    const unsigned char* getBlobField(const char* szField, int& nLen);

    bool fieldIsNull(int nField)
    {
        //TODO
        return false;
    }
    bool fieldIsNull(const char* szField)
    {
        //TODO
        return false;
    }

    bool eof();
    void nextRow();
    void finalize()
    {
        //TODO

        freeRes();
        
        return;
    }

private:
    void freeRes();
    void checkVM()
    {
        //TODO
        return ;
    }
private:
     MYSQL_RES*     _mysql_res;
     MYSQL_FIELD*   _field;
     MYSQL_ROW      _row;
     int            _row_count;
     u_int          _field_count;
};

class CppMySQL3DB
{
public:
    int myDBTableExit; 

    //重新连接次数
    int mConnetNum;
    //重新连接次数

    CppMySQL3DB();

    virtual ~CppMySQL3DB();

    int open(const char* host, const char* user, const char* passwd, const char* db,
             unsigned int port = 0, unsigned long client_flag = 0);

    void close();

     /* 返回句柄 */
     MYSQL* getMysql();

     /* 处理返回多行的查询，返回影响的行数 */
     //返回引用是因为在CppMySQLQuery的赋值构造函数中要把成员变量_mysql_res置为空
     CppMySQLQuery& querySQL(const char *sql,int& flag);
     /* 执行非返回结果查询 */
     int execSQL(const char* sql);
     /* 测试mysql服务器是否存活 */
     int ping();
     int mysql_reconnet();
     /* 关闭mysql 服务器 */
     int shutDown();
     /* 主要功能:重新启动mysql 服务器 */
     int reboot();
     /*
        说明:事务支持InnoDB or BDB表类型
    */
    /* 主要功能:开始事务 */
    int startTransaction();
    /* 主要功能:提交事务 */
    int commit();
    /* 主要功能:回滚事务 */
    int rollback();
    /* 得到客户信息 */
    const char * getClientInfo();
    /* 主要功能:得到客户版本信息 */
    const unsigned long  getClientVersion();
    /* 主要功能:得到主机信息 */
    const char * getHostInfo();
    /* 主要功能:得到服务器信息 */
    const char * getServerInfo();
    /*主要功能:得到服务器版本信息*/
    const unsigned long  getServerVersion();
    /*主要功能:得到 当前连接的默认字符集*/
    const char *  getCharacterSetName();
    /* 得到系统时间 */
    const char * getSysTime();
    /* 建立新数据库 */
    int createDB(const char* name);
    /* 删除制定的数据库*/
    int dropDB(const char* name);

    bool tableExists(const char* table);

    u_int lastRowId();

    void setBusyTimeout(int nMillisecs){};

    void GetMysqlError()
    {
        iErrorCode =0;
        strErrorMsg ="";
        if (NULL == _db_ptr)
        {
            iErrorCode = 0;
            strErrorMsg ="";
        }
        else
        {
            iErrorCode = mysql_errno(_db_ptr);
            strErrorMsg = mysql_error(_db_ptr);
        }
    }
    void GetMysqlError(MYSQL_STMT* _stmt)
    {
        iErrorCode =0;
        strErrorMsg ="";
        if (NULL == _stmt)
        {
            iErrorCode = 0;
            strErrorMsg ="";
        }
        else
        {
            iErrorCode = mysql_stmt_errno(_stmt);
            strErrorMsg = mysql_stmt_error(_stmt);
        }
    }

private:

    CppMySQL3DB(const CppMySQL3DB& db);
    CppMySQL3DB& operator=(const CppMySQL3DB& db);

    void checkDB();

    bool ReConnect();

//private:
public:
     /* msyql 连接句柄 */
     MYSQL* _db_ptr;
     //MYSQL_STMT *m_MyStmt;
     CppMySQLQuery _db_query;
     unsigned int   iErrorCode;    //错误码
     string         strErrorMsg;   //错误说明
     string m_host;
     string m_dbname;
     int m_port;
     string m_password;
     string m_user;
};

#endif

