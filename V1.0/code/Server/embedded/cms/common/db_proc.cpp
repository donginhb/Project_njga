/******************************************************************************

                  ��Ȩ���� (C), 2001-2011, ������Ѷ�������޹�˾

 ******************************************************************************
  �� �� ��   : db_proc.cpp
  �� �� ��   : ����
  ��    ��   : ���
  ��������   : 2014��2��17�� ����һ
  ����޸�   :
  ��������   : ���ݿ⹫������ģ��
  �����б�   :
              ClosedDB
              db_connect
              db_connect_tmp
              db_createTable
              db_createTable_tmp
              db_gDBOperconnect
              db_gDBOperconnect_tmp
              db_gDBOperconnect_tmp_dbsql
              InitAllGBDeviceRegStatus
              set_db_data_to_cruise_srv_list
              set_db_data_to_Device_info_list
              set_db_data_to_LogicDevice_info_list
              set_db_data_to_plan_srv_list
              set_db_data_to_poll_srv_list
              set_db_data_to_record_info_list
              set_db_data_to_route_info_list
              set_db_data_to_tsu_info_list
              UpdateAllGBDeviceRegStatus2DB
              UpdateAllGBLogicDeviceRegStatus2DB
              UpdateAllUserRegStatus2DB
              WriteErrorInfo2GBPhyDeviceTmpDB
  �޸���ʷ   :
  1.��    ��   : 2014��2��17�� ����һ
    ��    ��   : ���
    �޸�����   : �����ļ�

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#include <string>

#ifdef WIN32
#include <winsock.h>
#include <sys/types.h>
#else
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <errno.h>

#include <asm/types.h>
#include <netinet/ether.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#endif

#include "db_proc.h"

#include "common/log_proc.inc"
#include "common/gblfunc_proc.inc"
#include "common/gblconfig_proc.inc"

#include "platformms/BoardInit.h"

#include "user/user_info_mgn.inc"
#include "device/device_info_mgn.inc"
#include "device/device_srv_proc.inc"
#include "record/record_info_mgn.inc"
#include "route/route_info_mgn.inc"
#include "route/platform_info_mgn.inc"
#include "resource/resource_info_mgn.inc"

#include "service/poll_srv_proc.inc"
#include "service/plan_srv_proc.inc"
#include "service/cruise_srv_proc.inc"
#include "service/preset_proc.inc"
#include "service/compress_task_proc.inc"

/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern BOARD_NET_ATTR  g_BoardNetConfig;
extern gbl_conf_t* pGblconf;              /* ȫ��������Ϣ */
//����TSU�ϱ�
extern unsigned int g_TSUUpLoadFileFlag;  /* TSU�Ƿ���Ҫ�����ϱ�¼���ļ���ʶ */
extern int g_MMSEnableFlag;               /* SX��RX�Ƿ�����MMS���ܣ�Ĭ������ */
extern int cms_run_status;                /* 0:û������,1:�������� */

/*----------------------------------------------*
 * �ⲿ����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ȫ�ֱ���                                     *
 *----------------------------------------------*/
DBOper g_DBOper;
DBOper g_LogDBOper;
int g_LogDBOperConnectStatus = 0;

DB_Thread* g_DBThread;
char g_strDBThreadPara[4][100] = {{0}, {0}, {0}, {0}};
char g_StrCon[2][100] = {{0}, {0}};
char g_StrConLog[2][100] = {{0}, {0}};

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/
/* �������ݵ����ݿ��ļ��޸�ʱ��,�����ж����ݿ��Ƿ��б仯 */
time_t db_file_AlarmDeployment_lmt;                 /* AlarmDeployment���ݿ��ļ����޸�ʱ�� */
time_t db_file_CruiseActionConfig_lmt;              /* CruiseActionConfig���ݿ��ļ����޸�ʱ�� */
time_t db_file_CruiseConfig_lmt;                    /* CruiseConfig���ݿ��ļ����޸�ʱ�� */
time_t db_file_GBLogicDeviceConfig_lmt;             /* GBLogicDeviceConfig���ݿ��ļ����޸�ʱ�� */
time_t db_file_GBPhyDeviceConfig_lmt;               /* GBPhyDeviceConfig���ݿ��ļ����޸�ʱ�� */
time_t db_file_LogicDeviceGroupConfig_lmt;          /* LogicDeviceGroupConfig���ݿ��ļ����޸�ʱ�� */
time_t db_file_LogicDeviceMapGroupConfig_lmt;       /* LogicDeviceMapGroupConfig���ݿ��ļ����޸�ʱ�� */
time_t db_file_PlanActionConfig_lmt;                /* PlanActionConfig���ݿ��ļ����޸�ʱ�� */
time_t db_file_PlanConfig_lmt;                      /* PlanConfig���ݿ��ļ����޸�ʱ�� */
time_t db_file_PollActionConfig_lmt;                /* PollActionConfig���ݿ��ļ����޸�ʱ�� */
time_t db_file_PollConfig_lmt;                      /* PollConfig���ݿ��ļ����޸�ʱ�� */
time_t db_file_RecordSchedConfig_lmt;               /* RecordSchedConfig���ݿ��ļ����޸�ʱ�� */
time_t db_file_RecordTimeSchedConfig_lmt;           /* RecordTimeSchedConfig���ݿ��ļ����޸�ʱ�� */
time_t db_file_RouteNetConfig_lmt;                  /* RouteNetConfig���ݿ��ļ����޸�ʱ�� */
time_t db_file_DiBiaoUploadPicMapConfig_lmt;        /* DiBiaoUploadPicMapConfig���ݿ��ļ����޸�ʱ�� */

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

int BakUpDateBase()
{
    system("mysql -uroot -pzbitcloud EV9000DB < /data/TMPEV9000DBF.sql");
    return 0;
}

void DBThreadNew()
{
    g_DBThread = new DB_Thread(g_strDBThreadPara, "");

    return;
}

void DBThreadDelete()
{
    if (g_DBThread)
    {
        try
        {
            delete g_DBThread;
            g_DBThread = NULL;
        }
        catch (...)
        {
            g_DBThread = NULL;
        }

        g_DBThread = NULL;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : ClosedDB
 ��������  : �ر����ݿ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��17�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int ClosedDB()
{
    g_DBOper.Close();
    g_LogDBOper.Close();
    return 0;
}

/*****************************************************************************
 �� �� ��  : db_gDBOperconnect
 ��������  : ȫ�����ݿ�����
 �������  : string strCon
             string strDb
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��23�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int db_gDBOperconnect()
{
    int iRet = g_DBOper.Connect(g_StrCon, (char*)"");

    printf("db_gDBOperconnect() g_DBOper Connect: iRet=%d\r\n", iRet);

    if (0 != iRet)
    {
        return iRet;
    }

	/* ��־���ݿ����� */
    iRet = g_LogDBOper.Connect(g_StrConLog, (char*)"");

    printf("db_gDBOperconnect() g_LogDBOper Connect: iRet=%d\r\n", iRet);

    if (0 != iRet)
    {
	    g_LogDBOperConnectStatus = 0;
        return 0; /* ��־���ݿ�û�����ӳɹ��������˳� */
    }

    g_LogDBOperConnectStatus = 1;

    return iRet;
}

/*****************************************************************************
 �� �� ��  : db_connect
 ��������  : ���ݿ�����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��23�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int db_connect()
{
    if (db_gDBOperconnect() < 0)
    {
        return -1;
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : load_device_group_cfg_from_db
 ��������  : �����ݿ��м��ط�����Ϣ
 �������  : DBOper* pDbOper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��9��2��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int load_device_group_cfg_from_db(DBOper* pDbOper)
{
    int i = 0;
    string strSQL = "";

    int record_count = 0;
    int while_count = 0;

    if (NULL == pDbOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "load_device_group_cfg_from_db() exit---: Param Error \r\n");
        return -1;
    }

    /* ��ȡ�������� */
    /* ��ѯ����� */
    strSQL.clear();
    strSQL = "SELECT * FROM LogicDeviceGroupConfig WHERE OtherRealm=0 GROUP BY GroupID";

    record_count = pDbOper->DB_Select(strSQL.c_str(), 1);

    printf("load_device_group_cfg_from_db() DB Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "load_device_group_cfg_from_db() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "load_device_group_cfg_from_db() ErrorMsg=%s\r\n", pDbOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "load_device_group_cfg_from_db() DB No Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        return -1;
    }

    printf("\r\n load_device_group_cfg_from_db() DB Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);

    /* ѭ����ȡ���ݿ����� */
    do
    {
        string strName = "";
        string strGroupID = "";
        int iSortID = 0;
        string strParentID = "";

        string strCivilCode = "";  /* ����������� */
        string strGroupCode = "";  /* ������֯���� */
        string strParentCode = ""; /* �ϼ���֯���� */

        int iNeedToUpLoad = 0;

        char pcTmpGroupCode[24] = {0};

        char strGroupCodeTmp1[3] = {0}; /* ʡ������ */
        char strGroupCodeTmp2[3] = {0}; /* �м����� */
        char strGroupCodeTmp3[3] = {0}; /* �������� */
        char strGroupCodeTmp4[3] = {0}; /* �ɳ������� */
        char strGroupCodeTmp5[3] = {0}; /* ������֯1���� */
        char strGroupCodeTmp6[3] = {0}; /* ������֯2���� */
        char strGroupCodeTmp7[3] = {0}; /* ������֯3���� */
        char strGroupCodeTmp8[3] = {0}; /* ������֯4���� */

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "load_device_group_cfg_from_db() While Count=%d \r\n", while_count);
        }

        /* ���� */
        strGroupID.clear();
        pDbOper->GetFieldValue("GroupID", strGroupID);

        /* ������ */
        strName.clear();
        pDbOper->GetFieldValue("Name", strName);

        /* ͬһ���ڵ����������� */
        iSortID = 0;
        pDbOper->GetFieldValue("SortID", iSortID);

        /* ���ڵ��� */
        strParentID.clear();
        pDbOper->GetFieldValue("ParentID", strParentID);


        if (strGroupID.empty())
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "load_device_group_cfg_from_db() GroupID Empty\r\n");
            continue;
        }

        if (strName.empty())
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "load_device_group_cfg_from_db() Name Empty\r\n");
            continue;
        }

        /* ����ID */
        osip_strncpy(pcTmpGroupCode, (char*)strGroupID.c_str(), 20);

        osip_strncpy(strGroupCodeTmp1, pcTmpGroupCode, 2);
        osip_strncpy(strGroupCodeTmp2, &pcTmpGroupCode[2], 2);
        osip_strncpy(strGroupCodeTmp3, &pcTmpGroupCode[4], 2);
        osip_strncpy(strGroupCodeTmp4, &pcTmpGroupCode[6], 2);
        osip_strncpy(strGroupCodeTmp5, &pcTmpGroupCode[8], 2);
        osip_strncpy(strGroupCodeTmp6, &pcTmpGroupCode[10], 2);
        osip_strncpy(strGroupCodeTmp7, &pcTmpGroupCode[12], 2);
        osip_strncpy(strGroupCodeTmp8, &pcTmpGroupCode[14], 2);

        if (0 != sstrcmp(strGroupCodeTmp1, (char*)"00")
            && 0 == sstrcmp(strGroupCodeTmp2, (char*)"00")
            && 0 == sstrcmp(strGroupCodeTmp3, (char*)"00")
            && 0 == sstrcmp(strGroupCodeTmp4, (char*)"00")
            && 0 == sstrcmp(strGroupCodeTmp5, (char*)"00")) /* ʡ������ */
        {
            /* ����������� */
            strCivilCode = strGroupCodeTmp1;

            /* �ϱ��ķ������ */
            strGroupCode = strGroupCodeTmp1;

            /* �ϼ���֯���� */
            strParentCode = "";
        }
        else if (0 != sstrcmp(strGroupCodeTmp1, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp2, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp3, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp4, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp5, (char*)"00")) /* ʡ������ֱ������ */
        {
            /* ����������� */
            strCivilCode = strGroupCodeTmp1;

            /* �ϱ��ķ������ */
            strGroupCode = strGroupCodeTmp1;
            strGroupCode += (char*)"00000000";
            strGroupCode += (char*)"2160";
            strGroupCode += strGroupCodeTmp5;
            strGroupCode += strGroupCodeTmp6;
            strGroupCode += strGroupCodeTmp7;

            /* �ϼ���֯���� */
            if (0 == sstrcmp(strGroupCodeTmp6, (char*)"00")
                && 0 == sstrcmp(strGroupCodeTmp7, (char*)"00"))

            {
                strParentCode = strGroupCodeTmp1;
            }
            else if (0 != sstrcmp(strGroupCodeTmp6, (char*)"00")
                     && 0 == sstrcmp(strGroupCodeTmp7, (char*)"00"))

            {
                strParentCode = strGroupCodeTmp1;
                strParentCode += (char*)"00000000";
                strParentCode += (char*)"2160";
                strParentCode += strGroupCodeTmp5;
                strParentCode += (char*)"00";
                strParentCode += (char*)"00";
            }
            else if (0 != sstrcmp(strGroupCodeTmp6, (char*)"00")
                     && 0 != sstrcmp(strGroupCodeTmp7, (char*)"00"))

            {
                strParentCode = strGroupCodeTmp1;
                strParentCode += (char*)"00000000";
                strParentCode += (char*)"2160";
                strParentCode += strGroupCodeTmp5;
                strParentCode += strGroupCodeTmp6;
                strParentCode += (char*)"00";
            }
        }
        else if (0 != sstrcmp(strGroupCodeTmp1, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp2, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp3, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp4, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp5, (char*)"00")) /* �м����� */
        {
            /* ����������� */
            strCivilCode = strGroupCodeTmp1;
            strCivilCode += strGroupCodeTmp2;

            /* �ϱ��ķ������ */
            strGroupCode = strGroupCodeTmp1;
            strGroupCode += strGroupCodeTmp2;

            /* �ϼ���֯���� */
            strParentCode = "";
        }
        else if (0 != sstrcmp(strGroupCodeTmp1, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp2, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp3, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp4, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp5, (char*)"00")) /* �м�����ֱ������ */
        {
            /* ����������� */
            strCivilCode = strGroupCodeTmp1;
            strCivilCode += strGroupCodeTmp2;

            /* �ϱ��ķ������ */
            strGroupCode = strGroupCodeTmp1;
            strGroupCode += strGroupCodeTmp2;
            strGroupCode += (char*)"000000";
            strGroupCode += (char*)"2160";
            strGroupCode += strGroupCodeTmp5;
            strGroupCode += strGroupCodeTmp6;
            strGroupCode += strGroupCodeTmp7;

            /* �ϼ���֯���� */
            if (0 == sstrcmp(strGroupCodeTmp6, (char*)"00")
                && 0 == sstrcmp(strGroupCodeTmp7, (char*)"00"))

            {
                strParentCode = strGroupCodeTmp1;
                strParentCode += strGroupCodeTmp2;
            }
            else if (0 != sstrcmp(strGroupCodeTmp6, (char*)"00")
                     && 0 == sstrcmp(strGroupCodeTmp7, (char*)"00"))

            {
                strParentCode = strGroupCodeTmp1;
                strParentCode += strGroupCodeTmp2;
                strParentCode += (char*)"000000";
                strParentCode += (char*)"2160";
                strParentCode += strGroupCodeTmp5;
                strParentCode += (char*)"00";
                strParentCode += (char*)"00";
            }
            else if (0 != sstrcmp(strGroupCodeTmp6, (char*)"00")
                     && 0 != sstrcmp(strGroupCodeTmp7, (char*)"00"))

            {
                strParentCode = strGroupCodeTmp1;
                strParentCode += strGroupCodeTmp2;
                strParentCode += (char*)"000000";
                strParentCode += (char*)"2160";
                strParentCode += strGroupCodeTmp5;
                strParentCode += strGroupCodeTmp6;
                strParentCode += (char*)"00";
            }
        }
        else if (0 != sstrcmp(strGroupCodeTmp1, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp2, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp3, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp4, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp5, (char*)"00")) /* �������� */
        {
            /* ����������� */
            strCivilCode = strGroupCodeTmp1;
            strCivilCode += strGroupCodeTmp2;
            strCivilCode += strGroupCodeTmp3;

            /* �ϱ��ķ������ */
            strGroupCode = strGroupCodeTmp1;
            strGroupCode += strGroupCodeTmp2;
            strGroupCode += strGroupCodeTmp3;

            /* �ϼ���֯���� */
            strParentCode = "";
        }
        else if (0 != sstrcmp(strGroupCodeTmp1, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp2, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp3, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp4, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp5, (char*)"00")) /* ��������ֱ������ */
        {
            /* ����������� */
            strCivilCode = strGroupCodeTmp1;
            strCivilCode += strGroupCodeTmp2;
            strCivilCode += strGroupCodeTmp3;

            /* �ϱ��ķ������ */
            strGroupCode = strGroupCodeTmp1;
            strGroupCode += strGroupCodeTmp2;
            strGroupCode += strGroupCodeTmp3;
            strGroupCode += (char*)"0000";
            strGroupCode += (char*)"2160";
            strGroupCode += strGroupCodeTmp5;
            strGroupCode += strGroupCodeTmp6;
            strGroupCode += strGroupCodeTmp7;

            /* �ϼ���֯���� */
            if (0 == sstrcmp(strGroupCodeTmp6, (char*)"00")
                && 0 == sstrcmp(strGroupCodeTmp7, (char*)"00"))

            {
                strParentCode = strGroupCodeTmp1;
                strParentCode += strGroupCodeTmp2;
                strParentCode += strGroupCodeTmp3;
            }
            else if (0 != sstrcmp(strGroupCodeTmp6, (char*)"00")
                     && 0 == sstrcmp(strGroupCodeTmp7, (char*)"00"))

            {
                strParentCode = strGroupCodeTmp1;
                strParentCode += strGroupCodeTmp2;
                strParentCode += strGroupCodeTmp3;
                strParentCode += (char*)"0000";
                strParentCode += (char*)"2160";
                strParentCode += strGroupCodeTmp5;
                strParentCode += (char*)"00";
                strParentCode += (char*)"00";
            }
            else if (0 != sstrcmp(strGroupCodeTmp6, (char*)"00")
                     && 0 != sstrcmp(strGroupCodeTmp7, (char*)"00"))

            {
                strParentCode = strGroupCodeTmp1;
                strParentCode += strGroupCodeTmp2;
                strParentCode += strGroupCodeTmp3;
                strParentCode += (char*)"0000";
                strParentCode += (char*)"2160";
                strParentCode += strGroupCodeTmp5;
                strParentCode += strGroupCodeTmp6;
                strParentCode += (char*)"00";
            }
        }
        else if (0 != sstrcmp(strGroupCodeTmp1, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp2, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp3, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp4, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp5, (char*)"00")) /* �ɳ������� */
        {
            /* ����������� */
            strCivilCode = strGroupCodeTmp1;
            strCivilCode += strGroupCodeTmp2;
            strCivilCode += strGroupCodeTmp3;
            strCivilCode += strGroupCodeTmp4;

            /* �ϱ��ķ������ */
            strGroupCode = strGroupCodeTmp1;
            strGroupCode += strGroupCodeTmp2;
            strGroupCode += strGroupCodeTmp3;
            strGroupCode += strGroupCodeTmp4;

            /* �ϼ���֯���� */
            strParentCode = "";
        }
        else if (0 != sstrcmp(strGroupCodeTmp1, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp2, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp3, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp4, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp5, (char*)"00")) /* �ɳ�������ֱ������ */
        {
            /* ����������� */
            strCivilCode = strGroupCodeTmp1;
            strCivilCode += strGroupCodeTmp2;
            strCivilCode += strGroupCodeTmp3;
            strCivilCode += strGroupCodeTmp4;

            /* �ϱ��ķ������ */
            strGroupCode = strGroupCodeTmp1;
            strGroupCode += strGroupCodeTmp2;
            strGroupCode += strGroupCodeTmp3;
            strGroupCode += strGroupCodeTmp4;
            strGroupCode += (char*)"00";
            strGroupCode += (char*)"2160";
            strGroupCode += strGroupCodeTmp5;
            strGroupCode += strGroupCodeTmp6;
            strGroupCode += strGroupCodeTmp7;

            /* �ϼ���֯���� */
            if (0 == sstrcmp(strGroupCodeTmp6, (char*)"00")
                && 0 == sstrcmp(strGroupCodeTmp7, (char*)"00"))

            {
                strParentCode = strGroupCodeTmp1;
                strParentCode += strGroupCodeTmp2;
                strParentCode += strGroupCodeTmp3;
                strParentCode += strGroupCodeTmp4;
            }
            else if (0 != sstrcmp(strGroupCodeTmp6, (char*)"00")
                     && 0 == sstrcmp(strGroupCodeTmp7, (char*)"00"))

            {
                strParentCode = strGroupCodeTmp1;
                strParentCode += strGroupCodeTmp2;
                strParentCode += strGroupCodeTmp3;
                strParentCode += strGroupCodeTmp4;
                strParentCode += (char*)"00";
                strParentCode += (char*)"2160";
                strParentCode += strGroupCodeTmp5;
                strParentCode += (char*)"00";
                strParentCode += (char*)"00";
            }
            else if (0 != sstrcmp(strGroupCodeTmp6, (char*)"00")
                     && 0 != sstrcmp(strGroupCodeTmp7, (char*)"00"))

            {
                strParentCode = strGroupCodeTmp1;
                strParentCode += strGroupCodeTmp2;
                strParentCode += strGroupCodeTmp3;
                strParentCode += strGroupCodeTmp4;
                strParentCode += (char*)"00";
                strParentCode += (char*)"2160";
                strParentCode += strGroupCodeTmp5;
                strParentCode += strGroupCodeTmp6;
                strParentCode += (char*)"00";
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "load_device_group_cfg_from_db() GroupID=%s, Name=%s Not Correct, Skipping \r\n", (char*)strGroupID.c_str(), (char*)strName.c_str());
            continue;
        }

        if (0 != sstrcmp(strGroupCodeTmp8, (char*)"00")) /* ����������������֯��ʱ���ϱ����ϼ� */
        {
            iNeedToUpLoad = 0;
        }
        else
        {
            iNeedToUpLoad = 1;
        }

        if (!strParentCode.empty())
        {
            /* ���뵽������ */
            if (!strParentID.empty())
            {
                i = primary_group_add((char*)strGroupID.c_str(), (char*)strName.c_str(), (char*)strCivilCode.c_str(), (char*)strGroupCode.c_str(), (char*)strParentCode.c_str(), iSortID, (char*)strParentID.c_str(), iNeedToUpLoad);
            }
            else
            {
                i = primary_group_add((char*)strGroupID.c_str(), (char*)strName.c_str(), (char*)strCivilCode.c_str(), (char*)strGroupCode.c_str(), (char*)strParentCode.c_str(), iSortID, NULL, iNeedToUpLoad);
            }
        }
        else
        {
            /* ���뵽������ */
            if (!strParentID.empty())
            {
                i = primary_group_add((char*)strGroupID.c_str(), (char*)strName.c_str(), (char*)strCivilCode.c_str(), (char*)strGroupCode.c_str(), NULL, iSortID, (char*)strParentID.c_str(), iNeedToUpLoad);
            }
            else
            {
                i = primary_group_add((char*)strGroupID.c_str(), (char*)strName.c_str(), (char*)strCivilCode.c_str(), (char*)strGroupCode.c_str(), NULL, iSortID, NULL, iNeedToUpLoad);
            }
        }

        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "load_device_group_cfg_from_db() primary_group_add:GroupID=%s, Name=%s, CivilCode=%s, GroupCode=%s, ParentCode=%s, NeedToUpLoad=%d, i=%d \r\n", (char*)strGroupID.c_str(), (char*)strName.c_str(), (char*)strCivilCode.c_str(), (char*)strGroupCode.c_str(), (char*)strParentCode.c_str(), iNeedToUpLoad, i);
    }
    while (pDbOper->MoveNext() >= 0);

    return 0;
}

/*****************************************************************************
 �� �� ��  : load_device_group_map_cfg_from_db
 ��������  : �����߼��豸�����ϵ
 �������  : DBOper* pDbOper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int load_device_group_map_cfg_from_db(DBOper* pDbOper)
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;

    if (NULL == pDbOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "load_device_group_map_cfg_from_db() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from LogicDeviceMapGroupConfig";

    record_count = pDbOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "load_device_group_map_cfg_from_db() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "load_device_group_map_cfg_from_db() ErrorMsg=%s\r\n", pDbOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "load_device_group_map_cfg_from_db() DB No Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        return -1;
    }

    printf("load_device_group_map_cfg_from_db() DB Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);

    do
    {
        string strGroupID = "";
        unsigned int uDeviceIndex = 0;
        int iSortID = 0;

        pDbOper->GetFieldValue("GroupID", strGroupID);
        pDbOper->GetFieldValue("DeviceIndex", uDeviceIndex);
        pDbOper->GetFieldValue("SortID", iSortID);

        /* ��ӵ����� */
        ret = device_map_group_add((char*)strGroupID.c_str(), uDeviceIndex, iSortID);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "load_device_group_map_cfg_from_db() device_map_group_add:GroupID=%s, DeviceIndex=%u, SortID=%d, i=%d", (char*)strGroupID.c_str(), uDeviceIndex, iSortID, ret);
    }
    while (pDbOper->MoveNext() >= 0);

    return 0;
}

/*****************************************************************************
 �� �� ��  : set_db_data_to_tsu_info_list
 ��������  : ����TSU��������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��14�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int set_db_data_to_tsu_info_list()
{
    int tsu_pos = -1;
    int record_count = -1;
    string strSQL = "";
    char strBoardType[16] = {0};

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
    \r\n |set_db_data_to_tsu_info_list:BEGIN \
    \r\n ********************************************** \r\n");

    strSQL.clear();
    strSQL = "select * from BoardConfig";
    strSQL += " WHERE BoardType = ";
    snprintf(strBoardType, 16, "%d", LOGIC_BOARD_TSU);
    strSQL += strBoardType;

    record_count = g_DBOper.DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_tsu_info_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_tsu_info_list() ErrorMsg=%s\r\n", g_DBOper.GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_tsu_info_list() exit---: No Record Count \r\n");
        return 0;
    }

    printf("\r\n set_db_data_to_tsu_info_list() DB Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_tsu_info_list() Load TSU Info: count=%d", record_count);

        int iID = 0;
        string strBoardID = "";
        int iAssignRecord = 0;

        g_DBOper.GetFieldValue("ID", iID);
        g_DBOper.GetFieldValue("BoardID", strBoardID);
        g_DBOper.GetFieldValue("AssignRecord", iAssignRecord);

        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_tsu_info_list() ID=%d, BoardID=%s, AssignRecord=%d", iID, strBoardID.c_str(), iAssignRecord);

        if (strBoardID.empty())
        {
            continue;
        }

        /* ��ӵ�TSU ���� */
        tsu_pos = tsu_resource_info_find((char*)strBoardID.c_str());

        if (tsu_pos < 0)
        {
            tsu_pos = tsu_resource_info_add((char*)strBoardID.c_str(), iID, iAssignRecord);
        }
    }
    while (g_DBOper.MoveNext() >= 0);

    printf("set_db_data_to_tsu_info_list() Load TSU Info: count=%d \r\n", record_count);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
    \r\n |set_db_data_to_tsu_info_list:END \
    \r\n ********************************************** \r\n");

    return 0;
}

/*****************************************************************************
 Prototype    : set_db_data_to_Device_info_list
 Description  : �������ݿ�����
 Input        : None
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int set_db_data_to_Device_info_list()
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_Device_info_list:BEGIN \
		\r\n ********************************************** \r\n");

    strSQL.clear();
    strSQL = "select * from GBPhyDeviceConfig WHERE Enable = 1";

    record_count = g_DBOper.DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_Device_info_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_Device_info_list() ErrorMsg=%s\r\n", g_DBOper.GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_Device_info_list() exit---: No Record Count \r\n");
        return 0;
    }

    printf("\r\n set_db_data_to_Device_info_list() DB Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() Load GBDevice info: count=%d", record_count);

        GBDevice_info_t* pGBDeviceInfo = NULL;
        int i_ret = GBDevice_info_init(&pGBDeviceInfo);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_Device_info_list() GBDevice_info_init:i_ret=%d", i_ret);
            continue;
        }

        int tmp_ivalue = 0;
        string tmp_svalue;

        /* �豸����*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_ivalue);

        pGBDeviceInfo->id = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->id:%d", pGBDeviceInfo->id);


        /* �豸ͳһ��� */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("DeviceID", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBDeviceInfo->device_id, tmp_svalue.c_str(), MAX_ID_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->device_id:%s", pGBDeviceInfo->device_id);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->device_id NULL");
        }


        /* IP��ַ */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("DeviceIP", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBDeviceInfo->login_ip, tmp_svalue.c_str(), MAX_ID_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->login_ip:%s", pGBDeviceInfo->login_ip);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->login_ip NULL");
        }


        /* �豸����(ǰ���������NVR������CMS��TSU) */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("DeviceType", tmp_ivalue);

        pGBDeviceInfo->device_type = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->device_type:%d \r\n", pGBDeviceInfo->device_type);


        /* ��������:0:���¼���1��ͬ����Ĭ��0 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("LinkType", tmp_ivalue);

        pGBDeviceInfo->link_type = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->link_type:%d", pGBDeviceInfo->link_type);


        /* ���䷽ʽ */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("TransferProtocol", tmp_ivalue);

        if (tmp_ivalue == 1)
        {
            pGBDeviceInfo->trans_protocol = 1;
        }
        else
        {
            pGBDeviceInfo->trans_protocol = 0;
        }


        /* ������Ϣ */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("Manufacturer", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBDeviceInfo->manufacturer, tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->manufacturer:%s", pGBDeviceInfo->manufacturer);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->manufacturer NULL");
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->trans_protocol:%d", pGBDeviceInfo->trans_protocol);

        if (pGBDeviceInfo->id <= 0 || pGBDeviceInfo->device_id[0] == '\0')
        {
            GBDevice_info_free(pGBDeviceInfo);
            delete pGBDeviceInfo;
            pGBDeviceInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_Device_info_list() device_id Error");
            continue;
        }

        /* ƽ̨�豸��Ĭ���ǵ���������ֹDeviceInfo��Ϣ����Ӧ */
        if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type)
        {
            if (NULL != pGBDeviceInfo->manufacturer && 0 == sstrcmp(pGBDeviceInfo->manufacturer, (char*)"WISCOM"))
            {
                pGBDeviceInfo->three_party_flag = 0;
            }
            else
            {
                pGBDeviceInfo->three_party_flag = 1;
            }
        }
        else
        {
            pGBDeviceInfo->three_party_flag = 0;
        }

        /* ��ӵ����� */
        if (GBDevice_info_add(pGBDeviceInfo) < 0)
        {
            GBDevice_info_free(pGBDeviceInfo);
            delete pGBDeviceInfo;
            pGBDeviceInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_Device_info_list() GBDevice Info Add Error");
            continue;
        }
    }
    while (g_DBOper.MoveNext() >= 0);

    printf("set_db_data_to_Device_info_list() Load GBDevice info: count=%d \r\n", record_count);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_Device_info_list:END \
		\r\n ********************************************** \r\n");

    return ret;

}

/*****************************************************************************
 Prototype    : set_db_data_to_ZRVDevice_info_list
 Description  : �������ݿ�����
 Input        : None
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int set_db_data_to_ZRVDevice_info_list()
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_ZRVDevice_info_list:BEGIN \
		\r\n ********************************************** \r\n");

    strSQL.clear();
    strSQL = "select * from ZRVDeviceInfo";

    record_count = g_DBOper.DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_ZRVDevice_info_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_ZRVDevice_info_list() ErrorMsg=%s\r\n", g_DBOper.GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_ZRVDevice_info_list() exit---: No Record Count \r\n");
        return 0;
    }

    printf("\r\n set_db_data_to_ZRVDevice_info_list() DB Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_ZRVDevice_info_list() Load GBDevice info: count=%d", record_count);

        ZRVDevice_info_t* pZRVDeviceInfo = NULL;
        int i_ret = ZRVDevice_info_init(&pZRVDeviceInfo);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_ZRVDevice_info_list() GBDevice_info_init:i_ret=%d", i_ret);
            continue;
        }

        unsigned int tmp_ivalue = 0;
        string tmp_svalue;

        /* �豸����*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_ivalue);

        pZRVDeviceInfo->id = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->id:%d", pGBDeviceInfo->id);

        /* IP��ַ */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("DeviceIP", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pZRVDeviceInfo->device_ip, tmp_svalue.c_str(), MAX_IP_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->login_ip:%s", pGBDeviceInfo->login_ip);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_ZRVDevice_info_list() pGBDeviceInfo->login_ip NULL");
        }


        /* �豸����(ǰ���������NVR������CMS��TSU) */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Status", tmp_ivalue);

        pZRVDeviceInfo->reg_status = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->device_type:%d \r\n", pGBDeviceInfo->device_type);


        /* ��������:0:���¼���1��ͬ����Ĭ��0 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Expires", tmp_ivalue);

        pZRVDeviceInfo->register_expires = tmp_ivalue;
        pZRVDeviceInfo->register_expires_count = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->link_type:%d", pGBDeviceInfo->link_type);

        if (pZRVDeviceInfo->id <= 0 || pZRVDeviceInfo->device_ip[0] == '\0')
        {
            ZRVDevice_info_free(pZRVDeviceInfo);
            delete pZRVDeviceInfo;
            pZRVDeviceInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_ZRVDevice_info_list() device_id Error");
            continue;
        }

        /* ��ӵ����� */
        if (ZRVDevice_info_add(pZRVDeviceInfo) < 0)
        {
            ZRVDevice_info_free(pZRVDeviceInfo);
            delete pZRVDeviceInfo;
            pZRVDeviceInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_ZRVDevice_info_list() GBDevice Info Add Error");
            continue;
        }
    }
    while (g_DBOper.MoveNext() >= 0);

    printf("set_db_data_to_ZRVDevice_info_list() Load GBDevice info: count=%d \r\n", record_count);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_ZRVDevice_info_list:END \
		\r\n ********************************************** \r\n");

    return ret;
}

/*****************************************************************************
 Prototype    : set_db_data_to_LogicDevice_info_list
 Description  : �������ݿ�����
 Input        : None
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int set_db_data_to_LogicDevice_info_list()
{
    int ret = 0;
    int gbdevice_pos = 0;
    string strSQL = "";
    int record_count = 0;

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_LogicDevice_info_list:BEGIN \
		\r\n ********************************************** \r\n");

    strSQL.clear();
    strSQL = "select * from GBLogicDeviceConfig order by DeviceName asc";

    record_count = g_DBOper.DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_LogicDevice_info_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_LogicDevice_info_list() ErrorMsg=%s\r\n", g_DBOper.GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() exit---: No Record Count \r\n");
        return 0;
    }

    printf("\r\n set_db_data_to_LogicDevice_info_list() DB Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() Load GBLogicDevice info: count=%d", record_count);

        GBDevice_info_t* pGBDeviceInfo = NULL;
        GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
        int i_ret = GBLogicDevice_info_init(&pGBLogicDeviceInfo);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_LogicDevice_info_list() GBLogicDevice_info_init:i_ret=%d", i_ret);
            continue;
        }

        int tmp_ivalue = 0;
        unsigned int tmp_uivalue = 0;
        string tmp_svalue = "";

        /* �豸����*/
        tmp_uivalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_uivalue);

        pGBLogicDeviceInfo->id = tmp_uivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->id:%u", pGBLogicDeviceInfo->id);


        /* ��λͳһ��� */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("DeviceID", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBLogicDeviceInfo->device_id, tmp_svalue.c_str(), MAX_ID_LEN);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->device_id:%s", pGBLogicDeviceInfo->device_id);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->device_id NULL");
        }


        /* ������CMS ͳһ��� */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("CMSID", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBLogicDeviceInfo->cms_id, (char*)tmp_svalue.c_str(), MAX_ID_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->cms_id:%s", pGBLogicDeviceInfo->cms_id);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->cms_id NULL");
        }


        /* ��λ���� */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("DeviceName", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBLogicDeviceInfo->device_name, tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->device_name:%s", pGBLogicDeviceInfo->device_name);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->device_name NULL");
        }


        /* �Ƿ����� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Enable", tmp_ivalue);

        pGBLogicDeviceInfo->enable = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->enable:%d", pGBLogicDeviceInfo->enable);


        /* �豸����*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("DeviceType", tmp_ivalue);

        pGBLogicDeviceInfo->device_type = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->device_type:%d", pGBLogicDeviceInfo->device_type);


        /* �����豸������ */
        tmp_uivalue = 0;
        g_DBOper.GetFieldValue("Resved1", tmp_uivalue);

        pGBLogicDeviceInfo->alarm_device_sub_type = tmp_uivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->alarm_device_sub_type:%u", pGBLogicDeviceInfo->alarm_device_sub_type);


        /* �Ƿ�ɿ�*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("CtrlEnable", tmp_ivalue);

        pGBLogicDeviceInfo->ctrl_enable = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->ctrl_enable:%d", pGBLogicDeviceInfo->ctrl_enable);


        /* �Ƿ�֧�ֶԽ���Ĭ��ֵ0 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("MicEnable", tmp_ivalue);

        pGBLogicDeviceInfo->mic_enable = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->mic_enable:%d", pGBLogicDeviceInfo->mic_enable);


        /* ֡�ʣ�Ĭ��25 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("FrameCount", tmp_ivalue);

        pGBLogicDeviceInfo->frame_count = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->frame_count:%d", pGBLogicDeviceInfo->frame_count);


        /* �澯ʱ�� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("AlarmLengthOfTime", tmp_ivalue);

        pGBLogicDeviceInfo->alarm_duration = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->alarm_duration:%d", pGBLogicDeviceInfo->alarm_duration);


        /* ��Ӧ��ý�������豸����*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("PhyDeviceIndex", tmp_ivalue);

        pGBLogicDeviceInfo->phy_mediaDeviceIndex = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->phy_mediaDeviceIndex:%d", pGBLogicDeviceInfo->phy_mediaDeviceIndex);


        /* ��Ӧ��ý�������豸ͨ�� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("PhyDeviceChannel", tmp_ivalue);

        pGBLogicDeviceInfo->phy_mediaDeviceChannel = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->phy_mediaDeviceChannel:%d", pGBLogicDeviceInfo->phy_mediaDeviceChannel);


        /* ��Ӧ�Ŀ��������豸���� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("CtrlDeviceIndex", tmp_ivalue);

        pGBLogicDeviceInfo->phy_ctrlDeviceIndex = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->phy_ctrlDeviceIndex:%d", pGBLogicDeviceInfo->phy_ctrlDeviceIndex);


        /* ��Ӧ�Ŀ��������豸ͨ�� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("CtrlDeviceChannel", tmp_ivalue);

        pGBLogicDeviceInfo->phy_ctrlDeviceChannel = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->phy_ctrlDeviceChannel:%d", pGBLogicDeviceInfo->phy_ctrlDeviceChannel);


        /* �Ƿ�֧�ֶ����� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("StreamCount", tmp_ivalue);

        pGBLogicDeviceInfo->stream_count = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->stream_count:%d", pGBLogicDeviceInfo->stream_count);


        /* ¼������ */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("RecordType", tmp_ivalue);

        pGBLogicDeviceInfo->record_type = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->record_type:%d", pGBLogicDeviceInfo->record_type);


        /* �Ƿ��������� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("OtherRealm", tmp_ivalue);

        pGBLogicDeviceInfo->other_realm = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->other_realm:%d", pGBLogicDeviceInfo->other_realm);


        /* �豸������ */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("Manufacturer", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBLogicDeviceInfo->manufacturer, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->manufacturer:%s", pGBLogicDeviceInfo->manufacturer);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->manufacturer NULL");
        }


        /* �豸�ͺ� */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("Model", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBLogicDeviceInfo->model, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->model:%s", pGBLogicDeviceInfo->model);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->model NULL");
        }


        /* �豸���� */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("Owner", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBLogicDeviceInfo->owner, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->owner:%s", pGBLogicDeviceInfo->owner);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->owner NULL");
        }

#if 0
        /* �������� */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("CivilCode", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            pGBLogicDeviceInfo->civil_code = osip_getcopy(tmp_svalue.c_str());
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->civil_code:%s", pGBLogicDeviceInfo->civil_code);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->civil_code NULL");
        }

#endif

        /* ���� */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("Block", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBLogicDeviceInfo->block, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->block:%s", pGBLogicDeviceInfo->block);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->block NULL");
        }


        /* ��װ��ַ */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("Address", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBLogicDeviceInfo->address, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->address:%s", pGBLogicDeviceInfo->address);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->address NULL");
        }


        /* �Ƿ������豸 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Parental", tmp_ivalue);

        pGBLogicDeviceInfo->parental = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->parental:%d", pGBLogicDeviceInfo->parental);


        /* ���豸/����/ϵͳID */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("ParentID", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBLogicDeviceInfo->parentID, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->parentID:%s", pGBLogicDeviceInfo->parentID);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->parentID NULL");
        }


        /* ���ȫģʽ*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("SafetyWay", tmp_ivalue);

        pGBLogicDeviceInfo->safety_way = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->safety_way:%d", pGBLogicDeviceInfo->safety_way);


        /* ע�᷽ʽ */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("RegisterWay", tmp_ivalue);

        pGBLogicDeviceInfo->register_way = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->safety_way:%d", pGBLogicDeviceInfo->register_way);


        /* ֤�����к�*/
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("CertNum", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBLogicDeviceInfo->cert_num, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->cert_num:%s", pGBLogicDeviceInfo->cert_num);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->cert_num NULL");
        }


        /* ֤����Ч��ʶ */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Certifiable", tmp_ivalue);

        pGBLogicDeviceInfo->certifiable = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->certifiable:%d", pGBLogicDeviceInfo->certifiable);


        /* ��Чԭ���� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("ErrCode", tmp_ivalue);

        pGBLogicDeviceInfo->error_code = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->error_code:%d", pGBLogicDeviceInfo->error_code);


        /* ֤����ֹ��Ч��*/
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("EndTime", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBLogicDeviceInfo->end_time, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->end_time:%s", pGBLogicDeviceInfo->end_time);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->end_time NULL");
        }


        /* �������� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Secrecy", tmp_ivalue);

        pGBLogicDeviceInfo->secrecy = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->error_code:%d", pGBLogicDeviceInfo->secrecy);


        /* IP��ַ*/
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("IPAddress", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBLogicDeviceInfo->ip_address, (char*)tmp_svalue.c_str(), MAX_IP_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->ip_address:%s", pGBLogicDeviceInfo->ip_address);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->ip_address NULL");
        }


        /* �˿ں� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Port", tmp_ivalue);

        pGBLogicDeviceInfo->port = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->port:%d", pGBLogicDeviceInfo->port);


        /* ����*/
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("Password", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBLogicDeviceInfo->password, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->password:%s", pGBLogicDeviceInfo->password);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->password NULL");
        }


        /* ���� */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("Longitude", tmp_svalue);

        pGBLogicDeviceInfo->longitude = strtod(tmp_svalue.c_str(), (char**) NULL);
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->longitude:%f", pGBLogicDeviceInfo->longitude);


        /* γ�� */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("Latitude", tmp_svalue);

        pGBLogicDeviceInfo->latitude = strtod(tmp_svalue.c_str(), (char**) NULL);
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list:device_id=%s, longitude=%.16lf, latitude=%.16lf \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->longitude, pGBLogicDeviceInfo->latitude);


        /* ������ͼ�� */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("Resved2", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBLogicDeviceInfo->map_layer, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->map_layer:%s", pGBLogicDeviceInfo->map_layer);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->map_layer NULL");
        }

        /* ӥ�������Ӧ��Ԥ��ID */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("Resved4", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pGBLogicDeviceInfo->strResved2, (char*)tmp_svalue.c_str(), MAX_32CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->map_layer:%s", pGBLogicDeviceInfo->map_layer);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->strResved2 NULL");
        }

        if (pGBLogicDeviceInfo->id <= 0 || '\0' == pGBLogicDeviceInfo->device_id[0])
        {
            GBLogicDevice_info_free(pGBLogicDeviceInfo);
            osip_free(pGBLogicDeviceInfo);
            pGBLogicDeviceInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_LogicDevice_info_list() device_id Error");
            continue;
        }

        /* ���������豸��Ϣ */
        pGBDeviceInfo = GBDevice_info_find_by_device_index(pGBLogicDeviceInfo->phy_mediaDeviceIndex);

        gbdevice_pos = GBDevice_add(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER, pGBDeviceInfo);

        if (gbdevice_pos < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_LogicDevice_info_list() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_MASTER Error:i=%d \r\n", pGBLogicDeviceInfo->device_id, gbdevice_pos);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_MASTER OK:i=%d \r\n", pGBLogicDeviceInfo->device_id, gbdevice_pos);
        }

        if (pGBLogicDeviceInfo->stream_count > 1)
        {
            gbdevice_pos = GBDevice_add(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_SLAVE, pGBDeviceInfo);

            if (gbdevice_pos < 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_LogicDevice_info_list() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_SLAVE Error:i=%d \r\n", pGBLogicDeviceInfo->device_id, gbdevice_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_SLAVE OK:i=%d \r\n", pGBLogicDeviceInfo->device_id, gbdevice_pos);
            }
        }

        /* �����߼��豸���� */
        if (GBLogicDevice_info_add(pGBLogicDeviceInfo) < 0)
        {
            GBLogicDevice_info_free(pGBLogicDeviceInfo);
            osip_free(pGBLogicDeviceInfo);
            pGBLogicDeviceInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_LogicDevice_info_list() GBLogicDevice Info Add Error");
            continue;
        }
    }
    while (g_DBOper.MoveNext() >= 0);

    printf("set_db_data_to_LogicDevice_info_list() Load GBLogicDevice info: count=%d \r\n", record_count);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_LogicDevice_info_list:END \
		\r\n ********************************************** \r\n");

    return ret;
}
/*****************************************************************************
 Prototype    : set_db_data_to_record_info_list
 Description  : �������ݿ�����
 Input        : None
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int set_db_data_to_record_info_list()
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;
    record_info_t* pOldRecordInfo = NULL;

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_record_info_list:BEGIN \
		\r\n ********************************************** \r\n");

    strSQL.clear();
    strSQL = "select * from RecordSchedConfig";

    record_count = g_DBOper.DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list() ErrorMsg=%s\r\n", g_DBOper.GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_record_info_list() exit---: No Record Count \r\n");
        return 0;
    }

    printf("\r\n set_db_data_to_record_info_list() DB Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() Load record info: count=%d", record_count);

        record_info_t* pRecordInfo = NULL;
        int i_ret = record_info_init(&pRecordInfo);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list() record_info_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        int tmp_ivalue = 0;
        unsigned int tmp_uivalue = 0;
        string tmp_svalue;

        /* ���ݿ�����*/
        tmp_uivalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_uivalue);

        pRecordInfo->uID = tmp_uivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->uID:%d", pRecordInfo->uID);


        /* �߼��豸ͳ����*/
        tmp_uivalue = 0;
        g_DBOper.GetFieldValue("DeviceIndex", tmp_uivalue);

        pRecordInfo->device_index = tmp_uivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->device_index:%u", pRecordInfo->device_index);


        /* �Ƿ�����¼��*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("RecordEnable", tmp_ivalue);

        pRecordInfo->record_enable = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->record_enable:%d", pRecordInfo->record_enable);


        /* ¼������ */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Days", tmp_ivalue);

        pRecordInfo->record_days = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->record_days:%d", pRecordInfo->record_days);


        /* ¼��ʱ�� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("TimeLength", tmp_ivalue);

        pRecordInfo->record_timeLen = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->record_timeLen:%d", pRecordInfo->record_timeLen);


        /* ¼������ */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Type", tmp_ivalue);

        pRecordInfo->record_type = tmp_ivalue;

        if (pRecordInfo->record_type < EV9000_RECORD_TYPE_NORMAL || pRecordInfo->record_type > EV9000_RECORD_TYPE_BACKUP)
        {
            pRecordInfo->record_type = EV9000_RECORD_TYPE_NORMAL;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->record_type:%d", pRecordInfo->record_type);


        /* �Ƿ�ָ��¼�� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("AssignRecord", tmp_ivalue);

        pRecordInfo->assign_record = tmp_ivalue;

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->assign_record:%d", pRecordInfo->assign_record);


        /* ָ��¼���TSU���� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("TSUIndex", tmp_ivalue);

        pRecordInfo->assign_tsu_index = tmp_ivalue;

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->assign_tsu_index:%d", pRecordInfo->assign_tsu_index);


        /* �������� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("StreamType", tmp_ivalue);

        pRecordInfo->stream_type = tmp_ivalue;

        if (pRecordInfo->stream_type != EV9000_STREAM_TYPE_MASTER
            && pRecordInfo->stream_type != EV9000_STREAM_TYPE_SLAVE
            && pRecordInfo->stream_type != EV9000_STREAM_TYPE_INTELLIGENCE)
        {
            pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->stream_type:%d", pRecordInfo->stream_type);


        /* �Ƿ�ȫ��¼�� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("TimeOfAllWeek", tmp_ivalue);

        pRecordInfo->TimeOfAllWeek = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->TimeOfAllWeek:%d", pRecordInfo->TimeOfAllWeek);

        /* ����Ҫ�Ĵ��� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("BandWidth", tmp_ivalue);

        pRecordInfo->bandwidth = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->bandwidth:%d \r\n", pRecordInfo->bandwidth);

        if (pRecordInfo->device_index <= 0 || pRecordInfo->uID <= 0)
        {
            record_info_free(pRecordInfo);
            pRecordInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list() device_index Error");
            continue;
        }

        /* ����¼�����ͺ��߼��豸��������¼����Ϣ */
        pOldRecordInfo = record_info_get_by_record_type(pRecordInfo->device_index, pRecordInfo->record_type);

        if (NULL != pOldRecordInfo)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() record_info_get_by_record_type:device_index=%u, record_type=%d", pRecordInfo->device_index, pRecordInfo->record_type);

            pOldRecordInfo->del_mark = 0;

            if (pOldRecordInfo->record_enable != pRecordInfo->record_enable) /* �Ƿ�����¼�� */
            {
                pOldRecordInfo->record_enable = pRecordInfo->record_enable;
            }

            if (pOldRecordInfo->record_days != pRecordInfo->record_days) /* ¼������ */
            {
                pOldRecordInfo->record_days = pRecordInfo->record_days;
            }

            if (pOldRecordInfo->record_timeLen != pRecordInfo->record_timeLen) /* ¼��ʱ�� */
            {
                pOldRecordInfo->record_timeLen = pRecordInfo->record_timeLen;
            }

            if (pOldRecordInfo->record_type != pRecordInfo->record_type) /* ¼������ */
            {
                pOldRecordInfo->record_type = pRecordInfo->record_type;
            }

            if (pOldRecordInfo->assign_record != pRecordInfo->assign_record) /* �Ƿ�ָ��¼�� */
            {
                pOldRecordInfo->assign_record = pRecordInfo->assign_record;
                pOldRecordInfo->del_mark = 2;
            }

            if (pOldRecordInfo->assign_tsu_index != pRecordInfo->assign_tsu_index) /* ָ��¼���TSU���� */
            {
                pOldRecordInfo->assign_tsu_index = pRecordInfo->assign_tsu_index;
                pOldRecordInfo->del_mark = 2;
            }

            if (pOldRecordInfo->stream_type != pRecordInfo->stream_type) /* �������� */
            {
                pOldRecordInfo->stream_type = pRecordInfo->stream_type;
                pOldRecordInfo->del_mark = 2;
            }

            if (pOldRecordInfo->bandwidth != pRecordInfo->bandwidth) /* ǰ�˴���*/
            {
                pOldRecordInfo->bandwidth = pRecordInfo->bandwidth;
            }

            if (pOldRecordInfo->TimeOfAllWeek != pRecordInfo->TimeOfAllWeek) /* �Ƿ�ȫ��¼�� */
            {
                pOldRecordInfo->TimeOfAllWeek = pRecordInfo->TimeOfAllWeek;
            }

            record_info_free(pRecordInfo);
            pRecordInfo = NULL;
            continue;
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() New Record Info:device_index=%u, record_type=%d", pRecordInfo->device_index, pRecordInfo->record_type);

            /* ��ӵ����� */
            if (record_info_add(pRecordInfo) < 0)
            {
                record_info_free(pRecordInfo);
                pRecordInfo = NULL;
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list() Record Info Add Error");
                continue;
            }
        }
    }
    while (g_DBOper.MoveNext() >= 0);

    printf("set_db_data_to_record_info_list() Load record info: count=%d \r\n", record_count);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_record_info_list:END \
		\r\n ********************************************** \r\n");

    return ret;
}

/*****************************************************************************
 �� �� ��  : set_db_data_to_record_info_list_by_plan_action_info
 ��������  : ��Ԥ������Ϊ3�����ͣ�����¼����뵽¼�����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��11��3��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int set_db_data_to_record_info_list_by_plan_action_info()
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;
    int record_info_pos = -1;
    record_info_t* pOldRecordInfo = NULL;

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_record_info_list_by_plan_action_info:BEGIN \
		\r\n ********************************************** \r\n");

    strSQL.clear();
    strSQL = "select * from PlanActionConfig WHERE Type = 3"; /* ����Ԥ����������Ϊ3�ı���¼�񣬼��뵽¼����� */

    record_count = g_DBOper.DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_plan_action_info() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_plan_action_info() ErrorMsg=%s\r\n", g_DBOper.GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_record_info_list_by_plan_action_info() exit---: No Record Count \r\n");
        return 0;
    }

    printf("\r\n set_db_data_to_record_info_list_by_plan_action_info() DB Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list_by_plan_action_info() Load record info: count=%d", record_count);

        record_info_t* pRecordInfo = NULL;
        int i_ret = record_info_init(&pRecordInfo);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_plan_action_info() record_info_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        unsigned int uDeviceIndex = 0;

        /* �����̶�Ϊ0 */
        pRecordInfo->uID = 0;

        /* �߼��豸ͳ����*/
        uDeviceIndex = 0;
        g_DBOper.GetFieldValue("DeviceIndex", uDeviceIndex);

        pRecordInfo->device_index = uDeviceIndex;

        /* �Ƿ�����¼��*/
        pRecordInfo->record_enable = 1;

        /* ¼������ */
        pRecordInfo->record_days = 1;

        /* ¼��ʱ�� */
        pRecordInfo->record_timeLen = 10;

        /* ¼������ */
        pRecordInfo->record_type = EV9000_RECORD_TYPE_ALARM;

        /* �������� */
        pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;

        /* �Ƿ�ָ��¼�� */
        pRecordInfo->assign_record = 0;

        /* �Ƿ�ȫ��¼�� */
        pRecordInfo->TimeOfAllWeek = 0;

        /* ǰ�˴��� */
        pRecordInfo->bandwidth = 1;

        /* �ж��豸�����Ƿ���ȷ */
        if (pRecordInfo->device_index <= 0)
        {
            record_info_free(pRecordInfo);
            pRecordInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_plan_action_info() device_index Error");
            continue;
        }

        /* ����¼�������Ͳ���¼���¼ */
        record_info_pos = record_info_find_by_stream_type(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER);

        if (record_info_pos >= 0)
        {
            pOldRecordInfo = record_info_get(record_info_pos);

            if (NULL == pOldRecordInfo)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list_by_plan_action_info() New Record Info:device_index=%u, record_type=%d", pRecordInfo->device_index, pRecordInfo->record_type);

                /* ��ӵ����� */
                if (record_info_add(pRecordInfo) < 0)
                {
                    record_info_free(pRecordInfo);
                    pRecordInfo = NULL;
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_plan_action_info() Record Info Add Error");
                    continue;
                }
            }
            else
            {
                record_info_free(pRecordInfo);
                pRecordInfo = NULL;
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list_by_plan_action_info() New Record Info:device_index=%u, record_type=%d", pRecordInfo->device_index, pRecordInfo->record_type);

            /* ��ӵ����� */
            if (record_info_add(pRecordInfo) < 0)
            {
                record_info_free(pRecordInfo);
                pRecordInfo = NULL;
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_plan_action_info() Record Info Add Error");
                continue;
            }
        }
    }
    while (g_DBOper.MoveNext() >= 0);

    printf("set_db_data_to_record_info_list_by_plan_action_info() Load record info: count=%d \r\n", record_count);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_record_info_list_by_plan_action_info:END \
		\r\n ********************************************** \r\n");

    return ret;
}

/*****************************************************************************
 �� �� ��  : set_db_data_to_record_info_list_by_shdb_daily_upload_pic
 ��������  : �����Ϻ��ر��ճ��ϴ�ͼƬ�������¼�����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��3��19��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int set_db_data_to_record_info_list_by_shdb_daily_upload_pic()
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;
    int record_info_pos = -1;
    record_info_t* pOldRecordInfo = NULL;
    vector<unsigned int> LogicDeviceIndexVector;

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_record_info_list_by_shdb_daily_upload_pic:BEGIN \
		\r\n ********************************************** \r\n");

    strSQL.clear();
    strSQL = "select DISTINCT DeviceIndex from DiBiaoUploadPicMapConfig order by DeviceIndex asc";

    record_count = g_DBOper.DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_shdb_daily_upload_pic() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_shdb_daily_upload_pic() ErrorMsg=%s\r\n", g_DBOper.GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_record_info_list_by_shdb_daily_upload_pic() exit---: No Record Count \r\n");
        return 0;
    }

    printf("\r\n set_db_data_to_record_info_list_by_shdb_daily_upload_pic() DB Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list_by_shdb_daily_upload_pic() Load record info: count=%d", record_count);

        record_info_t* pRecordInfo = NULL;
        int i_ret = record_info_init(&pRecordInfo);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_shdb_daily_upload_pic() record_info_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        unsigned int uDeviceIndex = 0;

        /* �����̶�Ϊ0 */
        pRecordInfo->uID = 0;

        /* �߼��豸ͳ����*/
        uDeviceIndex = 0;
        g_DBOper.GetFieldValue("DeviceIndex", uDeviceIndex);

        pRecordInfo->device_index = uDeviceIndex;

        /* �Ƿ�����¼��*/
        pRecordInfo->record_enable = 1;

        /* ¼������ */
        pRecordInfo->record_days = 1;

        /* ¼��ʱ�� */
        pRecordInfo->record_timeLen = 10;

        /* ¼������ */
        pRecordInfo->record_type = EV9000_RECORD_TYPE_NORMAL;

        /* �������� */
        pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;

        /* �Ƿ�ָ��¼�� */
        pRecordInfo->assign_record = 0;

        /* �Ƿ�ȫ��¼�� */
        pRecordInfo->TimeOfAllWeek = 0;

        /* ǰ�˴��� */
        pRecordInfo->bandwidth = 1;

        /* �ж��豸�����Ƿ���ȷ */
        if (pRecordInfo->device_index <= 0)
        {
            record_info_free(pRecordInfo);
            pRecordInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_shdb_daily_upload_pic() device_index Error");
            continue;
        }

        /* ����¼�������Ͳ���¼���¼ */
        record_info_pos = record_info_find_by_stream_type(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER);

        if (record_info_pos >= 0)
        {
            pOldRecordInfo = record_info_get(record_info_pos);

            if (NULL == pOldRecordInfo)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list_by_shdb_daily_upload_pic() New Record Info:device_index=%u, record_type=%d", pRecordInfo->device_index, pRecordInfo->record_type);

                /* ��ӵ����� */
                if (record_info_add(pRecordInfo) < 0)
                {
                    record_info_free(pRecordInfo);
                    pRecordInfo = NULL;
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_shdb_daily_upload_pic() Record Info Add Error");
                    continue;
                }
            }
            else
            {
                record_info_free(pRecordInfo);
                pRecordInfo = NULL;
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list_by_shdb_daily_upload_pic() New Record Info:device_index=%u, record_type=%d", pRecordInfo->device_index, pRecordInfo->record_type);

            /* ��ӵ����� */
            if (record_info_add(pRecordInfo) < 0)
            {
                record_info_free(pRecordInfo);
                pRecordInfo = NULL;
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_shdb_daily_upload_pic() Record Info Add Error");
                continue;
            }
        }
    }
    while (g_DBOper.MoveNext() >= 0);

    printf("set_db_data_to_record_info_list_by_shdb_daily_upload_pic() Load record info: count=%d \r\n", record_count);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_record_info_list_by_shdb_daily_upload_pic:END \
		\r\n ********************************************** \r\n");

    return ret;
}

/*****************************************************************************
 �� �� ��  : set_db_data_to_record_info_list_by_shdb_alarm_upload_pic
 ��������  : �����Ϻ��ر걨���ϴ�ͼƬ�������¼�����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��3��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int set_db_data_to_record_info_list_by_shdb_alarm_upload_pic()
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;
    int record_info_pos = -1;
    record_info_t* pOldRecordInfo = NULL;
    vector<unsigned int> LogicDeviceIndexVector;

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_record_info_list_by_shdb_alarm_upload_pic:BEGIN \
		\r\n ********************************************** \r\n");

    strSQL.clear();
    strSQL = "select * from PlanActionConfig WHERE Type = 5"; /* ����Ԥ����������Ϊ5�ı�����ͼ�ϴ������뵽¼����� */

    record_count = g_DBOper.DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_shdb_alarm_upload_pic() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_shdb_alarm_upload_pic() ErrorMsg=%s\r\n", g_DBOper.GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_record_info_list_by_shdb_alarm_upload_pic() exit---: No Record Count \r\n");
        return 0;
    }

    printf("\r\n set_db_data_to_record_info_list_by_shdb_alarm_upload_pic() DB Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list_by_shdb_alarm_upload_pic() Load record info: count=%d", record_count);

        record_info_t* pRecordInfo = NULL;
        int i_ret = record_info_init(&pRecordInfo);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_shdb_alarm_upload_pic() record_info_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        unsigned int uDeviceIndex = 0;

        /* �����̶�Ϊ0 */
        pRecordInfo->uID = 0;

        /* �߼��豸ͳ����*/
        uDeviceIndex = 0;
        g_DBOper.GetFieldValue("DeviceIndex", uDeviceIndex);

        pRecordInfo->device_index = uDeviceIndex;

        /* �Ƿ�����¼��*/
        pRecordInfo->record_enable = 1;

        /* ¼������ */
        pRecordInfo->record_days = 1;

        /* ¼��ʱ�� */
        pRecordInfo->record_timeLen = 10;

        /* ¼������ */
        pRecordInfo->record_type = EV9000_RECORD_TYPE_NORMAL;

        /* �������� */
        pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;

        /* �Ƿ�ָ��¼�� */
        pRecordInfo->assign_record = 0;

        /* �Ƿ�ȫ��¼�� */
        pRecordInfo->TimeOfAllWeek = 0;

        /* ǰ�˴��� */
        pRecordInfo->bandwidth = 1;

        /* �ж��豸�����Ƿ���ȷ */
        if (pRecordInfo->device_index <= 0)
        {
            record_info_free(pRecordInfo);
            pRecordInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_shdb_alarm_upload_pic() device_index Error");
            continue;
        }

        /* ����¼�������Ͳ���¼���¼ */
        record_info_pos = record_info_find_by_stream_type(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER);

        if (record_info_pos >= 0)
        {
            pOldRecordInfo = record_info_get(record_info_pos);

            if (NULL == pOldRecordInfo)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list_by_shdb_alarm_upload_pic() New Record Info:device_index=%u, record_type=%d", pRecordInfo->device_index, pRecordInfo->record_type);

                /* ��ӵ����� */
                if (record_info_add(pRecordInfo) < 0)
                {
                    record_info_free(pRecordInfo);
                    pRecordInfo = NULL;
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_shdb_alarm_upload_pic() Record Info Add Error");
                    continue;
                }
            }
            else
            {
                record_info_free(pRecordInfo);
                pRecordInfo = NULL;
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list_by_shdb_alarm_upload_pic() New Record Info:device_index=%u, record_type=%d", pRecordInfo->device_index, pRecordInfo->record_type);

            /* ��ӵ����� */
            if (record_info_add(pRecordInfo) < 0)
            {
                record_info_free(pRecordInfo);
                pRecordInfo = NULL;
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_shdb_alarm_upload_pic() Record Info Add Error");
                continue;
            }
        }
    }
    while (g_DBOper.MoveNext() >= 0);

    printf("set_db_data_to_record_info_list_by_shdb_alarm_upload_pic() Load record info: count=%d \r\n", record_count);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_record_info_list_by_shdb_alarm_upload_pic:END \
		\r\n ********************************************** \r\n");

    return ret;
}

/*****************************************************************************
 Prototype    : set_db_data_to_poll_srv
 Description  : �������ݿ�����
 Input        : None
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int set_db_data_to_poll_srv_list()
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_poll_srv_list:BEGIN \
		\r\n ********************************************** \r\n");

    strSQL.clear();
    strSQL = "select * from PollConfig";

    record_count = g_DBOper.DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_poll_srv_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_poll_srv_list() ErrorMsg=%s\r\n", g_DBOper.GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_poll_srv_list() exit---: No Record Count \r\n");
        return 0;
    }

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_poll_srv_list() Load poll info: count=%d", record_count);

        poll_srv_t* pPollSrv = NULL;
        int i_ret = poll_srv_init(&pPollSrv);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_poll_srv_list() poll_srv_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        int tmp_ivalue = 0;
        unsigned int tmp_uvalue = 0;
        string tmp_svalue = "";

        /* ��Ѳid */
        tmp_uvalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_uvalue);

        pPollSrv->poll_id = tmp_uvalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_poll_srv_list() pPollSrv->poll_id:%u", pPollSrv->poll_id);


        /* ��Ѳ���� */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("PollName", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pPollSrv->poll_name, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_poll_srv_list() pPollSrv->poll_name:%s", pPollSrv->poll_name);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_poll_srv_list() pPollSrv->poll_name NULL");
        }


        /* ��ʼʱ�� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("StartTime", tmp_ivalue);

        pPollSrv->start_time = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_poll_srv_list() pPollSrv->start_time:%d", pPollSrv->start_time);


        /* ����ʱ�� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("DurationTime", tmp_ivalue);

        pPollSrv->duration_time = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_poll_srv_list() pPollSrv->duration_time:%d \r\n", pPollSrv->duration_time);

        if (pPollSrv->poll_id <= 0)
        {
            poll_srv_free(pPollSrv);
            pPollSrv = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_poll_srv_list() poll_id Error");
            continue;
        }

        /* ��ӵ����� */
        if (poll_srv_add(pPollSrv) < 0)
        {
            poll_srv_free(pPollSrv);
            pPollSrv = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_poll_srv_list() Poll Srv Add Error");
            continue;
        }
    }
    while (g_DBOper.MoveNext() >= 0);

    printf("set_db_data_to_poll_srv_list() Load poll info: count=%d \r\n", record_count);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_poll_srv_list:END \
		\r\n ********************************************** \r\n");

    return ret;
}

/*****************************************************************************
 Prototype    : set_db_data_to_plan_srv_list
 Description  : �������ݿ�����
 Input        : None
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int set_db_data_to_plan_srv_list()
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_plan_srv_list:BEGIN \
		\r\n ********************************************** \r\n");

    strSQL.clear();
    strSQL = "select * from PlanConfig";

    record_count = g_DBOper.DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_plan_srv_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_plan_srv_list() ErrorMsg=%s\r\n", g_DBOper.GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_plan_srv_list() exit---: No Record Count \r\n");
        return 0;
    }

    printf("\r\n set_db_data_to_plan_srv_list() DB Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_plan_srv_list() Load plan info: count=%d", record_count);

        plan_srv_t* pPlanSrv = NULL;
        int i_ret = plan_srv_init(&pPlanSrv);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_plan_srv_list() plan_srv_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        int tmp_ivalue = 0;
        unsigned int tmp_uvalue = 0;
        string tmp_svalue = "";

        /* Ԥ��id */
        tmp_uvalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_uvalue);

        pPlanSrv->plan_id = tmp_uvalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_plan_srv_list() pPlanSrv->plan_id:%u", pPlanSrv->plan_id);


        /* Ԥ������ */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("PlanName", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pPlanSrv->plan_name, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_plan_srv_list() pPlanSrv->plan_name:%s", pPlanSrv->plan_name);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_plan_srv_list() pPlanSrv->plan_name NULL");
        }


        /* ��ʼʱ�� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("StartTime", tmp_ivalue);

        pPlanSrv->start_time = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_plan_srv_list() pPlanSrv->start_time:%d \r\n", pPlanSrv->start_time);

        if (pPlanSrv->plan_id <= 0)
        {
            plan_srv_free(pPlanSrv);
            pPlanSrv = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_plan_srv_list() plan_id Error");
            continue;
        }

        if (plan_srv_add(pPlanSrv) < 0)
        {
            plan_srv_free(pPlanSrv);
            pPlanSrv = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_plan_srv_list() Plan Srv Add Error");
            continue;
        }
    }
    while (g_DBOper.MoveNext() >= 0);

    printf("set_db_data_to_plan_srv_list() Load plan info: count=%d \r\n", record_count);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_plan_srv_list:END \
		\r\n ********************************************** \r\n");

    return ret;
}
/*****************************************************************************
 Prototype    : set_db_data_to_cruise_srv_list
 Description  : �������ݿ�����
 Input        : None
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int set_db_data_to_cruise_srv_list()
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_cruise_srv_list:BEGIN \
		\r\n ********************************************** \r\n");

    strSQL.clear();
    strSQL = "select * from CruiseConfig";

    record_count = g_DBOper.DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_cruise_srv_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_cruise_srv_list() ErrorMsg=%s\r\n", g_DBOper.GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_cruise_srv_list() exit---: No Record Count \r\n");
        return 0;
    }

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_cruise_srv_list() Load cruise info: count=%d", record_count);

        cruise_srv_t* pCruiseSrv = NULL;
        int i_ret = cruise_srv_init(&pCruiseSrv);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_cruise_srv_list() cruise_srv_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        int tmp_ivalue = 0;
        string tmp_svalue;

        /* Ѳ��id */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_ivalue);

        pCruiseSrv->cruise_id = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_cruise_srv_list() pCruiseSrv->cruise_id: %d", pCruiseSrv->cruise_id);


        /* Ѳ������ */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("CruiseName", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pCruiseSrv->cruise_name, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_cruise_srv_list() pCruiseSrv->cruise_name: %s", pCruiseSrv->cruise_name);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_cruise_srv_list() pCruiseSrv->cruise_name NULL");
        }


        /* ��ʼʱ�� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("StartTime", tmp_ivalue);

        pCruiseSrv->start_time = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_cruise_srv_list() pCruiseSrv->start_time: %d", pCruiseSrv->start_time);


        /* ����ʱ�� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("DurationTime", tmp_ivalue);

        pCruiseSrv->duration_time = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_cruise_srv_list() pCruiseSrv->duration_time:%d \r\n", pCruiseSrv->duration_time);

        if (pCruiseSrv->cruise_id <= 0)
        {
            cruise_srv_free(pCruiseSrv);
            pCruiseSrv = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_cruise_srv_list() cruise_id Error");
            continue;
        }

        /* ��ӵ����� */
        if (cruise_srv_add(pCruiseSrv) < 0)
        {
            cruise_srv_free(pCruiseSrv);
            pCruiseSrv = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_cruise_srv_list() Cruise Srv Add Error");
            continue;
        }
    }
    while (g_DBOper.MoveNext() >= 0);

    printf("set_db_data_to_cruise_srv_list() Load cruise info: count=%d \r\n", record_count);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_cruise_srv_list:END \
		\r\n ********************************************** \r\n");

    return ret;
}

/*****************************************************************************
 �� �� ��  : add_default_mms_route_info_to_route_info_list
 ��������  : ���Ĭ�ϵ�mmsע��·�ɵ�·�ɶ���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��10��20��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int add_default_mms_route_info_to_route_info_list()
{
    int ret = 0;
    vector<string> SubCmsIPVector;
    int iRet = 0;
    route_info_t* pRouteInfo = NULL;

    if (0 == g_MMSEnableFlag)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "add_default_mms_route_info_to_route_info_list() Exit:--- MMS Not Enable \r\n", ret);
        return 0;
    }

    if (NULL == pGblconf || pGblconf->board_id[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "add_default_mms_route_info_to_route_info_list() Exit:--- Param Error \r\n", ret);
        return -1;
    }

    ret = route_info_init(&pRouteInfo);

    if (ret != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "add_default_mms_route_info_to_route_info_list() route_info_init:ret=%d \r\n", ret);
        return -1;
    }

    pRouteInfo->id = 0;

    osip_strncpy(pRouteInfo->server_id, pGblconf->mms_id, MAX_ID_LEN);
    osip_strncpy(pRouteInfo->register_account, pGblconf->board_id, MAX_128CHAR_STRING_LEN);

    osip_strncpy(pRouteInfo->server_ip, local_ip_get(default_eth_name_get()), MAX_IP_LEN);
    pRouteInfo->server_port = 5061;
    osip_strncpy(pRouteInfo->register_password, (char*)"12345678", MAX_128CHAR_STRING_LEN);
    pRouteInfo->link_type = 0;
    pRouteInfo->three_party_flag = 0;
    pRouteInfo->trans_protocol = 0;
    osip_strncpy(pRouteInfo->strRegLocalEthName, default_eth_name_get(), MAX_IP_LEN);

    /* ���¼�CMS��IP��ַȡ�� */
    SubCmsIPVector.clear();
    AddAllSubCMSIPToVector(SubCmsIPVector);

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "add_default_mms_route_info_to_route_info_list() SubCmsIPVector.size()=%d \r\n", (int)SubCmsIPVector.size());

    /* ���ض���ip��ַ���˵� */
    iRet = IsIPInSubCMS(SubCmsIPVector, pRouteInfo->server_ip);

    if (1 == iRet)
    {
        pRouteInfo->ip_is_in_sub = 1;
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "add_default_mms_route_info_to_route_info_list() server_ip=%s Is Sub CMS \r\n", pRouteInfo->server_ip);
    }
    else
    {
        pRouteInfo->ip_is_in_sub = 0;
    }

    /* ��ӵ����� */
    if (route_info_add(pRouteInfo) < 0)
    {
        route_info_free(pRouteInfo);
        pRouteInfo = NULL;
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "add_default_mms_route_info_to_route_info_list() Route Info Add Error");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���Ĭ�ϵ��ֻ�MMS�ϼ�·��������Ϣ: MMS ID=%s, MMS IP=%s, MMS Port=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Add default MMS Route Info: MMS ID=%s, MMS IP=%s, MMS Port=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    return 0;
}

/*****************************************************************************
 Prototype    : set_db_data_to_route_info_list
 Description  : �������ݿ�����
 Input        : None
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int set_db_data_to_route_info_list()
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_route_info_list:BEGIN \
		\r\n ********************************************** \r\n");

    strSQL.clear();
    strSQL = "select * from RouteNetConfig";

    record_count = g_DBOper.DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_route_info_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_route_info_list() ErrorMsg=%s\r\n", g_DBOper.GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_route_info_list() exit---: No Record Count \r\n");
        return 0;
    }

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() Load route info: count=%d", record_count);

        route_info_t* pRouteInfo = NULL;
        int i_ret = route_info_init(&pRouteInfo);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_route_info_list() route_info_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        int tmp_ivalue = 0;
        string tmp_svalue;
        vector<string> SubCmsIPVector;
        int iRet = 0;

        /* ���� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_ivalue);

        pRouteInfo->id = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->id: %d", pRouteInfo->id);


        /* �ϼ�������CMSͳһ���id */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("ServerID", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pRouteInfo->server_id, (char*)tmp_svalue.c_str(), MAX_ID_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->server_id: %s", pRouteInfo->server_id);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->server_id NULL");
        }


        /* �ϼ�������CMS ip��ַ*/
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("ServerIP", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pRouteInfo->server_ip, (char*)tmp_svalue.c_str(), MAX_IP_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->server_ip: %s", pRouteInfo->server_ip);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->server_ip NULL");
        }


        /* �ϼ�������CMS ������ַ */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("ServerHost", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pRouteInfo->server_host, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->server_host: %s", pRouteInfo->server_host);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->server_host NULL");
        }


        /* �ϼ�������CMS �˿ں�*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("ServerPort", tmp_ivalue);

        pRouteInfo->server_port = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->server_port: %d", pRouteInfo->server_port);


        /* ע���˺� */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("UserName", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pRouteInfo->register_account, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->register_account: %s", pRouteInfo->register_account);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->register_account NULL");
        }


        /* ע������ */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("Password", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pRouteInfo->register_password, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->register_password:%s \r\n", pRouteInfo->register_password);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->register_password NULL \r\n");
        }


        /* ��������*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("LinkType", tmp_ivalue);

        pRouteInfo->link_type = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->link_type: %d", pRouteInfo->link_type);


        /* �Ƿ��ǵ�����ƽ̨ */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Resved1", tmp_ivalue);

        pRouteInfo->three_party_flag = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->three_party_flag: %d", pRouteInfo->three_party_flag);


        /* ����Э������ */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("TransferProtocol", tmp_ivalue);

        pRouteInfo->trans_protocol = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->trans_protocol: %d", pRouteInfo->trans_protocol);


        /* ����ʹ�õ��������� */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("LocalEthName", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            memset(pRouteInfo->strRegLocalEthName, 0, MAX_IP_LEN);
            osip_strncpy(pRouteInfo->strRegLocalEthName, tmp_svalue.c_str(), MAX_IP_LEN);
        }
        else
        {
            memset(pRouteInfo->strRegLocalEthName, 0, MAX_IP_LEN);
            osip_strncpy(pRouteInfo->strRegLocalEthName, default_eth_name_get(), MAX_IP_LEN);
        }

        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->strRegLocalEthName: %s", pRouteInfo->strRegLocalEthName);

        if (pRouteInfo->id <= 0 || '\0' == pRouteInfo->server_id[0] || '\0' == pRouteInfo->server_ip[0])
        {
            route_info_free(pRouteInfo);
            pRouteInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_route_info_list() server_id Error");
            continue;
        }

        if (IsLocalHost(pRouteInfo->server_ip) && 0 == sstrcmp(pRouteInfo->server_id, local_cms_id_get())) /* ���˵��ͱ���CMS����һ�����ϼ�CMS */
        {
            route_info_free(pRouteInfo);
            pRouteInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_route_info_list() Server Info Is Match Local CMS");
            continue;
        }

        /* ���¼�CMS��IP��ַȡ�� */
        SubCmsIPVector.clear();
        AddAllSubCMSIPToVector(SubCmsIPVector);

        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "set_db_data_to_route_info_list() SubCmsIPVector.size()=%d \r\n", (int)SubCmsIPVector.size());

        /* ���ض���ip��ַ���˵� */
        iRet = IsIPInSubCMS(SubCmsIPVector, pRouteInfo->server_ip);

        if (1 == iRet)
        {
            pRouteInfo->ip_is_in_sub = 1;
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_route_info_list() server_ip=%s Is Sub CMS \r\n", pRouteInfo->server_ip);
        }
        else
        {
            pRouteInfo->ip_is_in_sub = 0;
        }

        /* ��ӵ����� */
        if (route_info_add(pRouteInfo) < 0)
        {
            route_info_free(pRouteInfo);
            pRouteInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_route_info_list() Route Info Add Error");
            continue;
        }
    }
    while (g_DBOper.MoveNext() >= 0);

    printf("set_db_data_to_route_info_list() Load route info: count=%d \r\n", record_count);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_route_info_list:END \
		\r\n ********************************************** \r\n");

    return ret;
}

/*****************************************************************************
 Prototype    : set_db_data_to_route_info_list
 Description  : �������ݿ�����
 Input        : None
 Output       : None
 Return Value : int
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/6/19
    Author       : wangqichao
    Modification : Created function

*****************************************************************************/
int set_db_data_to_platform_info_list()
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_platform_info_list:BEGIN \
		\r\n ********************************************** \r\n");

    strSQL.clear();
    strSQL = "select * from VideoManagePlatformInfo";

    record_count = g_DBOper.DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_platform_info_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_platform_info_list() ErrorMsg=%s\r\n", g_DBOper.GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_platform_info_list() exit---: No Record Count \r\n");
        return 0;
    }

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_platform_info_list() Load route info: count=%d", record_count);

        platform_info_t* pPlatfromInfo = NULL;
        int i_ret = platform_info_init(&pPlatfromInfo);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_platform_info_list() route_info_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        unsigned int tmp_ivalue = 0;
        string tmp_svalue;

        /* ���� */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_ivalue);

        pPlatfromInfo->id = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->id: %d", pRouteInfo->id);

        /* �ϼ�������CMS ip��ַ*/
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("PlatformIP", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pPlatfromInfo->platform_ip, (char*)tmp_svalue.c_str(), MAX_IP_LEN);
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->server_ip: %s", pRouteInfo->server_ip);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_platform_info_list() pRouteInfo->platform_ip NULL");
        }

        if (pPlatfromInfo->id <= 0 || '\0' == pPlatfromInfo->platform_ip[0])
        {
            platform_info_free(pPlatfromInfo);
            pPlatfromInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_platform_info_list() platform_ip Error");
            continue;
        }

        /* ��ӵ����� */
        if (platform_info_add(pPlatfromInfo) < 0)
        {
            platform_info_free(pPlatfromInfo);
            pPlatfromInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_platform_info_list() Route Info Add Error");
            continue;
        }
    }
    while (g_DBOper.MoveNext() >= 0);

    printf("set_db_data_to_platform_info_list() Load platform info: count=%d \r\n", record_count);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_platform_info_list:END \
		\r\n ********************************************** \r\n");

    return ret;
}

/*****************************************************************************
 �� �� ��  : get_platform_last_task_time_and_status
 ��������  : ��ȡƽ̨�����ʱ���Լ�״̬
 �������  : char* platform_ip
             DBOper* pDbOper
             int* iTaskTime
             int* iTaskStatus
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��8��21��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_platform_last_task_time_and_status(char* platform_ip, DBOper* pDbOper, int* iTaskTime, int* iTaskStatus)
{
    string strSQL = "";
    int record_count = 0;
    int last_task_time = 0;
    int compress_task_status = 0;

    if (NULL == platform_ip || NULL == pDbOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "get_platform_last_task_time_and_status() exit---:  Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from VideoManagePlatformInfo WHERE PlatformIP LIKE '";
    strSQL += platform_ip;
    strSQL += "'";

    record_count = pDbOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "get_platform_last_task_time_and_status() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "get_platform_last_task_time_and_status() ErrorMsg=%s\r\n", pDbOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "get_platform_last_task_time_and_status() exit---: No Record Count:strSQL=%s \r\n", strSQL.c_str());
        return 0;
    }

    last_task_time = 0;
    pDbOper->GetFieldValue("LastTaskTime", last_task_time);
    *iTaskTime = last_task_time;

    compress_task_status = 0;
    pDbOper->GetFieldValue("CompressTaskStatus", compress_task_status);
    *iTaskStatus = compress_task_status;

    return 0;
}

/*****************************************************************************
 �� �� ��  : get_platform_task_mode_and_time
 ��������  : ��ȡƽ̨������ִ��ģʽ��ʱ��
 �������  : char* platform_ip
             DBOper* pDbOper
             int* iTaskMode
             int* iTaskBeginTime
             int* iTaskEndTime
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��8��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_platform_task_mode_and_time(char* platform_ip, DBOper* pDbOper, int* iTaskMode, int* iTaskBeginTime, int* iTaskEndTime)
{
    string strSQL = "";
    int record_count = 0;
    int task_mode = 0;
    int task_begin_time = 0;
    int task_end_time = 0;

    if (NULL == platform_ip || NULL == pDbOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "get_platform_task_mode_and_time() exit---:  Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from VideoManagePlatformInfo WHERE PlatformIP LIKE '";
    strSQL += platform_ip;
    strSQL += "'";

    record_count = pDbOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "get_platform_task_mode_and_time() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "get_platform_task_mode_and_time() ErrorMsg=%s\r\n", pDbOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "get_platform_task_mode_and_time() exit---: No Record Count:strSQL=%s \r\n", strSQL.c_str());
        return 0;
    }

    task_mode = 0;
    pDbOper->GetFieldValue("TaskMode", task_mode);
    *iTaskMode = task_mode;

    task_begin_time = 0;
    pDbOper->GetFieldValue("TaskBeginTime", task_begin_time);
    *iTaskBeginTime = task_begin_time;

    task_end_time = 0;
    pDbOper->GetFieldValue("TaskEndTime", task_end_time);
    *iTaskEndTime = task_end_time;

    return 0;
}

/*****************************************************************************
 �� �� ��  : set_db_data_to_compress_task_list
 ��������  : �����ϴ�û�н���������
 �������  : char* platform_ip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��8��21��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int set_db_data_to_compress_task_list(char* platform_ip, DBOper* pDbOper)
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;

    if (NULL == platform_ip || NULL == pDbOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "set_db_data_to_compress_task_list() exit---:  Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_compress_task_list:BEGIN \
		\r\n ********************************************** \r\n");

    strSQL.clear();
    strSQL = "select * from ZRVCompressTaskAssignInfo WHERE TaskStatus=0 OR TaskStatus=1 AND PlatformIP like '";
    strSQL += platform_ip;
    strSQL += "'";

    record_count = pDbOper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_compress_task_list() Load compress task info: count=%d", record_count);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_compress_task_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_compress_task_list() ErrorMsg=%s\r\n", pDbOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_compress_task_list() exit---: No Record Count:strSQL=%s \r\n", strSQL.c_str());
        return 0;
    }

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_compress_task_list() Load route info: count=%d", record_count);

        compress_task_t* compress_task = NULL;
        int i_ret = compress_task_init(&compress_task);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_compress_task_list() route_info_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        unsigned int tmp_ivalue = 0;
        string tmp_svalue;

        /* ��¼��� */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("RecordNum", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.jlbh, (char*)tmp_svalue.c_str(), MAX_TSU_TASK_LEN);

        /* �ļ����� */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("FileName", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.wjmc, (char*)tmp_svalue.c_str(), 128);

        /* �ļ���׺ */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("FileSuffixName", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.kzm, (char*)tmp_svalue.c_str(), 32);

        /* �ļ���С */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("FileSize", tmp_ivalue);
        compress_task->stYSPB.wjdx = tmp_ivalue;

        /* �ϴ���λ */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("UploadUnit", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.scdw, (char*)tmp_svalue.c_str(), 128);

        /* �ϴ�ʱ�� */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("UploadTime", tmp_ivalue);
        compress_task->stYSPB.scsj = tmp_ivalue;

        /* �洢·�� */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("StoragePath", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.cclj, (char*)tmp_svalue.c_str(), 128);

        /* �����ʶ */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("AssignFlag", tmp_ivalue);
        compress_task->iAssignFlag = tmp_ivalue;

        /* ƽ̨IP��ַ */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("PlatformIP", tmp_svalue);
        osip_strncpy(compress_task->strPlatformIP, (char*)tmp_svalue.c_str(), MAX_IP_LEN);

        /* ZRV IP��ַ */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("ZRVDeviceIP", tmp_svalue);
        osip_strncpy(compress_task->strZRVDeviceIP, (char*)tmp_svalue.c_str(), MAX_IP_LEN);

        /* ����״̬ */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("TaskStatus", tmp_ivalue);
        compress_task->iTaskStatus = tmp_ivalue;

        /* ������ */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("TaskResult", tmp_ivalue);
        compress_task->iTaskResult = tmp_ivalue;

        compress_task->wait_answer_expire = 0;
        compress_task->resend_count = 0;

        /* ��ӵ����� */
        if (compress_task_add(compress_task) < 0)
        {
            compress_task_free(compress_task);
            osip_free(compress_task);
            compress_task = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_compress_task_list() compress_task_add Error");
            continue;
        }
    }
    while (pDbOper->MoveNext() >= 0);

    printf("set_db_data_to_compress_task_list() Load platform info: count=%d \r\n", record_count);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_compress_task_list:END \
		\r\n ********************************************** \r\n");

    return ret;
}

int get_compress_task_count_from_db(char* platform_ip, DBOper* pDbOper)
{
    string strSQL = "";
    int record_count = 0;

    if (NULL == platform_ip || NULL == pDbOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "get_compress_task_count_from_db() exit---:  Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "SELECT * FROM ZRVCompressTaskAssignInfo WHERE PlatformIP like '";
    strSQL += platform_ip;
    strSQL += "'";

    record_count = pDbOper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_count_from_db() Load compress task info: count=%d", record_count);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "get_compress_task_count_from_db() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "get_compress_task_count_from_db() ErrorMsg=%s\r\n", pDbOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "get_compress_task_count_from_db() exit---: No Record Count \r\n");
        return 0;
    }

    return record_count;
}

/*****************************************************************************
 �� �� ��  : set_db_data_to_preset_auto_back_list
 ��������  : ����Ԥ��λ�Զ���λ��Ϣ������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��18�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int set_db_data_to_preset_auto_back_list()
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_preset_auto_back_list:BEGIN \
		\r\n ********************************************** \r\n");

    strSQL.clear();
    strSQL = "select * from PresetConfig WHERE Resved1 >= 1";

    record_count = g_DBOper.DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_preset_auto_back_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_preset_auto_back_list() ErrorMsg=%s\r\n", g_DBOper.GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_db_data_to_preset_auto_back_list() exit---: No Record Count \r\n");
        return 0;
    }

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_preset_auto_back_list() Load route info: count=%d", record_count);

        unsigned int uDeviceIndex = 0;
        unsigned int uPresetID = 0;
        unsigned int iResved1 = 0;

        /* �߼��豸���� */
        g_DBOper.GetFieldValue("DeviceIndex", uDeviceIndex);

        /* Ԥ��λ��� */
        g_DBOper.GetFieldValue("PresetID", uPresetID);

        /* Ԥ��λ�Զ���λʱ�� */
        g_DBOper.GetFieldValue("Resved1", iResved1);

        if (iResved1 >= 60)
        {
            /* ��ӵ����� */
            ret = preset_auto_back_use(uDeviceIndex, uPresetID, iResved1);
        }
        else
        {
            /* ��ӵ����� */
            ret = preset_auto_back_use(uDeviceIndex, uPresetID, 60);
        }

        if (ret < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_preset_auto_back_list() preset_auto_back_use Error");
            continue;
        }

        ret = preset_auto_back_update(uDeviceIndex);

        if (ret < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_preset_auto_back_list() preset_auto_back_update Error");
            continue;
        }
    }
    while (g_DBOper.MoveNext() >= 0);

    printf("set_db_data_to_preset_auto_back_list() Load route info: count=%d \r\n", record_count);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "\r\n ********************************************** \
		\r\n |set_db_data_to_preset_auto_back_list:END \
		\r\n ********************************************** \r\n");

    return ret;
}

/*****************************************************************************
 �� �� ��  : UpdateAllUserRegStatus2DB
 ��������  : ���������û����ݿ�ע��״̬
 �������  : int status
             DBOper* pUser_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��9�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int UpdateAllUserRegStatus2DB(int status, DBOper* pUser_Srv_dboper)
{
    int iRet = 0;
    string strSQL = "";
    char strStatus[16] = {0};

    if (NULL == pUser_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "UpdateAllUserRegStatus2DB() exit---: User Srv DB Oper Error \r\n");
        return -1;
    }

    /* �������ݿ� */
    strSQL.clear();
    strSQL = "UPDATE UserConfig SET Status = ";
    snprintf(strStatus, 16, "%d", status);
    strSQL += strStatus;

    iRet = pUser_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateAllUserRegStatus2DB() DB Oper Error:strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateAllUserRegStatus2DB() ErrorMsg=%s\r\n", pUser_Srv_dboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : UpdateAllGBDeviceRegStatus2DB
 ��������  : �������б�׼�����豸���ݿ�ע��״̬
 �������  : int status
                            DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :int
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int UpdateAllGBDeviceRegStatus2DB(int status, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;
    string strSQL = "";
    char strStatus[16] = {0};

    if (NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "UpdateAllGBDeviceRegStatus2DB() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    /* �������ݿ� */
    strSQL.clear();
    strSQL = "UPDATE GBPhyDeviceConfig SET Status = ";
    snprintf(strStatus, 16, "%d", status);
    strSQL += strStatus;

    iRet = pDevice_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateAllGBDeviceRegStatus2DB() DB Oper Error:strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateAllGBDeviceRegStatus2DB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : UpdateAllGBLogicDeviceRegStatus2DB
 ��������  : �������б�׼�߼��豸���ݿ�ע��״̬
 �������  : int status
                            DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :int
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int UpdateAllGBLogicDeviceRegStatus2DB(int status, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;
    string strSQL = "";
    char strStatus[16] = {0};

    if (NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "UpdateAllGBLogicDeviceRegStatus2DB() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    /* �������ݿ� */
    strSQL.clear();
    strSQL = "UPDATE GBLogicDeviceConfig SET Status = ";
    snprintf(strStatus, 16, "%d", status);
    strSQL += strStatus;

    iRet = pDevice_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateAllGBLogicDeviceRegStatus2DB() DB Oper Error:strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateAllGBLogicDeviceRegStatus2DB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : UpdateAllZRVDeviceRegStatus2DB
 ��������  : �������б�׼�����豸���ݿ�ע��״̬
 �������  : int status
                            DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :int
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int UpdateAllZRVDeviceRegStatus2DB(int status, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;
    string strSQL = "";
    char strStatus[16] = {0};

    if (NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "UpdateAllZRVDeviceRegStatus2DB() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    /* �������ݿ� */
    strSQL.clear();
    strSQL = "UPDATE ZRVDeviceInfo SET Status = ";
    snprintf(strStatus, 16, "%d", status);
    strSQL += strStatus;

    iRet = pDevice_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateAllZRVDeviceRegStatus2DB() DB Oper Error:strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateAllZRVDeviceRegStatus2DB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : UpdateMgwOptionsIPAddress2DB
 ��������  : ����ý�����ص�IP��ַ����
 �������  : char* pcIPAddr
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��9��28��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int UpdateMgwOptionsIPAddress2DB(char* pcIPAddr, DBOper* pdboper)
{
    int iRet = 0;
    string strSQL = "";

    if (NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "UpdateMgwOptionsIPAddress2DB() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    /* �������ݿ� */
    strSQL.clear();
    strSQL = "UPDATE MgwOptions SET OptionValue = '";
    strSQL += pcIPAddr;
    strSQL += "' WHERE OptionID=102 OR OptionID=110";

    iRet = pdboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateMgwOptionsIPAddress2DB() DB Oper Error:strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateMgwOptionsIPAddress2DB() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : InitAllGBDeviceRegStatus
 ��������  : ��ʼ�����е��豸ע��״̬
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��27�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void InitAllGBDeviceRegStatus()
{
    UpdateAllUserRegStatus2DB(0, &g_DBOper);
    UpdateAllGBDeviceRegStatus2DB(0, &g_DBOper);
    UpdateAllGBLogicDeviceRegStatus2DB(0, &g_DBOper);
    UpdateAllBoardConfigTableStatus(0, &g_DBOper);
    UpdateAllTopologyPhyDeviceStatus2DB((char*)"0", &g_DBOper);
    return;
}

/*****************************************************************************
 �� �� ��  : InitAllZRVDeviceRegStatus
 ��������  : ��ʼ�����е��豸ע��״̬
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��27�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void InitAllZRVDeviceRegStatus()
{
    UpdateAllZRVDeviceRegStatus2DB(0, &g_DBOper);
    DeleteCompressTaskFromDBForStart(&g_DBOper);
    return;
}

/*****************************************************************************
 �� �� ��  : InitAllUserRegStatus
 ��������  : ��ʼ�����е��û�ע��״̬
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��4��15�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void InitAllUserRegStatus()
{
    UpdateAllUserRegInfo2DB(0, &g_DBOper);
    DeleteAllUserRegInfoFromDB(&g_DBOper);
}

/*****************************************************************************
 �� �� ��  : SetBoardConfigTable
 ��������  : ���õ�����Ϣ�����ݿ�
 �������  : char* pcBoardID
              int iSlotID
              int iBoardType
              DBOper* ptDBoper
              int* iAssignRecord
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��3��1�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SetBoardConfigTable(char* pcBoardID, int iSlotID, int iBoardType, DBOper* ptDBoper, int* iAssignRecord)
{
    int iRet = 0;
    int record_count = -1;
    string strSQL = "";
    string strUpdateSQL = "";
    string strInsertSQL = "";
    char strSlotID[16] = {0};
    char strBoardType[16] = {0};
    char strBoardIndex[16] = {0};

    int iBoardIndex = 0;
    int iTmpSlotID = 0;
    int iTmpAssignRecord = 0;
    string strBoardID = "";
    string strCMSID = "";

    if (NULL == pcBoardID || NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "SetBoardConfigTable() exit---:  Param Error \r\n");
        return -1;
    }

    if (pcBoardID[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "SetBoardConfigTable() exit---:  BoardID NULL \r\n");
        return -1;
    }

    snprintf(strBoardType, 16, "%d", iBoardType);
    snprintf(strSlotID, 16, "%d", iSlotID);

    /* ��ȡ�������ñ�BoardConfig ������*/
    strSQL.clear();

    if (LOGIC_BOARD_CMS == iBoardType)
    {
        strSQL = "select * from BoardConfig";
        strSQL += " WHERE BoardType = ";
        strSQL += strBoardType;
    }
    else
    {
        strSQL = "select * from BoardConfig";
        strSQL += " WHERE BoardType = ";
        strSQL += strBoardType;
        strSQL += " AND BoardID like '";
        strSQL += pcBoardID;
        strSQL += "'";
    }

    record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "SetBoardConfigTable() record_count=%d, strSQL=%s \r\n", record_count, strSQL.c_str());

    if (record_count < 0) /* ���ݿ���� */
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardConfigTable() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0) /* û�м�¼ */
    {
        strInsertSQL.clear();
        strInsertSQL = "insert into BoardConfig (BoardType,Enable,SlotID,BoardID,CMSID,Status) values (";

        strInsertSQL += strBoardType;
        strInsertSQL += ",";

        strInsertSQL += "1";
        strInsertSQL += ",";

        strInsertSQL += strSlotID;
        strInsertSQL += ",";

        strInsertSQL += "'";
        strInsertSQL += pcBoardID;
        strInsertSQL += "'";
        strInsertSQL += ",";

        strInsertSQL += "'";
        strInsertSQL += local_cms_id_get();
        strInsertSQL += "'";
        strInsertSQL += ",";

        strInsertSQL += "1";

        strInsertSQL += ")";

        iRet = ptDBoper->DB_Insert("", "", strInsertSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardConfigTable() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
            return -1;
        }

        record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

        if (record_count < 0) /* ���ݿ���� */
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardConfigTable() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
            return -1;
        }
        else if (record_count == 0) /* û�в鵽��¼ */
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "SetBoardConfigTable() exit---: No Record Count \r\n");
            return -1;
        }

        /* ��ȡ���ݿ��¼��Ϣ */
        ptDBoper->GetFieldValue("ID", iBoardIndex);
        ptDBoper->GetFieldValue("AssignRecord", iTmpAssignRecord);
    }
    else
    {
        /* ��ȡ���ݿ��¼��Ϣ */
        ptDBoper->GetFieldValue("ID", iBoardIndex);
        ptDBoper->GetFieldValue("SlotID", iTmpSlotID);
        ptDBoper->GetFieldValue("BoardID", strBoardID);
        ptDBoper->GetFieldValue("CMSID", strCMSID);
        ptDBoper->GetFieldValue("AssignRecord", iTmpAssignRecord);

        if (iTmpSlotID != iSlotID
            || 0 != sstrcmp((char*)strBoardID.c_str(), pcBoardID)
            || 0 != sstrcmp((char*)strCMSID.c_str(), local_cms_id_get())) /* �б仯�����µ����ݿ� */
        {
            snprintf(strBoardIndex, 16, "%d", iBoardIndex);

            strUpdateSQL.clear();
            strUpdateSQL = "UPDATE BoardConfig SET";

            strUpdateSQL += " SlotID = ";
            strUpdateSQL += strSlotID;

            strUpdateSQL += ",";

            strUpdateSQL += " BoardID = ";
            strUpdateSQL += "'";
            strUpdateSQL += pcBoardID;
            strUpdateSQL += "'";

            strUpdateSQL += ",";

            strUpdateSQL += " CMSID = ";
            strUpdateSQL += "'";
            strUpdateSQL += local_cms_id_get();
            strUpdateSQL += "'";

            strUpdateSQL += " WHERE ID = ";
            strUpdateSQL += strBoardIndex;

            iRet = ptDBoper->DB_Update(strUpdateSQL.c_str(), 1);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardConfigTable() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
                return -1;
            }
        }
    }

    *iAssignRecord = iTmpAssignRecord;
    return iBoardIndex;
}

/*****************************************************************************
 �� �� ��  : UpdateBoardConfigTableStatus
 ��������  : ���µ������ñ��״̬�ֶ�
 �������  : char* pcBoardID
             int iBoardType
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int UpdateBoardConfigTableStatus(char* pcBoardID, int iBoardType, int iStatus, DBOper* ptDBoper)
{
    int iRet = 0;
    string strUpdateSQL = "";
    char strBoardType[16] = {0};
    char strStatus[16] = {0};

    if (NULL == pcBoardID || NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "UpdateBoardConfigTableStatus() exit---:  Param Error \r\n");
        return -1;
    }

    if (pcBoardID[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "UpdateBoardConfigTableStatus() exit---:  BoardID NULL \r\n");
        return -1;
    }

    snprintf(strBoardType, 16, "%d", iBoardType);
    snprintf(strStatus, 16, "%d", iStatus);

    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE BoardConfig SET";

    strUpdateSQL += " Status=";
    strUpdateSQL += strStatus;

    strUpdateSQL += " WHERE BoardID like '";
    strUpdateSQL += pcBoardID;
    strUpdateSQL += "' AND BoardType = ";
    strUpdateSQL += strBoardType;

    iRet = ptDBoper->DB_Update(strUpdateSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateBoardConfigTableStatus() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateBoardConfigTableStatus() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : UpdateAllBoardConfigTableStatus
 ��������  : ���µ������ñ��״̬�����ֶ�
 �������  : int iStatus
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int UpdateAllBoardConfigTableStatus(int iStatus, DBOper* ptDBoper)
{
    int iRet = 0;
    string strUpdateSQL = "";
    char strStatus[16] = {0};

    if (NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "UpdateAllBoardConfigTableStatus() exit---:  Param Error \r\n");
        return -1;
    }

    snprintf(strStatus, 16, "%d", iStatus);

    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE BoardConfig SET";

    strUpdateSQL += " Status=";
    strUpdateSQL += strStatus;

    iRet = ptDBoper->DB_Update(strUpdateSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateAllBoardConfigTableStatus() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateAllBoardConfigTableStatus() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : SetBoardNetConfigTable
 ��������  : ���õ����������Ϣ
 �������  : int iBoardIndex
             int iIPEth
             char* pcIPAddr
             int iIPUsedFlag
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��3��1��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SetBoardNetConfigTable(int iBoardIndex, int iIPEth, char* pcIPAddr, int iIPUsedFlag, DBOper* ptDBoper)
{
    int iRet = 0;
    int record_count = -1;
    string strSQL = "";
    string strInsertSQL = "";
    string strUpdateSQL = "";
    string strDeleteSQL = "";
    char strBoardIndex[16] = {0};
    char strIPEth[16] = {0};
    char strIPUsedFlag[16] = {0};
    string strIP = "";
    string strCMSID = "";
    int iEnable = 0;

    if (iBoardIndex <= 0 || NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "SetBoardNetConfigTable() exit---:  Param Error \r\n");
        return -1;
    }

    snprintf(strBoardIndex, 16, "%d", iBoardIndex);
    snprintf(strIPEth, 16, "%d", iIPEth);
    snprintf(strIPUsedFlag, 16, "%d", iIPUsedFlag);

    /* ��ȡ�����������ñ�BoardNetConfig �豸��������*/
    strSQL.clear();
    strSQL = "select * from BoardNetConfig";
    strSQL += " WHERE BoardIndex = ";
    strSQL += strBoardIndex;
    strSQL += " AND EthID = ";
    strSQL += strIPEth;

    record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "SetBoardNetConfigTable() record_count=%d \r\n", record_count);

    if (record_count < 0) /* ���ݿ���� */
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardNetConfigTable() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardNetConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0) /* û�м�¼ */
    {
        if (NULL == pcIPAddr || pcIPAddr[0] == '\0')
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardNetConfigTable() exit---: Not Config Net IP \r\n");
        }
        else /* �����µ����� */
        {
            strInsertSQL.clear();
            strInsertSQL = "insert into BoardNetConfig (BoardIndex,CMSID,Enable,EthID,IP,Status) values (";

            strInsertSQL += strBoardIndex;
            strInsertSQL += ",";

            strInsertSQL += "'";
            strInsertSQL += local_cms_id_get();
            strInsertSQL += "'";
            strInsertSQL += ",";

            strInsertSQL += strIPUsedFlag;
            strInsertSQL += ",";

            strInsertSQL += strIPEth;
            strInsertSQL += ",";

            strInsertSQL += "'";
            strInsertSQL += pcIPAddr;
            strInsertSQL += "'";
            strInsertSQL += ",";

            strInsertSQL += "1";

            strInsertSQL += ")";

            iRet = ptDBoper->DB_Insert("", "", strInsertSQL.c_str(), 1);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardNetConfigTable() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardNetConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
                return -1;
            }
        }
    }
    else
    {
        if (NULL == pcIPAddr || pcIPAddr[0] == '\0')/* �� IP ��ַɾ�� */
        {
            strDeleteSQL.clear();
            strDeleteSQL = "delete from BoardNetConfig";
            strDeleteSQL += " WHERE BoardIndex = ";
            strDeleteSQL += strBoardIndex;
            strDeleteSQL += " AND EthID = ";
            strDeleteSQL += strIPEth;

            iRet = ptDBoper->DB_Delete(strDeleteSQL.c_str(), 1);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardNetConfigTable() DB Oper Error:strDeleteSQL=%s, iRet=%d \r\n", strDeleteSQL.c_str(), iRet);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardNetConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
                return -1;
            }
        }
        else /* ���Ƿ���Ҫ����IP��ַ */
        {
            strIP.clear();
            ptDBoper->GetFieldValue("IP", strIP);
            ptDBoper->GetFieldValue("Enable", iEnable);
            ptDBoper->GetFieldValue("CMSID", strCMSID);

            if (strIP.empty()
                || (!strIP.empty() && 0 != sstrcmp((char*)strIP.c_str(), pcIPAddr))
                || (!strCMSID.empty() && 0 != sstrcmp((char*)strCMSID.c_str(), local_cms_id_get()))
                || iEnable != iIPUsedFlag) /* �б仯����Ҫ�������ݿ� */
            {
                strUpdateSQL.clear();
                strUpdateSQL = "UPDATE BoardNetConfig SET";

                strUpdateSQL += " IP = ";
                strUpdateSQL += "'";
                strUpdateSQL += pcIPAddr;
                strUpdateSQL += "'";

                strUpdateSQL += ",";

                strUpdateSQL += " CMSID = ";
                strUpdateSQL += "'";
                strUpdateSQL += local_cms_id_get();
                strUpdateSQL += "'";

                strUpdateSQL += ",";

                strUpdateSQL += " Enable = ";
                strUpdateSQL += strIPUsedFlag;

                strUpdateSQL += " WHERE BoardIndex = ";
                strUpdateSQL += strBoardIndex;
                strUpdateSQL += " AND EthID = ";
                strUpdateSQL += strIPEth;

                iRet = ptDBoper->DB_Update(strUpdateSQL.c_str(), 1);

                if (iRet < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardNetConfigTable() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardNetConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
                    return -1;
                }
            }
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : SetCMSBoardInfoToDB
 ��������  : ����CMS�ĵ��弰����������Ϣ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��3��1�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SetCMSBoardInfoToDB()
{
    int i = 0;
    int iEthNum = 0;
    ip_pair_t* pIPaddr = NULL;
    char* tmp = NULL;

    int iRet = 0;
    int iBoardIndex = 0;
    char* local_ip = NULL;
    char strEthName[MAX_IP_LEN] = {0};
    char strDeviceType[16] = {0};
    int iAssignRecord = 0;

    iBoardIndex = SetBoardConfigTable(local_cms_id_get(), 0, LOGIC_BOARD_CMS, &g_DBOper, &iAssignRecord);

    if (iBoardIndex > 0)
    {
        pGblconf->board_index = iBoardIndex;

        for (i = 0; i < osip_list_size(pGblconf->pLocalIPAddrList); i++)
        {
            pIPaddr = (ip_pair_t*)osip_list_get(pGblconf->pLocalIPAddrList, i);

            if (NULL == pIPaddr || pIPaddr->local_ip[0] == '\0')
            {
                continue;
            }

            if (0 == strncmp(pIPaddr->eth_name, (char*)"eth", 3))
            {
                tmp = &pIPaddr->eth_name[3];
                iEthNum = osip_atoi(tmp);

                iRet = SetBoardNetConfigTable(iBoardIndex, iEthNum, pIPaddr->local_ip, 1, &g_DBOper);
            }
            else if (0 == strncmp(pIPaddr->eth_name, (char*)"bond", 4)) /* �����ڵ�֧�� */
            {
                tmp = &pIPaddr->eth_name[4];
                iEthNum = osip_atoi(tmp);

                iRet = SetBoardNetConfigTable(iBoardIndex, iEthNum, pIPaddr->local_ip, 1, &g_DBOper);
            }
            else if (0 == strncmp(pIPaddr->eth_name, (char*)"wlan", 4)) /* �������ڵ�֧�� */
            {
                tmp = &pIPaddr->eth_name[4];
                iEthNum = osip_atoi(tmp);

                iRet = SetBoardNetConfigTable(iBoardIndex, iEthNum, pIPaddr->local_ip, 1, &g_DBOper);
            }
        }
    }
    else
    {
        return -1;
    }

    /* ���µ�������״̬ */
    iRet = UpdateBoardConfigTableStatus(local_cms_id_get(), LOGIC_BOARD_CMS, 1, &g_DBOper);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetCMSBoardInfoToDB() UpdateBoardConfigTableStatus Error \r\n");
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "SetCMSBoardInfoToDB() UpdateBoardConfigTableStatus OK \r\n");
    }

    /* ������˽ṹ����Ϣ */
    local_ip = local_ip_get(default_eth_name_get());
    snprintf(strDeviceType, 16, "%u", EV9000_DEVICETYPE_SIPSERVER);
    iRet = AddTopologyPhyDeviceInfo2DB(local_cms_id_get(), local_cms_name_get(), strDeviceType, local_ip, (char*)"1", local_cms_id_get(), (char*)"0", &g_DBOper);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetCMSBoardInfoToDB() AddTopologyPhyDeviceInfo2DB Error \r\n");
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "SetCMSBoardInfoToDB() AddTopologyPhyDeviceInfo2DB OK \r\n");
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : InsertDefaultDBData
 ��������  : �������ݿ�Ĭ������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��5��16�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void InsertDefaultDBData()
{
    int iRet = 0;

    /* ����Ĭ�ϵ�����������뵽���ݿ� */
    iRet = InsertGBCodesDefautConfig(&g_DBOper);

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "InsertDefaultDBData() InsertGBCodesDefautConfig:iRet=%d \r\n", iRet);

    iRet = InsertDefaultUserInfo((char*)"WiscomV", &g_DBOper);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "InsertDefaultDBData() InsertDefaultUserInfo:iRet=%d \r\n", iRet);

    iRet = InsertDefaultUserInfo((char*)"admin", &g_DBOper);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "InsertDefaultDBData() InsertDefaultUserInfo:iRet=%d \r\n", iRet);

    return;
}

/*****************************************************************************
 �� �� ��  : DeleteUnnecessaryDBData
 ��������  : ɾ������Ҫ�����ݿ�����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��8��12��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void DeleteUnnecessaryDBData()
{
    int iRet = 0;
    string strSQL = "";

    return;
}

/*****************************************************************************
 �� �� ��  : InsertGBCodesDefautConfig
 ��������  : ����Ĭ�ϵ�����������뵽���ݿ�
 �������  : DBOper* pGBCodes_Info_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��5��16�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int InsertGBCodesDefautConfig(DBOper* pGBCodes_Info_dboper)
{
    int iRet = 0;
    string strInsertSQL = "";
    string strSQL = "";

    if (NULL == pGBCodes_Info_dboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "InsertGBCodesDefautConfig() exit---: Param Error \r\n");
        return -1;
    }

    /* ���� */
    strSQL.clear();
    strSQL = "CREATE TABLE GBCodes ( \
			 ID INTEGER UNSIGNED  NOT NULL PRIMARY KEY AUTO_INCREMENT, \
			 Code varchar(36) NOT NULL, \
			 Name varchar(36) NOT NULL, \
			 Resved1 int(10) unsigned DEFAULT NULL, \
			 Resved2 varchar(32) DEFAULT NULL \
		);";

    iRet = pGBCodes_Info_dboper->DBExecute(strSQL.c_str());

    if (iRet >= 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "InsertGBCodesDefautConfig() CREATE TABLE [GBCodes] Success \r\n");
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "InsertGBCodesDefautConfig() CREATE TABLE [GBCodes] Error, iRet=%d, Reason=%s\r\n", iRet, g_DBOper.GetLastDbErrorMsg());
    }

    /* ����SQL��� */
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3523,'110000','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3524,'110100','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3525,'110101','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3526,'110102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3527,'110105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3528,'110106','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3529,'110107','ʯ��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3530,'110108','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3531,'110109','��ͷ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3532,'110111','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3533,'110112','ͨ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3534,'110113','˳����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3535,'110114','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3536,'110115','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3537,'110116','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3538,'110117','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3539,'110200','��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3540,'110228','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3541,'110229','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3542,'120000','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3543,'120100','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3544,'120101','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3545,'120102','�Ӷ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3546,'120103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3547,'120104','�Ͽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3548,'120105','�ӱ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3549,'120106','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3550,'120110','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3551,'120111','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3552,'120112','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3553,'120113','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3554,'120114','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3555,'120115','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3556,'120116','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3557,'120200','��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3558,'120221','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3559,'120223','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3560,'120225','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3561,'130000','�ӱ�ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3562,'130100','ʯ��ׯ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3563,'130101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3564,'130102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3565,'130103','�Ŷ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3566,'130104','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3567,'130105','�»���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3568,'130107','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3569,'130108','ԣ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3570,'130121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3571,'130123','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3572,'130124','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3573,'130125','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3574,'130126','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3575,'130127','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3576,'130128','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3577,'130129','�޻���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3578,'130130','�޼���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3579,'130131','ƽɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3580,'130132','Ԫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3581,'130133','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3582,'130181','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3583,'130182','޻����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3584,'130183','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3585,'130184','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3586,'130185','¹Ȫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3587,'130200','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3588,'130201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3589,'130202','·����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3590,'130203','·����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3591,'130204','��ұ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3592,'130205','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3593,'130207','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3594,'130208','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3595,'130209','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3596,'130223','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3597,'130224','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3598,'130225','��ͤ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3599,'130227','Ǩ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3600,'130229','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3601,'130281','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3602,'130283','Ǩ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3603,'130300','�ػʵ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3604,'130301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3605,'130302','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3606,'130303','ɽ������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3607,'130304','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3608,'130321','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3609,'130322','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3610,'130323','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3611,'130324','¬����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3612,'130400','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3613,'130401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3614,'130402','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3615,'130403','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3616,'130404','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3617,'130406','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3618,'130421','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3619,'130423','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3620,'130424','�ɰ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3621,'130425','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3622,'130426','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3623,'130427','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3624,'130428','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3625,'130429','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3626,'130430','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3627,'130431','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3628,'130432','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3629,'130433','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3630,'130434','κ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3631,'130435','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3632,'130481','�䰲��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3633,'130500','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3634,'130501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3635,'130502','�Ŷ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3636,'130503','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3637,'130521','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3638,'130522','�ٳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3639,'130523','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3640,'130524','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3641,'130525','¡Ң��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3642,'130526','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3643,'130527','�Ϻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3644,'130528','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3645,'130529','��¹��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3646,'130530','�º���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3647,'130531','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3648,'130532','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3649,'130533','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3650,'130534','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3651,'130535','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3652,'130581','�Ϲ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3653,'130582','ɳ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3654,'130600','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3655,'130601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3656,'130602','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3657,'130603','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3658,'130604','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3659,'130621','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3660,'130622','��Է��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3661,'130623','�ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3662,'130624','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3663,'130625','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3664,'130626','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3665,'130627','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3666,'130628','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3667,'130629','�ݳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3668,'130630','�Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3669,'130631','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3670,'130632','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3671,'130633','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3672,'130634','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3673,'130635','���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3674,'130636','˳ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3675,'130637','��Ұ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3676,'130638','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3677,'130681','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3678,'130682','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3679,'130683','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3680,'130684','�߱�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3681,'130700','�żҿ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3682,'130701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3683,'130702','�Ŷ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3684,'130703','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3685,'130705','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3686,'130706','�»�԰��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3687,'130721','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3688,'130722','�ű���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3689,'130723','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3690,'130724','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3691,'130725','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3692,'130726','ε��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3693,'130727','��ԭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3694,'130728','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3695,'130729','��ȫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3696,'130730','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3697,'130731','��¹��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3698,'130732','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3699,'130733','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3700,'130800','�е���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3701,'130801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3702,'130802','˫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3703,'130803','˫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3704,'130804','ӥ��Ӫ�ӿ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3705,'130821','�е���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3706,'130822','��¡��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3707,'130823','ƽȪ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3708,'130824','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3709,'130825','¡����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3710,'130826','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3711,'130827','�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3712,'130828','Χ�������ɹ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3713,'130900','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3714,'130901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3715,'130902','�»���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3716,'130903','�˺���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3717,'130921','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3718,'130922','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3719,'130923','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3720,'130924','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3721,'130925','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3722,'130926','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3723,'130927','��Ƥ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3724,'130928','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3725,'130929','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3726,'130930','�ϴ����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3727,'130981','��ͷ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3728,'130982','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3729,'130983','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3730,'130984','�Ӽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3731,'131000','�ȷ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3732,'131001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3733,'131002','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3734,'131003','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3735,'131022','�̰���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3736,'131023','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3737,'131024','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3738,'131025','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3739,'131026','�İ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3740,'131028','�󳧻���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3741,'131081','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3742,'131082','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3743,'131100','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3744,'131101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3745,'131102','�ҳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3746,'131121','��ǿ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3747,'131122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3748,'131123','��ǿ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3749,'131124','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3750,'131125','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3751,'131126','�ʳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3752,'131127','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3753,'131128','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3754,'131181','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3755,'131182','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3756,'140000','ɽ��ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3757,'140100','̫ԭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3758,'140101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3759,'140105','С����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3760,'140106','ӭ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3761,'140107','�ӻ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3762,'140108','���ƺ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3763,'140109','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3764,'140110','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3765,'140121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3766,'140122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3767,'140123','¦����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3768,'140181','�Ž���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3769,'140200','��ͬ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3770,'140201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3771,'140202','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3772,'140203','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3773,'140211','�Ͻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3774,'140212','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3775,'140221','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3776,'140222','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3777,'140223','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3778,'140224','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3779,'140225','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3780,'140226','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3781,'140227','��ͬ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3782,'140300','��Ȫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3783,'140301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3784,'140302','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3785,'140303','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3786,'140311','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3787,'140321','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3788,'140322','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3789,'140400','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3790,'140401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3791,'140402','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3792,'140411','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3793,'140421','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3794,'140423','��ԫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3795,'140424','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3796,'140425','ƽ˳��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3797,'140426','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3798,'140427','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3799,'140428','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3800,'140429','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3801,'140430','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3802,'140431','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3803,'140481','º����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3804,'140500','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3805,'140501','��������Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3806,'140502','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3807,'140521','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3808,'140522','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3809,'140524','�괨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3810,'140525','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3811,'140581','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3812,'140600','˷����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3813,'140601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3814,'140602','˷����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3815,'140603','ƽ³��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3816,'140621','ɽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3817,'140622','Ӧ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3818,'140623','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3819,'140624','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3820,'140700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3821,'140701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3822,'140702','�ܴ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3823,'140721','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3824,'140722','��Ȩ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3825,'140723','��˳��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3826,'140724','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3827,'140725','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3828,'140726','̫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3829,'140727','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3830,'140728','ƽң��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3831,'140729','��ʯ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3832,'140781','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3833,'140800','�˳���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3834,'140801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3835,'140802','�κ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3836,'140821','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3837,'140822','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3838,'140823','��ϲ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3839,'140824','�ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3840,'140825','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3841,'140826','���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3842,'140827','ԫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3843,'140828','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3844,'140829','ƽ½��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3845,'140830','�ǳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3846,'140881','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3847,'140882','�ӽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3848,'140900','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3849,'140901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3850,'140902','�ø���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3851,'140921','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3852,'140922','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3853,'140923','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3854,'140924','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3855,'140925','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3856,'140926','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3857,'140927','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3858,'140928','��կ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3859,'140929','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3860,'140930','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3861,'140931','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3862,'140932','ƫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3863,'140981','ԭƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3864,'141000','�ٷ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3865,'141001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3866,'141002','Ң����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3867,'141021','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3868,'141022','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3869,'141023','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3870,'141024','�鶴��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3871,'141025','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3872,'141026','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3873,'141027','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3874,'141028','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3875,'141029','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3876,'141030','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3877,'141031','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3878,'141032','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3879,'141033','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3880,'141034','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3881,'141081','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3882,'141082','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3883,'141100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3884,'141101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3885,'141102','��ʯ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3886,'141121','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3887,'141122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3888,'141123','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3889,'141124','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3890,'141125','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3891,'141126','ʯ¥��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3892,'141127','���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3893,'141128','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3894,'141129','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3895,'141130','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3896,'141181','Т����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3897,'141182','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3898,'150000','���ɹ�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3899,'150100','���ͺ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3900,'150101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3901,'150102','�³���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3902,'150103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3903,'150104','��Ȫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3904,'150105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3905,'150121','��Ĭ������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3906,'150122','�п�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3907,'150123','���ָ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3908,'150124','��ˮ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3909,'150125','�䴨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3910,'150200','��ͷ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3911,'150201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3912,'150202','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3913,'150203','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3914,'150204','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3915,'150205','ʯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3916,'150206','���ƶ�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3917,'150207','��ԭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3918,'150221','��Ĭ������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3919,'150222','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3920,'150223','�����ï����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3921,'150300','�ں���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3922,'150301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3923,'150302','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3924,'150303','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3925,'150304','�ڴ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3926,'150400','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3927,'150401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3928,'150402','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3929,'150403','Ԫ��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3930,'150404','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3931,'150421','��³�ƶ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3932,'150422','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3933,'150423','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3934,'150424','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3935,'150425','��ʲ������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3936,'150426','��ţ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3937,'150428','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3938,'150429','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3939,'150430','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3940,'150500','ͨ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3941,'150501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3942,'150502','�ƶ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3943,'150521','�ƶ�����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3944,'150522','�ƶ����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3945,'150523','��³��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3946,'150524','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3947,'150525','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3948,'150526','��³����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3949,'150581','���ֹ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3950,'150600','������˹��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3951,'150601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3952,'150602','��ʤ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3953,'150621','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3954,'150622','׼�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3955,'150623','���п�ǰ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3956,'150624','���п���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3957,'150625','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3958,'150626','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3959,'150627','���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3960,'150700','���ױ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3961,'150701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3962,'150702','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3963,'150721','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3964,'150722','Ī�����ߴ��Ӷ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3965,'150723','���״�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3966,'150724','���¿���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3967,'150725','�°Ͷ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3968,'150726','�°Ͷ�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3969,'150727','�°Ͷ�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3970,'150781','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3971,'150782','����ʯ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3972,'150783','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3973,'150784','���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3974,'150785','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3975,'150800','�����׶���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3976,'150801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3977,'150802','�ٺ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3978,'150821','��ԭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3979,'150822','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3980,'150823','������ǰ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3981,'150824','����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3982,'150825','�����غ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3983,'150826','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3984,'150900','�����첼��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3985,'150901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3986,'150902','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3987,'150921','׿����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3988,'150922','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3989,'150923','�̶���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3990,'150924','�˺���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3991,'150925','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3992,'150926','���������ǰ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3993,'150927','�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3994,'150928','������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3995,'150929','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3996,'150981','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3997,'152200','�˰���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3998,'152201','����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3999,'152202','����ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4000,'152221','�ƶ�������ǰ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4001,'152222','�ƶ�����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4002,'152223','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4003,'152224','ͻȪ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4004,'152500','���ֹ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4005,'152501','����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4006,'152502','���ֺ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4007,'152522','���͸���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4008,'152523','����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4009,'152524','����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4010,'152525','������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4011,'152526','������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4012,'152527','̫������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4013,'152528','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4014,'152529','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4015,'152530','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4016,'152531','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4017,'152900','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4018,'152921','����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4019,'152922','����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4020,'152923','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4021,'210000','����ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4022,'210100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4023,'210101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4024,'210102','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4025,'210103','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4026,'210104','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4027,'210105','�ʹ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4028,'210106','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4029,'210111','�ռ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4030,'210112','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4031,'210113','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4032,'210114','�ں���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4033,'210122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4034,'210123','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4035,'210124','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4036,'210181','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4037,'210200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4038,'210201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4039,'210202','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4040,'210203','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4041,'210204','ɳ�ӿ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4042,'210211','�ʾ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4043,'210212','��˳����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4044,'210213','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4045,'210224','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4046,'210281','�߷�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4047,'210282','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4048,'210283','ׯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4049,'210300','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4050,'210301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4051,'210302','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4052,'210303','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4053,'210304','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4054,'210311','ǧɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4055,'210321','̨����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4056,'210323','�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4057,'210381','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4058,'210400','��˳��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4059,'210401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4060,'210402','�¸���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4061,'210403','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4062,'210404','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4063,'210411','˳����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4064,'210421','��˳��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4065,'210422','�±�����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4066,'210423','��ԭ����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4067,'210500','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4068,'210501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4069,'210502','ƽɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4070,'210503','Ϫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4071,'210504','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4072,'210505','�Ϸ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4073,'210521','��Ϫ����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4074,'210522','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4075,'210600','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4076,'210601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4077,'210602','Ԫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4078,'210603','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4079,'210604','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4080,'210624','�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4081,'210681','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4082,'210682','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4083,'210700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4084,'210701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4085,'210702','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4086,'210703','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4087,'210711','̫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4088,'210726','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4089,'210727','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4090,'210781','�躣��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4091,'210782','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4092,'210800','Ӫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4093,'210801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4094,'210802','վǰ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4095,'210803','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4096,'210804','����Ȧ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4097,'210811','�ϱ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4098,'210881','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4099,'210882','��ʯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4100,'210900','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4101,'210901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4102,'210902','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4103,'210903','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4104,'210904','̫ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4105,'210905','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4106,'210911','ϸ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4107,'210921','�����ɹ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4108,'210922','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4109,'211000','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4110,'211001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4111,'211002','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4112,'211003','��ʥ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4113,'211004','��ΰ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4114,'211005','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4115,'211011','̫�Ӻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4116,'211021','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4117,'211081','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4118,'211100','�̽���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4119,'211101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4120,'211102','˫̨����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4121,'211103','��¡̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4122,'211121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4123,'211122','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4124,'211200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4125,'211201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4126,'211202','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4127,'211204','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4128,'211221','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4129,'211223','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4130,'211224','��ͼ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4131,'211281','����ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4132,'211282','��ԭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4133,'211300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4134,'211301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4135,'211302','˫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4136,'211303','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4137,'211321','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4138,'211322','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4139,'211324','�����������ɹ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4140,'211381','��Ʊ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4141,'211382','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4142,'211400','��«����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4143,'211401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4144,'211402','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4145,'211403','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4146,'211404','��Ʊ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4147,'211421','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4148,'211422','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4149,'211481','�˳���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4150,'220000','����ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4151,'220100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4152,'220101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4153,'220102','�Ϲ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4154,'220103','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4155,'220104','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4156,'220105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4157,'220106','��԰��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4158,'220112','˫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4159,'220122','ũ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4160,'220181','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4161,'220182','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4162,'220183','�»���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4163,'220200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4164,'220201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4165,'220202','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4166,'220203','��̶��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4167,'220204','��Ӫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4168,'220211','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4169,'220221','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4170,'220281','�Ժ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4171,'220282','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4172,'220283','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4173,'220284','��ʯ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4174,'220300','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4175,'220301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4176,'220302','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4177,'220303','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4178,'220322','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4179,'220323','��ͨ����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4180,'220381','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4181,'220382','˫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4182,'220400','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4183,'220401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4184,'220402','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4185,'220403','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4186,'220421','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4187,'220422','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4188,'220500','ͨ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4189,'220501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4190,'220502','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4191,'220503','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4192,'220521','ͨ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4193,'220523','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4194,'220524','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4195,'220581','÷�ӿ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4196,'220582','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4197,'220600','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4198,'220601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4199,'220602','�뽭��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4200,'220605','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4201,'220621','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4202,'220622','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4203,'220623','���׳�����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4204,'220681','�ٽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4205,'220700','��ԭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4206,'220701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4207,'220702','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4208,'220721','ǰ������˹�ɹ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4209,'220722','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4210,'220723','Ǭ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4211,'220724','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4212,'220800','�׳���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4213,'220801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4214,'220802','䬱���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4215,'220821','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4216,'220822','ͨ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4217,'220881','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4218,'220882','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4219,'222400','�ӱ߳�����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4220,'222401','�Ӽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4221,'222402','ͼ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4222,'222403','�ػ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4223,'222404','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4224,'222405','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4225,'222406','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4226,'222424','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4227,'222426','��ͼ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4228,'230000','������ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4229,'230100','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4230,'230101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4231,'230102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4232,'230103','�ϸ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4233,'230104','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4234,'230108','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4235,'230109','�ɱ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4236,'230110','�㷻��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4237,'230111','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4238,'230112','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4239,'230123','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4240,'230124','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4241,'230125','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4242,'230126','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4243,'230127','ľ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4244,'230128','ͨ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4245,'230129','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4246,'230182','˫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4247,'230183','��־��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4248,'230184','�峣��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4249,'230200','���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4250,'230201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4251,'230202','��ɳ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4252,'230203','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4253,'230204','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4254,'230205','����Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4255,'230206','����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4256,'230207','����ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4257,'230208','÷��˹���Ӷ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4258,'230221','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4259,'230223','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4260,'230224','̩����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4261,'230225','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4262,'230227','��ԣ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4263,'230229','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4264,'230230','�˶���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4265,'230231','��Ȫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4266,'230281','ګ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4267,'230300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4268,'230301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4269,'230302','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4270,'230303','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4271,'230304','�ε���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4272,'230305','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4273,'230306','���Ӻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4274,'230307','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4275,'230321','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4276,'230381','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4277,'230382','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4278,'230400','�׸���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4279,'230401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4280,'230402','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4281,'230403','��ũ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4282,'230404','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4283,'230405','�˰���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4284,'230406','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4285,'230407','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4286,'230421','�ܱ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4287,'230422','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4288,'230500','˫Ѽɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4289,'230501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4290,'230502','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4291,'230503','�붫��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4292,'230505','�ķ�̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4293,'230506','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4294,'230521','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4295,'230522','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4296,'230523','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4297,'230524','�ĺ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4298,'230600','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4299,'230601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4300,'230602','����ͼ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4301,'230603','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4302,'230604','�ú�·��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4303,'230605','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4304,'230606','��ͬ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4305,'230621','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4306,'230622','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4307,'230623','�ֵ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4308,'230624','�Ŷ������ɹ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4309,'230700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4310,'230701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4311,'230702','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4312,'230703','�ϲ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4313,'230704','�Ѻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4314,'230705','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4315,'230706','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4316,'230707','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4317,'230708','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4318,'230709','��ɽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4319,'230710','��Ӫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4320,'230711','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4321,'230712','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4322,'230713','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4323,'230714','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4324,'230715','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4325,'230716','�ϸ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4326,'230722','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4327,'230781','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4328,'230800','��ľ˹��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4329,'230801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4330,'230803','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4331,'230804','ǰ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4332,'230805','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4333,'230811','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4334,'230822','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4335,'230826','�봨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4336,'230828','��ԭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4337,'230833','��Զ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4338,'230881','ͬ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4339,'230882','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4340,'230900','��̨����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4341,'230901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4342,'230902','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4343,'230903','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4344,'230904','���Ӻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4345,'230921','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4346,'231000','ĵ������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4347,'231001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4348,'231002','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4349,'231003','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4350,'231004','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4351,'231005','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4352,'231024','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4353,'231025','�ֿ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4354,'231081','��Һ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4355,'231083','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4356,'231084','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4357,'231085','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4358,'231100','�ں���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4359,'231101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4360,'231102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4361,'231121','�۽���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4362,'231123','ѷ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4363,'231124','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4364,'231181','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4365,'231182','���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4366,'231200','�绯��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4367,'231201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4368,'231202','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4369,'231221','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4370,'231222','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4371,'231223','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4372,'231224','�찲��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4373,'231225','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4374,'231226','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4375,'231281','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4376,'231282','�ض���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4377,'231283','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4378,'232700','���˰������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4379,'232721','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4380,'232722','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4381,'232723','Į����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4382,'310000','�Ϻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4383,'310100','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4384,'310101','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4385,'310104','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4386,'310105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4387,'310106','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4388,'310107','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4389,'310108','բ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4390,'310109','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4391,'310110','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4392,'310112','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4393,'310113','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4394,'310114','�ζ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4395,'310115','�ֶ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4396,'310116','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4397,'310117','�ɽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4398,'310118','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4399,'310120','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4400,'310200','��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4401,'310230','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4402,'320000','����ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4403,'320100','�Ͼ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4404,'320101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4405,'320102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4406,'320103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4407,'320104','�ػ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4408,'320105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4409,'320106','��¥��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4410,'320107','�¹���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4411,'320111','�ֿ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4412,'320113','��ϼ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4413,'320114','�껨̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4414,'320115','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4415,'320116','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4416,'320124','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4417,'320125','�ߴ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4418,'320200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4419,'320201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4420,'320202','�簲��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4421,'320203','�ϳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4422,'320204','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4423,'320205','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4424,'320206','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4425,'320211','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4426,'320281','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4427,'320282','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4428,'320300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4429,'320301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4430,'320302','��¥��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4431,'320303','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4432,'320305','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4433,'320311','Ȫɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4434,'320312','ͭɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4435,'320321','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4436,'320322','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4437,'320324','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4438,'320381','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4439,'320382','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4440,'320400','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4441,'320401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4442,'320402','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4443,'320404','��¥��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4444,'320405','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4445,'320411','�±���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4446,'320412','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4447,'320481','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4448,'320482','��̳��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4449,'320500','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4450,'320501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4451,'320505','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4452,'320506','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4453,'320507','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4454,'320508','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4455,'320509','�⽭��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4456,'320581','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4457,'320582','�żҸ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4458,'320583','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4459,'320585','̫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4460,'320600','��ͨ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4461,'320601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4462,'320602','�紨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4463,'320611','��բ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4464,'320612','ͨ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4465,'320621','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4466,'320623','�綫��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4467,'320681','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4468,'320682','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4469,'320684','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4470,'320700','���Ƹ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4471,'320701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4472,'320703','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4473,'320705','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4474,'320706','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4475,'320721','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4476,'320722','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4477,'320723','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4478,'320724','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4479,'320800','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4480,'320801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4481,'320802','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4482,'320803','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4483,'320804','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4484,'320811','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4485,'320826','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4486,'320829','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4487,'320830','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4488,'320831','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4489,'320900','�γ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4490,'320901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4491,'320902','ͤ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4492,'320903','�ζ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4493,'320921','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4494,'320922','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4495,'320923','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4496,'320924','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4497,'320925','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4498,'320981','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4499,'320982','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4500,'321000','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4501,'321001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4502,'321002','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4503,'321003','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4504,'321012','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4505,'321023','��Ӧ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4506,'321081','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4507,'321084','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4508,'321100','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4509,'321101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4510,'321102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4511,'321111','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4512,'321112','��ͽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4513,'321181','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4514,'321182','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4515,'321183','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4516,'321200','̩����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4517,'321201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4518,'321202','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4519,'321203','�߸���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4520,'321281','�˻���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4521,'321282','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4522,'321283','̩����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4523,'321284','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4524,'321300','��Ǩ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4525,'321301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4526,'321302','�޳���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4527,'321311','��ԥ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4528,'321322','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4529,'321323','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4530,'321324','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4531,'330000','�㽭ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4532,'330100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4533,'330101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4534,'330102','�ϳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4535,'330103','�³���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4536,'330104','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4537,'330105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4538,'330106','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4539,'330108','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4540,'330109','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4541,'330110','�ຼ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4542,'330122','ͩ®��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4543,'330127','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4544,'330182','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4545,'330183','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4546,'330185','�ٰ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4547,'330200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4548,'330201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4549,'330203','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4550,'330204','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4551,'330205','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4552,'330206','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4553,'330211','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4554,'330212','۴����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4555,'330225','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4556,'330226','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4557,'330281','��Ҧ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4558,'330282','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4559,'330283','���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4560,'330300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4561,'330301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4562,'330302','¹����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4563,'330303','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4564,'330304','걺���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4565,'330322','��ͷ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4566,'330324','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4567,'330326','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4568,'330327','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4569,'330328','�ĳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4570,'330329','̩˳��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4571,'330381','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4572,'330382','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4573,'330400','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4574,'330401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4575,'330402','�Ϻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4576,'330411','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4577,'330421','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4578,'330424','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4579,'330481','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4580,'330482','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4581,'330483','ͩ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4582,'330500','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4583,'330501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4584,'330502','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4585,'330503','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4586,'330521','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4587,'330522','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4588,'330523','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4589,'330600','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4590,'330601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4591,'330602','Խ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4592,'330621','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4593,'330624','�²���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4594,'330681','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4595,'330682','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4596,'330683','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4597,'330700','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4598,'330701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4599,'330702','�ĳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4600,'330703','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4601,'330723','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4602,'330726','�ֽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4603,'330727','�Ͱ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4604,'330781','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4605,'330782','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4606,'330783','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4607,'330784','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4608,'330800','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4609,'330801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4610,'330802','�³���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4611,'330803','�齭��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4612,'330822','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4613,'330824','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4614,'330825','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4615,'330881','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4616,'330900','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4617,'330901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4618,'330902','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4619,'330903','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4620,'330921','�ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4621,'330922','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4622,'331000','̨����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4623,'331001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4624,'331002','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4625,'331003','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4626,'331004','·����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4627,'331021','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4628,'331022','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4629,'331023','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4630,'331024','�ɾ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4631,'331081','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4632,'331082','�ٺ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4633,'331100','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4634,'331101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4635,'331102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4636,'331121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4637,'331122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4638,'331123','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4639,'331124','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4640,'331125','�ƺ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4641,'331126','��Ԫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4642,'331127','�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4643,'331181','��Ȫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4644,'340000','����ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4645,'340100','�Ϸ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4646,'340101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4647,'340102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4648,'340103','®����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4649,'340104','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4650,'340111','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4651,'340121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4652,'340122','�ʶ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4653,'340123','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4654,'340124','®����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4655,'340181','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4656,'340200','�ߺ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4657,'340201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4658,'340202','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4659,'340203','߮����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4660,'340207','𯽭��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4661,'340208','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4662,'340221','�ߺ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4663,'340222','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4664,'340223','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4665,'340225','��Ϊ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4666,'340300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4667,'340301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4668,'340302','���Ӻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4669,'340303','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4670,'340304','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4671,'340311','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4672,'340321','��Զ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4673,'340322','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4674,'340323','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4675,'340400','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4676,'340401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4677,'340402','��ͨ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4678,'340403','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4679,'340404','л�Ҽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4680,'340405','�˹�ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4681,'340406','�˼���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4682,'340421','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4683,'340500','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4684,'340501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4685,'340503','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4686,'340504','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4687,'340506','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4688,'340521','��Ϳ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4689,'340522','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4690,'340523','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4691,'340600','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4692,'340601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4693,'340602','�ż���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4694,'340603','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4695,'340604','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4696,'340621','�Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4697,'340700','ͭ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4698,'340701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4699,'340702','ͭ��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4700,'340703','ʨ��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4701,'340711','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4702,'340721','ͭ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4703,'340800','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4704,'340801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4705,'340802','ӭ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4706,'340803','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4707,'340811','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4708,'340822','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4709,'340823','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4710,'340824','Ǳɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4711,'340825','̫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4712,'340826','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4713,'340827','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4714,'340828','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4715,'340881','ͩ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4716,'341000','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4717,'341001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4718,'341002','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4719,'341003','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4720,'341004','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4721,'341021','���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4722,'341022','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4723,'341023','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4724,'341024','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4725,'341100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4726,'341101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4727,'341102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4728,'341103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4729,'341122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4730,'341124','ȫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4731,'341125','��Զ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4732,'341126','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4733,'341181','�쳤��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4734,'341182','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4735,'341200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4736,'341201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4737,'341202','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4738,'341203','򣶫��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4739,'341204','�Ȫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4740,'341221','��Ȫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4741,'341222','̫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4742,'341225','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4743,'341226','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4744,'341282','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4745,'341300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4746,'341301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4747,'341302','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4748,'341321','�ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4749,'341322','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4750,'341323','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4751,'341324','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4752,'341500','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4753,'341501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4754,'341502','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4755,'341503','ԣ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4756,'341521','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4757,'341522','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4758,'341523','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4759,'341524','��կ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4760,'341525','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4761,'341600','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4762,'341601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4763,'341602','�۳���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4764,'341621','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4765,'341622','�ɳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4766,'341623','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4767,'341700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4768,'341701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4769,'341702','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4770,'341721','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4771,'341722','ʯ̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4772,'341723','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4773,'341800','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4774,'341801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4775,'341802','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4776,'341821','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4777,'341822','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4778,'341823','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4779,'341824','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4780,'341825','캵���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4781,'341881','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4782,'350000','����ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4783,'350100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4784,'350101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4785,'350102','��¥��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4786,'350103','̨����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4787,'350104','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4788,'350105','��β��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4789,'350111','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4790,'350121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4791,'350122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4792,'350123','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4793,'350124','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4794,'350125','��̩��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4795,'350128','ƽ̶��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4796,'350181','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4797,'350182','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4798,'350200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4799,'350201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4800,'350203','˼����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4801,'350205','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4802,'350206','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4803,'350211','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4804,'350212','ͬ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4805,'350213','�谲��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4806,'350300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4807,'350301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4808,'350302','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4809,'350303','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4810,'350304','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4811,'350305','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4812,'350322','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4813,'350400','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4814,'350401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4815,'350402','÷����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4816,'350403','��Ԫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4817,'350421','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4818,'350423','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4819,'350424','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4820,'350425','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4821,'350426','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4822,'350427','ɳ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4823,'350428','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4824,'350429','̩����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4825,'350430','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4826,'350481','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4827,'350500','Ȫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4828,'350501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4829,'350502','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4830,'350503','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4831,'350504','�彭��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4832,'350505','Ȫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4833,'350521','�ݰ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4834,'350524','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4835,'350525','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4836,'350526','�»���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4837,'350527','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4838,'350581','ʯʨ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4839,'350582','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4840,'350583','�ϰ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4841,'350600','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4842,'350601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4843,'350602','ܼ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4844,'350603','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4845,'350622','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4846,'350623','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4847,'350624','گ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4848,'350625','��̩��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4849,'350626','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4850,'350627','�Ͼ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4851,'350628','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4852,'350629','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4853,'350681','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4854,'350700','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4855,'350701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4856,'350702','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4857,'350721','˳����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4858,'350722','�ֳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4859,'350723','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4860,'350724','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4861,'350725','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4862,'350781','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4863,'350782','����ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4864,'350783','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4865,'350784','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4866,'350800','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4867,'350801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4868,'350802','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4869,'350821','��͡��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4870,'350822','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4871,'350823','�Ϻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4872,'350824','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4873,'350825','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4874,'350881','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4875,'350900','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4876,'350901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4877,'350902','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4878,'350921','ϼ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4879,'350922','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4880,'350923','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4881,'350924','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4882,'350925','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4883,'350926','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4884,'350981','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4885,'350982','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4886,'360000','����ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4887,'360100','�ϲ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4888,'360101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4889,'360102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4890,'360103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4891,'360104','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4892,'360105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4893,'360111','��ɽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4894,'360121','�ϲ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4895,'360122','�½���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4896,'360123','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4897,'360124','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4898,'360200','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4899,'360201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4900,'360202','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4901,'360203','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4902,'360222','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4903,'360281','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4904,'360300','Ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4905,'360301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4906,'360302','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4907,'360313','�涫��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4908,'360321','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4909,'360322','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4910,'360323','«Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4911,'360400','�Ž���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4912,'360401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4913,'360402','®ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4914,'360403','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4915,'360421','�Ž���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4916,'360423','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4917,'360424','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4918,'360425','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4919,'360426','�°���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4920,'360427','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4921,'360428','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4922,'360429','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4923,'360430','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4924,'360481','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4925,'360482','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4926,'360500','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4927,'360501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4928,'360502','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4929,'360521','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4930,'360600','ӥ̶��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4931,'360601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4932,'360602','�º���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4933,'360622','�཭��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4934,'360681','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4935,'360700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4936,'360701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4937,'360702','�¹���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4938,'360721','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4939,'360722','�ŷ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4940,'360723','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4941,'360724','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4942,'360725','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4943,'360726','��Զ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4944,'360727','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4945,'360728','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4946,'360729','ȫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4947,'360730','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4948,'360731','�ڶ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4949,'360732','�˹���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4950,'360733','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4951,'360734','Ѱ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4952,'360735','ʯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4953,'360781','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4954,'360782','�Ͽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4955,'360800','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4956,'360801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4957,'360802','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4958,'360803','��ԭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4959,'360821','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4960,'360822','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4961,'360823','Ͽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4962,'360824','�¸���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4963,'360825','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4964,'360826','̩����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4965,'360827','�촨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4966,'360828','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4967,'360829','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4968,'360830','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4969,'360881','����ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4970,'360900','�˴���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4971,'360901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4972,'360902','Ԭ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4973,'360921','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4974,'360922','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4975,'360923','�ϸ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4976,'360924','�˷���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4977,'360925','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4978,'360926','ͭ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4979,'360981','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4980,'360982','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4981,'360983','�߰���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4982,'361000','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4983,'361001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4984,'361002','�ٴ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4985,'361021','�ϳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4986,'361022','�质��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4987,'361023','�Ϸ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4988,'361024','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4989,'361025','�ְ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4990,'361026','�˻���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4991,'361027','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4992,'361028','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4993,'361029','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4994,'361030','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4995,'361100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4996,'361101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4997,'361102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4998,'361121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4999,'361122','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5000,'361123','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5001,'361124','Ǧɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5002,'361125','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5003,'361126','߮����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5004,'361127','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5005,'361128','۶����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5006,'361129','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5007,'361130','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5008,'361181','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5009,'370000','ɽ��ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5010,'370100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5011,'370101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5012,'370102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5013,'370103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5014,'370104','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5015,'370105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5016,'370112','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5017,'370113','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5018,'370124','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5019,'370125','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5020,'370126','�̺���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5021,'370181','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5022,'370200','�ൺ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5023,'370201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5024,'370202','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5025,'370203','�б���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5026,'370205','�ķ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5027,'370211','�Ƶ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5028,'370212','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5029,'370213','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5030,'370214','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5031,'370281','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5032,'370282','��ī��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5033,'370283','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5034,'370284','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5035,'370285','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5036,'370300','�Ͳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5037,'370301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5038,'370302','�ʹ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5039,'370303','�ŵ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5040,'370304','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5041,'370305','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5042,'370306','�ܴ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5043,'370321','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5044,'370322','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5045,'370323','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5046,'370400','��ׯ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5047,'370401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5048,'370402','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5049,'370403','Ѧ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5050,'370404','ỳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5051,'370405','̨��ׯ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5052,'370406','ɽͤ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5053,'370481','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5054,'370500','��Ӫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5055,'370501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5056,'370502','��Ӫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5057,'370503','�ӿ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5058,'370521','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5059,'370522','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5060,'370523','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5061,'370600','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5062,'370601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5063,'370602','֥���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5064,'370611','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5065,'370612','Ĳƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5066,'370613','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5067,'370634','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5068,'370681','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5069,'370682','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5070,'370683','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5071,'370684','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5072,'370685','��Զ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5073,'370686','��ϼ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5074,'370687','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5075,'370700','Ϋ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5076,'370701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5077,'370702','Ϋ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5078,'370703','��ͤ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5079,'370704','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5080,'370705','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5081,'370724','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5082,'370725','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5083,'370781','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5084,'370782','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5085,'370783','�ٹ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5086,'370784','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5087,'370785','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5088,'370786','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5089,'370800','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5090,'370801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5091,'370802','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5092,'370811','�γ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5093,'370826','΢ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5094,'370827','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5095,'370828','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5096,'370829','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5097,'370830','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5098,'370831','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5099,'370832','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5100,'370881','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5101,'370882','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5102,'370883','�޳���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5103,'370900','̩����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5104,'370901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5105,'370902','̩ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5106,'370911','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5107,'370921','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5108,'370923','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5109,'370982','��̩��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5110,'370983','�ʳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5111,'371000','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5112,'371001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5113,'371002','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5114,'371081','�ĵ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5115,'371082','�ٳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5116,'371083','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5117,'371100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5118,'371101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5119,'371102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5120,'371103','�ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5121,'371121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5122,'371122','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5123,'371200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5124,'371201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5125,'371202','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5126,'371203','�ֳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5127,'371300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5128,'371301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5129,'371302','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5130,'371311','��ׯ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5131,'371312','�Ӷ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5132,'371321','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5133,'371322','۰����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5134,'371323','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5135,'371324','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5136,'371325','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5137,'371326','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5138,'371327','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5139,'371328','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5140,'371329','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5141,'371400','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5142,'371401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5143,'371402','�³���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5144,'371421','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5145,'371422','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5146,'371423','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5147,'371424','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5148,'371425','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5149,'371426','ƽԭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5150,'371427','�Ľ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5151,'371428','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5152,'371481','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5153,'371482','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5154,'371500','�ĳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5155,'371501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5156,'371502','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5157,'371521','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5158,'371522','ݷ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5159,'371523','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5160,'371524','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5161,'371525','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5162,'371526','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5163,'371581','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5164,'371600','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5165,'371601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5166,'371602','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5167,'371621','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5168,'371622','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5169,'371623','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5170,'371624','մ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5171,'371625','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5172,'371626','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5173,'371700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5174,'371701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5175,'371702','ĵ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5176,'371721','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5177,'371722','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5178,'371723','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5179,'371724','��Ұ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5180,'371725','۩����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5181,'371726','۲����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5182,'371727','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5183,'371728','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5184,'410000','����ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5185,'410100','֣����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5186,'410101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5187,'410102','��ԭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5188,'410103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5189,'410104','�ܳǻ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5190,'410105','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5191,'410106','�Ͻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5192,'410108','�ݼ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5193,'410122','��Ĳ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5194,'410181','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5195,'410182','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5196,'410183','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5197,'410184','��֣��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5198,'410185','�Ƿ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5199,'410200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5200,'410201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5201,'410202','��ͤ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5202,'410203','˳�ӻ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5203,'410204','��¥��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5204,'410205','����̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5205,'410211','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5206,'410221','���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5207,'410222','ͨ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5208,'410223','ξ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5209,'410224','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5210,'410225','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5211,'410300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5212,'410301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5213,'410302','�ϳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5214,'410303','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5215,'410304','�e�ӻ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5216,'410305','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5217,'410306','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5218,'410311','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5219,'410322','�Ͻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5220,'410323','�°���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5221,'410324','�ﴨ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5222,'410325','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5223,'410326','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5224,'410327','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5225,'410328','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5226,'410329','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5227,'410381','��ʦ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5228,'410400','ƽ��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5229,'410401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5230,'410402','�»���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5231,'410403','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5232,'410404','ʯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5233,'410411','տ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5234,'410421','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5235,'410422','Ҷ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5236,'410423','³ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5237,'410425','ۣ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5238,'410481','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5239,'410482','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5240,'410500','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5241,'410501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5242,'410502','�ķ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5243,'410503','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5244,'410505','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5245,'410506','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5246,'410522','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5247,'410523','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5248,'410526','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5249,'410527','�ڻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5250,'410581','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5251,'410600','�ױ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5252,'410601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5253,'410602','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5254,'410603','ɽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5255,'410611','俱���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5256,'410621','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5257,'410622','���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5258,'410700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5259,'410701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5260,'410702','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5261,'410703','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5262,'410704','��Ȫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5263,'410711','��Ұ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5264,'410721','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5265,'410724','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5266,'410725','ԭ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5267,'410726','�ӽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5268,'410727','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5269,'410728','��ԫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5270,'410781','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5271,'410782','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5272,'410800','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5273,'410801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5274,'410802','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5275,'410803','��վ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5276,'410804','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5277,'410811','ɽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5278,'410821','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5279,'410822','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5280,'410823','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5281,'410825','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5282,'410882','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5283,'410883','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5284,'410900','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5285,'410901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5286,'410902','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5287,'410922','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5288,'410923','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5289,'410926','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5290,'410927','̨ǰ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5291,'410928','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5292,'411000','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5293,'411001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5294,'411002','κ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5295,'411023','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5296,'411024','۳����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5297,'411025','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5298,'411081','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5299,'411082','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5300,'411100','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5301,'411101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5302,'411102','Դ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5303,'411103','۱����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5304,'411104','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5305,'411121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5306,'411122','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5307,'411200','����Ͽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5308,'411201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5309,'411202','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5310,'411221','�ų���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5311,'411222','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5312,'411224','¬����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5313,'411281','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5314,'411282','�鱦��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5315,'411300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5316,'411301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5317,'411302','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5318,'411303','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5319,'411321','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5320,'411322','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5321,'411323','��Ͽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5322,'411324','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5323,'411325','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5324,'411326','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5325,'411327','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5326,'411328','�ƺ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5327,'411329','��Ұ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5328,'411330','ͩ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5329,'411381','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5330,'411400','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5331,'411401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5332,'411402','��԰��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5333,'411403','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5334,'411421','��Ȩ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5335,'411422','���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5336,'411423','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5337,'411424','�ϳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5338,'411425','�ݳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5339,'411426','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5340,'411481','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5341,'411500','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5342,'411501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5343,'411502','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5344,'411503','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5345,'411521','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5346,'411522','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5347,'411523','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5348,'411524','�̳���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5349,'411525','��ʼ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5350,'411526','�괨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5351,'411527','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5352,'411528','Ϣ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5353,'411600','�ܿ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5354,'411601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5355,'411602','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5356,'411621','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5357,'411622','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5358,'411623','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5359,'411624','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5360,'411625','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5361,'411626','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5362,'411627','̫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5363,'411628','¹����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5364,'411681','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5365,'411700','פ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5366,'411701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5367,'411702','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5368,'411721','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5369,'411722','�ϲ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5370,'411723','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5371,'411724','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5372,'411725','ȷɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5373,'411726','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5374,'411727','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5375,'411728','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5376,'411729','�²���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5377,'419000','ʡֱϽ�ؼ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5378,'419001','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5379,'420000','����ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5380,'420100','�人��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5381,'420101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5382,'420102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5383,'420103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5384,'420104','�~����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5385,'420105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5386,'420106','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5387,'420107','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5388,'420111','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5389,'420112','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5390,'420113','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5391,'420114','�̵���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5392,'420115','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5393,'420116','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5394,'420117','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5395,'420200','��ʯ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5396,'420201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5397,'420202','��ʯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5398,'420203','����ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5399,'420204','��½��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5400,'420205','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5401,'420222','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5402,'420281','��ұ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5403,'420300','ʮ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5404,'420301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5405,'420302','é����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5406,'420303','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5407,'420321','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5408,'420322','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5409,'420323','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5410,'420324','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5411,'420325','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5412,'420381','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5413,'420500','�˲���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5414,'420501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5415,'420502','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5416,'420503','��Ҹ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5417,'420504','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5418,'420505','�Vͤ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5419,'420506','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5420,'420525','Զ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5421,'420526','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5422,'420527','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5423,'420528','����������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5424,'420529','���������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5425,'420581','�˶���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5426,'420582','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5427,'420583','֦����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5428,'420600','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5429,'420601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5430,'420602','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5431,'420606','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5432,'420607','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5433,'420624','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5434,'420625','�ȳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5435,'420626','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5436,'420682','�Ϻӿ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5437,'420683','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5438,'420684','�˳���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5439,'420700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5440,'420701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5441,'420702','���Ӻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5442,'420703','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5443,'420704','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5444,'420800','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5445,'420801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5446,'420802','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5447,'420804','�޵���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5448,'420821','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5449,'420822','ɳ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5450,'420881','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5451,'420900','Т����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5452,'420901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5453,'420902','Т����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5454,'420921','Т����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5455,'420922','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5456,'420923','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5457,'420981','Ӧ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5458,'420982','��½��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5459,'420984','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5460,'421000','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5461,'421001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5462,'421002','ɳ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5463,'421003','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5464,'421022','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5465,'421023','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5466,'421024','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5467,'421081','ʯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5468,'421083','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5469,'421087','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5470,'421100','�Ƹ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5471,'421101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5472,'421102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5473,'421121','�ŷ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5474,'421122','�찲��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5475,'421123','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5476,'421124','Ӣɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5477,'421125','�ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5478,'421126','ޭ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5479,'421127','��÷��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5480,'421181','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5481,'421182','��Ѩ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5482,'421200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5483,'421201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5484,'421202','�̰���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5485,'421221','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5486,'421222','ͨ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5487,'421223','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5488,'421224','ͨɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5489,'421281','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5490,'421300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5491,'421301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5492,'421303','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5493,'421321','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5494,'421381','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5495,'422800','��ʩ����������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5496,'422801','��ʩ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5497,'422802','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5498,'422822','��ʼ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5499,'422823','�Ͷ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5500,'422825','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5501,'422826','�̷���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5502,'422827','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5503,'422828','�׷���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5504,'429000','ʡֱϽ�ؼ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5505,'429004','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5506,'429005','Ǳ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5507,'429006','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5508,'429021','��ũ������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5509,'430000','����ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5510,'430100','��ɳ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5511,'430101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5512,'430102','ܽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5513,'430103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5514,'430104','��´��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5515,'430105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5516,'430111','�껨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5517,'430112','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5518,'430121','��ɳ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5519,'430124','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5520,'430181','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5521,'430200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5522,'430201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5523,'430202','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5524,'430203','«����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5525,'430204','ʯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5526,'430211','��Ԫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5527,'430221','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5528,'430223','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5529,'430224','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5530,'430225','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5531,'430281','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5532,'430300','��̶��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5533,'430301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5534,'430302','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5535,'430304','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5536,'430321','��̶��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5537,'430381','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5538,'430382','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5539,'430400','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5540,'430401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5541,'430405','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5542,'430406','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5543,'430407','ʯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5544,'430408','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5545,'430412','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5546,'430421','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5547,'430422','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5548,'430423','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5549,'430424','�ⶫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5550,'430426','���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5551,'430481','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5552,'430482','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5553,'430500','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5554,'430501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5555,'430502','˫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5556,'430503','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5557,'430511','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5558,'430521','�۶���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5559,'430522','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5560,'430523','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5561,'430524','¡����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5562,'430525','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5563,'430527','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5564,'430528','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5565,'430529','�ǲ�����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5566,'430581','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5567,'430600','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5568,'430601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5569,'430602','����¥��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5570,'430603','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5571,'430611','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5572,'430621','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5573,'430623','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5574,'430624','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5575,'430626','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5576,'430681','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5577,'430682','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5578,'430700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5579,'430701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5580,'430702','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5581,'430703','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5582,'430721','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5583,'430722','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5584,'430723','���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5585,'430724','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5586,'430725','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5587,'430726','ʯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5588,'430781','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5589,'430800','�żҽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5590,'430801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5591,'430802','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5592,'430811','����Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5593,'430821','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5594,'430822','ɣֲ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5595,'430900','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5596,'430901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5597,'430902','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5598,'430903','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5599,'430921','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5600,'430922','�ҽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5601,'430923','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5602,'430981','�佭��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5603,'431000','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5604,'431001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5605,'431002','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5606,'431003','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5607,'431021','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5608,'431022','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5609,'431023','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5610,'431024','�κ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5611,'431025','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5612,'431026','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5613,'431027','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5614,'431028','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5615,'431081','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5616,'431100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5617,'431101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5618,'431102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5619,'431103','��ˮ̲��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5620,'431121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5621,'431122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5622,'431123','˫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5623,'431124','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5624,'431125','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5625,'431126','��Զ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5626,'431127','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5627,'431128','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5628,'431129','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5629,'431200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5630,'431201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5631,'431202','�׳���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5632,'431221','�з���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5633,'431222','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5634,'431223','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5635,'431224','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5636,'431225','��ͬ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5637,'431226','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5638,'431227','�»ζ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5639,'431228','�ƽ�����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5640,'431229','�������嶱��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5641,'431230','ͨ������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5642,'431281','�齭��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5643,'431300','¦����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5644,'431301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5645,'431302','¦����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5646,'431321','˫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5647,'431322','�»���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5648,'431381','��ˮ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5649,'431382','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5650,'433100','��������������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5651,'433101','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5652,'433122','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5653,'433123','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5654,'433124','��ԫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5655,'433125','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5656,'433126','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5657,'433127','��˳��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5658,'433130','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5659,'440000','�㶫ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5660,'440100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5661,'440101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5662,'440103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5663,'440104','Խ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5664,'440105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5665,'440106','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5666,'440111','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5667,'440112','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5668,'440113','��خ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5669,'440114','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5670,'440115','��ɳ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5671,'440116','�ܸ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5672,'440183','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5673,'440184','�ӻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5674,'440200','�ع���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5675,'440201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5676,'440203','�佭��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5677,'440204','䥽���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5678,'440205','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5679,'440222','ʼ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5680,'440224','�ʻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5681,'440229','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5682,'440232','��Դ����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5683,'440233','�·���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5684,'440281','�ֲ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5685,'440282','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5686,'440300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5687,'440301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5688,'440303','�޺���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5689,'440304','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5690,'440305','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5691,'440306','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5692,'440307','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5693,'440308','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5694,'440400','�麣��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5695,'440401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5696,'440402','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5697,'440403','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5698,'440404','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5699,'440500','��ͷ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5700,'440501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5701,'440507','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5702,'440511','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5703,'440512','婽���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5704,'440513','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5705,'440514','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5706,'440515','�κ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5707,'440523','�ϰ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5708,'440600','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5709,'440601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5710,'440604','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5711,'440605','�Ϻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5712,'440606','˳����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5713,'440607','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5714,'440608','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5715,'440700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5716,'440701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5717,'440703','���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5718,'440704','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5719,'440705','�»���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5720,'440781','̨ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5721,'440783','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5722,'440784','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5723,'440785','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5724,'440800','տ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5725,'440801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5726,'440802','�࿲��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5727,'440803','ϼɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5728,'440804','��ͷ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5729,'440811','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5730,'440823','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5731,'440825','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5732,'440881','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5733,'440882','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5734,'440883','�⴨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5735,'440900','ï����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5736,'440901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5737,'440902','ï����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5738,'440903','ï����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5739,'440923','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5740,'440981','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5741,'440982','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5742,'440983','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5743,'441200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5744,'441201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5745,'441202','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5746,'441203','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5747,'441223','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5748,'441224','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5749,'441225','�⿪��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5750,'441226','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5751,'441283','��Ҫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5752,'441284','�Ļ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5753,'441300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5754,'441301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5755,'441302','�ݳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5756,'441303','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5757,'441322','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5758,'441323','�ݶ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5759,'441324','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5760,'441400','÷����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5761,'441401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5762,'441402','÷����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5763,'441421','÷��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5764,'441422','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5765,'441423','��˳��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5766,'441424','�廪��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5767,'441426','ƽԶ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5768,'441427','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5769,'441481','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5770,'441500','��β��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5771,'441501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5772,'441502','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5773,'441521','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5774,'441523','½����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5775,'441581','½����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5776,'441600','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5777,'441601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5778,'441602','Դ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5779,'441621','�Ͻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5780,'441622','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5781,'441623','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5782,'441624','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5783,'441625','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5784,'441700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5785,'441701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5786,'441702','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5787,'441721','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5788,'441723','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5789,'441781','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5790,'441800','��Զ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5791,'441801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5792,'441802','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5793,'441821','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5794,'441823','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5795,'441825','��ɽ׳������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5796,'441826','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5797,'441827','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5798,'441881','Ӣ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5799,'441882','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5800,'441900','��ݸ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5801,'442000','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5802,'445100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5803,'445101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5804,'445102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5805,'445121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5806,'445122','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5807,'445200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5808,'445201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5809,'445202','�ų���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5810,'445221','�Ҷ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5811,'445222','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5812,'445224','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5813,'445281','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5814,'445300','�Ƹ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5815,'445301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5816,'445302','�Ƴ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5817,'445321','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5818,'445322','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5819,'445323','�ư���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5820,'445381','�޶���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5821,'450000','����׳��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5822,'450100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5823,'450101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5824,'450102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5825,'450103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5826,'450105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5827,'450107','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5828,'450108','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5829,'450109','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5830,'450122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5831,'450123','¡����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5832,'450124','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5833,'450125','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5834,'450126','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5835,'450127','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5836,'450200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5837,'450201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5838,'450202','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5839,'450203','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5840,'450204','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5841,'450205','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5842,'450221','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5843,'450222','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5844,'450223','¹կ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5845,'450224','�ڰ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5846,'450225','��ˮ����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5847,'450226','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5848,'450300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5849,'450301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5850,'450302','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5851,'450303','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5852,'450304','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5853,'450305','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5854,'450311','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5855,'450321','��˷��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5856,'450322','�ٹ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5857,'450323','�鴨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5858,'450324','ȫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5859,'450325','�˰���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5860,'450326','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5861,'450327','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5862,'450328','��ʤ����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5863,'450329','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5864,'450330','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5865,'450331','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5866,'450332','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5867,'450400','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5868,'450401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5869,'450403','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5870,'450404','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5871,'450405','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5872,'450421','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5873,'450422','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5874,'450423','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5875,'450481','�Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5876,'450500','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5877,'450501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5878,'450502','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5879,'450503','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5880,'450512','��ɽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5881,'450521','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5882,'450600','���Ǹ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5883,'450601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5884,'450602','�ۿ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5885,'450603','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5886,'450621','��˼��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5887,'450681','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5888,'450700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5889,'450701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5890,'450702','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5891,'450703','�ձ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5892,'450721','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5893,'450722','�ֱ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5894,'450800','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5895,'450801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5896,'450802','�۱���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5897,'450803','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5898,'450804','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5899,'450821','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5900,'450881','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5901,'450900','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5902,'450901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5903,'450902','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5904,'450921','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5905,'450922','½����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5906,'450923','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5907,'450924','��ҵ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5908,'450981','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5909,'451000','��ɫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5910,'451001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5911,'451002','�ҽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5912,'451021','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5913,'451022','�ﶫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5914,'451023','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5915,'451024','�±���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5916,'451025','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5917,'451026','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5918,'451027','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5919,'451028','��ҵ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5920,'451029','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5921,'451030','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5922,'451031','¡�ָ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5923,'451100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5924,'451101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5925,'451102','�˲���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5926,'451121','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5927,'451122','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5928,'451123','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5929,'451200','�ӳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5930,'451201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5931,'451202','��ǽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5932,'451221','�ϵ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5933,'451222','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5934,'451223','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5935,'451224','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5936,'451225','�޳�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5937,'451226','����ë����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5938,'451227','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5939,'451228','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5940,'451229','������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5941,'451281','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5942,'451300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5943,'451301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5944,'451302','�˱���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5945,'451321','�ó���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5946,'451322','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5947,'451323','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5948,'451324','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5949,'451381','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5950,'451400','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5951,'451401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5952,'451402','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5953,'451421','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5954,'451422','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5955,'451423','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5956,'451424','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5957,'451425','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5958,'451481','ƾ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5959,'460000','����ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5960,'460100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5961,'460101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5962,'460105','��Ӣ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5963,'460106','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5964,'460107','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5965,'460108','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5966,'460200','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5967,'460201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5968,'460300','��ɳ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5969,'460321','��ɳȺ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5970,'460322','��ɳȺ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5971,'460323','��ɳȺ���ĵ������亣��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5972,'469000','ʡֱϽ�ؼ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5973,'469001','��ָɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5974,'469002','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5975,'469003','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5976,'469005','�Ĳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5977,'469006','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5978,'469007','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5979,'469021','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5980,'469022','�Ͳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5981,'469023','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5982,'469024','�ٸ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5983,'469025','��ɳ����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5984,'469026','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5985,'469027','�ֶ�����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5986,'469028','��ˮ����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5987,'469029','��ͤ��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5988,'469030','������������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5989,'500000','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5990,'500100','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5991,'500101','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5992,'500102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5993,'500103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5994,'500104','��ɿ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5995,'500105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5996,'500106','ɳƺ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5997,'500107','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5998,'500108','�ϰ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5999,'500109','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6000,'500110','�뽭��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6001,'500111','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6002,'500112','�山��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6003,'500113','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6004,'500114','ǭ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6005,'500115','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6006,'500116','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6007,'500117','�ϴ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6008,'500118','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6009,'500119','�ϴ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6010,'500200','��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6011,'500223','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6012,'500224','ͭ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6013,'500226','�ٲ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6014,'500227','�ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6015,'500228','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6016,'500229','�ǿ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6017,'500230','�ᶼ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6018,'500231','�潭��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6019,'500232','��¡��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6020,'500233','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6021,'500234','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6022,'500235','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6023,'500236','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6024,'500237','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6025,'500238','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6026,'500240','ʯ��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6027,'500241','��ɽ����������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6028,'500242','��������������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6029,'500243','��ˮ����������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6030,'510000','�Ĵ�ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6031,'510100','�ɶ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6032,'510101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6033,'510104','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6034,'510105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6035,'510106','��ţ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6036,'510107','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6037,'510108','�ɻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6038,'510112','��Ȫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6039,'510113','��׽���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6040,'510114','�¶���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6041,'510115','�½���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6042,'510121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6043,'510122','˫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6044,'510124','ۯ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6045,'510129','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6046,'510131','�ѽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6047,'510132','�½���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6048,'510181','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6049,'510182','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6050,'510183','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6051,'510184','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6052,'510300','�Թ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6053,'510301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6054,'510302','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6055,'510303','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6056,'510304','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6057,'510311','��̲��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6058,'510321','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6059,'510322','��˳��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6060,'510400','��֦����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6061,'510401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6062,'510402','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6063,'510403','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6064,'510411','�ʺ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6065,'510421','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6066,'510422','�α���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6067,'510500','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6068,'510501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6069,'510502','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6070,'510503','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6071,'510504','����̶��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6072,'510521','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6073,'510522','�Ͻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6074,'510524','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6075,'510525','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6076,'510600','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6077,'510601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6078,'510603','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6079,'510623','�н���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6080,'510626','�޽���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6081,'510681','�㺺��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6082,'510682','ʲ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6083,'510683','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6084,'510700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6085,'510701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6086,'510703','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6087,'510704','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6088,'510722','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6089,'510723','��ͤ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6090,'510724','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6091,'510725','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6092,'510726','����Ǽ��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6093,'510727','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6094,'510781','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6095,'510800','��Ԫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6096,'510801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6097,'510802','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6098,'510811','Ԫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6099,'510812','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6100,'510821','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6101,'510822','�ന��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6102,'510823','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6103,'510824','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6104,'510900','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6105,'510901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6106,'510903','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6107,'510904','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6108,'510921','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6109,'510922','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6110,'510923','��Ӣ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6111,'511000','�ڽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6112,'511001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6113,'511002','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6114,'511011','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6115,'511024','��Զ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6116,'511025','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6117,'511028','¡����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6118,'511100','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6119,'511101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6120,'511102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6121,'511111','ɳ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6122,'511112','��ͨ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6123,'511113','��ں���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6124,'511123','��Ϊ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6125,'511124','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6126,'511126','�н���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6127,'511129','�崨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6128,'511132','�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6129,'511133','�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6130,'511181','��üɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6131,'511300','�ϳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6132,'511301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6133,'511302','˳����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6134,'511303','��ƺ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6135,'511304','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6136,'511321','�ϲ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6137,'511322','Ӫɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6138,'511323','���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6139,'511324','��¤��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6140,'511325','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6141,'511381','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6142,'511400','üɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6143,'511401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6144,'511402','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6145,'511421','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6146,'511422','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6147,'511423','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6148,'511424','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6149,'511425','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6150,'511500','�˱���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6151,'511501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6152,'511502','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6153,'511503','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6154,'511521','�˱���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6155,'511523','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6156,'511524','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6157,'511525','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6158,'511526','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6159,'511527','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6160,'511528','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6161,'511529','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6162,'511600','�㰲��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6163,'511601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6164,'511602','�㰲��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6165,'511621','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6166,'511622','��ʤ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6167,'511623','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6168,'511681','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6169,'511700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6170,'511701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6171,'511702','ͨ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6172,'511721','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6173,'511722','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6174,'511723','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6175,'511724','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6176,'511725','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6177,'511781','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6178,'511800','�Ű���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6179,'511801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6180,'511802','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6181,'511803','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6182,'511822','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6183,'511823','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6184,'511824','ʯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6185,'511825','��ȫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6186,'511826','«ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6187,'511827','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6188,'511900','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6189,'511901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6190,'511902','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6191,'511921','ͨ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6192,'511922','�Ͻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6193,'511923','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6194,'512000','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6195,'512001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6196,'512002','�㽭��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6197,'512021','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6198,'512022','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6199,'512081','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6200,'513200','���Ӳ���Ǽ��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6201,'513221','�봨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6202,'513222','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6203,'513223','ï��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6204,'513224','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6205,'513225','��կ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6206,'513226','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6207,'513227','С����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6208,'513228','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6209,'513229','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6210,'513230','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6211,'513231','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6212,'513232','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6213,'513233','��ԭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6214,'513300','���β���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6215,'513321','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6216,'513322','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6217,'513323','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6218,'513324','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6219,'513325','�Ž���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6220,'513326','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6221,'513327','¯����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6222,'513328','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6223,'513329','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6224,'513330','�¸���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6225,'513331','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6226,'513332','ʯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6227,'513333','ɫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6228,'513334','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6229,'513335','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6230,'513336','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6231,'513337','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6232,'513338','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6233,'513400','��ɽ����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6234,'513401','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6235,'513422','ľ�����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6236,'513423','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6237,'513424','�²���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6238,'513425','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6239,'513426','�ᶫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6240,'513427','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6241,'513428','�ո���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6242,'513429','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6243,'513430','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6244,'513431','�Ѿ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6245,'513432','ϲ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6246,'513433','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6247,'513434','Խ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6248,'513435','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6249,'513436','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6250,'513437','�ײ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6251,'520000','����ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6252,'520100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6253,'520101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6254,'520102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6255,'520103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6256,'520111','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6257,'520112','�ڵ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6258,'520113','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6259,'520114','С����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6260,'520121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6261,'520122','Ϣ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6262,'520123','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6263,'520181','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6264,'520200','����ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6265,'520201','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6266,'520203','��֦����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6267,'520221','ˮ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6268,'520222','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6269,'520300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6270,'520301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6271,'520302','�컨����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6272,'520303','�㴨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6273,'520321','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6274,'520322','ͩ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6275,'520323','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6276,'520324','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6277,'520325','��������������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6278,'520326','������������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6279,'520327','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6280,'520328','��̶��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6281,'520329','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6282,'520330','ϰˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6283,'520381','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6284,'520382','�ʻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6285,'520400','��˳��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6286,'520401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6287,'520402','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6288,'520421','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6289,'520422','�ն���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6290,'520423','��������������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6291,'520424','���벼��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6292,'520425','�������岼����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6293,'520500','�Ͻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6294,'520502','���ǹ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6295,'520521','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6296,'520522','ǭ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6297,'520523','��ɳ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6298,'520524','֯����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6299,'520525','��Ӻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6300,'520526','���������������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6301,'520527','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6302,'520600','ͭ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6303,'520602','�̽���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6304,'520603','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6305,'520621','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6306,'520622','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6307,'520623','ʯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6308,'520624','˼����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6309,'520625','ӡ������������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6310,'520626','�½���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6311,'520627','�غ�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6312,'520628','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6313,'522300','ǭ���ϲ���������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6314,'522301','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6315,'522322','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6316,'522323','�հ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6317,'522324','��¡��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6318,'522325','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6319,'522326','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6320,'522327','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6321,'522328','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6322,'522600','ǭ�������嶱��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6323,'522601','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6324,'522622','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6325,'522623','ʩ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6326,'522624','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6327,'522625','��Զ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6328,'522626','᯹���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6329,'522627','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6330,'522628','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6331,'522629','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6332,'522630','̨����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6333,'522631','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6334,'522632','�Ž���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6335,'522633','�ӽ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6336,'522634','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6337,'522635','�齭��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6338,'522636','��կ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6339,'522700','ǭ�ϲ���������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6340,'522701','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6341,'522702','��Ȫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6342,'522722','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6343,'522723','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6344,'522725','�Ͱ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6345,'522726','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6346,'522727','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6347,'522728','�޵���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6348,'522729','��˳��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6349,'522730','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6350,'522731','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6351,'522732','����ˮ��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6352,'530000','����ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6353,'530100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6354,'530101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6355,'530102','�廪��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6356,'530103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6357,'530111','�ٶ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6358,'530112','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6359,'530113','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6360,'530114','�ʹ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6361,'530122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6362,'530124','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6363,'530125','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6364,'530126','ʯ������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6365,'530127','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6366,'530128','»Ȱ��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6367,'530129','Ѱ���������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6368,'530181','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6369,'530300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6370,'530301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6371,'530302','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6372,'530321','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6373,'530322','½����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6374,'530323','ʦ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6375,'530324','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6376,'530325','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6377,'530326','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6378,'530328','մ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6379,'530381','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6380,'530400','��Ϫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6381,'530402','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6382,'530421','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6383,'530422','�ν���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6384,'530423','ͨ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6385,'530424','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6386,'530425','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6387,'530426','��ɽ����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6388,'530427','��ƽ�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6389,'530428','Ԫ���������������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6390,'530500','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6391,'530501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6392,'530502','¡����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6393,'530521','ʩ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6394,'530522','�ڳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6395,'530523','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6396,'530524','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6397,'530600','��ͨ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6398,'530601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6399,'530602','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6400,'530621','³����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6401,'530622','�ɼ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6402,'530623','�ν���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6403,'530624','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6404,'530625','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6405,'530626','�罭��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6406,'530627','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6407,'530628','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6408,'530629','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6409,'530630','ˮ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6410,'530700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6411,'530701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6412,'530702','�ų���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6413,'530721','����������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6414,'530722','��ʤ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6415,'530723','��ƺ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6416,'530724','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6417,'530800','�ն���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6418,'530801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6419,'530802','˼é��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6420,'530821','��������������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6421,'530822','ī��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6422,'530823','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6423,'530824','���ȴ�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6424,'530825','�������������������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6425,'530826','���ǹ���������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6426,'530827','������������������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6427,'530828','����������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6428,'530829','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6429,'530900','�ٲ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6430,'530901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6431,'530902','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6432,'530921','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6433,'530922','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6434,'530923','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6435,'530924','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6436,'530925','˫�����������岼�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6437,'530926','�����������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6438,'530927','��Դ����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6439,'532300','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6440,'532301','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6441,'532322','˫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6442,'532323','Ĳ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6443,'532324','�ϻ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6444,'532325','Ҧ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6445,'532326','��Ҧ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6446,'532327','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6447,'532328','Ԫı��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6448,'532329','�䶨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6449,'532331','»����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6450,'532500','��ӹ���������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6451,'532501','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6452,'532502','��Զ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6453,'532503','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6454,'532523','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6455,'532524','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6456,'532525','ʯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6457,'532526','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6458,'532527','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6459,'532528','Ԫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6460,'532529','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6461,'532530','��ƽ�����������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6462,'532531','�̴���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6463,'532532','�ӿ�����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6464,'532600','��ɽ׳������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6465,'532601','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6466,'532622','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6467,'532623','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6468,'532624','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6469,'532625','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6470,'532626','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6471,'532627','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6472,'532628','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6473,'532800','��˫���ɴ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6474,'532801','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6475,'532822','�º���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6476,'532823','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6477,'532900','�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6478,'532901','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6479,'532922','�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6480,'532923','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6481,'532924','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6482,'532925','�ֶ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6483,'532926','�Ͻ�����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6484,'532927','Ρɽ�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6485,'532928','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6486,'532929','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6487,'532930','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6488,'532931','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6489,'532932','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6490,'533100','�º���徰����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6491,'533102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6492,'533103','â��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6493,'533122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6494,'533123','ӯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6495,'533124','¤����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6496,'533300','ŭ��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6497,'533321','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6498,'533323','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6499,'533324','��ɽ������ŭ��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6500,'533325','��ƺ����������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6501,'533400','�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6502,'533421','���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6503,'533422','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6504,'533423','ά��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6505,'540000','����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6506,'540100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6507,'540101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6508,'540102','�ǹ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6509,'540121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6510,'540122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6511,'540123','��ľ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6512,'540124','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6513,'540125','����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6514,'540126','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6515,'540127','ī�񹤿���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6516,'542100','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6517,'542121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6518,'542122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6519,'542123','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6520,'542124','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6521,'542125','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6522,'542126','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6523,'542127','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6524,'542128','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6525,'542129','â����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6526,'542132','��¡��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6527,'542133','�߰���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6528,'542200','ɽ�ϵ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6529,'542221','�˶���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6530,'542222','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6531,'542223','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6532,'542224','ɣ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6533,'542225','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6534,'542226','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6535,'542227','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6536,'542228','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6537,'542229','�Ӳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6538,'542231','¡����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6539,'542232','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6540,'542233','�˿�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6541,'542300','�տ������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6542,'542301','�տ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6543,'542322','��ľ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6544,'542323','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6545,'542324','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6546,'542325','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6547,'542326','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6548,'542327','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6549,'542328','лͨ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6550,'542329','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6551,'542330','�ʲ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6552,'542331','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6553,'542332','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6554,'542333','�ٰ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6555,'542334','�Ƕ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6556,'542335','��¡��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6557,'542336','����ľ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6558,'542337','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6559,'542338','�ڰ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6560,'542400','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6561,'542421','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6562,'542422','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6563,'542423','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6564,'542424','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6565,'542425','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6566,'542426','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6567,'542427','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6568,'542428','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6569,'542429','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6570,'542430','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6571,'542500','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6572,'542521','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6573,'542522','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6574,'542523','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6575,'542524','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6576,'542525','�Ｊ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6577,'542526','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6578,'542527','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6579,'542600','��֥����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6580,'542621','��֥��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6581,'542622','����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6582,'542623','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6583,'542624','ī����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6584,'542625','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6585,'542626','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6586,'542627','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6587,'610000','����ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6588,'610100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6589,'610101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6590,'610102','�³���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6591,'610103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6592,'610104','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6593,'610111','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6594,'610112','δ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6595,'610113','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6596,'610114','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6597,'610115','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6598,'610116','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6599,'610122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6600,'610124','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6601,'610125','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6602,'610126','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6603,'610200','ͭ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6604,'610201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6605,'610202','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6606,'610203','ӡ̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6607,'610204','ҫ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6608,'610222','�˾���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6609,'610300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6610,'610301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6611,'610302','μ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6612,'610303','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6613,'610304','�²���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6614,'610322','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6615,'610323','�ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6616,'610324','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6617,'610326','ü��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6618,'610327','¤��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6619,'610328','ǧ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6620,'610329','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6621,'610330','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6622,'610331','̫����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6623,'610400','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6624,'610401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6625,'610402','�ض���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6626,'610403','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6627,'610404','μ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6628,'610422','��ԭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6629,'610423','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6630,'610424','Ǭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6631,'610425','��Ȫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6632,'610426','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6633,'610427','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6634,'610428','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6635,'610429','Ѯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6636,'610430','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6637,'610431','�书��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6638,'610481','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6639,'610500','μ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6640,'610501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6641,'610502','��μ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6642,'610521','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6643,'610522','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6644,'610523','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6645,'610524','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6646,'610525','�γ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6647,'610526','�ѳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6648,'610527','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6649,'610528','��ƽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6650,'610581','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6651,'610582','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6652,'610600','�Ӱ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6653,'610601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6654,'610602','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6655,'610621','�ӳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6656,'610622','�Ӵ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6657,'610623','�ӳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6658,'610624','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6659,'610625','־����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6660,'610626','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6661,'610627','��Ȫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6662,'610628','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6663,'610629','�崨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6664,'610630','�˴���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6665,'610631','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6666,'610632','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6667,'610700','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6668,'610701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6669,'610702','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6670,'610721','��֣��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6671,'610722','�ǹ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6672,'610723','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6673,'610724','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6674,'610725','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6675,'610726','��ǿ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6676,'610727','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6677,'610728','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6678,'610729','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6679,'610730','��ƺ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6680,'610800','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6681,'610801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6682,'610802','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6683,'610821','��ľ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6684,'610822','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6685,'610823','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6686,'610824','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6687,'610825','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6688,'610826','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6689,'610827','��֬��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6690,'610828','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6691,'610829','�Ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6692,'610830','�彧��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6693,'610831','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6694,'610900','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6695,'610901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6696,'610902','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6697,'610921','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6698,'610922','ʯȪ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6699,'610923','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6700,'610924','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6701,'610925','᰸���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6702,'610926','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6703,'610927','��ƺ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6704,'610928','Ѯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6705,'610929','�׺���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6706,'611000','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6707,'611001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6708,'611002','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6709,'611021','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6710,'611022','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6711,'611023','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6712,'611024','ɽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6713,'611025','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6714,'611026','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6715,'620000','����ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6716,'620100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6717,'620101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6718,'620102','�ǹ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6719,'620103','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6720,'620104','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6721,'620105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6722,'620111','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6723,'620121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6724,'620122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6725,'620123','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6726,'620200','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6727,'620201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6728,'620300','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6729,'620301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6730,'620302','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6731,'620321','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6732,'620400','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6733,'620401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6734,'620402','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6735,'620403','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6736,'620421','��Զ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6737,'620422','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6738,'620423','��̩��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6739,'620500','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6740,'620501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6741,'620502','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6742,'620503','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6743,'620521','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6744,'620522','�ذ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6745,'620523','�ʹ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6746,'620524','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6747,'620525','�żҴ�����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6748,'620600','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6749,'620601','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6750,'620602','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6751,'620621','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6752,'620622','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6753,'620623','��ף����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6754,'620700','��Ҵ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6755,'620701','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6756,'620702','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6757,'620721','����ԣ����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6758,'620722','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6759,'620723','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6760,'620724','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6761,'620725','ɽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6762,'620800','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6763,'620801','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6764,'620802','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6765,'620821','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6766,'620822','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6767,'620823','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6768,'620824','��ͤ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6769,'620825','ׯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6770,'620826','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6771,'620900','��Ȫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6772,'620901','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6773,'620902','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6774,'620921','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6775,'620922','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6776,'620923','�౱�ɹ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6777,'620924','��������������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6778,'620981','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6779,'620982','�ػ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6780,'621000','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6781,'621001','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6782,'621002','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6783,'621021','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6784,'621022','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6785,'621023','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6786,'621024','��ˮ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6787,'621025','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6788,'621026','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6789,'621027','��ԭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6790,'621100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6791,'621101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6792,'621102','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6793,'621121','ͨμ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6794,'621122','¤����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6795,'621123','μԴ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6796,'621124','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6797,'621125','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6798,'621126','���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6799,'621200','¤����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6800,'621201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6801,'621202','�䶼��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6802,'621221','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6803,'621222','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6804,'621223','崲���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6805,'621224','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6806,'621225','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6807,'621226','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6808,'621227','����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6809,'621228','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6810,'622900','���Ļ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6811,'622901','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6812,'622921','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6813,'622922','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6814,'622923','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6815,'622924','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6816,'622925','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6817,'622926','������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6818,'622927','��ʯɽ�����嶫����������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6819,'623000','���ϲ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6820,'623001','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6821,'623021','��̶��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6822,'623022','׿����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6823,'623023','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6824,'623024','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6825,'623025','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6826,'623026','µ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6827,'623027','�ĺ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6828,'630000','�ຣʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6829,'630100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6830,'630101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6831,'630102','�Ƕ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6832,'630103','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6833,'630104','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6834,'630105','�Ǳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6835,'630121','��ͨ��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6836,'630122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6837,'630123','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6838,'632100','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6839,'632121','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6840,'632122','��ͻ�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6841,'632123','�ֶ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6842,'632126','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6843,'632127','��¡����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6844,'632128','ѭ��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6845,'632200','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6846,'632221','��Դ����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6847,'632222','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6848,'632223','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6849,'632224','�ղ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6850,'632300','���ϲ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6851,'632321','ͬ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6852,'632322','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6853,'632323','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6854,'632324','�����ɹ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6855,'632500','���ϲ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6856,'632521','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6857,'632522','ͬ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6858,'632523','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6859,'632524','�˺���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6860,'632525','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6861,'632600','�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6862,'632621','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6863,'632622','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6864,'632623','�ʵ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6865,'632624','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6866,'632625','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6867,'632626','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6868,'632700','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6869,'632721','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6870,'632722','�Ӷ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6871,'632723','�ƶ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6872,'632724','�ζ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6873,'632725','��ǫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6874,'632726','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6875,'632800','�����ɹ������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6876,'632801','���ľ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6877,'632802','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6878,'632821','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6879,'632822','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6880,'632823','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6881,'640000','���Ļ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6882,'640100','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6883,'640101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6884,'640104','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6885,'640105','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6886,'640106','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6887,'640121','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6888,'640122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6889,'640181','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6890,'640200','ʯ��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6891,'640201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6892,'640202','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6893,'640205','��ũ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6894,'640221','ƽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6895,'640300','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6896,'640301','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6897,'640302','��ͨ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6898,'640303','���±���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6899,'640323','�γ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6900,'640324','ͬ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6901,'640381','��ͭϿ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6902,'640400','��ԭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6903,'640401','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6904,'640402','ԭ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6905,'640422','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6906,'640423','¡����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6907,'640424','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6908,'640425','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6909,'640500','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6910,'640501','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6911,'640502','ɳ��ͷ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6912,'640521','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6913,'640522','��ԭ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6914,'650000','�½�ά���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6915,'650100','��³ľ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6916,'650101','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6917,'650102','��ɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6918,'650103','ɳ���Ϳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6919,'650104','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6920,'650105','ˮĥ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6921,'650106','ͷ�ͺ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6922,'650107','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6923,'650109','�׶���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6924,'650121','��³ľ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6925,'650200','����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6926,'650201','��Ͻ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6927,'650202','��ɽ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6928,'650203','����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6929,'650204','�׼�̲��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6930,'650205','�ڶ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6931,'652100','��³������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6932,'652101','��³����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6933,'652122','۷����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6934,'652123','�п�ѷ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6935,'652200','���ܵ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6936,'652201','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6937,'652222','������������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6938,'652223','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6939,'652300','��������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6940,'652301','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6941,'652302','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6942,'652323','��ͼ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6943,'652324','����˹��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6944,'652325','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6945,'652327','��ľ������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6946,'652328','ľ�ݹ�����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6947,'652700','���������ɹ�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6948,'652701','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6949,'652722','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6950,'652723','��Ȫ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6951,'652800','���������ɹ�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6952,'652801','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6953,'652822','��̨��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6954,'652823','ξ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6955,'652824','��Ǽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6956,'652825','��ĩ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6957,'652826','���Ȼ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6958,'652827','�;���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6959,'652828','��˶��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6960,'652829','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6961,'652900','�����յ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6962,'652901','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6963,'652922','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6964,'652923','�⳵��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6965,'652924','ɳ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6966,'652925','�º���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6967,'652926','�ݳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6968,'652927','��ʲ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6969,'652928','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6970,'652929','��ƺ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6971,'653000','�������տ¶�����������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6972,'653001','��ͼʲ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6973,'653022','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6974,'653023','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6975,'653024','��ǡ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6976,'653100','��ʲ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6977,'653101','��ʲ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6978,'653121','�踽��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6979,'653122','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6980,'653123','Ӣ��ɳ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6981,'653124','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6982,'653125','ɯ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6983,'653126','Ҷ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6984,'653127','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6985,'653128','���պ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6986,'653129','٤ʦ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6987,'653130','�ͳ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6988,'653131','��ʲ�����������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6989,'653200','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6990,'653201','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6991,'653221','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6992,'653222','ī����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6993,'653223','Ƥɽ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6994,'653224','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6995,'653225','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6996,'653226','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6997,'653227','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6998,'654000','���������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6999,'654002','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7000,'654003','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7001,'654021','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7002,'654022','�첼�������������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7003,'654023','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7004,'654024','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7005,'654025','��Դ��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7006,'654026','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7007,'654027','�ؿ�˹��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7008,'654028','���տ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7009,'654200','���ǵ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7010,'654201','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7011,'654202','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7012,'654221','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7013,'654223','ɳ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7014,'654224','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7015,'654225','ԣ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7016,'654226','�Ͳ��������ɹ�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7017,'654300','����̩����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7018,'654301','����̩��',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7019,'654321','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7020,'654322','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7021,'654323','������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7022,'654324','���ͺ���',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7023,'654325','�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7024,'654326','��ľ����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7025,'659000','������ֱϽ�ؼ���������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7026,'659001','ʯ������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7027,'659002','��������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7028,'659003','ͼľ�����',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7029,'659004','�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7030,'710000','̨��ʡ',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7031,'810000','����ر�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7032,'820000','�����ر�������',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : DeleteDefaultUserInfo
 ��������  : ɾ��Ĭ���û�
 �������  : int iUserType
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��5��25��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int DeleteDefaultUserInfo(int iUserType, DBOper* pdboper)
{
    int iRet = 0;
    char strWiscomVID[36] = {0};
    string strDeleteSQL = "";

    if (NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "DeleteDefaultUserInfo() exit---: User Srv DB Oper Error \r\n");
        return -1;
    }

    if (pGblconf->board_id[0] == '\0' || pGblconf->center_code[0] == '\0' || pGblconf->trade_code[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "DeleteDefaultUserInfo() exit---: CMS ID Not Config Or Not Right \r\n");
        return -1;
    }

    /* ����Ĭ�ϵ��û�ID */
    if (iUserType == 0) /* �����û� */
    {
        snprintf(strWiscomVID, 36, "%s%s%d%s", pGblconf->center_code, pGblconf->trade_code, 300, (char*)"0000000");
    }
    else if (iUserType == 1) /* admin�û� */
    {
        snprintf(strWiscomVID, 36, "%s%s%d%s", pGblconf->center_code, pGblconf->trade_code, 300, (char*)"0001230");
    }

    /* ɾ�����ݿ��û�ID */
    strDeleteSQL.clear();
    strDeleteSQL = "Delete from UserConfig WHERE UserID like '";
    strDeleteSQL += strWiscomVID;
    strDeleteSQL += "'";

    iRet = pdboper->DB_Delete(strDeleteSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "DeleteDefaultUserInfo() DB Oper Error:strDeleteSQL=%s, iRet=%d \r\n", strDeleteSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "DeleteDefaultUserInfo() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
        return -1;
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : InsertDefaultUserInfo
 ��������  : ����Ĭ�ϵ��û���Ϣ
 �������  : char* userName
             DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��12��2��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int InsertDefaultUserInfo(char* userName, DBOper* pdboper)
{
    int i = 0;
    char strWiscomVID[36] = {0};

    string strQuerySQL = "";
    string strInsertSQL = "";
    string strUpdateSQL = "";
    int record_count = 0;

    string strSuperUserName = "";
    string strSuperUserPassword = "";
    string strAdminUserName = "";
    string strAdminUserPassword = "";

    int iUserEnable = 0;
    string strUserID = "";
    string strUserPassword = "";

    if (NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "InsertDefaultUserInfo() exit---: User Srv DB Oper Error \r\n");
        return -1;
    }

    if (NULL == userName)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "InsertDefaultUserInfo() exit---: User Name Error \r\n");
        return -1;
    }

    if (pGblconf->board_id[0] == '\0' || pGblconf->center_code[0] == '\0' || pGblconf->trade_code[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() exit---: CMS ID Not Config Or Not Right \r\n");
        return -1;
    }

    /* ����Ĭ�ϵ��û��������� */
    strSuperUserName = "WiscomV";
    strSuperUserPassword = "WiscomV";

    strAdminUserName = "admin";
    strAdminUserPassword = "admin";

    /* ����Ĭ�ϵ��û�ID */
    if (sstrcmp(userName, (char*)strSuperUserName.c_str()) == 0) /* �����û� */
    {
        snprintf(strWiscomVID, 36, "%s%s%d%s", pGblconf->center_code, pGblconf->trade_code, 300, (char*)"0000000");
    }
    else if (sstrcmp(userName, (char*)strAdminUserName.c_str()) == 0) /* admin�û� */
    {
        snprintf(strWiscomVID, 36, "%s%s%d%s", pGblconf->center_code, pGblconf->trade_code, 300, (char*)"0001230");
    }

    /* �����ݿ��ȡ�û�ID */
    strQuerySQL.clear();
    strQuerySQL = "select * from UserConfig WHERE UserName like '";
    strQuerySQL += userName;
    strQuerySQL += "'";

    record_count = pdboper->DB_Select(strQuerySQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() DB Oper Error:strQuerySQL=%s, record_count=%d \r\n", strQuerySQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        /* ����ǳ����û��Ļ���ϵͳ�Զ��������� */
        if (sstrcmp(userName, (char*)strSuperUserName.c_str()) == 0)
        {
            /* ��Ĭ�����ɵ�ID�ٲ���һ�飬��ֹWiscomV�л����û�����һ�� */
            strQuerySQL.clear();
            strQuerySQL = "select * from UserConfig WHERE UserID like '";
            strQuerySQL += strWiscomVID;
            strQuerySQL += "'";

            record_count = pdboper->DB_Select(strQuerySQL.c_str(), 1);

            if (record_count < 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() DB Oper Error:strSQL=%s, record_count=%d \r\n", strQuerySQL.c_str(), record_count);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
                return -1;
            }
            else if (record_count == 1)
            {
                /* �����û��������� */
                strUpdateSQL.clear();
                strUpdateSQL = "UPDATE UserConfig SET UserName = '";
                strUpdateSQL += strSuperUserName;
                strUpdateSQL += "', Password = '";
                strUpdateSQL += strSuperUserPassword;
                strUpdateSQL += "', Enable = 1 WHERE UserID like '";
                strUpdateSQL += strWiscomVID;
                strUpdateSQL += "'";

                i = pdboper->DB_Update(strUpdateSQL.c_str(), 1);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() DB Oper Error:strUpdateSQL=%s, i=%d \r\n", strUpdateSQL.c_str(), i);
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
                    return -1;
                }
            }
            else if (record_count == 0)
            {
                /* ���û���Ϣд�����ݿ� */
                strInsertSQL.clear();

                strInsertSQL = "insert into UserConfig (UserID,UserName,Enable,Password,Level,Permission) values (";

                strInsertSQL += "'";
                strInsertSQL += strWiscomVID;
                strInsertSQL += "'";

                strInsertSQL += ",";

                strInsertSQL += "'";
                strInsertSQL += strSuperUserName;
                strInsertSQL += "'";

                strInsertSQL += ",";
                strInsertSQL += "1";
                strInsertSQL += ",";

                strInsertSQL += "'";
                strInsertSQL += strSuperUserPassword;
                strInsertSQL += "'";

                strInsertSQL += ",";
                strInsertSQL += "15";
                strInsertSQL += ",";

                strInsertSQL += "4294967295";

                strInsertSQL += ")";

                i = pdboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() DB Oper Error:strInsertSQL=%s, i=%d \r\n", strInsertSQL.c_str(), i);
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
                    return -1;
                }
            }
        }
        else if (sstrcmp(userName, (char*)strAdminUserName.c_str()) == 0)
        {
            /* ��Ĭ�����ɵ�ID�ٲ���һ�飬��ֹWiscomV�л����û�����һ�� */
            strQuerySQL.clear();
            strQuerySQL = "select * from UserConfig WHERE UserID like '";
            strQuerySQL += strWiscomVID;
            strQuerySQL += "'";

            record_count = pdboper->DB_Select(strQuerySQL.c_str(), 1);

            if (record_count < 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() DB Oper Error:strSQL=%s, record_count=%d \r\n", strQuerySQL.c_str(), record_count);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
                return -1;
            }
            else if (record_count == 1)
            {
                /* �����û��� */
                strUpdateSQL.clear();
                strUpdateSQL = "UPDATE UserConfig SET UserName = '";
                strUpdateSQL += strAdminUserName;
                strUpdateSQL += "', Enable = 1 WHERE UserID like '";
                strUpdateSQL += strWiscomVID;
                strUpdateSQL += "'";

                i = pdboper->DB_Update(strUpdateSQL.c_str(), 1);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() DB Oper Error:strUpdateSQL=%s, i=%d \r\n", strUpdateSQL.c_str(), i);
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
                    return -1;
                }
            }
            else if (record_count == 0)
            {
                /* ���û���Ϣд�����ݿ� */
                strInsertSQL.clear();

                strInsertSQL = "insert into UserConfig (UserID,UserName,Enable,Password,Level,Permission) values (";

                strInsertSQL += "'";
                strInsertSQL += strWiscomVID;
                strInsertSQL += "'";

                strInsertSQL += ",";

                strInsertSQL += "'";
                strInsertSQL += strAdminUserName;
                strInsertSQL += "'";

                strInsertSQL += ",";
                strInsertSQL += "1";
                strInsertSQL += ",";

                strInsertSQL += "'";
                strInsertSQL += strAdminUserPassword;
                strInsertSQL += "'";

                strInsertSQL += ",";
                strInsertSQL += "15";
                strInsertSQL += ",";

                strInsertSQL += "4294967295";

                strInsertSQL += ")";

                i = pdboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() DB Oper Error:strInsertSQL=%s, i=%d \r\n", strInsertSQL.c_str(), i);
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
                    return -1;
                }
            }
        }
    }
    else
    {
        /* �Ƿ����� */
        iUserEnable = 0;
        pdboper->GetFieldValue("Enable", iUserEnable);

        /* �û�ͳһ���id */
        strUserID.clear();
        pdboper->GetFieldValue("UserID", strUserID);

        /* �û����� */
        strUserPassword.clear();
        pdboper->GetFieldValue("Password", strUserPassword);

        /* �ж��û�ID�Ƿ���Ҫ���� */
        if (sstrcmp(userName, (char*)strSuperUserName.c_str()) == 0)
        {
            if (0 != sstrcmp((char*)strUserID.c_str(), strWiscomVID)
                || 0 != sstrcmp((char*)strUserPassword.c_str(), (char*)strSuperUserPassword.c_str())
                || iUserEnable != 1) /* ��ƥ�䣬��Ҫ����WiscomV���û�ID */
            {
                strUpdateSQL.clear();
                strUpdateSQL = "UPDATE UserConfig SET UserID = '";
                strUpdateSQL += strWiscomVID;
                strUpdateSQL += "', Password = '";
                strUpdateSQL += strSuperUserPassword;
                strUpdateSQL += "', Enable = 1";
                strUpdateSQL += " WHERE UserName like '";
                strUpdateSQL += strSuperUserName;
                strUpdateSQL += "'";

                i = pdboper->DB_Update(strUpdateSQL.c_str(), 1);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() DB Oper Error:strUpdateSQL=%s, i=%d \r\n", strUpdateSQL.c_str(), i);
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
                    return -1;
                }
            }
        }
        else if (sstrcmp(userName, (char*)strAdminUserName.c_str()) == 0)
        {
            if (0 != sstrcmp((char*)strUserID.c_str(), strWiscomVID) || iUserEnable != 1) /* ��ƥ�䣬��Ҫ����admin���û�ID */
            {
                strUpdateSQL.clear();
                strUpdateSQL = "UPDATE UserConfig SET UserID = '";
                strUpdateSQL += strWiscomVID;
                strUpdateSQL += "', Enable = 1";
                strUpdateSQL += " WHERE UserName like '";
                strUpdateSQL += strAdminUserName;
                strUpdateSQL += "'";

                i = pdboper->DB_Update(strUpdateSQL.c_str(), 1);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() DB Oper Error:strUpdateSQL=%s, i=%d \r\n", strUpdateSQL.c_str(), i);
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "InsertDefaultUserInfo() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
                    return -1;
                }
            }
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : SetMySQLEventOn
 ��������  : �������ݿ�ɾ���¼�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��3��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SetMySQLEventOn()
{
    int iRet = 0;
    int record_count = 0;
    string strSQL = "";

    strSQL.clear();
    strSQL = "SET GLOBAL event_scheduler = ON;";

    iRet = g_DBOper.DBExecute(strSQL.c_str());

    if (iRet > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "SetMySQLEventOn() g_DBOper.DBExecute Success \r\n");
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "SetMySQLEventOn() g_DBOper.DBExecute Error, SQL=%s, iRet=%d, Reason=%s\r\n", strSQL.c_str(), iRet, g_DBOper.GetLastDbErrorMsg());
    }

    if (g_LogDBOperConnectStatus)
    {
        iRet = g_LogDBOper.DBExecute(strSQL.c_str());

        if (iRet > 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "SetMySQLEventOn() g_LogDBOper.DBExecute Success \r\n");
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "SetMySQLEventOn() g_LogDBOper.DBExecute Error, SQL=%s, iRet=%d, Reason=%s\r\n", strSQL.c_str(), iRet, g_LogDBOper.GetLastDbErrorMsg());
        }
    }

    /* ����Ĭ������ */
    strSQL.clear();
    strSQL = "select * from SystemConfig WHERE KeyName like 'DaysToKeepForSystemLog'";

    record_count = g_DBOper.DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetMySQLEventOn() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetMySQLEventOn() ErrorMsg=%s\r\n", g_DBOper.GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        strSQL.clear();
        strSQL = "insert into SystemConfig (KeyName,KeyValue) values ('DaysToKeepForSystemLog','604800')";

        iRet = g_DBOper.DB_Insert("", "", strSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "SetMySQLEventOn() g_DBOper.DB_Insert Error, SQL=%s, iRet=%d, Reason=%s\r\n", strSQL.c_str(), iRet, g_DBOper.GetLastDbErrorMsg());
        }
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : WebNotifyDBRefreshProc
 ��������  : web֪ͨˢ�����ݿ⴦��
 �������  : char* pcTabName
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��3��17��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int WebNotifyDBRefreshProc(char* pcTabName)
{
    int iRet = 0;

    if (NULL == pcTabName)
    {
        return -1;
    }

    if (0 == sstrcmp(pcTabName, (char*)"DiBiaoUploadPicMapConfig"))
    {
        /* �����н��Ϻ��ر궨ʱ�����ͼ�ϴ�¼�����ݿ��еı仯���ݣ���Ҫ���¼���һ��¼����� */
        iRet = RecordInfo_db_refresh_proc();
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "WebNotifyDBRefreshProc() : DiBiaoUploadPicMapConfig:iRet=%d\r\n", iRet);
    }
    else if (0 == sstrcmp(pcTabName, (char*)"GBLogicDeviceConfig"))
    {
        /* �߼��豸�仯 */
        //iRet = GBLogicDeviceConfig_db_refresh_proc();
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "WebNotifyDBRefreshProc() : GBLogicDeviceConfig:iRet=%d\r\n", iRet);
    }
    else if (0 == sstrcmp(pcTabName, (char*)"GBPhyDeviceConfig"))
    {
        /* �����豸�仯 */
        //iRet = GBDeviceConfig_db_refresh_proc();
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "WebNotifyDBRefreshProc() : GBPhyDeviceConfig:iRet=%d\r\n", iRet);
    }
    else if (0 == sstrcmp(pcTabName, (char*)"RouteNetConfig"))
    {
        /* �ϼ�·�����ñ仯 */
        iRet = RouteInfoConfig_db_refresh_proc();
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "WebNotifyDBRefreshProc() : RouteNetConfig:iRet=%d\r\n", iRet);
    }
    else
    {
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : WebNotifyDBSyncProc
 ��������  : Web֪ͨ��������µ����ݿ�ͬ������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��12��8��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int WebNotifyDBSyncProc()
{
    char strCmd[256] = {0};

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�������ñ�ʶ:dwMSFlag=%d, ��������ʶ:st_MSFlag=%d", g_BoardNetConfig.dwMSFlag, g_BoardNetConfig.st_MSFlag);

    if (g_BoardNetConfig.dwMSFlag == 1)  /* �������õ������ */
    {
        if (g_BoardNetConfig.st_MSFlag == 2) /* ��������������Ƿ��� */
        {
            if (cms_run_status) /* ������������ģʽ,�ӱ������ݿ�ͬ�������� */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "������������ģʽ,�ӱ������ݿ�ͬ��������, ��ʼ---");

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB.sh %s %s", (char*)"10.168.0.102", (char*)"10.168.0.101");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB���ݿⱸ������:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_mobile.sh %s %s", (char*)"10.168.0.102", (char*)"10.168.0.101");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_MOBILE���ݿⱸ������:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_TSU.sh %s %s", (char*)"10.168.0.102", (char*)"10.168.0.101");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_TSU���ݿⱸ������:%s", strCmd);
                system(strCmd);

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ���������ݿⱸ��, ����---");
            }
            else /* ���������ݿ�ͬ�������� */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���������ڱ�ģʽ,���������ݿ�ͬ��������, ��ʼ---");

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB���ݿⱸ������:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_mobile.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_MOBILE���ݿⱸ������:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_TSU.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_TSU���ݿⱸ������:%s", strCmd);
                system(strCmd);

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ���������ݿⱸ��, ����---");
            }
        }
        else if (g_BoardNetConfig.st_MSFlag == 1) /* ���� */
        {
            if (cms_run_status) /* ������������ģʽ,���������ݿ�ͬ�������� */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "������������ģʽ,���������ݿ�ͬ��������, ��ʼ---");

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB���ݿⱸ������:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_mobile.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_MOBILE���ݿⱸ������:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_TSU.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_TSU���ݿⱸ������:%s", strCmd);
                system(strCmd);

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ���������ݿⱸ��, ����---");
            }
            else /* �ӱ������ݿ�ͬ�������� */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���������ڱ�ģʽ,�ӱ������ݿ�ͬ��������, ��ʼ---");

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB.sh %s %s", (char*)"10.168.0.102", (char*)"10.168.0.101");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB���ݿⱸ������:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_mobile.sh %s %s", (char*)"10.168.0.102", (char*)"10.168.0.101");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_MOBILE���ݿⱸ������:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_TSU.sh %s %s", (char*)"10.168.0.102", (char*)"10.168.0.101");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_TSU���ݿⱸ������:%s", strCmd);
                system(strCmd);

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ���������ݿⱸ��, ����---");
            }
        }
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����û������,����Ҫ�������ݿ�ͬ������");
    }

    return 0;
}
