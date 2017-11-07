
///////////////////////////////////////////////////
#include "DBOper.h"
#include <iostream>
#include <map>
#include <string>
#include <fstream>

using namespace std;

//DBOper* DBOper::DB_pInstance = NULL;

void osip_usleep(int useconds)
{
#ifdef WIN32
        Sleep(useconds / 1000);
#else
        struct timeval delay;
        int sec;
    
        sec = (int) useconds / 1000000;
    
        if (sec > 0)
        {
            delay.tv_sec = sec;
            delay.tv_usec = 0;
        }
        else
        {
            delay.tv_sec = 0;
            delay.tv_usec = useconds;
        }
    
        select(0, 0, 0, 0, &delay);
#endif

}

/* DBOper���켰�������� */
DBOperSon::DBOperSon()
{
	m_pDb = NULL;
#ifdef USE_BDB
    m_pDb = new DBOperBdb();
#else
    m_pDb = new DBOperMysql();
#endif

    #ifndef WIN32
    m_iceConPara.clear();
    m_dbPath.clear();
    #else
    m_iceConPara.empty();
    m_dbPath.empty();
    #endif
}
//DBOper ��������
DBOperSon::~DBOperSon()
{
   if(m_pDb)
   {
      delete(m_pDb);
      m_pDb =NULL;
   }
}

bool DBOperSon::Ice_Ping()
{
    return true;
    if(m_pDb)
    {
        return m_pDb->Ice_Ping();
    }
    return false;
}

//����������Ϣ
int  DBOperSon::GetLastOpErrorCode()
{
    if(m_pDb)
    {
        return m_pDb->GetLastOpErrorCode();
    }
    return -1;
}

const char*  DBOperSon::GetLastOpErrorMsg()
{
    if(m_pDb)
    {
        return m_pDb->GetLastOpErrorMsg();
    }
    else
    {
        return "";
    }
}

//���ݿ������Ϣ
int DBOperSon::GetLastDbErrorCode()
{
    if(m_pDb)
    {
        return m_pDb->GetLastDbErrorCode();
    }
    return -1;
}

const char* DBOperSon::GetLastDbErrorMsg()
{
    if(m_pDb)
    {
        return m_pDb->GetLastDbErrorMsg();
    }
    return "";
}

/* ���ݿ����ӣ�����ʱ���� */
int DBOperSon::Connect(const char* iceConPara, const char* dbPath, int iIceTimeOut) //����Ice::databus���ݿ������
{
    if(m_pDb)
    {
        m_iceConPara = iceConPara;
        m_dbPath = dbPath;
        return m_pDb->Connect(iceConPara, dbPath, iIceTimeOut);
    }
    return -1;
}


/*****************************************************************************
 Prototype    : safeCopyStr
 Description  : ���ݿ��ַ�����������
 Input        : char* dst, const char* src, int maxlen
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/28
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
char* DBOperSon::safeCopyStr(char* dst, const char* src, int maxlen)
{
    if(m_pDb)
    {
        return m_pDb->safeCopyStr(dst,src,maxlen);
    }
    return NULL;
}

// ִ�в���
int DBOperSon::DBExecute(const char* sql)
{
    if(m_pDb)
    {
        return m_pDb->DBExecute(sql);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

/*
 * //���ݿ�� ��ѯ
 *���� ��ѯ�������
 * const char * sql sql��ѯ���
 */
int DBOperSon::DBQuery(const char* sql)
{
    if(m_pDb)
    {

        FreeResult();

        #ifndef WIN32
        return m_pDb->DBQuery(sql);
        #else
        int ret = m_pDb->DBQuery(sql);
        if(ret == DB_ERR_OK)
        {
            return GetTotalNum();
        } 
        else
        {
            return ret;
        }
        #endif

    }
    return DB_ERR_OTHER_UNKNOWN;
}

/*
 * //���ݿ�� ��ѯ
 *���� ��ѯ�������
 * const char * sql sql��ѯ���
 */
int DBOperSon::DBQueryNext()
{
    if(m_pDb)
    {
        return m_pDb->DBQueryNext();
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//�ͷŲ�ѯ�Ľ����
void DBOperSon::FreeResult()
{
    if(m_pDb)
    {
        m_pDb->FreeResult();
    }
    return;
}

// �ر�����
void DBOperSon::Close()
{
    if(m_pDb)
    {
        m_pDb->Close();
    }
    m_pDb = NULL;
    return;
}

// ����һ����¼
int DBOperSon::MoveNext()
{
    if(m_pDb)
    {
        return m_pDb->MoveNext();
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//----��������ӿ�--begin------
//��������
int DBOperSon::BeginTransaction()
{
    if(m_pDb)
    {
        return m_pDb->BeginTransaction();
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//�ύ����
int DBOperSon::Commit()
{
    if(m_pDb)
    {
        return m_pDb->Commit();
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//�ع�����
int DBOperSon::RollBack()
{
    if(m_pDb)
    {
        return m_pDb->RollBack();
    }
    return DB_ERR_OTHER_UNKNOWN;
}
//----��������ӿ�--end------

//-------��ADO�ӿ�---begin---------
//  ���ָ���к�ָ���е�����ֵ
//  ���ж���0��ʼ���
//  row  ���к�
//  fd_name ��������
//  fd_num  �����ֶκ�

//unsigned int
int DBOperSon::GetFieldValue(const char* fd_name, unsigned int& nValue)
{
    if(m_pDb)
    {
        return m_pDb->GetFieldValue(fd_name, nValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

int DBOperSon::GetFieldValue(int fd_num, unsigned int& nValue)
{
    if(m_pDb)
    {
        return m_pDb->GetFieldValue(fd_num, nValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//int
int DBOperSon::GetFieldValue(const char* fd_name, int& nValue)
{
    if(m_pDb)
    {
        return m_pDb->GetFieldValue(fd_name, nValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

int DBOperSon::GetFieldValue(int fd_num, int& nValue)
{
    if(m_pDb)
    {
        return m_pDb->GetFieldValue(fd_num, nValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//float
int DBOperSon::GetFieldValue(const char* fd_name, float& fValue)
{
    if(m_pDb)
    {
        return m_pDb->GetFieldValue(fd_name, fValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

int DBOperSon::GetFieldValue(int fd_num, float& fValue)
{
    if(m_pDb)
    {
        return m_pDb->GetFieldValue(fd_num, fValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//string
int DBOperSon::GetFieldValue(const char* fd_name, string& strValue)
{
    if(m_pDb)
    {
        return m_pDb->GetFieldValue(fd_name, strValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

int DBOperSon::GetFieldValue(int fd_num, string& strValue)
{
    if(m_pDb)
    {
        return m_pDb->GetFieldValue(fd_num, strValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//char* with length
int DBOperSon::GetFieldValue(const char* fd_name, char* pBuf, int nSize, unsigned* length)
{
    if(m_pDb)
    {
        return m_pDb->GetFieldValue(fd_name, pBuf, nSize, length);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

int DBOperSon::GetFieldValue(int fd_num, char* pBuf, int nSize, unsigned* length)
{
    if(m_pDb)
    {
        return m_pDb->GetFieldValue(fd_num, pBuf, nSize, length);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//char*
int DBOperSon::GetFieldValue(const char* fd_name, char* pBuf, int nSize)
{
    if(m_pDb)
    {
        return m_pDb->GetFieldValue(fd_name, pBuf, nSize);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

int DBOperSon::GetFieldValue(int fd_num, char* pBuf, int nSize)
{
    if(m_pDb)
    {
        return m_pDb->GetFieldValue(fd_num, pBuf, nSize);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//bin
int DBOperSon::GetFieldValue(const char* fd_name, BYTE* pBuf, int nSize, unsigned* length)
{
    if(m_pDb)
    {
        return m_pDb->GetFieldValue(fd_name,pBuf,  nSize, length);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

int DBOperSon::GetFieldValue(int fd_num, BYTE* pBuf, int nSize, unsigned* length)
{
    if(m_pDb)
    {
        return m_pDb->GetFieldValue(fd_num,  pBuf,  nSize, length);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//-------��ADO�ӿ�---end-----------

// ***BEGIN***  �������ݿ��쳣�������� wangqichao 2013/6/17 add
/*****************************************************************************
 Prototype    : DB_Abnormity_Manage_Select
 Description  : ���ݿ��ѯ����ύ�쳣����
 Input        : const char * sql
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int DBOperSon::DB_Abnormity_Manage_Select(const char* sql)
{
    if (sql == NULL)
    {
        return DB_ERR_OTHER_STRISNULL;
    }

    int i_Result = 0;

    try
    {
        int i_time_out = 0;
        bool bIsOpenDB = false;

        while (!Ice_Ping())
        {
            //���ݿ��ն˻�ICE�������⣬�ظ�pingֱ����ͨΪֹ
            bIsOpenDB = true;

            if (i_time_out >= MAXTIMENUM)
            {
                //��������Ƿ�����CMS
                return DB_WRETURN_CODE;
            }

#ifdef WIN32
            Sleep(i_time_out);
#else
            osip_usleep(1000 * i_time_out);
#endif
            i_time_out++;
        }

        if (bIsOpenDB)
        {
            if(!m_pDb)
            {
                return DB_ERROR_CODE;
            }
            i_Result = Connect(m_pDb->m_pciceConPara, m_pDb->m_pcdbPath);

            if (i_Result < 0)
            {
                //�������
                return i_Result;
            }
        }

        i_Result = DBQuery(sql);

        if (i_Result < 0)
        {
            return i_Result;
        }

    }
    catch (...)
    {
        return DB_ERROR_CODE;
    }

    return DB_ERR_OK;
}


/*****************************************************************************
 Prototype    : DB_Abnormity_Manage_Insert
 Description  : ���ݿ��������ύ�쳣����
 Input        : const char * sql_Select,const char * sql_Update,const char * sql_Insert
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int DBOperSon::DB_Abnormity_Manage_Insert(const char* sql_Select, const char* sql_Update, const char* sql_Insert)
{
    if (sql_Select == NULL || sql_Update == NULL || sql_Insert == NULL)
    {
        return DB_ERR_OTHER_STRISNULL;
    }

    int i_Result = 0;

    try
    {
        int i_time_out = 0;
        bool bIsOpenDB = false;

        while (!Ice_Ping())
        {
            //���ݿ��ն˻�ICE�������⣬�ظ�pingֱ����ͨΪֹ
            bIsOpenDB = true;

            if (i_time_out >= MAXTIMENUM)
            {
                //��������Ƿ�����CMS
                return DB_WRETURN_CODE;
            }

#ifdef WIN32
            Sleep(i_time_out);
#else
            osip_usleep(1000 * i_time_out);
#endif
            i_time_out++;
        }

        if (bIsOpenDB)
        {
            if(!m_pDb)
            {
                return DB_ERROR_CODE;
            }
            i_Result = Connect(m_pDb->m_pciceConPara, m_pDb->m_pcdbPath);

            if (i_Result < 0)
            {
                //�������
                return i_Result;
            }
        }

        // ��һ ��ѯ
        i_Result = DBQuery(sql_Select);

        if (i_Result < 0)
        {
            return i_Result;
        }

        if (GetTotalNum() > 0)
        {
            //�ϴβ������ݳɹ�������ֻ��Ҫ����һ�¡�
            i_Result = DBExecute(sql_Update);

            if (i_Result < 0)
            {
                return i_Result;
            }
        }
        else
        {
            //�ϴβ�������û�гɹ���������Ҫ���²������ݡ�
            i_Result = DBExecute(sql_Insert);

            if (i_Result < 0)
            {
                return i_Result;
            }
        }

    }
    catch (...)
    {

    }

    return DB_ERR_OK;
}


/*****************************************************************************
 Prototype    : DB_Abnormity_Manage_Update
 Description  : ���ݿ��������ύ�쳣����
 Input        : const char * sql
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int DBOperSon::DB_Abnormity_Manage_Update(const char* sql)
{
    if (sql == NULL)
    {
        return DB_ERR_OTHER_STRISNULL;
    }

    int i_Result = 0;

    try
    {
        int i_time_out = 0;
        bool bIsOpenDB = false;

        while (!Ice_Ping())
        {
            //���ݿ��ն˻�ICE�������⣬�ظ�pingֱ����ͨΪֹ
            bIsOpenDB = true;

            if (i_time_out >= MAXTIMENUM)
            {
                //��������Ƿ�����CMS
                return DB_WRETURN_CODE;
            }

#ifdef WIN32
            Sleep(i_time_out);
#else
            osip_usleep(1000 * i_time_out);
#endif
            i_time_out++;
        }

        if (bIsOpenDB)
        {
            if(!m_pDb)
            {
                return DB_ERROR_CODE;
            }
            i_Result = Connect(m_pDb->m_pciceConPara, m_pDb->m_pcdbPath);

            if (i_Result < 0)
            {
                //�������
                return i_Result;
            }
        }

        i_Result = DBExecute(sql);

        if (i_Result < 0)
        {
            return i_Result;
        }

    }
    catch (...)
    {
        return DB_ERROR_CODE;
    }

    return DB_ERR_OK;
}

/*****************************************************************************
 Prototype    : DB_Abnormity_Manage_Delete
 Description  : ���ݿ�ɾ������ύ�쳣����
 Input        : const char * sql
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int DBOperSon::DB_Abnormity_Manage_Delete(const char* sql)
{
    if (sql == NULL)
    {
        return DB_ERR_OTHER_STRISNULL;
    }

    int i_Result = 0;

    try
    {
        int i_time_out = 0;
        bool bIsOpenDB = false;

        while (!Ice_Ping())
        {
            //���ݿ��ն˻�ICE�������⣬�ظ�pingֱ����ͨΪֹ
            bIsOpenDB = true;

            if (i_time_out >= MAXTIMENUM)
            {
                //��������Ƿ�����CMS
                return DB_WRETURN_CODE;
            }

#ifdef WIN32
            Sleep(i_time_out);
#else
            osip_usleep(1000 * i_time_out);
#endif
            i_time_out++;
        }

        if (bIsOpenDB)
        {
            if(!m_pDb)
            {
                return DB_ERROR_CODE;
            }
            i_Result = Connect(m_pDb->m_pciceConPara, m_pDb->m_pcdbPath);

            if (i_Result < 0)
            {
                //�������
                return i_Result;
            }
        }

        i_Result = DBExecute(sql);

        if (i_Result < 0)
        {
            return i_Result;
        }

    }
    catch (...)
    {
        return DB_ERROR_CODE;
    }

    return DB_ERR_OK;
}


/*****************************************************************************
 Prototype    : DB_Select
 Description  : ���ݿ��ѯ����ύ
 Input        : const char * sql,int Flag
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int DBOperSon::GetmyDBTableExit()
{
    return m_pDb->GetMyDBExit();
}
void DBOperSon::SetmyDBTableExit()
{
    m_pDb->SetMyDBExit();
}

int DBOperSon::DB_Select(const char* sql, int Flag)
{
    if (NULL == sql || "" == sql)
    {
        return DB_ERR_OTHER_STRISNULL;
    }
    
    int iRet = 0;
    try
    {
         iRet = DBQuery(sql);
         if(MySQL_Err_Code_Ping == iRet)
         {
             if(MySQL_Err_Code_Connect == Connect(m_iceConPara.c_str(),m_dbPath.c_str()))
             {
                 return MySQL_Err_Code_Connect;
             }
             else
             {
                 iRet = DBQuery(sql);
             }
         }
    }
    catch(...)
    {
         return MySQL_Err_Code_Catch_Select;
    }


        
    if(iRet == 0)
    {
         iRet = GetTotalNum();
    }
    return iRet;
    
}
/*****************************************************************************
 Prototype    : DB_Insert
 Description  : ���ݿ��������ύ
 Input        : const char * sql_Select,const char * sql_Update,const char * sql_Insert,int Flag
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int DBOperSon::DB_Insert(const char* sql_Select, const char* sql_Update, const char* sql_Insert, int Flag)
{
    if (NULL == sql_Insert || "" == sql_Insert)
    {
        return DB_ERR_OTHER_STRISNULL;
    }
    int iRet = 0;
    try
    {
        iRet = DBExecute(sql_Insert);
        if(MySQL_Err_Code_Ping == iRet)
         {
             if(MySQL_Err_Code_Connect == Connect(m_iceConPara.c_str(),m_dbPath.c_str()))
             {
                 return MySQL_Err_Code_Connect;
             }
             else
             {
                 iRet = DBExecute(sql_Insert);
             }
         }
    }
    
    catch(...)
    {
         return MySQL_Err_Code_Catch_Insert;
    }


    return iRet;


    
}
/*****************************************************************************
 Prototype    : DB_Update
 Description  : ���ݿ��������ύ
 Input        : const char * sql,int Flag
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int DBOperSon::DB_Update(const char* sql, int Flag)
{
    if (NULL == sql || "" == sql)
    {
        return DB_ERR_OTHER_STRISNULL;
    }
    int iRet = 0;
    try
    {
        iRet = DBExecute(sql);
        if(MySQL_Err_Code_Ping == iRet)
         {
             if(MySQL_Err_Code_Connect == Connect(m_iceConPara.c_str(),m_dbPath.c_str()))
             {
                 return MySQL_Err_Code_Connect;
             }
             else
             {
                 iRet = DBExecute(sql);
             }
         }
    }
    
    catch(...)
    {
         return MySQL_Err_Code_Catch_Update;
    }

    return iRet;
}
/*****************************************************************************
 Prototype    : DB_Delete
 Description  : ���ݿ�ɾ������ύ
 Input        : const char * sql,int Flag
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int DBOperSon::DB_Delete(const char* sql, int Flag)
{
    if (NULL == sql || "" == sql)
    {
        return DB_ERR_OTHER_STRISNULL;
    }
    int iRet = 0;
    try
    {
        iRet = DBExecute(sql);
        if(MySQL_Err_Code_Ping == iRet)
         {
             if(MySQL_Err_Code_Connect == Connect(m_iceConPara.c_str(),m_dbPath.c_str()))
             {
                 return MySQL_Err_Code_Connect;
             }
             else
             {
                 iRet = DBExecute(sql);
             }
         }
    }
    catch(...)
    {
         return MySQL_Err_Code_Catch_Delete;
    }
    
    return iRet;

    
}

int DBOperSon::DB_WSelect(const char* sql, CppMySQLQuery& m_oQueryRes, int Flag)
{
    if (NULL == sql || "" == sql)
    {
        return DB_ERR_OTHER_STRISNULL;
    }
    int ret = DBQuery(sql);
    if(ret >= 0)
    {
        m_oQueryRes = m_pDb->getMySQLQueryRes();
    }
    
    return ret;
}

//--------------------------------------
DBOper::DBOper()
{
    pDBOperSon_EV9000DB = NULL;
    pDBOperSon_MyDB = NULL;
}
DBOper::~DBOper()
{
    if(NULL != pDBOperSon_EV9000DB)
    {
        pDBOperSon_EV9000DB->Close();
        delete pDBOperSon_EV9000DB;
    }
	
    if(NULL != pDBOperSon_MyDB)
    {
        pDBOperSon_MyDB->Close();
        delete pDBOperSon_MyDB;
    }
}

void DBOper::Close()
{
    if(NULL != pDBOperSon_EV9000DB)
    {
        pDBOperSon_EV9000DB->Close();
        pDBOperSon_EV9000DB = NULL;
        //delete pDBOperSon_EV9000DB;
    }
    if(NULL != pDBOperSon_MyDB)
    {
        pDBOperSon_MyDB->Close();
        pDBOperSon_MyDB = NULL;
        //delete pDBOperSon_MyDB;
    }
}
    
int  DBOper::GetFieldValue(const char* fd_name, unsigned int& nValue)
{
    if(pDBOperSon_EV9000DB)
    {
        return pDBOperSon_EV9000DB->GetFieldValue(fd_name, nValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}
int  DBOper::GetFieldValue(int fd_num, unsigned int& nValue)
{
    if(pDBOperSon_EV9000DB)
    {
        return pDBOperSon_EV9000DB->GetFieldValue(fd_num, nValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}
//int
int  DBOper::GetFieldValue(const char* fd_name, int& nValue)
{
    if(pDBOperSon_EV9000DB)
    {
        return pDBOperSon_EV9000DB->GetFieldValue(fd_name, nValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}
int  DBOper::GetFieldValue(int fd_num, int& nValue)
{
    if(pDBOperSon_EV9000DB)
    {
        return pDBOperSon_EV9000DB->GetFieldValue(fd_num, nValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//float
int  DBOper::GetFieldValue(const char* fd_name, float& fValue)
{
    if(pDBOperSon_EV9000DB)
    {
        return pDBOperSon_EV9000DB->GetFieldValue(fd_name, fValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}
int  DBOper::GetFieldValue(int fd_num, float& fValue)
{
    if(pDBOperSon_EV9000DB)
    {
        return pDBOperSon_EV9000DB->GetFieldValue(fd_num, fValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//string
int  DBOper::GetFieldValue(const char* fd_name, string& strValue)
{
    if(pDBOperSon_EV9000DB)
    {
        return pDBOperSon_EV9000DB->GetFieldValue(fd_name, strValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}
int  DBOper::GetFieldValue(int fd_num, string& strValue)
{
    if(pDBOperSon_EV9000DB)
    {
        return pDBOperSon_EV9000DB->GetFieldValue(fd_num, strValue);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//char* with length
int  DBOper::GetFieldValue(const char* fd_name, char* pBuf, int nSize, unsigned* length)
{
    if(pDBOperSon_EV9000DB)
    {
        return pDBOperSon_EV9000DB->GetFieldValue(fd_name, pBuf, nSize, length);
    }
    return DB_ERR_OTHER_UNKNOWN;
}
int  DBOper::GetFieldValue(int fd_num, char* pBuf, int nSize, unsigned* length)
{
    if(pDBOperSon_EV9000DB)
    {
        return pDBOperSon_EV9000DB->GetFieldValue(fd_num, pBuf, nSize, length);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//char*
int  DBOper::GetFieldValue(const char* fd_name, char* pBuf, int nSize)
{
    if(pDBOperSon_EV9000DB)
    {
        return pDBOperSon_EV9000DB->GetFieldValue(fd_name, pBuf, nSize);
    }
    return DB_ERR_OTHER_UNKNOWN;
}
int  DBOper::GetFieldValue(int fd_num, char* pBuf, int nSize)
{
    if(pDBOperSon_EV9000DB)
    {
        return pDBOperSon_EV9000DB->GetFieldValue(fd_num, pBuf, nSize);
    }
    return DB_ERR_OTHER_UNKNOWN;
}

//bin
int  DBOper::GetFieldValue(const char* fd_name, BYTE* pBuf, int nSize, unsigned* length)
{
    if(pDBOperSon_EV9000DB)
    {
        return pDBOperSon_EV9000DB->GetFieldValue(fd_name, pBuf, nSize, length);
    }
    return DB_ERR_OTHER_UNKNOWN;
}
int  DBOper::GetFieldValue(int fd_num, BYTE* pBuf, int nSize, unsigned* length)
{
    if(pDBOperSon_EV9000DB)
    {
        return pDBOperSon_EV9000DB->GetFieldValue(fd_num, pBuf, nSize, length);
    }
    return DB_ERR_OTHER_UNKNOWN;
}


int  DBOper::Connect(char iceConPara[2][100], const char* Mytable)
{
    m_MyTable = Mytable;
	
    if(NULL == pDBOperSon_EV9000DB)
    {
        pDBOperSon_EV9000DB = new DBOperSon();
    }
	
    if(NULL != pDBOperSon_EV9000DB)
    {
	    if(pDBOperSon_EV9000DB->Connect(iceConPara[0], Mytable)<0)
	    {
	        delete pDBOperSon_EV9000DB;
	        pDBOperSon_EV9000DB = NULL;
	        return -1;
	    }
    }

	if (strlen(iceConPara[1]) > 0)
	{
	    if(NULL == pDBOperSon_MyDB)
	    {
	        pDBOperSon_MyDB = new DBOperSon();
	    }

	    if(NULL != pDBOperSon_MyDB)
		{
		    if(pDBOperSon_MyDB->Connect(iceConPara[1], Mytable)<0)
		    {
		        delete pDBOperSon_MyDB;
		        pDBOperSon_MyDB = NULL;
		        return -1;
		    }
		}
	}
		    
    return 0;
}

//����������Ϣ
int  DBOper::GetLastOpErrorCode()
{
    return -1;
}
const char* DBOper::GetLastOpErrorMsg()
{
    return "";
}

//���ݿ������Ϣ
int  DBOper::GetLastDbErrorCode()
{
    return -1;
}
const char* DBOper::GetLastDbErrorMsg()
{
    return "";
}


int DBOper::DBExecute(const char* sql, int Flag)
{
    if(Flag == 1)
    {
    	if (NULL != pDBOperSon_EV9000DB)
		{
	         return pDBOperSon_EV9000DB->DBExecute(sql);
		}
    }
    else
    {
    	if (NULL != pDBOperSon_MyDB)
		{
	        return pDBOperSon_MyDB->DBExecute(sql);
		}
    }
}

int DBOper::DB_Select(const char* sql, int Flag, int Level)
{
    if(Flag == 1)
    {
         return pDBOperSon_EV9000DB->DB_Select(sql, Flag);
    }
    else
    {
        //��ѯû���첽
        return pDBOperSon_EV9000DB->DB_Select(sql, Flag);
    }
}
int DBOper::DB_Insert(const char* sql_Select, const char* sql_Update, const char* sql_Insert, int Flag, int Level)
{
    if(Flag == 1)
    {
    	if (NULL != pDBOperSon_EV9000DB)
		{
	         return pDBOperSon_EV9000DB->DB_Insert(sql_Select,sql_Update,sql_Insert, Flag);
		}
    }
    else
    {
    	if (NULL != pDBOperSon_MyDB)
		{
	        char tmp_sql[2000] = {0};
	        sprintf(tmp_sql,"insert into %s(sql_str) values(\"%s\");",m_MyTable.c_str(),sql_Insert);
	        return pDBOperSon_MyDB->DB_Insert("","",tmp_sql, Flag);
		}
    }
}
int DBOper::DB_Update(const char* sql, int Flag, int Level)
{
    if(Flag == 1)
    {
    	if (NULL != pDBOperSon_EV9000DB)
		{
	         return pDBOperSon_EV9000DB->DB_Update(sql, Flag);
		}
    }
    else
    {
     	if (NULL != pDBOperSon_MyDB)
		{
	        char tmp_sql[2000] = {0};
	        sprintf(tmp_sql,"insert into %s(sql_str) values(\"%s\");",m_MyTable.c_str(),sql);
	        return pDBOperSon_MyDB->DB_Insert("","",tmp_sql, Flag);
		}
    }
}
int DBOper::DB_Delete(const char* sql, int Flag, int Level)
{
    if(Flag == 1)
    {
    	if (NULL != pDBOperSon_EV9000DB)
		{
	         return pDBOperSon_EV9000DB->DB_Delete(sql, Flag);
		}
    }
    else
    {
     	if (NULL != pDBOperSon_MyDB)
		{
	        char tmp_sql[2000] = {0};
	        sprintf(tmp_sql,"insert into %s(sql_str) values(\"%s\");",m_MyTable.c_str(),sql);
	        return pDBOperSon_MyDB->DB_Insert("","",tmp_sql, Flag);
		}
    }
}

// ***END***  �������ݿ��쳣�������� wangqichao 2013/6/17 add
#ifndef WIN32

void * DB_Thread::Thread_run(void* p)
{
    DB_Thread* This = (DB_Thread*)p;
    //string strStrTest = "";
    //string strInsertSQL = "";
    while(0 == This->Thread_exit)
    {
        This->SqlRead(); 
        osip_usleep(1000);
    }
    pthread_join(This->m_DBthreadId, NULL);
    return NULL;
}


void DB_Thread::SqlRead()
{
    string strSQL = "";
    char strtmpIndex[100] = {0};
    
    string str_EV9000DB = "";
    int index_EV9000DB = -1;
    int record_count = -1;
    //���ݿ� EV9000DB
    str_EV9000DB.clear();
    index_EV9000DB = -1;
    record_count = -1;
    record_count = m_pEV9000MyOper->DB_Select("select * from table_EV9000DB limit 1;");
    if (record_count > 0)
    {        
        m_pEV9000MyOper->GetFieldValue("ID", index_EV9000DB);
        m_pEV9000MyOper->GetFieldValue("sql_str", str_EV9000DB);

        m_pEV9000DBOper->DB_Insert("", "", str_EV9000DB.c_str());

        strSQL.clear();
        memset(strtmpIndex,0,100);
        strSQL = "delete from table_EV9000DB";
        strSQL += " WHERE ID = ";
        sprintf(strtmpIndex, "%d", index_EV9000DB);
        strSQL += strtmpIndex;
        
        m_pEV9000MyOper->DB_Delete(strSQL.c_str());
    }

    //���ݿ� EV9000TSU
    str_EV9000DB.clear();
    index_EV9000DB = -1;
    record_count = -1;
    record_count = m_pEV9000MyOper->DB_Select("select * from table_EV9000TSU limit 1;");
    if (record_count > 0)
    {        
        m_pEV9000MyOper->GetFieldValue("ID", index_EV9000DB);
        m_pEV9000MyOper->GetFieldValue("sql_str", str_EV9000DB);

        m_pEV9000TSUOper->DB_Insert("", "", str_EV9000DB.c_str());

        strSQL.clear();
        memset(strtmpIndex,0,100);
        strSQL = "delete from table_EV9000TSU";
        strSQL += " WHERE ID = ";
        sprintf(strtmpIndex, "%d", index_EV9000DB);
        strSQL += strtmpIndex;
        
        m_pEV9000MyOper->DB_Delete(strSQL.c_str());
    }
    
    //���ݿ� EV9000LOG
    str_EV9000DB.clear();
    index_EV9000DB = -1;
    record_count = -1;
    record_count = m_pEV9000MyOper->DB_Select("select * from table_EV9000LOG limit 1;");
    if (record_count > 0)
    {        
        m_pEV9000MyOper->GetFieldValue("ID", index_EV9000DB);
        m_pEV9000MyOper->GetFieldValue("sql_str", str_EV9000DB);

        m_pEV9000LOGOper->DB_Insert("", "", str_EV9000DB.c_str());

        strSQL.clear();
        memset(strtmpIndex,0,100);
        strSQL = "delete from table_EV9000LOG";
        strSQL += " WHERE ID = ";
        sprintf(strtmpIndex, "%d", index_EV9000DB);
        strSQL += strtmpIndex;
        
        m_pEV9000MyOper->DB_Delete(strSQL.c_str());
    }
    
}
#endif


