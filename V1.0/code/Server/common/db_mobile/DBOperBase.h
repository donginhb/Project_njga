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

#define MAX_QUERY_NUM    100  //ÿ����ѯ����

//ͳһ������
/* 0X F8XX XXXX	���ݿ� */
#define DB_ERR_OK                       0
#define DB_ERR_BASE                     0XF8000000     //����ֵ

#define DB_ERR_ICE                      0XF8010000     //ICE����
#define DB_ERR_DATABUS                  0XF8020000     //databus���ش���
#define DB_ERR_BDB                      0XF8040000     //BDB����
#define DB_ERR_MYSQL                    0XF8080000     //Mysql����
#define DB_ERR_OTHER                    0XF8100000     //��������

//ICE
#define DB_ERR_ICE_EXCEPTION           (DB_ERR_ICE | 1)

//DATABUS
#define DB_ERR_DATABUS_DBNOTEXIST      (DB_ERR_DATABUS | 1)
#define DB_ERR_DATABUS_FIELDNOTEXIST   (DB_ERR_DATABUS | 2)
#define DB_ERR_DATABUS_NOTGETREADY     (DB_ERR_DATABUS | 4)
#define DB_ERR_DATABUS_SQLEXCEPTION    (DB_ERR_DATABUS | 8)

//BDB

//Mysql  ����cppmysql��Ĵ���
#define DB_ERR_MYSQL_TRANSFAIL         (DB_ERR_MYSQL | 1)     //��ʼ����ʧ��
#define DB_ERR_MYSQL_COMMITFAIL        (DB_ERR_MYSQL | 2)     //�ύ����ʧ��
#define DB_ERR_MYSQL_ROLLBACKFAIL      (DB_ERR_MYSQL | 3)     //�ع�����ʧ��
#define DB_ERR_MYSQL_OTHER             (DB_ERR_MYSQL | 4)     //����mysqlʧ��

//OTHER
#define DB_ERR_OTHER_UNCONNECT         (DB_ERR_OTHER | 1)
#define DB_ERR_OTHER_STRISNULL         (DB_ERR_OTHER | 2)     //�ַ���Ϊ��
#define DB_ERR_OTHER_OVERMAXROW        (DB_ERR_OTHER | 3)    //�кų�����Χ
#define DB_ERR_OTHER_MOVENEXTFAIL      (DB_ERR_OTHER | 4)   //movenext ʧ��
#define DB_ERR_OTHER_GETVALUEFAIL      (DB_ERR_OTHER | 5)    //ȡֵʧ��
#define DB_ERR_OTHER_FIELDNUMERROR     (DB_ERR_OTHER | 6)
#define DB_ERR_OTHER_PARA              (DB_ERR_MYSQL | 7)     //��������
#define DB_ERR_OTHER_CONFAIL           (DB_ERR_MYSQL | 8)     //����ʧ��
#define DB_ERR_OHTER_SQLNULL           (DB_ERR_MYSQL | 9)     //sqlΪ��
#define DB_ERR_OHTER_FILENAMENOT       (DB_ERR_MYSQL | 11)     //sqlΪ��
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



// ***BEGIN***  �������ݿ��쳣�������� wangqichao 2013/6/17 add
#define DB_ERROR_CODE         DB_ERR_BASE-100
#define DB_WRETURN_CODE       DB_ERR_BASE-101
const int MAXTIMENUM = 5;
const int CharNum = 500;
const int MAXGetNum = 10;
const int MAXSetNum = 10;
const int MAXWITEMNUM = 1000;
// ***END***  �������ݿ��쳣�������� wangqichao 2013/6/17 add

typedef struct _dbErrorInfo
{
    unsigned int   iErrorCode;    //������
    string         strErrorMsg;   //����˵��
} DBERRORINFO;

class DBOperBase  
{
public:
    DBOperBase()
    {
        m_pciceConPara =NULL;
        m_pcdbPath =NULL;
        // ***BEGIN***  �������ݿ��쳣�������� wangqichao 2013/6/17 add
        m_pciceConPara = new char[CharNum + 1];
        memset(m_pciceConPara, 0, CharNum);
        m_pcdbPath = new char[CharNum + 1];
        memset(m_pcdbPath, 0, CharNum);
        // ***END***  �������ݿ��쳣�������� wangqichao 2013/6/17 add
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

    virtual int  Connect(const char* szConPara, const char* dbPath, int iIceTimeOut = 3)=0; //����ʱ���ã�����Ice::databus���ݿ������
    virtual int  DBExecute(const char* sql)=0;    // ִ�в��� int DBExecute(const char * sql,int &nExecRet);
    virtual int  DBQuery(const char* sql)=0;  // ִ�в�ѯ���
    virtual int  DBQueryNext(){return 0;}  // ִ�в�ѯ��һ��

    virtual void FreeResult()=0; // �ͷŲ�ѯ�Ľ����
    virtual void Close()=0; // �ر�����
    virtual int  MoveNext()=0;  //����һ����¼

    //----��������ӿ�--begin------
    virtual int  BeginTransaction()=0;  //��������
    virtual int  Commit()=0;            //�ύ����
    virtual int  RollBack()=0;          //�ع�����
    //----��������ӿ�--end------

    //-------��ADO�ӿ�---begin---------
    //  ���ָ���к�ָ���е�����ֵ
    //  ���ж���0��ʼ���
    //  row  ���к�
    //  fd_name ��������
    //  fd_num  �����ֶκ�
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
    //-------��ADO�ӿ�---end-----------
    virtual CppMySQLQuery& getMySQLQueryRes()=0;
    

    WBOOL GetDBStatus(){return Status;}

    virtual int          GetLastOpErrorCode(){return m_iLastOpErrorCode;};
    virtual const char*  GetLastOpErrorMsg()
    {
        unsigned int dwErrorType=0;
        dwErrorType =  ((m_iLastOpErrorCode & 0X00040000) ||(m_iLastOpErrorCode & 0X00080000)); //BDB/mysql���ݿ��ڲ�����
        if(dwErrorType) //DBD/mysql����
        {
            return m_tLastDbErrorInfo.strErrorMsg.c_str();
        }
        else
        {
            switch (m_iLastOpErrorCode)
            {
            case DB_ERR_OK               :
                return "���ݿ�����ɹ�";

            case DB_ERR_DATABUS_DBNOTEXIST :
                return "���ݿⲻ����";

            case DB_ERR_DATABUS_FIELDNOTEXIST:
                return "�ֶβ�����";

            case DB_ERR_DATABUS_NOTGETREADY  :
                return "���ݿ�û��׼����";

            case DB_ERR_DATABUS_SQLEXCEPTION :
                return "SQLִ���쳣";

            case DB_ERR_ICE_EXCEPTION        :
                return "ICEͨ���쳣";

            case DB_ERR_OTHER_UNCONNECT      :
                return "��û�д����ݿ�����";

            case DB_ERR_OTHER_STRISNULL      :
                return "�����ַ���Ϊ��";

            case DB_ERR_OTHER_OVERMAXROW     :
                return "�ѳ�����¼���������";

            case DB_ERR_OTHER_MOVENEXTFAIL   :
                return "���Ƽ�¼�쳣";

            case DB_ERR_OTHER_GETVALUEFAIL   :
                return "ȡ�ֶ�ֵʧ��";
                
            case DB_ERR_OTHER_FIELDNUMERROR  :
                return "�ֶα�Ŵ���";

            default:
                return "���ݿ�ִ��ʧ��";
          }
         }
        return "";
     }

    //���ݿ������Ϣ
    virtual int          GetLastDbErrorCode(){return GetLastOpErrorCode();};
    virtual const char*  GetLastDbErrorMsg(){return GetLastOpErrorMsg();};
    /*****************************************************************************
	Prototype    : safeCopyStr
	Description  : ���ݿ��ַ�����������
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
        // ֻ��Ŀ�껺����ָ�벻Ϊ�ղ��ܿ���
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
    WBOOL         Status;              //����״̬
    char*        m_pciceConPara;
    char*        m_pcdbPath;
    //int          m_iIceTimeOut;       //Ice ��ʱ�趨  ��λs
    int                         m_iLastOpErrorCode;  //���������룬EV9000��׼������
    int                         m_iIceTimeOut;       //Ice ��ʱ�趨  ��λs
    DBERRORINFO                 m_tLastDbErrorInfo;  //���ݿ����������Ϣ��DBD������
};

#endif // !defined(AFX_DBOPERBASE_H__720058DD_AF8E_4251_B537_AFBB5E1CB9EA__INCLUDED_)

#endif
