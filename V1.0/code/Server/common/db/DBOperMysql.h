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

    int  Connect(const char* szConPara, const char* dbPath, int iTimeOut = 3); //����MySql������
    int  DBExecute(const char* sql);      //ִ�в��� int DBExecute(const char * sql,int &nExecRet);
    int  DBQuery(const char* sql);  // ִ�в�ѯ���
    //int  DBQueryNext(); // ִ�в�ѯ��һ��
    void FreeResult(); // �ͷŲ�ѯ�Ľ����
    void Close(); // �ر�����
    int  MoveNext();  //����һ����¼

    int  GetAffectedRows();
    //----------------------
    //----��������ӿ�--begin------
    int  BeginTransaction();  //��������
    int  Commit();            //�ύ����
    int  RollBack();          //�ع�����
    //----��������ӿ�--end------

    //-------��ADO�ӿ�---begin---------
    //  ���ָ���к�ָ���е�����ֵ
    //  ���ж���0��ʼ���
    //  row  ���к�
    //  fd_name ��������
    //  fd_num  �����ֶκ�
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
    //-------��ADO�ӿ�---end-----------
    CppMySQLQuery& getMySQLQueryRes();

private:
    bool ReConnect();       //�������ݿ�
    void StrSplit(const string &str,const char delimter, vector<string> &strList);
private:

    int           m_iAffectedRows;    // DBExecuteִ��ʱ����Ӱ������

    //���Ӳ���
    string        m_strHost;
    string        m_strUser;
    string        m_strPwd;
    string        m_strDBName;
    int           m_iPort;
    int           m_iTimeOut;
    int           m_nTotalNum;         //��¼������
    int           m_nCurPos;           //��ǰ��¼λ��
    
    CppMySQL3DB   *m_pdb;              //���ݿ������
    CppMySQLQuery m_oQueryRes;            //��ѯ�����
    //wangqichao add �ֺ�
    char m_sql_str[4048];
};

#endif // !defined(AFX_DBOPERMYSQL_H__DE8FE54F_0392_4A77_B167_0E92961753EC__INCLUDED_)
