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
//
// V1.0		18/09/2009	-Initial Version for cppmysql
////////////////////////////////////////////////////////////////////////////////

#include "cppmysql.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
using namespace std;

CppMySQLQuery::CppMySQLQuery()
{
    _mysql_res = NULL;
    _field = NULL;
    _row = NULL;
    _row_count = 0;
    _field_count = 0;

}

CppMySQLQuery::CppMySQLQuery(CppMySQLQuery& rQuery)
{
    *this = rQuery;
}

CppMySQLQuery& CppMySQLQuery::operator=(CppMySQLQuery& rQuery)
{
    if ( this == &rQuery )
        return *this;

    _mysql_res = rQuery._mysql_res;
    _row = NULL;
    _row_count = 0;
    _field_count = 0;
    _field = NULL;

    if ( _mysql_res != NULL )
    {
        //定位游标位置到第一个位置
        mysql_data_seek(_mysql_res, 0);
        _row =  mysql_fetch_row( _mysql_res );
        _row_count = mysql_num_rows( _mysql_res ); 
        //得到字段数量
        _field_count = mysql_num_fields( _mysql_res );
    }

    rQuery._mysql_res = NULL;
    rQuery._field = NULL;
    rQuery._row = NULL;
    rQuery._row_count = 0;
    rQuery._field_count = 0;

    return *this;

}

CppMySQLQuery::~CppMySQLQuery()
{
    freeRes();
}

void CppMySQLQuery::freeRes()
{
    if ( _mysql_res != NULL )
    {
        mysql_free_result(_mysql_res);
        _mysql_res = NULL;
    }
}

int CppMySQLQuery::numRow()
{
    return _row_count;
}

int CppMySQLQuery::numFields()
{
    return _field_count;
}

u_long CppMySQLQuery::seekRow(u_long offerset)
{
    if ( offerset < 0 )
        offerset = 0;
    if ( offerset >= _row_count )
        offerset = _row_count -1;

    mysql_data_seek(_mysql_res, offerset);

    _row = mysql_fetch_row(_mysql_res);
    return offerset;
}

int CppMySQLQuery::fieldIndex(const char* szField)
{
    if ( NULL == _mysql_res )
        return -1;
    if ( NULL == szField )
        return -1;

    mysql_field_seek(_mysql_res, 0);//定位到第0列
    u_int i = 0;
    while ( i < _field_count )
    {
        _field = mysql_fetch_field( _mysql_res );
        if ( _field != NULL && strcmp(_field->name, szField) == 0 )//找到
            return i;

        i++;
    }

    return -1;
}

const char* CppMySQLQuery::fieldName(int nCol)
{
     if ( _mysql_res == NULL )
         return NULL;

     mysql_field_seek(_mysql_res, nCol);
     _field = mysql_fetch_field(_mysql_res);

     if ( _field != NULL )
         return _field->name;
     else
         return  NULL;
}


unsigned int CppMySQLQuery::getunsignedField(int nField, unsigned int nNullValue/*=0*/)
{
    if ( NULL == _mysql_res )
        return nNullValue;
    if ( nField + 1 > (int)_field_count )
        return nNullValue;

    if ( NULL == _row )
        return nNullValue;

    //return atoi(_row[nField]);
    return strtoul(_row[nField], NULL, 10);
}

unsigned int CppMySQLQuery::getunsignedField(const char* szField, unsigned int nNullValue/*=0*/)
{
    if ( NULL == _mysql_res || NULL == szField )
        return nNullValue;

    if ( NULL == _row )
        return nNullValue;

    const char* filed = getStringField(szField);

    if ( NULL == filed )
        return nNullValue;

    //return atoi(filed);
    return strtoul(filed, NULL, 10);
}

int CppMySQLQuery::getIntField(int nField, int nNullValue/*=0*/)
{
    if ( NULL == _mysql_res )
        return nNullValue;

    if ( nField + 1 > (int)_field_count )
        return nNullValue;

    if ( NULL == _row )
        return nNullValue;

    return atoi(_row[nField]);
}

int CppMySQLQuery::getIntField(const char* szField, int nNullValue/*=0*/)
{
    if ( NULL == _mysql_res || NULL == szField )
        return nNullValue;

    if ( NULL == _row )
        return nNullValue;

    const char* filed = getStringField(szField);

    if ( NULL == filed )
        return nNullValue;

    return atoi(filed);
}

const char* CppMySQLQuery::getStringField(int nField, const char* szNullValue/*=""*/)
{
    if ( NULL == _mysql_res )
        return szNullValue;

    if ( nField + 1 > (int)_field_count )
        return szNullValue;

    if ( NULL == _row )
        return szNullValue;

    return _row[nField];
}
    
const char* CppMySQLQuery::getStringField(const char* szField, const char* szNullValue/*=""*/)
{
    if ( NULL == _mysql_res )
        return szNullValue;

    int nField = fieldIndex(szField);

    #ifdef WIN32

    if ( nField == -1 )
        return szNullValue;

    #else
    if ( nField == -1 )
    {
        return szNullValue;
    }
    
    #endif
    
    return getStringField(nField);
}

double CppMySQLQuery::getFloatField(int nField, double fNullValue/*=0.0*/)
{
    const char* field = getStringField(nField);

    if ( NULL == field )
        return fNullValue;

    return atol(field);
}
 
double CppMySQLQuery::getFloatField(const char* szField, double fNullValue/*=0.0*/)
{
    const char* field = getStringField(szField);
    if ( NULL == field )
        return fNullValue;

    return atol(field);
}


//TODO
const unsigned char* CppMySQLQuery::getBlobField(int nField, int& nLen)
{
    return NULL;
}

const unsigned char* CppMySQLQuery::getBlobField(const char* szField, int& nLen)
{
    return NULL;
}

void CppMySQLQuery::nextRow()
{
    if ( NULL == _mysql_res )
        return;

    _row = mysql_fetch_row(_mysql_res);
}

bool CppMySQLQuery::eof()
{
    if ( _row == NULL )
        return true;

    return false;
}

CppMySQL3DB::CppMySQL3DB()
{
    _db_ptr = NULL;
    myDBTableExit = 0;
    mConnetNum = 0;
}

CppMySQL3DB::~CppMySQL3DB()
{
    if ( _db_ptr != NULL )
    {
        close();
    }
}

int CppMySQL3DB::open(const char* host, const char* user, const char* passwd, const char* db,
                      unsigned int port /*= 0*/, unsigned long client_flag /*= 0*/)
{
    int ret = -1;
    printf("\r\n host = %s, user = %s, passwd = %s, db = %s, port = %d\r\n",host,user,passwd,db,port);
    try
    {
        if(_db_ptr != NULL)
        {
            mysql_close(_db_ptr);
            _db_ptr = NULL;
        }
    }
    catch(...)
    {
        _db_ptr = NULL;
    }
    _db_ptr = NULL;

    _db_ptr = mysql_init(NULL);
    if( NULL == _db_ptr ) 
    {
        strErrorMsg = "mysql_init失败";
        goto EXT;
    }
    //如果连接失败，返回NULL。对于成功的连接，返回值与第1个参数的值相同。
    if ( NULL == mysql_real_connect( _db_ptr, host, user, passwd, db,port, NULL, 0) )
    {
        cout<<"errno:"<<mysql_errno(_db_ptr)<<" error:"<<mysql_error(_db_ptr)<<endl;
        GetMysqlError();
        goto EXT;
    }
    //选择制定的数据库失败
    //0表示成功，非0值表示出现错误。
    if ( mysql_select_db( _db_ptr, db ) != 0 ) 
    {
        GetMysqlError();
        mysql_close(_db_ptr);
        _db_ptr = NULL;
        goto EXT;
    }

    ret = 0;
EXT:
    //初始化mysql结构失败
    if ( ret == -1 && _db_ptr != NULL )
    {
        GetMysqlError();
        mysql_close( _db_ptr );
        _db_ptr = NULL;
    }
    return ret;
}

void CppMySQL3DB::close()
{
    if ( _db_ptr != NULL )
    {
        try
        {
            mysql_close( _db_ptr );
        }
        catch(...)
        {
            _db_ptr = NULL; 
        }
        _db_ptr = NULL;
    }
}

MYSQL* CppMySQL3DB::getMysql()
{
     return _db_ptr;
}

/* 处理返回多行的查询，返回影响的行数 */
CppMySQLQuery& CppMySQL3DB::querySQL(const char *sql,int& flag)
{
//static int iflag=0;
try
{
    flag = 0;
    _db_query.finalize();
    if(ping()!=0)
    {
        flag = -1000;
        return _db_query;
    }

    if(sql == NULL || sql == "")
    {
        return _db_query;
    }
    #ifdef WIN32
    if(ping()!=0)
    {
        ReConnect();
    }
    #endif
    unsigned int my_leng = strlen(sql);    
    int resault = mysql_real_query( _db_ptr, sql, my_leng );
    
    if ( !resault )
    {
        //iflag = 0;
        mConnetNum = 0;
    
        _db_query._mysql_res = mysql_store_result( _db_ptr );
        if(_db_query._mysql_res == NULL)
        {
            printf("\r\n _db_query._mysql_res is NULL \r\n");
            
        }
        else
        {

        }




    }
    else if(1 == resault)
    {
        myDBTableExit = -1;
        printf("\r\n querySQL1:sql = %s resault = %d mConnetNum = %d\r\n",sql,resault,mConnetNum);
        mConnetNum++;
        if(mConnetNum <= 5)
        {
            int flag;
            return querySQL(sql,flag);
        }
        else
        {
            ReConnect();
        }
        
        //iflag++;
        //if(iflag <= 3)
        //{
        //    int flag;
        //    return querySQL(sql,flag);
        //} 
        mConnetNum = 0;
        return _db_query;
    }
    else
    {
       printf("\r\n querySQL2:sql = %s resault = %d\r\n",sql,resault);
       mConnetNum++;
        if(mConnetNum <= 5)
        {
            int flag;
            return querySQL(sql,flag);
        }
        else
        {
            ReConnect();
        }
   
       
       //ReConnect();
       myDBTableExit = -1;
       //iflag++;
       //if(iflag <= 3)
       //{
       //    int flag;
       //    return querySQL(sql,flag);
       //}

       mConnetNum = 0;
       return _db_query;
    }

}
catch(...)
{
    printf("\r\n querySQL3:sql = %s\r\n",sql); 
    ReConnect();
    _db_query.finalize();
    myDBTableExit = -1;
}
    mConnetNum = 0;
    return _db_query;
}

/* 执行非返回结果查询 */
int CppMySQL3DB::execSQL(const char* sql)
{
    if(ping()!=0)
    {
	    printf("\r\n execSQL1:sql = %s\r\n",sql); 
        ReConnect();
        return -1000;
    }

    // 参数合法性校验
    if (NULL == sql)
    {
        strErrorMsg = "sql语句为空";
        return -1;
    }

     // 初始化句柄
     // MySql预处理句柄
     MYSQL_STMT *m_MyStmt =NULL;
     m_MyStmt=mysql_stmt_init(_db_ptr);
     if (!m_MyStmt)
     {


        strErrorMsg = "mysql_stmt_init失败";
        return -1;
     }

    unsigned int my_leng = strlen(sql);

    
    if (0 != mysql_stmt_prepare(m_MyStmt, sql, my_leng))    
    {
         if (CR_SERVER_GONE_ERROR == mysql_stmt_errno(m_MyStmt)
            || CR_SERVER_LOST == mysql_stmt_errno(m_MyStmt))
         {       
            cout<<"DBOper::DBExecute： mysql_stmt_prepare failed! error:"<<mysql_stmt_errno(m_MyStmt)<<endl;
            GetMysqlError(m_MyStmt);
		    printf("\r\n execSQL2:sql = %s\r\n",sql); 
            if(!ReConnect())
            {
                return -1;
            }

            mysql_stmt_close(m_MyStmt);
            return -1;
        }
        else
        {
            cout << "DBOper::DBExecute: mysql_stmt_prepare failed! error: "<<mysql_stmt_error(m_MyStmt)<<endl;
            GetMysqlError(m_MyStmt);
            mysql_stmt_close(m_MyStmt);
        }
        return -1;
     }

    if (mysql_stmt_execute(m_MyStmt))
    {
        if (CR_SERVER_GONE_ERROR == mysql_stmt_errno(m_MyStmt)
           || CR_SERVER_LOST == mysql_stmt_errno(m_MyStmt))
        {

           
            cout << "DBOper::DBExecute: mysql_stmt_execute failed! errno: " << mysql_stmt_errno(m_MyStmt)<<endl;
            cout << "DBOper::DBExecute: reconnect!" << endl;
            GetMysqlError(m_MyStmt);
		    printf("\r\n execSQL3:sql = %s\r\n",sql); 
            if(!ReConnect())
            {
                return -1;
            } 
        }
        else
        {

            if(mysql_stmt_errno(m_MyStmt) == 1062 )
            {
                GetMysqlError(m_MyStmt);
                mysql_stmt_close(m_MyStmt);
                return -11062;
            }
            if(mysql_stmt_errno(m_MyStmt) == 1243)
            {
                GetMysqlError(m_MyStmt);
                mysql_stmt_close(m_MyStmt);
                return -11243;
            }
            cout <<"DBOper::DBExecute: mysql_stmt_execute failed! error: "<<mysql_stmt_errno(m_MyStmt)<<endl;
            GetMysqlError(m_MyStmt);
            mysql_stmt_close(m_MyStmt);
         }
        
         return -1;
     }
     // 获得受影响的行数
     int m_iAffectedRows=0;
     m_iAffectedRows = (unsigned)mysql_stmt_affected_rows(m_MyStmt);
     mysql_stmt_close(m_MyStmt);
     return m_iAffectedRows;
}

/* 测试mysql服务器是否存活 */
int CppMySQL3DB::ping()
{
    try
    {
        if(_db_ptr == NULL)
        {
            return -1;
        }
        if( mysql_ping(_db_ptr) == 0 )
            return 0;
        else
            return -1;
    }
    catch(...)
    {
        //delete _db_ptr;
        _db_ptr = NULL;
        return -1;
    }
        
}

int CppMySQL3DB::mysql_reconnet()
{
    if(_db_ptr == NULL)
    {
        return -1;
    }
    char value = 1;
    (void) mysql_init (_db_ptr);
    mysql_options(_db_ptr, MYSQL_OPT_RECONNECT, (char *)&value);
    return 0;
}

/* 关闭mysql 服务器 */
int CppMySQL3DB::shutDown()
{
    if( mysql_shutdown(_db_ptr,SHUTDOWN_DEFAULT) == 0 )
        return 0;
    else
    {
        strErrorMsg ="shutDown失败";
        return -1;
    }
}

/* 主要功能:重新启动mysql 服务器 */
int CppMySQL3DB::reboot()
{
     if(!mysql_reload(_db_ptr))
         return 0;
     else
         return -1;
}

/*
* 说明:事务支持InnoDB or BDB表类型
*/
/* 主要功能:开始事务 */
int CppMySQL3DB::startTransaction()
{
    if(!mysql_real_query(_db_ptr, "START TRANSACTION" ,
       (unsigned long)strlen("START TRANSACTION") ))
    {
         return 0;
    }
    else
    {
        GetMysqlError();
        //执行查询失败
        return -1;
    }
}

/* 主要功能:提交事务 */
int CppMySQL3DB::commit()
{
    if(!mysql_real_query( _db_ptr, "COMMIT",
      (unsigned long)strlen("COMMIT") ) )
    {
         return 0;
    }
    else 
    {
         GetMysqlError();
         //执行查询失败
         return -1;
    }
}

/* 主要功能:回滚事务 */
int CppMySQL3DB::rollback()
{
     if(!mysql_real_query(_db_ptr, "ROLLBACK",
        (unsigned long)strlen("ROLLBACK") ) )
           return 0;
     else
     {
         GetMysqlError();
         //执行查询失败
         return -1;
     }
}

/* 得到客户信息 */
const char * CppMySQL3DB::getClientInfo()
{
     return mysql_get_client_info();
}

/* 主要功能:得到客户版本信息 */
const unsigned long  CppMySQL3DB::getClientVersion()
{
    return mysql_get_client_version();
}

/* 主要功能:得到主机信息 */
const char * CppMySQL3DB::getHostInfo()
{
    return mysql_get_host_info(_db_ptr);
}

/* 主要功能:得到服务器信息 */
const char * CppMySQL3DB::getServerInfo()
{
    return mysql_get_server_info( _db_ptr ); 

}
/*主要功能:得到服务器版本信息*/
const unsigned long  CppMySQL3DB::getServerVersion()
{
    return mysql_get_server_version(_db_ptr);

}

/*主要功能:得到 当前连接的默认字符集*/
const char *  CppMySQL3DB::getCharacterSetName()
{
    return mysql_character_set_name(_db_ptr);

}

/* 得到系统时间 */
const char * CppMySQL3DB::getSysTime()
{
    //return ExecQueryGetSingValue("select now()");
    return NULL;

}

/* 建立新数据库 */
int CppMySQL3DB::createDB(const char* name)
{
     char temp[1024];

     printf(temp, "CREATE DATABASE %s", name);

     if(!mysql_real_query( _db_ptr, temp, strlen(temp)) )
         return 0;

     else
     {
         GetMysqlError();
         //执行查询失败
         return -1;
     }
}

/* 删除制定的数据库*/
int CppMySQL3DB::dropDB(const char*  name)
{
     char temp[1024];

     printf(temp, "DROP DATABASE %s", name);

     if(!mysql_real_query( _db_ptr, temp, strlen(temp)) )
         return 0;
     else
         //执行查询失败
         return -1;
}

bool CppMySQL3DB::tableExists(const char* table)
{
     return false;
}

u_int CppMySQL3DB::lastRowId()
{
     return 0;
}


bool CppMySQL3DB::ReConnect()
{
    printf("\r\n ReConnect() host = %s, user = %s, passwd = %s, db = %s, port = %d\r\n",m_host.c_str(),m_user.c_str(),m_password.c_str(),m_dbname.c_str(),m_port);
    if(m_host == "" || m_user == ""
        ||m_password == "" || m_dbname == ""
      )
    {
        return false;    
    }
    if(0 == open(m_host.c_str(),m_user.c_str(),m_password.c_str(),m_dbname.c_str(),m_port,0))
    {
        string sqlset;
        #ifndef WIN32
        sqlset.clear();
        sqlset = "SET character_set_client='gbk'";
        execSQL(sqlset.c_str());
        sqlset.clear();
        sqlset = "SET character_set_connection='gbk'";
        execSQL(sqlset.c_str());
        sqlset.clear();
        sqlset = "SET character_set_results='gbk'";
        execSQL(sqlset.c_str());
        sqlset.clear();
        sqlset = "SET interactive_timeout=604800";
        execSQL(sqlset.c_str());
        sqlset.clear();
        sqlset = "SET wait_timeout=604800";
        execSQL(sqlset.c_str());
        sqlset.clear();
        sqlset = "SET GLOBAL max_connections=100000";
        execSQL(sqlset.c_str());
        #else
        sqlset.empty();
        sqlset = "SET character_set_client='gbk'";
        execSQL(sqlset.c_str());
        sqlset.empty();
        sqlset = "SET character_set_connection='gbk'";
        execSQL(sqlset.c_str());
        sqlset.empty();
        sqlset = "SET character_set_results='gbk'";
        execSQL(sqlset.c_str());
        #endif
        return true;
    }
    else
    {
        return false;
    }
}

