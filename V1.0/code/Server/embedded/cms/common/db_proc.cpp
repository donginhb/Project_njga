/******************************************************************************

                  版权所有 (C), 2001-2011, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : db_proc.cpp
  版 本 号   : 初稿
  作    者   : 杨海锋
  生成日期   : 2014年2月17日 星期一
  最近修改   :
  功能描述   : 数据库公共操作模块
  函数列表   :
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
  修改历史   :
  1.日    期   : 2014年2月17日 星期一
    作    者   : 杨海锋
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
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
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern BOARD_NET_ATTR  g_BoardNetConfig;
extern gbl_conf_t* pGblconf;              /* 全局配置信息 */
//设置TSU上报
extern unsigned int g_TSUUpLoadFileFlag;  /* TSU是否需要重新上报录像文件标识 */
extern int g_MMSEnableFlag;               /* SX、RX是否启用MMS功能，默认启用 */
extern int cms_run_status;                /* 0:没有运行,1:正常运行 */

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/
DBOper g_DBOper;
DBOper g_LogDBOper;
int g_LogDBOperConnectStatus = 0;

DB_Thread* g_DBThread;
char g_strDBThreadPara[4][100] = {{0}, {0}, {0}, {0}};
char g_StrCon[2][100] = {{0}, {0}};
char g_StrConLog[2][100] = {{0}, {0}};

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
/* 配置数据的数据库文件修改时间,用于判断数据库是否有变化 */
time_t db_file_AlarmDeployment_lmt;                 /* AlarmDeployment数据库文件件修改时间 */
time_t db_file_CruiseActionConfig_lmt;              /* CruiseActionConfig数据库文件件修改时间 */
time_t db_file_CruiseConfig_lmt;                    /* CruiseConfig数据库文件件修改时间 */
time_t db_file_GBLogicDeviceConfig_lmt;             /* GBLogicDeviceConfig数据库文件件修改时间 */
time_t db_file_GBPhyDeviceConfig_lmt;               /* GBPhyDeviceConfig数据库文件件修改时间 */
time_t db_file_LogicDeviceGroupConfig_lmt;          /* LogicDeviceGroupConfig数据库文件件修改时间 */
time_t db_file_LogicDeviceMapGroupConfig_lmt;       /* LogicDeviceMapGroupConfig数据库文件件修改时间 */
time_t db_file_PlanActionConfig_lmt;                /* PlanActionConfig数据库文件件修改时间 */
time_t db_file_PlanConfig_lmt;                      /* PlanConfig数据库文件件修改时间 */
time_t db_file_PollActionConfig_lmt;                /* PollActionConfig数据库文件件修改时间 */
time_t db_file_PollConfig_lmt;                      /* PollConfig数据库文件件修改时间 */
time_t db_file_RecordSchedConfig_lmt;               /* RecordSchedConfig数据库文件件修改时间 */
time_t db_file_RecordTimeSchedConfig_lmt;           /* RecordTimeSchedConfig数据库文件件修改时间 */
time_t db_file_RouteNetConfig_lmt;                  /* RouteNetConfig数据库文件件修改时间 */
time_t db_file_DiBiaoUploadPicMapConfig_lmt;        /* DiBiaoUploadPicMapConfig数据库文件件修改时间 */

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
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
 函 数 名  : ClosedDB
 功能描述  : 关闭数据库
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月17日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ClosedDB()
{
    g_DBOper.Close();
    g_LogDBOper.Close();
    return 0;
}

/*****************************************************************************
 函 数 名  : db_gDBOperconnect
 功能描述  : 全局数据库连接
 输入参数  : string strCon
             string strDb
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int db_gDBOperconnect()
{
    int iRet = g_DBOper.Connect(g_StrCon, (char*)"");

    printf("db_gDBOperconnect() g_DBOper Connect: iRet=%d\r\n", iRet);

    if (0 != iRet)
    {
        return iRet;
    }

	/* 日志数据库连接 */
    iRet = g_LogDBOper.Connect(g_StrConLog, (char*)"");

    printf("db_gDBOperconnect() g_LogDBOper Connect: iRet=%d\r\n", iRet);

    if (0 != iRet)
    {
	    g_LogDBOperConnectStatus = 0;
        return 0; /* 日志数据库没有连接成功，不能退出 */
    }

    g_LogDBOperConnectStatus = 1;

    return iRet;
}

/*****************************************************************************
 函 数 名  : db_connect
 功能描述  : 数据库连接
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : load_device_group_cfg_from_db
 功能描述  : 从数据库中加载分组信息
 输入参数  : DBOper* pDbOper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月2日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 获取分组名称 */
    /* 查询分组表 */
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

    /* 循环获取数据库数据 */
    do
    {
        string strName = "";
        string strGroupID = "";
        int iSortID = 0;
        string strParentID = "";

        string strCivilCode = "";  /* 行政区域编码 */
        string strGroupCode = "";  /* 虚拟组织编码 */
        string strParentCode = ""; /* 上级组织编码 */

        int iNeedToUpLoad = 0;

        char pcTmpGroupCode[24] = {0};

        char strGroupCodeTmp1[3] = {0}; /* 省级编码 */
        char strGroupCodeTmp2[3] = {0}; /* 市级编码 */
        char strGroupCodeTmp3[3] = {0}; /* 区级编码 */
        char strGroupCodeTmp4[3] = {0}; /* 派出所编码 */
        char strGroupCodeTmp5[3] = {0}; /* 虚拟组织1编码 */
        char strGroupCodeTmp6[3] = {0}; /* 虚拟组织2编码 */
        char strGroupCodeTmp7[3] = {0}; /* 虚拟组织3编码 */
        char strGroupCodeTmp8[3] = {0}; /* 虚拟组织4编码 */

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "load_device_group_cfg_from_db() While Count=%d \r\n", while_count);
        }

        /* 组编号 */
        strGroupID.clear();
        pDbOper->GetFieldValue("GroupID", strGroupID);

        /* 组名称 */
        strName.clear();
        pDbOper->GetFieldValue("Name", strName);

        /* 同一父节点下组排序编号 */
        iSortID = 0;
        pDbOper->GetFieldValue("SortID", iSortID);

        /* 父节点编号 */
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

        /* 分组ID */
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
            && 0 == sstrcmp(strGroupCodeTmp5, (char*)"00")) /* 省级编码 */
        {
            /* 行政区域编码 */
            strCivilCode = strGroupCodeTmp1;

            /* 上报的分组编码 */
            strGroupCode = strGroupCodeTmp1;

            /* 上级组织编码 */
            strParentCode = "";
        }
        else if (0 != sstrcmp(strGroupCodeTmp1, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp2, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp3, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp4, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp5, (char*)"00")) /* 省级下面直属分组 */
        {
            /* 行政区域编码 */
            strCivilCode = strGroupCodeTmp1;

            /* 上报的分组编码 */
            strGroupCode = strGroupCodeTmp1;
            strGroupCode += (char*)"00000000";
            strGroupCode += (char*)"2160";
            strGroupCode += strGroupCodeTmp5;
            strGroupCode += strGroupCodeTmp6;
            strGroupCode += strGroupCodeTmp7;

            /* 上级组织编码 */
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
                 && 0 == sstrcmp(strGroupCodeTmp5, (char*)"00")) /* 市级编码 */
        {
            /* 行政区域编码 */
            strCivilCode = strGroupCodeTmp1;
            strCivilCode += strGroupCodeTmp2;

            /* 上报的分组编码 */
            strGroupCode = strGroupCodeTmp1;
            strGroupCode += strGroupCodeTmp2;

            /* 上级组织编码 */
            strParentCode = "";
        }
        else if (0 != sstrcmp(strGroupCodeTmp1, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp2, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp3, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp4, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp5, (char*)"00")) /* 市级下面直属分组 */
        {
            /* 行政区域编码 */
            strCivilCode = strGroupCodeTmp1;
            strCivilCode += strGroupCodeTmp2;

            /* 上报的分组编码 */
            strGroupCode = strGroupCodeTmp1;
            strGroupCode += strGroupCodeTmp2;
            strGroupCode += (char*)"000000";
            strGroupCode += (char*)"2160";
            strGroupCode += strGroupCodeTmp5;
            strGroupCode += strGroupCodeTmp6;
            strGroupCode += strGroupCodeTmp7;

            /* 上级组织编码 */
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
                 && 0 == sstrcmp(strGroupCodeTmp5, (char*)"00")) /* 区级编码 */
        {
            /* 行政区域编码 */
            strCivilCode = strGroupCodeTmp1;
            strCivilCode += strGroupCodeTmp2;
            strCivilCode += strGroupCodeTmp3;

            /* 上报的分组编码 */
            strGroupCode = strGroupCodeTmp1;
            strGroupCode += strGroupCodeTmp2;
            strGroupCode += strGroupCodeTmp3;

            /* 上级组织编码 */
            strParentCode = "";
        }
        else if (0 != sstrcmp(strGroupCodeTmp1, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp2, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp3, (char*)"00")
                 && 0 == sstrcmp(strGroupCodeTmp4, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp5, (char*)"00")) /* 区级下面直属分组 */
        {
            /* 行政区域编码 */
            strCivilCode = strGroupCodeTmp1;
            strCivilCode += strGroupCodeTmp2;
            strCivilCode += strGroupCodeTmp3;

            /* 上报的分组编码 */
            strGroupCode = strGroupCodeTmp1;
            strGroupCode += strGroupCodeTmp2;
            strGroupCode += strGroupCodeTmp3;
            strGroupCode += (char*)"0000";
            strGroupCode += (char*)"2160";
            strGroupCode += strGroupCodeTmp5;
            strGroupCode += strGroupCodeTmp6;
            strGroupCode += strGroupCodeTmp7;

            /* 上级组织编码 */
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
                 && 0 == sstrcmp(strGroupCodeTmp5, (char*)"00")) /* 派出所编码 */
        {
            /* 行政区域编码 */
            strCivilCode = strGroupCodeTmp1;
            strCivilCode += strGroupCodeTmp2;
            strCivilCode += strGroupCodeTmp3;
            strCivilCode += strGroupCodeTmp4;

            /* 上报的分组编码 */
            strGroupCode = strGroupCodeTmp1;
            strGroupCode += strGroupCodeTmp2;
            strGroupCode += strGroupCodeTmp3;
            strGroupCode += strGroupCodeTmp4;

            /* 上级组织编码 */
            strParentCode = "";
        }
        else if (0 != sstrcmp(strGroupCodeTmp1, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp2, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp3, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp4, (char*)"00")
                 && 0 != sstrcmp(strGroupCodeTmp5, (char*)"00")) /* 派出所下面直属分组 */
        {
            /* 行政区域编码 */
            strCivilCode = strGroupCodeTmp1;
            strCivilCode += strGroupCodeTmp2;
            strCivilCode += strGroupCodeTmp3;
            strCivilCode += strGroupCodeTmp4;

            /* 上报的分组编码 */
            strGroupCode = strGroupCodeTmp1;
            strGroupCode += strGroupCodeTmp2;
            strGroupCode += strGroupCodeTmp3;
            strGroupCode += strGroupCodeTmp4;
            strGroupCode += (char*)"00";
            strGroupCode += (char*)"2160";
            strGroupCode += strGroupCodeTmp5;
            strGroupCode += strGroupCodeTmp6;
            strGroupCode += strGroupCodeTmp7;

            /* 上级组织编码 */
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

        if (0 != sstrcmp(strGroupCodeTmp8, (char*)"00")) /* 超过三级的虚拟组织暂时不上报给上级 */
        {
            iNeedToUpLoad = 0;
        }
        else
        {
            iNeedToUpLoad = 1;
        }

        if (!strParentCode.empty())
        {
            /* 加入到队列中 */
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
            /* 加入到队列中 */
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
 函 数 名  : load_device_group_map_cfg_from_db
 功能描述  : 加载逻辑设备分组关系
 输入参数  : DBOper* pDbOper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

        /* 添加到队列 */
        ret = device_map_group_add((char*)strGroupID.c_str(), uDeviceIndex, iSortID);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "load_device_group_map_cfg_from_db() device_map_group_add:GroupID=%s, DeviceIndex=%u, SortID=%d, i=%d", (char*)strGroupID.c_str(), uDeviceIndex, iSortID, ret);
    }
    while (pDbOper->MoveNext() >= 0);

    return 0;
}

/*****************************************************************************
 函 数 名  : set_db_data_to_tsu_info_list
 功能描述  : 加载TSU单板数据
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月14日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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

        /* 添加到TSU 队列 */
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
 Description  : 加载数据库数据
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

        /* 设备索引*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_ivalue);

        pGBDeviceInfo->id = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->id:%d", pGBDeviceInfo->id);


        /* 设备统一编号 */
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


        /* IP地址 */
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


        /* 设备类型(前端摄像机、NVR、互联CMS、TSU) */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("DeviceType", tmp_ivalue);

        pGBDeviceInfo->device_type = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->device_type:%d \r\n", pGBDeviceInfo->device_type);


        /* 联网类型:0:上下级，1：同级，默认0 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("LinkType", tmp_ivalue);

        pGBDeviceInfo->link_type = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->link_type:%d", pGBDeviceInfo->link_type);


        /* 传输方式 */
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


        /* 厂家信息 */
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

        /* 平台设备，默认是第三方，防止DeviceInfo消息不回应 */
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

        /* 添加到队列 */
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
 Description  : 加载数据库数据
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

        /* 设备索引*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_ivalue);

        pZRVDeviceInfo->id = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->id:%d", pGBDeviceInfo->id);

        /* IP地址 */
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


        /* 设备类型(前端摄像机、NVR、互联CMS、TSU) */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Status", tmp_ivalue);

        pZRVDeviceInfo->reg_status = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_Device_info_list() pGBDeviceInfo->device_type:%d \r\n", pGBDeviceInfo->device_type);


        /* 联网类型:0:上下级，1：同级，默认0 */
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

        /* 添加到队列 */
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
 Description  : 加载数据库数据
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

        /* 设备索引*/
        tmp_uivalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_uivalue);

        pGBLogicDeviceInfo->id = tmp_uivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->id:%u", pGBLogicDeviceInfo->id);


        /* 点位统一编号 */
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


        /* 所属的CMS 统一编号 */
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


        /* 点位名称 */
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


        /* 是否启用 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Enable", tmp_ivalue);

        pGBLogicDeviceInfo->enable = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->enable:%d", pGBLogicDeviceInfo->enable);


        /* 设备类型*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("DeviceType", tmp_ivalue);

        pGBLogicDeviceInfo->device_type = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->device_type:%d", pGBLogicDeviceInfo->device_type);


        /* 报警设备子类型 */
        tmp_uivalue = 0;
        g_DBOper.GetFieldValue("Resved1", tmp_uivalue);

        pGBLogicDeviceInfo->alarm_device_sub_type = tmp_uivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->alarm_device_sub_type:%u", pGBLogicDeviceInfo->alarm_device_sub_type);


        /* 是否可控*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("CtrlEnable", tmp_ivalue);

        pGBLogicDeviceInfo->ctrl_enable = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->ctrl_enable:%d", pGBLogicDeviceInfo->ctrl_enable);


        /* 是否支持对讲，默认值0 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("MicEnable", tmp_ivalue);

        pGBLogicDeviceInfo->mic_enable = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->mic_enable:%d", pGBLogicDeviceInfo->mic_enable);


        /* 帧率，默认25 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("FrameCount", tmp_ivalue);

        pGBLogicDeviceInfo->frame_count = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->frame_count:%d", pGBLogicDeviceInfo->frame_count);


        /* 告警时长 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("AlarmLengthOfTime", tmp_ivalue);

        pGBLogicDeviceInfo->alarm_duration = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->alarm_duration:%d", pGBLogicDeviceInfo->alarm_duration);


        /* 对应的媒体物理设备索引*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("PhyDeviceIndex", tmp_ivalue);

        pGBLogicDeviceInfo->phy_mediaDeviceIndex = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->phy_mediaDeviceIndex:%d", pGBLogicDeviceInfo->phy_mediaDeviceIndex);


        /* 对应的媒体物理设备通道 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("PhyDeviceChannel", tmp_ivalue);

        pGBLogicDeviceInfo->phy_mediaDeviceChannel = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->phy_mediaDeviceChannel:%d", pGBLogicDeviceInfo->phy_mediaDeviceChannel);


        /* 对应的控制物理设备索引 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("CtrlDeviceIndex", tmp_ivalue);

        pGBLogicDeviceInfo->phy_ctrlDeviceIndex = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->phy_ctrlDeviceIndex:%d", pGBLogicDeviceInfo->phy_ctrlDeviceIndex);


        /* 对应的控制物理设备通道 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("CtrlDeviceChannel", tmp_ivalue);

        pGBLogicDeviceInfo->phy_ctrlDeviceChannel = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->phy_ctrlDeviceChannel:%d", pGBLogicDeviceInfo->phy_ctrlDeviceChannel);


        /* 是否支持多码流 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("StreamCount", tmp_ivalue);

        pGBLogicDeviceInfo->stream_count = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->stream_count:%d", pGBLogicDeviceInfo->stream_count);


        /* 录像类型 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("RecordType", tmp_ivalue);

        pGBLogicDeviceInfo->record_type = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->record_type:%d", pGBLogicDeviceInfo->record_type);


        /* 是否是其他域 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("OtherRealm", tmp_ivalue);

        pGBLogicDeviceInfo->other_realm = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->other_realm:%d", pGBLogicDeviceInfo->other_realm);


        /* 设备生产商 */
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


        /* 设备型号 */
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


        /* 设备归属 */
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
        /* 行政区域 */
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

        /* 警区 */
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


        /* 安装地址 */
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


        /* 是否有子设备 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Parental", tmp_ivalue);

        pGBLogicDeviceInfo->parental = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->parental:%d", pGBLogicDeviceInfo->parental);


        /* 父设备/区域/系统ID */
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


        /* 信令安全模式*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("SafetyWay", tmp_ivalue);

        pGBLogicDeviceInfo->safety_way = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->safety_way:%d", pGBLogicDeviceInfo->safety_way);


        /* 注册方式 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("RegisterWay", tmp_ivalue);

        pGBLogicDeviceInfo->register_way = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->safety_way:%d", pGBLogicDeviceInfo->register_way);


        /* 证书序列号*/
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


        /* 证书有效标识 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Certifiable", tmp_ivalue);

        pGBLogicDeviceInfo->certifiable = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->certifiable:%d", pGBLogicDeviceInfo->certifiable);


        /* 无效原因码 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("ErrCode", tmp_ivalue);

        pGBLogicDeviceInfo->error_code = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->error_code:%d", pGBLogicDeviceInfo->error_code);


        /* 证书终止有效期*/
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


        /* 保密属性 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Secrecy", tmp_ivalue);

        pGBLogicDeviceInfo->secrecy = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->error_code:%d", pGBLogicDeviceInfo->secrecy);


        /* IP地址*/
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


        /* 端口号 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Port", tmp_ivalue);

        pGBLogicDeviceInfo->port = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->port:%d", pGBLogicDeviceInfo->port);


        /* 密码*/
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


        /* 经度 */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("Longitude", tmp_svalue);

        pGBLogicDeviceInfo->longitude = strtod(tmp_svalue.c_str(), (char**) NULL);
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list() pGBLogicDeviceInfo->longitude:%f", pGBLogicDeviceInfo->longitude);


        /* 纬度 */
        tmp_svalue.clear();
        g_DBOper.GetFieldValue("Latitude", tmp_svalue);

        pGBLogicDeviceInfo->latitude = strtod(tmp_svalue.c_str(), (char**) NULL);
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_LogicDevice_info_list:device_id=%s, longitude=%.16lf, latitude=%.16lf \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->longitude, pGBLogicDeviceInfo->latitude);


        /* 所属的图层 */
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

        /* 鹰眼相机对应的预案ID */
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

        /* 增加物理设备信息 */
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

        /* 加入逻辑设备队列 */
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
 Description  : 加载数据库数据
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

        /* 数据库索引*/
        tmp_uivalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_uivalue);

        pRecordInfo->uID = tmp_uivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->uID:%d", pRecordInfo->uID);


        /* 逻辑设备统索引*/
        tmp_uivalue = 0;
        g_DBOper.GetFieldValue("DeviceIndex", tmp_uivalue);

        pRecordInfo->device_index = tmp_uivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->device_index:%u", pRecordInfo->device_index);


        /* 是否启动录像*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("RecordEnable", tmp_ivalue);

        pRecordInfo->record_enable = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->record_enable:%d", pRecordInfo->record_enable);


        /* 录像天数 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Days", tmp_ivalue);

        pRecordInfo->record_days = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->record_days:%d", pRecordInfo->record_days);


        /* 录像时长 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("TimeLength", tmp_ivalue);

        pRecordInfo->record_timeLen = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->record_timeLen:%d", pRecordInfo->record_timeLen);


        /* 录像类型 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Type", tmp_ivalue);

        pRecordInfo->record_type = tmp_ivalue;

        if (pRecordInfo->record_type < EV9000_RECORD_TYPE_NORMAL || pRecordInfo->record_type > EV9000_RECORD_TYPE_BACKUP)
        {
            pRecordInfo->record_type = EV9000_RECORD_TYPE_NORMAL;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->record_type:%d", pRecordInfo->record_type);


        /* 是否指定录像 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("AssignRecord", tmp_ivalue);

        pRecordInfo->assign_record = tmp_ivalue;

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->assign_record:%d", pRecordInfo->assign_record);


        /* 指定录像的TSU索引 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("TSUIndex", tmp_ivalue);

        pRecordInfo->assign_tsu_index = tmp_ivalue;

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->assign_tsu_index:%d", pRecordInfo->assign_tsu_index);


        /* 码流类型 */
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


        /* 是否全周录像 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("TimeOfAllWeek", tmp_ivalue);

        pRecordInfo->TimeOfAllWeek = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() pRecordInfo->TimeOfAllWeek:%d", pRecordInfo->TimeOfAllWeek);

        /* 所需要的带宽 */
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

        /* 根据录像类型和逻辑设备索引查找录像信息 */
        pOldRecordInfo = record_info_get_by_record_type(pRecordInfo->device_index, pRecordInfo->record_type);

        if (NULL != pOldRecordInfo)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list() record_info_get_by_record_type:device_index=%u, record_type=%d", pRecordInfo->device_index, pRecordInfo->record_type);

            pOldRecordInfo->del_mark = 0;

            if (pOldRecordInfo->record_enable != pRecordInfo->record_enable) /* 是否启动录像 */
            {
                pOldRecordInfo->record_enable = pRecordInfo->record_enable;
            }

            if (pOldRecordInfo->record_days != pRecordInfo->record_days) /* 录像天数 */
            {
                pOldRecordInfo->record_days = pRecordInfo->record_days;
            }

            if (pOldRecordInfo->record_timeLen != pRecordInfo->record_timeLen) /* 录像时长 */
            {
                pOldRecordInfo->record_timeLen = pRecordInfo->record_timeLen;
            }

            if (pOldRecordInfo->record_type != pRecordInfo->record_type) /* 录像类型 */
            {
                pOldRecordInfo->record_type = pRecordInfo->record_type;
            }

            if (pOldRecordInfo->assign_record != pRecordInfo->assign_record) /* 是否指定录像 */
            {
                pOldRecordInfo->assign_record = pRecordInfo->assign_record;
                pOldRecordInfo->del_mark = 2;
            }

            if (pOldRecordInfo->assign_tsu_index != pRecordInfo->assign_tsu_index) /* 指定录像的TSU索引 */
            {
                pOldRecordInfo->assign_tsu_index = pRecordInfo->assign_tsu_index;
                pOldRecordInfo->del_mark = 2;
            }

            if (pOldRecordInfo->stream_type != pRecordInfo->stream_type) /* 码流类型 */
            {
                pOldRecordInfo->stream_type = pRecordInfo->stream_type;
                pOldRecordInfo->del_mark = 2;
            }

            if (pOldRecordInfo->bandwidth != pRecordInfo->bandwidth) /* 前端带宽*/
            {
                pOldRecordInfo->bandwidth = pRecordInfo->bandwidth;
            }

            if (pOldRecordInfo->TimeOfAllWeek != pRecordInfo->TimeOfAllWeek) /* 是否全周录像 */
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

            /* 添加到队列 */
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
 函 数 名  : set_db_data_to_record_info_list_by_plan_action_info
 功能描述  : 将预案动作为3的类型，报警录像加入到录像队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月3日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
    strSQL = "select * from PlanActionConfig WHERE Type = 3"; /* 加载预案动作类型为3的报警录像，加入到录像队列 */

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

        /* 索引固定为0 */
        pRecordInfo->uID = 0;

        /* 逻辑设备统索引*/
        uDeviceIndex = 0;
        g_DBOper.GetFieldValue("DeviceIndex", uDeviceIndex);

        pRecordInfo->device_index = uDeviceIndex;

        /* 是否启动录像*/
        pRecordInfo->record_enable = 1;

        /* 录像天数 */
        pRecordInfo->record_days = 1;

        /* 录像时长 */
        pRecordInfo->record_timeLen = 10;

        /* 录像类型 */
        pRecordInfo->record_type = EV9000_RECORD_TYPE_ALARM;

        /* 码流类型 */
        pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;

        /* 是否指定录像 */
        pRecordInfo->assign_record = 0;

        /* 是否全周录像 */
        pRecordInfo->TimeOfAllWeek = 0;

        /* 前端带宽 */
        pRecordInfo->bandwidth = 1;

        /* 判断设备索引是否正确 */
        if (pRecordInfo->device_index <= 0)
        {
            record_info_free(pRecordInfo);
            pRecordInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_plan_action_info() device_index Error");
            continue;
        }

        /* 根据录像流类型查找录像记录 */
        record_info_pos = record_info_find_by_stream_type(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER);

        if (record_info_pos >= 0)
        {
            pOldRecordInfo = record_info_get(record_info_pos);

            if (NULL == pOldRecordInfo)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list_by_plan_action_info() New Record Info:device_index=%u, record_type=%d", pRecordInfo->device_index, pRecordInfo->record_type);

                /* 添加到队列 */
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

            /* 添加到队列 */
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
 函 数 名  : set_db_data_to_record_info_list_by_shdb_daily_upload_pic
 功能描述  : 根据上海地标日常上传图片任务，添加录像策略
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月19日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

        /* 索引固定为0 */
        pRecordInfo->uID = 0;

        /* 逻辑设备统索引*/
        uDeviceIndex = 0;
        g_DBOper.GetFieldValue("DeviceIndex", uDeviceIndex);

        pRecordInfo->device_index = uDeviceIndex;

        /* 是否启动录像*/
        pRecordInfo->record_enable = 1;

        /* 录像天数 */
        pRecordInfo->record_days = 1;

        /* 录像时长 */
        pRecordInfo->record_timeLen = 10;

        /* 录像类型 */
        pRecordInfo->record_type = EV9000_RECORD_TYPE_NORMAL;

        /* 码流类型 */
        pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;

        /* 是否指定录像 */
        pRecordInfo->assign_record = 0;

        /* 是否全周录像 */
        pRecordInfo->TimeOfAllWeek = 0;

        /* 前端带宽 */
        pRecordInfo->bandwidth = 1;

        /* 判断设备索引是否正确 */
        if (pRecordInfo->device_index <= 0)
        {
            record_info_free(pRecordInfo);
            pRecordInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_shdb_daily_upload_pic() device_index Error");
            continue;
        }

        /* 根据录像流类型查找录像记录 */
        record_info_pos = record_info_find_by_stream_type(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER);

        if (record_info_pos >= 0)
        {
            pOldRecordInfo = record_info_get(record_info_pos);

            if (NULL == pOldRecordInfo)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list_by_shdb_daily_upload_pic() New Record Info:device_index=%u, record_type=%d", pRecordInfo->device_index, pRecordInfo->record_type);

                /* 添加到队列 */
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

            /* 添加到队列 */
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
 函 数 名  : set_db_data_to_record_info_list_by_shdb_alarm_upload_pic
 功能描述  : 根据上海地标报警上传图片任务，添加录像策略
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月22日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
    strSQL = "select * from PlanActionConfig WHERE Type = 5"; /* 加载预案动作类型为5的报警截图上传，加入到录像队列 */

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

        /* 索引固定为0 */
        pRecordInfo->uID = 0;

        /* 逻辑设备统索引*/
        uDeviceIndex = 0;
        g_DBOper.GetFieldValue("DeviceIndex", uDeviceIndex);

        pRecordInfo->device_index = uDeviceIndex;

        /* 是否启动录像*/
        pRecordInfo->record_enable = 1;

        /* 录像天数 */
        pRecordInfo->record_days = 1;

        /* 录像时长 */
        pRecordInfo->record_timeLen = 10;

        /* 录像类型 */
        pRecordInfo->record_type = EV9000_RECORD_TYPE_NORMAL;

        /* 码流类型 */
        pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;

        /* 是否指定录像 */
        pRecordInfo->assign_record = 0;

        /* 是否全周录像 */
        pRecordInfo->TimeOfAllWeek = 0;

        /* 前端带宽 */
        pRecordInfo->bandwidth = 1;

        /* 判断设备索引是否正确 */
        if (pRecordInfo->device_index <= 0)
        {
            record_info_free(pRecordInfo);
            pRecordInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_record_info_list_by_shdb_alarm_upload_pic() device_index Error");
            continue;
        }

        /* 根据录像流类型查找录像记录 */
        record_info_pos = record_info_find_by_stream_type(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER);

        if (record_info_pos >= 0)
        {
            pOldRecordInfo = record_info_get(record_info_pos);

            if (NULL == pOldRecordInfo)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_record_info_list_by_shdb_alarm_upload_pic() New Record Info:device_index=%u, record_type=%d", pRecordInfo->device_index, pRecordInfo->record_type);

                /* 添加到队列 */
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

            /* 添加到队列 */
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
 Description  : 加载数据库数据
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

        /* 轮巡id */
        tmp_uvalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_uvalue);

        pPollSrv->poll_id = tmp_uvalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_poll_srv_list() pPollSrv->poll_id:%u", pPollSrv->poll_id);


        /* 轮巡名称 */
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


        /* 开始时间 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("StartTime", tmp_ivalue);

        pPollSrv->start_time = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_poll_srv_list() pPollSrv->start_time:%d", pPollSrv->start_time);


        /* 持续时间 */
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

        /* 添加到队列 */
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
 Description  : 加载数据库数据
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

        /* 预案id */
        tmp_uvalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_uvalue);

        pPlanSrv->plan_id = tmp_uvalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_plan_srv_list() pPlanSrv->plan_id:%u", pPlanSrv->plan_id);


        /* 预案名称 */
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


        /* 开始时间 */
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
 Description  : 加载数据库数据
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

        /* 巡航id */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_ivalue);

        pCruiseSrv->cruise_id = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_cruise_srv_list() pCruiseSrv->cruise_id: %d", pCruiseSrv->cruise_id);


        /* 巡航名称 */
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


        /* 开始时间 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("StartTime", tmp_ivalue);

        pCruiseSrv->start_time = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_cruise_srv_list() pCruiseSrv->start_time: %d", pCruiseSrv->start_time);


        /* 持续时间 */
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

        /* 添加到队列 */
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
 函 数 名  : add_default_mms_route_info_to_route_info_list
 功能描述  : 添加默认的mms注册路由到路由队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月20日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 将下级CMS的IP地址取出 */
    SubCmsIPVector.clear();
    AddAllSubCMSIPToVector(SubCmsIPVector);

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "add_default_mms_route_info_to_route_info_list() SubCmsIPVector.size()=%d \r\n", (int)SubCmsIPVector.size());

    /* 将特定的ip地址过滤掉 */
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

    /* 添加到队列 */
    if (route_info_add(pRouteInfo) < 0)
    {
        route_info_free(pRouteInfo);
        pRouteInfo = NULL;
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "add_default_mms_route_info_to_route_info_list() Route Info Add Error");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "添加默认的手机MMS上级路由配置信息: MMS ID=%s, MMS IP=%s, MMS Port=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Add default MMS Route Info: MMS ID=%s, MMS IP=%s, MMS Port=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    return 0;
}

/*****************************************************************************
 Prototype    : set_db_data_to_route_info_list
 Description  : 加载数据库数据
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

        /* 索引 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_ivalue);

        pRouteInfo->id = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->id: %d", pRouteInfo->id);


        /* 上级服务器CMS统一编号id */
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


        /* 上级服务器CMS ip地址*/
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


        /* 上级服务器CMS 域名地址 */
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


        /* 上级服务器CMS 端口号*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("ServerPort", tmp_ivalue);

        pRouteInfo->server_port = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->server_port: %d", pRouteInfo->server_port);


        /* 注册账号 */
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


        /* 注册密码 */
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


        /* 联网类型*/
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("LinkType", tmp_ivalue);

        pRouteInfo->link_type = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->link_type: %d", pRouteInfo->link_type);


        /* 是否是第三方平台 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("Resved1", tmp_ivalue);

        pRouteInfo->three_party_flag = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->three_party_flag: %d", pRouteInfo->three_party_flag);


        /* 传输协议类型 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("TransferProtocol", tmp_ivalue);

        pRouteInfo->trans_protocol = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->trans_protocol: %d", pRouteInfo->trans_protocol);


        /* 本地使用的网口名称 */
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

        if (IsLocalHost(pRouteInfo->server_ip) && 0 == sstrcmp(pRouteInfo->server_id, local_cms_id_get())) /* 过滤掉和本地CMS配置一样的上级CMS */
        {
            route_info_free(pRouteInfo);
            pRouteInfo = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_db_data_to_route_info_list() Server Info Is Match Local CMS");
            continue;
        }

        /* 将下级CMS的IP地址取出 */
        SubCmsIPVector.clear();
        AddAllSubCMSIPToVector(SubCmsIPVector);

        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "set_db_data_to_route_info_list() SubCmsIPVector.size()=%d \r\n", (int)SubCmsIPVector.size());

        /* 将特定的ip地址过滤掉 */
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

        /* 添加到队列 */
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
 Description  : 加载数据库数据
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

        /* 索引 */
        tmp_ivalue = 0;
        g_DBOper.GetFieldValue("ID", tmp_ivalue);

        pPlatfromInfo->id = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->id: %d", pRouteInfo->id);

        /* 上级服务器CMS ip地址*/
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

        /* 添加到队列 */
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
 函 数 名  : get_platform_last_task_time_and_status
 功能描述  : 获取平台的最后时间以及状态
 输入参数  : char* platform_ip
             DBOper* pDbOper
             int* iTaskTime
             int* iTaskStatus
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年8月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : get_platform_task_mode_and_time
 功能描述  : 获取平台的任务执行模式和时间
 输入参数  : char* platform_ip
             DBOper* pDbOper
             int* iTaskMode
             int* iTaskBeginTime
             int* iTaskEndTime
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年8月22日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : set_db_data_to_compress_task_list
 功能描述  : 加载上次没有结束的任务
 输入参数  : char* platform_ip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年8月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

        /* 记录编号 */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("RecordNum", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.jlbh, (char*)tmp_svalue.c_str(), MAX_TSU_TASK_LEN);

        /* 文件名称 */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("FileName", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.wjmc, (char*)tmp_svalue.c_str(), 128);

        /* 文件后缀 */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("FileSuffixName", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.kzm, (char*)tmp_svalue.c_str(), 32);

        /* 文件大小 */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("FileSize", tmp_ivalue);
        compress_task->stYSPB.wjdx = tmp_ivalue;

        /* 上传单位 */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("UploadUnit", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.scdw, (char*)tmp_svalue.c_str(), 128);

        /* 上传时间 */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("UploadTime", tmp_ivalue);
        compress_task->stYSPB.scsj = tmp_ivalue;

        /* 存储路径 */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("StoragePath", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.cclj, (char*)tmp_svalue.c_str(), 128);

        /* 分配标识 */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("AssignFlag", tmp_ivalue);
        compress_task->iAssignFlag = tmp_ivalue;

        /* 平台IP地址 */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("PlatformIP", tmp_svalue);
        osip_strncpy(compress_task->strPlatformIP, (char*)tmp_svalue.c_str(), MAX_IP_LEN);

        /* ZRV IP地址 */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("ZRVDeviceIP", tmp_svalue);
        osip_strncpy(compress_task->strZRVDeviceIP, (char*)tmp_svalue.c_str(), MAX_IP_LEN);

        /* 任务状态 */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("TaskStatus", tmp_ivalue);
        compress_task->iTaskStatus = tmp_ivalue;

        /* 任务结果 */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("TaskResult", tmp_ivalue);
        compress_task->iTaskResult = tmp_ivalue;

        compress_task->wait_answer_expire = 0;
        compress_task->resend_count = 0;

        /* 添加到队列 */
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
 函 数 名  : set_db_data_to_preset_auto_back_list
 功能描述  : 加载预置位自动归位信息到队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月18日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

        /* 逻辑设备索引 */
        g_DBOper.GetFieldValue("DeviceIndex", uDeviceIndex);

        /* 预置位编号 */
        g_DBOper.GetFieldValue("PresetID", uPresetID);

        /* 预置位自动归位时长 */
        g_DBOper.GetFieldValue("Resved1", iResved1);

        if (iResved1 >= 60)
        {
            /* 添加到队列 */
            ret = preset_auto_back_use(uDeviceIndex, uPresetID, iResved1);
        }
        else
        {
            /* 添加到队列 */
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
 函 数 名  : UpdateAllUserRegStatus2DB
 功能描述  : 更新所有用户数据库注册状态
 输入参数  : int status
             DBOper* pUser_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月9日 星期五
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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

    /* 更新数据库 */
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
 函 数 名  : UpdateAllGBDeviceRegStatus2DB
 功能描述  : 更新所有标准物理设备数据库注册状态
 输入参数  : int status
                            DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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

    /* 更新数据库 */
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
 函 数 名  : UpdateAllGBLogicDeviceRegStatus2DB
 功能描述  : 更新所有标准逻辑设备数据库注册状态
 输入参数  : int status
                            DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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

    /* 更新数据库 */
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
 函 数 名  : UpdateAllZRVDeviceRegStatus2DB
 功能描述  : 更新所有标准物理设备数据库注册状态
 输入参数  : int status
                            DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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

    /* 更新数据库 */
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
 函 数 名  : UpdateMgwOptionsIPAddress2DB
 功能描述  : 更新媒体网关的IP地址参数
 输入参数  : char* pcIPAddr
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年9月28日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 更新数据库 */
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
 函 数 名  : InitAllGBDeviceRegStatus
 功能描述  : 初始化所有的设备注册状态
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月27日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : InitAllZRVDeviceRegStatus
 功能描述  : 初始化所有的设备注册状态
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月27日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void InitAllZRVDeviceRegStatus()
{
    UpdateAllZRVDeviceRegStatus2DB(0, &g_DBOper);
    DeleteCompressTaskFromDBForStart(&g_DBOper);
    return;
}

/*****************************************************************************
 函 数 名  : InitAllUserRegStatus
 功能描述  : 初始化所有的用户注册状态
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年4月15日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void InitAllUserRegStatus()
{
    UpdateAllUserRegInfo2DB(0, &g_DBOper);
    DeleteAllUserRegInfoFromDB(&g_DBOper);
}

/*****************************************************************************
 函 数 名  : SetBoardConfigTable
 功能描述  : 配置单板信息到数据库
 输入参数  : char* pcBoardID
              int iSlotID
              int iBoardType
              DBOper* ptDBoper
              int* iAssignRecord
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月1日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 获取单板配置表BoardConfig 的数据*/
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

    if (record_count < 0) /* 数据库错误 */
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardConfigTable() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0) /* 没有记录 */
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

        if (record_count < 0) /* 数据库错误 */
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardConfigTable() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
            return -1;
        }
        else if (record_count == 0) /* 没有查到记录 */
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "SetBoardConfigTable() exit---: No Record Count \r\n");
            return -1;
        }

        /* 获取数据库记录信息 */
        ptDBoper->GetFieldValue("ID", iBoardIndex);
        ptDBoper->GetFieldValue("AssignRecord", iTmpAssignRecord);
    }
    else
    {
        /* 获取数据库记录信息 */
        ptDBoper->GetFieldValue("ID", iBoardIndex);
        ptDBoper->GetFieldValue("SlotID", iTmpSlotID);
        ptDBoper->GetFieldValue("BoardID", strBoardID);
        ptDBoper->GetFieldValue("CMSID", strCMSID);
        ptDBoper->GetFieldValue("AssignRecord", iTmpAssignRecord);

        if (iTmpSlotID != iSlotID
            || 0 != sstrcmp((char*)strBoardID.c_str(), pcBoardID)
            || 0 != sstrcmp((char*)strCMSID.c_str(), local_cms_id_get())) /* 有变化，更新到数据库 */
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
 函 数 名  : UpdateBoardConfigTableStatus
 功能描述  : 更新单板配置表的状态字段
 输入参数  : char* pcBoardID
             int iBoardType
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : UpdateAllBoardConfigTableStatus
 功能描述  : 更新单板配置表的状态所有字段
 输入参数  : int iStatus
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : SetBoardNetConfigTable
 功能描述  : 配置单板的网络信息
 输入参数  : int iBoardIndex
             int iIPEth
             char* pcIPAddr
             int iIPUsedFlag
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月1日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 获取单板网络配置表BoardNetConfig 设备网的数据*/
    strSQL.clear();
    strSQL = "select * from BoardNetConfig";
    strSQL += " WHERE BoardIndex = ";
    strSQL += strBoardIndex;
    strSQL += " AND EthID = ";
    strSQL += strIPEth;

    record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "SetBoardNetConfigTable() record_count=%d \r\n", record_count);

    if (record_count < 0) /* 数据库错误 */
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardNetConfigTable() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardNetConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0) /* 没有记录 */
    {
        if (NULL == pcIPAddr || pcIPAddr[0] == '\0')
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetBoardNetConfigTable() exit---: Not Config Net IP \r\n");
        }
        else /* 插入新的数据 */
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
        if (NULL == pcIPAddr || pcIPAddr[0] == '\0')/* 将 IP 地址删除 */
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
        else /* 看是否需要更新IP地址 */
        {
            strIP.clear();
            ptDBoper->GetFieldValue("IP", strIP);
            ptDBoper->GetFieldValue("Enable", iEnable);
            ptDBoper->GetFieldValue("CMSID", strCMSID);

            if (strIP.empty()
                || (!strIP.empty() && 0 != sstrcmp((char*)strIP.c_str(), pcIPAddr))
                || (!strCMSID.empty() && 0 != sstrcmp((char*)strCMSID.c_str(), local_cms_id_get()))
                || iEnable != iIPUsedFlag) /* 有变化，需要更新数据库 */
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
 函 数 名  : SetCMSBoardInfoToDB
 功能描述  : 设置CMS的单板及网络配置信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月1日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
            else if (0 == strncmp(pIPaddr->eth_name, (char*)"bond", 4)) /* 绑定网口的支持 */
            {
                tmp = &pIPaddr->eth_name[4];
                iEthNum = osip_atoi(tmp);

                iRet = SetBoardNetConfigTable(iBoardIndex, iEthNum, pIPaddr->local_ip, 1, &g_DBOper);
            }
            else if (0 == strncmp(pIPaddr->eth_name, (char*)"wlan", 4)) /* 无线网口的支持 */
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

    /* 更新单板配置状态 */
    iRet = UpdateBoardConfigTableStatus(local_cms_id_get(), LOGIC_BOARD_CMS, 1, &g_DBOper);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SetCMSBoardInfoToDB() UpdateBoardConfigTableStatus Error \r\n");
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "SetCMSBoardInfoToDB() UpdateBoardConfigTableStatus OK \r\n");
    }

    /* 添加拓扑结构表信息 */
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
 函 数 名  : InsertDefaultDBData
 功能描述  : 插入数据库默认数据
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年5月16日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void InsertDefaultDBData()
{
    int iRet = 0;

    /* 插入默认的行政区域编码到数据库 */
    iRet = InsertGBCodesDefautConfig(&g_DBOper);

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "InsertDefaultDBData() InsertGBCodesDefautConfig:iRet=%d \r\n", iRet);

    iRet = InsertDefaultUserInfo((char*)"WiscomV", &g_DBOper);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "InsertDefaultDBData() InsertDefaultUserInfo:iRet=%d \r\n", iRet);

    iRet = InsertDefaultUserInfo((char*)"admin", &g_DBOper);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "InsertDefaultDBData() InsertDefaultUserInfo:iRet=%d \r\n", iRet);

    return;
}

/*****************************************************************************
 函 数 名  : DeleteUnnecessaryDBData
 功能描述  : 删除不需要的数据库数据
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年8月12日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void DeleteUnnecessaryDBData()
{
    int iRet = 0;
    string strSQL = "";

    return;
}

/*****************************************************************************
 函 数 名  : InsertGBCodesDefautConfig
 功能描述  : 插入默认的行政区域编码到数据库
 输入参数  : DBOper* pGBCodes_Info_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年5月16日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 建表 */
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

    /* 插入SQL语句 */
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3523,'110000','北京市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3524,'110100','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3525,'110101','东城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3526,'110102','西城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3527,'110105','朝阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3528,'110106','丰台区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3529,'110107','石景山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3530,'110108','海淀区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3531,'110109','门头沟区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3532,'110111','房山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3533,'110112','通州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3534,'110113','顺义区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3535,'110114','昌平区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3536,'110115','大兴区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3537,'110116','怀柔区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3538,'110117','平谷区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3539,'110200','县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3540,'110228','密云县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3541,'110229','延庆县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3542,'120000','天津市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3543,'120100','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3544,'120101','和平区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3545,'120102','河东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3546,'120103','河西区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3547,'120104','南开区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3548,'120105','河北区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3549,'120106','红桥区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3550,'120110','东丽区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3551,'120111','西青区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3552,'120112','津南区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3553,'120113','北辰区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3554,'120114','武清区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3555,'120115','宝坻区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3556,'120116','滨海新区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3557,'120200','县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3558,'120221','宁河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3559,'120223','静海县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3560,'120225','蓟县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3561,'130000','河北省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3562,'130100','石家庄市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3563,'130101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3564,'130102','长安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3565,'130103','桥东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3566,'130104','桥西区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3567,'130105','新华区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3568,'130107','井陉矿区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3569,'130108','裕华区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3570,'130121','井陉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3571,'130123','正定县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3572,'130124','栾城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3573,'130125','行唐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3574,'130126','灵寿县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3575,'130127','高邑县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3576,'130128','深泽县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3577,'130129','赞皇县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3578,'130130','无极县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3579,'130131','平山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3580,'130132','元氏县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3581,'130133','赵县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3582,'130181','辛集市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3583,'130182','藁城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3584,'130183','晋州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3585,'130184','新乐市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3586,'130185','鹿泉市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3587,'130200','唐山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3588,'130201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3589,'130202','路南区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3590,'130203','路北区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3591,'130204','古冶区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3592,'130205','开平区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3593,'130207','丰南区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3594,'130208','丰润区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3595,'130209','曹妃甸区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3596,'130223','滦县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3597,'130224','滦南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3598,'130225','乐亭县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3599,'130227','迁西县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3600,'130229','玉田县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3601,'130281','遵化市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3602,'130283','迁安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3603,'130300','秦皇岛市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3604,'130301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3605,'130302','海港区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3606,'130303','山海关区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3607,'130304','北戴河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3608,'130321','青龙满族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3609,'130322','昌黎县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3610,'130323','抚宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3611,'130324','卢龙县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3612,'130400','邯郸市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3613,'130401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3614,'130402','邯山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3615,'130403','丛台区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3616,'130404','复兴区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3617,'130406','峰峰矿区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3618,'130421','邯郸县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3619,'130423','临漳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3620,'130424','成安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3621,'130425','大名县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3622,'130426','涉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3623,'130427','磁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3624,'130428','肥乡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3625,'130429','永年县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3626,'130430','邱县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3627,'130431','鸡泽县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3628,'130432','广平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3629,'130433','馆陶县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3630,'130434','魏县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3631,'130435','曲周县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3632,'130481','武安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3633,'130500','邢台市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3634,'130501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3635,'130502','桥东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3636,'130503','桥西区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3637,'130521','邢台县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3638,'130522','临城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3639,'130523','内丘县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3640,'130524','柏乡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3641,'130525','隆尧县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3642,'130526','任县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3643,'130527','南和县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3644,'130528','宁晋县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3645,'130529','巨鹿县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3646,'130530','新河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3647,'130531','广宗县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3648,'130532','平乡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3649,'130533','威县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3650,'130534','清河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3651,'130535','临西县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3652,'130581','南宫市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3653,'130582','沙河市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3654,'130600','保定市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3655,'130601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3656,'130602','新市区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3657,'130603','北市区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3658,'130604','南市区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3659,'130621','满城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3660,'130622','清苑县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3661,'130623','涞水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3662,'130624','阜平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3663,'130625','徐水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3664,'130626','定兴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3665,'130627','唐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3666,'130628','高阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3667,'130629','容城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3668,'130630','涞源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3669,'130631','望都县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3670,'130632','安新县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3671,'130633','易县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3672,'130634','曲阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3673,'130635','蠡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3674,'130636','顺平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3675,'130637','博野县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3676,'130638','雄县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3677,'130681','涿州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3678,'130682','定州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3679,'130683','安国市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3680,'130684','高碑店市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3681,'130700','张家口市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3682,'130701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3683,'130702','桥东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3684,'130703','桥西区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3685,'130705','宣化区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3686,'130706','下花园区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3687,'130721','宣化县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3688,'130722','张北县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3689,'130723','康保县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3690,'130724','沽源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3691,'130725','尚义县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3692,'130726','蔚县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3693,'130727','阳原县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3694,'130728','怀安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3695,'130729','万全县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3696,'130730','怀来县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3697,'130731','涿鹿县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3698,'130732','赤城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3699,'130733','崇礼县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3700,'130800','承德市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3701,'130801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3702,'130802','双桥区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3703,'130803','双滦区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3704,'130804','鹰手营子矿区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3705,'130821','承德县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3706,'130822','兴隆县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3707,'130823','平泉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3708,'130824','滦平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3709,'130825','隆化县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3710,'130826','丰宁满族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3711,'130827','宽城满族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3712,'130828','围场满族蒙古族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3713,'130900','沧州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3714,'130901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3715,'130902','新华区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3716,'130903','运河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3717,'130921','沧县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3718,'130922','青县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3719,'130923','东光县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3720,'130924','海兴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3721,'130925','盐山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3722,'130926','肃宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3723,'130927','南皮县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3724,'130928','吴桥县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3725,'130929','献县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3726,'130930','孟村回族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3727,'130981','泊头市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3728,'130982','任丘市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3729,'130983','黄骅市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3730,'130984','河间市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3731,'131000','廊坊市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3732,'131001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3733,'131002','安次区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3734,'131003','广阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3735,'131022','固安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3736,'131023','永清县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3737,'131024','香河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3738,'131025','大城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3739,'131026','文安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3740,'131028','大厂回族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3741,'131081','霸州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3742,'131082','三河市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3743,'131100','衡水市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3744,'131101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3745,'131102','桃城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3746,'131121','枣强县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3747,'131122','武邑县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3748,'131123','武强县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3749,'131124','饶阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3750,'131125','安平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3751,'131126','故城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3752,'131127','景县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3753,'131128','阜城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3754,'131181','冀州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3755,'131182','深州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3756,'140000','山西省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3757,'140100','太原市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3758,'140101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3759,'140105','小店区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3760,'140106','迎泽区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3761,'140107','杏花岭区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3762,'140108','尖草坪区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3763,'140109','万柏林区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3764,'140110','晋源区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3765,'140121','清徐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3766,'140122','阳曲县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3767,'140123','娄烦县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3768,'140181','古交市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3769,'140200','大同市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3770,'140201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3771,'140202','城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3772,'140203','矿区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3773,'140211','南郊区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3774,'140212','新荣区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3775,'140221','阳高县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3776,'140222','天镇县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3777,'140223','广灵县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3778,'140224','灵丘县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3779,'140225','浑源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3780,'140226','左云县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3781,'140227','大同县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3782,'140300','阳泉市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3783,'140301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3784,'140302','城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3785,'140303','矿区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3786,'140311','郊区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3787,'140321','平定县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3788,'140322','盂县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3789,'140400','长治市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3790,'140401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3791,'140402','城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3792,'140411','郊区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3793,'140421','长治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3794,'140423','襄垣县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3795,'140424','屯留县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3796,'140425','平顺县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3797,'140426','黎城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3798,'140427','壶关县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3799,'140428','长子县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3800,'140429','武乡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3801,'140430','沁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3802,'140431','沁源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3803,'140481','潞城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3804,'140500','晋城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3805,'140501','晋城市市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3806,'140502','城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3807,'140521','沁水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3808,'140522','阳城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3809,'140524','陵川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3810,'140525','泽州县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3811,'140581','高平市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3812,'140600','朔州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3813,'140601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3814,'140602','朔城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3815,'140603','平鲁区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3816,'140621','山阴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3817,'140622','应县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3818,'140623','右玉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3819,'140624','怀仁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3820,'140700','晋中市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3821,'140701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3822,'140702','榆次区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3823,'140721','榆社县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3824,'140722','左权县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3825,'140723','和顺县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3826,'140724','昔阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3827,'140725','寿阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3828,'140726','太谷县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3829,'140727','祁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3830,'140728','平遥县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3831,'140729','灵石县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3832,'140781','介休市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3833,'140800','运城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3834,'140801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3835,'140802','盐湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3836,'140821','临猗县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3837,'140822','万荣县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3838,'140823','闻喜县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3839,'140824','稷山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3840,'140825','新绛县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3841,'140826','绛县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3842,'140827','垣曲县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3843,'140828','夏县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3844,'140829','平陆县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3845,'140830','芮城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3846,'140881','永济市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3847,'140882','河津市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3848,'140900','忻州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3849,'140901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3850,'140902','忻府区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3851,'140921','定襄县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3852,'140922','五台县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3853,'140923','代县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3854,'140924','繁峙县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3855,'140925','宁武县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3856,'140926','静乐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3857,'140927','神池县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3858,'140928','五寨县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3859,'140929','岢岚县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3860,'140930','河曲县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3861,'140931','保德县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3862,'140932','偏关县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3863,'140981','原平市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3864,'141000','临汾市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3865,'141001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3866,'141002','尧都区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3867,'141021','曲沃县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3868,'141022','翼城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3869,'141023','襄汾县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3870,'141024','洪洞县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3871,'141025','古县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3872,'141026','安泽县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3873,'141027','浮山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3874,'141028','吉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3875,'141029','乡宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3876,'141030','大宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3877,'141031','隰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3878,'141032','永和县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3879,'141033','蒲县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3880,'141034','汾西县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3881,'141081','侯马市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3882,'141082','霍州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3883,'141100','吕梁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3884,'141101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3885,'141102','离石区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3886,'141121','文水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3887,'141122','交城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3888,'141123','兴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3889,'141124','临县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3890,'141125','柳林县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3891,'141126','石楼县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3892,'141127','岚县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3893,'141128','方山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3894,'141129','中阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3895,'141130','交口县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3896,'141181','孝义市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3897,'141182','汾阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3898,'150000','内蒙古自治区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3899,'150100','呼和浩特市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3900,'150101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3901,'150102','新城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3902,'150103','回民区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3903,'150104','玉泉区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3904,'150105','赛罕区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3905,'150121','土默特左旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3906,'150122','托克托县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3907,'150123','和林格尔县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3908,'150124','清水河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3909,'150125','武川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3910,'150200','包头市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3911,'150201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3912,'150202','东河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3913,'150203','昆都仑区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3914,'150204','青山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3915,'150205','石拐区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3916,'150206','白云鄂博矿区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3917,'150207','九原区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3918,'150221','土默特右旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3919,'150222','固阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3920,'150223','达尔罕茂明安联合旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3921,'150300','乌海市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3922,'150301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3923,'150302','海勃湾区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3924,'150303','海南区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3925,'150304','乌达区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3926,'150400','赤峰市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3927,'150401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3928,'150402','红山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3929,'150403','元宝山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3930,'150404','松山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3931,'150421','阿鲁科尔沁旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3932,'150422','巴林左旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3933,'150423','巴林右旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3934,'150424','林西县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3935,'150425','克什克腾旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3936,'150426','翁牛特旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3937,'150428','喀喇沁旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3938,'150429','宁城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3939,'150430','敖汉旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3940,'150500','通辽市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3941,'150501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3942,'150502','科尔沁区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3943,'150521','科尔沁左翼中旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3944,'150522','科尔沁左翼后旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3945,'150523','开鲁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3946,'150524','库伦旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3947,'150525','奈曼旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3948,'150526','扎鲁特旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3949,'150581','霍林郭勒市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3950,'150600','鄂尔多斯市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3951,'150601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3952,'150602','东胜区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3953,'150621','达拉特旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3954,'150622','准格尔旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3955,'150623','鄂托克前旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3956,'150624','鄂托克旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3957,'150625','杭锦旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3958,'150626','乌审旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3959,'150627','伊金霍洛旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3960,'150700','呼伦贝尔市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3961,'150701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3962,'150702','海拉尔区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3963,'150721','阿荣旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3964,'150722','莫力达瓦达斡尔族自治旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3965,'150723','鄂伦春自治旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3966,'150724','鄂温克族自治旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3967,'150725','陈巴尔虎旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3968,'150726','新巴尔虎左旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3969,'150727','新巴尔虎右旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3970,'150781','满洲里市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3971,'150782','牙克石市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3972,'150783','扎兰屯市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3973,'150784','额尔古纳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3974,'150785','根河市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3975,'150800','巴彦淖尔市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3976,'150801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3977,'150802','临河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3978,'150821','五原县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3979,'150822','磴口县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3980,'150823','乌拉特前旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3981,'150824','乌拉特中旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3982,'150825','乌拉特后旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3983,'150826','杭锦后旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3984,'150900','乌兰察布市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3985,'150901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3986,'150902','集宁区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3987,'150921','卓资县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3988,'150922','化德县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3989,'150923','商都县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3990,'150924','兴和县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3991,'150925','凉城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3992,'150926','察哈尔右翼前旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3993,'150927','察哈尔右翼中旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3994,'150928','察哈尔右翼后旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3995,'150929','四子王旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3996,'150981','丰镇市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3997,'152200','兴安盟',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3998,'152201','乌兰浩特市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (3999,'152202','阿尔山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4000,'152221','科尔沁右翼前旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4001,'152222','科尔沁右翼中旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4002,'152223','扎赉特旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4003,'152224','突泉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4004,'152500','锡林郭勒盟',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4005,'152501','二连浩特市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4006,'152502','锡林浩特市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4007,'152522','阿巴嘎旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4008,'152523','苏尼特左旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4009,'152524','苏尼特右旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4010,'152525','东乌珠穆沁旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4011,'152526','西乌珠穆沁旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4012,'152527','太仆寺旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4013,'152528','镶黄旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4014,'152529','正镶白旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4015,'152530','正蓝旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4016,'152531','多伦县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4017,'152900','阿拉善盟',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4018,'152921','阿拉善左旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4019,'152922','阿拉善右旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4020,'152923','额济纳旗',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4021,'210000','辽宁省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4022,'210100','沈阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4023,'210101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4024,'210102','和平区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4025,'210103','沈河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4026,'210104','大东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4027,'210105','皇姑区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4028,'210106','铁西区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4029,'210111','苏家屯区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4030,'210112','东陵区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4031,'210113','沈北新区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4032,'210114','于洪区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4033,'210122','辽中县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4034,'210123','康平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4035,'210124','法库县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4036,'210181','新民市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4037,'210200','大连市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4038,'210201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4039,'210202','中山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4040,'210203','西岗区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4041,'210204','沙河口区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4042,'210211','甘井子区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4043,'210212','旅顺口区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4044,'210213','金州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4045,'210224','长海县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4046,'210281','瓦房店市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4047,'210282','普兰店市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4048,'210283','庄河市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4049,'210300','鞍山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4050,'210301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4051,'210302','铁东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4052,'210303','铁西区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4053,'210304','立山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4054,'210311','千山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4055,'210321','台安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4056,'210323','岫岩满族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4057,'210381','海城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4058,'210400','抚顺市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4059,'210401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4060,'210402','新抚区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4061,'210403','东洲区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4062,'210404','望花区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4063,'210411','顺城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4064,'210421','抚顺县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4065,'210422','新宾满族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4066,'210423','清原满族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4067,'210500','本溪市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4068,'210501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4069,'210502','平山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4070,'210503','溪湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4071,'210504','明山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4072,'210505','南芬区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4073,'210521','本溪满族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4074,'210522','桓仁满族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4075,'210600','丹东市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4076,'210601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4077,'210602','元宝区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4078,'210603','振兴区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4079,'210604','振安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4080,'210624','宽甸满族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4081,'210681','东港市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4082,'210682','凤城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4083,'210700','锦州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4084,'210701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4085,'210702','古塔区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4086,'210703','凌河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4087,'210711','太和区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4088,'210726','黑山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4089,'210727','义县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4090,'210781','凌海市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4091,'210782','北镇市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4092,'210800','营口市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4093,'210801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4094,'210802','站前区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4095,'210803','西市区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4096,'210804','鲅鱼圈区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4097,'210811','老边区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4098,'210881','盖州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4099,'210882','大石桥市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4100,'210900','阜新市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4101,'210901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4102,'210902','海州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4103,'210903','新邱区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4104,'210904','太平区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4105,'210905','清河门区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4106,'210911','细河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4107,'210921','阜新蒙古族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4108,'210922','彰武县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4109,'211000','辽阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4110,'211001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4111,'211002','白塔区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4112,'211003','文圣区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4113,'211004','宏伟区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4114,'211005','弓长岭区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4115,'211011','太子河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4116,'211021','辽阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4117,'211081','灯塔市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4118,'211100','盘锦市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4119,'211101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4120,'211102','双台子区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4121,'211103','兴隆台区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4122,'211121','大洼县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4123,'211122','盘山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4124,'211200','铁岭市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4125,'211201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4126,'211202','银州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4127,'211204','清河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4128,'211221','铁岭县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4129,'211223','西丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4130,'211224','昌图县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4131,'211281','调兵山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4132,'211282','开原市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4133,'211300','朝阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4134,'211301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4135,'211302','双塔区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4136,'211303','龙城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4137,'211321','朝阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4138,'211322','建平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4139,'211324','喀喇沁左翼蒙古族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4140,'211381','北票市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4141,'211382','凌源市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4142,'211400','葫芦岛市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4143,'211401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4144,'211402','连山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4145,'211403','龙港区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4146,'211404','南票区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4147,'211421','绥中县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4148,'211422','建昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4149,'211481','兴城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4150,'220000','吉林省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4151,'220100','长春市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4152,'220101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4153,'220102','南关区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4154,'220103','宽城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4155,'220104','朝阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4156,'220105','二道区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4157,'220106','绿园区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4158,'220112','双阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4159,'220122','农安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4160,'220181','九台市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4161,'220182','榆树市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4162,'220183','德惠市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4163,'220200','吉林市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4164,'220201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4165,'220202','昌邑区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4166,'220203','龙潭区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4167,'220204','船营区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4168,'220211','丰满区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4169,'220221','永吉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4170,'220281','蛟河市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4171,'220282','桦甸市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4172,'220283','舒兰市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4173,'220284','磐石市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4174,'220300','四平市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4175,'220301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4176,'220302','铁西区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4177,'220303','铁东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4178,'220322','梨树县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4179,'220323','伊通满族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4180,'220381','公主岭市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4181,'220382','双辽市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4182,'220400','辽源市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4183,'220401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4184,'220402','龙山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4185,'220403','西安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4186,'220421','东丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4187,'220422','东辽县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4188,'220500','通化市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4189,'220501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4190,'220502','东昌区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4191,'220503','二道江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4192,'220521','通化县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4193,'220523','辉南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4194,'220524','柳河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4195,'220581','梅河口市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4196,'220582','集安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4197,'220600','白山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4198,'220601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4199,'220602','浑江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4200,'220605','江源区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4201,'220621','抚松县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4202,'220622','靖宇县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4203,'220623','长白朝鲜族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4204,'220681','临江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4205,'220700','松原市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4206,'220701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4207,'220702','宁江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4208,'220721','前郭尔罗斯蒙古族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4209,'220722','长岭县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4210,'220723','乾安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4211,'220724','扶余县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4212,'220800','白城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4213,'220801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4214,'220802','洮北区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4215,'220821','镇赉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4216,'220822','通榆县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4217,'220881','洮南市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4218,'220882','大安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4219,'222400','延边朝鲜族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4220,'222401','延吉市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4221,'222402','图们市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4222,'222403','敦化市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4223,'222404','珲春市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4224,'222405','龙井市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4225,'222406','和龙市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4226,'222424','汪清县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4227,'222426','安图县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4228,'230000','黑龙江省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4229,'230100','哈尔滨市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4230,'230101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4231,'230102','道里区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4232,'230103','南岗区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4233,'230104','道外区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4234,'230108','平房区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4235,'230109','松北区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4236,'230110','香坊区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4237,'230111','呼兰区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4238,'230112','阿城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4239,'230123','依兰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4240,'230124','方正县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4241,'230125','宾县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4242,'230126','巴彦县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4243,'230127','木兰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4244,'230128','通河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4245,'230129','延寿县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4246,'230182','双城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4247,'230183','尚志市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4248,'230184','五常市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4249,'230200','齐齐哈尔市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4250,'230201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4251,'230202','龙沙区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4252,'230203','建华区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4253,'230204','铁锋区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4254,'230205','昂昂溪区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4255,'230206','富拉尔基区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4256,'230207','碾子山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4257,'230208','梅里斯达斡尔族区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4258,'230221','龙江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4259,'230223','依安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4260,'230224','泰来县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4261,'230225','甘南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4262,'230227','富裕县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4263,'230229','克山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4264,'230230','克东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4265,'230231','拜泉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4266,'230281','讷河市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4267,'230300','鸡西市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4268,'230301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4269,'230302','鸡冠区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4270,'230303','恒山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4271,'230304','滴道区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4272,'230305','梨树区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4273,'230306','城子河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4274,'230307','麻山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4275,'230321','鸡东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4276,'230381','虎林市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4277,'230382','密山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4278,'230400','鹤岗市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4279,'230401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4280,'230402','向阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4281,'230403','工农区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4282,'230404','南山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4283,'230405','兴安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4284,'230406','东山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4285,'230407','兴山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4286,'230421','萝北县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4287,'230422','绥滨县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4288,'230500','双鸭山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4289,'230501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4290,'230502','尖山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4291,'230503','岭东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4292,'230505','四方台区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4293,'230506','宝山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4294,'230521','集贤县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4295,'230522','友谊县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4296,'230523','宝清县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4297,'230524','饶河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4298,'230600','大庆市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4299,'230601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4300,'230602','萨尔图区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4301,'230603','龙凤区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4302,'230604','让胡路区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4303,'230605','红岗区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4304,'230606','大同区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4305,'230621','肇州县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4306,'230622','肇源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4307,'230623','林甸县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4308,'230624','杜尔伯特蒙古族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4309,'230700','伊春市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4310,'230701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4311,'230702','伊春区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4312,'230703','南岔区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4313,'230704','友好区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4314,'230705','西林区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4315,'230706','翠峦区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4316,'230707','新青区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4317,'230708','美溪区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4318,'230709','金山屯区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4319,'230710','五营区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4320,'230711','乌马河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4321,'230712','汤旺河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4322,'230713','带岭区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4323,'230714','乌伊岭区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4324,'230715','红星区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4325,'230716','上甘岭区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4326,'230722','嘉荫县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4327,'230781','铁力市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4328,'230800','佳木斯市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4329,'230801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4330,'230803','向阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4331,'230804','前进区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4332,'230805','东风区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4333,'230811','郊区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4334,'230822','桦南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4335,'230826','桦川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4336,'230828','汤原县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4337,'230833','抚远县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4338,'230881','同江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4339,'230882','富锦市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4340,'230900','七台河市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4341,'230901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4342,'230902','新兴区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4343,'230903','桃山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4344,'230904','茄子河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4345,'230921','勃利县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4346,'231000','牡丹江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4347,'231001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4348,'231002','东安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4349,'231003','阳明区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4350,'231004','爱民区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4351,'231005','西安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4352,'231024','东宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4353,'231025','林口县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4354,'231081','绥芬河市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4355,'231083','海林市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4356,'231084','宁安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4357,'231085','穆棱市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4358,'231100','黑河市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4359,'231101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4360,'231102','爱辉区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4361,'231121','嫩江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4362,'231123','逊克县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4363,'231124','孙吴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4364,'231181','北安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4365,'231182','五大连池市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4366,'231200','绥化市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4367,'231201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4368,'231202','北林区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4369,'231221','望奎县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4370,'231222','兰西县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4371,'231223','青冈县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4372,'231224','庆安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4373,'231225','明水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4374,'231226','绥棱县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4375,'231281','安达市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4376,'231282','肇东市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4377,'231283','海伦市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4378,'232700','大兴安岭地区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4379,'232721','呼玛县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4380,'232722','塔河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4381,'232723','漠河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4382,'310000','上海市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4383,'310100','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4384,'310101','黄浦区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4385,'310104','徐汇区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4386,'310105','长宁区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4387,'310106','静安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4388,'310107','普陀区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4389,'310108','闸北区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4390,'310109','虹口区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4391,'310110','杨浦区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4392,'310112','闵行区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4393,'310113','宝山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4394,'310114','嘉定区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4395,'310115','浦东新区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4396,'310116','金山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4397,'310117','松江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4398,'310118','青浦区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4399,'310120','奉贤区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4400,'310200','县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4401,'310230','崇明县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4402,'320000','江苏省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4403,'320100','南京市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4404,'320101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4405,'320102','玄武区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4406,'320103','白下区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4407,'320104','秦淮区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4408,'320105','建邺区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4409,'320106','鼓楼区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4410,'320107','下关区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4411,'320111','浦口区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4412,'320113','栖霞区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4413,'320114','雨花台区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4414,'320115','江宁区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4415,'320116','六合区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4416,'320124','溧水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4417,'320125','高淳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4418,'320200','无锡市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4419,'320201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4420,'320202','崇安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4421,'320203','南长区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4422,'320204','北塘区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4423,'320205','锡山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4424,'320206','惠山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4425,'320211','滨湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4426,'320281','江阴市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4427,'320282','宜兴市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4428,'320300','徐州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4429,'320301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4430,'320302','鼓楼区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4431,'320303','云龙区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4432,'320305','贾汪区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4433,'320311','泉山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4434,'320312','铜山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4435,'320321','丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4436,'320322','沛县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4437,'320324','睢宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4438,'320381','新沂市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4439,'320382','邳州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4440,'320400','常州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4441,'320401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4442,'320402','天宁区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4443,'320404','钟楼区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4444,'320405','戚墅堰区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4445,'320411','新北区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4446,'320412','武进区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4447,'320481','溧阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4448,'320482','金坛市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4449,'320500','苏州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4450,'320501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4451,'320505','虎丘区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4452,'320506','吴中区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4453,'320507','相城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4454,'320508','姑苏区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4455,'320509','吴江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4456,'320581','常熟市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4457,'320582','张家港市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4458,'320583','昆山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4459,'320585','太仓市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4460,'320600','南通市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4461,'320601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4462,'320602','崇川区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4463,'320611','港闸区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4464,'320612','通州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4465,'320621','海安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4466,'320623','如东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4467,'320681','启东市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4468,'320682','如皋市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4469,'320684','海门市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4470,'320700','连云港市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4471,'320701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4472,'320703','连云区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4473,'320705','新浦区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4474,'320706','海州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4475,'320721','赣榆县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4476,'320722','东海县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4477,'320723','灌云县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4478,'320724','灌南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4479,'320800','淮安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4480,'320801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4481,'320802','清河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4482,'320803','淮安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4483,'320804','淮阴区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4484,'320811','清浦区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4485,'320826','涟水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4486,'320829','洪泽县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4487,'320830','盱眙县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4488,'320831','金湖县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4489,'320900','盐城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4490,'320901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4491,'320902','亭湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4492,'320903','盐都区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4493,'320921','响水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4494,'320922','滨海县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4495,'320923','阜宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4496,'320924','射阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4497,'320925','建湖县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4498,'320981','东台市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4499,'320982','大丰市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4500,'321000','扬州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4501,'321001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4502,'321002','广陵区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4503,'321003','邗江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4504,'321012','江都区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4505,'321023','宝应县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4506,'321081','仪征市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4507,'321084','高邮市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4508,'321100','镇江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4509,'321101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4510,'321102','京口区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4511,'321111','润州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4512,'321112','丹徒区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4513,'321181','丹阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4514,'321182','扬中市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4515,'321183','句容市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4516,'321200','泰州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4517,'321201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4518,'321202','海陵区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4519,'321203','高港区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4520,'321281','兴化市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4521,'321282','靖江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4522,'321283','泰兴市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4523,'321284','姜堰市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4524,'321300','宿迁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4525,'321301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4526,'321302','宿城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4527,'321311','宿豫区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4528,'321322','沭阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4529,'321323','泗阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4530,'321324','泗洪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4531,'330000','浙江省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4532,'330100','杭州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4533,'330101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4534,'330102','上城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4535,'330103','下城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4536,'330104','江干区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4537,'330105','拱墅区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4538,'330106','西湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4539,'330108','滨江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4540,'330109','萧山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4541,'330110','余杭区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4542,'330122','桐庐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4543,'330127','淳安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4544,'330182','建德市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4545,'330183','富阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4546,'330185','临安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4547,'330200','宁波市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4548,'330201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4549,'330203','海曙区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4550,'330204','江东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4551,'330205','江北区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4552,'330206','北仑区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4553,'330211','镇海区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4554,'330212','鄞州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4555,'330225','象山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4556,'330226','宁海县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4557,'330281','余姚市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4558,'330282','慈溪市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4559,'330283','奉化市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4560,'330300','温州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4561,'330301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4562,'330302','鹿城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4563,'330303','龙湾区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4564,'330304','瓯海区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4565,'330322','洞头县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4566,'330324','永嘉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4567,'330326','平阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4568,'330327','苍南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4569,'330328','文成县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4570,'330329','泰顺县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4571,'330381','瑞安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4572,'330382','乐清市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4573,'330400','嘉兴市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4574,'330401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4575,'330402','南湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4576,'330411','秀洲区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4577,'330421','嘉善县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4578,'330424','海盐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4579,'330481','海宁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4580,'330482','平湖市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4581,'330483','桐乡市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4582,'330500','湖州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4583,'330501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4584,'330502','吴兴区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4585,'330503','南浔区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4586,'330521','德清县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4587,'330522','长兴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4588,'330523','安吉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4589,'330600','绍兴市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4590,'330601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4591,'330602','越城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4592,'330621','绍兴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4593,'330624','新昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4594,'330681','诸暨市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4595,'330682','上虞市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4596,'330683','嵊州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4597,'330700','金华市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4598,'330701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4599,'330702','婺城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4600,'330703','金东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4601,'330723','武义县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4602,'330726','浦江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4603,'330727','磐安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4604,'330781','兰溪市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4605,'330782','义乌市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4606,'330783','东阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4607,'330784','永康市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4608,'330800','衢州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4609,'330801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4610,'330802','柯城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4611,'330803','衢江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4612,'330822','常山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4613,'330824','开化县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4614,'330825','龙游县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4615,'330881','江山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4616,'330900','舟山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4617,'330901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4618,'330902','定海区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4619,'330903','普陀区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4620,'330921','岱山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4621,'330922','嵊泗县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4622,'331000','台州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4623,'331001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4624,'331002','椒江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4625,'331003','黄岩区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4626,'331004','路桥区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4627,'331021','玉环县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4628,'331022','三门县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4629,'331023','天台县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4630,'331024','仙居县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4631,'331081','温岭市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4632,'331082','临海市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4633,'331100','丽水市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4634,'331101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4635,'331102','莲都区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4636,'331121','青田县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4637,'331122','缙云县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4638,'331123','遂昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4639,'331124','松阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4640,'331125','云和县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4641,'331126','庆元县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4642,'331127','景宁畲族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4643,'331181','龙泉市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4644,'340000','安徽省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4645,'340100','合肥市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4646,'340101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4647,'340102','瑶海区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4648,'340103','庐阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4649,'340104','蜀山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4650,'340111','包河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4651,'340121','长丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4652,'340122','肥东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4653,'340123','肥西县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4654,'340124','庐江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4655,'340181','巢湖市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4656,'340200','芜湖市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4657,'340201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4658,'340202','镜湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4659,'340203','弋江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4660,'340207','鸠江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4661,'340208','三山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4662,'340221','芜湖县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4663,'340222','繁昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4664,'340223','南陵县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4665,'340225','无为县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4666,'340300','蚌埠市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4667,'340301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4668,'340302','龙子湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4669,'340303','蚌山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4670,'340304','禹会区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4671,'340311','淮上区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4672,'340321','怀远县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4673,'340322','五河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4674,'340323','固镇县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4675,'340400','淮南市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4676,'340401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4677,'340402','大通区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4678,'340403','田家庵区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4679,'340404','谢家集区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4680,'340405','八公山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4681,'340406','潘集区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4682,'340421','凤台县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4683,'340500','马鞍山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4684,'340501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4685,'340503','花山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4686,'340504','雨山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4687,'340506','博望区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4688,'340521','当涂县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4689,'340522','含山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4690,'340523','和县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4691,'340600','淮北市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4692,'340601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4693,'340602','杜集区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4694,'340603','相山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4695,'340604','烈山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4696,'340621','濉溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4697,'340700','铜陵市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4698,'340701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4699,'340702','铜官山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4700,'340703','狮子山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4701,'340711','郊区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4702,'340721','铜陵县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4703,'340800','安庆市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4704,'340801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4705,'340802','迎江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4706,'340803','大观区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4707,'340811','宜秀区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4708,'340822','怀宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4709,'340823','枞阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4710,'340824','潜山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4711,'340825','太湖县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4712,'340826','宿松县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4713,'340827','望江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4714,'340828','岳西县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4715,'340881','桐城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4716,'341000','黄山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4717,'341001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4718,'341002','屯溪区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4719,'341003','黄山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4720,'341004','徽州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4721,'341021','歙县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4722,'341022','休宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4723,'341023','黟县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4724,'341024','祁门县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4725,'341100','滁州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4726,'341101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4727,'341102','琅琊区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4728,'341103','南谯区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4729,'341122','来安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4730,'341124','全椒县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4731,'341125','定远县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4732,'341126','凤阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4733,'341181','天长市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4734,'341182','明光市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4735,'341200','阜阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4736,'341201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4737,'341202','颍州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4738,'341203','颍东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4739,'341204','颍泉区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4740,'341221','临泉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4741,'341222','太和县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4742,'341225','阜南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4743,'341226','颍上县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4744,'341282','界首市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4745,'341300','宿州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4746,'341301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4747,'341302','埇桥区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4748,'341321','砀山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4749,'341322','萧县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4750,'341323','灵璧县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4751,'341324','泗县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4752,'341500','六安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4753,'341501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4754,'341502','金安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4755,'341503','裕安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4756,'341521','寿县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4757,'341522','霍邱县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4758,'341523','舒城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4759,'341524','金寨县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4760,'341525','霍山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4761,'341600','亳州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4762,'341601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4763,'341602','谯城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4764,'341621','涡阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4765,'341622','蒙城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4766,'341623','利辛县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4767,'341700','池州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4768,'341701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4769,'341702','贵池区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4770,'341721','东至县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4771,'341722','石台县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4772,'341723','青阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4773,'341800','宣城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4774,'341801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4775,'341802','宣州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4776,'341821','郎溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4777,'341822','广德县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4778,'341823','泾县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4779,'341824','绩溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4780,'341825','旌德县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4781,'341881','宁国市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4782,'350000','福建省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4783,'350100','福州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4784,'350101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4785,'350102','鼓楼区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4786,'350103','台江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4787,'350104','仓山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4788,'350105','马尾区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4789,'350111','晋安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4790,'350121','闽侯县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4791,'350122','连江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4792,'350123','罗源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4793,'350124','闽清县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4794,'350125','永泰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4795,'350128','平潭县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4796,'350181','福清市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4797,'350182','长乐市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4798,'350200','厦门市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4799,'350201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4800,'350203','思明区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4801,'350205','海沧区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4802,'350206','湖里区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4803,'350211','集美区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4804,'350212','同安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4805,'350213','翔安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4806,'350300','莆田市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4807,'350301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4808,'350302','城厢区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4809,'350303','涵江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4810,'350304','荔城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4811,'350305','秀屿区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4812,'350322','仙游县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4813,'350400','三明市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4814,'350401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4815,'350402','梅列区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4816,'350403','三元区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4817,'350421','明溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4818,'350423','清流县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4819,'350424','宁化县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4820,'350425','大田县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4821,'350426','尤溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4822,'350427','沙县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4823,'350428','将乐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4824,'350429','泰宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4825,'350430','建宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4826,'350481','永安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4827,'350500','泉州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4828,'350501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4829,'350502','鲤城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4830,'350503','丰泽区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4831,'350504','洛江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4832,'350505','泉港区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4833,'350521','惠安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4834,'350524','安溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4835,'350525','永春县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4836,'350526','德化县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4837,'350527','金门县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4838,'350581','石狮市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4839,'350582','晋江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4840,'350583','南安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4841,'350600','漳州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4842,'350601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4843,'350602','芗城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4844,'350603','龙文区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4845,'350622','云霄县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4846,'350623','漳浦县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4847,'350624','诏安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4848,'350625','长泰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4849,'350626','东山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4850,'350627','南靖县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4851,'350628','平和县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4852,'350629','华安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4853,'350681','龙海市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4854,'350700','南平市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4855,'350701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4856,'350702','延平区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4857,'350721','顺昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4858,'350722','浦城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4859,'350723','光泽县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4860,'350724','松溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4861,'350725','政和县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4862,'350781','邵武市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4863,'350782','武夷山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4864,'350783','建瓯市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4865,'350784','建阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4866,'350800','龙岩市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4867,'350801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4868,'350802','新罗区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4869,'350821','长汀县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4870,'350822','永定县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4871,'350823','上杭县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4872,'350824','武平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4873,'350825','连城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4874,'350881','漳平市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4875,'350900','宁德市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4876,'350901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4877,'350902','蕉城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4878,'350921','霞浦县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4879,'350922','古田县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4880,'350923','屏南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4881,'350924','寿宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4882,'350925','周宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4883,'350926','柘荣县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4884,'350981','福安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4885,'350982','福鼎市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4886,'360000','江西省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4887,'360100','南昌市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4888,'360101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4889,'360102','东湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4890,'360103','西湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4891,'360104','青云谱区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4892,'360105','湾里区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4893,'360111','青山湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4894,'360121','南昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4895,'360122','新建县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4896,'360123','安义县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4897,'360124','进贤县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4898,'360200','景德镇市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4899,'360201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4900,'360202','昌江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4901,'360203','珠山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4902,'360222','浮梁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4903,'360281','乐平市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4904,'360300','萍乡市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4905,'360301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4906,'360302','安源区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4907,'360313','湘东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4908,'360321','莲花县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4909,'360322','上栗县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4910,'360323','芦溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4911,'360400','九江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4912,'360401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4913,'360402','庐山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4914,'360403','浔阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4915,'360421','九江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4916,'360423','武宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4917,'360424','修水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4918,'360425','永修县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4919,'360426','德安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4920,'360427','星子县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4921,'360428','都昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4922,'360429','湖口县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4923,'360430','彭泽县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4924,'360481','瑞昌市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4925,'360482','共青城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4926,'360500','新余市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4927,'360501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4928,'360502','渝水区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4929,'360521','分宜县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4930,'360600','鹰潭市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4931,'360601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4932,'360602','月湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4933,'360622','余江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4934,'360681','贵溪市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4935,'360700','赣州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4936,'360701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4937,'360702','章贡区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4938,'360721','赣县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4939,'360722','信丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4940,'360723','大余县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4941,'360724','上犹县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4942,'360725','崇义县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4943,'360726','安远县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4944,'360727','龙南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4945,'360728','定南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4946,'360729','全南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4947,'360730','宁都县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4948,'360731','于都县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4949,'360732','兴国县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4950,'360733','会昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4951,'360734','寻乌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4952,'360735','石城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4953,'360781','瑞金市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4954,'360782','南康市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4955,'360800','吉安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4956,'360801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4957,'360802','吉州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4958,'360803','青原区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4959,'360821','吉安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4960,'360822','吉水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4961,'360823','峡江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4962,'360824','新干县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4963,'360825','永丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4964,'360826','泰和县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4965,'360827','遂川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4966,'360828','万安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4967,'360829','安福县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4968,'360830','永新县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4969,'360881','井冈山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4970,'360900','宜春市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4971,'360901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4972,'360902','袁州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4973,'360921','奉新县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4974,'360922','万载县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4975,'360923','上高县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4976,'360924','宜丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4977,'360925','靖安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4978,'360926','铜鼓县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4979,'360981','丰城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4980,'360982','樟树市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4981,'360983','高安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4982,'361000','抚州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4983,'361001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4984,'361002','临川区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4985,'361021','南城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4986,'361022','黎川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4987,'361023','南丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4988,'361024','崇仁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4989,'361025','乐安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4990,'361026','宜黄县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4991,'361027','金溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4992,'361028','资溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4993,'361029','东乡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4994,'361030','广昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4995,'361100','上饶市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4996,'361101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4997,'361102','信州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4998,'361121','上饶县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (4999,'361122','广丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5000,'361123','玉山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5001,'361124','铅山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5002,'361125','横峰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5003,'361126','弋阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5004,'361127','余干县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5005,'361128','鄱阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5006,'361129','万年县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5007,'361130','婺源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5008,'361181','德兴市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5009,'370000','山东省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5010,'370100','济南市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5011,'370101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5012,'370102','历下区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5013,'370103','市中区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5014,'370104','槐荫区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5015,'370105','天桥区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5016,'370112','历城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5017,'370113','长清区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5018,'370124','平阴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5019,'370125','济阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5020,'370126','商河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5021,'370181','章丘市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5022,'370200','青岛市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5023,'370201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5024,'370202','市南区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5025,'370203','市北区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5026,'370205','四方区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5027,'370211','黄岛区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5028,'370212','崂山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5029,'370213','李沧区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5030,'370214','城阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5031,'370281','胶州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5032,'370282','即墨市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5033,'370283','平度市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5034,'370284','胶南市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5035,'370285','莱西市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5036,'370300','淄博市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5037,'370301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5038,'370302','淄川区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5039,'370303','张店区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5040,'370304','博山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5041,'370305','临淄区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5042,'370306','周村区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5043,'370321','桓台县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5044,'370322','高青县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5045,'370323','沂源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5046,'370400','枣庄市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5047,'370401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5048,'370402','市中区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5049,'370403','薛城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5050,'370404','峄城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5051,'370405','台儿庄区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5052,'370406','山亭区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5053,'370481','滕州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5054,'370500','东营市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5055,'370501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5056,'370502','东营区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5057,'370503','河口区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5058,'370521','垦利县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5059,'370522','利津县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5060,'370523','广饶县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5061,'370600','烟台市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5062,'370601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5063,'370602','芝罘区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5064,'370611','福山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5065,'370612','牟平区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5066,'370613','莱山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5067,'370634','长岛县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5068,'370681','龙口市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5069,'370682','莱阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5070,'370683','莱州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5071,'370684','蓬莱市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5072,'370685','招远市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5073,'370686','栖霞市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5074,'370687','海阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5075,'370700','潍坊市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5076,'370701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5077,'370702','潍城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5078,'370703','寒亭区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5079,'370704','坊子区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5080,'370705','奎文区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5081,'370724','临朐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5082,'370725','昌乐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5083,'370781','青州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5084,'370782','诸城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5085,'370783','寿光市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5086,'370784','安丘市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5087,'370785','高密市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5088,'370786','昌邑市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5089,'370800','济宁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5090,'370801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5091,'370802','市中区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5092,'370811','任城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5093,'370826','微山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5094,'370827','鱼台县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5095,'370828','金乡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5096,'370829','嘉祥县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5097,'370830','汶上县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5098,'370831','泗水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5099,'370832','梁山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5100,'370881','曲阜市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5101,'370882','兖州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5102,'370883','邹城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5103,'370900','泰安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5104,'370901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5105,'370902','泰山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5106,'370911','岱岳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5107,'370921','宁阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5108,'370923','东平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5109,'370982','新泰市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5110,'370983','肥城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5111,'371000','威海市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5112,'371001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5113,'371002','环翠区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5114,'371081','文登市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5115,'371082','荣成市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5116,'371083','乳山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5117,'371100','日照市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5118,'371101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5119,'371102','东港区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5120,'371103','岚山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5121,'371121','五莲县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5122,'371122','莒县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5123,'371200','莱芜市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5124,'371201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5125,'371202','莱城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5126,'371203','钢城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5127,'371300','临沂市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5128,'371301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5129,'371302','兰山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5130,'371311','罗庄区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5131,'371312','河东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5132,'371321','沂南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5133,'371322','郯城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5134,'371323','沂水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5135,'371324','苍山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5136,'371325','费县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5137,'371326','平邑县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5138,'371327','莒南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5139,'371328','蒙阴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5140,'371329','临沭县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5141,'371400','德州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5142,'371401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5143,'371402','德城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5144,'371421','陵县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5145,'371422','宁津县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5146,'371423','庆云县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5147,'371424','临邑县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5148,'371425','齐河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5149,'371426','平原县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5150,'371427','夏津县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5151,'371428','武城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5152,'371481','乐陵市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5153,'371482','禹城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5154,'371500','聊城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5155,'371501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5156,'371502','东昌府区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5157,'371521','阳谷县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5158,'371522','莘县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5159,'371523','茌平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5160,'371524','东阿县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5161,'371525','冠县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5162,'371526','高唐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5163,'371581','临清市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5164,'371600','滨州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5165,'371601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5166,'371602','滨城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5167,'371621','惠民县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5168,'371622','阳信县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5169,'371623','无棣县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5170,'371624','沾化县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5171,'371625','博兴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5172,'371626','邹平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5173,'371700','菏泽市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5174,'371701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5175,'371702','牡丹区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5176,'371721','曹县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5177,'371722','单县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5178,'371723','成武县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5179,'371724','巨野县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5180,'371725','郓城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5181,'371726','鄄城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5182,'371727','定陶县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5183,'371728','东明县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5184,'410000','河南省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5185,'410100','郑州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5186,'410101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5187,'410102','中原区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5188,'410103','二七区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5189,'410104','管城回族区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5190,'410105','金水区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5191,'410106','上街区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5192,'410108','惠济区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5193,'410122','中牟县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5194,'410181','巩义市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5195,'410182','荥阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5196,'410183','新密市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5197,'410184','新郑市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5198,'410185','登封市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5199,'410200','开封市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5200,'410201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5201,'410202','龙亭区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5202,'410203','顺河回族区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5203,'410204','鼓楼区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5204,'410205','禹王台区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5205,'410211','金明区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5206,'410221','杞县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5207,'410222','通许县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5208,'410223','尉氏县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5209,'410224','开封县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5210,'410225','兰考县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5211,'410300','洛阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5212,'410301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5213,'410302','老城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5214,'410303','西工区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5215,'410304','瀍河回族区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5216,'410305','涧西区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5217,'410306','吉利区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5218,'410311','洛龙区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5219,'410322','孟津县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5220,'410323','新安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5221,'410324','栾川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5222,'410325','嵩县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5223,'410326','汝阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5224,'410327','宜阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5225,'410328','洛宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5226,'410329','伊川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5227,'410381','偃师市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5228,'410400','平顶山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5229,'410401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5230,'410402','新华区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5231,'410403','卫东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5232,'410404','石龙区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5233,'410411','湛河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5234,'410421','宝丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5235,'410422','叶县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5236,'410423','鲁山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5237,'410425','郏县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5238,'410481','舞钢市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5239,'410482','汝州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5240,'410500','安阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5241,'410501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5242,'410502','文峰区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5243,'410503','北关区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5244,'410505','殷都区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5245,'410506','龙安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5246,'410522','安阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5247,'410523','汤阴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5248,'410526','滑县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5249,'410527','内黄县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5250,'410581','林州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5251,'410600','鹤壁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5252,'410601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5253,'410602','鹤山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5254,'410603','山城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5255,'410611','淇滨区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5256,'410621','浚县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5257,'410622','淇县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5258,'410700','新乡市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5259,'410701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5260,'410702','红旗区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5261,'410703','卫滨区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5262,'410704','凤泉区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5263,'410711','牧野区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5264,'410721','新乡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5265,'410724','获嘉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5266,'410725','原阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5267,'410726','延津县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5268,'410727','封丘县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5269,'410728','长垣县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5270,'410781','卫辉市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5271,'410782','辉县市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5272,'410800','焦作市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5273,'410801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5274,'410802','解放区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5275,'410803','中站区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5276,'410804','马村区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5277,'410811','山阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5278,'410821','修武县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5279,'410822','博爱县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5280,'410823','武陟县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5281,'410825','温县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5282,'410882','沁阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5283,'410883','孟州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5284,'410900','濮阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5285,'410901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5286,'410902','华龙区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5287,'410922','清丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5288,'410923','南乐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5289,'410926','范县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5290,'410927','台前县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5291,'410928','濮阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5292,'411000','许昌市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5293,'411001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5294,'411002','魏都区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5295,'411023','许昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5296,'411024','鄢陵县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5297,'411025','襄城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5298,'411081','禹州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5299,'411082','长葛市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5300,'411100','漯河市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5301,'411101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5302,'411102','源汇区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5303,'411103','郾城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5304,'411104','召陵区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5305,'411121','舞阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5306,'411122','临颍县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5307,'411200','三门峡市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5308,'411201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5309,'411202','湖滨区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5310,'411221','渑池县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5311,'411222','陕县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5312,'411224','卢氏县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5313,'411281','义马市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5314,'411282','灵宝市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5315,'411300','南阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5316,'411301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5317,'411302','宛城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5318,'411303','卧龙区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5319,'411321','南召县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5320,'411322','方城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5321,'411323','西峡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5322,'411324','镇平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5323,'411325','内乡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5324,'411326','淅川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5325,'411327','社旗县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5326,'411328','唐河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5327,'411329','新野县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5328,'411330','桐柏县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5329,'411381','邓州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5330,'411400','商丘市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5331,'411401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5332,'411402','梁园区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5333,'411403','睢阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5334,'411421','民权县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5335,'411422','睢县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5336,'411423','宁陵县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5337,'411424','柘城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5338,'411425','虞城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5339,'411426','夏邑县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5340,'411481','永城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5341,'411500','信阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5342,'411501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5343,'411502','浉河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5344,'411503','平桥区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5345,'411521','罗山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5346,'411522','光山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5347,'411523','新县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5348,'411524','商城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5349,'411525','固始县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5350,'411526','潢川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5351,'411527','淮滨县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5352,'411528','息县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5353,'411600','周口市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5354,'411601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5355,'411602','川汇区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5356,'411621','扶沟县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5357,'411622','西华县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5358,'411623','商水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5359,'411624','沈丘县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5360,'411625','郸城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5361,'411626','淮阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5362,'411627','太康县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5363,'411628','鹿邑县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5364,'411681','项城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5365,'411700','驻马店市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5366,'411701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5367,'411702','驿城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5368,'411721','西平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5369,'411722','上蔡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5370,'411723','平舆县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5371,'411724','正阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5372,'411725','确山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5373,'411726','泌阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5374,'411727','汝南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5375,'411728','遂平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5376,'411729','新蔡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5377,'419000','省直辖县级行政区划',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5378,'419001','济源市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5379,'420000','湖北省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5380,'420100','武汉市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5381,'420101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5382,'420102','江岸区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5383,'420103','江汉区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5384,'420104','硚口区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5385,'420105','汉阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5386,'420106','武昌区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5387,'420107','青山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5388,'420111','洪山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5389,'420112','东西湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5390,'420113','汉南区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5391,'420114','蔡甸区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5392,'420115','江夏区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5393,'420116','黄陂区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5394,'420117','新洲区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5395,'420200','黄石市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5396,'420201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5397,'420202','黄石港区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5398,'420203','西塞山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5399,'420204','下陆区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5400,'420205','铁山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5401,'420222','阳新县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5402,'420281','大冶市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5403,'420300','十堰市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5404,'420301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5405,'420302','茅箭区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5406,'420303','张湾区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5407,'420321','郧县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5408,'420322','郧西县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5409,'420323','竹山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5410,'420324','竹溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5411,'420325','房县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5412,'420381','丹江口市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5413,'420500','宜昌市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5414,'420501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5415,'420502','西陵区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5416,'420503','伍家岗区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5417,'420504','点军区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5418,'420505','猇亭区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5419,'420506','夷陵区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5420,'420525','远安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5421,'420526','兴山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5422,'420527','秭归县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5423,'420528','长阳土家族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5424,'420529','五峰土家族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5425,'420581','宜都市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5426,'420582','当阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5427,'420583','枝江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5428,'420600','襄阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5429,'420601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5430,'420602','襄城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5431,'420606','樊城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5432,'420607','襄州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5433,'420624','南漳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5434,'420625','谷城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5435,'420626','保康县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5436,'420682','老河口市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5437,'420683','枣阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5438,'420684','宜城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5439,'420700','鄂州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5440,'420701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5441,'420702','梁子湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5442,'420703','华容区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5443,'420704','鄂城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5444,'420800','荆门市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5445,'420801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5446,'420802','东宝区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5447,'420804','掇刀区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5448,'420821','京山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5449,'420822','沙洋县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5450,'420881','钟祥市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5451,'420900','孝感市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5452,'420901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5453,'420902','孝南区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5454,'420921','孝昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5455,'420922','大悟县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5456,'420923','云梦县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5457,'420981','应城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5458,'420982','安陆市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5459,'420984','汉川市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5460,'421000','荆州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5461,'421001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5462,'421002','沙市区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5463,'421003','荆州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5464,'421022','公安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5465,'421023','监利县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5466,'421024','江陵县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5467,'421081','石首市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5468,'421083','洪湖市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5469,'421087','松滋市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5470,'421100','黄冈市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5471,'421101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5472,'421102','黄州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5473,'421121','团风县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5474,'421122','红安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5475,'421123','罗田县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5476,'421124','英山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5477,'421125','浠水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5478,'421126','蕲春县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5479,'421127','黄梅县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5480,'421181','麻城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5481,'421182','武穴市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5482,'421200','咸宁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5483,'421201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5484,'421202','咸安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5485,'421221','嘉鱼县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5486,'421222','通城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5487,'421223','崇阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5488,'421224','通山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5489,'421281','赤壁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5490,'421300','随州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5491,'421301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5492,'421303','曾都区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5493,'421321','随县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5494,'421381','广水市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5495,'422800','恩施土家族苗族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5496,'422801','恩施市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5497,'422802','利川市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5498,'422822','建始县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5499,'422823','巴东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5500,'422825','宣恩县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5501,'422826','咸丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5502,'422827','来凤县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5503,'422828','鹤峰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5504,'429000','省直辖县级行政区划',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5505,'429004','仙桃市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5506,'429005','潜江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5507,'429006','天门市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5508,'429021','神农架林区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5509,'430000','湖南省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5510,'430100','长沙市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5511,'430101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5512,'430102','芙蓉区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5513,'430103','天心区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5514,'430104','岳麓区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5515,'430105','开福区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5516,'430111','雨花区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5517,'430112','望城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5518,'430121','长沙县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5519,'430124','宁乡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5520,'430181','浏阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5521,'430200','株洲市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5522,'430201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5523,'430202','荷塘区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5524,'430203','芦淞区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5525,'430204','石峰区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5526,'430211','天元区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5527,'430221','株洲县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5528,'430223','攸县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5529,'430224','茶陵县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5530,'430225','炎陵县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5531,'430281','醴陵市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5532,'430300','湘潭市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5533,'430301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5534,'430302','雨湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5535,'430304','岳塘区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5536,'430321','湘潭县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5537,'430381','湘乡市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5538,'430382','韶山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5539,'430400','衡阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5540,'430401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5541,'430405','珠晖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5542,'430406','雁峰区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5543,'430407','石鼓区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5544,'430408','蒸湘区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5545,'430412','南岳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5546,'430421','衡阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5547,'430422','衡南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5548,'430423','衡山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5549,'430424','衡东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5550,'430426','祁东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5551,'430481','耒阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5552,'430482','常宁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5553,'430500','邵阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5554,'430501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5555,'430502','双清区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5556,'430503','大祥区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5557,'430511','北塔区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5558,'430521','邵东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5559,'430522','新邵县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5560,'430523','邵阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5561,'430524','隆回县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5562,'430525','洞口县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5563,'430527','绥宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5564,'430528','新宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5565,'430529','城步苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5566,'430581','武冈市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5567,'430600','岳阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5568,'430601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5569,'430602','岳阳楼区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5570,'430603','云溪区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5571,'430611','君山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5572,'430621','岳阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5573,'430623','华容县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5574,'430624','湘阴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5575,'430626','平江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5576,'430681','汨罗市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5577,'430682','临湘市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5578,'430700','常德市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5579,'430701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5580,'430702','武陵区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5581,'430703','鼎城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5582,'430721','安乡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5583,'430722','汉寿县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5584,'430723','澧县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5585,'430724','临澧县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5586,'430725','桃源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5587,'430726','石门县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5588,'430781','津市市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5589,'430800','张家界市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5590,'430801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5591,'430802','永定区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5592,'430811','武陵源区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5593,'430821','慈利县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5594,'430822','桑植县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5595,'430900','益阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5596,'430901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5597,'430902','资阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5598,'430903','赫山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5599,'430921','南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5600,'430922','桃江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5601,'430923','安化县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5602,'430981','沅江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5603,'431000','郴州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5604,'431001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5605,'431002','北湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5606,'431003','苏仙区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5607,'431021','桂阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5608,'431022','宜章县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5609,'431023','永兴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5610,'431024','嘉禾县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5611,'431025','临武县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5612,'431026','汝城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5613,'431027','桂东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5614,'431028','安仁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5615,'431081','资兴市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5616,'431100','永州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5617,'431101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5618,'431102','零陵区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5619,'431103','冷水滩区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5620,'431121','祁阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5621,'431122','东安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5622,'431123','双牌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5623,'431124','道县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5624,'431125','江永县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5625,'431126','宁远县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5626,'431127','蓝山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5627,'431128','新田县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5628,'431129','江华瑶族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5629,'431200','怀化市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5630,'431201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5631,'431202','鹤城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5632,'431221','中方县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5633,'431222','沅陵县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5634,'431223','辰溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5635,'431224','溆浦县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5636,'431225','会同县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5637,'431226','麻阳苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5638,'431227','新晃侗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5639,'431228','芷江侗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5640,'431229','靖州苗族侗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5641,'431230','通道侗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5642,'431281','洪江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5643,'431300','娄底市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5644,'431301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5645,'431302','娄星区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5646,'431321','双峰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5647,'431322','新化县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5648,'431381','冷水江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5649,'431382','涟源市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5650,'433100','湘西土家族苗族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5651,'433101','吉首市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5652,'433122','泸溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5653,'433123','凤凰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5654,'433124','花垣县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5655,'433125','保靖县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5656,'433126','古丈县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5657,'433127','永顺县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5658,'433130','龙山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5659,'440000','广东省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5660,'440100','广州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5661,'440101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5662,'440103','荔湾区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5663,'440104','越秀区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5664,'440105','海珠区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5665,'440106','天河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5666,'440111','白云区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5667,'440112','黄埔区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5668,'440113','番禺区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5669,'440114','花都区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5670,'440115','南沙区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5671,'440116','萝岗区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5672,'440183','增城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5673,'440184','从化市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5674,'440200','韶关市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5675,'440201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5676,'440203','武江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5677,'440204','浈江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5678,'440205','曲江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5679,'440222','始兴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5680,'440224','仁化县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5681,'440229','翁源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5682,'440232','乳源瑶族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5683,'440233','新丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5684,'440281','乐昌市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5685,'440282','南雄市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5686,'440300','深圳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5687,'440301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5688,'440303','罗湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5689,'440304','福田区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5690,'440305','南山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5691,'440306','宝安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5692,'440307','龙岗区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5693,'440308','盐田区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5694,'440400','珠海市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5695,'440401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5696,'440402','香洲区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5697,'440403','斗门区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5698,'440404','金湾区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5699,'440500','汕头市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5700,'440501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5701,'440507','龙湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5702,'440511','金平区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5703,'440512','濠江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5704,'440513','潮阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5705,'440514','潮南区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5706,'440515','澄海区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5707,'440523','南澳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5708,'440600','佛山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5709,'440601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5710,'440604','禅城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5711,'440605','南海区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5712,'440606','顺德区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5713,'440607','三水区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5714,'440608','高明区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5715,'440700','江门市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5716,'440701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5717,'440703','蓬江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5718,'440704','江海区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5719,'440705','新会区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5720,'440781','台山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5721,'440783','开平市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5722,'440784','鹤山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5723,'440785','恩平市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5724,'440800','湛江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5725,'440801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5726,'440802','赤坎区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5727,'440803','霞山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5728,'440804','坡头区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5729,'440811','麻章区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5730,'440823','遂溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5731,'440825','徐闻县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5732,'440881','廉江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5733,'440882','雷州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5734,'440883','吴川市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5735,'440900','茂名市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5736,'440901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5737,'440902','茂南区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5738,'440903','茂港区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5739,'440923','电白县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5740,'440981','高州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5741,'440982','化州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5742,'440983','信宜市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5743,'441200','肇庆市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5744,'441201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5745,'441202','端州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5746,'441203','鼎湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5747,'441223','广宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5748,'441224','怀集县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5749,'441225','封开县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5750,'441226','德庆县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5751,'441283','高要市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5752,'441284','四会市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5753,'441300','惠州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5754,'441301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5755,'441302','惠城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5756,'441303','惠阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5757,'441322','博罗县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5758,'441323','惠东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5759,'441324','龙门县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5760,'441400','梅州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5761,'441401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5762,'441402','梅江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5763,'441421','梅县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5764,'441422','大埔县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5765,'441423','丰顺县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5766,'441424','五华县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5767,'441426','平远县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5768,'441427','蕉岭县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5769,'441481','兴宁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5770,'441500','汕尾市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5771,'441501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5772,'441502','城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5773,'441521','海丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5774,'441523','陆河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5775,'441581','陆丰市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5776,'441600','河源市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5777,'441601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5778,'441602','源城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5779,'441621','紫金县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5780,'441622','龙川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5781,'441623','连平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5782,'441624','和平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5783,'441625','东源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5784,'441700','阳江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5785,'441701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5786,'441702','江城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5787,'441721','阳西县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5788,'441723','阳东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5789,'441781','阳春市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5790,'441800','清远市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5791,'441801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5792,'441802','清城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5793,'441821','佛冈县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5794,'441823','阳山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5795,'441825','连山壮族瑶族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5796,'441826','连南瑶族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5797,'441827','清新县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5798,'441881','英德市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5799,'441882','连州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5800,'441900','东莞市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5801,'442000','中山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5802,'445100','潮州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5803,'445101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5804,'445102','湘桥区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5805,'445121','潮安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5806,'445122','饶平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5807,'445200','揭阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5808,'445201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5809,'445202','榕城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5810,'445221','揭东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5811,'445222','揭西县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5812,'445224','惠来县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5813,'445281','普宁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5814,'445300','云浮市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5815,'445301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5816,'445302','云城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5817,'445321','新兴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5818,'445322','郁南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5819,'445323','云安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5820,'445381','罗定市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5821,'450000','广西壮族自治区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5822,'450100','南宁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5823,'450101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5824,'450102','兴宁区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5825,'450103','青秀区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5826,'450105','江南区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5827,'450107','西乡塘区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5828,'450108','良庆区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5829,'450109','邕宁区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5830,'450122','武鸣县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5831,'450123','隆安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5832,'450124','马山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5833,'450125','上林县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5834,'450126','宾阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5835,'450127','横县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5836,'450200','柳州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5837,'450201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5838,'450202','城中区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5839,'450203','鱼峰区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5840,'450204','柳南区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5841,'450205','柳北区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5842,'450221','柳江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5843,'450222','柳城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5844,'450223','鹿寨县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5845,'450224','融安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5846,'450225','融水苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5847,'450226','三江侗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5848,'450300','桂林市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5849,'450301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5850,'450302','秀峰区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5851,'450303','叠彩区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5852,'450304','象山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5853,'450305','七星区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5854,'450311','雁山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5855,'450321','阳朔县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5856,'450322','临桂县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5857,'450323','灵川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5858,'450324','全州县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5859,'450325','兴安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5860,'450326','永福县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5861,'450327','灌阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5862,'450328','龙胜各族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5863,'450329','资源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5864,'450330','平乐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5865,'450331','荔浦县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5866,'450332','恭城瑶族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5867,'450400','梧州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5868,'450401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5869,'450403','万秀区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5870,'450404','蝶山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5871,'450405','长洲区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5872,'450421','苍梧县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5873,'450422','藤县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5874,'450423','蒙山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5875,'450481','岑溪市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5876,'450500','北海市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5877,'450501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5878,'450502','海城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5879,'450503','银海区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5880,'450512','铁山港区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5881,'450521','合浦县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5882,'450600','防城港市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5883,'450601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5884,'450602','港口区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5885,'450603','防城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5886,'450621','上思县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5887,'450681','东兴市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5888,'450700','钦州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5889,'450701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5890,'450702','钦南区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5891,'450703','钦北区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5892,'450721','灵山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5893,'450722','浦北县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5894,'450800','贵港市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5895,'450801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5896,'450802','港北区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5897,'450803','港南区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5898,'450804','覃塘区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5899,'450821','平南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5900,'450881','桂平市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5901,'450900','玉林市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5902,'450901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5903,'450902','玉州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5904,'450921','容县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5905,'450922','陆川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5906,'450923','博白县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5907,'450924','兴业县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5908,'450981','北流市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5909,'451000','百色市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5910,'451001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5911,'451002','右江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5912,'451021','田阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5913,'451022','田东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5914,'451023','平果县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5915,'451024','德保县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5916,'451025','靖西县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5917,'451026','那坡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5918,'451027','凌云县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5919,'451028','乐业县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5920,'451029','田林县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5921,'451030','西林县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5922,'451031','隆林各族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5923,'451100','贺州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5924,'451101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5925,'451102','八步区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5926,'451121','昭平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5927,'451122','钟山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5928,'451123','富川瑶族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5929,'451200','河池市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5930,'451201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5931,'451202','金城江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5932,'451221','南丹县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5933,'451222','天峨县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5934,'451223','凤山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5935,'451224','东兰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5936,'451225','罗城仫佬族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5937,'451226','环江毛南族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5938,'451227','巴马瑶族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5939,'451228','都安瑶族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5940,'451229','大化瑶族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5941,'451281','宜州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5942,'451300','来宾市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5943,'451301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5944,'451302','兴宾区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5945,'451321','忻城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5946,'451322','象州县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5947,'451323','武宣县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5948,'451324','金秀瑶族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5949,'451381','合山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5950,'451400','崇左市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5951,'451401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5952,'451402','江洲区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5953,'451421','扶绥县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5954,'451422','宁明县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5955,'451423','龙州县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5956,'451424','大新县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5957,'451425','天等县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5958,'451481','凭祥市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5959,'460000','海南省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5960,'460100','海口市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5961,'460101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5962,'460105','秀英区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5963,'460106','龙华区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5964,'460107','琼山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5965,'460108','美兰区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5966,'460200','三亚市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5967,'460201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5968,'460300','三沙市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5969,'460321','西沙群岛',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5970,'460322','南沙群岛',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5971,'460323','中沙群岛的岛礁及其海域',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5972,'469000','省直辖县级行政区划',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5973,'469001','五指山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5974,'469002','琼海市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5975,'469003','儋州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5976,'469005','文昌市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5977,'469006','万宁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5978,'469007','东方市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5979,'469021','定安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5980,'469022','屯昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5981,'469023','澄迈县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5982,'469024','临高县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5983,'469025','白沙黎族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5984,'469026','昌江黎族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5985,'469027','乐东黎族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5986,'469028','陵水黎族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5987,'469029','保亭黎族苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5988,'469030','琼中黎族苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5989,'500000','重庆市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5990,'500100','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5991,'500101','万州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5992,'500102','涪陵区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5993,'500103','渝中区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5994,'500104','大渡口区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5995,'500105','江北区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5996,'500106','沙坪坝区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5997,'500107','九龙坡区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5998,'500108','南岸区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (5999,'500109','北碚区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6000,'500110','綦江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6001,'500111','大足区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6002,'500112','渝北区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6003,'500113','巴南区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6004,'500114','黔江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6005,'500115','长寿区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6006,'500116','江津区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6007,'500117','合川区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6008,'500118','永川区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6009,'500119','南川区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6010,'500200','县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6011,'500223','潼南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6012,'500224','铜梁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6013,'500226','荣昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6014,'500227','璧山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6015,'500228','梁平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6016,'500229','城口县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6017,'500230','丰都县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6018,'500231','垫江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6019,'500232','武隆县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6020,'500233','忠县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6021,'500234','开县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6022,'500235','云阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6023,'500236','奉节县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6024,'500237','巫山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6025,'500238','巫溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6026,'500240','石柱土家族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6027,'500241','秀山土家族苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6028,'500242','酉阳土家族苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6029,'500243','彭水苗族土家族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6030,'510000','四川省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6031,'510100','成都市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6032,'510101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6033,'510104','锦江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6034,'510105','青羊区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6035,'510106','金牛区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6036,'510107','武侯区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6037,'510108','成华区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6038,'510112','龙泉驿区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6039,'510113','青白江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6040,'510114','新都区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6041,'510115','温江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6042,'510121','金堂县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6043,'510122','双流县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6044,'510124','郫县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6045,'510129','大邑县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6046,'510131','蒲江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6047,'510132','新津县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6048,'510181','都江堰市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6049,'510182','彭州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6050,'510183','邛崃市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6051,'510184','崇州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6052,'510300','自贡市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6053,'510301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6054,'510302','自流井区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6055,'510303','贡井区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6056,'510304','大安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6057,'510311','沿滩区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6058,'510321','荣县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6059,'510322','富顺县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6060,'510400','攀枝花市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6061,'510401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6062,'510402','东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6063,'510403','西区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6064,'510411','仁和区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6065,'510421','米易县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6066,'510422','盐边县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6067,'510500','泸州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6068,'510501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6069,'510502','江阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6070,'510503','纳溪区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6071,'510504','龙马潭区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6072,'510521','泸县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6073,'510522','合江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6074,'510524','叙永县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6075,'510525','古蔺县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6076,'510600','德阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6077,'510601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6078,'510603','旌阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6079,'510623','中江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6080,'510626','罗江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6081,'510681','广汉市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6082,'510682','什邡市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6083,'510683','绵竹市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6084,'510700','绵阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6085,'510701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6086,'510703','涪城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6087,'510704','游仙区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6088,'510722','三台县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6089,'510723','盐亭县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6090,'510724','安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6091,'510725','梓潼县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6092,'510726','北川羌族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6093,'510727','平武县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6094,'510781','江油市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6095,'510800','广元市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6096,'510801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6097,'510802','利州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6098,'510811','元坝区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6099,'510812','朝天区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6100,'510821','旺苍县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6101,'510822','青川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6102,'510823','剑阁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6103,'510824','苍溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6104,'510900','遂宁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6105,'510901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6106,'510903','船山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6107,'510904','安居区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6108,'510921','蓬溪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6109,'510922','射洪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6110,'510923','大英县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6111,'511000','内江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6112,'511001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6113,'511002','市中区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6114,'511011','东兴区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6115,'511024','威远县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6116,'511025','资中县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6117,'511028','隆昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6118,'511100','乐山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6119,'511101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6120,'511102','市中区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6121,'511111','沙湾区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6122,'511112','五通桥区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6123,'511113','金口河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6124,'511123','犍为县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6125,'511124','井研县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6126,'511126','夹江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6127,'511129','沐川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6128,'511132','峨边彝族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6129,'511133','马边彝族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6130,'511181','峨眉山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6131,'511300','南充市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6132,'511301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6133,'511302','顺庆区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6134,'511303','高坪区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6135,'511304','嘉陵区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6136,'511321','南部县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6137,'511322','营山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6138,'511323','蓬安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6139,'511324','仪陇县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6140,'511325','西充县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6141,'511381','阆中市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6142,'511400','眉山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6143,'511401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6144,'511402','东坡区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6145,'511421','仁寿县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6146,'511422','彭山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6147,'511423','洪雅县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6148,'511424','丹棱县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6149,'511425','青神县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6150,'511500','宜宾市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6151,'511501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6152,'511502','翠屏区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6153,'511503','南溪区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6154,'511521','宜宾县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6155,'511523','江安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6156,'511524','长宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6157,'511525','高县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6158,'511526','珙县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6159,'511527','筠连县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6160,'511528','兴文县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6161,'511529','屏山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6162,'511600','广安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6163,'511601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6164,'511602','广安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6165,'511621','岳池县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6166,'511622','武胜县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6167,'511623','邻水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6168,'511681','华蓥市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6169,'511700','达州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6170,'511701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6171,'511702','通川区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6172,'511721','达县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6173,'511722','宣汉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6174,'511723','开江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6175,'511724','大竹县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6176,'511725','渠县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6177,'511781','万源市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6178,'511800','雅安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6179,'511801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6180,'511802','雨城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6181,'511803','名山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6182,'511822','荥经县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6183,'511823','汉源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6184,'511824','石棉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6185,'511825','天全县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6186,'511826','芦山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6187,'511827','宝兴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6188,'511900','巴中市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6189,'511901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6190,'511902','巴州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6191,'511921','通江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6192,'511922','南江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6193,'511923','平昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6194,'512000','资阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6195,'512001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6196,'512002','雁江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6197,'512021','安岳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6198,'512022','乐至县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6199,'512081','简阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6200,'513200','阿坝藏族羌族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6201,'513221','汶川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6202,'513222','理县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6203,'513223','茂县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6204,'513224','松潘县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6205,'513225','九寨沟县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6206,'513226','金川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6207,'513227','小金县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6208,'513228','黑水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6209,'513229','马尔康县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6210,'513230','壤塘县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6211,'513231','阿坝县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6212,'513232','若尔盖县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6213,'513233','红原县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6214,'513300','甘孜藏族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6215,'513321','康定县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6216,'513322','泸定县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6217,'513323','丹巴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6218,'513324','九龙县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6219,'513325','雅江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6220,'513326','道孚县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6221,'513327','炉霍县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6222,'513328','甘孜县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6223,'513329','新龙县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6224,'513330','德格县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6225,'513331','白玉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6226,'513332','石渠县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6227,'513333','色达县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6228,'513334','理塘县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6229,'513335','巴塘县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6230,'513336','乡城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6231,'513337','稻城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6232,'513338','得荣县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6233,'513400','凉山彝族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6234,'513401','西昌市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6235,'513422','木里藏族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6236,'513423','盐源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6237,'513424','德昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6238,'513425','会理县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6239,'513426','会东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6240,'513427','宁南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6241,'513428','普格县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6242,'513429','布拖县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6243,'513430','金阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6244,'513431','昭觉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6245,'513432','喜德县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6246,'513433','冕宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6247,'513434','越西县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6248,'513435','甘洛县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6249,'513436','美姑县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6250,'513437','雷波县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6251,'520000','贵州省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6252,'520100','贵阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6253,'520101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6254,'520102','南明区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6255,'520103','云岩区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6256,'520111','花溪区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6257,'520112','乌当区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6258,'520113','白云区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6259,'520114','小河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6260,'520121','开阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6261,'520122','息烽县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6262,'520123','修文县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6263,'520181','清镇市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6264,'520200','六盘水市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6265,'520201','钟山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6266,'520203','六枝特区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6267,'520221','水城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6268,'520222','盘县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6269,'520300','遵义市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6270,'520301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6271,'520302','红花岗区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6272,'520303','汇川区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6273,'520321','遵义县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6274,'520322','桐梓县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6275,'520323','绥阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6276,'520324','正安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6277,'520325','道真仡佬族苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6278,'520326','务川仡佬族苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6279,'520327','凤冈县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6280,'520328','湄潭县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6281,'520329','余庆县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6282,'520330','习水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6283,'520381','赤水市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6284,'520382','仁怀市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6285,'520400','安顺市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6286,'520401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6287,'520402','西秀区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6288,'520421','平坝县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6289,'520422','普定县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6290,'520423','镇宁布依族苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6291,'520424','关岭布依族苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6292,'520425','紫云苗族布依族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6293,'520500','毕节市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6294,'520502','七星关区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6295,'520521','大方县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6296,'520522','黔西县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6297,'520523','金沙县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6298,'520524','织金县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6299,'520525','纳雍县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6300,'520526','威宁彝族回族苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6301,'520527','赫章县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6302,'520600','铜仁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6303,'520602','碧江区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6304,'520603','万山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6305,'520621','江口县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6306,'520622','玉屏侗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6307,'520623','石阡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6308,'520624','思南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6309,'520625','印江土家族苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6310,'520626','德江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6311,'520627','沿河土家族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6312,'520628','松桃苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6313,'522300','黔西南布依族苗族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6314,'522301','兴义市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6315,'522322','兴仁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6316,'522323','普安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6317,'522324','晴隆县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6318,'522325','贞丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6319,'522326','望谟县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6320,'522327','册亨县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6321,'522328','安龙县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6322,'522600','黔东南苗族侗族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6323,'522601','凯里市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6324,'522622','黄平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6325,'522623','施秉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6326,'522624','三穗县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6327,'522625','镇远县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6328,'522626','岑巩县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6329,'522627','天柱县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6330,'522628','锦屏县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6331,'522629','剑河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6332,'522630','台江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6333,'522631','黎平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6334,'522632','榕江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6335,'522633','从江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6336,'522634','雷山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6337,'522635','麻江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6338,'522636','丹寨县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6339,'522700','黔南布依族苗族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6340,'522701','都匀市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6341,'522702','福泉市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6342,'522722','荔波县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6343,'522723','贵定县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6344,'522725','瓮安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6345,'522726','独山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6346,'522727','平塘县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6347,'522728','罗甸县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6348,'522729','长顺县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6349,'522730','龙里县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6350,'522731','惠水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6351,'522732','三都水族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6352,'530000','云南省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6353,'530100','昆明市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6354,'530101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6355,'530102','五华区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6356,'530103','盘龙区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6357,'530111','官渡区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6358,'530112','西山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6359,'530113','东川区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6360,'530114','呈贡区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6361,'530122','晋宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6362,'530124','富民县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6363,'530125','宜良县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6364,'530126','石林彝族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6365,'530127','嵩明县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6366,'530128','禄劝彝族苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6367,'530129','寻甸回族彝族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6368,'530181','安宁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6369,'530300','曲靖市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6370,'530301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6371,'530302','麒麟区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6372,'530321','马龙县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6373,'530322','陆良县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6374,'530323','师宗县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6375,'530324','罗平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6376,'530325','富源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6377,'530326','会泽县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6378,'530328','沾益县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6379,'530381','宣威市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6380,'530400','玉溪市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6381,'530402','红塔区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6382,'530421','江川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6383,'530422','澄江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6384,'530423','通海县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6385,'530424','华宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6386,'530425','易门县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6387,'530426','峨山彝族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6388,'530427','新平彝族傣族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6389,'530428','元江哈尼族彝族傣族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6390,'530500','保山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6391,'530501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6392,'530502','隆阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6393,'530521','施甸县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6394,'530522','腾冲县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6395,'530523','龙陵县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6396,'530524','昌宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6397,'530600','昭通市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6398,'530601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6399,'530602','昭阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6400,'530621','鲁甸县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6401,'530622','巧家县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6402,'530623','盐津县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6403,'530624','大关县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6404,'530625','永善县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6405,'530626','绥江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6406,'530627','镇雄县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6407,'530628','彝良县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6408,'530629','威信县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6409,'530630','水富县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6410,'530700','丽江市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6411,'530701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6412,'530702','古城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6413,'530721','玉龙纳西族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6414,'530722','永胜县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6415,'530723','华坪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6416,'530724','宁蒗彝族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6417,'530800','普洱市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6418,'530801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6419,'530802','思茅区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6420,'530821','宁洱哈尼族彝族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6421,'530822','墨江哈尼族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6422,'530823','景东彝族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6423,'530824','景谷傣族彝族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6424,'530825','镇沅彝族哈尼族拉祜族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6425,'530826','江城哈尼族彝族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6426,'530827','孟连傣族拉祜族佤族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6427,'530828','澜沧拉祜族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6428,'530829','西盟佤族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6429,'530900','临沧市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6430,'530901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6431,'530902','临翔区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6432,'530921','凤庆县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6433,'530922','云县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6434,'530923','永德县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6435,'530924','镇康县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6436,'530925','双江拉祜族佤族布朗族傣族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6437,'530926','耿马傣族佤族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6438,'530927','沧源佤族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6439,'532300','楚雄彝族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6440,'532301','楚雄市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6441,'532322','双柏县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6442,'532323','牟定县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6443,'532324','南华县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6444,'532325','姚安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6445,'532326','大姚县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6446,'532327','永仁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6447,'532328','元谋县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6448,'532329','武定县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6449,'532331','禄丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6450,'532500','红河哈尼族彝族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6451,'532501','个旧市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6452,'532502','开远市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6453,'532503','蒙自市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6454,'532523','屏边苗族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6455,'532524','建水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6456,'532525','石屏县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6457,'532526','弥勒县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6458,'532527','泸西县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6459,'532528','元阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6460,'532529','红河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6461,'532530','金平苗族瑶族傣族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6462,'532531','绿春县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6463,'532532','河口瑶族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6464,'532600','文山壮族苗族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6465,'532601','文山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6466,'532622','砚山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6467,'532623','西畴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6468,'532624','麻栗坡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6469,'532625','马关县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6470,'532626','丘北县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6471,'532627','广南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6472,'532628','富宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6473,'532800','西双版纳傣族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6474,'532801','景洪市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6475,'532822','勐海县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6476,'532823','勐腊县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6477,'532900','大理白族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6478,'532901','大理市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6479,'532922','漾濞彝族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6480,'532923','祥云县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6481,'532924','宾川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6482,'532925','弥渡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6483,'532926','南涧彝族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6484,'532927','巍山彝族回族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6485,'532928','永平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6486,'532929','云龙县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6487,'532930','洱源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6488,'532931','剑川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6489,'532932','鹤庆县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6490,'533100','德宏傣族景颇族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6491,'533102','瑞丽市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6492,'533103','芒市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6493,'533122','梁河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6494,'533123','盈江县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6495,'533124','陇川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6496,'533300','怒江傈僳族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6497,'533321','泸水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6498,'533323','福贡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6499,'533324','贡山独龙族怒族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6500,'533325','兰坪白族普米族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6501,'533400','迪庆藏族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6502,'533421','香格里拉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6503,'533422','德钦县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6504,'533423','维西傈僳族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6505,'540000','西藏自治区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6506,'540100','拉萨市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6507,'540101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6508,'540102','城关区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6509,'540121','林周县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6510,'540122','当雄县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6511,'540123','尼木县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6512,'540124','曲水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6513,'540125','堆龙德庆县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6514,'540126','达孜县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6515,'540127','墨竹工卡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6516,'542100','昌都地区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6517,'542121','昌都县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6518,'542122','江达县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6519,'542123','贡觉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6520,'542124','类乌齐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6521,'542125','丁青县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6522,'542126','察雅县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6523,'542127','八宿县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6524,'542128','左贡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6525,'542129','芒康县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6526,'542132','洛隆县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6527,'542133','边坝县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6528,'542200','山南地区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6529,'542221','乃东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6530,'542222','扎囊县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6531,'542223','贡嘎县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6532,'542224','桑日县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6533,'542225','琼结县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6534,'542226','曲松县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6535,'542227','措美县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6536,'542228','洛扎县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6537,'542229','加查县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6538,'542231','隆子县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6539,'542232','错那县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6540,'542233','浪卡子县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6541,'542300','日喀则地区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6542,'542301','日喀则市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6543,'542322','南木林县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6544,'542323','江孜县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6545,'542324','定日县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6546,'542325','萨迦县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6547,'542326','拉孜县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6548,'542327','昂仁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6549,'542328','谢通门县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6550,'542329','白朗县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6551,'542330','仁布县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6552,'542331','康马县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6553,'542332','定结县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6554,'542333','仲巴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6555,'542334','亚东县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6556,'542335','吉隆县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6557,'542336','聂拉木县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6558,'542337','萨嘎县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6559,'542338','岗巴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6560,'542400','那曲地区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6561,'542421','那曲县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6562,'542422','嘉黎县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6563,'542423','比如县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6564,'542424','聂荣县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6565,'542425','安多县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6566,'542426','申扎县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6567,'542427','索县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6568,'542428','班戈县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6569,'542429','巴青县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6570,'542430','尼玛县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6571,'542500','阿里地区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6572,'542521','普兰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6573,'542522','札达县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6574,'542523','噶尔县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6575,'542524','日土县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6576,'542525','革吉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6577,'542526','改则县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6578,'542527','措勤县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6579,'542600','林芝地区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6580,'542621','林芝县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6581,'542622','工布江达县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6582,'542623','米林县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6583,'542624','墨脱县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6584,'542625','波密县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6585,'542626','察隅县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6586,'542627','朗县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6587,'610000','陕西省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6588,'610100','西安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6589,'610101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6590,'610102','新城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6591,'610103','碑林区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6592,'610104','莲湖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6593,'610111','灞桥区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6594,'610112','未央区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6595,'610113','雁塔区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6596,'610114','阎良区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6597,'610115','临潼区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6598,'610116','长安区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6599,'610122','蓝田县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6600,'610124','周至县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6601,'610125','户县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6602,'610126','高陵县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6603,'610200','铜川市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6604,'610201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6605,'610202','王益区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6606,'610203','印台区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6607,'610204','耀州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6608,'610222','宜君县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6609,'610300','宝鸡市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6610,'610301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6611,'610302','渭滨区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6612,'610303','金台区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6613,'610304','陈仓区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6614,'610322','凤翔县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6615,'610323','岐山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6616,'610324','扶风县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6617,'610326','眉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6618,'610327','陇县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6619,'610328','千阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6620,'610329','麟游县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6621,'610330','凤县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6622,'610331','太白县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6623,'610400','咸阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6624,'610401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6625,'610402','秦都区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6626,'610403','杨陵区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6627,'610404','渭城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6628,'610422','三原县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6629,'610423','泾阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6630,'610424','乾县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6631,'610425','礼泉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6632,'610426','永寿县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6633,'610427','彬县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6634,'610428','长武县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6635,'610429','旬邑县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6636,'610430','淳化县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6637,'610431','武功县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6638,'610481','兴平市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6639,'610500','渭南市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6640,'610501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6641,'610502','临渭区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6642,'610521','华县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6643,'610522','潼关县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6644,'610523','大荔县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6645,'610524','合阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6646,'610525','澄城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6647,'610526','蒲城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6648,'610527','白水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6649,'610528','富平县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6650,'610581','韩城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6651,'610582','华阴市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6652,'610600','延安市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6653,'610601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6654,'610602','宝塔区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6655,'610621','延长县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6656,'610622','延川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6657,'610623','子长县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6658,'610624','安塞县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6659,'610625','志丹县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6660,'610626','吴起县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6661,'610627','甘泉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6662,'610628','富县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6663,'610629','洛川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6664,'610630','宜川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6665,'610631','黄龙县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6666,'610632','黄陵县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6667,'610700','汉中市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6668,'610701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6669,'610702','汉台区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6670,'610721','南郑县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6671,'610722','城固县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6672,'610723','洋县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6673,'610724','西乡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6674,'610725','勉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6675,'610726','宁强县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6676,'610727','略阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6677,'610728','镇巴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6678,'610729','留坝县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6679,'610730','佛坪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6680,'610800','榆林市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6681,'610801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6682,'610802','榆阳区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6683,'610821','神木县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6684,'610822','府谷县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6685,'610823','横山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6686,'610824','靖边县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6687,'610825','定边县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6688,'610826','绥德县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6689,'610827','米脂县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6690,'610828','佳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6691,'610829','吴堡县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6692,'610830','清涧县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6693,'610831','子洲县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6694,'610900','安康市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6695,'610901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6696,'610902','汉滨区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6697,'610921','汉阴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6698,'610922','石泉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6699,'610923','宁陕县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6700,'610924','紫阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6701,'610925','岚皋县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6702,'610926','平利县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6703,'610927','镇坪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6704,'610928','旬阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6705,'610929','白河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6706,'611000','商洛市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6707,'611001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6708,'611002','商州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6709,'611021','洛南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6710,'611022','丹凤县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6711,'611023','商南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6712,'611024','山阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6713,'611025','镇安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6714,'611026','柞水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6715,'620000','甘肃省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6716,'620100','兰州市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6717,'620101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6718,'620102','城关区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6719,'620103','七里河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6720,'620104','西固区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6721,'620105','安宁区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6722,'620111','红古区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6723,'620121','永登县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6724,'620122','皋兰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6725,'620123','榆中县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6726,'620200','嘉峪关市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6727,'620201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6728,'620300','金昌市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6729,'620301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6730,'620302','金川区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6731,'620321','永昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6732,'620400','白银市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6733,'620401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6734,'620402','白银区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6735,'620403','平川区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6736,'620421','靖远县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6737,'620422','会宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6738,'620423','景泰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6739,'620500','天水市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6740,'620501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6741,'620502','秦州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6742,'620503','麦积区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6743,'620521','清水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6744,'620522','秦安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6745,'620523','甘谷县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6746,'620524','武山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6747,'620525','张家川回族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6748,'620600','武威市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6749,'620601','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6750,'620602','凉州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6751,'620621','民勤县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6752,'620622','古浪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6753,'620623','天祝藏族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6754,'620700','张掖市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6755,'620701','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6756,'620702','甘州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6757,'620721','肃南裕固族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6758,'620722','民乐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6759,'620723','临泽县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6760,'620724','高台县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6761,'620725','山丹县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6762,'620800','平凉市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6763,'620801','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6764,'620802','崆峒区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6765,'620821','泾川县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6766,'620822','灵台县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6767,'620823','崇信县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6768,'620824','华亭县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6769,'620825','庄浪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6770,'620826','静宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6771,'620900','酒泉市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6772,'620901','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6773,'620902','肃州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6774,'620921','金塔县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6775,'620922','瓜州县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6776,'620923','肃北蒙古族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6777,'620924','阿克塞哈萨克族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6778,'620981','玉门市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6779,'620982','敦煌市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6780,'621000','庆阳市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6781,'621001','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6782,'621002','西峰区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6783,'621021','庆城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6784,'621022','环县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6785,'621023','华池县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6786,'621024','合水县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6787,'621025','正宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6788,'621026','宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6789,'621027','镇原县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6790,'621100','定西市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6791,'621101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6792,'621102','安定区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6793,'621121','通渭县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6794,'621122','陇西县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6795,'621123','渭源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6796,'621124','临洮县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6797,'621125','漳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6798,'621126','岷县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6799,'621200','陇南市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6800,'621201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6801,'621202','武都区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6802,'621221','成县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6803,'621222','文县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6804,'621223','宕昌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6805,'621224','康县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6806,'621225','西和县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6807,'621226','礼县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6808,'621227','徽县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6809,'621228','两当县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6810,'622900','临夏回族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6811,'622901','临夏市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6812,'622921','临夏县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6813,'622922','康乐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6814,'622923','永靖县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6815,'622924','广河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6816,'622925','和政县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6817,'622926','东乡族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6818,'622927','积石山保安族东乡族撒拉族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6819,'623000','甘南藏族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6820,'623001','合作市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6821,'623021','临潭县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6822,'623022','卓尼县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6823,'623023','舟曲县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6824,'623024','迭部县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6825,'623025','玛曲县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6826,'623026','碌曲县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6827,'623027','夏河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6828,'630000','青海省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6829,'630100','西宁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6830,'630101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6831,'630102','城东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6832,'630103','城中区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6833,'630104','城西区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6834,'630105','城北区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6835,'630121','大通回族土族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6836,'630122','湟中县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6837,'630123','湟源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6838,'632100','海东地区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6839,'632121','平安县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6840,'632122','民和回族土族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6841,'632123','乐都县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6842,'632126','互助土族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6843,'632127','化隆回族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6844,'632128','循化撒拉族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6845,'632200','海北藏族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6846,'632221','门源回族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6847,'632222','祁连县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6848,'632223','海晏县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6849,'632224','刚察县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6850,'632300','黄南藏族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6851,'632321','同仁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6852,'632322','尖扎县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6853,'632323','泽库县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6854,'632324','河南蒙古族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6855,'632500','海南藏族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6856,'632521','共和县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6857,'632522','同德县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6858,'632523','贵德县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6859,'632524','兴海县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6860,'632525','贵南县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6861,'632600','果洛藏族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6862,'632621','玛沁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6863,'632622','班玛县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6864,'632623','甘德县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6865,'632624','达日县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6866,'632625','久治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6867,'632626','玛多县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6868,'632700','玉树藏族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6869,'632721','玉树县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6870,'632722','杂多县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6871,'632723','称多县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6872,'632724','治多县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6873,'632725','囊谦县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6874,'632726','曲麻莱县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6875,'632800','海西蒙古族藏族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6876,'632801','格尔木市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6877,'632802','德令哈市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6878,'632821','乌兰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6879,'632822','都兰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6880,'632823','天峻县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6881,'640000','宁夏回族自治区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6882,'640100','银川市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6883,'640101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6884,'640104','兴庆区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6885,'640105','西夏区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6886,'640106','金凤区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6887,'640121','永宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6888,'640122','贺兰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6889,'640181','灵武市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6890,'640200','石嘴山市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6891,'640201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6892,'640202','大武口区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6893,'640205','惠农区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6894,'640221','平罗县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6895,'640300','吴忠市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6896,'640301','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6897,'640302','利通区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6898,'640303','红寺堡区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6899,'640323','盐池县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6900,'640324','同心县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6901,'640381','青铜峡市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6902,'640400','固原市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6903,'640401','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6904,'640402','原州区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6905,'640422','西吉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6906,'640423','隆德县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6907,'640424','泾源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6908,'640425','彭阳县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6909,'640500','中卫市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6910,'640501','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6911,'640502','沙坡头区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6912,'640521','中宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6913,'640522','海原县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6914,'650000','新疆维吾尔自治区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6915,'650100','乌鲁木齐市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6916,'650101','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6917,'650102','天山区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6918,'650103','沙依巴克区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6919,'650104','新市区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6920,'650105','水磨沟区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6921,'650106','头屯河区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6922,'650107','达坂城区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6923,'650109','米东区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6924,'650121','乌鲁木齐县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6925,'650200','克拉玛依市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6926,'650201','市辖区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6927,'650202','独山子区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6928,'650203','克拉玛依区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6929,'650204','白碱滩区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6930,'650205','乌尔禾区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6931,'652100','吐鲁番地区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6932,'652101','吐鲁番市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6933,'652122','鄯善县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6934,'652123','托克逊县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6935,'652200','哈密地区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6936,'652201','哈密市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6937,'652222','巴里坤哈萨克自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6938,'652223','伊吾县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6939,'652300','昌吉回族自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6940,'652301','昌吉市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6941,'652302','阜康市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6942,'652323','呼图壁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6943,'652324','玛纳斯县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6944,'652325','奇台县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6945,'652327','吉木萨尔县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6946,'652328','木垒哈萨克自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6947,'652700','博尔塔拉蒙古自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6948,'652701','博乐市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6949,'652722','精河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6950,'652723','温泉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6951,'652800','巴音郭楞蒙古自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6952,'652801','库尔勒市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6953,'652822','轮台县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6954,'652823','尉犁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6955,'652824','若羌县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6956,'652825','且末县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6957,'652826','焉耆回族自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6958,'652827','和静县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6959,'652828','和硕县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6960,'652829','博湖县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6961,'652900','阿克苏地区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6962,'652901','阿克苏市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6963,'652922','温宿县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6964,'652923','库车县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6965,'652924','沙雅县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6966,'652925','新和县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6967,'652926','拜城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6968,'652927','乌什县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6969,'652928','阿瓦提县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6970,'652929','柯坪县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6971,'653000','克孜勒苏柯尔克孜自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6972,'653001','阿图什市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6973,'653022','阿克陶县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6974,'653023','阿合奇县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6975,'653024','乌恰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6976,'653100','喀什地区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6977,'653101','喀什市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6978,'653121','疏附县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6979,'653122','疏勒县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6980,'653123','英吉沙县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6981,'653124','泽普县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6982,'653125','莎车县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6983,'653126','叶城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6984,'653127','麦盖提县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6985,'653128','岳普湖县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6986,'653129','伽师县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6987,'653130','巴楚县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6988,'653131','塔什库尔干塔吉克自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6989,'653200','和田地区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6990,'653201','和田市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6991,'653221','和田县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6992,'653222','墨玉县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6993,'653223','皮山县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6994,'653224','洛浦县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6995,'653225','策勒县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6996,'653226','于田县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6997,'653227','民丰县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6998,'654000','伊犁哈萨克自治州',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (6999,'654002','伊宁市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7000,'654003','奎屯市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7001,'654021','伊宁县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7002,'654022','察布查尔锡伯自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7003,'654023','霍城县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7004,'654024','巩留县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7005,'654025','新源县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7006,'654026','昭苏县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7007,'654027','特克斯县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7008,'654028','尼勒克县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7009,'654200','塔城地区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7010,'654201','塔城市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7011,'654202','乌苏市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7012,'654221','额敏县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7013,'654223','沙湾县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7014,'654224','托里县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7015,'654225','裕民县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7016,'654226','和布克赛尔蒙古自治县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7017,'654300','阿勒泰地区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7018,'654301','阿勒泰市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7019,'654321','布尔津县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7020,'654322','富蕴县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7021,'654323','福海县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7022,'654324','哈巴河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7023,'654325','青河县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7024,'654326','吉木乃县',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7025,'659000','自治区直辖县级行政区划',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7026,'659001','石河子市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7027,'659002','阿拉尔市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7028,'659003','图木舒克市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7029,'659004','五家渠市',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7030,'710000','台湾省',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7031,'810000','香港特别行政区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);
    strInsertSQL.clear();
    strInsertSQL = "insert into `GBCodes`(`ID`,`Code`,`Name`,`Resved1`,`Resved2`) values (7032,'820000','澳门特别行政区',NULL,NULL);";
    iRet |= pGBCodes_Info_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

    return iRet;
}

/*****************************************************************************
 函 数 名  : DeleteDefaultUserInfo
 功能描述  : 删除默认用户
 输入参数  : int iUserType
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月25日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 生成默认的用户ID */
    if (iUserType == 0) /* 超级用户 */
    {
        snprintf(strWiscomVID, 36, "%s%s%d%s", pGblconf->center_code, pGblconf->trade_code, 300, (char*)"0000000");
    }
    else if (iUserType == 1) /* admin用户 */
    {
        snprintf(strWiscomVID, 36, "%s%s%d%s", pGblconf->center_code, pGblconf->trade_code, 300, (char*)"0001230");
    }

    /* 删除数据库用户ID */
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
 函 数 名  : InsertDefaultUserInfo
 功能描述  : 插入默认的用户信息
 输入参数  : char* userName
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月2日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 生成默认的用户名和密码 */
    strSuperUserName = "WiscomV";
    strSuperUserPassword = "WiscomV";

    strAdminUserName = "admin";
    strAdminUserPassword = "admin";

    /* 生成默认的用户ID */
    if (sstrcmp(userName, (char*)strSuperUserName.c_str()) == 0) /* 超级用户 */
    {
        snprintf(strWiscomVID, 36, "%s%s%d%s", pGblconf->center_code, pGblconf->trade_code, 300, (char*)"0000000");
    }
    else if (sstrcmp(userName, (char*)strAdminUserName.c_str()) == 0) /* admin用户 */
    {
        snprintf(strWiscomVID, 36, "%s%s%d%s", pGblconf->center_code, pGblconf->trade_code, 300, (char*)"0001230");
    }

    /* 从数据库获取用户ID */
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
        /* 如果是超级用户的话，系统自动插入数据 */
        if (sstrcmp(userName, (char*)strSuperUserName.c_str()) == 0)
        {
            /* 用默认生成的ID再查找一遍，防止WiscomV切换后用户名不一致 */
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
                /* 更新用户名和密码 */
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
                /* 将用户信息写入数据库 */
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
            /* 用默认生成的ID再查找一遍，防止WiscomV切换后用户名不一致 */
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
                /* 更新用户名 */
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
                /* 将用户信息写入数据库 */
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
        /* 是否启用 */
        iUserEnable = 0;
        pdboper->GetFieldValue("Enable", iUserEnable);

        /* 用户统一编号id */
        strUserID.clear();
        pdboper->GetFieldValue("UserID", strUserID);

        /* 用户密码 */
        strUserPassword.clear();
        pdboper->GetFieldValue("Password", strUserPassword);

        /* 判断用户ID是否需要更新 */
        if (sstrcmp(userName, (char*)strSuperUserName.c_str()) == 0)
        {
            if (0 != sstrcmp((char*)strUserID.c_str(), strWiscomVID)
                || 0 != sstrcmp((char*)strUserPassword.c_str(), (char*)strSuperUserPassword.c_str())
                || iUserEnable != 1) /* 不匹配，需要更新WiscomV的用户ID */
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
            if (0 != sstrcmp((char*)strUserID.c_str(), strWiscomVID) || iUserEnable != 1) /* 不匹配，需要更新admin的用户ID */
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
 函 数 名  : SetMySQLEventOn
 功能描述  : 设置数据库删除事件
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月27日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 插入默认数据 */
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
 函 数 名  : WebNotifyDBRefreshProc
 功能描述  : web通知刷新数据库处理
 输入参数  : char* pcTabName
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月17日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
        /* 可能有将上海地标定时任务截图上传录像数据库中的变化数据，需要重新加载一下录像策略 */
        iRet = RecordInfo_db_refresh_proc();
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "WebNotifyDBRefreshProc() : DiBiaoUploadPicMapConfig:iRet=%d\r\n", iRet);
    }
    else if (0 == sstrcmp(pcTabName, (char*)"GBLogicDeviceConfig"))
    {
        /* 逻辑设备变化 */
        //iRet = GBLogicDeviceConfig_db_refresh_proc();
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "WebNotifyDBRefreshProc() : GBLogicDeviceConfig:iRet=%d\r\n", iRet);
    }
    else if (0 == sstrcmp(pcTabName, (char*)"GBPhyDeviceConfig"))
    {
        /* 物理设备变化 */
        //iRet = GBDeviceConfig_db_refresh_proc();
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "WebNotifyDBRefreshProc() : GBPhyDeviceConfig:iRet=%d\r\n", iRet);
    }
    else if (0 == sstrcmp(pcTabName, (char*)"RouteNetConfig"))
    {
        /* 上级路由配置变化 */
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
 函 数 名  : WebNotifyDBSyncProc
 功能描述  : Web通知主备情况下的数据库同步操作
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年12月8日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int WebNotifyDBSyncProc()
{
    char strCmd[256] = {0};

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "主备启用标识:dwMSFlag=%d, 主备机标识:st_MSFlag=%d", g_BoardNetConfig.dwMSFlag, g_BoardNetConfig.st_MSFlag);

    if (g_BoardNetConfig.dwMSFlag == 1)  /* 主备启用的情况下 */
    {
        if (g_BoardNetConfig.st_MSFlag == 2) /* 备机，检测主机是否存活 */
        {
            if (cms_run_status) /* 备机运行在主模式,从备机数据库同步到主机 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "备机运行在主模式,从备机数据库同步到主机, 开始---");

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB.sh %s %s", (char*)"10.168.0.102", (char*)"10.168.0.101");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB数据库备份命令:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_mobile.sh %s %s", (char*)"10.168.0.102", (char*)"10.168.0.101");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_MOBILE数据库备份命令:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_TSU.sh %s %s", (char*)"10.168.0.102", (char*)"10.168.0.101");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_TSU数据库备份命令:%s", strCmd);
                system(strCmd);

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行主备数据库备份, 结束---");
            }
            else /* 从主机数据库同步到备机 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "备机运行在备模式,从主机数据库同步到备机, 开始---");

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB数据库备份命令:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_mobile.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_MOBILE数据库备份命令:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_TSU.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_TSU数据库备份命令:%s", strCmd);
                system(strCmd);

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行主备数据库备份, 结束---");
            }
        }
        else if (g_BoardNetConfig.st_MSFlag == 1) /* 主机 */
        {
            if (cms_run_status) /* 主机运行在主模式,从主机数据库同步到备机 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "主机运行在主模式,从主机数据库同步到备机, 开始---");

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB数据库备份命令:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_mobile.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_MOBILE数据库备份命令:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_TSU.sh %s %s", (char*)"10.168.0.101", (char*)"10.168.0.102");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_TSU数据库备份命令:%s", strCmd);
                system(strCmd);

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行主备数据库备份, 结束---");
            }
            else /* 从备机数据库同步到主机 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "主机运行在备模式,从备机数据库同步到主机, 开始---");

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB.sh %s %s", (char*)"10.168.0.102", (char*)"10.168.0.101");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB数据库备份命令:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_mobile.sh %s %s", (char*)"10.168.0.102", (char*)"10.168.0.101");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_MOBILE数据库备份命令:%s", strCmd);
                system(strCmd);

                memset(strCmd, 0, 256);
                snprintf(strCmd, 256, "/app/BakeupDB_TSU.sh %s %s", (char*)"10.168.0.102", (char*)"10.168.0.101");
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "EV9000DB_TSU数据库备份命令:%s", strCmd);
                system(strCmd);

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行主备数据库备份, 结束---");
            }
        }
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "主备没有启用,不需要进行数据库同步操作");
    }

    return 0;
}
