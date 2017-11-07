
/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <stdio.h>
#ifdef WIN32
#include <winsock.h>
#include <sys/types.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "common/gbldef.inc"
#include "common/gblfunc_proc.inc"
#include "common/gblconfig_proc.inc"
#include "common/log_proc.inc"

#include "device/device_info_mgn.inc"
#include "device/device_thread_proc.inc"
#include "device/device_reg_proc.inc"
#include "route/route_srv_proc.inc"

#include "service/alarm_proc.inc"
#include "service/preset_proc.inc"
#include "platformms/CPing.h"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern DBOper g_DBOper;
extern int cms_run_status;  /* 0:没有运行,1:正常运行 */

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/
int subscribe_event_id = 133;
int notify_catalog_sn = 111;

unsigned long long iGBDeviceInfoLockCount = 0;
unsigned long long iGBDeviceInfoUnLockCount = 0;

unsigned long long iGBLogicDeviceInfoLockCount = 0;
unsigned long long iGBLogicDeviceInfoUnLockCount = 0;

unsigned long long iZRVDeviceInfoLockCount = 0;
unsigned long long iZRVDeviceInfoUnLockCount = 0;

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
GBDevice_Info_MAP g_GBDeviceInfoMap;              /* 标准物理设备信息队列 */
#ifdef MULTI_THR
osip_mutex_t* g_GBDeviceInfoMapLock = NULL;
#endif

GBLogicDevice_Info_MAP g_GBLogicDeviceInfoMap;    /* 标准逻辑设备信息队列 */
#ifdef MULTI_THR
osip_mutex_t* g_GBLogicDeviceInfoMapLock = NULL;
#endif

ZRVDevice_Info_MAP g_ZRVDeviceInfoMap;              /* ZRV设备信息队列 */
#ifdef MULTI_THR
osip_mutex_t* g_ZRVDeviceInfoMapLock = NULL;
#endif

int db_GBLogicDeviceInfo_reload_mark = 0; /* 逻辑设备数据库更新标识:0:不需要更新，1:需要更新数据库 */
int db_GBDeviceInfo_reload_mark = 0;      /* 标准物理设备数据库更新标识:0:不需要更新，1:需要更新数据库 */

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#if DECS("逻辑设备分组")
/*****************************************************************************
 函 数 名  : LogicDeviceGroup_init
 功能描述  : 逻辑设备分组信息结构初始化
 输入参数  : LogicDeviceGroup_t** logic_device_group
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int LogicDeviceGroup_init(LogicDeviceGroup_t** logic_device_group)
{
    *logic_device_group = (LogicDeviceGroup_t*)smalloc(sizeof(LogicDeviceGroup_t));

    if (*logic_device_group == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "LogicDeviceGroup_init() exit---: *LogicDeviceGroup_t Smalloc Error \r\n");
        return -1;
    }

    (*logic_device_group)->GroupID[0] = '\0';
    (*logic_device_group)->CMSID[0] = '\0';
    (*logic_device_group)->Name[0] = '\0';
    (*logic_device_group)->SortID = -1;
    (*logic_device_group)->ParentID[0] = '\0';
    (*logic_device_group)->iChangeFlag = 0;

    return 0;
}

/*****************************************************************************
 函 数 名  : LogicDeviceGroup_free
 功能描述  : 逻辑设备分组信息结构释放
 输入参数  : LogicDeviceGroup_t* logic_device_group
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void LogicDeviceGroup_free(LogicDeviceGroup_t* logic_device_group)
{
    if (logic_device_group == NULL)
    {
        return;
    }

    memset(logic_device_group->GroupID, 0, 32 + 4);
    memset(logic_device_group->CMSID, 0, MAX_ID_LEN + 4);
    memset(logic_device_group->Name, 0, 64 + 4);
    logic_device_group->SortID = -1;
    memset(logic_device_group->ParentID, 0, 32 + 4);
    logic_device_group->iChangeFlag = 0;

    osip_free(logic_device_group);
    logic_device_group = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : AddLogicDeviceGroup
 功能描述  : 添加逻辑设备分组信息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             char* group_id
             char* cms_id
             char* group_name
             int sort_id
             char* parent_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddLogicDeviceGroup(GBDevice_info_t* pGBDeviceInfo, char* group_id, char* cms_id, char* group_name, int sort_id, char* parent_id)
{
    int i = 0;
    LogicDeviceGroup_t* pLogicDeviceGroup = NULL;

    if (pGBDeviceInfo == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "AddLogicDeviceGroup() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == group_id || NULL == cms_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "AddLogicDeviceGroup() exit---: Param 2 Error \r\n");
        return -1;
    }

    i = LogicDeviceGroup_init(&pLogicDeviceGroup);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddLogicDeviceGroup() exit---: Logic Device Group Init Error \r\n");
        return -1;
    }

    memset(pLogicDeviceGroup->GroupID, 0, 32 + 4);
    osip_strncpy(pLogicDeviceGroup->GroupID, group_id, 32);

    memset(pLogicDeviceGroup->CMSID, 0, MAX_ID_LEN + 4);
    osip_strncpy(pLogicDeviceGroup->CMSID, cms_id, MAX_ID_LEN);

    memset(pLogicDeviceGroup->Name, 0, 64 + 4);
    osip_strncpy(pLogicDeviceGroup->Name, group_name, 64);

    pLogicDeviceGroup->SortID = sort_id;

    memset(pLogicDeviceGroup->ParentID, 0, 32 + 4);
    osip_strncpy(pLogicDeviceGroup->ParentID, parent_id, 32);

    pLogicDeviceGroup->iChangeFlag = 1;

    pGBDeviceInfo->LogicDeviceGroupList[group_id] = pLogicDeviceGroup;

    return 0;
}

/*****************************************************************************
 函 数 名  : ModifyLogicDeviceGroup
 功能描述  : 修改逻辑设备分组信息
 输入参数  : LogicDeviceGroup_t* pLogicDeviceGroup
             char* group_name
             int sort_id
             char* parent_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ModifyLogicDeviceGroup(LogicDeviceGroup_t* pLogicDeviceGroup, char* group_name, int sort_id, char* parent_id)
{
    if (NULL == pLogicDeviceGroup)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "ModifyLogicDeviceGroup() exit---: Param Error \r\n");
        return -1;
    }

    pLogicDeviceGroup->iChangeFlag = 0;

    if (0 != sstrcmp(pLogicDeviceGroup->Name, group_name))
    {
        memset(pLogicDeviceGroup->Name, 0, 64 + 4);
        osip_strncpy(pLogicDeviceGroup->Name, group_name, 64);
        pLogicDeviceGroup->iChangeFlag = 2;
    }

    if (0 != sstrcmp(pLogicDeviceGroup->ParentID, parent_id))
    {
        memset(pLogicDeviceGroup->ParentID, 0, 32 + 4);
        osip_strncpy(pLogicDeviceGroup->ParentID, parent_id, 32);
        pLogicDeviceGroup->iChangeFlag = 2;
    }

    if (pLogicDeviceGroup->SortID != sort_id)
    {
        pLogicDeviceGroup->SortID = sort_id;
        pLogicDeviceGroup->iChangeFlag = 2;
    }

    if (pLogicDeviceGroup->iChangeFlag == 2)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*****************************************************************************
 函 数 名  : GetLogicDeviceGroup
 功能描述  : 获取逻辑设备分组信息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             char* group_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
LogicDeviceGroup_t* GetLogicDeviceGroup(GBDevice_info_t* pGBDeviceInfo, char* group_id)
{
    LogicDeviceGroup_t* pLogicDeviceGroup = NULL;
    LogicDeviceGroup_Iterator Itr;

    if (NULL == pGBDeviceInfo || NULL == group_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GetLogicDeviceGroup() exit---: Param Error \r\n");
        return NULL;
    }

    if (pGBDeviceInfo->LogicDeviceGroupList.size() <= 0)
    {
        return NULL;
    }

    Itr = pGBDeviceInfo->LogicDeviceGroupList.find(group_id);

    if (Itr == pGBDeviceInfo->LogicDeviceGroupList.end())
    {
        return NULL;
    }
    else
    {
        pLogicDeviceGroup = Itr->second;
        return pLogicDeviceGroup;
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : SetLogicDeviceGroupChangeFlag
 功能描述  : 设置逻辑设备分组信息的修改标识
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             int iChangeFlag
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SetLogicDeviceGroupChangeFlag(GBDevice_info_t* pGBDeviceInfo, int iChangeFlag)
{
    LogicDeviceGroup_t* pLogicDeviceGroup = NULL;
    LogicDeviceGroup_Iterator Itr;

    if (pGBDeviceInfo == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SetLogicDeviceGroupChangeFlag() exit---: Param Error \r\n");
        return -1;
    }

    if (pGBDeviceInfo->LogicDeviceGroupList.size() <= 0)
    {
        return 0;
    }

    for (Itr = pGBDeviceInfo->LogicDeviceGroupList.begin(); Itr != pGBDeviceInfo->LogicDeviceGroupList.end(); Itr++)
    {
        pLogicDeviceGroup = Itr->second;

        if (NULL == pLogicDeviceGroup || pLogicDeviceGroup->GroupID[0] == '\0' || pLogicDeviceGroup->CMSID[0] == '\0')
        {
            continue;
        }

        pLogicDeviceGroup->iChangeFlag = iChangeFlag;

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "设置分组信息删除标识:分组ID=%s, 分组名称=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Setting the deleted mark of group identification information:group_ID=%s, group_name=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SynLogicDeviceGroupInfoToDB
 功能描述  : 同步逻辑设备分组信息到数据库
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SynLogicDeviceGroupInfoToDB(GBDevice_info_t* pGBDeviceInfo, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;
    LogicDeviceGroup_t* pLogicDeviceGroup = NULL;
    LogicDeviceGroup_Iterator Itr;

    if (pGBDeviceInfo == NULL || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SynLogicDeviceGroupInfoToDB() exit---: Param Error \r\n");
        return -1;
    }

    if (pGBDeviceInfo->LogicDeviceGroupList.size() <= 0)
    {
        return 0;
    }

    for (Itr = pGBDeviceInfo->LogicDeviceGroupList.begin(); Itr != pGBDeviceInfo->LogicDeviceGroupList.end(); Itr++)
    {
        pLogicDeviceGroup = Itr->second;

        if (NULL == pLogicDeviceGroup || pLogicDeviceGroup->GroupID[0] == '\0' || pLogicDeviceGroup->CMSID[0] == '\0')
        {
            continue;
        }

        if (0 == pLogicDeviceGroup->iChangeFlag || 1 == pLogicDeviceGroup->iChangeFlag) /* 增加 */
        {
            iRet = InsertLogicDeviceGroupConfig(pLogicDeviceGroup, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceGroupInfoToDB() InsertLogicDeviceGroupConfig Error:GroupID=%s, CMSID=%s, iRet=%d \r\n", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->CMSID, iRet);
            }

            if (0 == pLogicDeviceGroup->iChangeFlag) /* 没有变化 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上报的分组信息没有变化:分组ID=%s, 分组名称=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Reported no change in the information packet:group_ID=%s, group_name=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            }
            else if (1 == pLogicDeviceGroup->iChangeFlag) /* 增加 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "增加分组信息到数据库:分组ID=%s, 分组名称=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Increasing the packet information to the database:group_ID=%s, group_name=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            }

            pLogicDeviceGroup->iChangeFlag = 0;
        }
        else if (2 == pLogicDeviceGroup->iChangeFlag) /* 修改 */
        {
            iRet = UpdateLogicDeviceGroupConfig(pLogicDeviceGroup, pDevice_Srv_dboper);
            pLogicDeviceGroup->iChangeFlag = 0;

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceGroupInfoToDB() UpdateLogicDeviceGroupConfig Error:GroupID=%s, CMSID=%s, iRet=%d \r\n", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->CMSID, iRet);
            }
            else if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "修改分组信息到数据库:分组ID=%s, 分组名称=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Modify group information to the database:GroupID=%s, GroupName=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            }
        }
        else if (3 == pLogicDeviceGroup->iChangeFlag) /* 删除 */
        {
            iRet = DeleteLogicDeviceGroupConfig(pLogicDeviceGroup, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceGroupInfoToDB() DeleteLogicDeviceGroupConfig Error:GroupID=%s, CMSID=%s, iRet=%d \r\n", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->CMSID, iRet);
            }
            else if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "从数据库删除分组信息:分组ID=%s, 分组名称=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Remove the group of information from the database:GropuID=%s, GroupName=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SynLogicDeviceGroupInfoToDB2
 功能描述  : 同步单个逻辑设备分组信息到数据库
 输入参数  : LogicDeviceGroup_t* pLogicDeviceGroup
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SynLogicDeviceGroupInfoToDB2(LogicDeviceGroup_t* pLogicDeviceGroup, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;

    if (pLogicDeviceGroup == NULL || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SynLogicDeviceGroupInfoToDB2() exit---: Param Error \r\n");
        return -1;
    }

    if (0 == pLogicDeviceGroup->iChangeFlag || 1 == pLogicDeviceGroup->iChangeFlag) /* 增加 */
    {
        iRet = InsertLogicDeviceGroupConfig(pLogicDeviceGroup, pDevice_Srv_dboper);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceGroupInfoToDB2() InsertLogicDeviceGroupConfig Error:GroupID=%s, CMSID=%s, iRet=%d \r\n", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->CMSID, iRet);
        }

        if (0 == pLogicDeviceGroup->iChangeFlag) /* 没有变化 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上报的分组信息没有变化:分组ID=%s, 分组名称=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Reported no change in the information packet:group_ID=%s, group_name=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
        }
        else if (1 == pLogicDeviceGroup->iChangeFlag) /* 增加 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "增加分组信息到数据库:分组ID=%s, 分组名称=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Increasing the packet information to the database:group_ID=%s, group_name=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
        }

        pLogicDeviceGroup->iChangeFlag = 0;
    }
    else if (2 == pLogicDeviceGroup->iChangeFlag) /* 修改 */
    {
        iRet = UpdateLogicDeviceGroupConfig(pLogicDeviceGroup, pDevice_Srv_dboper);
        pLogicDeviceGroup->iChangeFlag = 0;

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceGroupInfoToDB2() UpdateLogicDeviceGroupConfig Error:GroupID=%s, CMSID=%s, iRet=%d \r\n", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->CMSID, iRet);
        }
        else if (iRet > 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "修改分组信息到数据库:分组ID=%s, 分组名称=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Modify group information to the database:GroupID=%s, GroupName=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
        }
    }
    else if (3 == pLogicDeviceGroup->iChangeFlag) /* 删除 */
    {
        iRet = DeleteLogicDeviceGroupConfig(pLogicDeviceGroup, pDevice_Srv_dboper);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceGroupInfoToDB2() DeleteLogicDeviceGroupConfig Error:GroupID=%s, CMSID=%s, iRet=%d \r\n", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->CMSID, iRet);
        }
        else if (iRet > 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "从数据库删除分组信息:分组ID=%s, 分组名称=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Remove the group of information from the database:GropuID=%s, GroupName=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : DelLogicDeviceGroupInfo
 功能描述  : 删除多余的逻辑设备分组信息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DelLogicDeviceGroupInfo(GBDevice_info_t* pGBDeviceInfo)
{
    int index = 0;
    LogicDeviceGroup_t* pLogicDeviceGroup = NULL;
    LogicDeviceGroup_Iterator Itr;
    vector<string> GroupIDVector;

    if (pGBDeviceInfo == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "DelLogicDeviceGroupInfo() exit---: Param Error \r\n");
        return -1;
    }

    GroupIDVector.clear();

    if (pGBDeviceInfo->LogicDeviceGroupList.size() <= 0)
    {
        return 0;
    }

    for (Itr = pGBDeviceInfo->LogicDeviceGroupList.begin(); Itr != pGBDeviceInfo->LogicDeviceGroupList.end(); Itr++)
    {
        pLogicDeviceGroup = Itr->second;

        if (NULL == pLogicDeviceGroup || pLogicDeviceGroup->GroupID[0] == '\0' || pLogicDeviceGroup->CMSID[0] == '\0')
        {
            continue;
        }

        if (3 == pLogicDeviceGroup->iChangeFlag) /* 删除 */
        {
            GroupIDVector.push_back(pLogicDeviceGroup->GroupID);
        }
    }

    if (GroupIDVector.size() > 0)
    {
        for (index = 0; index < (int)GroupIDVector.size(); index++)
        {
            pLogicDeviceGroup = GetLogicDeviceGroup(pGBDeviceInfo, (char*)GroupIDVector[index].c_str());

            if (NULL != pLogicDeviceGroup)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "从内存中删除分组信息:分组ID=%s, 分组名称=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Remove the grouppacket of information from the database:GroupID=%s, GroupName=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DelLogicDeviceGroupInfo() osip_list_remove:GroupID=%s, CMSID=%s \r\n", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->CMSID);

                pGBDeviceInfo->LogicDeviceGroupList.erase(pLogicDeviceGroup->GroupID);
                LogicDeviceGroup_free(pLogicDeviceGroup);
                pLogicDeviceGroup = NULL;
            }
        }
    }

    GroupIDVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : DelLogicDeviceGroupInfo2
 功能描述  : 删除单个多余的逻辑设备分组信息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             LogicDeviceGroup_t* pLogicDeviceGroup
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DelLogicDeviceGroupInfo2(GBDevice_info_t* pGBDeviceInfo, LogicDeviceGroup_t* pLogicDeviceGroup)
{
    if (pGBDeviceInfo == NULL || pLogicDeviceGroup == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "DelLogicDeviceGroupInfo() exit---: Param Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "从内存中删除分组信息:分组ID=%s, 分组名称=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Remove the grouppacket of information from the database:GroupID=%s, GroupName=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DelLogicDeviceGroupInfo() osip_list_remove:GroupID=%s, CMSID=%s \r\n", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->CMSID);

    pGBDeviceInfo->LogicDeviceGroupList.erase(pLogicDeviceGroup->GroupID);
    LogicDeviceGroup_free(pLogicDeviceGroup);
    pLogicDeviceGroup = NULL;

    return 0;
}


/*****************************************************************************
 函 数 名  : InsertLogicDeviceGroupConfig
 功能描述  : 插入数据到逻辑设备分组配置表
 输入参数  : LogicDeviceGroup_t* pLogicDeviceGroup
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月13日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int InsertLogicDeviceGroupConfig(LogicDeviceGroup_t* pLogicDeviceGroup, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;
    int record_count = 0;
    string strQuerySQL = "";
    string strInsertSQL = "";
    char strSortID[32] = {0};

    if (NULL == pLogicDeviceGroup || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "InsertLogicDeviceGroupConfig() exit---: Param Error \r\n");
        return -1;
    }

    snprintf(strSortID, 32, "%d", pLogicDeviceGroup->SortID);

    /* 1、查询SQL语句 */
    strQuerySQL.clear();
    strQuerySQL = "select * from LogicDeviceGroupConfig WHERE GroupID like '";
    strQuerySQL += pLogicDeviceGroup->GroupID;
    strQuerySQL += "'";

    /* 插入SQL语句 */
    strInsertSQL.clear();
    strInsertSQL = "insert into LogicDeviceGroupConfig (GroupID,CMSID,Name,SortID,ParentID) values (";

    /* 组编号 */
    strInsertSQL += "'";
    strInsertSQL += pLogicDeviceGroup->GroupID;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 所属的CMS ID */
    strInsertSQL += "'";
    strInsertSQL += pLogicDeviceGroup->CMSID;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 组名称 */
    strInsertSQL += "'";
    strInsertSQL += pLogicDeviceGroup->Name;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 同一父节点下组排序编号，默认0不排序 */
    strInsertSQL += strSortID;

    strInsertSQL += ",";

    /* 父节点编号 */
    strInsertSQL += "'";
    strInsertSQL += pLogicDeviceGroup->ParentID;
    strInsertSQL += "'";

    strInsertSQL += ")";

    /* 查询数据库 */
    record_count = pDevice_Srv_dboper->DB_Select(strQuerySQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "InsertLogicDeviceGroupConfig() DB Select:strSQL=%s,record_count=%d \r\n", strQuerySQL.c_str(), record_count);

    if (record_count <= 0)
    {
        iRet = pDevice_Srv_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "InsertLogicDeviceGroupConfig() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "InsertLogicDeviceGroupConfig() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        }
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : UpdateLogicDeviceGroupConfig
 功能描述  : 更新数据到逻辑设备分组配置表
 输入参数  : LogicDeviceGroup_t* pLogicDeviceGroup
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int UpdateLogicDeviceGroupConfig(LogicDeviceGroup_t* pLogicDeviceGroup, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;
    string strUpdateSQL = "";
    char strSortID[32] = {0};

    if (NULL == pLogicDeviceGroup || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "UpdateLogicDeviceGroupConfig() exit---: Param Error \r\n");
        return -1;
    }

    snprintf(strSortID, 32, "%d", pLogicDeviceGroup->SortID);

    /* 更新SQL语句 */
    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE LogicDeviceGroupConfig SET";

    strUpdateSQL += " Name = ";
    strUpdateSQL += "'";
    strUpdateSQL += pLogicDeviceGroup->Name;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    strUpdateSQL += " SortID = ";
    strUpdateSQL += strSortID;

    strUpdateSQL += ",";

    strUpdateSQL += " ParentID = ";
    strUpdateSQL += "'";
    strUpdateSQL += pLogicDeviceGroup->ParentID;
    strUpdateSQL += "'";

    strUpdateSQL += " WHERE GroupID like '";
    strUpdateSQL += pLogicDeviceGroup->GroupID;
    strUpdateSQL += "' and CMSID like '";
    strUpdateSQL += pLogicDeviceGroup->CMSID;
    strUpdateSQL += "'";

    iRet = pDevice_Srv_dboper->DB_Update(strUpdateSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateLogicDeviceGroupConfig() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateLogicDeviceGroupConfig() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : DeleteLogicDeviceGroupConfig
 功能描述  : 从数据库中删除逻辑设备分组信息
 输入参数  : LogicDeviceGroup_t* pLogicDeviceGroup
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DeleteLogicDeviceGroupConfig(LogicDeviceGroup_t* pLogicDeviceGroup, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;
    string strDeleteSQL = "";

    if (NULL == pLogicDeviceGroup || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "DeleteLogicDeviceGroupConfig() exit---: Param Error \r\n");
        return -1;
    }

    /* 删除SQL语句 */
    strDeleteSQL.clear();
    strDeleteSQL = "DELETE FROM LogicDeviceGroupConfig WHERE";
    strDeleteSQL += " GroupID like '";
    strDeleteSQL += pLogicDeviceGroup->GroupID;
    strDeleteSQL += "' and CMSID like '";
    strDeleteSQL += pLogicDeviceGroup->CMSID;
    strDeleteSQL += "'";

    iRet = pDevice_Srv_dboper->DB_Delete(strDeleteSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DeleteLogicDeviceGroupConfig() DB Oper Error:strDeleteSQL=%s, iRet=%d \r\n", strDeleteSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DeleteLogicDeviceGroupConfig() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : AddLogicDeviceGroupConfigToDeviceInfo
 功能描述  : 添加逻辑设备分组信息到物理设备中
 输入参数  : GBDevice_info_t* pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddLogicDeviceGroupConfigToDeviceInfo(GBDevice_info_t* pGBDeviceInfo, DBOper* pDevice_Srv_dboper)
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;

    if (NULL == pGBDeviceInfo || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "AddLogicDeviceGroupConfigToDeviceInfo() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from LogicDeviceGroupConfig WHERE CMSID like '";
    strSQL += pGBDeviceInfo->device_id;
    strSQL += "'";

    record_count = pDevice_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddLogicDeviceGroupConfigToDeviceInfo() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddLogicDeviceGroupConfigToDeviceInfo() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "AddLogicDeviceGroupConfigToDeviceInfo() exit---: No Record Count:DeviceID=%s \r\n", pGBDeviceInfo->device_id);
        return 0;
    }

    printf("\r\n AddLogicDeviceGroupConfigToDeviceInfo() DB Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);

    do
    {
        string strGroupID = "";
        string strName = "";
        int iSortID = 0;
        string strParentID = "";

        pDevice_Srv_dboper->GetFieldValue("GroupID", strGroupID);
        pDevice_Srv_dboper->GetFieldValue("Name", strName);
        pDevice_Srv_dboper->GetFieldValue("SortID", iSortID);
        pDevice_Srv_dboper->GetFieldValue("ParentID", strParentID);

        /* 添加到队列 */
        ret = DeviceGroupConfigInfoProc(pGBDeviceInfo, (char*)strGroupID.c_str(), (char*)strName.c_str(), (char*)strParentID.c_str(), iSortID, 1, pDevice_Srv_dboper, 0);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddLogicDeviceGroupConfigToDeviceInfo() DeviceGroupConfigInfoProc:GroupID=%s, Name=%s, SortID=%d, ParentID=%s, i=%d", (char*)strGroupID.c_str(), (char*)strName.c_str(), iSortID, (char*)strParentID.c_str(), ret);
    }
    while (pDevice_Srv_dboper->MoveNext() >= 0);

    return ret;

}
#endif

#if DECS("逻辑设备分组关系")
/*****************************************************************************
 函 数 名  : LogicDeviceMapGroup_init
 功能描述  : 逻辑设备分组关系信息结构初始化
 输入参数  : LogicDeviceMapGroup_t** logic_device_map_group
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int LogicDeviceMapGroup_init(LogicDeviceMapGroup_t** logic_device_map_group)
{
    *logic_device_map_group = (LogicDeviceMapGroup_t*)smalloc(sizeof(LogicDeviceMapGroup_t));

    if (*logic_device_map_group == NULL)
    {
        return -1;
    }

    (*logic_device_map_group)->GroupID[0] = '\0';
    (*logic_device_map_group)->CMSID[0] = '\0';
    (*logic_device_map_group)->DeviceIndex = 0;
    (*logic_device_map_group)->SortID = -1;
    (*logic_device_map_group)->iChangeFlag = 0;

    return 0;
}

/*****************************************************************************
 函 数 名  : LogicDeviceGroup_free
 功能描述  : 逻辑设备分组信息结构释放
 输入参数  : LogicDeviceMapGroup_t* logic_device_map_group
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void LogicDeviceMapGroup_free(LogicDeviceMapGroup_t* logic_device_map_group)
{
    if (logic_device_map_group == NULL)
    {
        return;
    }

    memset(logic_device_map_group->GroupID, 0, 32 + 4);
    memset(logic_device_map_group->CMSID, 0, MAX_ID_LEN + 4);
    logic_device_map_group->DeviceIndex = 0;
    logic_device_map_group->SortID = -1;
    logic_device_map_group->iChangeFlag = 0;

    osip_free(logic_device_map_group);
    logic_device_map_group = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : AddLogicDeviceMapGroup
 功能描述  : 添加逻辑设备分组关系信息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             char* group_id
             unsigned int device_index
             char* cms_id
             int sort_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddLogicDeviceMapGroup(GBDevice_info_t* pGBDeviceInfo, char* group_id, unsigned int device_index, char* cms_id, int sort_id)
{
    int i = 0;
    LogicDeviceMapGroup_t* pLogicDeviceMapGroup = NULL;

    if (pGBDeviceInfo == NULL)
    {
        return -1;
    }

    if (NULL == group_id || NULL == cms_id || device_index <= 0)
    {
        return -1;
    }

    i = LogicDeviceMapGroup_init(&pLogicDeviceMapGroup);

    if (i != 0)
    {
        return -1;
    }

    memset(pLogicDeviceMapGroup->GroupID, 0, 32 + 4);
    osip_strncpy(pLogicDeviceMapGroup->GroupID, group_id, 32);

    memset(pLogicDeviceMapGroup->CMSID, 0, MAX_ID_LEN + 4);
    osip_strncpy(pLogicDeviceMapGroup->CMSID, cms_id, MAX_ID_LEN);

    pLogicDeviceMapGroup->DeviceIndex = device_index;

    pLogicDeviceMapGroup->SortID = sort_id;

    pLogicDeviceMapGroup->iChangeFlag = 1;

    pGBDeviceInfo->LogicDeviceMapGroupList[device_index] = pLogicDeviceMapGroup;

    if (i == -1)
    {
        LogicDeviceMapGroup_free(pLogicDeviceMapGroup);
        pLogicDeviceMapGroup = NULL;
        return -1;
    }

    return i - 1;
}

/*****************************************************************************
 函 数 名  : ModifyLogicDeviceMapGroup
 功能描述  : 修改逻辑设备分组关系
 输入参数  : LogicDeviceMapGroup_t* pLogicDeviceMapGroup
             char* group_id
             char* cms_id
             int sort_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ModifyLogicDeviceMapGroup(LogicDeviceMapGroup_t* pLogicDeviceMapGroup, char* group_id, char* cms_id, int sort_id)
{
    if (NULL == pLogicDeviceMapGroup || NULL == group_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "ModifyLogicDeviceMapGroup() exit---: Param Error \r\n");
        return -1;
    }

    pLogicDeviceMapGroup->iChangeFlag = 0;

    if (0 != sstrcmp(pLogicDeviceMapGroup->GroupID, group_id))
    {
        memset(pLogicDeviceMapGroup->GroupID, 0, 32 + 4);
        osip_strncpy(pLogicDeviceMapGroup->GroupID, group_id, 32);
        pLogicDeviceMapGroup->iChangeFlag = 2;
    }

    if (0 != sstrcmp(pLogicDeviceMapGroup->CMSID, cms_id))
    {
        memset(pLogicDeviceMapGroup->CMSID, 0, MAX_ID_LEN + 4);
        osip_strncpy(pLogicDeviceMapGroup->CMSID, cms_id, MAX_ID_LEN);
        pLogicDeviceMapGroup->iChangeFlag = 2;
    }

    if (pLogicDeviceMapGroup->SortID != sort_id)
    {
        pLogicDeviceMapGroup->SortID = sort_id;
        pLogicDeviceMapGroup->iChangeFlag = 2;
    }

    if (pLogicDeviceMapGroup->iChangeFlag == 2)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*****************************************************************************
 函 数 名  : GetLogicDeviceMapGroup
 功能描述  : 获取逻辑设备分组关系信息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             unsigned int device_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
LogicDeviceMapGroup_t* GetLogicDeviceMapGroup(GBDevice_info_t* pGBDeviceInfo, unsigned int device_index)
{
    LogicDeviceMapGroup_t* pLogicDeviceMapGroup = NULL;
    LogicDeviceMapGroup_Iterator Itr;

    if (pGBDeviceInfo == NULL)
    {
        return NULL;
    }

    if (pGBDeviceInfo->LogicDeviceMapGroupList.size() <= 0)
    {
        return NULL;
    }

    Itr = pGBDeviceInfo->LogicDeviceMapGroupList.find(device_index);

    if (Itr == pGBDeviceInfo->LogicDeviceMapGroupList.end())
    {
        return NULL;
    }
    else
    {
        pLogicDeviceMapGroup = Itr->second;
        return pLogicDeviceMapGroup;
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : SetLogicDeviceMapGroupChangeFlag
 功能描述  : 设置逻辑设备分组关系信息的修改标识
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             int iChangeFlag
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SetLogicDeviceMapGroupChangeFlag(GBDevice_info_t* pGBDeviceInfo, int iChangeFlag)
{
    LogicDeviceMapGroup_t* pLogicDeviceMapGroup = NULL;
    LogicDeviceMapGroup_Iterator Itr;

    if (pGBDeviceInfo == NULL)
    {
        return -1;
    }

    if (pGBDeviceInfo->LogicDeviceMapGroupList.size() <= 0)
    {
        return 0;
    }

    for (Itr = pGBDeviceInfo->LogicDeviceMapGroupList.begin(); Itr != pGBDeviceInfo->LogicDeviceMapGroupList.end(); Itr++)
    {
        pLogicDeviceMapGroup = Itr->second;

        if (NULL == pLogicDeviceMapGroup || pLogicDeviceMapGroup->GroupID[0] == '\0' || pLogicDeviceMapGroup->CMSID[0] == '\0' || pLogicDeviceMapGroup->DeviceIndex <= 0)
        {
            continue;
        }

        pLogicDeviceMapGroup->iChangeFlag = iChangeFlag;

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "设置分组关系信息删除标识:分组ID=%s, 逻辑设备索引=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Setting group relation information deletion mark:GroupID=%s,LogicDeviceIndex=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SynLogicDeviceGroupInfoToDB
 功能描述  : 同步逻辑设备分组信息到数据库
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SynLogicDeviceMapGroupInfoToDB(GBDevice_info_t* pGBDeviceInfo, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;
    LogicDeviceMapGroup_t* pLogicDeviceMapGroup = NULL;
    LogicDeviceMapGroup_Iterator Itr;

    if (pGBDeviceInfo == NULL || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SynLogicDeviceMapGroupInfoToDB() exit---: Param Error \r\n");
        return -1;
    }

    if (pGBDeviceInfo->LogicDeviceMapGroupList.size() <= 0)
    {
        return 0;
    }

    for (Itr = pGBDeviceInfo->LogicDeviceMapGroupList.begin(); Itr != pGBDeviceInfo->LogicDeviceMapGroupList.end(); Itr++)
    {
        pLogicDeviceMapGroup = Itr->second;

        if (NULL == pLogicDeviceMapGroup || pLogicDeviceMapGroup->GroupID[0] == '\0' || pLogicDeviceMapGroup->CMSID[0] == '\0' || pLogicDeviceMapGroup->DeviceIndex <= 0)
        {
            continue;
        }

        if (0 == pLogicDeviceMapGroup->iChangeFlag || 1 == pLogicDeviceMapGroup->iChangeFlag) /* 增加 */
        {
            iRet = InsertLogicDeviceMapGroupConfig(pLogicDeviceMapGroup, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceMapGroupInfoToDB() InsertLogicDeviceMapGroupConfig Error:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "SynLogicDeviceMapGroupInfoToDB() InsertLogicDeviceMapGroupConfig OK:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
            }

            /* 可能原来上级已经将点位分组了，需要删除掉 */
            iRet = DeleteExcessLogicDeviceMapGroupConfig(pLogicDeviceMapGroup, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceMapGroupInfoToDB() DeleteExcessLogicDeviceMapGroupConfig Error:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
            }

            if (0 == pLogicDeviceMapGroup->iChangeFlag) /* 没有变化 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上报的分组关系信息没有变化:分组ID=%s, 逻辑设备索引=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Group information reporting  has not changed:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            }
            else if (1 == pLogicDeviceMapGroup->iChangeFlag) /* 增加 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "增加分组关系信息到数据库:分组ID=%s, 逻辑设备索引=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Increasing the group information to the database:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            }

            pLogicDeviceMapGroup->iChangeFlag = 0;
        }
        else if (2 == pLogicDeviceMapGroup->iChangeFlag) /* 修改 */
        {
            iRet = UpdateLogicDeviceMapGroupConfig(pLogicDeviceMapGroup, pDevice_Srv_dboper);
            pLogicDeviceMapGroup->iChangeFlag = 0;

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceMapGroupInfoToDB() UpdateLogicDeviceMapGroupConfig Error:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
            }
            else if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "修改分组关系信息到数据库:分组ID=%s, 逻辑设备索引=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Modifying group information into database:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            }
        }
        else if (3 == pLogicDeviceMapGroup->iChangeFlag) /* 删除 */
        {
            iRet = DeleteLogicDeviceMapGroupConfig(pLogicDeviceMapGroup, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceMapGroupInfoToDB() DeleteLogicDeviceMapGroupConfig Error:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
            }
            else if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "删除分组关系信息到数据库:分组ID=%s, 逻辑设备索引=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Delete group relationship information from the database:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SynLogicDeviceMapGroupInfoToDB2
 功能描述  : 同步摸个点位的逻辑设备分组信息到数据库
 输入参数  : LogicDeviceMapGroup_t* pLogicDeviceMapGroup
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SynLogicDeviceMapGroupInfoToDB2(LogicDeviceMapGroup_t* pLogicDeviceMapGroup, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;

    if (pLogicDeviceMapGroup == NULL || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SynLogicDeviceMapGroupInfoToDB2() exit---: Param Error \r\n");
        return -1;
    }

    if (0 == pLogicDeviceMapGroup->iChangeFlag || 1 == pLogicDeviceMapGroup->iChangeFlag) /* 增加 */
    {
        iRet = InsertLogicDeviceMapGroupConfig(pLogicDeviceMapGroup, pDevice_Srv_dboper);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceMapGroupInfoToDB() InsertLogicDeviceMapGroupConfig Error:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "SynLogicDeviceMapGroupInfoToDB() InsertLogicDeviceMapGroupConfig OK:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
        }

        /* 可能原来上级已经将点位分组了，需要删除掉 */
        iRet = DeleteExcessLogicDeviceMapGroupConfig(pLogicDeviceMapGroup, pDevice_Srv_dboper);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceMapGroupInfoToDB() DeleteExcessLogicDeviceMapGroupConfig Error:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
        }

        if (0 == pLogicDeviceMapGroup->iChangeFlag) /* 没有变化 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上报的分组关系信息没有变化:分组ID=%s, 逻辑设备索引=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Group information reporting  has not changed:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
        }
        else if (1 == pLogicDeviceMapGroup->iChangeFlag) /* 增加 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "增加分组关系信息到数据库:分组ID=%s, 逻辑设备索引=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Increasing the group information to the database:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
        }

        pLogicDeviceMapGroup->iChangeFlag = 0;
    }
    else if (2 == pLogicDeviceMapGroup->iChangeFlag) /* 修改 */
    {
        iRet = UpdateLogicDeviceMapGroupConfig(pLogicDeviceMapGroup, pDevice_Srv_dboper);
        pLogicDeviceMapGroup->iChangeFlag = 0;

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceMapGroupInfoToDB() UpdateLogicDeviceMapGroupConfig Error:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
        }
        else if (iRet > 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "修改分组关系信息到数据库:分组ID=%s, 逻辑设备索引=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Modifying group information into database:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
        }
    }
    else if (3 == pLogicDeviceMapGroup->iChangeFlag) /* 删除 */
    {
        iRet = DeleteLogicDeviceMapGroupConfig(pLogicDeviceMapGroup, pDevice_Srv_dboper);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceMapGroupInfoToDB() DeleteLogicDeviceMapGroupConfig Error:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
        }
        else if (iRet > 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "删除分组关系信息到数据库:分组ID=%s, 逻辑设备索引=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Delete group relationship information from the database:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
        }
    }

    return 0;
}


/*****************************************************************************
 函 数 名  : DelLogicDeviceGroupInfo
 功能描述  : 删除多余的逻辑设备分组信息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DelLogicDeviceMapGroupInfo(GBDevice_info_t* pGBDeviceInfo)
{
    int index = 0;
    LogicDeviceMapGroup_t* pLogicDeviceMapGroup = NULL;
    LogicDeviceMapGroup_Iterator Itr;
    vector<unsigned int> DeviceIndexVector;

    if (pGBDeviceInfo == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "DelLogicDeviceGroupInfo() exit---: Param Error \r\n");
        return -1;
    }

    DeviceIndexVector.clear();

    if (pGBDeviceInfo->LogicDeviceMapGroupList.size() <= 0)
    {
        return 0;
    }

    for (Itr = pGBDeviceInfo->LogicDeviceMapGroupList.begin(); Itr != pGBDeviceInfo->LogicDeviceMapGroupList.end(); Itr++)
    {
        pLogicDeviceMapGroup = Itr->second;

        if (NULL == pLogicDeviceMapGroup || pLogicDeviceMapGroup->GroupID[0] == '\0' || pLogicDeviceMapGroup->CMSID[0] == '\0' || pLogicDeviceMapGroup->DeviceIndex <= 0)
        {
            continue;
        }

        if (3 == pLogicDeviceMapGroup->iChangeFlag) /* 删除 */
        {
            DeviceIndexVector.push_back(pLogicDeviceMapGroup->DeviceIndex);
        }
    }

    if (DeviceIndexVector.size() > 0)
    {
        for (index = 0; index < (int)DeviceIndexVector.size(); index++)
        {
            pLogicDeviceMapGroup = GetLogicDeviceMapGroup(pGBDeviceInfo, DeviceIndexVector[index]);

            if (NULL != pLogicDeviceMapGroup)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "从内存中删除分组关系信息:分组ID=%s, 逻辑设备索引=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Delete group relationship information from memory:Group_ID=%s, LogicDeviceIndex=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DelLogicDeviceGroupInfo() osip_list_remove:GroupID=%s, CMSID=%s, DeviceIndex=%u \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex);

                pGBDeviceInfo->LogicDeviceMapGroupList.erase(pLogicDeviceMapGroup->DeviceIndex);
                LogicDeviceMapGroup_free(pLogicDeviceMapGroup);
                pLogicDeviceMapGroup = NULL;
            }
        }
    }

    DeviceIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : DelLogicDeviceMapGroupInfo2
 功能描述  : 删除某一个多余的逻辑设备分组信息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             LogicDeviceMapGroup_t* pLogicDeviceMapGroup
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DelLogicDeviceMapGroupInfo2(GBDevice_info_t* pGBDeviceInfo, LogicDeviceMapGroup_t* pLogicDeviceMapGroup)
{
    if (NULL == pGBDeviceInfo || NULL == pLogicDeviceMapGroup)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "DelLogicDeviceGroupInfo() exit---: Param Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "从内存中删除分组关系信息:分组ID=%s, 逻辑设备索引=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Delete group relationship information from memory:Group_ID=%s, LogicDeviceIndex=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DelLogicDeviceGroupInfo() osip_list_remove:GroupID=%s, CMSID=%s, DeviceIndex=%u \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex);

    pGBDeviceInfo->LogicDeviceMapGroupList.erase(pLogicDeviceMapGroup->DeviceIndex);
    LogicDeviceMapGroup_free(pLogicDeviceMapGroup);
    pLogicDeviceMapGroup = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : InsertLogicDeviceMapGroupConfig
 功能描述  : 插入数据到逻辑设备分组关系配置表
 输入参数  : LogicDeviceMapGroup_t* pLogicDeviceMapGroup
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月13日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int InsertLogicDeviceMapGroupConfig(LogicDeviceMapGroup_t* pLogicDeviceMapGroup, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;
    int record_count = 0;
    string strQuerySQL = "";
    string strInsertSQL = "";
    char strDeviceIndex[64] = {0};
    char strSortID[32] = {0};

    if (NULL == pLogicDeviceMapGroup || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "InsertLogicDeviceMapGroupConfig() exit---: Param Error \r\n");
        return -1;
    }

    snprintf(strDeviceIndex, 64, "%u", pLogicDeviceMapGroup->DeviceIndex);
    snprintf(strSortID, 32, "%d", pLogicDeviceMapGroup->SortID);

    /* 1、查询SQL语句 */
    strQuerySQL.clear();
    strQuerySQL = "select * from LogicDeviceMapGroupConfig WHERE GroupID like '";
    strQuerySQL += pLogicDeviceMapGroup->GroupID;
    strQuerySQL += "' and CMSID like '";
    strQuerySQL += pLogicDeviceMapGroup->CMSID;
    strQuerySQL += "'";
    strQuerySQL += " and DeviceIndex = ";
    strQuerySQL += strDeviceIndex;

    /* 插入SQL语句 */
    strInsertSQL.clear();
    strInsertSQL = "insert into LogicDeviceMapGroupConfig (GroupID,DeviceIndex,CMSID,SortID) values (";

    /* 组编号 */
    strInsertSQL += "'";
    strInsertSQL += pLogicDeviceMapGroup->GroupID;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 逻辑设备索引*/
    strInsertSQL += strDeviceIndex;

    strInsertSQL += ",";

    /* 所属的CMS ID */
    strInsertSQL += "'";
    strInsertSQL += pLogicDeviceMapGroup->CMSID;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 同一父节点下组排序编号，默认0不排序 */
    strInsertSQL += strSortID;

    strInsertSQL += ")";

    /* 查询数据库 */
    record_count = pDevice_Srv_dboper->DB_Select(strQuerySQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "InsertLogicDeviceMapGroupConfig() DB Select:strSQL=%s,record_count=%d \r\n", strQuerySQL.c_str(), record_count);

    if (record_count <= 0)
    {
        iRet = pDevice_Srv_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "InsertLogicDeviceMapGroupConfig() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "InsertLogicDeviceMapGroupConfig() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        }
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : UpdateLogicDeviceMapGroupConfig
 功能描述  : 更新数据到逻辑设备分组关系配置表
 输入参数  : LogicDeviceGroup_t* pLogicDeviceGroup
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int UpdateLogicDeviceMapGroupConfig(LogicDeviceMapGroup_t* pLogicDeviceMapGroup, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;
    string strUpdateSQL = "";
    char strSortID[32] = {0};
    char strDeviceIndex[64] = {0};

    if (NULL == pLogicDeviceMapGroup || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "UpdateLogicDeviceMapGroupConfig() exit---: Param Error \r\n");
        return -1;
    }

    snprintf(strSortID, 32, "%d", pLogicDeviceMapGroup->SortID);
    snprintf(strDeviceIndex, 64, "%u", pLogicDeviceMapGroup->DeviceIndex);

    /* 更新SQL语句 */
    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE LogicDeviceMapGroupConfig SET";

    strUpdateSQL += " SortID = ";
    strUpdateSQL += strSortID;

    strUpdateSQL += ", GroupID = '";
    strUpdateSQL += pLogicDeviceMapGroup->GroupID;
    strUpdateSQL += "'";

    strUpdateSQL += ", CMSID = '";
    strUpdateSQL += pLogicDeviceMapGroup->CMSID;
    strUpdateSQL += "'";

    strUpdateSQL += " WHERE DeviceIndex = ";
    strUpdateSQL += strDeviceIndex;

    iRet = pDevice_Srv_dboper->DB_Update(strUpdateSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateLogicDeviceGroupConfig() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateLogicDeviceGroupConfig() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : DeleteLogicDeviceMapGroupConfig
 功能描述  : 从数据库中删除逻辑设备分组关系信息
 输入参数  : LogicDeviceMapGroup_t* pLogicDeviceMapGroup
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DeleteLogicDeviceMapGroupConfig(LogicDeviceMapGroup_t* pLogicDeviceMapGroup, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;
    string strDeleteSQL = "";
    char strDeviceIndex[64] = {0};

    if (NULL == pLogicDeviceMapGroup || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "DeleteLogicDeviceMapGroupConfig() exit---: Param Error \r\n");
        return -1;
    }

    snprintf(strDeviceIndex, 64, "%u", pLogicDeviceMapGroup->DeviceIndex);

    /* 删除SQL语句 */
    strDeleteSQL.clear();
    strDeleteSQL = "DELETE FROM LogicDeviceMapGroupConfig WHERE";
    strDeleteSQL += " DeviceIndex = ";
    strDeleteSQL += strDeviceIndex;

    iRet = pDevice_Srv_dboper->DB_Delete(strDeleteSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DeleteLogicDeviceMapGroupConfig() DB Oper Error:strDeleteSQL=%s, iRet=%d \r\n", strDeleteSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DeleteLogicDeviceMapGroupConfig() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (iRet > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "从数据库中删除逻辑设备分组关系:分组ID=%s, CMS ID=%s, 逻辑设备索引=%s", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, strDeviceIndex);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Deleted logical device group relations from the database :Group ID=%s, CMS ID=%s, Logic Device Index=%s", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, strDeviceIndex);
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : DeleteExcessLogicDeviceMapGroupConfig
 功能描述  : 删除掉上级CMS配置的多余的分组关系
 输入参数  : LogicDeviceMapGroup_t* pLogicDeviceMapGroup
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年7月13日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DeleteExcessLogicDeviceMapGroupConfig(LogicDeviceMapGroup_t* pLogicDeviceMapGroup, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;
    string strDeleteSQL = "";
    char strDeviceIndex[64] = {0};

    if (NULL == pLogicDeviceMapGroup || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "DeleteExcessLogicDeviceMapGroupConfig() exit---: Param Error \r\n");
        return -1;
    }

    snprintf(strDeviceIndex, 64, "%u", pLogicDeviceMapGroup->DeviceIndex);

    /* 删除SQL语句 */
    strDeleteSQL.clear();
    strDeleteSQL = "DELETE FROM LogicDeviceMapGroupConfig WHERE (GroupID <> '";
    strDeleteSQL += pLogicDeviceMapGroup->GroupID;
    strDeleteSQL += "' OR CMSID <> '";
    strDeleteSQL += pLogicDeviceMapGroup->CMSID;
    strDeleteSQL += "') AND DeviceIndex = ";
    strDeleteSQL += strDeviceIndex;

    iRet = pDevice_Srv_dboper->DB_Delete(strDeleteSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DeleteExcessLogicDeviceMapGroupConfig() DB Oper Error:strDeleteSQL=%s, iRet=%d \r\n", strDeleteSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DeleteExcessLogicDeviceMapGroupConfig() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (iRet > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "从数据库中删除掉多余的逻辑设备分组关系:分组ID<>%s或者CMS ID<>%s, 逻辑设备索引=%s", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, strDeviceIndex);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Delete redundant grouping of logical device from the database:Group ID<>%s or CMS ID<>%s, Logic Device Index=%s", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, strDeviceIndex);
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : AddLogicDeviceGroupMapConfigToDeviceInfo
 功能描述  : 添加逻辑设备分组关系信息到物理设备中
 输入参数  : GBDevice_info_t* pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddLogicDeviceGroupMapConfigToDeviceInfo(GBDevice_info_t* pGBDeviceInfo, DBOper* pDevice_Srv_dboper)
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;

    if (NULL == pGBDeviceInfo || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "AddLogicDeviceGroupMapConfigToDeviceInfo() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from LogicDeviceMapGroupConfig WHERE CMSID like '";
    strSQL += pGBDeviceInfo->device_id;
    strSQL += "'";

    record_count = pDevice_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddLogicDeviceGroupMapConfigToDeviceInfo() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddLogicDeviceGroupMapConfigToDeviceInfo() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "AddLogicDeviceGroupMapConfigToDeviceInfo() exit---: No Record Count:DeviceID=%s \r\n", pGBDeviceInfo->device_id);
        return 0;
    }

    printf("\r\n AddLogicDeviceGroupMapConfigToDeviceInfo() DB Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);

    do
    {
        string strGroupID = "";
        unsigned int uDeviceIndex = 0;
        int iSortID = 0;

        pDevice_Srv_dboper->GetFieldValue("GroupID", strGroupID);
        pDevice_Srv_dboper->GetFieldValue("DeviceIndex", uDeviceIndex);
        pDevice_Srv_dboper->GetFieldValue("SortID", iSortID);

        /* 添加到队列 */
        ret = DeviceGroupMapConfigInfoProc(pGBDeviceInfo, (char*)strGroupID.c_str(), uDeviceIndex, iSortID, 1, pDevice_Srv_dboper, 0);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddLogicDeviceGroupMapConfigToDeviceInfo() DeviceGroupMapConfigInfoProc:GroupID=%s, DeviceIndex=%u, SortID=%d, i=%d", (char*)strGroupID.c_str(), uDeviceIndex, iSortID, ret);
    }
    while (pDevice_Srv_dboper->MoveNext() >= 0);

    return ret;

}
#endif

#if DECS("标准物理设备信息队列")
/*****************************************************************************
 函 数 名  : GBDevice_info_init
 功能描述  : 标准物理设备结构初始化
 输入参数  : GBDevice_info_t ** GBDevice_info
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int GBDevice_info_init(GBDevice_info_t** GBDevice_info)
{
    *GBDevice_info = new GBDevice_info_t;

    if (*GBDevice_info == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_info_init() exit---: *GBDevice_info Smalloc Error \r\n");
        return -1;
    }

    (*GBDevice_info)->id = 0;
    (*GBDevice_info)->device_id[0] = '\0';
    (*GBDevice_info)->device_type = 0;
    (*GBDevice_info)->link_type = 0;
    (*GBDevice_info)->access_method = 0;
    (*GBDevice_info)->tcp_sock = -1;
    (*GBDevice_info)->login_ip[0] = '\0';
    (*GBDevice_info)->login_port = 0;
    (*GBDevice_info)->reg_status = 0;
    (*GBDevice_info)->reg_info_index = -1;
    (*GBDevice_info)->auth_count = 0;
    (*GBDevice_info)->last_keep_alive_time = 0;
    (*GBDevice_info)->keep_alive_expires = 0;
    (*GBDevice_info)->keep_alive_expires_count = 0;
    (*GBDevice_info)->manufacturer[0] = '\0';
    (*GBDevice_info)->three_party_flag = 0;
    (*GBDevice_info)->trans_protocol = 0;
    (*GBDevice_info)->strRegServerEthName[0] = '\0';
    (*GBDevice_info)->strRegServerIP[0] = '\0';
    (*GBDevice_info)->iRegServerPort = 5060;

    (*GBDevice_info)->call_id[0] = '\0';

    (*GBDevice_info)->CataLogNumCount = 0;
    (*GBDevice_info)->CataLogSN = 0;
    (*GBDevice_info)->iGetCataLogStatus = 0;
    (*GBDevice_info)->iLastGetCataLogTime = 0;
    (*GBDevice_info)->iGetLogicDeviceStatusCount = 0;
    (*GBDevice_info)->keep_alive_count = 0;

    (*GBDevice_info)->catalog_subscribe_flag = 0;
    (*GBDevice_info)->catalog_subscribe_expires = 0;
    (*GBDevice_info)->catalog_subscribe_expires_count = 0;
    (*GBDevice_info)->catalog_subscribe_event_id = 0;
    (*GBDevice_info)->catalog_subscribe_dialog_index = -1;

    (*GBDevice_info)->LogicDeviceGroupConfigCount = 0;
    (*GBDevice_info)->LogicDeviceGroupSN = 0;
    (*GBDevice_info)->LogicDeviceMapGroupConfigCount = 0;
    (*GBDevice_info)->LogicDeviceMapGroupSN = 0;
    (*GBDevice_info)->del_mark = 0;

    /* 下级上报的逻辑设备分组信息队列初始化 */
    (*GBDevice_info)->LogicDeviceGroupList.clear();

    /* 下级上报的逻辑设备分组关系信息队列初始化 */
    (*GBDevice_info)->LogicDeviceMapGroupList.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : GBDevice_info_free
 功能描述  : 标准物理设备结构释放
 输入参数  : GBDevice_info_t * GBDevice_info
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void GBDevice_info_free(GBDevice_info_t* GBDevice_info)
{
    LogicDeviceGroup_Iterator LogicDeviceGroupItr;
    LogicDeviceMapGroup_Iterator LogicDeviceMapGroupItr;
    LogicDeviceGroup_t* pLogicDeviceGroup = NULL;
    LogicDeviceMapGroup_t* pLogicDeviceMapGroup = NULL;

    if (GBDevice_info == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_info_free() exit---: Param Error \r\n");
        return;
    }

    GBDevice_info->id = 0;
    memset(GBDevice_info->device_id, 0, MAX_ID_LEN + 4);
    GBDevice_info->device_type = 0;
    GBDevice_info->link_type = 0;
    GBDevice_info->access_method = 0;
    GBDevice_info->tcp_sock = -1;
    memset(GBDevice_info->login_ip, 0, MAX_IP_LEN);
    GBDevice_info->login_port = 0;
    GBDevice_info->reg_status = 0;
    GBDevice_info->reg_info_index = -1;
    GBDevice_info->auth_count = 0;
    GBDevice_info->last_keep_alive_time = 0;
    GBDevice_info->keep_alive_expires = 0;
    GBDevice_info->keep_alive_expires_count = 0;
    memset(GBDevice_info->manufacturer, 0, MAX_128CHAR_STRING_LEN + 4);
    GBDevice_info->three_party_flag = 0;
    GBDevice_info->trans_protocol = 0;
    memset(GBDevice_info->strRegServerEthName, 0, MAX_IP_LEN);
    memset(GBDevice_info->strRegServerIP, 0, MAX_IP_LEN);
    GBDevice_info->iRegServerPort = 5060;

    memset(GBDevice_info->call_id, 0, MAX_128CHAR_STRING_LEN + 4);

    GBDevice_info->CataLogNumCount = 0;
    GBDevice_info->CataLogSN = 0;
    GBDevice_info->iGetCataLogStatus = 0;
    GBDevice_info->iLastGetCataLogTime = 0;
    GBDevice_info->iGetLogicDeviceStatusCount = 0;
    GBDevice_info->keep_alive_count = 0;

    GBDevice_info->catalog_subscribe_flag = 0;
    GBDevice_info->catalog_subscribe_expires = 0;
    GBDevice_info->catalog_subscribe_expires_count = 0;
    GBDevice_info->catalog_subscribe_event_id = 0;
    GBDevice_info->catalog_subscribe_dialog_index = -1;

    GBDevice_info->LogicDeviceGroupConfigCount = 0;
    GBDevice_info->LogicDeviceGroupSN = 0;
    GBDevice_info->LogicDeviceMapGroupConfigCount = 0;
    GBDevice_info->LogicDeviceMapGroupSN = 0;
    GBDevice_info->del_mark = 0;

    /* 下级上报的逻辑设备分组信息队列释放 */
    for (LogicDeviceGroupItr = GBDevice_info->LogicDeviceGroupList.begin(); LogicDeviceGroupItr != GBDevice_info->LogicDeviceGroupList.end(); LogicDeviceGroupItr++)
    {
        pLogicDeviceGroup = LogicDeviceGroupItr->second;

        if (NULL != pLogicDeviceGroup)
        {
            LogicDeviceGroup_free(pLogicDeviceGroup);
            pLogicDeviceGroup = NULL;
        }
    }

    GBDevice_info->LogicDeviceGroupList.clear();

    /* 下级上报的逻辑设备分组关系信息队列释放 */
    for (LogicDeviceMapGroupItr = GBDevice_info->LogicDeviceMapGroupList.begin(); LogicDeviceMapGroupItr != GBDevice_info->LogicDeviceMapGroupList.end(); LogicDeviceMapGroupItr++)
    {
        pLogicDeviceMapGroup = LogicDeviceMapGroupItr->second;

        if (NULL != pLogicDeviceMapGroup)
        {
            LogicDeviceMapGroup_free(pLogicDeviceMapGroup);
            pLogicDeviceMapGroup = NULL;
        }
    }

    GBDevice_info->LogicDeviceMapGroupList.clear();

    return;
}

/*****************************************************************************
 函 数 名  : GBDevice_info_list_init
 功能描述  : 标准物理设备信息队列初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int GBDevice_info_list_init()
{
    g_GBDeviceInfoMap.clear();

#ifdef MULTI_THR
    g_GBDeviceInfoMapLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_GBDeviceInfoMapLock)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_info_list_init() exit---: GBDevice Info Map Lock Init Error \r\n");
        return -1;
    }

#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : GBDevice_info_list_free
 功能描述  : 标准物理设备队列释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void GBDevice_info_list_free()
{
    GBDevice_Info_Iterator Itr;
    GBDevice_info_t* GBDevice_info = NULL;

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        GBDevice_info = Itr->second;

        if (NULL != GBDevice_info)
        {
            GBDevice_info_free(GBDevice_info);
            delete GBDevice_info;
            GBDevice_info = NULL;
        }
    }

    g_GBDeviceInfoMap.clear();

#ifdef MULTI_THR

    if (NULL != g_GBDeviceInfoMapLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_GBDeviceInfoMapLock);
        g_GBDeviceInfoMapLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 函 数 名  : GBDevice_info_list_lock
 功能描述  : 标准物理设备队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int GBDevice_info_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GBDeviceInfoMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_GBDeviceInfoMapLock);

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : GBDevice_info_list_unlock
 功能描述  : 标准物理设备队列解锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int GBDevice_info_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GBDeviceInfoMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_GBDeviceInfoMapLock);

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_GBDevice_info_list_lock
 功能描述  : 标准物理设备队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int debug_GBDevice_info_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GBDeviceInfoMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "debug_GBDevice_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_GBDeviceInfoMapLock, file, line, func);

    iGBDeviceInfoLockCount++;

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_GBDevice_info_list_lock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iGBDeviceInfoLockCount != iGBDeviceInfoUnLockCount + 1)
        {
            //printf("\r\n**********%s:%d:%s:debug_GBDevice_info_list_lock:iRet=%d, iGBDeviceInfoLockCount=%lld**********\r\n", file, line, func, iRet, iGBDeviceInfoLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_GBDevice_info_list_lock:iRet=%d, iGBDeviceInfoLockCount=%lld", file, line, func, iRet, iGBDeviceInfoLockCount);
        }
    }

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_GBDevice_info_list_unlock
 功能描述  : 标准物理设备队列解锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int debug_GBDevice_info_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GBDeviceInfoMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "debug_GBDevice_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_GBDeviceInfoMapLock, file,  line, func);

    iGBDeviceInfoUnLockCount++;

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_GBDevice_info_list_unlock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iGBDeviceInfoLockCount != iGBDeviceInfoUnLockCount)
        {
            //printf("\r\n**********%s:%d:%s:debug_GBDevice_info_list_unlock:iRet=%d, iGBDeviceInfoUnLockCount=%lld**********\r\n", file, line, func, iRet, iGBDeviceInfoUnLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_GBDevice_info_list_unlock:iRet=%d, iGBDeviceInfoUnLockCount=%lld", file, line, func, iRet, iGBDeviceInfoUnLockCount);
        }
    }

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : GBDevice_info_add
 功能描述  : 添加标准物理设备信息到队列中
 输入参数  : char* device_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
//int GBDevice_info_add(char* device_id)
//{
//    GBDevice_info_t* pGBDeviceInfo = NULL;
//    int i = 0;

//    if (g_GBDeviceInfoList == NULL || device_id == NULL)
//    {
//        return -1;
//    }

//    i = GBDevice_info_init(&pGBDeviceInfo);

//    if (i != 0)
//    {
//        return -1;
//    }

//    pGBDeviceInfo->device_id = sgetcopy(device_id);

//    GBDevice_info_list_lock();
//    i = list_add(g_GBDeviceInfoList->pGBDeviceInfoList, pGBDeviceInfo, -1); /* add to list tail */

//    if (i == -1)
//    {
//        GBDevice_info_list_unlock();
//        GBDevice_info_free(pGBDeviceInfo);
//        sfree(pGBDeviceInfo);
//        return -1;
//    }

//    GBDevice_info_list_unlock();
//    return i;
//}

int GBDevice_info_add(GBDevice_info_t* pGBDeviceInfo)
{
    if (pGBDeviceInfo == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_info_add() exit---: Param Error \r\n");
        return -1;
    }

    GBDEVICE_SMUTEX_LOCK();

    g_GBDeviceInfoMap[pGBDeviceInfo->device_id] = pGBDeviceInfo;

    GBDEVICE_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 函 数 名  : GBDevice_info_remove
 功能描述  : 从队列中移除标准物理设备信息
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int GBDevice_info_remove(char* device_id)
{
    if (NULL == device_id)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_info_remove() exit---: Param Error \r\n");
        return -1;
    }

    GBDEVICE_SMUTEX_LOCK();
    g_GBDeviceInfoMap.erase(device_id);
    GBDEVICE_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : GBDevice_info_find
 功能描述  : 从队列中查找标准物理设备
 输入参数  : char* device_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月16日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
GBDevice_info_t* GBDevice_info_find(char* device_id)
{
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBDevice_Info_Iterator Itr;

    if (NULL == device_id)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_info_find() exit---: Param Error \r\n");
        return NULL;
    }

    GBDEVICE_SMUTEX_LOCK();

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        GBDEVICE_SMUTEX_UNLOCK();
        return NULL;
    }

    Itr = g_GBDeviceInfoMap.find(device_id);

    if (Itr == g_GBDeviceInfoMap.end())
    {
        GBDEVICE_SMUTEX_UNLOCK();
        return NULL;
    }
    else
    {
        pGBDeviceInfo = Itr->second;
        GBDEVICE_SMUTEX_UNLOCK();
        return pGBDeviceInfo;
    }

    GBDEVICE_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : Get_GBDevice_Index_By_Device_ID
 功能描述  : 根据物理设备的ID获取物理设备的索引
 输入参数  : char* device_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月22日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int Get_GBDevice_Index_By_Device_ID(char* device_id)
{
    GBDevice_info_t* pGBDeviceInfo = NULL;

    if (NULL == device_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "Get_GBDevice_Index_By_Device_ID() exit---: Param Error \r\n");
        return -1;
    }

    pGBDeviceInfo = GBDevice_info_find(device_id);

    if (NULL != pGBDeviceInfo)
    {
        return pGBDeviceInfo->id;
    }
    else
    {
        return -1;
    }
}

/*****************************************************************************
 函 数 名  : GBDevice_info_find_by_device_index
 功能描述  : 通过物理设备索引获取物理设备信息
 输入参数  : int device_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年11月29日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
GBDevice_info_t* GBDevice_info_find_by_device_index(int device_index)
{
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBDevice_Info_Iterator Itr;

    if (device_index <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_info_find_by_device_index() exit---: Param Error \r\n");
        return NULL;
    }

    GBDEVICE_SMUTEX_LOCK();

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        GBDEVICE_SMUTEX_UNLOCK();
        return NULL;
    }

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        pGBDeviceInfo = Itr->second;

        if ((NULL == pGBDeviceInfo) || (pGBDeviceInfo->id <= 0))
        {
            continue;
        }

        if (pGBDeviceInfo->id == device_index)
        {
            pGBDeviceInfo = Itr->second;
            GBDEVICE_SMUTEX_UNLOCK();
            return pGBDeviceInfo;
        }
    }

    GBDEVICE_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : GBDevice_info_find_by_ip_and_port
 功能描述  : 通过物理设备的IP和端口查找物理设备
 输入参数  : char* login_ip
             int login_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月9日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
GBDevice_info_t* GBDevice_info_find_by_ip_and_port(char* login_ip, int login_port)
{
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBDevice_Info_Iterator Itr;

    if (login_ip == NULL || login_port <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_info_find_by_ip_and_port() exit---: Param Error \r\n");
        return NULL;
    }

    GBDEVICE_SMUTEX_LOCK();

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        GBDEVICE_SMUTEX_UNLOCK();
        return NULL;
    }

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        pGBDeviceInfo = Itr->second;

        if ((NULL == pGBDeviceInfo) || (pGBDeviceInfo->login_ip == NULL) || (pGBDeviceInfo->login_port <= 0))
        {
            continue;
        }

        if (0 == sstrcmp(pGBDeviceInfo->login_ip, login_ip) && pGBDeviceInfo->login_port == login_port)
        {
            if (pGBDeviceInfo->reg_status == 0)
            {
                //DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "GBDevice_info_find_by_ip_and_port() Device Not Registered:login_ip=%s,login_port=%d \r\n", login_ip, login_port);
                continue;
            }
            else
            {
                pGBDeviceInfo = Itr->second;
                GBDEVICE_SMUTEX_UNLOCK();
                return pGBDeviceInfo;
            }
        }
    }

    GBDEVICE_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : GBDevice_info_find_by_id_ip_and_port
 功能描述  : 通过物理设备的ID，IP和端口查找物理设备
 输入参数  : char* device_id
             char* login_ip
             int login_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月9日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
GBDevice_info_t* GBDevice_info_find_by_id_ip_and_port(char* device_id, char* login_ip, int login_port)
{
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBDevice_Info_Iterator Itr;

    if (NULL == device_id || login_ip == NULL || login_port <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_info_find_by_id_ip_and_port() exit---: Param Error \r\n");
        return NULL;
    }

    GBDEVICE_SMUTEX_LOCK();

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        GBDEVICE_SMUTEX_UNLOCK();
        return NULL;
    }

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        pGBDeviceInfo = Itr->second;

        if ((NULL == pGBDeviceInfo) || (pGBDeviceInfo->login_ip == NULL) || (pGBDeviceInfo->login_port <= 0))
        {
            continue;
        }

        if (0 == sstrcmp(pGBDeviceInfo->device_id, device_id)
            && 0 == sstrcmp(pGBDeviceInfo->login_ip, login_ip)
            && pGBDeviceInfo->login_port == login_port)
        {
            if (pGBDeviceInfo->reg_status == 0)
            {
                //DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "GBDevice_info_find_by_id_ip_and_port() Device Not Registered:login_ip=%s,login_port=%d \r\n", login_ip, login_port);
                continue;
            }
            else
            {
                pGBDeviceInfo = Itr->second;
                GBDEVICE_SMUTEX_UNLOCK();
                return pGBDeviceInfo;
            }
        }
    }

    GBDEVICE_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : AddLogicDeviceGroupAndMapDataDeviceInfo
 功能描述  : 添加逻辑设备分组和分组关系信息到物理设备信息中
 输入参数  : DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddLogicDeviceGroupAndMapDataDeviceInfo(DBOper* pDevice_Srv_dboper)
{
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBDevice_Info_Iterator Itr;

    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "\r\n ********************************************** \
		\r\n |AddLogicDeviceGroupAndMapDataDeviceInfo:BEGIN \
		\r\n ********************************************** \r\n");

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        return -1;
    }

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        pGBDeviceInfo = Itr->second;

        if ((NULL == pGBDeviceInfo) || (pGBDeviceInfo->id <= 0))
        {
            continue;
        }

        if (EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type)
        {
            continue;
        }

        AddLogicDeviceGroupConfigToDeviceInfo(pGBDeviceInfo, pDevice_Srv_dboper);
        AddLogicDeviceGroupMapConfigToDeviceInfo(pGBDeviceInfo, pDevice_Srv_dboper);
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "\r\n ********************************************** \
		\r\n |AddLogicDeviceGroupAndMapDataDeviceInfo:END \
		\r\n ********************************************** \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : scan_GBDevice_info_list_for_subscribe
 功能描述  : 扫描标准物理设备队列，发送订阅消息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月16日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void scan_GBDevice_info_list_for_subscribe()
{
    int iRet = -1;
    GBDevice_Info_Iterator Itr;

    GBDevice_info_t* pGBDeviceInfo = NULL;

    GBDevice_info_t* pSubscribeGBDeviceInfo = NULL;
    GBDevice_info_t* pRefreshGBDeviceInfo = NULL;
    GBDevice_info_t* pUnSubscribeGBDeviceInfo = NULL;

    needtoproc_GBDeviceinfo_queue needSubscribe;
    needtoproc_GBDeviceinfo_queue needRefresh;
    needtoproc_GBDeviceinfo_queue needUnSubscribe;

    needSubscribe.clear();
    needRefresh.clear();
    needUnSubscribe.clear();

    GBDEVICE_SMUTEX_LOCK();

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        GBDEVICE_SMUTEX_UNLOCK();
        return;
    }

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        pGBDeviceInfo = Itr->second;

        if ((NULL == pGBDeviceInfo) || (pGBDeviceInfo->id <= 0))
        {
            continue;
        }

        if (EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type) /* 目前仅限于第三方平台可以发起订阅 */
        {
            continue;
        }

        if (pGBDeviceInfo->three_party_flag == 0) /* 非第三方暂时不发起订阅 */
        {
            continue;
        }

        if (pGBDeviceInfo->reg_status == 0
            && pGBDeviceInfo->catalog_subscribe_flag == 1) /* 需要去订阅 */
        {
            needUnSubscribe.push_back(pGBDeviceInfo);
            continue;
        }
        else if (pGBDeviceInfo->reg_status == 1
                 && pGBDeviceInfo->catalog_subscribe_flag == 0) /* 需要发起订阅 */
        {
            pGBDeviceInfo->catalog_subscribe_interval--;

            if (pGBDeviceInfo->catalog_subscribe_interval <= 0)
            {
                needSubscribe.push_back(pGBDeviceInfo);
                continue;
            }
        }
        else if (pGBDeviceInfo->reg_status == 1
                 && pGBDeviceInfo->catalog_subscribe_flag == 1) /* 需要发起刷新订阅 */
        {
            pGBDeviceInfo->catalog_subscribe_expires_count--;

            if (pGBDeviceInfo->catalog_subscribe_expires_count <= (pGBDeviceInfo->catalog_subscribe_expires) / 2)
            {
                needRefresh.push_back(pGBDeviceInfo); /* 发送刷新订阅 */
                continue;
            }
        }
    }

    GBDEVICE_SMUTEX_UNLOCK();

    /* 处理需要发送订阅消息的 */
    while (!needSubscribe.empty())
    {
        pSubscribeGBDeviceInfo = (GBDevice_info_t*) needSubscribe.front();
        needSubscribe.pop_front();

        if (NULL != pSubscribeGBDeviceInfo)
        {
            /* 发送初始订阅 */
            iRet = SendSubscribeMessageToSubGBDevice(pSubscribeGBDeviceInfo, 0);

            if (iRet != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送初始订阅消息到前端设备失败:前端设备ID=%s, IP地址=%s, 端口号=%d", pSubscribeGBDeviceInfo->device_id, pSubscribeGBDeviceInfo->login_ip, pSubscribeGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendSubscribeMessageToSubGBDevice Error:SubGBDeviceID=%s, IP=%s, port=%d", pSubscribeGBDeviceInfo->device_id, pSubscribeGBDeviceInfo->login_ip, pSubscribeGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "scan_GBDevice_info_list_for_subscribe() SendSubscribeMessageToSubGBDevice Error \r\n");
                continue;
            }
            else if (iRet == 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送初始订阅消息到前端设备成功:前端设备ID=%s, IP地址=%s, 端口号=%d", pSubscribeGBDeviceInfo->device_id, pSubscribeGBDeviceInfo->login_ip, pSubscribeGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendSubscribeMessageToSubGBDevice Ok:SubGBDeviceID=%s, IP=%s, port=%d", pSubscribeGBDeviceInfo->device_id, pSubscribeGBDeviceInfo->login_ip, pSubscribeGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "scan_GBDevice_info_list_for_subscribe() SendSubscribeMessageToSubGBDevice OK \r\n");
            }
        }
    }

    needSubscribe.clear();

    /* 处理需要发送刷新注册消息的 */
    while (!needRefresh.empty())
    {
        pRefreshGBDeviceInfo = (GBDevice_info_t*) needRefresh.front();
        needRefresh.pop_front();

        if (NULL != pRefreshGBDeviceInfo)
        {
            /* 发送刷新订阅 */
            iRet = SendSubscribeMessageToSubGBDevice(pRefreshGBDeviceInfo, 1);

            if (iRet != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送刷新订阅消息到前端设备失败:前端设备ID=%s, IP地址=%s, 端口号=%d", pRefreshGBDeviceInfo->device_id, pRefreshGBDeviceInfo->login_ip, pRefreshGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fial to send the message of refreshing the subscribe to the front-end equipment :ID=%s, IP=%s, port=%d", pRefreshGBDeviceInfo->device_id, pRefreshGBDeviceInfo->login_ip, pRefreshGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "scan_GBDevice_info_list_for_subscribe() SendSubscribeMessageToSubGBDevice Error \r\n");
                continue;
            }
            else if (iRet == 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送刷新订阅消息到前端设备成功:前端设备ID=%s, IP地址=%s, 端口号=%d", pRefreshGBDeviceInfo->device_id, pRefreshGBDeviceInfo->login_ip, pRefreshGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send the message of refreshing the subscribe to the front-end equipment :ID=%s, IP=%s, port=%d", pRefreshGBDeviceInfo->device_id, pRefreshGBDeviceInfo->login_ip, pRefreshGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "scan_GBDevice_info_list_for_subscribe() SendSubscribeMessageToSubGBDevice OK \r\n");
            }
        }
    }

    needRefresh.clear();

    /* 处理需要发送去订阅消息的 */
    while (!needUnSubscribe.empty())
    {
        pUnSubscribeGBDeviceInfo = (GBDevice_info_t*) needUnSubscribe.front();
        needUnSubscribe.pop_front();

        if (NULL != pUnSubscribeGBDeviceInfo)
        {
            /* 发送取消订阅 */
            iRet = SendSubscribeMessageToSubGBDevice(pUnSubscribeGBDeviceInfo, 2);

            if (iRet != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送取消订阅消息到前端设备失败:前端设备ID=%s, IP地址=%s, 端口号=%d", pUnSubscribeGBDeviceInfo->device_id, pUnSubscribeGBDeviceInfo->login_ip, pUnSubscribeGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fial to send the message of refreshing the subscribe to the front-end equipment :ID=%s, IP=%s, port=%d", pRefreshGBDeviceInfo->device_id, pRefreshGBDeviceInfo->login_ip, pRefreshGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "scan_GBDevice_info_list_for_subscribe() SendSubscribeMessageToSubGBDevice Error \r\n");
                continue;
            }
            else if (iRet == 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送取消订阅消息到前端设备成功:前端设备ID=%s, IP地址=%s, 端口号=%d", pUnSubscribeGBDeviceInfo->device_id, pUnSubscribeGBDeviceInfo->login_ip, pUnSubscribeGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send the message of refreshing the subscribe to the front-end equipment :ID=%s, IP=%s, port=%d", pRefreshGBDeviceInfo->device_id, pRefreshGBDeviceInfo->login_ip, pRefreshGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "scan_GBDevice_info_list_for_subscribe() SendSubscribeMessageToSubGBDevice OK \r\n");
            }
        }
    }

    needUnSubscribe.clear();

    return;
}

/*****************************************************************************
 函 数 名  : scan_GBDevice_info_list_for_expires
 功能描述  : 扫描物理设备队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月6日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void scan_GBDevice_info_list_for_expires()
{
    int iRet = -1;
    GBDevice_Info_Iterator Itr;

    GBDevice_info_t* pProcGBDeviceInfo = NULL;
    needtoproc_GBDeviceinfo_queue needProc;

    needProc.clear();

    GBDEVICE_SMUTEX_LOCK();

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        GBDEVICE_SMUTEX_UNLOCK();
        return;
    }

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        pProcGBDeviceInfo = Itr->second;

        if ((NULL == pProcGBDeviceInfo) || (pProcGBDeviceInfo->id <= 0))
        {
            continue;
        }

        if (pProcGBDeviceInfo->reg_status == 0)
        {
            continue;
        }

        /* 没有最后保活时间的过滤掉 */
        if (pProcGBDeviceInfo->last_keep_alive_time <= 0)
        {
            continue;
        }

        /* 保活时间的间隔不合法的过滤掉 */
        if (pProcGBDeviceInfo->keep_alive_expires <= 0)
        {
            continue;
        }

        pProcGBDeviceInfo->keep_alive_expires_count = pProcGBDeviceInfo->keep_alive_expires_count - 10;

        if (pProcGBDeviceInfo->keep_alive_expires_count <= 0)
        {
            /* 通过ping监测一下前端是否在线 */
            iRet = checkGBDeviceIsOnline(pProcGBDeviceInfo->login_ip);

            if (0 == iRet)
            {
                needProc.push_back(pProcGBDeviceInfo);
            }
            else
            {
                pProcGBDeviceInfo->last_keep_alive_time = 0;
                pProcGBDeviceInfo->keep_alive_expires = 0;
                pProcGBDeviceInfo->keep_alive_expires_count = 0;
                //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "scan_GBDevice_info_list_for_expires() device_id=%s, device_ip=%s, Ping OK \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip);
                //SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "前端设备保活消息超时, 但是可以ping通，取消注销处理:前端设备ID=%s, IP地址=%s, 端口号=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
                //EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Front end device keep-alive message timeout, but ping OK, cancel log off:front-end device ID=%s, IP address=%s, Port number=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
            }
        }
    }

    GBDEVICE_SMUTEX_UNLOCK();

    /* 处理需要发送注销消息的 */
    while (!needProc.empty())
    {
        pProcGBDeviceInfo = (GBDevice_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pProcGBDeviceInfo)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "前端设备保活消息超时, 主动注销登录:前端设备ID=%s, IP地址=%s, 端口号=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Front end device keep-alive message timeout, active log off:front-end device ID=%s, IP address=%s, Port number=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);

            SIP_UASRemoveRegisterInfo(pProcGBDeviceInfo->reg_info_index);

            iRet = GBDevice_reg_msg_add(pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->device_type, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, NULL, 0, pProcGBDeviceInfo->reg_info_index, 0);

            if (iRet != 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "scan_GBDevice_info_list_for_expires() GBDevice_reg_msg_add Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "scan_GBDevice_info_list_for_expires() GBDevice_reg_msg_add OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
            }
        }
    }

    needProc.clear();

    return;
}

/*****************************************************************************
 函 数 名  : check_GBDevice_info_from_db_to_list
 功能描述  : 更新物理设备配置信息到内存中
 输入参数  : DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月7日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int check_GBDevice_info_from_db_to_list(DBOper* pdboper)
{
    int i = 0;
    int iRet = -1;
    GBDevice_Info_Iterator Itr;

    GBDevice_info_t* pProcGBDeviceInfo = NULL;
    needtoproc_GBDeviceinfo_queue needProc;
    GBDevice_cfg_t GBDevice_cfg;

    needProc.clear();

    GBDEVICE_SMUTEX_LOCK();

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        GBDEVICE_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        pProcGBDeviceInfo = Itr->second;

        if ((NULL == pProcGBDeviceInfo) || (pProcGBDeviceInfo->id <= 0))
        {
            continue;
        }

        if (pProcGBDeviceInfo->device_id[0] == '\0')
        {
            continue;
        }

        needProc.push_back(pProcGBDeviceInfo);
    }

    GBDEVICE_SMUTEX_UNLOCK();

    /* 处理需要发送订阅消息的 */
    while (!needProc.empty())
    {
        pProcGBDeviceInfo = (GBDevice_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pProcGBDeviceInfo)
        {
            /* 传输协议 */
            if (GetDevCfg(pdboper, pProcGBDeviceInfo->device_id, GBDevice_cfg) > 0)
            {
                /* 不能删除 */
                pProcGBDeviceInfo->del_mark = 0;

                if ((EV9000_DEVICETYPE_SIPSERVER == pProcGBDeviceInfo->device_type && 0 == pProcGBDeviceInfo->three_party_flag)
                    || EV9000_DEVICETYPE_DECODER == pProcGBDeviceInfo->device_type
                    || EV9000_DEVICETYPE_MGWSERVER == pProcGBDeviceInfo->device_type)
                {

                }
                else
                {
                    /* 传输方式重新赋值 */
                    if (pProcGBDeviceInfo->trans_protocol != GBDevice_cfg.trans_protocol)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端标准物理设备信息发生变化, 物理设备ID=%s, IP地址=%s, 传输方式发生变化: 老的传输方式=%d, 新的传输方式=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->trans_protocol, GBDevice_cfg.trans_protocol);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "The front standard physical device information change ,ID=%s, IP=%s, Transmission mode change: old transmission mode change=%d, new transmission mode change=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->trans_protocol, GBDevice_cfg.trans_protocol);

                        pProcGBDeviceInfo->trans_protocol = GBDevice_cfg.trans_protocol;
                        DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "check_GBDevice_info_from_db_to_list() trans_protocol changed:device_id=%s, trans_protocol=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->trans_protocol);
                    }
                }

                /* 禁用标识 */
                if (0 == GBDevice_cfg.enable)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端标准物理设备信息发生变化, 物理设备ID=%s, IP地址=%s, 设备被禁用, 将清除掉该设备的所有业务", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "The front standard physical device information change ,ID=%s, IP=%s, Equipment is disabled, all the business of this device will be removed", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip);


                    /* 删除拓扑结构表信息 */
                    iRet = DeleteTopologyPhyDeviceInfoFromDB(pProcGBDeviceInfo->device_id, pdboper);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "check_GBDevice_info_from_db_to_list() DeleteTopologyPhyDeviceInfoFromDB:device_id=%s\r\n", pProcGBDeviceInfo->device_id);

                    SIP_UASRemoveRegisterInfo(pProcGBDeviceInfo->reg_info_index);

                    /* 如果是拓扑结构表里面的设备类型，那么需要删除拓扑结构表信息，并上报给上级CMS */
                    if (EV9000_DEVICETYPE_DECODER == pProcGBDeviceInfo->device_type
                        || EV9000_DEVICETYPE_SIPSERVER == pProcGBDeviceInfo->device_type
                        || EV9000_DEVICETYPE_MGWSERVER == pProcGBDeviceInfo->device_type)
                    {
                        /* 删除拓扑结构表信息 */
                        i = DeleteTopologyPhyDeviceInfoFromDB(pProcGBDeviceInfo->device_id, pdboper);
                    }

                    /* 查找点位的业务，并停止所有业务 */
                    if (EV9000_DEVICETYPE_DECODER == pProcGBDeviceInfo->device_type) /* 解码器根据主叫侧信息停止业务 */
                    {
                        i = StopAllServiceTaskByCallerIPAndPort(pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_GBDevice_info_from_db_to_list() StopAllServiceTaskByCallerIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "check_GBDevice_info_from_db_to_list() StopAllServiceTaskByCallerIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }
                    }
                    else if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pProcGBDeviceInfo->device_type) /* IVS根据主叫、被叫侧信息停止业务 */
                    {
                        i = StopAllServiceTaskByCallerIPAndPort(pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port); /* 主动请求的视频流 */

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_GBDevice_info_from_db_to_list() StopAllServiceTaskByCallerIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "check_GBDevice_info_from_db_to_list() StopAllServiceTaskByCallerIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }

                        i = StopAllServiceTaskByCalleeIPAndPort(pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port); /* 被动发送的智能分析频流 */

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_GBDevice_info_from_db_to_list() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "check_GBDevice_info_from_db_to_list() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }
                    }
                    else if (EV9000_DEVICETYPE_SIPSERVER == pProcGBDeviceInfo->device_type) /* 下级CMS */
                    {
                        i = StopAllServiceTaskByCalleeIPAndPort(pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_GBDevice_info_from_db_to_list() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "check_GBDevice_info_from_db_to_list() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }
                    }
                    else
                    {
                        i = StopAllServiceTaskByCalleeIPAndPort(pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_GBDevice_info_from_db_to_list() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "check_GBDevice_info_from_db_to_list() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }
                    }

                    if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pProcGBDeviceInfo->device_type)
                    {
                        /* 通知客户端，逻辑设备智能分析掉线 */
                        i = SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser(pProcGBDeviceInfo->id);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_GBDevice_info_from_db_to_list() SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser Error:device_id=%s, i=%d \r\n", pProcGBDeviceInfo->device_id, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "check_GBDevice_info_from_db_to_list() SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser OK:device_id=%s, i=%d \r\n", pProcGBDeviceInfo->device_id, i);
                        }
                    }
                    else
                    {
                        /* 通知客户端，逻辑设备掉线 */
                        i = SendAllGBLogicDeviceStatusOffAlarmToAllClientUser(pProcGBDeviceInfo->id);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_GBDevice_info_from_db_to_list() SendAllGBLogicDeviceStatusOffAlarmToAllClientUser Error:device_id=%s, i=%d \r\n", pProcGBDeviceInfo->device_id, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "check_GBDevice_info_from_db_to_list() SendAllGBLogicDeviceStatusOffAlarmToAllClientUser OK:device_id=%s, i=%d \r\n", pProcGBDeviceInfo->device_id, i);
                        }
                    }

                    if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pProcGBDeviceInfo->device_type)
                    {
                        /* 发送智能分析设备状态消息给客户端 */
                        i = SendAllIntelligentGBLogicDeviceStatusProc(pProcGBDeviceInfo->id, 0, pdboper);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_GBDevice_info_from_db_to_list() SendAllIntelligentGBLogicDeviceStatusProc Error:device_id=%s, i=%d \r\n", pProcGBDeviceInfo->device_id, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "check_GBDevice_info_from_db_to_list() SendAllIntelligentGBLogicDeviceStatusProc OK:device_id=%s, i=%d \r\n", pProcGBDeviceInfo->device_id, i);
                        }
                    }
                    else
                    {
                        /* 发送设备状态消息给客户端 */
                        i = SendAllGBLogicDeviceStatusProc(pProcGBDeviceInfo->id, 0, 0, pdboper);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_GBDevice_info_from_db_to_list() SendAllGBLogicDeviceStatusProc Error:device_id=%s, i=%d \r\n", pProcGBDeviceInfo->device_id, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "check_GBDevice_info_from_db_to_list() SendAllGBLogicDeviceStatusProc OK:device_id=%s, i=%d \r\n", pProcGBDeviceInfo->device_id, i);
                        }
                    }

                    /* 移除逻辑设备 */
                    if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pProcGBDeviceInfo->device_type)
                    {
                        //修正逻辑设备的智能分析状态
                        i = SetGBLogicDeviceIntelligentStatus(pProcGBDeviceInfo->id, INTELLIGENT_STATUS_NULL);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_GBDevice_info_from_db_to_list() SetGBLogicDeviceIntelligentStatus Error:device_id=%s, i=%d \r\n", pProcGBDeviceInfo->device_id, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "check_GBDevice_info_from_db_to_list() SetGBLogicDeviceIntelligentStatus OK:device_id=%s, i=%d \r\n", pProcGBDeviceInfo->device_id, i);
                        }
                    }
                    else
                    {
                        //移除下面对应的逻辑设备
                        i = RemoveGBLogicDevice(pProcGBDeviceInfo->id);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_GBDevice_info_from_db_to_list() RemoveGBLogicDevice Error:device_id=%s, i=%d \r\n", pProcGBDeviceInfo->device_id, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "check_GBDevice_info_from_db_to_list() RemoveGBLogicDevice OK:device_id=%s, i=%d \r\n", pProcGBDeviceInfo->device_id, i);
                        }
                    }

                    /* 回收设备业务线程 */
                    if (EV9000_DEVICETYPE_SIPSERVER == pProcGBDeviceInfo->device_type
                        || EV9000_DEVICETYPE_MGWSERVER == pProcGBDeviceInfo->device_type)
                    {
                        i = device_srv_proc_thread_recycle(pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_GBDevice_info_from_db_to_list() device_srv_proc_thread_recycle Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "check_GBDevice_info_from_db_to_list() device_srv_proc_thread_recycle OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }
                    }

                    /* 内存删除掉 */
                    pProcGBDeviceInfo->del_mark = 1;
                }
            }
        }
    }

    needProc.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : set_GBDevice_info_list_del_mark
 功能描述  : 设置物理设备删除标识
 输入参数  : int del_mark
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月23日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int set_GBDevice_info_list_del_mark(int del_mark)
{
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBDevice_Info_Iterator Itr;

    GBDEVICE_SMUTEX_LOCK();

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        GBDEVICE_SMUTEX_UNLOCK();
        return NULL;
    }

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        pGBDeviceInfo = Itr->second;

        if ((NULL == pGBDeviceInfo) || (pGBDeviceInfo->id <= 0))
        {
            continue;
        }

        pGBDeviceInfo->del_mark = del_mark;
    }

    GBDEVICE_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : delete_GBLogicDevice_info_from_list_by_mark
 功能描述  : 根据删除标识，删除物理设备信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月23日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int delete_GBDevice_info_from_list_by_mark()
{
    int index = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBDevice_Info_Iterator Itr;
    vector<string> DeviceIDVector;
    DeviceIDVector.clear();

    GBDEVICE_SMUTEX_LOCK();

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        GBDEVICE_SMUTEX_UNLOCK();
        return NULL;
    }

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        pGBDeviceInfo = Itr->second;

        if ((NULL == pGBDeviceInfo) || (pGBDeviceInfo->id <= 0))
        {
            continue;
        }

        if (pGBDeviceInfo->del_mark == 1)
        {
            DeviceIDVector.push_back(pGBDeviceInfo->device_id);
        }
    }

    GBDEVICE_SMUTEX_UNLOCK();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "根据删除标识, 删除掉多余的标准物理设备信息: 删除的标准物理设备总数=%d", (int)DeviceIDVector.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "According to delete mark, delete the redundant standard physical device information: the sum of standard physical device is deleted =%d", (int)DeviceIDVector.size());

    if (DeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)DeviceIDVector.size(); index++)
        {
            pGBDeviceInfo = GBDevice_info_find((char*)DeviceIDVector[index].c_str());

            if (NULL != pGBDeviceInfo)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "根据删除标识, 删除掉多余的标准物理设备信息成功:标准物理设备ID=%s, IP地址=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "According to delete mark, delete the redundant standard physical device information successfully:ID=%s, IP=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);

                GBDEVICE_SMUTEX_LOCK();
                g_GBDeviceInfoMap.erase(pGBDeviceInfo->device_id);
                GBDevice_info_free(pGBDeviceInfo);
                delete pGBDeviceInfo;
                pGBDeviceInfo = NULL;
                GBDEVICE_SMUTEX_UNLOCK();
            }
        }
    }

    DeviceIDVector.clear();
    return 0;
}
#endif

#if DECS("标准逻辑设备信息队列")

/*****************************************************************************
 函 数 名  : GBDevice_init
 功能描述  : 逻辑设备中的物理设备结构初始化
 输入参数  : GBDevice_t* gb_device
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月26日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GBDevice_init(GBDevice_t** gb_device)
{
    *gb_device = (GBDevice_t*)smalloc(sizeof(GBDevice_t));

    if (*gb_device == NULL)
    {
        return -1;
    }

    (*gb_device)->stream_type = EV9000_STREAM_TYPE_MASTER;
    (*gb_device)->ptGBDeviceInfo = NULL;

    return 0;
}

/*****************************************************************************
 函 数 名  : GBDevice_free
 功能描述  : 逻辑设备中的物理设备结构释放
 输入参数  : GBDevice_t* gb_device
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月26日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void GBDevice_free(GBDevice_t* gb_device)
{
    if (gb_device == NULL)
    {
        return;
    }

    gb_device->stream_type = EV9000_STREAM_TYPE_MASTER;
    gb_device->ptGBDeviceInfo = NULL;

    osip_free(gb_device);
    gb_device = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : GBDevice_add
 功能描述  : 逻辑设备中的物理设备添加
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int stream_type
             GBDevice_info_t* pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月26日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GBDevice_add(GBLogicDevice_info_t* pGBLogicDeviceInfo, int stream_type, GBDevice_info_t* pGBDeviceInfo)
{
    int i = 0;
    int iRet = 0;
    GBDevice_t* pGBDevice = NULL;

    if (NULL == pGBLogicDeviceInfo || NULL == pGBLogicDeviceInfo->pGBDeviceInfoList)
    {
        return -1;
    }

    if (stream_type <= 0 || NULL == pGBDeviceInfo)
    {
        return -1;
    }

    i = GBDevice_init(&pGBDevice);

    if (i != 0)
    {
        return -1;
    }


    pGBDevice->stream_type = stream_type;
    pGBDevice->ptGBDeviceInfo = pGBDeviceInfo;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)pGBLogicDeviceInfo->pGBDeviceInfoListLock);
#endif

    i = osip_list_add(pGBLogicDeviceInfo->pGBDeviceInfoList, pGBDevice, -1); /* add to list tail */

    if (i == -1)
    {
        GBDevice_free(pGBDevice);
        pGBDevice = NULL;

#ifdef MULTI_THR
        CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)pGBLogicDeviceInfo->pGBDeviceInfoListLock);
#endif
        return -1;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)pGBLogicDeviceInfo->pGBDeviceInfoListLock);
#endif
    return i - 1;
}

/*****************************************************************************
 函 数 名  : GBDevice_remove
 功能描述  : 逻辑设备中的物理设备移除
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int stream_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年9月4日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GBDevice_remove(GBLogicDevice_info_t* pGBLogicDeviceInfo, int stream_type)
{
    int i = 0;
    int iRet = 0;
    GBDevice_t* pGBDevice = NULL;

    if (NULL == pGBLogicDeviceInfo || NULL == pGBLogicDeviceInfo->pGBDeviceInfoList)
    {
        return -1;
    }

    if (stream_type <= 0)
    {
        return -1;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)pGBLogicDeviceInfo->pGBDeviceInfoListLock);
#endif

    if (osip_list_size(pGBLogicDeviceInfo->pGBDeviceInfoList) <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_remove() exit---: GBDevice Info List NULL \r\n");

#ifdef MULTI_THR
        CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)pGBLogicDeviceInfo->pGBDeviceInfoListLock);
#endif
        return NULL;
    }

    for (i = 0; i < osip_list_size(pGBLogicDeviceInfo->pGBDeviceInfoList); i++)
    {
        pGBDevice = (GBDevice_t*)osip_list_get(pGBLogicDeviceInfo->pGBDeviceInfoList, i);

        if (NULL == pGBDevice || NULL == pGBDevice->ptGBDeviceInfo)
        {
            continue;
        }

        if (pGBDevice->stream_type == stream_type)
        {
            osip_list_remove(pGBLogicDeviceInfo->pGBDeviceInfoList, i);
            GBDevice_free(pGBDevice);
            pGBDevice = NULL;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)pGBLogicDeviceInfo->pGBDeviceInfoListLock);
#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : GBDevice_get_by_stream_type
 功能描述  : 根据媒体流类型获取逻辑设备中的物理设备
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int stream_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月26日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
GBDevice_t* GBDevice_get_by_stream_type(GBLogicDevice_info_t* pGBLogicDeviceInfo, int stream_type)
{
    int i = 0;
    int iRet = 0;
    GBDevice_t* pGBDevice = NULL;

    if (NULL == pGBLogicDeviceInfo || NULL == pGBLogicDeviceInfo->pGBDeviceInfoList)
    {
        return NULL;
    }

    if (stream_type <= 0)
    {
        return NULL;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)pGBLogicDeviceInfo->pGBDeviceInfoListLock);
#endif

    if (osip_list_size(pGBLogicDeviceInfo->pGBDeviceInfoList) <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_get_by_stream_type() exit---: GBDevice Info List NULL \r\n");

#ifdef MULTI_THR
        CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)pGBLogicDeviceInfo->pGBDeviceInfoListLock);
#endif
        return NULL;
    }

    for (i = 0; i < osip_list_size(pGBLogicDeviceInfo->pGBDeviceInfoList); i++)
    {
        pGBDevice = (GBDevice_t*)osip_list_get(pGBLogicDeviceInfo->pGBDeviceInfoList, i);

        if (NULL == pGBDevice || NULL == pGBDevice->ptGBDeviceInfo)
        {
            continue;
        }

        if (pGBDevice->stream_type == stream_type)
        {
#ifdef MULTI_THR
            CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)pGBLogicDeviceInfo->pGBDeviceInfoListLock);
#endif
            return pGBDevice;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)pGBLogicDeviceInfo->pGBDeviceInfoListLock);
#endif
    return NULL;
}

/*****************************************************************************
 函 数 名  : GBDevice_info_get_by_stream_type
 功能描述  : 根据媒体流类型获取物理设备信息
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int stream_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月26日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
GBDevice_info_t* GBDevice_info_get_by_stream_type(GBLogicDevice_info_t* pGBLogicDeviceInfo, int stream_type)
{
    int i = 0;
    int iRet = 0;
    GBDevice_t* pGBDevice = NULL;

    if (NULL == pGBLogicDeviceInfo || NULL == pGBLogicDeviceInfo->pGBDeviceInfoList)
    {
        return NULL;
    }

    if (stream_type <= 0)
    {
        return NULL;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)pGBLogicDeviceInfo->pGBDeviceInfoListLock);
#endif

    if (osip_list_size(pGBLogicDeviceInfo->pGBDeviceInfoList) <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_info_get_by_stream_type() exit---: GBDevice Info List NULL \r\n");

#ifdef MULTI_THR
        CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)pGBLogicDeviceInfo->pGBDeviceInfoListLock);
#endif
        return NULL;
    }

    for (i = 0; i < osip_list_size(pGBLogicDeviceInfo->pGBDeviceInfoList); i++)
    {
        pGBDevice = (GBDevice_t*)osip_list_get(pGBLogicDeviceInfo->pGBDeviceInfoList, i);

        if (NULL == pGBDevice || NULL == pGBDevice->ptGBDeviceInfo)
        {
            continue;
        }

        if (pGBDevice->stream_type == stream_type)
        {

#ifdef MULTI_THR
            CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)pGBLogicDeviceInfo->pGBDeviceInfoListLock);
#endif
            return pGBDevice->ptGBDeviceInfo;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)pGBLogicDeviceInfo->pGBDeviceInfoListLock);
#endif
    return NULL;
}

/*****************************************************************************
 函 数 名  : GBDevice_info_get_by_stream_type2
 功能描述  : 根据媒体流类型获取物理设备信息
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int stream_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月23日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
GBDevice_info_t* GBDevice_info_get_by_stream_type2(GBLogicDevice_info_t* pGBLogicDeviceInfo, int stream_type)
{
    int i = 0;
    GBDevice_t* pGBDevice = NULL;

    if (NULL == pGBLogicDeviceInfo || NULL == pGBLogicDeviceInfo->pGBDeviceInfoList)
    {
        return NULL;
    }

    if (stream_type <= 0)
    {
        return NULL;
    }

    if (osip_list_size(pGBLogicDeviceInfo->pGBDeviceInfoList) <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_info_get_by_stream_type() exit---: GBDevice Info List NULL \r\n");
        return NULL;
    }

    for (i = 0; i < osip_list_size(pGBLogicDeviceInfo->pGBDeviceInfoList); i++)
    {
        pGBDevice = (GBDevice_t*)osip_list_get(pGBLogicDeviceInfo->pGBDeviceInfoList, i);

        if (NULL == pGBDevice || NULL == pGBDevice->ptGBDeviceInfo)
        {
            continue;
        }

        if (pGBDevice->stream_type == stream_type)
        {
            return pGBDevice->ptGBDeviceInfo;
        }
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : GBLogicDevice_info_init
 功能描述  : 标准逻辑设备结构初始化
 输入参数  : GBLogicDevice_info_t ** GBLogicDevice_info
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int GBLogicDevice_info_init(GBLogicDevice_info_t** GBLogicDevice_info)
{
    *GBLogicDevice_info = (GBLogicDevice_info_t*)osip_malloc(sizeof(GBLogicDevice_info_t));

    if (*GBLogicDevice_info == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDevice_info_init() exit---: *GBLogicDevice_info Smalloc Error \r\n");
        return -1;
    }

    (*GBLogicDevice_info)->id = 0;
    (*GBLogicDevice_info)->device_id[0] = '\0';
    (*GBLogicDevice_info)->device_name[0] = '\0';
    (*GBLogicDevice_info)->enable = 0;
    (*GBLogicDevice_info)->device_type = 0;
    (*GBLogicDevice_info)->alarm_device_sub_type = 0;
    (*GBLogicDevice_info)->ctrl_enable = 0;
    (*GBLogicDevice_info)->mic_enable = 0;
    (*GBLogicDevice_info)->frame_count = 25;
    (*GBLogicDevice_info)->alarm_duration = 0;
    (*GBLogicDevice_info)->phy_mediaDeviceIndex = 0;
    (*GBLogicDevice_info)->phy_mediaDeviceChannel = 0;
    (*GBLogicDevice_info)->phy_ctrlDeviceIndex = 0;
    (*GBLogicDevice_info)->phy_ctrlDeviceChannel = 0;
    (*GBLogicDevice_info)->stream_count = 0;
    (*GBLogicDevice_info)->record_type = 0;
    (*GBLogicDevice_info)->guard_type = 1;
    (*GBLogicDevice_info)->other_realm = 0;

    (*GBLogicDevice_info)->manufacturer[0] = '\0';
    (*GBLogicDevice_info)->model[0] = '\0';
    (*GBLogicDevice_info)->owner[0] = '\0';
    (*GBLogicDevice_info)->civil_code[0] = '\0';
    (*GBLogicDevice_info)->block[0] = '\0';
    (*GBLogicDevice_info)->address[0] = '\0';
    (*GBLogicDevice_info)->parental = 0;
    (*GBLogicDevice_info)->parentID[0] = '\0';
    (*GBLogicDevice_info)->virtualParentID[0] = '\0';
    (*GBLogicDevice_info)->safety_way = 0;
    (*GBLogicDevice_info)->register_way = 0;

    (*GBLogicDevice_info)->cert_num[0] = '\0';
    (*GBLogicDevice_info)->certifiable = 0;
    (*GBLogicDevice_info)->error_code = 0;
    (*GBLogicDevice_info)->end_time[0] = '\0';

    (*GBLogicDevice_info)->secrecy = 0;

    (*GBLogicDevice_info)->ip_address[0] = '\0';
    (*GBLogicDevice_info)->port = 0;
    (*GBLogicDevice_info)->password[0] = '\0';

    (*GBLogicDevice_info)->status = 0;
    (*GBLogicDevice_info)->intelligent_status = INTELLIGENT_STATUS_NULL;
    (*GBLogicDevice_info)->alarm_status = ALARM_STATUS_NULL;
    (*GBLogicDevice_info)->lock_status = LOCK_STATUS_OFF;
    (*GBLogicDevice_info)->pLockUserInfo = NULL;
    (*GBLogicDevice_info)->pLockRouteInfo = NULL;

    (*GBLogicDevice_info)->longitude = 0.0;
    (*GBLogicDevice_info)->latitude = 0.0;
    (*GBLogicDevice_info)->map_layer[0] = '\0';

    (*GBLogicDevice_info)->cms_id[0] = '\0';

    (*GBLogicDevice_info)->no_stream_count = 0;
    (*GBLogicDevice_info)->shdb_channel_id = 0;
    (*GBLogicDevice_info)->del_mark = 0;

    (*GBLogicDevice_info)->Value[0] = '0';
    (*GBLogicDevice_info)->Unit[0] = '\0';
    (*GBLogicDevice_info)->AlarmPriority = 0;

    (*GBLogicDevice_info)->nResved1 = 0;
    memset((*GBLogicDevice_info)->strResved2, 0, MAX_32CHAR_STRING_LEN + 4);

    /* 对应的标准物理设备队列初始化 */
    (*GBLogicDevice_info)->pGBDeviceInfoList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == (*GBLogicDevice_info)->pGBDeviceInfoList)
    {
        osip_free(*GBLogicDevice_info);
        *GBLogicDevice_info = NULL;
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDevice_info_init() exit---: GBDevice Info List Init Error \r\n");
        return -1;
    }

    osip_list_init((*GBLogicDevice_info)->pGBDeviceInfoList);

#ifdef MULTI_THR
    /* init smutex */
    (*GBLogicDevice_info)->pGBDeviceInfoListLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == (*GBLogicDevice_info)->pGBDeviceInfoListLock)
    {
        osip_free((*GBLogicDevice_info)->pGBDeviceInfoList);
        (*GBLogicDevice_info)->pGBDeviceInfoList = NULL;
        osip_free(*GBLogicDevice_info);
        *GBLogicDevice_info = NULL;
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDevice_info_init() exit---: GBDevice Info List Lock Init Error \r\n");
        return -1;
    }

#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : GBLogicDevice_info_free
 功能描述  : 标准逻辑设备结构释放
 输入参数  : GBLogicDevice_info_t * GBLogicDevice_info
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void GBLogicDevice_info_free(GBLogicDevice_info_t* GBLogicDevice_info)
{
    if (GBLogicDevice_info == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDevice_info_free() exit---: Param Error \r\n");
        return;
    }

    GBLogicDevice_info->id = 0;

    memset(GBLogicDevice_info->device_id, 0, MAX_ID_LEN + 4);
    memset(GBLogicDevice_info->device_name, 0, MAX_128CHAR_STRING_LEN + 4);

    GBLogicDevice_info->enable = 0;
    GBLogicDevice_info->device_type = 0;
    GBLogicDevice_info->alarm_device_sub_type = 0;
    GBLogicDevice_info->ctrl_enable = 0;
    GBLogicDevice_info->mic_enable = 0;
    GBLogicDevice_info->frame_count = 25;
    GBLogicDevice_info->alarm_duration = 0;

    GBLogicDevice_info->phy_mediaDeviceIndex = 0;
    GBLogicDevice_info->phy_mediaDeviceChannel = 0;

    GBLogicDevice_info->phy_ctrlDeviceIndex = 0;
    GBLogicDevice_info->phy_ctrlDeviceChannel = 0;

    GBLogicDevice_info->stream_count = 0;
    GBLogicDevice_info->record_type = 0;
    GBLogicDevice_info->guard_type = 1;
    GBLogicDevice_info->other_realm = 0;

    memset(GBLogicDevice_info->manufacturer, 0, MAX_128CHAR_STRING_LEN + 4);
    memset(GBLogicDevice_info->model, 0, MAX_128CHAR_STRING_LEN + 4);
    memset(GBLogicDevice_info->owner, 0, MAX_128CHAR_STRING_LEN + 4);
    memset(GBLogicDevice_info->civil_code, 0, MAX_128CHAR_STRING_LEN + 4);
    memset(GBLogicDevice_info->block, 0, MAX_128CHAR_STRING_LEN + 4);
    memset(GBLogicDevice_info->address, 0, MAX_128CHAR_STRING_LEN + 4);
    GBLogicDevice_info->parental = 0;
    memset(GBLogicDevice_info->parentID, 0, MAX_128CHAR_STRING_LEN + 4);
    memset(GBLogicDevice_info->virtualParentID, 0, MAX_128CHAR_STRING_LEN + 4);

    GBLogicDevice_info->safety_way = 0;
    GBLogicDevice_info->register_way = 0;

    memset(GBLogicDevice_info->cert_num, 0, MAX_128CHAR_STRING_LEN + 4);

    GBLogicDevice_info->certifiable = 0;
    GBLogicDevice_info->error_code = 0;

    memset(GBLogicDevice_info->end_time, 0, MAX_128CHAR_STRING_LEN + 4);

    GBLogicDevice_info->secrecy = 0;

    memset(GBLogicDevice_info->ip_address, 0, MAX_IP_LEN);

    GBLogicDevice_info->port = 0;

    memset(GBLogicDevice_info->password, 0, MAX_128CHAR_STRING_LEN + 4);

    GBLogicDevice_info->status = 0;
    GBLogicDevice_info->intelligent_status = INTELLIGENT_STATUS_NULL;
    GBLogicDevice_info->alarm_status = ALARM_STATUS_NULL;
    GBLogicDevice_info->lock_status = LOCK_STATUS_OFF;
    GBLogicDevice_info->pLockUserInfo = NULL;
    GBLogicDevice_info->pLockRouteInfo = NULL;

    GBLogicDevice_info->longitude = 0.0;
    GBLogicDevice_info->latitude = 0.0;

    memset(GBLogicDevice_info->map_layer, 0, MAX_128CHAR_STRING_LEN + 4);
    memset(GBLogicDevice_info->cms_id, 0, MAX_ID_LEN + 4);

    GBLogicDevice_info->no_stream_count = 0;
    GBLogicDevice_info->shdb_channel_id = 0;
    GBLogicDevice_info->del_mark = 0;

    memset(GBLogicDevice_info->Value, 0, EV9000_LONG_LONG_STRING_LEN);
    memset(GBLogicDevice_info->Unit, 0, EV9000_SHORT_STRING_LEN);
    GBLogicDevice_info->AlarmPriority = 0;

    GBLogicDevice_info->nResved1 = 0;
    memset(GBLogicDevice_info->strResved2, 0, MAX_32CHAR_STRING_LEN + 4);

    if (NULL != GBLogicDevice_info->pGBDeviceInfoList)
    {
        osip_list_special_free(GBLogicDevice_info->pGBDeviceInfoList, (void (*)(void*))&GBDevice_free);
        osip_free(GBLogicDevice_info->pGBDeviceInfoList);
        GBLogicDevice_info->pGBDeviceInfoList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != GBLogicDevice_info->pGBDeviceInfoListLock)
    {
        osip_mutex_destroy((struct osip_mutex*)GBLogicDevice_info->pGBDeviceInfoListLock);
        GBLogicDevice_info->pGBDeviceInfoListLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 函 数 名  : GBLogicDevice_info_list_init
 功能描述  : 标准逻辑设备信息队列初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int GBLogicDevice_info_list_init()
{
    g_GBLogicDeviceInfoMap.clear();

#ifdef MULTI_THR
    g_GBLogicDeviceInfoMapLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_GBLogicDeviceInfoMapLock)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDevice_info_list_init() exit---: GBLogic Device Info Map Lock Init Error \r\n");
        return -1;
    }

#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : GBLogicDevice_info_list_free
 功能描述  : 标准逻辑设备队列释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void GBLogicDevice_info_list_free()
{
    GBLogicDevice_Info_Iterator Itr;
    GBLogicDevice_info_t* GBLogicDevice_info = NULL;

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        GBLogicDevice_info = Itr->second;

        if (NULL != GBLogicDevice_info)
        {
            GBLogicDevice_info_free(GBLogicDevice_info);
            osip_free(GBLogicDevice_info);
            GBLogicDevice_info = NULL;
        }
    }

    g_GBLogicDeviceInfoMap.clear();

#ifdef MULTI_THR

    if (NULL != g_GBLogicDeviceInfoMapLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_GBLogicDeviceInfoMapLock);
        g_GBLogicDeviceInfoMapLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 函 数 名  : GBLogicDevice_info_list_lock
 功能描述  : 标准逻辑设备队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int GBLogicDevice_info_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GBLogicDeviceInfoMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDevice_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_GBLogicDeviceInfoMapLock);

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : GBLogicDevice_info_list_unlock
 功能描述  : 标准逻辑设备队列解锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int GBLogicDevice_info_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GBLogicDeviceInfoMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDevice_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_GBLogicDeviceInfoMapLock);

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_GBLogicDevice_info_list_lock
 功能描述  : 标准逻辑设备队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int debug_GBLogicDevice_info_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GBLogicDeviceInfoMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "debug_GBLogicDevice_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_GBLogicDeviceInfoMapLock, file, line, func);

    iGBLogicDeviceInfoLockCount++;

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_GBLogicDevice_info_list_lock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iGBLogicDeviceInfoLockCount != iGBLogicDeviceInfoUnLockCount + 1)
        {
            //printf("\r\n**********%s:%d:%s:debug_GBLogicDevice_info_list_lock:iRet=%d, iGBLogicDeviceInfoLockCount=%lld**********\r\n", file, line, func, iRet, iGBLogicDeviceInfoLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_GBLogicDevice_info_list_lock:iRet=%d, iGBLogicDeviceInfoLockCount=%lld", file, line, func, iRet, iGBLogicDeviceInfoLockCount);
        }
    }

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_GBLogicDevice_info_list_unlock
 功能描述  : 标准逻辑设备队列解锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int debug_GBLogicDevice_info_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_GBLogicDeviceInfoMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "debug_GBLogicDevice_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_GBLogicDeviceInfoMapLock, file, line, func);

    iGBLogicDeviceInfoUnLockCount++;

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_GBLogicDevice_info_list_unlock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iGBLogicDeviceInfoLockCount != iGBLogicDeviceInfoUnLockCount)
        {
            //printf("\r\n**********%s:%d:%s:debug_GBLogicDevice_info_list_unlock:iRet=%d, iGBLogicDeviceInfoUnLockCount=%lld**********\r\n", file, line, func, iRet, iGBLogicDeviceInfoUnLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_GBLogicDevice_info_list_unlock:iRet=%d, iGBLogicDeviceInfoUnLockCount=%lld", file, line, func, iRet, iGBLogicDeviceInfoUnLockCount);
        }
    }

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : GBLogicDevice_info_add
 功能描述  : 添加逻辑设备信息到队列
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月10日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GBLogicDevice_info_add(GBLogicDevice_info_t* pGBLogicDeviceInfo)
{
    if (pGBLogicDeviceInfo == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDevice_info_add() exit---: Param Error \r\n");
        return -1;
    }

    GBLOGICDEVICE_SMUTEX_LOCK();

    g_GBLogicDeviceInfoMap[pGBLogicDeviceInfo->device_id] = pGBLogicDeviceInfo;

    GBLOGICDEVICE_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 函 数 名  : GBLogicDevice_info_remove
 功能描述  : 从队列中移除标准逻辑设备信息
 输入参数  : char* device_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int GBLogicDevice_info_remove(char* device_id)
{
    if (NULL == device_id)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDevice_info_remove() exit---: Param Error \r\n");
        return -1;
    }

    GBLOGICDEVICE_SMUTEX_LOCK();
    g_GBLogicDeviceInfoMap.erase(device_id);
    GBLOGICDEVICE_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : GBLogicDevice_info_find
 功能描述  : 从队列中查找标准逻辑设备
 输入参数  : char* device_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月16日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
GBLogicDevice_info_t* GBLogicDevice_info_find(char* device_id)
{
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;

    if (NULL == device_id)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDevice_info_find() exit---: Param Error \r\n");
        return NULL;
    }

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return NULL;
    }

    Itr = g_GBLogicDeviceInfoMap.find(device_id);

    if (Itr == g_GBLogicDeviceInfoMap.end())
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return NULL;
    }
    else
    {
        pGBLogicDeviceInfo = Itr->second;
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return pGBLogicDeviceInfo;
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : GBLogicDevice_info_find_by_device_index
 功能描述  : 根据设备索引从队列中查找标准逻辑设备
 输入参数  : int device_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年11月27日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
GBLogicDevice_info_t* GBLogicDevice_info_find_by_device_index(unsigned int device_index)
{
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;

    if (device_index <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDevice_info_find_by_device_index() exit---: Param Error:device_index=%u \r\n", device_index);
        return NULL;
    }

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return NULL;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->id <= 0))
        {
            continue;
        }

        if (pGBLogicDeviceInfo->id == device_index)
        {
            pGBLogicDeviceInfo = Itr->second;
            GBLOGICDEVICE_SMUTEX_UNLOCK();
            return pGBLogicDeviceInfo;
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : GBLogicDevice_info_find_by_device_index2
 功能描述  : 根据设备索引从队列中查找标准逻辑设备
 输入参数  : unsigned int device_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月14日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
GBLogicDevice_info_t* GBLogicDevice_info_find_by_device_index2(unsigned int device_index)
{
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;

    if (device_index <= 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDevice_info_find_by_device_index2() exit---: Param Error:device_index=%u \r\n", device_index);
        return NULL;
    }

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        return NULL;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->id <= 0))
        {
            continue;
        }

        if (pGBLogicDeviceInfo->id == device_index)
        {
            pGBLogicDeviceInfo = Itr->second;
            return pGBLogicDeviceInfo;
        }
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : GBLogicDevice_info_update
 功能描述  : 更新标准逻辑设备信息
 输入参数  : GBLogicDevice_info_t* pOldGBLogicDeviceInfo
                            GBLogicDevice_info_t* pNewGBLogicDeviceInfo
                            int change_type:0:内存到数据库的更新，1:数据库到内存的更新
 输出参数  : 无
 返 回 值  :int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int GBLogicDevice_info_update(GBLogicDevice_info_t* pOldGBLogicDeviceInfo, GBLogicDevice_info_t* pNewGBLogicDeviceInfo, int change_type)
{
    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBDevice_info_t* pSlaveGBDeviceInfo = NULL;
    GBDevice_t* pSlaveGBDevice = NULL;

    if (NULL == pOldGBLogicDeviceInfo || NULL == pNewGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDevice_info_update() exit---: Param Error \r\n");
        return -1;
    }

    pGBDeviceInfo = GBDevice_info_get_by_stream_type(pNewGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDevice_info_update() exit---: GBDevice_info_get_by_stream_type Error \r\n");
        return -1;
    }

    if (0 == change_type) /* 以前端上报为准，内存到数据库的比较 */
    {
        if (('\0' != pNewGBLogicDeviceInfo->device_name[0]
             && (0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IP Camera", 9)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPCamera", 8)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"Camera", 6)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPC", 3)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"通道", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"固枪", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"动球", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"半球", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_id, MAX_ID_LEN)))
            || '\0' == pNewGBLogicDeviceInfo->device_name[0])
        {
            /* 如果前端上报的是 IP Camera，Camera，的名称，或者上报的为空，则不更新，以数据库配置为准 */
        }
        else
        {
            /* 点位名称 */
            if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
            {
                memset(pOldGBLogicDeviceInfo->device_name, 0, MAX_128CHAR_STRING_LEN + 4);
                osip_strncpy(pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name, MAX_128CHAR_STRING_LEN);
            }
        }

        /* 是否启用 */
        pOldGBLogicDeviceInfo->enable = pNewGBLogicDeviceInfo->enable;

        /* 报警输入输出通道，需要更新报警设备子类型 */
        pOldGBLogicDeviceInfo->alarm_device_sub_type = pNewGBLogicDeviceInfo->alarm_device_sub_type;

        /* 是否属于其他域名 */
        pOldGBLogicDeviceInfo->other_realm = pNewGBLogicDeviceInfo->other_realm;

        /* 是否可控，如果前端上报的大于0,则以前端上报的为准 */
        if (pNewGBLogicDeviceInfo->ctrl_enable > 0)
        {
            pOldGBLogicDeviceInfo->ctrl_enable = pNewGBLogicDeviceInfo->ctrl_enable;
        }
    }

    if (1 == change_type) /* 以数据库为准，数据库到内存的更新 */
    {
        /* 点位名称 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
        {
            memset(pOldGBLogicDeviceInfo->device_name, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name, MAX_128CHAR_STRING_LEN);
        }

        /* 是否启用 */
        pOldGBLogicDeviceInfo->enable = pNewGBLogicDeviceInfo->enable;

        /* 是否可控 */
        pOldGBLogicDeviceInfo->ctrl_enable = pNewGBLogicDeviceInfo->ctrl_enable;
        pOldGBLogicDeviceInfo->mic_enable = pNewGBLogicDeviceInfo->mic_enable;
        pOldGBLogicDeviceInfo->frame_count = pNewGBLogicDeviceInfo->frame_count;
        pOldGBLogicDeviceInfo->alarm_duration = pNewGBLogicDeviceInfo->alarm_duration;
    }

    if (NULL != pGBDeviceInfo
        && EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
        && 0 == pGBDeviceInfo->three_party_flag) /* 如果是下级本身平台的点位，则需要比较禁用标识*/
    {
        pOldGBLogicDeviceInfo->mic_enable = pNewGBLogicDeviceInfo->mic_enable;
        pOldGBLogicDeviceInfo->frame_count = pNewGBLogicDeviceInfo->frame_count;
        pOldGBLogicDeviceInfo->alarm_duration = pNewGBLogicDeviceInfo->alarm_duration;
    }

    /* 是否支持多码流 */
    if (1 == change_type) /* 以数据库为准，数据库到内存的更新 */
    {
        pOldGBLogicDeviceInfo->stream_count = pNewGBLogicDeviceInfo->stream_count;
    }

    if (NULL != pGBDeviceInfo
        && ((EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type && 0 == pGBDeviceInfo->three_party_flag)
            || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)) /* 如果是下级平台的点位，则需要比较是否支持多码流标识*/
    {
        pOldGBLogicDeviceInfo->stream_count = pNewGBLogicDeviceInfo->stream_count;
    }

    if (0 == change_type) /* 以内存为准，内存到数据库的更新 */
    {
        /* 对应的媒体物理设备索引 */
        pOldGBLogicDeviceInfo->phy_mediaDeviceIndex = pNewGBLogicDeviceInfo->phy_mediaDeviceIndex;
    }

    if (1 == change_type) /* 以数据库为准，数据库到内存的更新 */
    {
        /* 对应的媒体物理设备通道 */
        pOldGBLogicDeviceInfo->phy_mediaDeviceChannel = pNewGBLogicDeviceInfo->phy_mediaDeviceChannel;

        /* 对应的控制物理设备索引 */
        pOldGBLogicDeviceInfo->phy_ctrlDeviceIndex = pNewGBLogicDeviceInfo->phy_ctrlDeviceIndex;

        /* 对应的控制物理设备通道 */
        pOldGBLogicDeviceInfo->phy_ctrlDeviceChannel = pNewGBLogicDeviceInfo->phy_ctrlDeviceChannel;
    }

    if (0 == change_type) /* 以前端上报为准，内存到数据库的比较 */
    {
        /* 设备生产商 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->manufacturer, pOldGBLogicDeviceInfo->manufacturer))
        {
            memset(pOldGBLogicDeviceInfo->manufacturer, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->manufacturer, pNewGBLogicDeviceInfo->manufacturer, MAX_128CHAR_STRING_LEN);
        }

        /* 设备型号 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->model, pOldGBLogicDeviceInfo->model))
        {
            memset(pOldGBLogicDeviceInfo->model, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->model, pNewGBLogicDeviceInfo->model, MAX_128CHAR_STRING_LEN);
        }

        /* 设备归属 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->owner, pOldGBLogicDeviceInfo->owner))
        {
            memset(pOldGBLogicDeviceInfo->owner, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->owner, pNewGBLogicDeviceInfo->owner, MAX_128CHAR_STRING_LEN);
        }

#if 0

        /* 行政区域 */
        if (NULL != pNewGBLogicDeviceInfo->civil_code && NULL != pOldGBLogicDeviceInfo->civil_code
            && 0 != sstrcmp(pNewGBLogicDeviceInfo->civil_code, pOldGBLogicDeviceInfo->civil_code))
        {
            osip_free(pOldGBLogicDeviceInfo->civil_code);
            pOldGBLogicDeviceInfo->civil_code = NULL;
            pOldGBLogicDeviceInfo->civil_code = osip_getcopy(pNewGBLogicDeviceInfo->civil_code);
        }
        else if (NULL == pNewGBLogicDeviceInfo->civil_code && NULL != pOldGBLogicDeviceInfo->civil_code)
        {
            osip_free(pOldGBLogicDeviceInfo->civil_code);
            pOldGBLogicDeviceInfo->civil_code = NULL;
        }
        else if (NULL != pNewGBLogicDeviceInfo->civil_code && NULL == pOldGBLogicDeviceInfo->civil_code)
        {
            pOldGBLogicDeviceInfo->civil_code = osip_getcopy(pNewGBLogicDeviceInfo->civil_code);
        }

#endif

        /* 警区 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->block, pOldGBLogicDeviceInfo->block))
        {
            memset(pOldGBLogicDeviceInfo->block, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->block, pNewGBLogicDeviceInfo->block, MAX_128CHAR_STRING_LEN);
        }

        /* 安装地址 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->address, pOldGBLogicDeviceInfo->address))
        {
            memset(pOldGBLogicDeviceInfo->address, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->address, pNewGBLogicDeviceInfo->address, MAX_128CHAR_STRING_LEN);
        }

        /* 是否有子设备 */
        pOldGBLogicDeviceInfo->parental = pNewGBLogicDeviceInfo->parental;

        /* 父设备/区域/系统ID */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->parentID, pOldGBLogicDeviceInfo->parentID))
        {
            memset(pOldGBLogicDeviceInfo->parentID, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->parentID, pNewGBLogicDeviceInfo->parentID, MAX_128CHAR_STRING_LEN);
        }

        /* 信令安全模式*/
        pOldGBLogicDeviceInfo->safety_way = pNewGBLogicDeviceInfo->safety_way;

        /* 注册方式 */
        pOldGBLogicDeviceInfo->register_way = pNewGBLogicDeviceInfo->register_way;

        /* 证书序列号*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->cert_num, pOldGBLogicDeviceInfo->cert_num))
        {
            memset(pOldGBLogicDeviceInfo->cert_num, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->cert_num, pNewGBLogicDeviceInfo->cert_num, MAX_128CHAR_STRING_LEN);
        }

        /* 证书有效标识 */
        pOldGBLogicDeviceInfo->certifiable = pNewGBLogicDeviceInfo->certifiable;

        /* 无效原因码 */
        pOldGBLogicDeviceInfo->error_code = pNewGBLogicDeviceInfo->error_code;

        /* 证书终止有效期*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->end_time, pOldGBLogicDeviceInfo->end_time))
        {
            memset(pOldGBLogicDeviceInfo->end_time, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->end_time, pNewGBLogicDeviceInfo->end_time, MAX_128CHAR_STRING_LEN);
        }

        /* 保密属性 */
        pOldGBLogicDeviceInfo->secrecy = pNewGBLogicDeviceInfo->secrecy;

        /* IP地址*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->ip_address, pOldGBLogicDeviceInfo->ip_address))
        {
            memset(pOldGBLogicDeviceInfo->ip_address, 0, MAX_IP_LEN);
            osip_strncpy(pOldGBLogicDeviceInfo->ip_address, pNewGBLogicDeviceInfo->ip_address, MAX_IP_LEN);
        }

        /* 端口号*/
        pOldGBLogicDeviceInfo->port = pNewGBLogicDeviceInfo->port;

        /* 密码*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->password, pOldGBLogicDeviceInfo->password))
        {
            memset(pOldGBLogicDeviceInfo->password, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->password, pNewGBLogicDeviceInfo->password, MAX_128CHAR_STRING_LEN);
        }

        /* 点位状态 */
        pOldGBLogicDeviceInfo->status = pNewGBLogicDeviceInfo->status;
    }

    if (1 == change_type) /* 以数据库为准，数据库到内存的更新 */
    {
        /* 经度 */
        if (pNewGBLogicDeviceInfo->longitude != pOldGBLogicDeviceInfo->longitude)
        {
            pOldGBLogicDeviceInfo->longitude = pNewGBLogicDeviceInfo->longitude;
        }

        /* 纬度 */
        if (pNewGBLogicDeviceInfo->latitude != pOldGBLogicDeviceInfo->latitude)
        {
            pOldGBLogicDeviceInfo->latitude = pNewGBLogicDeviceInfo->latitude;
        }

        /*  所属的图层 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->map_layer, pOldGBLogicDeviceInfo->map_layer))
        {
            memset(pOldGBLogicDeviceInfo->map_layer, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->map_layer, pNewGBLogicDeviceInfo->map_layer, MAX_128CHAR_STRING_LEN);
        }

        /* 鹰眼相机对应的预案ID */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->strResved2, pOldGBLogicDeviceInfo->strResved2))
        {
            memset(pOldGBLogicDeviceInfo->strResved2, 0, MAX_32CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->strResved2, pNewGBLogicDeviceInfo->strResved2, MAX_32CHAR_STRING_LEN);
        }
    }

    if (NULL != pGBDeviceInfo
        && EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
        && 0 == pGBDeviceInfo->three_party_flag) /* 如果是下级平台的点位，则需要比较经纬度 */
    {
        /* 经度 */
        if (pNewGBLogicDeviceInfo->longitude > 0 && pNewGBLogicDeviceInfo->longitude != pOldGBLogicDeviceInfo->longitude)
        {
            pOldGBLogicDeviceInfo->longitude = pNewGBLogicDeviceInfo->longitude;
        }

        /* 纬度 */
        if (pNewGBLogicDeviceInfo->latitude > 0 && pNewGBLogicDeviceInfo->latitude != pOldGBLogicDeviceInfo->latitude)
        {
            pOldGBLogicDeviceInfo->latitude = pNewGBLogicDeviceInfo->latitude;
        }

        /*  所属的图层 */
        if (pNewGBLogicDeviceInfo->map_layer[0] != '\0' && 0 != sstrcmp(pNewGBLogicDeviceInfo->map_layer, pOldGBLogicDeviceInfo->map_layer))
        {
            memset(pOldGBLogicDeviceInfo->map_layer, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->map_layer, pNewGBLogicDeviceInfo->map_layer, MAX_128CHAR_STRING_LEN);
        }
    }

    /* 类型 */
    pOldGBLogicDeviceInfo->device_type = pNewGBLogicDeviceInfo->device_type;

#if 0
    /* 索引*/
    pOldGBLogicDeviceInfo->id = pNewGBLogicDeviceInfo->id;
#endif

    /*  所属的CMSID */
    if (0 != sstrcmp(pNewGBLogicDeviceInfo->cms_id, pOldGBLogicDeviceInfo->cms_id))
    {
        memset(pOldGBLogicDeviceInfo->cms_id, 0, MAX_ID_LEN + 4);
        osip_strncpy(pOldGBLogicDeviceInfo->cms_id, pNewGBLogicDeviceInfo->cms_id, MAX_ID_LEN);
    }

    /* 录像类型*/
    if (1 == change_type) /* 以数据库为准，数据库到内存的更新 */
    {
        pOldGBLogicDeviceInfo->record_type = pNewGBLogicDeviceInfo->record_type;
    }

#if 1/*2016.10.10 add for RCU*/

    if (0 == change_type) /* 内存的更新 */
    {
        /* RCU Value */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->Value, pOldGBLogicDeviceInfo->Value))
        {
            memset(pOldGBLogicDeviceInfo->Value, 0, EV9000_LONG_LONG_STRING_LEN);
            osip_strncpy(pOldGBLogicDeviceInfo->Value, pNewGBLogicDeviceInfo->Value, 256);
        }

        /* RCU Unit */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->Unit, pOldGBLogicDeviceInfo->Unit))
        {
            memset(pOldGBLogicDeviceInfo->Unit, 0, EV9000_SHORT_STRING_LEN);
            osip_strncpy(pOldGBLogicDeviceInfo->Unit, pNewGBLogicDeviceInfo->Unit, 32);
        }

        /* RCU AlarmPriority */
        pOldGBLogicDeviceInfo->AlarmPriority = pNewGBLogicDeviceInfo->AlarmPriority;

        /* RCU Guard */
        pOldGBLogicDeviceInfo->guard_type = pNewGBLogicDeviceInfo->guard_type;
    }

#endif/*2016.10.10 add for RCU*/

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDevice_info_update() GBDevice_add:device_id=%s, stream_count=%d \r\n", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->stream_count);

    if (pOldGBLogicDeviceInfo->stream_count > 1) /* 增加从码流物理设备 */
    {
        /* 从流类型设备 */
        pSlaveGBDevice = GBDevice_get_by_stream_type(pOldGBLogicDeviceInfo, EV9000_STREAM_TYPE_SLAVE);

        if (NULL != pSlaveGBDevice) /* 已经存在，比较看是否一样 */
        {
            pSlaveGBDeviceInfo = pSlaveGBDevice->ptGBDeviceInfo;

            if (NULL != pSlaveGBDeviceInfo)
            {
                if (pSlaveGBDeviceInfo != pGBDeviceInfo)
                {
                    pSlaveGBDevice->ptGBDeviceInfo = pGBDeviceInfo;
                }
            }
            else
            {
                pSlaveGBDevice->ptGBDeviceInfo = pGBDeviceInfo;
            }
        }
        else /* 不存在，直接添加 */
        {
            i = GBDevice_add(pOldGBLogicDeviceInfo, EV9000_STREAM_TYPE_SLAVE, pGBDeviceInfo);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDevice_info_update() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_SLAVE Error:i=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GBLogicDevice_info_update() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_SLAVE OK:i=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
            }

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端设备点位更新:逻辑设备ID=%s, 逻辑点位名称=%s, 增加从流物理设备", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front-end logic device info update:device ID=%s, logic point name=%s, add slave stream device", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name);
        }
    }
    else if (pOldGBLogicDeviceInfo->stream_count <= 1)/* 去掉从码流物理设备 */
    {
        /* 从流类型设备 */
        pSlaveGBDevice = GBDevice_get_by_stream_type(pOldGBLogicDeviceInfo, EV9000_STREAM_TYPE_SLAVE);

        if (NULL != pSlaveGBDevice) /* 已经存在 */
        {
            i = GBDevice_remove(pOldGBLogicDeviceInfo, EV9000_STREAM_TYPE_SLAVE);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDevice_info_update() GBDevice_remove:device_id=%s, stream_type=EV9000_STREAM_TYPE_SLAVE Error:i=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GBLogicDevice_info_update() GBDevice_remove:device_id=%s, stream_type=EV9000_STREAM_TYPE_SLAVE OK:i=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
            }

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端设备点位更新:逻辑设备ID=%s, 逻辑点位名称=%s, 删除从流物理设备", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front-end logic device info update:device ID=%s, logic point name=%s, remove slave stream device", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : GBLogicDevice_info_update_for_route
 功能描述  : 更新标准逻辑设备信息
 输入参数  : GBLogicDevice_info_t* pOldGBLogicDeviceInfo
             GBLogicDevice_info_t* pNewGBLogicDeviceInfo
             int change_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月10日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GBLogicDevice_info_update_for_route(GBLogicDevice_info_t* pOldGBLogicDeviceInfo, GBLogicDevice_info_t* pNewGBLogicDeviceInfo, int change_type)
{
    if (NULL == pOldGBLogicDeviceInfo || NULL == pNewGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDevice_info_update() exit---: Param Error \r\n");
        return -1;
    }

    if (0 == change_type) /* 以前端上报为准，内存到数据库的比较 */
    {
        if (('\0' != pNewGBLogicDeviceInfo->device_name[0]
             && (0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IP Camera", 9)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPCamera", 8)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"Camera", 6)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPC", 3)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"通道", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"固枪", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"动球", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"半球", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_id, MAX_ID_LEN)))
            || '\0' == pNewGBLogicDeviceInfo->device_name[0])
        {
            /* 如果前端上报的是 IP Camera，Camera，的名称，或者上报的为空，则不更新，以数据库配置为准 */
        }
        else
        {
            /* 点位名称 */
            if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
            {
                memset(pOldGBLogicDeviceInfo->device_name, 0, MAX_128CHAR_STRING_LEN + 4);
                osip_strncpy(pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name, MAX_128CHAR_STRING_LEN);
            }
        }

        /* 是否启用 */
        pOldGBLogicDeviceInfo->enable = pNewGBLogicDeviceInfo->enable;

        /* 报警输入输出通道，需要更新报警设备子类型 */
        pOldGBLogicDeviceInfo->alarm_device_sub_type = pNewGBLogicDeviceInfo->alarm_device_sub_type;

        /* 是否属于其他域名 */
        pOldGBLogicDeviceInfo->other_realm = pNewGBLogicDeviceInfo->other_realm;
    }

    if (1 == change_type) /* 以数据库为准，数据库到内存的更新 */
    {
        /* 点位名称 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
        {
            memset(pOldGBLogicDeviceInfo->device_name, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name, MAX_128CHAR_STRING_LEN);
        }
    }

    if (0 == change_type) /* 以前端上报为准，内存到数据库的比较 */
    {
        pOldGBLogicDeviceInfo->ctrl_enable = pNewGBLogicDeviceInfo->ctrl_enable;
        pOldGBLogicDeviceInfo->mic_enable = pNewGBLogicDeviceInfo->mic_enable;
        pOldGBLogicDeviceInfo->frame_count = pNewGBLogicDeviceInfo->frame_count;
        pOldGBLogicDeviceInfo->alarm_duration = pNewGBLogicDeviceInfo->alarm_duration;
        pOldGBLogicDeviceInfo->stream_count = pNewGBLogicDeviceInfo->stream_count;
        /* 对应的媒体物理设备索引 */
        pOldGBLogicDeviceInfo->phy_mediaDeviceIndex = pNewGBLogicDeviceInfo->phy_mediaDeviceIndex;
    }

    if (1 == change_type) /* 以数据库为准，数据库到内存的更新 */
    {
        /* 对应的媒体物理设备通道 */
        pOldGBLogicDeviceInfo->phy_mediaDeviceChannel = pNewGBLogicDeviceInfo->phy_mediaDeviceChannel;

        /* 对应的控制物理设备索引 */
        pOldGBLogicDeviceInfo->phy_ctrlDeviceIndex = pNewGBLogicDeviceInfo->phy_ctrlDeviceIndex;

        /* 对应的控制物理设备通道 */
        pOldGBLogicDeviceInfo->phy_ctrlDeviceChannel = pNewGBLogicDeviceInfo->phy_ctrlDeviceChannel;
    }

    if (0 == change_type) /* 以前端上报为准，内存到数据库的比较 */
    {
        /* 设备生产商 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->manufacturer, pOldGBLogicDeviceInfo->manufacturer))
        {
            memset(pOldGBLogicDeviceInfo->manufacturer, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->manufacturer, pNewGBLogicDeviceInfo->manufacturer, MAX_128CHAR_STRING_LEN);
        }

        /* 设备型号 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->model, pOldGBLogicDeviceInfo->model))
        {
            memset(pOldGBLogicDeviceInfo->model, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->model, pNewGBLogicDeviceInfo->model, MAX_128CHAR_STRING_LEN);
        }

        /* 设备归属 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->owner, pOldGBLogicDeviceInfo->owner))
        {
            memset(pOldGBLogicDeviceInfo->owner, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->owner, pNewGBLogicDeviceInfo->owner, MAX_128CHAR_STRING_LEN);
        }

#if 0

        /* 行政区域 */
        if (NULL != pNewGBLogicDeviceInfo->civil_code && NULL != pOldGBLogicDeviceInfo->civil_code
            && 0 != sstrcmp(pNewGBLogicDeviceInfo->civil_code, pOldGBLogicDeviceInfo->civil_code))
        {
            osip_free(pOldGBLogicDeviceInfo->civil_code);
            pOldGBLogicDeviceInfo->civil_code = NULL;
            pOldGBLogicDeviceInfo->civil_code = osip_getcopy(pNewGBLogicDeviceInfo->civil_code);
        }
        else if (NULL == pNewGBLogicDeviceInfo->civil_code && NULL != pOldGBLogicDeviceInfo->civil_code)
        {
            osip_free(pOldGBLogicDeviceInfo->civil_code);
            pOldGBLogicDeviceInfo->civil_code = NULL;
        }
        else if (NULL != pNewGBLogicDeviceInfo->civil_code && NULL == pOldGBLogicDeviceInfo->civil_code)
        {
            pOldGBLogicDeviceInfo->civil_code = osip_getcopy(pNewGBLogicDeviceInfo->civil_code);
        }

#endif

        /* 警区 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->block, pOldGBLogicDeviceInfo->block))
        {
            memset(pOldGBLogicDeviceInfo->block, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->block, pNewGBLogicDeviceInfo->block, MAX_128CHAR_STRING_LEN);
        }

        /* 安装地址 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->address, pOldGBLogicDeviceInfo->address))
        {
            memset(pOldGBLogicDeviceInfo->address, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->address, pNewGBLogicDeviceInfo->address, MAX_128CHAR_STRING_LEN);
        }

        /* 是否有子设备 */
        pOldGBLogicDeviceInfo->parental = pNewGBLogicDeviceInfo->parental;

        /* 父设备/区域/系统ID */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->parentID, pOldGBLogicDeviceInfo->parentID))
        {
            memset(pOldGBLogicDeviceInfo->parentID, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->parentID, pNewGBLogicDeviceInfo->parentID, MAX_128CHAR_STRING_LEN);
        }

        /* 信令安全模式*/
        pOldGBLogicDeviceInfo->safety_way = pNewGBLogicDeviceInfo->safety_way;

        /* 注册方式 */
        pOldGBLogicDeviceInfo->register_way = pNewGBLogicDeviceInfo->register_way;

        /* 证书序列号*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->cert_num, pOldGBLogicDeviceInfo->cert_num))
        {
            memset(pOldGBLogicDeviceInfo->cert_num, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->cert_num, pNewGBLogicDeviceInfo->cert_num, MAX_128CHAR_STRING_LEN);
        }

        /* 证书有效标识 */
        pOldGBLogicDeviceInfo->certifiable = pNewGBLogicDeviceInfo->certifiable;

        /* 无效原因码 */
        pOldGBLogicDeviceInfo->error_code = pNewGBLogicDeviceInfo->error_code;

        /* 证书终止有效期*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->end_time, pOldGBLogicDeviceInfo->end_time))
        {
            memset(pOldGBLogicDeviceInfo->end_time, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->end_time, pNewGBLogicDeviceInfo->end_time, MAX_128CHAR_STRING_LEN);
        }

        /* 保密属性 */
        pOldGBLogicDeviceInfo->secrecy = pNewGBLogicDeviceInfo->secrecy;

        /* IP地址*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->ip_address, pOldGBLogicDeviceInfo->ip_address))
        {
            memset(pOldGBLogicDeviceInfo->ip_address, 0, MAX_IP_LEN);
            osip_strncpy(pOldGBLogicDeviceInfo->ip_address, pNewGBLogicDeviceInfo->ip_address, MAX_IP_LEN);
        }

        /* 端口号*/
        pOldGBLogicDeviceInfo->port = pNewGBLogicDeviceInfo->port;

        /* 密码*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->password, pOldGBLogicDeviceInfo->password))
        {
            memset(pOldGBLogicDeviceInfo->password, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->password, pNewGBLogicDeviceInfo->password, MAX_128CHAR_STRING_LEN);
        }

        /* 点位状态 */
        pOldGBLogicDeviceInfo->status = pNewGBLogicDeviceInfo->status;
    }

    if (0 == change_type) /* 以前端上报为准，内存到数据库的比较 */
    {
        /* 经度 */
        if (pNewGBLogicDeviceInfo->longitude > 0 && pNewGBLogicDeviceInfo->longitude != pOldGBLogicDeviceInfo->longitude)
        {
            pOldGBLogicDeviceInfo->longitude = pNewGBLogicDeviceInfo->longitude;
        }

        /* 纬度 */
        if (pNewGBLogicDeviceInfo->latitude > 0 && pNewGBLogicDeviceInfo->latitude != pOldGBLogicDeviceInfo->latitude)
        {
            pOldGBLogicDeviceInfo->latitude = pNewGBLogicDeviceInfo->latitude;
        }

        /*  所属的图层 */
        if (pNewGBLogicDeviceInfo->map_layer[0] != '\0' && 0 != sstrcmp(pNewGBLogicDeviceInfo->map_layer, pOldGBLogicDeviceInfo->map_layer))
        {
            memset(pOldGBLogicDeviceInfo->map_layer, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->map_layer, pNewGBLogicDeviceInfo->map_layer, MAX_128CHAR_STRING_LEN);
        }
    }

    /* 类型 */
    pOldGBLogicDeviceInfo->device_type = pNewGBLogicDeviceInfo->device_type;

#if 0
    /* 索引*/
    pOldGBLogicDeviceInfo->id = pNewGBLogicDeviceInfo->id;
#endif

    /*  所属的CMSID */
    if (0 != sstrcmp(pNewGBLogicDeviceInfo->cms_id, pOldGBLogicDeviceInfo->cms_id))
    {
        memset(pOldGBLogicDeviceInfo->cms_id, 0, MAX_ID_LEN + 4);
        osip_strncpy(pOldGBLogicDeviceInfo->cms_id, pNewGBLogicDeviceInfo->cms_id, MAX_ID_LEN);
    }

    /* 录像类型*/
    if (1 == change_type) /* 以数据库为准，数据库到内存的更新 */
    {
        pOldGBLogicDeviceInfo->record_type = pNewGBLogicDeviceInfo->record_type;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : Get_GBLogicDevice_Index_By_Device_ID
 功能描述  : 根据逻辑设备的ID获取物理设备的索引
 输入参数  : char* device_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月22日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
unsigned int Get_GBLogicDevice_Index_By_Device_ID(char* device_id)
{
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == device_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "Get_GBLogicDevice_Index_By_Device_ID() exit---: Param Error \r\n");
        return 0;
    }

    pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        return pGBLogicDeviceInfo->id;
    }
    else
    {
        return 0;
    }
}

/*****************************************************************************
 函 数 名  : AddAllGBLogicDeviceIDToVectorForDevice
 功能描述  : 添加所有逻辑设备的ID 信息到容器中
 输入参数  : vector<string>& DeviceIDVector
             int device_type
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月10日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddAllGBLogicDeviceIDToVectorForDevice(vector<string>& DeviceIDVector, int device_type, DBOper* ptDBoper)
{
    int record_count = 0;
    string strSQL = "";
    char strDeviceType[16] = {0};
    int while_count = 0;

    if (NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "AddAllGBLogicDeviceIDToVectorForDevice() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select DeviceID from GBLogicDeviceConfig WHERE DeviceType <> ";
    snprintf(strDeviceType, 16, "%d", EV9000_DEVICETYPE_SCREEN);
    strSQL += strDeviceType;

    /* 如果是智能分析和智能诊断设备，过滤掉报警点位 */
    if (EV9000_DEVICETYPE_VIDEODIAGNOSIS == device_type
        || EV9000_DEVICETYPE_INTELLIGENTANALYSIS == device_type)
    {
        strSQL += " && DeviceType <> 134 && DeviceType <> 135";
    }

    strSQL += " && Enable=1 order by DeviceName asc";

    record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddAllGBLogicDeviceIDToVectorForDevice() record_countCount=%d \r\n", record_count);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForDevice() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForDevice() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count > 0)
    {
        //将DeviceIndex加入容器
        do
        {
            string tmp_svalue = "";

            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "AddAllGBLogicDeviceIDToVectorForDevice() While Count=%d \r\n", while_count);
            }

            tmp_svalue.clear();
            ptDBoper->GetFieldValue(0, tmp_svalue);

            if (tmp_svalue.empty())
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForDevice() DeviceIndex Error \r\n");
                continue;
            }

            //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddAllGBLogicDeviceIDToVector() DeviceIndex=%u \r\n", tmp_uivalue);
            //DeviceIDVector.push_back(tmp_svalue);
            AddDeviceIndexToDeviceIDVector(DeviceIDVector, tmp_svalue);
        }
        while (ptDBoper->MoveNext() >= 0);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : AddAllGBLogicDeviceIDToVectorForRoute
 功能描述  : 级联点位上报添加所有逻辑设备的ID 信息到容器中
 输入参数  : vector<string>& DeviceIDVector
             int route_index
             int iThreePartyFlag
             int link_type
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月22日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddAllGBLogicDeviceIDToVectorForRoute(vector<string>& DeviceIDVector, int route_index, int iThreePartyFlag, int link_type, DBOper* ptDBoper)
{
    int record_count = 0;
    string strSQL = "";
    char strRouteIndex[16] = {0};
    char strDeviceType[16] = {0};
    int while_count = 0;

    if (NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "AddAllGBLogicDeviceIDToVectorForRoute() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();

    if (iThreePartyFlag) /* 第三方平台 */
    {
        strSQL.clear();
        strSQL = "select GDC.DeviceID from GBLogicDeviceConfig as GDC, RouteDevicePermConfig as RDPC WHERE GDC.ID = RDPC.DeviceIndex and GDC.Enable=1 and RDPC.RouteIndex = "; /* 查询权限表 */
        snprintf(strRouteIndex, 16, "%u", route_index);
        strSQL += strRouteIndex;

        record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddAllGBLogicDeviceIDToVectorForRoute() record_count=%d, route_index=%d \r\n", record_count, route_index);

        if (record_count <= 0) /* 兼容原来的, 获取所有的逻辑点位 */
        {
            strSQL = "select DeviceID from GBLogicDeviceConfig WHERE Enable = 1 AND OtherRealm = 0 AND DeviceType <> ";
            snprintf(strDeviceType, 16, "%d", EV9000_DEVICETYPE_SCREEN);
            strSQL += strDeviceType;
            strSQL += " order by DeviceName asc";

            record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddAllGBLogicDeviceIDToVectorForRoute() record_count=%d, route_index=%d \r\n", record_count, route_index);

            if (record_count < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForRoute() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForRoute() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
                return -1;
            }
        }
    }
    else
    {
        if (1 == link_type) /* 同级的情况下，需要上报电视墙通道,权限不生效 */
        {
            strSQL = "select DeviceID from GBLogicDeviceConfig WHERE OtherRealm = 0 order by DeviceName asc";

            record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddAllGBLogicDeviceIDToVectorForRoute() record_count=%d, route_index=%d \r\n", record_count, route_index);

            if (record_count < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForRoute() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForRoute() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
                return -1;
            }
        }
        else
        {
            strSQL.clear();
            strSQL = "select GDC.DeviceID from GBLogicDeviceConfig as GDC, RouteDevicePermConfig as RDPC WHERE GDC.ID = RDPC.DeviceIndex and RDPC.RouteIndex = "; /* 查询权限表 */
            snprintf(strRouteIndex, 16, "%u", route_index);
            strSQL += strRouteIndex;

            record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddAllGBLogicDeviceIDToVectorForRoute() record_count=%d, route_index=%d \r\n", record_count, route_index);

            if (record_count <= 0) /* 兼容原来的, 获取所有的逻辑点位 */
            {
                strSQL = "select DeviceID from GBLogicDeviceConfig WHERE OtherRealm = 0 AND DeviceType <> ";
                snprintf(strDeviceType, 16, "%d", EV9000_DEVICETYPE_SCREEN);
                strSQL += strDeviceType;
                strSQL += " order by DeviceName asc";

                record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddAllGBLogicDeviceIDToVectorForRoute() record_count=%d, route_index=%d \r\n", record_count, route_index);

                if (record_count < 0)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForRoute() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForRoute() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
                    return -1;
                }
            }
        }
    }

    if (record_count > 0)
    {
        //将DeviceIndex加入容器
        do
        {
            string tmp_svalue = "";

            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "AddAllGBLogicDeviceIDToVectorForRoute() While Count=%d \r\n", while_count);
            }

            tmp_svalue.clear();
            ptDBoper->GetFieldValue(0, tmp_svalue);

            if (tmp_svalue.empty())
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForRoute() DeviceID Error \r\n");
                continue;
            }

            //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddAllGBLogicDeviceIDToVector() DeviceIndex=%u \r\n", tmp_uivalue);
            //DeviceIDVector.push_back(tmp_svalue);
            AddDeviceIndexToDeviceIDVector(DeviceIDVector, tmp_svalue);
        }
        while (ptDBoper->MoveNext() >= 0);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : AddAllGBLogicDeviceIDToVectorForUser
 功能描述  : 用户获取点位的时候添加所有点位DeviceID到容器
 输入参数  : vector<string>& DeviceIDVector
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月25日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddAllGBLogicDeviceIDToVectorForUser(vector<string>& DeviceIDVector, DBOper* ptDBoper)
{
    int record_count = 0;
    string strSQL = "";
    int while_count = 0;

    if (NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "AddAllGBLogicDeviceIDToVectorForUser() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select DeviceID from GBLogicDeviceConfig WHERE Enable = 1 and (DeviceType < 180 or DeviceType > 185) order by DeviceName asc";

    record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddAllGBLogicDeviceIDToVectorForUser() record_countCount=%d \r\n", record_count);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForUser() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForUser() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count > 0)
    {
        //将DeviceIndex加入容器
        do
        {
            string tmp_svalue = "";

            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "AddAllGBLogicDeviceIDToVectorForUser() While Count=%d \r\n", while_count);
            }

            tmp_svalue.clear();
            ptDBoper->GetFieldValue(0, tmp_svalue);

            if (tmp_svalue.empty())
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForUser() DeviceIndex Error \r\n");
                continue;
            }

            //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddAllGBLogicDeviceIDToVector() DeviceIndex=%u \r\n", tmp_uivalue);
            //DeviceIDVector.push_back(tmp_svalue);
            AddDeviceIndexToDeviceIDVector(DeviceIDVector, tmp_svalue);
        }
        while (ptDBoper->MoveNext() >= 0);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : AddRCUGBLogicDeviceIDToVectorForUser
 功能描述  : 用户获取RCU点位的时候添加所有RCU点位DeviceID到容器
 输入参数  : vector<string>& DeviceIDVector
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月25日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddAllRCUGBLogicDeviceIDToVectorForUser(vector<string>& DeviceIDVector, DBOper* ptDBoper)
{
    int record_count = 0;
    string strSQL = "";
    int while_count = 0;

    if (NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "AddAllRCUGBLogicDeviceIDToVectorForUser() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select DeviceID from GBLogicDeviceConfig WHERE Enable = 1 and DeviceType >= 180 and DeviceType <= 185 order by DeviceName asc";

    record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddAllRCUGBLogicDeviceIDToVectorForUser() record_countCount=%d \r\n", record_count);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllRCUGBLogicDeviceIDToVectorForUser() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllRCUGBLogicDeviceIDToVectorForUser() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count > 0)
    {
        //将DeviceIndex加入容器
        do
        {
            string tmp_svalue = "";

            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "AddAllGBLogicDeviceIDToVectorForUser() While Count=%d \r\n", while_count);
            }

            tmp_svalue.clear();
            ptDBoper->GetFieldValue(0, tmp_svalue);

            if (tmp_svalue.empty())
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForUser() DeviceIndex Error \r\n");
                continue;
            }

            //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddAllGBLogicDeviceIDToVector() DeviceIndex=%u \r\n", tmp_uivalue);
            //DeviceIDVector.push_back(tmp_svalue);
            AddDeviceIndexToDeviceIDVector(DeviceIDVector, tmp_svalue);
        }
        while (ptDBoper->MoveNext() >= 0);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : AddAllGBLogicDeviceIDToVectorForSubCMS
 功能描述  : 根据下级CMS的点位权限获取点位添加index到容器
 输入参数  : vector<string>& DeviceIDVector
             int device_index
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月9日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddAllGBLogicDeviceIDToVectorForSubCMS(vector<string>& DeviceIDVector, int device_index, DBOper* ptDBoper)
{
    int record_count = 0;
    string strSQL = "";
    char strDeviceIndex[16] = {0};
    int while_count = 0;

    if (NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "AddAllGBLogicDeviceIDToVectorForSubCMS() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select GDC.DeviceID from GBLogicDeviceConfig as GDC, GBPhyDevicePermConfig as GDPC WHERE GDC.ID = GDPC.DeviceIndex and GDPC.GBPhyDeviceIndex = "; /* 查询权限表 */
    snprintf(strDeviceIndex, 16, "%u", device_index);
    strSQL += strDeviceIndex;

    record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddAllGBLogicDeviceIDToVectorForSubCMS() record_count=%d, GBPhyDeviceIndex=%d \r\n", record_count, device_index);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForSubCMS() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForSubCMS() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "AddAllGBLogicDeviceIDToVectorForSubCMS() No Record Count:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        return 0;
    }

    if (record_count > 0)
    {
        //将DeviceIndex加入容器
        do
        {
            string tmp_svalue = "";

            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "AddAllGBLogicDeviceIDToVectorForSubCMS() While Count=%d \r\n", while_count);
            }

            tmp_svalue.clear();
            ptDBoper->GetFieldValue(0, tmp_svalue);

            if (tmp_svalue.empty())
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVectorForSubCMS() DeviceIndex Error \r\n");
                continue;
            }

            //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddAllGBLogicDeviceIDToVector() DeviceIndex=%u \r\n", tmp_uivalue);
            //DeviceIDVector.push_back(tmp_svalue);
            AddDeviceIndexToDeviceIDVector(DeviceIDVector, tmp_svalue);
        }
        while (ptDBoper->MoveNext() >= 0);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : AddAllGBLogicDeviceIDToVector
 功能描述  : 添加所有点位index到容器
 输入参数  : vector<string>& DeviceIDVector
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月23日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddAllGBLogicDeviceIDToVector(vector<string>& DeviceIDVector, DBOper* ptDBoper)
{
    int record_count = 0;
    string strSQL = "";
    int while_count = 0;

    if (NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "AddAllGBLogicDeviceIDToVector() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select DeviceID from GBLogicDeviceConfig order by DeviceName asc";

    record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddAllGBLogicDeviceIDToVector() record_countCount=%d \r\n", record_count);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVector() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVector() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count > 0)
    {
        //将DeviceIndex加入容器
        do
        {
            string tmp_svalue = "";

            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "AddAllGBLogicDeviceIDToVector() While Count=%d \r\n", while_count);
            }


            tmp_svalue.clear();
            ptDBoper->GetFieldValue(0, tmp_svalue);

            if (tmp_svalue.empty())
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddAllGBLogicDeviceIDToVector() DeviceIndex Error \r\n");
                continue;
            }

            //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddAllGBLogicDeviceIDToVector() DeviceIndex=%u \r\n", tmp_uivalue);
            //DeviceIDVector.push_back(tmp_svalue);
            AddDeviceIndexToDeviceIDVector(DeviceIDVector, tmp_svalue);
        }
        while (ptDBoper->MoveNext() >= 0);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SetGBLogicDeviceInfoDelFlag
 功能描述  : 根据物理设备的信息设置下面逻辑通道的删除标识
 输入参数  : GBDevice_info_t* pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年12月23日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SetGBLogicDeviceInfoDelFlag(GBDevice_info_t* pGBDeviceInfo)
{
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    GBDevice_info_t* pIntelligentGBDeviceInfo = NULL;

    if (pGBDeviceInfo == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SetLogicDeviceGroupChangeFlag() exit---: Param Error \r\n");
        return -1;
    }

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if (NULL == pGBLogicDeviceInfo)
        {
            continue;
        }

        if (pGBLogicDeviceInfo->enable == 0) /* 已经禁用的不需要重复设置 */
        {
            continue;
        }

        if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
        {
            pIntelligentGBDeviceInfo = GBDevice_info_get_by_stream_type2(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_INTELLIGENCE);

            if (NULL != pIntelligentGBDeviceInfo)
            {
                if (pIntelligentGBDeviceInfo->id == pGBDeviceInfo->id)
                {
                    pGBLogicDeviceInfo->del_mark = 2;
                }
            }
        }
        else
        {
            if (pGBLogicDeviceInfo->phy_mediaDeviceIndex == pGBDeviceInfo->id)
            {
                pGBLogicDeviceInfo->del_mark = 1;
            }
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : SetGBLogicDeviceInfoEnableFlagByDelMark
 功能描述  : 根据删除标识设置逻辑设备的禁用标识
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年12月23日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SetGBLogicDeviceInfoEnableFlagByDelMark(GBDevice_info_t* pGBDeviceInfo, DBOper* pDevice_Srv_dboper)
{
    int index = 0;
    int iRet = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    vector<string> IntelligentDeviceIDVector;
    vector<string> MasterDeviceIDVector;
    GBDevice_info_t* pIntelligentGBDeviceInfo = NULL;

    if (pGBDeviceInfo == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SetGBLogicDeviceInfoEnableFlagByDelMark() exit---: Param Error \r\n");
        return -1;
    }

    IntelligentDeviceIDVector.clear();
    MasterDeviceIDVector.clear();

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if (NULL == pGBLogicDeviceInfo)
        {
            continue;
        }

        if (pGBLogicDeviceInfo->enable == 0) /* 已经禁用的不需要重复设置 */
        {
            continue;
        }

        if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
        {
            pIntelligentGBDeviceInfo = GBDevice_info_get_by_stream_type2(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_INTELLIGENCE);

            if (NULL != pIntelligentGBDeviceInfo)
            {
                if (pIntelligentGBDeviceInfo->id == pGBDeviceInfo->id && pGBLogicDeviceInfo->del_mark == 2)
                {
                    pGBLogicDeviceInfo->intelligent_status = INTELLIGENT_STATUS_NULL;
                    IntelligentDeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
                }
            }
        }
        else
        {
            if (pGBLogicDeviceInfo->phy_mediaDeviceIndex == pGBDeviceInfo->id && pGBLogicDeviceInfo->del_mark == 1)
            {
                pGBLogicDeviceInfo->enable = 0;
                pGBLogicDeviceInfo->status = 0;
                MasterDeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
            }
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    if (IntelligentDeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)IntelligentDeviceIDVector.size(); index++)
        {
            pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)IntelligentDeviceIDVector[index].c_str());

            if (NULL == pGBLogicDeviceInfo)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMark() exit---: Get Device Info Error:device_id=%s \r\n", (char*)IntelligentDeviceIDVector[index].c_str());
                continue;
            }

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端设备目录查询响应消息:根据删除标识删除逻辑点位的智能分析标识, 逻辑设备ID=%s", pGBLogicDeviceInfo->device_id);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end device directory query response message:Delete logic point's intelligent analysis identification based on the deletion identification, logic device ID=%s", pGBLogicDeviceInfo->device_id);
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO,  "SetGBLogicDeviceInfoEnableFlagByDelMark(): Disable Device Intelligent Status: device_id=%s \r\n", pGBLogicDeviceInfo->device_id);

            /* 发送设备状态变化消息  */
            iRet = SendDeviceStatusMessageProc(pGBLogicDeviceInfo, pGBLogicDeviceInfo->status, pDevice_Srv_dboper);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMark() SendDeviceStatusMessageProc ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SetGBLogicDeviceInfoEnableFlagByDelMark() SendDeviceStatusMessageProc OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, iRet);
            }

            /* 发送设备状态告警消息给在线用户  */
            iRet = SendIntelligentDeviceOffLineAlarmToAllClientUser((char*)IntelligentDeviceIDVector[index].c_str());

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMark() SendIntelligentDeviceOffLineAlarmToAllClientUser Error:DeviceID=%s, iRet=%d \r\n", (char*)IntelligentDeviceIDVector[index].c_str(), iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SetGBLogicDeviceInfoEnableFlagByDelMark() SendIntelligentDeviceOffLineAlarmToAllClientUser OK:DeviceID=%s, iRet=%d \r\n", (char*)IntelligentDeviceIDVector[index].c_str(), iRet);
            }

            iRet = StopAllServiceTaskByLogicDeviceIDAndStreamType((char*)IntelligentDeviceIDVector[index].c_str(), EV9000_STREAM_TYPE_INTELLIGENCE);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMark() StopAllServiceTaskByLogicDeviceIDAndStreamType ERROR:device_id=%s, iRet=%d \r\n", (char*)IntelligentDeviceIDVector[index].c_str(), iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SetGBLogicDeviceInfoEnableFlagByDelMark() StopAllServiceTaskByLogicDeviceIDAndStreamType OK:device_id=%s, iRet=%d \r\n", (char*)IntelligentDeviceIDVector[index].c_str(), iRet);
            }

            iRet = GBDevice_remove(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_INTELLIGENCE);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMark() GBDevice_remove:device_id=%s, stream_type=EV9000_STREAM_TYPE_INTELLIGENCE Error:i=%d \r\n", pGBLogicDeviceInfo->device_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "SetGBLogicDeviceInfoEnableFlagByDelMark() GBDevice_remove:device_id=%s, stream_type=EV9000_STREAM_TYPE_INTELLIGENCE OK:i=%d \r\n", pGBLogicDeviceInfo->device_id, iRet);
            }
        }
    }

    IntelligentDeviceIDVector.clear();

    if (MasterDeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)MasterDeviceIDVector.size(); index++)
        {
            pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)MasterDeviceIDVector[index].c_str());

            if (NULL == pGBLogicDeviceInfo)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMark() exit---: Get Device Info Error:device_id=%s \r\n", (char*)IntelligentDeviceIDVector[index].c_str());
                continue;
            }

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端设备目录查询响应消息:根据删除标识设置逻辑设备的禁用标识, 逻辑设备ID=%s", pGBLogicDeviceInfo->device_id);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end device directory query response message:Set up the logical device's disabled identification based on the deletion identification,logic device ID=%s", pGBLogicDeviceInfo->device_id);
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO,  "SetGBLogicDeviceInfoEnableFlagByDelMark(): Disable Device: device_id=%s \r\n", pGBLogicDeviceInfo->device_id);

            /* 发送设备状态消息给在线用户  */
            iRet = SendDeviceStatusToAllClientUser((char*)MasterDeviceIDVector[index].c_str(), 0, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "SetGBLogicDeviceInfoEnableFlagByDelMark() SendDeviceStatusToAllClientUser Error:iRet=%d \r\n", iRet);
            }
            else if (iRet > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "SetGBLogicDeviceInfoEnableFlagByDelMark() SendDeviceStatusToAllClientUser OK:iRet=%d \r\n", iRet);
            }

            /* 发送Catalog变化通知事件消息  */
            iRet = SendNotifyCatalogMessageToAllRoute(pGBLogicDeviceInfo, 1, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "SetGBLogicDeviceInfoEnableFlagByDelMark() SendNotifyCatalogMessageToAllRoute Error:iRet=%d \r\n", iRet);
            }
            else if (iRet > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "SetGBLogicDeviceInfoEnableFlagByDelMark() SendNotifyCatalogMessageToAllRoute OK:iRet=%d \r\n", iRet);
            }

            /* 发送设备状态告警消息给在线用户  */
            iRet = SendDeviceOffLineAlarmToAllClientUser((char*)MasterDeviceIDVector[index].c_str());

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMark() SendDeviceOffLineAlarmToAllClientUser Error:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SetGBLogicDeviceInfoEnableFlagByDelMark() SendDeviceOffLineAlarmToAllClientUser OK:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }

            /* 查找所有逻辑设备点位业务并停止*/
            iRet = StopAllServiceTaskByLogicDeviceID((char*)MasterDeviceIDVector[index].c_str());

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMark() StopAllServiceTaskByLogicDeviceID Error:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SetGBLogicDeviceInfoEnableFlagByDelMark() StopAllServiceTaskByLogicDeviceID OK:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }

            /* 查找所有逻辑设备点位音频对讲业务并停止 */
            iRet = StopAudioServiceTaskByLogicDeviceID((char*)MasterDeviceIDVector[index].c_str());

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMark() StopAudioServiceTaskByLogicDeviceID Error:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SetGBLogicDeviceInfoEnableFlagByDelMark() StopAudioServiceTaskByLogicDeviceID OK:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }

            /* 同步到数据库 */
            iRet = AddGBLogicDeviceInfo2DB((char*)MasterDeviceIDVector[index].c_str(), pDevice_Srv_dboper);
        }
    }

    MasterDeviceIDVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : GBLogicDeviceCatalogInfoProc
 功能描述  : 物理设备上报的逻辑通道信息处理
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             GBLogicDevice_info_t* pNewGBLogicDeviceInfo
             GBLogicDevice_info_t* pOldGBLogicDeviceInfo
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年12月23日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GBLogicDeviceCatalogInfoProc(GBDevice_info_t* pGBDeviceInfo, GBLogicDevice_info_t* pNewGBLogicDeviceInfo, GBLogicDevice_info_t* pOldGBLogicDeviceInfo, DBOper* pDevice_Srv_dboper)
{
    int i = 0;
    int change_flag = 0;
    int change_flag_RCU = 0;
    GBLogicDevice_info_t* pDBGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pMasterGBDeviceInfo = NULL;
    GBDevice_t* pMasterGBDevice = NULL;

    if (NULL == pGBDeviceInfo || NULL == pNewGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDeviceCatalogInfoProc() exit---: Param Error \r\n");
        return -1;
    }

    /* 主流类型设备 */
    pMasterGBDevice = GBDevice_get_by_stream_type(pOldGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

    if (NULL != pMasterGBDevice) /* 已经存在，比较看是否一样 */
    {
        pMasterGBDeviceInfo = pMasterGBDevice->ptGBDeviceInfo;

        if (NULL != pMasterGBDeviceInfo)
        {
            if (0 != sstrcmp(pMasterGBDeviceInfo->device_id, pGBDeviceInfo->device_id))
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "前端设备目录查询响应消息:上报的逻辑设备ID=%s, 逻辑点位名称=%s, 该逻辑设备两次上报的所属物理设备不一致，可能该逻辑点位ID存在重复配置,老的物理设备ID=%s, IP=%s, 新的物理设备ID=%s, IP=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pMasterGBDeviceInfo->device_id, pMasterGBDeviceInfo->login_ip, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Front end device directory query response message:Logic device state change:device ID=%s, old state =%d, new state=%d; intelligent analysis old state=%d,new state=%d; alarm old state =%d,New state=%d", pNewGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status, pOldGBLogicDeviceInfo->intelligent_status, pNewGBLogicDeviceInfo->intelligent_status, pOldGBLogicDeviceInfo->alarm_status, pNewGBLogicDeviceInfo->alarm_status);

                pMasterGBDevice->ptGBDeviceInfo = pGBDeviceInfo;
            }
        }
        else
        {
            pMasterGBDevice->ptGBDeviceInfo = pGBDeviceInfo;
        }
    }
    else /* 不存在，直接添加 */
    {
        i = GBDevice_add(pOldGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER, pGBDeviceInfo);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_MASTER Error:i=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GBLogicDeviceCatalogInfoProc() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_MASTER OK:i=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
        }
    }

    /* 状态变化需要通知给客户端 */
    if (pOldGBLogicDeviceInfo->status != pNewGBLogicDeviceInfo->status
        || pOldGBLogicDeviceInfo->intelligent_status != pNewGBLogicDeviceInfo->intelligent_status
        || pOldGBLogicDeviceInfo->alarm_status != pNewGBLogicDeviceInfo->alarm_status)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端设备目录查询响应消息, 逻辑设备状态变化:设备ID=%s, 老状态=%d, 新状态=%d; 智能分析老状态=%d,新状态=%d; 报警老状态=%d,新状态=%d", pNewGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status, pOldGBLogicDeviceInfo->intelligent_status, pNewGBLogicDeviceInfo->intelligent_status, pOldGBLogicDeviceInfo->alarm_status, pNewGBLogicDeviceInfo->alarm_status);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end device directory query response message:Reported logic device ID=%s, logic point name =%s, New disabled identification=%d, Old disable identification=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->enable, pOldGBLogicDeviceInfo->enable);

        if (1 == pNewGBLogicDeviceInfo->status && INTELLIGENT_STATUS_ON == pOldGBLogicDeviceInfo->intelligent_status)
        {
            /* 发送设备状态变化消息 */
            i = SendDeviceStatusMessageProc(pNewGBLogicDeviceInfo, 4, pDevice_Srv_dboper);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() SendDeviceStatusMessageProc ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 4, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() SendDeviceStatusMessageProc OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 4, i);
            }
        }
        else if (1 == pNewGBLogicDeviceInfo->status && ALARM_STATUS_CLOSE == pNewGBLogicDeviceInfo->alarm_status)
        {
            /* 发送设备状态变化消息 */
            i = SendDeviceStatusMessageProc(pNewGBLogicDeviceInfo, 5, pDevice_Srv_dboper);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() SendDeviceStatusMessageProc ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 5, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() SendDeviceStatusMessageProc OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 5, i);
            }
        }
        else if (1 == pNewGBLogicDeviceInfo->status && ALARM_STATUS_APART == pNewGBLogicDeviceInfo->alarm_status)
        {
            /* 发送设备状态变化消息 */
            i = SendDeviceStatusMessageProc(pNewGBLogicDeviceInfo, 6, pDevice_Srv_dboper);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() SendDeviceStatusMessageProc ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 6, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() SendDeviceStatusMessageProc OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 6, i);
            }
        }
        else
        {
            /* 发送设备状态变化消息 */
            i = SendDeviceStatusMessageProc(pNewGBLogicDeviceInfo, pNewGBLogicDeviceInfo->status, pDevice_Srv_dboper);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() SendDeviceStatusMessageProc ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() SendDeviceStatusMessageProc OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, i);
            }
        }

        pOldGBLogicDeviceInfo->intelligent_status = pNewGBLogicDeviceInfo->intelligent_status;
        pOldGBLogicDeviceInfo->alarm_status = pNewGBLogicDeviceInfo->alarm_status;
    }

    if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type) /* 如果是CMS过来的 */
    {
        if (pOldGBLogicDeviceInfo->status == 1 && (pNewGBLogicDeviceInfo->status == 0 || pNewGBLogicDeviceInfo->status == 2))
        {
            if (pOldGBLogicDeviceInfo->status == 1 && pNewGBLogicDeviceInfo->status == 0)
            {
                /* 发送告警信息到客户端 */
                i = SendDeviceOffLineAlarmToAllClientUser(pOldGBLogicDeviceInfo->device_id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() SendDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() SendDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                }
            }
            else if (pOldGBLogicDeviceInfo->status == 1 && pNewGBLogicDeviceInfo->status == 2)
            {
                /* 发送告警信息到客户端 */
                i = SendDeviceNoStreamAlarmToAllClientUser(pOldGBLogicDeviceInfo->device_id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() SendDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() SendDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                }
            }

            i = StopAllServiceTaskByLogicDeviceID(pNewGBLogicDeviceInfo->device_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() StopAllServiceTaskByLogicDeviceID ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() StopAllServiceTaskByLogicDeviceID OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
            }

            if (pOldGBLogicDeviceInfo->status == 1 && pNewGBLogicDeviceInfo->status == 0)
            {
                /* 停止音频对讲业务 */
                i = StopAudioServiceTaskByLogicDeviceID(pNewGBLogicDeviceInfo->device_id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() StopAudioServiceTaskByLogicDeviceID Error:DeviceID=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() StopAudioServiceTaskByLogicDeviceID OK:DeviceID=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
                }
            }
        }
        else if (INTELLIGENT_STATUS_ON == pOldGBLogicDeviceInfo->intelligent_status && INTELLIGENT_STATUS_NULL == pNewGBLogicDeviceInfo->intelligent_status)
        {
            /* 发送告警信息到客户端 */
            i = SendIntelligentDeviceOffLineAlarmToAllClientUser(pOldGBLogicDeviceInfo->device_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() SendIntelligentDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() SendIntelligentDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
            }

            i = StopAllServiceTaskByLogicDeviceIDAndStreamType(pNewGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_INTELLIGENCE);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() StopAllServiceTaskByLogicDeviceIDAndStreamType ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() StopAllServiceTaskByLogicDeviceIDAndStreamType OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
            }
        }
    }
    else
    {
        if (pOldGBLogicDeviceInfo->status == 1 && (pNewGBLogicDeviceInfo->status == 0 || pNewGBLogicDeviceInfo->status == 2))
        {
            if (pOldGBLogicDeviceInfo->status == 1 && pNewGBLogicDeviceInfo->status == 0)
            {
                /* 发送告警信息到客户端 */
                i = SendDeviceOffLineAlarmToAllClientUser(pOldGBLogicDeviceInfo->device_id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() SendDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() SendDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                }
            }
            else if (pOldGBLogicDeviceInfo->status == 1 && pNewGBLogicDeviceInfo->status == 2)
            {
                /* 发送告警信息到客户端 */
                i = SendDeviceNoStreamAlarmToAllClientUser(pOldGBLogicDeviceInfo->device_id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() SendDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() SendDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                }
            }

            if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_DECODER) /* 解码器通道根据主叫ID停止业务 */
            {
                i = StopAllServiceTaskByCallerID(pNewGBLogicDeviceInfo->device_id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() StopAllServiceTaskByCallerID ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() StopAllServiceTaskByCallerID OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
                }
            }
            else if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
            {
                i = StopAllServiceTaskByLogicDeviceIDAndStreamType(pNewGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_INTELLIGENCE);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() StopAllServiceTaskByLogicDeviceIDAndStreamType Error:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() StopAllServiceTaskByLogicDeviceIDAndStreamType OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
                }
            }
            else
            {
                i = StopAllServiceTaskByLogicDeviceID(pNewGBLogicDeviceInfo->device_id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() StopAllServiceTaskByLogicDeviceID ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() StopAllServiceTaskByLogicDeviceID OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
                }

                if (pOldGBLogicDeviceInfo->status == 1 && pNewGBLogicDeviceInfo->status == 0)
                {
                    /* 停止音频对讲业务 */
                    i = StopAudioServiceTaskByLogicDeviceID(pNewGBLogicDeviceInfo->device_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() StopAudioServiceTaskByLogicDeviceID Error:DeviceID=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() StopAudioServiceTaskByLogicDeviceID OK:DeviceID=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
                    }
                }
            }
        }
    }

    /* 查看设备信息是否有变化 */
    change_flag = IsGBLogicDeviceInfoHasChange(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo, 0);

    if (1 == change_flag) /* 内存中是否有变化 */
    {
        change_flag_RCU = IsGBLogicDeviceInfoHasChangeForRCU(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo);

        /* 更新内存 */
        i = GBLogicDevice_info_update(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo, 0);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() GBLogicDevice_info_update ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() GBLogicDevice_info_update OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }

        if (1 == change_flag_RCU) /* 内存中RCU是否有变化 */
        {
            /* 发送RCU设备状态消息给客户端 */
            i = SendRCUDeviceStatusToAllClientUser(pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->AlarmPriority, pNewGBLogicDeviceInfo->guard_type, pNewGBLogicDeviceInfo->Value, pNewGBLogicDeviceInfo->Unit);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() SendRCUDeviceStatusToAllClientUser ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() SendRCUDeviceStatusToAllClientUser OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, i);
            }
        }

        /* 发送Catalog变化通知消息  */
        i = SendNotifyCatalogMessageToAllRoute(pOldGBLogicDeviceInfo, 2, pDevice_Srv_dboper);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "GBLogicDeviceCatalogInfoProc() SendNotifyCatalogMessageToAllRoute Error:iRet=%d \r\n", i);
        }
        else if (i > 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "GBLogicDeviceCatalogInfoProc() SendNotifyCatalogMessageToAllRoute OK:iRet=%d \r\n", i);
        }

        /* 更新数据库 */
        i = AddGBLogicDeviceInfo2DB(pNewGBLogicDeviceInfo->device_id, pDevice_Srv_dboper);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() AddGBLogicDeviceInfo2DB ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() AddGBLogicDeviceInfo2DB OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
    }
    else if (2 == change_flag) /* 仅仅状态发生了变化, 就不需要在发送变化Catalog了 */
    {
        /* 更新内存 */
        i = GBLogicDevice_info_update(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo, 0);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() GBLogicDevice_info_update ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() GBLogicDevice_info_update OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }

        /* 更新数据库 */
        i = UpdateGBLogicDeviceRegStatus2DB(pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, pDevice_Srv_dboper);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() UpdateGBLogicDeviceRegStatus2DB ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() UpdateGBLogicDeviceRegStatus2DB OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
    }
    else
    {
        /* 看数据库中是否有以前端上报为准的数据发生了改变，需要同步到数据库 */
        i = load_GBLogicDevice_info_from_db_by_device_id(pDevice_Srv_dboper, pOldGBLogicDeviceInfo->device_id, &pDBGBLogicDeviceInfo);

        if (i == 0)
        {
            /* 更新数据库 */
            i = AddGBLogicDeviceInfo2DB(pNewGBLogicDeviceInfo->device_id, pDevice_Srv_dboper);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() AddGBLogicDeviceInfo2DB ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() AddGBLogicDeviceInfo2DB OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
            }
        }
        else if (i > 0)
        {
            /* 看数据库中的数据是否有变化 */
            change_flag = IsGBLogicDeviceInfoHasChange(pDBGBLogicDeviceInfo, pOldGBLogicDeviceInfo, 0);

            if (1 == change_flag)
            {
                /* 从内存更新到数据库 */
                i = AddGBLogicDeviceInfo2DB(pNewGBLogicDeviceInfo->device_id, pDevice_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() AddGBLogicDeviceInfo2DB ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() AddGBLogicDeviceInfo2DB OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
                }
            }

            GBLogicDevice_info_free(pDBGBLogicDeviceInfo);
            osip_free(pDBGBLogicDeviceInfo);
            pDBGBLogicDeviceInfo = NULL;
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : GBLogicDeviceCatalogInfoProcForRoute
 功能描述  : 上级CMS推送逻辑通道点位信息处理
 输入参数  : GBLogicDevice_info_t* pNewGBLogicDeviceInfo
             GBLogicDevice_info_t* pOldGBLogicDeviceInfo
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月10日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GBLogicDeviceCatalogInfoProcForRoute(GBLogicDevice_info_t* pNewGBLogicDeviceInfo, GBLogicDevice_info_t* pOldGBLogicDeviceInfo, DBOper* pDevice_Srv_dboper)
{
    int i = 0;
    int change_flag = 0;
    int change_flag_RCU = 0;
    GBLogicDevice_info_t* pDBGBLogicDeviceInfo = NULL;

    if (NULL == pNewGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDeviceCatalogInfoProcForRoute() exit---: Param Error \r\n");
        return -1;
    }

    /* 状态变化需要通知给客户端 */
    if (pOldGBLogicDeviceInfo->status != pNewGBLogicDeviceInfo->status
        || pOldGBLogicDeviceInfo->intelligent_status != pNewGBLogicDeviceInfo->intelligent_status
        || pOldGBLogicDeviceInfo->alarm_status != pNewGBLogicDeviceInfo->alarm_status)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级CMS推送点位消息, 逻辑设备状态变化:设备ID=%s, 老状态=%d, 新状态=%d; 智能分析老状态=%d,新状态=%d; 报警老状态=%d,新状态=%d", pNewGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status, pOldGBLogicDeviceInfo->intelligent_status, pNewGBLogicDeviceInfo->intelligent_status, pOldGBLogicDeviceInfo->alarm_status, pNewGBLogicDeviceInfo->alarm_status);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Superior CMS push point message:logic device status change:device ID=%s, old state=%d, new state=%d; intelligent analysis old state=%d,new state=%d; alarm old state=%d,new state=%d", pNewGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status, pOldGBLogicDeviceInfo->intelligent_status, pNewGBLogicDeviceInfo->intelligent_status, pOldGBLogicDeviceInfo->alarm_status, pNewGBLogicDeviceInfo->alarm_status);

        if (1 == pNewGBLogicDeviceInfo->status && INTELLIGENT_STATUS_ON == pOldGBLogicDeviceInfo->intelligent_status)
        {
            /* 发送设备状态消息给客户端 */
            i = SendDeviceStatusToAllClientUser(pNewGBLogicDeviceInfo->device_id, 4, pDevice_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 4, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 4, i);
            }

            /* 发送设备状态消息给下级CMS  */
            i = SendDeviceStatusToSubCMS(pNewGBLogicDeviceInfo, 4, pDevice_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 4, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 4, i);
            }
        }
        else if (1 == pNewGBLogicDeviceInfo->status && ALARM_STATUS_CLOSE == pNewGBLogicDeviceInfo->alarm_status)
        {
            /* 发送设备状态消息给客户端 */
            i = SendDeviceStatusToAllClientUser(pNewGBLogicDeviceInfo->device_id, 5, pDevice_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 5, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 5, i);
            }

            /* 发送设备状态消息给下级CMS  */
            i = SendDeviceStatusToSubCMS(pNewGBLogicDeviceInfo, 5, pDevice_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 5, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 5, i);
            }
        }
        else if (1 == pNewGBLogicDeviceInfo->status && ALARM_STATUS_APART == pNewGBLogicDeviceInfo->alarm_status)
        {
            /* 发送设备状态消息给客户端 */
            i = SendDeviceStatusToAllClientUser(pNewGBLogicDeviceInfo->device_id, 6, pDevice_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 6, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 6, i);
            }

            /* 发送设备状态消息给下级CMS  */
            i = SendDeviceStatusToSubCMS(pNewGBLogicDeviceInfo, 6, pDevice_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 6, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 6, i);
            }
        }
        else
        {
            /* 发送设备状态消息给客户端 */
            i = SendDeviceStatusToAllClientUser(pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, pDevice_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, i);
            }

            /* 发送设备状态消息给下级CMS  */
            i = SendDeviceStatusToSubCMS(pNewGBLogicDeviceInfo, pNewGBLogicDeviceInfo->status, pDevice_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, i);
            }
        }

        pOldGBLogicDeviceInfo->intelligent_status = pNewGBLogicDeviceInfo->intelligent_status;
        pOldGBLogicDeviceInfo->alarm_status = pNewGBLogicDeviceInfo->alarm_status;
    }

    /* 发送告警信息 */
    if (pOldGBLogicDeviceInfo->status == 1
        && (pNewGBLogicDeviceInfo->status == 0 || pNewGBLogicDeviceInfo->status == 2))
    {
        if (pOldGBLogicDeviceInfo->status == 1 && pNewGBLogicDeviceInfo->status == 0)
        {
            /* 发送告警信息到客户端 */
            i = SendDeviceOffLineAlarmToAllClientUser(pOldGBLogicDeviceInfo->device_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
            }
        }
        else if (pOldGBLogicDeviceInfo->status == 1 && pNewGBLogicDeviceInfo->status == 2)
        {
            /* 发送告警信息到客户端 */
            i = SendDeviceNoStreamAlarmToAllClientUser(pOldGBLogicDeviceInfo->device_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
            }
        }

        i = StopAllServiceTaskByLogicDeviceID(pNewGBLogicDeviceInfo->device_id);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() StopAllServiceTaskByLogicDeviceID ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() StopAllServiceTaskByLogicDeviceID OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }

        if (pOldGBLogicDeviceInfo->status == 1 && pNewGBLogicDeviceInfo->status == 0)
        {
            /* 停止音频对讲业务 */
            i = StopAudioServiceTaskByLogicDeviceID(pNewGBLogicDeviceInfo->device_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() StopAudioServiceTaskByLogicDeviceID Error:DeviceID=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() StopAudioServiceTaskByLogicDeviceID OK:DeviceID=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
            }
        }
    }
    else if (INTELLIGENT_STATUS_ON == pOldGBLogicDeviceInfo->intelligent_status && INTELLIGENT_STATUS_NULL == pNewGBLogicDeviceInfo->intelligent_status)
    {
        /* 发送告警信息到客户端 */
        i = SendIntelligentDeviceOffLineAlarmToAllClientUser(pOldGBLogicDeviceInfo->device_id);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendIntelligentDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendIntelligentDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
        }

        i = StopAllServiceTaskByLogicDeviceIDAndStreamType(pNewGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_INTELLIGENCE);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() StopAllServiceTaskByLogicDeviceIDAndStreamType ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() StopAllServiceTaskByLogicDeviceIDAndStreamType OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
    }

    /* 查看设备信息是否有变化 */
    change_flag = IsGBLogicDeviceInfoHasChangeForRoute(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo, 0);

    if (1 == change_flag) /* 内存中是否有变化 */
    {
        change_flag_RCU = IsGBLogicDeviceInfoHasChangeForRCU(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo);

        /* 更新内存 */
        i = GBLogicDevice_info_update_for_route(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo, 0);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() GBLogicDevice_info_update ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() GBLogicDevice_info_update OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }

        if (1 == change_flag_RCU) /* 内存中RCU是否有变化 */
        {
            /* 发送RCU设备状态消息给客户端 */
            i = SendRCUDeviceStatusToAllClientUser(pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->AlarmPriority, pNewGBLogicDeviceInfo->guard_type, pNewGBLogicDeviceInfo->Value, pNewGBLogicDeviceInfo->Unit);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendRCUDeviceStatusToAllClientUser ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendRCUDeviceStatusToAllClientUser OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, i);
            }
        }

        /* 发送更新消息给下级CMS  */
        i = SendNotifyCatalogToSubCMS(pOldGBLogicDeviceInfo, 2, pDevice_Srv_dboper);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "GBLogicDeviceCatalogInfoProcForRoute() SendNotifyCatalogToSubCMS Error:iRet=%d \r\n", i);
        }
        else if (i > 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "GBLogicDeviceCatalogInfoProcForRoute() SendNotifyCatalogToSubCMS OK:iRet=%d \r\n", i);
        }

        /* 更新数据库 */
        i = AddGBLogicDeviceInfo2DBForRoute(pNewGBLogicDeviceInfo->device_id, pDevice_Srv_dboper);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() AddGBLogicDeviceInfo2DBForRoute ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() AddGBLogicDeviceInfo2DBForRoute OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
    }
    else if (2 == change_flag) /* 仅仅状态发生了变化, 就不需要在发送变化Catalog了 */
    {
        /* 更新内存 */
        i = GBLogicDevice_info_update_for_route(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo, 0);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() GBLogicDevice_info_update ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() GBLogicDevice_info_update OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }

        /* 更新数据库 */
        i = UpdateGBLogicDeviceRegStatus2DB(pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, pDevice_Srv_dboper);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() UpdateGBLogicDeviceRegStatus2DB ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() UpdateGBLogicDeviceRegStatus2DB OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
    }
    else
    {
        /* 看数据库中是否有用户配置了的变化的数据, 需要更新到内存*/
        i = load_GBLogicDevice_info_from_db_by_device_id(pDevice_Srv_dboper, pOldGBLogicDeviceInfo->device_id, &pDBGBLogicDeviceInfo);

        if (i == 0)
        {
            /* 更新数据库 */
            i = AddGBLogicDeviceInfo2DB(pNewGBLogicDeviceInfo->device_id, pDevice_Srv_dboper);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() AddGBLogicDeviceInfo2DB ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() AddGBLogicDeviceInfo2DB OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
            }
        }
        else if (i > 0)
        {
            /* 看数据库中的数据是否有变化 */
            if (IsGBLogicDeviceInfoHasChangeForRoute(pOldGBLogicDeviceInfo, pDBGBLogicDeviceInfo, 0))
            {
                /* 从内存更新到数据库 */
                i = AddGBLogicDeviceInfo2DB(pNewGBLogicDeviceInfo->device_id, pDevice_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() AddGBLogicDeviceInfo2DB ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() AddGBLogicDeviceInfo2DB OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
                }
            }

            GBLogicDevice_info_free(pDBGBLogicDeviceInfo);
            osip_free(pDBGBLogicDeviceInfo);
            pDBGBLogicDeviceInfo = NULL;
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : IntelligentAnalysisGBLogicDeviceCatalogInfoProc
 功能描述  : 智能分析设备的Catalog处理
 输入参数  : GBDevice_info_t * pGBDeviceInfo
             char* device_id
             DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月8日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int IntelligentAnalysisGBLogicDeviceCatalogInfoProc(GBDevice_info_t * pGBDeviceInfo, char* device_id, DBOper* pDboper)
{
    int i = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pIntelligentGBDeviceInfo = NULL;
    GBDevice_t* pIntelligentGBDevice = NULL;

    if (NULL == pGBDeviceInfo || NULL == device_id || NULL == pDboper)
    {
        return -1;
    }

    /* 旧的逻辑设备 */
    pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

    if (NULL == pGBLogicDeviceInfo)
    {
        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "前端设备目录查询响应消息:上报的逻辑设备ID=%s, 前端设备是智能行为分析设备，上报的分析点位不是已经存在的逻辑点位", device_id);
        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Front-end device directory search response message:logic device reported ID=%s, front-end device is intelligent analysis device，analysis point reported is not existed logic device.", device_id);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "IntelligentAnalysisGBLogicDeviceCatalogInfoProc() exit---: Find GBLogicDevice Info Error \r\n");
        return -1;
    }

    if (0 == pGBLogicDeviceInfo->enable)
    {
        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "前端设备目录查询响应消息:上报的逻辑设备ID=%s, 前端设备是智能行为分析设备，上报的分析点位已经被禁用", device_id);
        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Front-end device directory search response message:logic device reported ID=%s, front-end device is intelligent analysis device，analysis point reported is disabled", device_id);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "IntelligentAnalysisGBLogicDeviceCatalogInfoProc() exit---: Find GBLogicDevice Info Error \r\n");
        return -1;
    }

    if (2 == pGBLogicDeviceInfo->del_mark)
    {
        /* 移除删除标识 */
        pGBLogicDeviceInfo->del_mark = 0;
    }

    if (INTELLIGENT_STATUS_NULL == pGBLogicDeviceInfo->intelligent_status)
    {
        pGBLogicDeviceInfo->intelligent_status = INTELLIGENT_STATUS_ON;

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端智能分析设备目录查询响应消息:前端物理设备ID=%s, IP=%s:上报的逻辑设备ID=%s, 智能分析状态=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->intelligent_status);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front-end intelligent analysis device directory search response message:front-end physical device ID=%s, IP=%s:logic device reported ID=%s, intelligent analysis status=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->intelligent_status);


        if (1 == pGBLogicDeviceInfo->status)
        {
            /* 发送设备状态变化消息 */
            i = SendDeviceStatusMessageProc(pGBLogicDeviceInfo, 4, pDboper);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "IntelligentAnalysisGBLogicDeviceCatalogInfoProc() SendDeviceStatusMessageProc ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "IntelligentAnalysisGBLogicDeviceCatalogInfoProc() SendDeviceStatusMessageProc OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
        }
    }

    /* 分析流类型设备 */
    pIntelligentGBDevice = GBDevice_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_INTELLIGENCE);

    if (NULL != pIntelligentGBDevice) /* 已经存在，比较看是否一样 */
    {
        pIntelligentGBDeviceInfo = pIntelligentGBDevice->ptGBDeviceInfo;

        if (NULL != pIntelligentGBDeviceInfo)
        {
            if (pIntelligentGBDeviceInfo != pGBDeviceInfo)
            {
                pIntelligentGBDevice->ptGBDeviceInfo = pGBDeviceInfo;
            }
        }
        else
        {
            pIntelligentGBDevice->ptGBDeviceInfo = pGBDeviceInfo;
        }
    }
    else /* 不存在，直接添加 */
    {
        i = GBDevice_add(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_INTELLIGENCE, pGBDeviceInfo);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "IntelligentAnalysisGBLogicDeviceCatalogInfoProc() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_INTELLIGENCE Error:i=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IntelligentAnalysisGBLogicDeviceCatalogInfoProc() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_INTELLIGENCE OK:i=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : CivilCodeGBLogicDeviceCatalogInfoProc
 功能描述  : 下级上报的行政区域组织处理
 输入参数  : GBDevice_info_t * pGBDeviceInfo
             char* civil_code
             char* civil_name
             char* parent_id
             int iEvent
             DBOper* pDboper
             int iNeedToSync
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月9日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int CivilCodeGBLogicDeviceCatalogInfoProc(GBDevice_info_t * pGBDeviceInfo, char* civil_code, char* civil_name, char* parent_id, int iEvent, DBOper* pDboper, int iNeedToSync)
{
    int i = 0;
    int iCivilCodeLen = 0;
    int iParentIDLen = 0;
    string strGroupID = "";
    string strParentID = "";
    char strParentCodePrex[10] = {0}; /* 编码前缀  */

    if (NULL == pGBDeviceInfo || NULL == civil_code || NULL == pDboper)
    {
        return -1;
    }

    if (civil_code[0] == '\0')
    {
        return -1;
    }

    iCivilCodeLen = strlen(civil_code);

    if (NULL != parent_id || parent_id[0] != '\0')
    {
        iParentIDLen = strlen(parent_id);
    }

    if (2 == iCivilCodeLen) /* 省级组织 */
    {
        strGroupID = civil_code;
        strGroupID += "000000000000000000000000000000";

        /* 上级全0 */
        strParentID = "00000000000000000000000000000000";
    }
    else if (4 == iCivilCodeLen) /* 市级组织 */
    {
        strGroupID = civil_code;
        strGroupID += "0000000000000000000000000000";

        memset(strParentCodePrex, 0, 10);

        if (NULL == parent_id || parent_id[0] == '\0')
        {
            osip_strncpy(strParentCodePrex, &civil_code[0], 2);
            strParentID = strParentCodePrex;
            strParentID += "000000000000000000000000000000";
        }
        else
        {
            if (iParentIDLen > 0 && 0 == strncmp(civil_code, parent_id, iParentIDLen))
            {
                strParentID = parent_id;
                strParentID += "000000000000000000000000000000";
            }
            else
            {
                osip_strncpy(strParentCodePrex, &civil_code[0], 2);
                strParentID = strParentCodePrex;
                strParentID += "000000000000000000000000000000";
            }
        }
    }
    else if (6 == iCivilCodeLen) /* 区级组织 */
    {
        strGroupID = civil_code;
        strGroupID += "00000000000000000000000000";

        memset(strParentCodePrex, 0, 10);

        if (NULL == parent_id || parent_id[0] == '\0')
        {
            osip_strncpy(strParentCodePrex, &civil_code[0], 4);
            strParentID = strParentCodePrex;
            strParentID += "0000000000000000000000000000";
        }
        else
        {
            if (iParentIDLen > 0 && 0 == strncmp(civil_code, parent_id, iParentIDLen))
            {
                strParentID = parent_id;
                strParentID += "0000000000000000000000000000";
            }
            else
            {
                osip_strncpy(strParentCodePrex, &civil_code[0], 4);
                strParentID = strParentCodePrex;
                strParentID += "0000000000000000000000000000";
            }
        }
    }
    else if (8 == iCivilCodeLen) /* 四级组织 */
    {
        strGroupID = civil_code;
        strGroupID += "000000000000000000000000";

        memset(strParentCodePrex, 0, 10);

        if (NULL == parent_id || parent_id[0] == '\0')
        {
            osip_strncpy(strParentCodePrex, &civil_code[0], 6);
            strParentID = strParentCodePrex;
            strParentID += "00000000000000000000000000";
        }
        else
        {
            if (iParentIDLen > 0 && 0 == strncmp(civil_code, parent_id, iParentIDLen))
            {
                strParentID = parent_id;
                strParentID += "00000000000000000000000000";
            }
            else
            {
                osip_strncpy(strParentCodePrex, &civil_code[0], 6);
                strParentID = strParentCodePrex;
                strParentID += "00000000000000000000000000";
            }
        }
    }
    else if (10 == iCivilCodeLen) /* 五级组织 */
    {
        strGroupID = civil_code;
        strGroupID += "0000000000000000000000";

        memset(strParentCodePrex, 0, 10);

        if (NULL == parent_id || parent_id[0] == '\0')
        {
            osip_strncpy(strParentCodePrex, &civil_code[0], 8);
            strParentID = strParentCodePrex;
            strParentID += "000000000000000000000000";
        }
        else
        {
            if (iParentIDLen > 0 && 0 == strncmp(civil_code, parent_id, iParentIDLen))
            {
                strParentID = parent_id;
                strParentID += "000000000000000000000000";
            }
            else
            {
                osip_strncpy(strParentCodePrex, &civil_code[0], 8);
                strParentID = strParentCodePrex;
                strParentID += "000000000000000000000000";
            }
        }
    }
    else
    {
        if (1 == iEvent)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "前端设备上报的行政区域编码处理:行政区域编码长度不合法:长度=%d", iCivilCodeLen);
            return -1;
        }
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "前端设备上报的行政区域编码处理:转换后的分组ID=%s, 分组名称=%s, 上级分组ID=%s", (char*)strGroupID.c_str(), civil_name, (char*)strParentID.c_str());

    i = DeviceGroupConfigInfoProc(pGBDeviceInfo, (char*)strGroupID.c_str(), civil_name, (char*)strParentID.c_str(), 0, iEvent, pDboper, iNeedToSync);

    return i;
}

/*****************************************************************************
 函 数 名  : GroupCodeGBLogicDeviceCatalogInfoProc
 功能描述  : 下级上报的业务分组虚拟组织处理
 输入参数  : GBDevice_info_t * pGBDeviceInfo
             char* group_id
             char* group_name
             char* parent_id
             int iEvent
             DBOper* pDboper
             int iNeedToSync
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月9日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GroupCodeGBLogicDeviceCatalogInfoProc(GBDevice_info_t * pGBDeviceInfo, char* group_id, char* group_name, char* parent_id, int iEvent, DBOper* pDboper, int iNeedToSync)
{
    int i = 0;
    int iParentIDLen = 0;
    string strGroupID = "";
    string strParentID = "";
    char strGroupIDSuffix[8] = {0};  /* 分组ID后缀,取分组的后6位 */
    char strGroupIDPrex[12] = {0};   /* 分组ID前缀,取分组的前8位  */
    char strParentIDSuffix[8] = {0}; /* 父分组ID后缀,取分组的后6位 */
    char strParentIDPrex[12] = {0};  /* 父组ID前缀,取分组的前8位  */

    if (NULL == pGBDeviceInfo || NULL == group_id || NULL == parent_id || NULL == pDboper)
    {
        return -1;
    }

    if (group_id[0] == '\0')
    {
        return -1;
    }

    /* 取出分组的后6位 */
    osip_strncpy(strGroupIDSuffix, &group_id[14], 6);

    /* 取出分组的前8位 */
    osip_strncpy(strGroupIDPrex, &group_id[0], 8);

    iParentIDLen = strlen(parent_id);

    if (2 == iParentIDLen) /* 省级组织 */
    {
        /* 分组编码 */
        strGroupID = parent_id;
        strGroupID += "000000";
        strGroupID += strGroupIDSuffix;
        strGroupID += "000000000000000000";

        /* 上级为省级 */
        strParentID = parent_id;
        strParentID += "000000000000000000000000000000";
    }
    else if (4 == iParentIDLen) /* 市级组织 */
    {
        /* 分组编码 */
        strGroupID = parent_id;
        strGroupID += "0000";
        strGroupID += strGroupIDSuffix;
        strGroupID += "000000000000000000";

        /* 上级为市级 */
        strParentID = parent_id;
        strParentID += "0000000000000000000000000000";
    }
    else if (6 == iParentIDLen) /* 区级组织 */
    {
        /* 分组编码 */
        strGroupID = parent_id;
        strGroupID += "00";
        strGroupID += strGroupIDSuffix;
        strGroupID += "000000000000000000";

        /* 上级为区级 */
        strParentID = parent_id;
        strParentID += "00000000000000000000000000";
    }
    else if (8 == iParentIDLen) /* 四级组织 */
    {
        /* 分组编码 */
        strGroupID = parent_id;
        strGroupID += strGroupIDSuffix;
        strGroupID += "000000000000000000";

        /* 上级为派出所 */
        strParentID = parent_id;
        strParentID += "000000000000000000000000";
    }
    else if (10 == iParentIDLen) /* 五级组织 */
    {
        /* 分组编码 */
        strGroupID = parent_id;
        strGroupID += strGroupIDSuffix;
        strGroupID += "0000000000000000";

        /* 上级为五级组织 */
        strParentID = parent_id;
        strParentID += "0000000000000000000000";
    }
    else if (20 == iParentIDLen) /* 业务分组 */
    {
        /* 分组编码 */
        strGroupID = strGroupIDPrex;
        strGroupID += strGroupIDSuffix;
        strGroupID += "000000000000000000";

        /* 上级分组编码 */
        /* 取出分组的后6位 */
        osip_strncpy(strParentIDSuffix, &parent_id[14], 6);
        /* 取出分组的前8位 */
        osip_strncpy(strParentIDPrex, &parent_id[0], 8);

        strParentID = strParentIDPrex;
        strParentID += strParentIDSuffix;
        strParentID += "000000000000000000";
    }
    else
    {
        /* 分组编码 */
        strGroupID = strGroupIDPrex;
        strGroupID += strGroupIDSuffix;
        strGroupID += "000000000000000000";

        if (1 == iEvent)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "前端设备上报的业务分组编码处理:业务分组编码长度不合法:长度=%d", iParentIDLen);
            return -1;
        }
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "前端设备上报的业务分组编码处理:转换后的分组ID=%s, 分组名称=%s, 上级分组ID=%s", (char*)strGroupID.c_str(), group_name, (char*)strParentID.c_str());

    i = DeviceGroupConfigInfoProc(pGBDeviceInfo, (char*)strGroupID.c_str(), group_name, (char*)strParentID.c_str(), 0, iEvent, pDboper, iNeedToSync);

    return i;
}

/*****************************************************************************
 函 数 名  : DeviceGroupConfigInfoProc
 功能描述  : 下级上报的分组信息处理
 输入参数  : GBDevice_info_t * pGBDeviceInfo
             char* group_id
             char* group_name
             char* parent_id
             int sort_id
             int iEvent
             DBOper* pDboper
             int iNeedToSync
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月9日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DeviceGroupConfigInfoProc(GBDevice_info_t* pGBDeviceInfo, char* group_id, char* group_name, char* parent_id, int sort_id, int iEvent, DBOper* pDboper, int iNeedToSync)
{
    int i = 0;
    int iChangeFlag = 0;
    LogicDeviceGroup_t* pLogicDeviceGroup = NULL;

    if (NULL == pGBDeviceInfo || NULL == group_id || NULL == pDboper)
    {
        return -1;
    }

    if (group_id[0] == '\0')
    {
        return -1;
    }

    /* 查找逻辑设备分组信息，看是否存在 */
    pLogicDeviceGroup = GetLogicDeviceGroup(pGBDeviceInfo, group_id);

    if (1 == iEvent) /* 添加或者更新 */
    {
        if (NULL == pLogicDeviceGroup) /* 不存在，添加 */
        {
            /* 加入到分组 */
            i = AddLogicDeviceGroup(pGBDeviceInfo, group_id, pGBDeviceInfo->device_id, group_name, sort_id, parent_id);

            if (i >= 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "前端设备上报的逻辑设备分组信息处理:分组ID=%s, 分组名称=%s, 上级分组ID=%s, 添加到新增分组成功", group_id, group_name, parent_id);
                iChangeFlag = 1;
            }
        }
        else /* 存在，看是否有变化 */
        {
            i = ModifyLogicDeviceGroup(pLogicDeviceGroup, group_name, sort_id, parent_id);

            if (i == 1)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "前端设备上报的逻辑设备分组信息变化处理:分组ID=%s, 分组名称=%s, 上级分组ID=%s, 修改分组信息成功", group_id, group_name, parent_id);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "DeviceGroupConfigInfoProc() GroupID=%s, Name=%s, CMSID=%s, ModifyLogicDeviceGroup:i=%d \r\n", group_id, group_name, pGBDeviceInfo->device_id, i);
                iChangeFlag = 1;
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "前端设备上报的逻辑设备分组信息没有变化:分组ID=%s, 分组名称=%s, 上级分组ID=%s", group_id, group_name, parent_id);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "DeviceGroupConfigInfoProc() GroupID=%s, Name=%s, CMSID=%s, No Change \r\n", group_id, group_name, pGBDeviceInfo->device_id);
            }
        }
    }
    else if (2 == iEvent) /* 删除 */
    {
        if (NULL == pLogicDeviceGroup) /* 不存在 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端设备上报的逻辑设备分组信息删除处理:分组ID=%s, 分组名称=%s, 上级分组ID=%s, 分组信息不存在", group_id, group_name, parent_id);
        }
        else /* 存在，看是否有变化 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端设备上报的逻辑设备分组信息删除:分组ID=%s, 分组名称=%s, 上级分组ID=%s", group_id, group_name, parent_id);
            pLogicDeviceGroup->iChangeFlag = 3;
            iChangeFlag = 1;
        }
    }

    if (1 == iNeedToSync && 1 == iChangeFlag) /* 是否需要同步到数据库 */
    {
        /* 将变化同步到数据库 */
        i = SynLogicDeviceGroupInfoToDB2(pLogicDeviceGroup, pDboper);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "DeviceGroupConfigInfoProc() SynLogicDeviceGroupInfoToDB2:i=%d \r\n", i);

        if (2 == iEvent) /* 删除 */
        {
            /* 删除内存中多余的信息 */
            i = DelLogicDeviceGroupInfo2(pGBDeviceInfo, pLogicDeviceGroup);
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "DeviceGroupConfigInfoProc() DelLogicDeviceGroupInfo2:i=%d \r\n", i);
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : DeviceGroupMapConfigInfoProc
 功能描述  : 下级上报的分组关系信息处理
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             char* group_id
             unsigned int device_index
             int sort_id
             int iEvent
             DBOper* pDboper
             int iNeedToSync
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DeviceGroupMapConfigInfoProc(GBDevice_info_t* pGBDeviceInfo, char* group_id, unsigned int device_index, int sort_id, int iEvent, DBOper* pDboper, int iNeedToSync)
{
    int i = 0;
    int iChangeFlag = 0;
    LogicDeviceMapGroup_t* pLogicDeviceMapGroup = NULL;

    if (NULL == pGBDeviceInfo || NULL == group_id || NULL == pDboper)
    {
        return -1;
    }

    if (device_index <= 0)
    {
        return -1;
    }

    if (1 == iEvent) /* 添加或者更新 */
    {
        /* 查找逻辑设备分组关系信息，看是否存在 */
        pLogicDeviceMapGroup = GetLogicDeviceMapGroup(pGBDeviceInfo, device_index);

        if (NULL == pLogicDeviceMapGroup) /* 不存在，添加 */
        {
            /* 加入到分组 */
            i = AddLogicDeviceMapGroup(pGBDeviceInfo, group_id, device_index, pGBDeviceInfo->device_id, sort_id);

            if (i >= 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "前端设备上报的逻辑设备分组关系信息处理:分组ID=%s, 逻辑设备索引=%u, 添加到新增分组关系成功", group_id, device_index);
                iChangeFlag = 1;
            }
        }
        else /* 存在，看是否有变化 */
        {
            i = ModifyLogicDeviceMapGroup(pLogicDeviceMapGroup, group_id, pGBDeviceInfo->device_id, sort_id);

            if (i == 1)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "前端设备上报的逻辑设备分组关系信息变化处理:分组ID=%s, 逻辑设备索引=%u, 修改分组关系信息成功", group_id, device_index);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "DeviceGroupMapConfigInfoProc() GroupID=%s, device_index=%u, ModifyLogicDeviceGroup:i=%d \r\n", group_id, device_index, i);
                iChangeFlag = 1;
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "前端设备上报的逻辑设备分组关系信息没有变化:分组ID=%s, 逻辑设备索引=%u", group_id, device_index);
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO,  "DeviceGroupMapConfigInfoProc() GroupID=%s, device_index=%u, No Change \r\n", group_id, device_index);
            }
        }
    }
    else if (2 == iEvent) /* 删除 */
    {
        /* 查找逻辑设备分组关系信息，看是否存在 */
        pLogicDeviceMapGroup = GetLogicDeviceMapGroup(pGBDeviceInfo, device_index);

        if (NULL == pLogicDeviceMapGroup) /* 不存在 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端设备上报的逻辑设备分组关系信息删除处理:分组ID=%s, 逻辑设备索引=%u, 分组关系信息不存在", group_id, device_index);
        }
        else /* 存在，看是否有变化 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端设备上报的逻辑设备分组关系信息删除:分组ID=%s, 逻辑设备索引=%u", group_id, device_index);
            pLogicDeviceMapGroup->iChangeFlag = 3;
            iChangeFlag = 1;
        }
    }

    if (1 == iNeedToSync && 1 == iChangeFlag) /* 是否需要同步到数据库 */
    {
        /* 将变化同步到数据库 */
        i = SynLogicDeviceMapGroupInfoToDB2(pLogicDeviceMapGroup, pDboper);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "DeviceGroupMapConfigInfoProc() SynLogicDeviceMapGroupInfoToDB2:i=%d \r\n", i);

        if (2 == iEvent)
        {
            /* 删除内存中多余的信息 */
            i = DelLogicDeviceMapGroupInfo2(pGBDeviceInfo, pLogicDeviceMapGroup);
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "DeviceGroupMapConfigInfoProc() DelLogicDeviceMapGroupInfo2:i=%d \r\n", i);
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : set_GBLogicDevice_info_list_del_mark
 功能描述  : 设置逻辑设备信息删除标识
 输入参数  : int del_mark
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月23日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int set_GBLogicDevice_info_list_del_mark(int del_mark)
{
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return 0;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->id <= 0))
        {
            continue;
        }

        pGBLogicDeviceInfo->del_mark = del_mark;
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : delete_GBLogicDevice_info_from_list_by_mark
 功能描述  : 根据删除标识，删除逻辑设备信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月23日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int delete_GBLogicDevice_info_from_list_by_mark()
{
    int index = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    vector<string> DeviceIDVector;
    DeviceIDVector.clear();

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return 0;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->id <= 0))
        {
            continue;
        }

        if (pGBLogicDeviceInfo->del_mark == 3)
        {
            DeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "根据删除标识, 删除掉多余的逻辑设备点位信息: 删除的逻辑点位总数=%d", (int)DeviceIDVector.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "According to delete mark, delete the redundant standard physical device information: Delete logic point total = %d", (int)DeviceIDVector.size());

    if (DeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)DeviceIDVector.size(); index++)
        {
            pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

            if (NULL != pGBLogicDeviceInfo)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "根据删除标识, 删除掉多余的逻辑设备点位信息成功:逻辑设备ID=%s, 点位名称=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "According to delete mark, delete the redundant standard physical device information: the sum of standard physical device is deleted =%d", (int)DeviceIDVector.size());

                GBLOGICDEVICE_SMUTEX_LOCK();
                g_GBLogicDeviceInfoMap.erase(pGBLogicDeviceInfo->device_id);
                GBLogicDevice_info_free(pGBLogicDeviceInfo);
                osip_free(pGBLogicDeviceInfo);
                pGBLogicDeviceInfo = NULL;
                GBLOGICDEVICE_SMUTEX_UNLOCK();
            }
        }
    }

    DeviceIDVector.clear();
    return 0;
}

/*****************************************************************************
 函 数 名  : set_all_vms_nvr_GBLogicDevice_info_enable_mark
 功能描述  : 设置VMS
 输入参数  : char* device_id
             int enable_mark
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年4月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int set_vms_nvr_GBLogicDevice_info_enable_mark(char* device_id, int enable_mark)
{
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return 0;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->id <= 0))
        {
            continue;
        }

        if (0 == sstrcmp(pGBLogicDeviceInfo->device_id, device_id))
        {
            pGBLogicDeviceInfo->enable = enable_mark;
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : AddCivilCodeToGBLogicDeviceInfo
 功能描述  : 添加逻辑点位的行政区域信息
 输入参数  : DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月6日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddCivilCodeToGBLogicDeviceInfo(DBOper* pDevice_Srv_dboper)
{
    int i = 0;
    int index = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    primary_group_t* pPrimaryGroup = NULL;
    char strOldCivilCode[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strOldParentID[MAX_128CHAR_STRING_LEN + 4] = {0};
    int iChangeFlag = 0;
    vector<string> NeedToProcDeviceIDVector;
    vector<string> DeviceIDVector;

    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "\r\n ********************************************** \
		\r\n |AddCivilCodeToGBLogicDeviceInfo:BEGIN \
		\r\n ********************************************** \r\n");


    if (NULL == pDevice_Srv_dboper)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "AddCivilCodeToGBLogicDeviceInfo() exit---: Param Error \r\n");
        return -1;
    }

    NeedToProcDeviceIDVector.clear();
    DeviceIDVector.clear();

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return 0;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->id <= 0))
        {
            continue;
        }

        /* 其他域的点位就不用生成组织编码了 */
        if (pGBLogicDeviceInfo->other_realm == 1)
        {
            continue;
        }

        NeedToProcDeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    if (NeedToProcDeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)NeedToProcDeviceIDVector.size(); index++)
        {
            pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)NeedToProcDeviceIDVector[index].c_str());

            if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->id <= 0))
            {
                continue;
            }

            /* 其他域的点位就不用生成组织编码了 */
            if (pGBLogicDeviceInfo->other_realm == 1)
            {
                continue;
            }

            pPrimaryGroup = GetPrimaryGroupInfoByGBLogicDeviceIndex(pGBLogicDeviceInfo->id, pDevice_Srv_dboper);

            /* 组织编码 */
            if ('\0' == pGBLogicDeviceInfo->civil_code[0])
            {
                if (NULL != pPrimaryGroup)
                {
                    if (pPrimaryGroup->civil_code[0] != '\0')
                    {
                        osip_strncpy(pGBLogicDeviceInfo->civil_code, pPrimaryGroup->civil_code, 8);
                    }
                    else
                    {
                        osip_strncpy(pGBLogicDeviceInfo->civil_code, local_civil_code_get(), 8);
                    }
                }
                else
                {
                    osip_strncpy(pGBLogicDeviceInfo->civil_code, local_civil_code_get(), 8);
                }

                iChangeFlag = 1;

                //printf("\r\nAddCivilCodeToGBLogicDeviceInfo() add:device_id=%s, civil_code=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->civil_code);
                //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddCivilCodeToGBLogicDeviceInfo() device_id=%s, civil_code=%s \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->civil_code);
            }
            else
            {
                osip_strncpy(strOldCivilCode, pGBLogicDeviceInfo->civil_code, MAX_128CHAR_STRING_LEN);

                if (NULL != pPrimaryGroup)
                {
                    if (pPrimaryGroup->civil_code[0] != '\0')
                    {
                        if (0 != sstrcmp(pGBLogicDeviceInfo->civil_code, pPrimaryGroup->civil_code))
                        {
                            memset(pGBLogicDeviceInfo->civil_code, 0, MAX_128CHAR_STRING_LEN + 4);
                            osip_strncpy(pGBLogicDeviceInfo->civil_code, pPrimaryGroup->civil_code, 8);
                            //printf("\r\nAddCivilCodeToGBLogicDeviceInfo() chanaged 1: device_id=%s, civil_code=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->civil_code);
                            //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddCivilCodeToGBLogicDeviceInfo() device_id=%s, civil_code=%s \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->civil_code);
                        }
                    }
                    else
                    {
                        if (0 != sstrcmp(pGBLogicDeviceInfo->civil_code, local_civil_code_get()))
                        {
                            memset(pGBLogicDeviceInfo->civil_code, 0, MAX_128CHAR_STRING_LEN + 4);
                            osip_strncpy(pGBLogicDeviceInfo->civil_code, local_civil_code_get(), 8);
                            //printf("\r\nAddCivilCodeToGBLogicDeviceInfo() chanaged 2: device_id=%s, civil_code=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->civil_code);
                            //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddCivilCodeToGBLogicDeviceInfo() device_id=%s, civil_code=%s \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->civil_code);
                        }
                    }
                }
                else
                {
                    if (0 != sstrcmp(pGBLogicDeviceInfo->civil_code, local_civil_code_get()))
                    {
                        memset(pGBLogicDeviceInfo->civil_code, 0, MAX_128CHAR_STRING_LEN + 4);
                        osip_strncpy(pGBLogicDeviceInfo->civil_code, local_civil_code_get(), 8);
                        //printf("\r\nAddCivilCodeToGBLogicDeviceInfo() chanaged 2: device_id=%s, civil_code=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->civil_code);
                        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddCivilCodeToGBLogicDeviceInfo() device_id=%s, civil_code=%s \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->civil_code);
                    }
                }

                if (0 != sstrcmp(pGBLogicDeviceInfo->civil_code, strOldCivilCode))
                {
                    iChangeFlag = 1;
                }
            }

            /* 虚拟组织编码 */
            if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
            {
                if (NULL != pPrimaryGroup)
                {
                    if (pPrimaryGroup->group_code[0] != '\0')
                    {
                        if (0 == sstrcmp(pPrimaryGroup->group_code, pPrimaryGroup->civil_code)) /* group_code 和 civil_code一样，表示直接挂在行政区域下面，没有业务分组 */
                        {
                            osip_strncpy(pGBLogicDeviceInfo->virtualParentID, local_cms_id_get(), MAX_ID_LEN);
                        }
                        else
                        {
                            osip_strncpy(pGBLogicDeviceInfo->virtualParentID, pPrimaryGroup->group_code, MAX_ID_LEN);
                        }
                    }
                    else
                    {
                        osip_strncpy(pGBLogicDeviceInfo->virtualParentID, local_cms_id_get(), MAX_ID_LEN);
                    }
                }
                else
                {
                    osip_strncpy(pGBLogicDeviceInfo->virtualParentID, local_cms_id_get(), MAX_ID_LEN);
                }

                iChangeFlag = 1;
                //printf("\r\nAddCivilCodeToGBLogicDeviceInfo() add:device_id=%s, civil_code=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->civil_code);
                //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddCivilCodeToGBLogicDeviceInfo() device_id=%s, virtualParentID=%s \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->virtualParentID);
            }
            else
            {
                osip_strncpy(strOldParentID, pGBLogicDeviceInfo->virtualParentID, MAX_128CHAR_STRING_LEN);

                if (NULL != pPrimaryGroup)
                {
                    if (pPrimaryGroup->group_code[0] != '\0')
                    {
                        if (0 == sstrcmp(pPrimaryGroup->group_code, pPrimaryGroup->civil_code)) /* group_code 和 civil_code一样，表示直接挂在行政区域下面，没有业务分组 */
                        {
                            if (0 != sstrcmp(pGBLogicDeviceInfo->virtualParentID, local_cms_id_get()))
                            {
                                memset(pGBLogicDeviceInfo->virtualParentID, 0, MAX_128CHAR_STRING_LEN + 4);
                                osip_strncpy(pGBLogicDeviceInfo->virtualParentID, local_cms_id_get(), MAX_ID_LEN);
                            }
                        }
                        else
                        {
                            if (0 != sstrcmp(pGBLogicDeviceInfo->virtualParentID, pPrimaryGroup->group_code))
                            {
                                memset(pGBLogicDeviceInfo->virtualParentID, 0, MAX_128CHAR_STRING_LEN + 4);
                                osip_strncpy(pGBLogicDeviceInfo->virtualParentID, pPrimaryGroup->group_code, MAX_ID_LEN);
                                //printf("\r\nAddCivilCodeToGBLogicDeviceInfo() chanaged 1: device_id=%s, civil_code=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->civil_code);
                                //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddCivilCodeToGBLogicDeviceInfo() device_id=%s, virtualParentID=%s \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->virtualParentID);
                            }
                        }
                    }
                    else
                    {
                        if (0 != sstrcmp(pGBLogicDeviceInfo->virtualParentID, local_cms_id_get()))
                        {
                            memset(pGBLogicDeviceInfo->virtualParentID, 0, MAX_128CHAR_STRING_LEN + 4);
                            osip_strncpy(pGBLogicDeviceInfo->virtualParentID, local_cms_id_get(), MAX_ID_LEN);
                            //printf("\r\nAddCivilCodeToGBLogicDeviceInfo() chanaged 2: device_id=%s, civil_code=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->civil_code);
                            //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddCivilCodeToGBLogicDeviceInfo() device_id=%s, virtualParentID=%s \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->virtualParentID);
                        }
                    }
                }
                else
                {
                    if (0 != sstrcmp(pGBLogicDeviceInfo->virtualParentID, local_cms_id_get()))
                    {
                        memset(pGBLogicDeviceInfo->virtualParentID, 0, MAX_128CHAR_STRING_LEN + 4);
                        osip_strncpy(pGBLogicDeviceInfo->virtualParentID, local_cms_id_get(), MAX_ID_LEN);
                        //printf("\r\nAddCivilCodeToGBLogicDeviceInfo() chanaged 2: device_id=%s, civil_code=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->civil_code);
                        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddCivilCodeToGBLogicDeviceInfo() device_id=%s, virtualParentID=%s \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->virtualParentID);
                    }
                }

                if (0 != sstrcmp(pGBLogicDeviceInfo->virtualParentID, strOldParentID))
                {
                    iChangeFlag = 1;
                }
            }

            if (1 == iChangeFlag)/* 发送点位信息变化到上级CMS */
            {
                DeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
            }
        }
    }

    NeedToProcDeviceIDVector.clear();

    if (1 == cms_run_status)
    {
        if (DeviceIDVector.size() > 0)
        {
            for (index = 0; index < (int)DeviceIDVector.size(); index++)
            {
                pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

                if (NULL != pGBLogicDeviceInfo)
                {
                    /* 发送变化通知到其他平台 */
                    i = SendNotifyGroupMapCatalogTo3PartyRouteCMS(pGBLogicDeviceInfo, 2, pDevice_Srv_dboper);

                    if (i > 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "逻辑点位分组关系发生变化, 发送逻辑设备变化通知给上级平台:逻辑设备ID=%s", pGBLogicDeviceInfo->device_id);
                    }
                }
            }
        }
    }

    DeviceIDVector.clear();

    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "\r\n ********************************************** \
		\r\n |AddCivilCodeToGBLogicDeviceInfo:END \
		\r\n ********************************************** \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : GBLogicDeviceCatalogGroupMapProc
 功能描述  : 逻辑点位分组关系处理
 输入参数  : GBDevice_info_t * pGBDeviceInfo
             GBLogicDevice_info_t* pGBLogicDeviceInfo
             char* parent_id
             DBOper* pDevice_Srv_dboper
             int iNeedToSync
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月15日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GBLogicDeviceCatalogGroupMapProc(GBDevice_info_t * pGBDeviceInfo, GBLogicDevice_info_t* pGBLogicDeviceInfo, char* parent_id, DBOper* pDevice_Srv_dboper, int iNeedToSync)
{
    int i = 0;
    int iParentIDLen = 0;
    string strParentID = "";
    char strParentIDSuffix[8] = {0}; /* 父分组ID后缀,取分组的后6位 */
    char strParentIDPrex[12] = {0};  /* 父分组ID前缀,取分组的前8位  */

    if (NULL == pGBDeviceInfo || NULL == pGBLogicDeviceInfo || NULL == parent_id || NULL == pDevice_Srv_dboper)
    {
        return -1;
    }

    if (parent_id[0] == '\0') /* 如果父节点是空，则表示没有分组,标识删除  */
    {
        i = DeviceGroupMapConfigInfoProc(pGBDeviceInfo, parent_id, pGBLogicDeviceInfo->id, 0, 2, pDevice_Srv_dboper, iNeedToSync);
    }
    else
    {
        iParentIDLen = strlen(parent_id);

        if (2 == iParentIDLen) /* 省级组织 */
        {
            /* 上级为省级 */
            strParentID = parent_id;
            strParentID += "000000000000000000000000000000";
        }
        else if (4 == iParentIDLen) /* 市级组织 */
        {
            /* 上级为市级 */
            strParentID = parent_id;
            strParentID += "0000000000000000000000000000";
        }
        else if (6 == iParentIDLen) /* 区级组织 */
        {
            /* 上级为区级 */
            strParentID = parent_id;
            strParentID += "00000000000000000000000000";
        }
        else if (8 == iParentIDLen) /* 四级组织 */
        {
            /* 上级为派出所 */
            strParentID = parent_id;
            strParentID += "000000000000000000000000";
        }
        else if (10 == iParentIDLen) /* 五级组织 */
        {
            /* 上级为五级组织 */
            strParentID = parent_id;
            strParentID += "0000000000000000000000";
        }
        else if (20 == iParentIDLen) /* 业务分组 */
        {
            /* 上级分组编码 */
            /* 取出分组的后6位 */
            osip_strncpy(strParentIDSuffix, &parent_id[14], 6);
            /* 取出分组的前8位 */
            osip_strncpy(strParentIDPrex, &parent_id[0], 8);

            strParentID = strParentIDPrex;
            strParentID += strParentIDSuffix;
            strParentID += "000000000000000000";
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "前端设备上报的分组关系处理:点位的父节点业务分组编码长度不合法:长度=%d", iParentIDLen);
            return -1;
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "前端设备上报的分组关系处理:转换后的父节点分组ID=%s,点位ID=%s", (char*)strParentID.c_str(), pGBLogicDeviceInfo->device_id);

        /* 把原有的删除掉 */
        i = DeviceGroupMapConfigInfoProc(pGBDeviceInfo, (char*)strParentID.c_str(), pGBLogicDeviceInfo->id, 0, 2, pDevice_Srv_dboper, 0);

        /* 增加新的 */
        i = DeviceGroupMapConfigInfoProc(pGBDeviceInfo, (char*)strParentID.c_str(), pGBLogicDeviceInfo->id, 0, 1, pDevice_Srv_dboper, iNeedToSync);
    }

    return i;
}
#endif

/*****************************************************************************
 函 数 名  : load_db_data_to_LogicDevice_info_list_by_device_id
 功能描述  : 根据逻辑设备DeviceID从数据库中加载逻辑设备信息
 输入参数  : DBOper * ptDBOper
             char* device_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月25日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int load_db_data_to_LogicDevice_info_list_by_device_id(DBOper * ptDBOper, char* device_id)
{
    int iRet = 0;
    string strSQL = "";
    int record_count = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    int tmp_ivalue = 0;
    unsigned int tmp_uivalue = 0;
    string tmp_svalue = "";
    int gbdevice_pos = 0;

    if (NULL == device_id || ptDBOper == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "load_db_data_to_LogicDevice_info_list_by_device_id() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from GBLogicDeviceConfig WHERE DeviceID like '";
    strSQL += device_id;
    strSQL += "'";

    record_count = ptDBOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_db_data_to_LogicDevice_info_list_by_device_id() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_db_data_to_LogicDevice_info_list_by_device_id() ErrorMsg=%s\r\n", ptDBOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "load_db_data_to_LogicDevice_info_list_by_device_id() exit---: No Record Count, device_id=%s \r\n", device_id);
        return 0;
    }
    else if (record_count != 1)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_db_data_to_LogicDevice_info_list_by_device_id() exit---: Record Count Error: record_count=%d, device_id=%s\r\n", record_count, device_id);
        return -1;
    }

    iRet = GBLogicDevice_info_init(&pGBLogicDeviceInfo);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_db_data_to_LogicDevice_info_list_by_device_id() GBLogicDevice_info_init:iRet=%d", iRet);
        return -1;
    }

    /* 设备索引*/
    tmp_uivalue = 0;
    ptDBOper->GetFieldValue("ID", tmp_uivalue);

    pGBLogicDeviceInfo->id = tmp_uivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->id:%u", pGBLogicDeviceInfo->id);


    /* 点位统一编号 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("DeviceID", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy(pGBLogicDeviceInfo->device_id, (char*)tmp_svalue.c_str(), MAX_ID_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->device_id:%s", pGBLogicDeviceInfo->device_id);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->device_id NULL");
    }

    /* 所属的CMS 统一编号 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("CMSID", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy(pGBLogicDeviceInfo->cms_id, (char*)tmp_svalue.c_str(), MAX_ID_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->cms_id:%s", pGBLogicDeviceInfo->cms_id);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->cms_id NULL");
    }


    /* 点位名称 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("DeviceName", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy(pGBLogicDeviceInfo->device_name, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->device_name:%s", pGBLogicDeviceInfo->device_name);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->device_name NULL");
    }


    /* 是否启用 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Enable", tmp_ivalue);

    pGBLogicDeviceInfo->enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->enable:%d", pGBLogicDeviceInfo->enable);


    /* 设备类型*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("DeviceType", tmp_ivalue);

    pGBLogicDeviceInfo->device_type = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->device_type:%d", pGBLogicDeviceInfo->device_type);


    /* 报警设备子类型 */
    tmp_uivalue = 0;
    ptDBOper->GetFieldValue("Resved1", tmp_uivalue);

    pGBLogicDeviceInfo->alarm_device_sub_type = tmp_uivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->alarm_device_sub_type:%u", pGBLogicDeviceInfo->alarm_device_sub_type);


    /* 是否可控*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("CtrlEnable", tmp_ivalue);

    pGBLogicDeviceInfo->ctrl_enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->ctrl_enable:%d", pGBLogicDeviceInfo->ctrl_enable);


    /* 是否支持对讲，默认值0 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("MicEnable", tmp_ivalue);

    pGBLogicDeviceInfo->mic_enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->mic_enable:%d", pGBLogicDeviceInfo->mic_enable);


    /* 帧率，默认25 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("FrameCount", tmp_ivalue);

    pGBLogicDeviceInfo->frame_count = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->frame_count:%d", pGBLogicDeviceInfo->frame_count);


    /* 告警时长 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("AlarmLengthOfTime", tmp_ivalue);

    pGBLogicDeviceInfo->alarm_duration = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->alarm_duration:%d", pGBLogicDeviceInfo->alarm_duration);


    /* 对应的媒体物理设备索引*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("PhyDeviceIndex", tmp_ivalue);

    pGBLogicDeviceInfo->phy_mediaDeviceIndex = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->phy_mediaDeviceIndex:%d", pGBLogicDeviceInfo->phy_mediaDeviceIndex);


    /* 对应的媒体物理设备通道 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("PhyDeviceChannel", tmp_ivalue);

    pGBLogicDeviceInfo->phy_mediaDeviceChannel = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->phy_mediaDeviceChannel:%d", pGBLogicDeviceInfo->phy_mediaDeviceChannel);


    /* 对应的控制物理设备索引 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("CtrlDeviceIndex", tmp_ivalue);

    pGBLogicDeviceInfo->phy_ctrlDeviceIndex = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->phy_ctrlDeviceIndex:%d", pGBLogicDeviceInfo->phy_ctrlDeviceIndex);


    /* 对应的控制物理设备通道 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("CtrlDeviceChannel", tmp_ivalue);

    pGBLogicDeviceInfo->phy_ctrlDeviceChannel = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->phy_ctrlDeviceChannel:%d", pGBLogicDeviceInfo->phy_ctrlDeviceChannel);


    /* 是否支持多码流 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("StreamCount", tmp_ivalue);

    pGBLogicDeviceInfo->stream_count = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->stream_count:%d", pGBLogicDeviceInfo->stream_count);


    /* 录像类型 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("RecordType", tmp_ivalue);

    pGBLogicDeviceInfo->record_type = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->record_type:%d", pGBLogicDeviceInfo->record_type);


    /* 设备生产商 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Manufacturer", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy(pGBLogicDeviceInfo->manufacturer, tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->manufacturer:%s", pGBLogicDeviceInfo->manufacturer);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->manufacturer NULL");
    }


    /* 设备型号 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Model", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy(pGBLogicDeviceInfo->model, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->model:%s", pGBLogicDeviceInfo->model);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->model NULL");
    }


    /* 设备归属 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Owner", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy(pGBLogicDeviceInfo->owner, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->owner:%s", pGBLogicDeviceInfo->owner);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->owner NULL");
    }

#if 0
    /* 行政区域 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("CivilCode", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        pGBLogicDeviceInfo->civil_code = osip_getcopy(tmp_svalue.c_str());
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->civil_code:%s", pGBLogicDeviceInfo->civil_code);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->civil_code NULL");
    }

#endif

    /* 警区 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Block", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy(pGBLogicDeviceInfo->block, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->block:%s", pGBLogicDeviceInfo->block);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->block NULL");
    }


    /* 安装地址 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Address", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy(pGBLogicDeviceInfo->address, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->address:%s", pGBLogicDeviceInfo->address);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->address NULL");
    }


    /* 是否有子设备 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Parental", tmp_ivalue);

    pGBLogicDeviceInfo->parental = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->parental:%d", pGBLogicDeviceInfo->parental);


    /* 父设备/区域/系统ID */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("ParentID", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy(pGBLogicDeviceInfo->parentID, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->parentID:%s", pGBLogicDeviceInfo->parentID);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->parentID NULL");
    }


    /* 信令安全模式*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("SafetyWay", tmp_ivalue);

    pGBLogicDeviceInfo->safety_way = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->safety_way:%d", pGBLogicDeviceInfo->safety_way);


    /* 注册方式 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("RegisterWay", tmp_ivalue);

    pGBLogicDeviceInfo->register_way = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->safety_way:%d", pGBLogicDeviceInfo->register_way);


    /* 证书序列号*/
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("CertNum", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy(pGBLogicDeviceInfo->cert_num, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->cert_num:%s", pGBLogicDeviceInfo->cert_num);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->cert_num NULL");
    }


    /* 证书有效标识 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Certifiable", tmp_ivalue);

    pGBLogicDeviceInfo->certifiable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->certifiable:%d", pGBLogicDeviceInfo->certifiable);


    /* 无效原因码 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("ErrCode", tmp_ivalue);

    pGBLogicDeviceInfo->error_code = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->error_code:%d", pGBLogicDeviceInfo->error_code);


    /* 证书终止有效期*/
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("EndTime", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy(pGBLogicDeviceInfo->end_time, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->end_time:%s", pGBLogicDeviceInfo->end_time);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->end_time NULL");
    }


    /* 保密属性 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Secrecy", tmp_ivalue);

    pGBLogicDeviceInfo->secrecy = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->error_code:%d", pGBLogicDeviceInfo->secrecy);


    /* IP地址*/
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("IPAddress", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy(pGBLogicDeviceInfo->ip_address, (char*)tmp_svalue.c_str(), MAX_IP_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->ip_address:%s", pGBLogicDeviceInfo->ip_address);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->ip_address NULL");
    }


    /* 端口号 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Port", tmp_ivalue);

    pGBLogicDeviceInfo->port = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->port:%d", pGBLogicDeviceInfo->port);


    /* 密码*/
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Password", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy(pGBLogicDeviceInfo->password, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->password:%s", pGBLogicDeviceInfo->password);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->password NULL");
    }


    /* 状态 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Status", tmp_ivalue);

    pGBLogicDeviceInfo->status = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->status:%d", pGBLogicDeviceInfo->status);


    /* 经度 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Longitude", tmp_svalue);

    pGBLogicDeviceInfo->longitude = strtod(tmp_svalue.c_str(), (char**) NULL);
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->longitude:%f", pGBLogicDeviceInfo->longitude);


    /* 纬度 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Latitude", tmp_svalue);

    pGBLogicDeviceInfo->latitude = strtod(tmp_svalue.c_str(), (char**) NULL);
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id:pGBLogicDeviceInfo->latitude : %f \r\n", pGBLogicDeviceInfo->latitude);

    /* 所属的图层 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Resved2", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy(pGBLogicDeviceInfo->map_layer, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->map_layer:%s", pGBLogicDeviceInfo->map_layer);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->map_layer NULL");
    }

    /* 鹰眼相机对应的预案ID */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Resved4", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy(pGBLogicDeviceInfo->strResved2, (char*)tmp_svalue.c_str(), MAX_32CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->map_layer:%s", pGBLogicDeviceInfo->map_layer);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->strResved2 NULL");
    }

    if (pGBLogicDeviceInfo->id <= 0 || '\0' == pGBLogicDeviceInfo->device_id[0])
    {
        GBLogicDevice_info_free(pGBLogicDeviceInfo);
        osip_free(pGBLogicDeviceInfo);
        pGBLogicDeviceInfo = NULL;
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_db_data_to_LogicDevice_info_list_by_device_id() device_id Error");
        return -1;
    }

    /* 增加物理设备信息 */
    pGBDeviceInfo = GBDevice_info_find_by_device_index(pGBLogicDeviceInfo->phy_mediaDeviceIndex);

    gbdevice_pos = GBDevice_add(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER, pGBDeviceInfo);

    if (gbdevice_pos < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_db_data_to_LogicDevice_info_list_by_device_id() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_MASTER Error:i=%d \r\n", pGBLogicDeviceInfo->device_id, gbdevice_pos);
    }
    else
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_MASTER OK:i=%d \r\n", pGBLogicDeviceInfo->device_id, gbdevice_pos);
    }

    if (pGBLogicDeviceInfo->stream_count > 1)
    {
        gbdevice_pos = GBDevice_add(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_SLAVE, pGBDeviceInfo);

        if (gbdevice_pos < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_db_data_to_LogicDevice_info_list_by_device_id() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_SLAVE Error:i=%d \r\n", pGBLogicDeviceInfo->device_id, gbdevice_pos);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_SLAVE OK:i=%d \r\n", pGBLogicDeviceInfo->device_id, gbdevice_pos);
        }
    }

    /* 加入逻辑设备队列 */
    if (GBLogicDevice_info_add(pGBLogicDeviceInfo) < 0)
    {
        GBLogicDevice_info_free(pGBLogicDeviceInfo);
        osip_free(pGBLogicDeviceInfo);
        pGBLogicDeviceInfo = NULL;
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_db_data_to_LogicDevice_info_list_by_device_id() GBLogicDevice Info Add Error");
        return -1;
    }

    /* 发送变化到上级Route */
    iRet = SendNotifyCatalogToRouteCMS(pGBLogicDeviceInfo, 0, ptDBOper);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "load_db_data_to_LogicDevice_info_list_by_device_id() SendNotifyCatalogToRouteCMS Error:device_id=%s, i=%d \r\n", pGBLogicDeviceInfo->device_id, iRet);
    }
    else if (iRet > 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "load_db_data_to_LogicDevice_info_list_by_device_id() SendNotifyCatalogToRouteCMS OK:device_id=%s, i=%d \r\n", pGBLogicDeviceInfo->device_id, iRet);
    }

    /* 发送添加消息给下级CMS  */
    iRet = SendNotifyCatalogToSubCMS(pGBLogicDeviceInfo, 0, ptDBOper);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "load_db_data_to_LogicDevice_info_list_by_device_id() SendNotifyCatalogToSubCMS Error:iRet=%d \r\n", iRet);
    }
    else if (iRet > 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "load_db_data_to_LogicDevice_info_list_by_device_id() SendNotifyCatalogToSubCMS OK:iRet=%d \r\n", iRet);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : load_GBLogicDevice_info_from_db_by_device_id
 功能描述  : 通过DeviceID获取数据库的逻辑设备信息
 输入参数  : DBOper * ptDBOper
             char* device_id
             GBLogicDevice_info_t** pGBLogicDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月26日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int load_GBLogicDevice_info_from_db_by_device_id(DBOper * ptDBOper, char* device_id, GBLogicDevice_info_t** pGBLogicDeviceInfo)
{
    int iRet = 0;
    string strSQL = "";
    int record_count = 0;
    int tmp_ivalue = 0;
    unsigned int tmp_uivalue = 0;
    string tmp_svalue = "";
    GBDevice_info_t* pGBDeviceInfo = NULL;
    int gbdevice_pos = 0;

    if (NULL == device_id || ptDBOper == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "load_GBLogicDevice_info_from_db_by_device_id() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from GBLogicDeviceConfig WHERE DeviceID like '";
    strSQL += device_id;
    strSQL += "'";

    record_count = ptDBOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_GBLogicDevice_info_from_db_by_device_id() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_GBLogicDevice_info_from_db_by_device_id() ErrorMsg=%s\r\n", ptDBOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "load_GBLogicDevice_info_from_db_by_device_id() exit---: No Record Count \r\n");
        return 0;
    }
    else if (record_count != 1)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_GBLogicDevice_info_from_db_by_device_id() exit---: Record Count Error: record_count=%d\r\n", record_count);
        return -1;
    }

    iRet = GBLogicDevice_info_init(pGBLogicDeviceInfo);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_GBLogicDevice_info_from_db_by_device_id() GBLogicDevice_info_init:iRet=%d", iRet);
        return -1;
    }

    /* 设备索引*/
    tmp_uivalue = 0;
    ptDBOper->GetFieldValue("ID", tmp_uivalue);

    (*pGBLogicDeviceInfo)->id = tmp_uivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->id:%u", pGBLogicDeviceInfo->id);


    /* 点位统一编号 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("DeviceID", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy((*pGBLogicDeviceInfo)->device_id, (char*)tmp_svalue.c_str(), MAX_ID_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->device_id:%s", pGBLogicDeviceInfo->device_id);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->device_id NULL");
    }


    /* 所属的CMS 统一编号 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("CMSID", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy((*pGBLogicDeviceInfo)->cms_id, (char*)tmp_svalue.c_str(), MAX_ID_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->cms_id:%s", pGBLogicDeviceInfo->cms_id);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->cms_id NULL");
    }


    /* 点位名称 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("DeviceName", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy((*pGBLogicDeviceInfo)->device_name, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->device_name:%s", pGBLogicDeviceInfo->device_name);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->device_name NULL");
    }


    /* 是否启用 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Enable", tmp_ivalue);

    (*pGBLogicDeviceInfo)->enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->enable:%d", pGBLogicDeviceInfo->enable);


    /* 设备类型*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("DeviceType", tmp_ivalue);

    (*pGBLogicDeviceInfo)->device_type = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->device_type:%d", pGBLogicDeviceInfo->device_type);


    /* 报警设备子类型 */
    tmp_uivalue = 0;
    ptDBOper->GetFieldValue("Resved1", tmp_uivalue);

    (*pGBLogicDeviceInfo)->alarm_device_sub_type = tmp_uivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->alarm_device_sub_type:%u", pGBLogicDeviceInfo->alarm_device_sub_type);


    /* 是否可控*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("CtrlEnable", tmp_ivalue);

    (*pGBLogicDeviceInfo)->ctrl_enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->ctrl_enable:%d", pGBLogicDeviceInfo->ctrl_enable);


    /* 是否支持对讲，默认值0 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("MicEnable", tmp_ivalue);

    (*pGBLogicDeviceInfo)->mic_enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->mic_enable:%d", pGBLogicDeviceInfo->mic_enable);


    /* 帧率，默认25 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("FrameCount", tmp_ivalue);

    (*pGBLogicDeviceInfo)->frame_count = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->frame_count:%d", pGBLogicDeviceInfo->frame_count);


    /* 告警时长 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("AlarmLengthOfTime", tmp_ivalue);

    (*pGBLogicDeviceInfo)->alarm_duration = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->alarm_duration:%d", pGBLogicDeviceInfo->alarm_duration);


    /* 对应的媒体物理设备索引*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("PhyDeviceIndex", tmp_ivalue);

    (*pGBLogicDeviceInfo)->phy_mediaDeviceIndex = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->phy_mediaDeviceIndex:%d", pGBLogicDeviceInfo->phy_mediaDeviceIndex);


    /* 对应的媒体物理设备通道 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("PhyDeviceChannel", tmp_ivalue);

    (*pGBLogicDeviceInfo)->phy_mediaDeviceChannel = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->phy_mediaDeviceChannel:%d", pGBLogicDeviceInfo->phy_mediaDeviceChannel);


    /* 对应的控制物理设备索引 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("CtrlDeviceIndex", tmp_ivalue);

    (*pGBLogicDeviceInfo)->phy_ctrlDeviceIndex = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->phy_ctrlDeviceIndex:%d", pGBLogicDeviceInfo->phy_ctrlDeviceIndex);


    /* 对应的控制物理设备通道 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("CtrlDeviceChannel", tmp_ivalue);

    (*pGBLogicDeviceInfo)->phy_ctrlDeviceChannel = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->phy_ctrlDeviceChannel:%d", pGBLogicDeviceInfo->phy_ctrlDeviceChannel);


    /* 是否支持多码流 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("StreamCount", tmp_ivalue);

    (*pGBLogicDeviceInfo)->stream_count = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->stream_count:%d", pGBLogicDeviceInfo->stream_count);


    /* 录像类型 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("RecordType", tmp_ivalue);

    (*pGBLogicDeviceInfo)->record_type = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->record_type:%d", pGBLogicDeviceInfo->record_type);


    /* 设备生产商 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Manufacturer", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy((*pGBLogicDeviceInfo)->manufacturer, tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->manufacturer:%s", pGBLogicDeviceInfo->manufacturer);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->manufacturer NULL");
    }


    /* 设备型号 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Model", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy((*pGBLogicDeviceInfo)->model, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->model:%s", pGBLogicDeviceInfo->model);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->model NULL");
    }


    /* 设备归属 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Owner", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy((*pGBLogicDeviceInfo)->owner, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->owner:%s", pGBLogicDeviceInfo->owner);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->owner NULL");
    }

#if 0
    /* 行政区域 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("CivilCode", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        (*pGBLogicDeviceInfo)->civil_code = osip_getcopy(tmp_svalue.c_str());
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->civil_code:%s", pGBLogicDeviceInfo->civil_code);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->civil_code NULL");
    }

#endif

    /* 警区 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Block", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy((*pGBLogicDeviceInfo)->block, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->block:%s", pGBLogicDeviceInfo->block);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->block NULL");
    }


    /* 安装地址 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Address", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy((*pGBLogicDeviceInfo)->address, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->address:%s", pGBLogicDeviceInfo->address);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->address NULL");
    }


    /* 是否有子设备 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Parental", tmp_ivalue);

    (*pGBLogicDeviceInfo)->parental = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->parental:%d", pGBLogicDeviceInfo->parental);


    /* 父设备/区域/系统ID */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("ParentID", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy((*pGBLogicDeviceInfo)->parentID, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->parentID:%s", pGBLogicDeviceInfo->parentID);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->parentID NULL");
    }


    /* 信令安全模式*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("SafetyWay", tmp_ivalue);

    (*pGBLogicDeviceInfo)->safety_way = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->safety_way:%d", pGBLogicDeviceInfo->safety_way);


    /* 注册方式 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("RegisterWay", tmp_ivalue);

    (*pGBLogicDeviceInfo)->register_way = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->safety_way:%d", pGBLogicDeviceInfo->register_way);


    /* 证书序列号*/
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("CertNum", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy((*pGBLogicDeviceInfo)->cert_num, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->cert_num:%s", pGBLogicDeviceInfo->cert_num);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->cert_num NULL");
    }


    /* 证书有效标识 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Certifiable", tmp_ivalue);

    (*pGBLogicDeviceInfo)->certifiable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->certifiable:%d", pGBLogicDeviceInfo->certifiable);


    /* 无效原因码 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("ErrCode", tmp_ivalue);

    (*pGBLogicDeviceInfo)->error_code = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->error_code:%d", pGBLogicDeviceInfo->error_code);


    /* 证书终止有效期*/
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("EndTime", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy((*pGBLogicDeviceInfo)->end_time, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->end_time:%s", pGBLogicDeviceInfo->end_time);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->end_time NULL");
    }


    /* 保密属性 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Secrecy", tmp_ivalue);

    (*pGBLogicDeviceInfo)->secrecy = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->error_code:%d", pGBLogicDeviceInfo->secrecy);


    /* IP地址*/
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("IPAddress", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy((*pGBLogicDeviceInfo)->ip_address, (char*)tmp_svalue.c_str(), MAX_IP_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->ip_address:%s", pGBLogicDeviceInfo->ip_address);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->ip_address NULL");
    }


    /* 端口号 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Port", tmp_ivalue);

    (*pGBLogicDeviceInfo)->port = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->port:%d", pGBLogicDeviceInfo->port);


    /* 密码*/
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Password", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy((*pGBLogicDeviceInfo)->password, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->password:%s", pGBLogicDeviceInfo->password);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->password NULL");
    }


    /* 状态 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Status", tmp_ivalue);

    (*pGBLogicDeviceInfo)->status = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->status:%d", pGBLogicDeviceInfo->status);


    /* 经度 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Longitude", tmp_svalue);

    (*pGBLogicDeviceInfo)->longitude = strtod(tmp_svalue.c_str(), (char**) NULL);
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->longitude:%f", pGBLogicDeviceInfo->longitude);


    /* 纬度 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Latitude", tmp_svalue);

    (*pGBLogicDeviceInfo)->latitude = strtod(tmp_svalue.c_str(), (char**) NULL);
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id:pGBLogicDeviceInfo->latitude : %f \r\n", pGBLogicDeviceInfo->latitude);

    /* 所属的图层 */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Resved2", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy((*pGBLogicDeviceInfo)->map_layer, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->map_layer:%s", pGBLogicDeviceInfo->map_layer);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->map_layer NULL");
    }

    /* 鹰眼相机对应的预案ID */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Resved4", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        osip_strncpy((*pGBLogicDeviceInfo)->strResved2, (char*)tmp_svalue.c_str(), MAX_32CHAR_STRING_LEN);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->map_layer:%s", pGBLogicDeviceInfo->map_layer);
    }
    else
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->strResved2 NULL");
    }

    /* 增加物理设备信息 */
    pGBDeviceInfo = GBDevice_info_find_by_device_index((*pGBLogicDeviceInfo)->phy_mediaDeviceIndex);

    gbdevice_pos = GBDevice_add((*pGBLogicDeviceInfo), EV9000_STREAM_TYPE_MASTER, pGBDeviceInfo);

    if (gbdevice_pos < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_GBLogicDevice_info_from_db_by_device_id() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_MASTER Error:i=%d \r\n", (*pGBLogicDeviceInfo)->device_id, gbdevice_pos);
    }
    else
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "load_GBLogicDevice_info_from_db_by_device_id() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_MASTER OK:i=%d \r\n", (*pGBLogicDeviceInfo)->device_id, gbdevice_pos);
    }

    if ((*pGBLogicDeviceInfo)->stream_count > 1)
    {
        gbdevice_pos = GBDevice_add((*pGBLogicDeviceInfo), EV9000_STREAM_TYPE_SLAVE, pGBDeviceInfo);

        if (gbdevice_pos < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_GBLogicDevice_info_from_db_by_device_id() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_SLAVE Error:i=%d \r\n", (*pGBLogicDeviceInfo)->device_id, gbdevice_pos);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "load_GBLogicDevice_info_from_db_by_device_id() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_SLAVE OK:i=%d \r\n", (*pGBLogicDeviceInfo)->device_id, gbdevice_pos);
        }
    }

    return record_count;
}

/*****************************************************************************
 函 数 名  : check_GBLogicDevice_info_from_db
 功能描述  : 检测逻辑设备数据库中的配置数据库是否有变化，如果有变化，则同步
             到内存中
 输入参数  : DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月23日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int check_GBLogicDevice_info_from_db_to_list(DBOper* pdboper)
{
    //int i = 0;
    int iRet = 0;
    int index = 0;
    int record_count = 0; /* 记录数 */
    int change_flag = 0;

    vector<string> DeviceIDVector;

    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_info_t* pDBGBLogicDeviceInfo = NULL;

    DeviceIDVector.clear();

    /* 1、添加所有的逻辑设备 */
    iRet = AddAllGBLogicDeviceIDToVector(DeviceIDVector, pdboper);

    /* 2、获取容器中的设备个数 */
    record_count = DeviceIDVector.size();

    printf("check_GBLogicDevice_info_from_db() DB Record:record_count=%d \r\n", record_count);

    /* 3、如果记录数为0 */
    if (record_count == 0)
    {
        return record_count;
    }

    for (index = 0; index < record_count; index++)
    {
        /* 根据Index 获取逻辑设备信息，可能是只配置了物理设备，还没有上线，数据库和内存中都没有的 */
        pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

        if (NULL != pGBLogicDeviceInfo)
        {
            pGBLogicDeviceInfo->del_mark = 0;

            /* 可能数据库中有变化，需要更新到内存 */
            iRet = load_GBLogicDevice_info_from_db_by_device_id(pdboper, (char*)DeviceIDVector[index].c_str(), &pDBGBLogicDeviceInfo);

            if (iRet > 0)
            {
                change_flag = IsGBLogicDeviceInfoHasChange(pGBLogicDeviceInfo, pDBGBLogicDeviceInfo, 1);

                if (change_flag) /* 看数据库中是否有变化 */
                {
                    /* 更新内存 */
                    iRet = GBLogicDevice_info_update(pGBLogicDeviceInfo, pDBGBLogicDeviceInfo, 1);

                    if (0 != iRet)
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "check_GBLogicDevice_info_from_db() GBLogicDevice_info_update Error:device_id=%s, i=%d \r\n", pGBLogicDeviceInfo->device_id, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "check_GBLogicDevice_info_from_db() GBLogicDevice_info_update OK:device_id=%s, i=%d \r\n", pGBLogicDeviceInfo->device_id, iRet);
                    }

                    if (pGBLogicDeviceInfo->enable)
                    {
                        /* 发送变化到上级Route */
                        iRet = SendNotifyCatalogToRouteCMS(pGBLogicDeviceInfo, 2, pdboper);

                        if (iRet < 0)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "check_GBLogicDevice_info_from_db() SendNotifyCatalogToRouteCMS Error:device_id=%s, i=%d \r\n", pGBLogicDeviceInfo->device_id, iRet);
                        }
                        else if (iRet > 0)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "check_GBLogicDevice_info_from_db() SendNotifyCatalogToRouteCMS OK:device_id=%s, i=%d \r\n", pGBLogicDeviceInfo->device_id, iRet);
                        }
                    }
                    else
                    {
                        if (3 != change_flag)
                        {
                            /* 发送变化到上级Route */
                            iRet = SendNotifyCatalogToRouteCMS(pGBLogicDeviceInfo, 1, pdboper);

                            if (iRet < 0)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "check_GBLogicDevice_info_from_db() SendNotifyCatalogToRouteCMS Error:device_id=%s, i=%d \r\n", pGBLogicDeviceInfo->device_id, iRet);
                            }
                            else if (iRet > 0)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "check_GBLogicDevice_info_from_db() SendNotifyCatalogToRouteCMS OK:device_id=%s, i=%d \r\n", pGBLogicDeviceInfo->device_id, iRet);
                            }
                        }
                    }
                }

                GBLogicDevice_info_free(pDBGBLogicDeviceInfo);
                osip_free(pDBGBLogicDeviceInfo);
                pDBGBLogicDeviceInfo = NULL;
            }
        }
        else
        {
            iRet = load_db_data_to_LogicDevice_info_list_by_device_id(pdboper, (char*)DeviceIDVector[index].c_str());
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "check_GBLogicDevice_info_from_db() load_db_data_to_LogicDevice_info_list_by_device_id:device_id=%s, i=%d \r\n", (char*)DeviceIDVector[index].c_str(), iRet);
        }
    }

    DeviceIDVector.clear();

    /* 获取上海地标对应的通道信息 */
    iRet = check_shdb_channel_info_from_db(pdboper);

    return iRet;
}

/*****************************************************************************
 函 数 名  : check_shdb_channel_info_from_db
 功能描述  : 加载上海地标对应的通道ID到逻辑设备
 输入参数  : DBOper * pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月23日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int check_shdb_channel_info_from_db(DBOper * pDboper)
{
    int record_count = 0;
    string strSQL = "";
    int while_count = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pDboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "check_shdb_channel_info_from_db() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from DiBiaoChannelMapConfig";

    record_count = pDboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_shdb_channel_info_from_db() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_shdb_channel_info_from_db() ErrorMsg=%s\r\n", pDboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "check_shdb_channel_info_from_db() exit---: No Record Count \r\n");
        return -1;
    }

    printf("\r\n check_shdb_channel_info_from_db() DB Record:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);

    do
    {
        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "check_shdb_channel_info_from_db() While Count=%d \r\n", while_count);
        }

        unsigned int device_index = 0;
        unsigned int iChannelID = 0;

        /* 通道索引 */
        iChannelID = 0;
        pDboper->GetFieldValue("ChannelID", iChannelID);

        /* 设备索引 */
        device_index = 0;
        pDboper->GetFieldValue("DeviceIndex", device_index);

        if (iChannelID <= 0 || device_index <= 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "check_shdb_channel_info_from_db() DeviceIndex Or DeviceIndex Error: ChannelID=%d, DeviceIndex=%u \r\n", iChannelID, device_index);
            continue;
        }

        pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(device_index);

        if (NULL != pGBLogicDeviceInfo
            && pGBLogicDeviceInfo->shdb_channel_id != iChannelID)
        {
            pGBLogicDeviceInfo->shdb_channel_id = iChannelID;
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "check_shdb_channel_info_from_db() DeviceID=%s, ChannelID=%u \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->shdb_channel_id);
        }
    }
    while (pDboper->MoveNext() >= 0);


    return 0;
}

/*****************************************************************************
 函 数 名  : SetGBLogicDeviceStatus
 功能描述  : 设置标准逻辑设备状态
 输入参数  : int device_index
             int status
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月19日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SetGBLogicDeviceStatus(int device_index, int status, DBOper * pDevice_Srv_dboper)
{
    int iRet = 0;
    int index = -1;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    vector<string> DeviceIDVector;

    if (device_index <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SetGBLogicDeviceStatus() exit---: Param Error \r\n");
        return -1;
    }

    DeviceIDVector.clear();

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->phy_mediaDeviceIndex <= 0))
        {
            continue;
        }

        if (pGBLogicDeviceInfo->phy_mediaDeviceIndex == device_index)
        {
            pGBLogicDeviceInfo->status = status;

            if (0 == status)
            {
                pGBLogicDeviceInfo->alarm_status = ALARM_STATUS_NULL;
            }

            DeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    if (DeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)DeviceIDVector.size(); index++)
        {
            /* 更新数据库 */
            iRet = UpdateGBLogicDeviceRegStatus2DB((char*)DeviceIDVector[index].c_str(), status, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetGBLogicDeviceStatus() UpdateGBLogicDeviceRegStatus2DB:iRet=%d \r\n", iRet);
            }

            if (0 == status)
            {
                pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

                if (NULL != pGBLogicDeviceInfo && EV9000_DEVICETYPE_SCREEN != pGBLogicDeviceInfo->device_type)
                {
                    //iRet = shdb_device_operate_cmd_proc(pGBLogicDeviceInfo, EV9000_SHDB_DVR_VIDEO_LOSS, pDevice_Srv_dboper);
                }
            }
        }
    }

    DeviceIDVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : RemoveGBLogicDevice
 功能描述  : 根据物理设备ID移除逻辑设备
 输入参数  : int device_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月10日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int RemoveGBLogicDevice(int device_index)
{
    int index = -1;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    vector<string> DeviceIDVector;

    if (device_index <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "RemoveGBLogicDevice() exit---: Param Error \r\n");
        return -1;
    }

    DeviceIDVector.clear();

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->phy_mediaDeviceIndex <= 0))
        {
            continue;
        }

        if (pGBLogicDeviceInfo->phy_mediaDeviceIndex == device_index)
        {
            DeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    if (DeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)DeviceIDVector.size(); index++)
        {
            pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

            if (NULL != pGBLogicDeviceInfo)
            {
                GBLogicDevice_info_remove((char*)DeviceIDVector[index].c_str());

                GBLogicDevice_info_free(pGBLogicDeviceInfo);
                osip_free(pGBLogicDeviceInfo);
                pGBLogicDeviceInfo = NULL;
            }
        }
    }

    DeviceIDVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : SetGBLogicDeviceIntelligentStatus
 功能描述  : 设置逻辑设备的智能分析状态
 输入参数  : int device_index
             intelligent_status_t status
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月23日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SetGBLogicDeviceIntelligentStatus(int device_index, intelligent_status_t status)
{
    int iRet = 0;
    int index = -1;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    vector<string> DeviceIDVector;
    GBDevice_info_t* pGBDeviceInfo = NULL;

    if (device_index <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SetGBLogicDeviceIntelligentStatus() exit---: Param Error \r\n");
        return -1;
    }

    DeviceIDVector.clear();

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->phy_mediaDeviceIndex <= 0))
        {
            continue;
        }

        pGBDeviceInfo = GBDevice_info_get_by_stream_type2(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_INTELLIGENCE);

        if (NULL != pGBDeviceInfo)
        {
            if (pGBDeviceInfo->id == device_index)
            {
                pGBLogicDeviceInfo->intelligent_status = status;
                DeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
            }
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    if (DeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)DeviceIDVector.size(); index++)
        {
            pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

            if (NULL != pGBLogicDeviceInfo)
            {
                iRet = GBDevice_remove(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_INTELLIGENCE);

                if (iRet < 0)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDevice_info_update() GBDevice_remove:device_id=%s, stream_type=EV9000_STREAM_TYPE_INTELLIGENCE Error:i=%d \r\n", pGBLogicDeviceInfo->device_id, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GBLogicDevice_info_update() GBDevice_remove:device_id=%s, stream_type=EV9000_STREAM_TYPE_INTELLIGENCE OK:i=%d \r\n", pGBLogicDeviceInfo->device_id, iRet);
                }
            }
        }
    }

    DeviceIDVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : IsGBLogicDeviceInfoHasChangeForRCU
 功能描述  : 标准逻辑设备信息是否有变化
 输入参数  : GBLogicDevice_info_t* pOldGBLogicDeviceInfo
             GBLogicDevice_info_t* pNewGBLogicDeviceInfo
             int change_type:0:内存到数据库的比较，1:数据库到内存的比较
 输出参数  : 无
 返 回 值  :int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int IsGBLogicDeviceInfoHasChangeForRCU(GBLogicDevice_info_t * pOldGBLogicDeviceInfo, GBLogicDevice_info_t * pNewGBLogicDeviceInfo)
{
    if (NULL == pOldGBLogicDeviceInfo || NULL == pNewGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "IsGBLogicDeviceInfoHasChangeForRCU() exit---: Param Error \r\n");
        return 0;
    }

    if (pNewGBLogicDeviceInfo->device_type >= EV9000_DEVICETYPE_RCU_YAOXIN && pNewGBLogicDeviceInfo->device_type <= EV9000_DEVICETYPE_RCU_MINGLIN)
    {
        /* RCU Value */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->Value, pOldGBLogicDeviceInfo->Value))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRCU() exit---: CHANGE Value 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* RCU Unit */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->Unit, pOldGBLogicDeviceInfo->Unit))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRCU() exit---: CHANGE Unit 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* RCU AlarmPriority */
        if (pOldGBLogicDeviceInfo->AlarmPriority != pNewGBLogicDeviceInfo->AlarmPriority)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端RCU逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, RCU上报的AlarmPriority发生变化: 老的AlarmPriority=%d, 新的AlarmPriority=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->AlarmPriority, pNewGBLogicDeviceInfo->AlarmPriority);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end RCU logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The AlarmPriority reported by RCU change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->AlarmPriority, pNewGBLogicDeviceInfo->AlarmPriority);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRCU() exit---: CHANGE AlarmPriority: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* RCU Guard */
        if (pOldGBLogicDeviceInfo->guard_type != pNewGBLogicDeviceInfo->guard_type)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端RCU逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, RCU上报的Guard发生变化: 老的Guard=%d, 新的Guard=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->guard_type, pNewGBLogicDeviceInfo->guard_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front RCU end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The Guard reported by RCU change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->guard_type, pNewGBLogicDeviceInfo->guard_type);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRCU() exit---: CHANGE AlarmPriority: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "IsGBLogicDeviceInfoHasChangeForRCU() exit---: NO CHANGE \r\n");
    return 0;
}

/*****************************************************************************
 函 数 名  : IsGBLogicDeviceInfoHasChange
 功能描述  : 标准逻辑设备信息是否有变化
 输入参数  : GBLogicDevice_info_t* pOldGBLogicDeviceInfo
             GBLogicDevice_info_t* pNewGBLogicDeviceInfo
             int change_type:0:内存到数据库的比较，1:数据库到内存的比较
 输出参数  : 无
 返 回 值  :int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int IsGBLogicDeviceInfoHasChange(GBLogicDevice_info_t * pOldGBLogicDeviceInfo, GBLogicDevice_info_t * pNewGBLogicDeviceInfo, int change_type)
{
    GBDevice_info_t* pGBDeviceInfo = NULL;

    if (NULL == pOldGBLogicDeviceInfo || NULL == pNewGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "IsGBLogicDeviceInfoHasChange() exit---: Param Error \r\n");
        return 0;
    }

    pGBDeviceInfo = GBDevice_info_get_by_stream_type(pNewGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "IsGBLogicDeviceInfoHasChange() exit---: GBDevice_info_get_by_stream_type Error \r\n");
        return 0;
    }

    if (0 == change_type) /* 以前端上报为准，内存到数据库的比较 */
    {
        /* 点位名称 */
        if (('\0' != pNewGBLogicDeviceInfo->device_name[0]
             && (0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IP Camera", 9)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPCamera", 8)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"Camera", 6)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPC", 3)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"通道", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"固枪", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"动球", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"半球", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_id, MAX_ID_LEN)))
            || '\0' == pNewGBLogicDeviceInfo->device_name[0])
        {
            /* 如果前端上报的是 IP Camera，Camera，的名称，或者上报的为空，则不去比较，以数据库配置为准 */
        }
        else
        {
            if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 前端上报的点位名称发生变化: 老的点位名称=%s, 新的点位名称=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point's name change: Old Poit Name=%s,New Poit Name=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE device_name 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }
        }

        /* 是否启用 */
        if (pOldGBLogicDeviceInfo->enable != pNewGBLogicDeviceInfo->enable)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 前端上报的点位禁用标识发生变化: 老的禁用标识=%d, 新的禁用标识=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->enable, pNewGBLogicDeviceInfo->enable);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point's disable mark change: Old Poit Mark=%s,New Poit Mark=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->enable, pNewGBLogicDeviceInfo->enable);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 报警输入输出通道，需要更新报警设备子类型 */
        if (pOldGBLogicDeviceInfo->alarm_device_sub_type != pNewGBLogicDeviceInfo->alarm_device_sub_type)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 前端上报的点位报警设备类型发生变化: 老的报警设备类型=%u, 新的报警设备类型=%u", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_device_sub_type, pNewGBLogicDeviceInfo->alarm_device_sub_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point's alarm type change: Old alarm type=%s,New alarm type=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name,  pOldGBLogicDeviceInfo->alarm_device_sub_type, pNewGBLogicDeviceInfo->alarm_device_sub_type);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE alarm_device_sub_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 是否属于其他域名 */
        if (pOldGBLogicDeviceInfo->other_realm != pNewGBLogicDeviceInfo->other_realm)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 前端上报的点位是否属于其他域名标识发生变化: 老的是否属于其他域名标识=%d, 新的是否属于其他域名标识=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->other_realm, pNewGBLogicDeviceInfo->other_realm);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, Front-end reporting point whether belong to other domain name change: Old =%s,New =%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->other_realm, pNewGBLogicDeviceInfo->other_realm);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE other_realm: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 对应的媒体物理设备索引 */
        if (pOldGBLogicDeviceInfo->phy_mediaDeviceIndex != pNewGBLogicDeviceInfo->phy_mediaDeviceIndex)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE phy_mediaDeviceIndex: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 设备生产商 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->manufacturer, pOldGBLogicDeviceInfo->manufacturer))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE manufacturer 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 设备型号 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->model, pOldGBLogicDeviceInfo->model))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE model 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 设备归属 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->owner, pOldGBLogicDeviceInfo->owner))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE owner 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

#if 0

        /* 行政区域 */
        if (NULL != pNewGBLogicDeviceInfo->civil_code && NULL != pOldGBLogicDeviceInfo->civil_code
            && 0 != sstrcmp(pNewGBLogicDeviceInfo->civil_code, pOldGBLogicDeviceInfo->civil_code))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE 18 \r\n");
            return 1;
        }
        else if (NULL == pNewGBLogicDeviceInfo->civil_code && NULL != pOldGBLogicDeviceInfo->civil_code)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE 19 \r\n");
            return 1;
        }
        else if (NULL != pNewGBLogicDeviceInfo->civil_code && NULL == pOldGBLogicDeviceInfo->civil_code)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE 20 \r\n");
            return 1;
        }

#endif

        /* 警区 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->block, pOldGBLogicDeviceInfo->block))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE block 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 安装地址 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->address, pOldGBLogicDeviceInfo->address))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE address 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 是否有子设备 */
        if (pOldGBLogicDeviceInfo->parental != pNewGBLogicDeviceInfo->parental)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE parental: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 父设备/区域/系统ID */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->parentID, pOldGBLogicDeviceInfo->parentID))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE parentID 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 信令安全模式*/
        if (pOldGBLogicDeviceInfo->safety_way != pNewGBLogicDeviceInfo->safety_way)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE safety_way: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 注册方式 */
        if (pOldGBLogicDeviceInfo->register_way != pNewGBLogicDeviceInfo->register_way)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE register_way: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 证书序列号*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->cert_num, pOldGBLogicDeviceInfo->cert_num))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE cert_num 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 证书有效标识 */
        if (pOldGBLogicDeviceInfo->certifiable != pNewGBLogicDeviceInfo->certifiable)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE certifiable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 无效原因码 */
        if (pOldGBLogicDeviceInfo->error_code != pNewGBLogicDeviceInfo->error_code)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE error_code: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 证书终止有效期*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->end_time, pOldGBLogicDeviceInfo->end_time))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE end_time 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 保密属性 */
        if (pOldGBLogicDeviceInfo->secrecy != pNewGBLogicDeviceInfo->secrecy)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE secrecy: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* IP地址*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->ip_address, pOldGBLogicDeviceInfo->ip_address))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE ip_address 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 端口号*/
        if (pOldGBLogicDeviceInfo->port != pNewGBLogicDeviceInfo->port)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE port: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 密码*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->password, pOldGBLogicDeviceInfo->password))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE password 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* RCU Value */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->Value, pOldGBLogicDeviceInfo->Value))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE Value 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* RCU Unit */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->Unit, pOldGBLogicDeviceInfo->Unit))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE Unit 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* RCU AlarmPriority */
        if (pOldGBLogicDeviceInfo->AlarmPriority != pNewGBLogicDeviceInfo->AlarmPriority)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, RCU上报的AlarmPriority发生变化: 老的AlarmPriority=%d, 新的AlarmPriority=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->AlarmPriority, pNewGBLogicDeviceInfo->AlarmPriority);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The AlarmPriority reported by RCU change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->AlarmPriority, pNewGBLogicDeviceInfo->AlarmPriority);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE AlarmPriority: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* RCU Guard */
        if (pOldGBLogicDeviceInfo->guard_type != pNewGBLogicDeviceInfo->guard_type)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, RCU上报的Guard发生变化: 老的Guard=%d, 新的Guard=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->guard_type, pNewGBLogicDeviceInfo->guard_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The Guard reported by RCU change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->guard_type, pNewGBLogicDeviceInfo->guard_type);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE guard_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 点位类型 */
        if (pOldGBLogicDeviceInfo->device_type != pNewGBLogicDeviceInfo->device_type)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE device_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 所属的CMSID */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->cms_id, pOldGBLogicDeviceInfo->cms_id))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE cms_id 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 是否可控,如果前端上报的大于0，以前端上报的为准 */


        if (pNewGBLogicDeviceInfo->ctrl_enable > 0)
        {

            if (pOldGBLogicDeviceInfo->ctrl_enable != pNewGBLogicDeviceInfo->ctrl_enable)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 前端上报的点位控球标识发生变化: 老的控球标识=%d, 新的控球标识=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->ctrl_enable, pNewGBLogicDeviceInfo->ctrl_enable);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s,CMS reporting point ball logo at a lower level change: Old =%s,New =%s",  pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->ctrl_enable, pNewGBLogicDeviceInfo->ctrl_enable);
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE ctrl_enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }
        }

        /* 如果是下级平台的点位，以前端上报为准的字段 */
        if (NULL != pGBDeviceInfo
            && EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
            && 0 == pGBDeviceInfo->three_party_flag)
        {
            /* 音频对讲 */
            if (pOldGBLogicDeviceInfo->mic_enable != pNewGBLogicDeviceInfo->mic_enable)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 下级CMS上报的点位是否支持对讲标识发生变化: 老的是否支持对讲标识=%d, 新的是否支持对讲标识=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING,  "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower  CMS whether to support the intercom CMS mark change: old =%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE mic_enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* 帧率 */
            if (pOldGBLogicDeviceInfo->frame_count != pNewGBLogicDeviceInfo->frame_count)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 下级CMS上报的点位帧率发生变化: 老的帧率=%d, 新的帧率=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->frame_count, pNewGBLogicDeviceInfo->frame_count);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The lower CMS reporting point frame rate change: old fram rate=%d, new fram rate=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->frame_count, pNewGBLogicDeviceInfo->frame_count);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE frame_count: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* 报警时长 */
            if (pOldGBLogicDeviceInfo->alarm_duration != pNewGBLogicDeviceInfo->alarm_duration)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 下级CMS上报的点位告警时长发生变化: 老的告警时长=%d, 新的告警时长=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_duration, pNewGBLogicDeviceInfo->alarm_duration);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The lower CMS reporting point alarm duration change: old alarm duration=%d, new alarm duration=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_duration, pNewGBLogicDeviceInfo->alarm_duration);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE alarm_duration: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* 是否支持多码流 */
            if (pOldGBLogicDeviceInfo->stream_count != pNewGBLogicDeviceInfo->stream_count)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 下级CMS上报的点位是否支持多码流发生变化: 老的是否支持多码流=%d, 新的是否支持多码流=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS whether to support support more stream change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE stream_count: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* 经度 */
            if (pNewGBLogicDeviceInfo->longitude > 0 && pOldGBLogicDeviceInfo->longitude != pNewGBLogicDeviceInfo->longitude)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 下级CMS上报的点位经度发生变化: 老的经度=%f, 新的经度=%f", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->longitude, pNewGBLogicDeviceInfo->longitude);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS longitude change: old=%f, new=%f", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->longitude, pNewGBLogicDeviceInfo->longitude);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE longitude: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* 纬度 */
            if (pNewGBLogicDeviceInfo->latitude > 0 && pOldGBLogicDeviceInfo->latitude != pNewGBLogicDeviceInfo->latitude)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 下级CMS上报的点位纬度发生变化: 老的纬度=%f, 新的纬度=%f", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->latitude, pNewGBLogicDeviceInfo->latitude);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS latitude change: old=%f, new=%f", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->latitude, pNewGBLogicDeviceInfo->latitude);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE latitude: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* 所属图层 */
            if (pNewGBLogicDeviceInfo->map_layer[0] != '\0' && 0 != sstrcmp(pNewGBLogicDeviceInfo->map_layer, pOldGBLogicDeviceInfo->map_layer))
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE map_layer 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }
        }

        /* 如果是媒体网关的点位，以前端上报为准的字段 */
        if (EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
        {
            if (pOldGBLogicDeviceInfo->stream_count != pNewGBLogicDeviceInfo->stream_count)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 媒体网关上报的点位是否支持多码流发生变化: 老的是否支持多码流=%d, 新的是否支持多码流=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS whether to support support more stream change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE stream_count: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }
        }

        /* 点位状态 */
        if (pOldGBLogicDeviceInfo->status != pNewGBLogicDeviceInfo->status)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 前端上报点位状态发生变化: 老的状态=%d, 新的状态=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, Front-end reporting point status change: Old =%s,New =%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE status: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 2;
        }
    }

    if (1 == change_type) /* 以数据库为准，数据库到内存的比较 */
    {
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 数据库点位名称发生变化: 老的点位名称=%s, 新的点位名称=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, Front-end reporting point name change: Old =%s,New =%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE device_name 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 对应的媒体物理设备通道 */
        if (pOldGBLogicDeviceInfo->phy_mediaDeviceChannel != pNewGBLogicDeviceInfo->phy_mediaDeviceChannel)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE phy_mediaDeviceChannel: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 对应的控制物理设备索引 */
        if (pOldGBLogicDeviceInfo->phy_ctrlDeviceIndex != pNewGBLogicDeviceInfo->phy_ctrlDeviceIndex)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE phy_ctrlDeviceIndex: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 对应的控制物理设备通道 */
        if (pOldGBLogicDeviceInfo->phy_ctrlDeviceChannel != pNewGBLogicDeviceInfo->phy_ctrlDeviceChannel)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE phy_ctrlDeviceChannel: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 录像类型*/
        if (pOldGBLogicDeviceInfo->record_type != pNewGBLogicDeviceInfo->record_type)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 数据库点位录像类型发生变化: 老的录像类型=%d, 新的录像类型=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->record_type, pNewGBLogicDeviceInfo->record_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, Front-end reporting point record type change: Old =%s,New =%s",  pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->record_type, pNewGBLogicDeviceInfo->record_type);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE record_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 是否可控 */
        if (pOldGBLogicDeviceInfo->ctrl_enable != pNewGBLogicDeviceInfo->ctrl_enable)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 数据库点位控球标识发生变化: 老的控球标识=%d, 新的控球标识=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->ctrl_enable, pNewGBLogicDeviceInfo->ctrl_enable);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s,CMS reporting point ball logo at a lower level change: Old =%s,New =%s",  pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->ctrl_enable, pNewGBLogicDeviceInfo->ctrl_enable);
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE ctrl_enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 如果非下级平台的点位，以数据库为准的字段 */
        if (NULL != pGBDeviceInfo
            && EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type)
        {
            /* 音频对讲 */
            if (pOldGBLogicDeviceInfo->mic_enable != pNewGBLogicDeviceInfo->mic_enable)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 数据库点位是否支持对讲标识发生变化: 老的是否支持对讲标识=%d, 新的是否支持对讲标识=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING,  "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower  CMS whether to support the intercom CMS mark change: old =%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE mic_enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* 帧率 */
            if (pOldGBLogicDeviceInfo->frame_count != pNewGBLogicDeviceInfo->frame_count)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 数据库点位帧率发生变化: 老的帧率=%d, 新的帧率=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->frame_count, pNewGBLogicDeviceInfo->frame_count);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING,  "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower  CMS whether to support the intercom CMS mark change: old =%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE frame_count: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* 报警时长 */
            if (pOldGBLogicDeviceInfo->alarm_duration != pNewGBLogicDeviceInfo->alarm_duration)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 数据库点位告警时长发生变化: 老的告警时长=%d, 新的告警时长=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_duration, pNewGBLogicDeviceInfo->alarm_duration);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The lower CMS reporting point alarm duration change: old alarm duration=%d, new alarm duration=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_duration, pNewGBLogicDeviceInfo->alarm_duration);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE alarm_duration: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* 经度 */
            if (pOldGBLogicDeviceInfo->longitude != pNewGBLogicDeviceInfo->longitude)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 数据库点位经度发生变化: 老的经度=%.16lf, 新的经度=%.16lf", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->longitude, pNewGBLogicDeviceInfo->longitude);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS longitude change: old=%f, new=%f", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->longitude, pNewGBLogicDeviceInfo->longitude);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE longitude: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* 纬度 */
            if (pOldGBLogicDeviceInfo->latitude != pNewGBLogicDeviceInfo->latitude)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 数据库点位经度发生变化: 老的纬度=%.16lf, 新的纬度=%.16lf", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->latitude, pNewGBLogicDeviceInfo->latitude);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS latitude change: old=%f, new=%f", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->latitude, pNewGBLogicDeviceInfo->latitude);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE latitude: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* 所属图层 */
            if (0 != sstrcmp(pNewGBLogicDeviceInfo->map_layer, pOldGBLogicDeviceInfo->map_layer))
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE map_layer 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }
        }

        /* 鹰眼相机对应的预案ID */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->strResved2, pOldGBLogicDeviceInfo->strResved2))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE strResved2: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 3;
        }

        /* 如果非下级平台的点位,并且不是媒体网关的点位，以数据库为准的字段 */
        if (NULL != pGBDeviceInfo
            && EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type
            && EV9000_DEVICETYPE_MGWSERVER != pGBDeviceInfo->device_type)
        {
            /* 是否支持多码流 */
            if (pOldGBLogicDeviceInfo->stream_count != pNewGBLogicDeviceInfo->stream_count)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 数据库点位是否支持多码流发生变化: 老的是否支持多码流=%d, 新的是否支持多码流=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS whether to support support more stream change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE stream_count: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : IsGBLogicDeviceInfoHasChangeForRoute
 功能描述  : 标准逻辑设备信息是否有变化
 输入参数  : GBLogicDevice_info_t * pOldGBLogicDeviceInfo
             GBLogicDevice_info_t * pNewGBLogicDeviceInfo
             int change_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月10日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int IsGBLogicDeviceInfoHasChangeForRoute(GBLogicDevice_info_t * pOldGBLogicDeviceInfo, GBLogicDevice_info_t * pNewGBLogicDeviceInfo, int change_type)
{
    if (NULL == pOldGBLogicDeviceInfo || NULL == pNewGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: Param Error \r\n");
        return 0;
    }

    if (0 == change_type) /* 以前端上报为准，内存到数据库的比较 */
    {
        /* 点位名称 */
        if (('\0' != pNewGBLogicDeviceInfo->device_name[0]
             && (0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IP Camera", 9)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPCamera", 8)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"Camera", 6)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPC", 3)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"通道", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"固枪", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"动球", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"半球", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_id, MAX_ID_LEN)))
            || '\0' == pNewGBLogicDeviceInfo->device_name[0])
        {
            /* 如果前端上报的是 IP Camera，Camera，的名称，或者上报的为空，则不去比较，以数据库配置为准 */
        }
        else
        {
            if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 前端上报的点位名称发生变化: 老的点位名称=%s, 新的点位名称=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point's name change: Old Poit Name=%s,New Poit Name=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE device_name 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }
        }

        /* 是否启用 */
        if (pOldGBLogicDeviceInfo->enable != pNewGBLogicDeviceInfo->enable)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 前端上报的点位禁用标识发生变化: 老的禁用标识=%d, 新的禁用标识=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->enable, pNewGBLogicDeviceInfo->enable);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point's disable mark change: Old Poit Mark=%s,New Poit Mark=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->enable, pNewGBLogicDeviceInfo->enable);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 报警输入输出通道，需要更新报警设备子类型 */
        if (pOldGBLogicDeviceInfo->alarm_device_sub_type != pNewGBLogicDeviceInfo->alarm_device_sub_type)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 前端上报的点位报警设备类型发生变化: 老的报警设备类型=%u, 新的报警设备类型=%u", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_device_sub_type, pNewGBLogicDeviceInfo->alarm_device_sub_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point's alarm type change: Old alarm type=%s,New alarm type=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name,  pOldGBLogicDeviceInfo->alarm_device_sub_type, pNewGBLogicDeviceInfo->alarm_device_sub_type);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE alarm_device_sub_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 是否属于其他域名 */
        if (pOldGBLogicDeviceInfo->other_realm != pNewGBLogicDeviceInfo->other_realm)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 前端上报的点位是否属于其他域名标识发生变化: 老的是否属于其他域名标识=%d, 新的是否属于其他域名标识=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->other_realm, pNewGBLogicDeviceInfo->other_realm);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, Front-end reporting point whether belong to other domain name change: Old =%s,New =%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->other_realm, pNewGBLogicDeviceInfo->other_realm);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE other_realm: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        if (pOldGBLogicDeviceInfo->ctrl_enable != pNewGBLogicDeviceInfo->ctrl_enable)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 下级CMS上报的点位控球标识发生变化: 老的控球标识=%d, 新的控球标识=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->ctrl_enable, pNewGBLogicDeviceInfo->ctrl_enable);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s,CMS reporting point ball logo at a lower level change: Old =%s,New =%s",  pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->ctrl_enable, pNewGBLogicDeviceInfo->ctrl_enable);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE ctrl_enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        if (pOldGBLogicDeviceInfo->mic_enable != pNewGBLogicDeviceInfo->mic_enable)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 下级CMS上报的点位是否支持对讲标识发生变化: 老的是否支持对讲标识=%d, 新的是否支持对讲标识=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING,  "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower	CMS whether to support the intercom CMS mark change: old =%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE mic_enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        if (pOldGBLogicDeviceInfo->frame_count != pNewGBLogicDeviceInfo->frame_count)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 下级CMS上报的点位帧率发生变化: 老的帧率=%d, 新的帧率=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->frame_count, pNewGBLogicDeviceInfo->frame_count);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING,  "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower	CMS whether to support the intercom CMS mark change: old =%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE frame_count: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        if (pOldGBLogicDeviceInfo->alarm_duration != pNewGBLogicDeviceInfo->alarm_duration)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 下级CMS上报的点位告警时长发生变化: 老的告警时长=%d, 新的告警时长=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_duration, pNewGBLogicDeviceInfo->alarm_duration);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The lower CMS reporting point alarm duration change: old alarm duration=%d, new alarm duration=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_duration, pNewGBLogicDeviceInfo->alarm_duration);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE alarm_duration: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 是否支持多码流 */
        if (pOldGBLogicDeviceInfo->stream_count != pNewGBLogicDeviceInfo->stream_count)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 下级CMS上报的点位是否支持多码流发生变化: 老的是否支持多码流=%d, 新的是否支持多码流=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS whether to support support more stream change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE stream_count: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 对应的媒体物理设备索引 */
        if (pOldGBLogicDeviceInfo->phy_mediaDeviceIndex != pNewGBLogicDeviceInfo->phy_mediaDeviceIndex)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE phy_mediaDeviceIndex: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 设备生产商 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->manufacturer, pOldGBLogicDeviceInfo->manufacturer))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE manufacturer 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 设备型号 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->model, pOldGBLogicDeviceInfo->model))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE model 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 设备归属 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->owner, pOldGBLogicDeviceInfo->owner))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE owner 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

#if 0

        /* 行政区域 */
        if (NULL != pNewGBLogicDeviceInfo->civil_code && NULL != pOldGBLogicDeviceInfo->civil_code
            && 0 != sstrcmp(pNewGBLogicDeviceInfo->civil_code, pOldGBLogicDeviceInfo->civil_code))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE 18 \r\n");
            return 1;
        }
        else if (NULL == pNewGBLogicDeviceInfo->civil_code && NULL != pOldGBLogicDeviceInfo->civil_code)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE 19 \r\n");
            return 1;
        }
        else if (NULL != pNewGBLogicDeviceInfo->civil_code && NULL == pOldGBLogicDeviceInfo->civil_code)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE 20 \r\n");
            return 1;
        }

#endif

        /* 警区 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->block, pOldGBLogicDeviceInfo->block))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE block 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 安装地址 */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->address, pOldGBLogicDeviceInfo->address))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE address 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 是否有子设备 */
        if (pOldGBLogicDeviceInfo->parental != pNewGBLogicDeviceInfo->parental)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE parental: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 父设备/区域/系统ID */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->parentID, pOldGBLogicDeviceInfo->parentID))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE parentID 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 信令安全模式*/
        if (pOldGBLogicDeviceInfo->safety_way != pNewGBLogicDeviceInfo->safety_way)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE safety_way: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 注册方式 */
        if (pOldGBLogicDeviceInfo->register_way != pNewGBLogicDeviceInfo->register_way)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE register_way: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 证书序列号*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->cert_num, pOldGBLogicDeviceInfo->cert_num))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE cert_num 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 证书有效标识 */
        if (pOldGBLogicDeviceInfo->certifiable != pNewGBLogicDeviceInfo->certifiable)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE certifiable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 无效原因码 */
        if (pOldGBLogicDeviceInfo->error_code != pNewGBLogicDeviceInfo->error_code)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE error_code: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 证书终止有效期*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->end_time, pOldGBLogicDeviceInfo->end_time))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE end_time 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 保密属性 */
        if (pOldGBLogicDeviceInfo->secrecy != pNewGBLogicDeviceInfo->secrecy)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE secrecy: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* IP地址*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->ip_address, pOldGBLogicDeviceInfo->ip_address))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE ip_address 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 端口号*/
        if (pOldGBLogicDeviceInfo->port != pNewGBLogicDeviceInfo->port)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE port: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 密码*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->password, pOldGBLogicDeviceInfo->password))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE password 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 经度 */
        if (pNewGBLogicDeviceInfo->longitude > 0 && pOldGBLogicDeviceInfo->longitude != pNewGBLogicDeviceInfo->longitude)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE longitude: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 纬度 */
        if (pNewGBLogicDeviceInfo->latitude > 0 && pOldGBLogicDeviceInfo->latitude != pNewGBLogicDeviceInfo->latitude)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE latitude: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 所属图层 */
        if (pNewGBLogicDeviceInfo->map_layer[0] != '\0' && 0 != sstrcmp(pNewGBLogicDeviceInfo->map_layer, pOldGBLogicDeviceInfo->map_layer))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE map_layer 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 点位类型 */
        if (pOldGBLogicDeviceInfo->device_type != pNewGBLogicDeviceInfo->device_type)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE device_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 所属的CMSID */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->cms_id, pOldGBLogicDeviceInfo->cms_id))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE cms_id 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* RCU Value */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->Value, pOldGBLogicDeviceInfo->Value))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE Value 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* RCU Unit */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->Unit, pOldGBLogicDeviceInfo->Unit))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE Unit 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* RCU AlarmPriority */
        if (pOldGBLogicDeviceInfo->AlarmPriority != pNewGBLogicDeviceInfo->AlarmPriority)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, RCU上报的AlarmPriority发生变化: 老的AlarmPriority=%d, 新的AlarmPriority=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->AlarmPriority, pNewGBLogicDeviceInfo->AlarmPriority);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The AlarmPriority reported by RCU change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->AlarmPriority, pNewGBLogicDeviceInfo->AlarmPriority);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE AlarmPriority: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* RCU Guard */
        if (pOldGBLogicDeviceInfo->guard_type != pNewGBLogicDeviceInfo->guard_type)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, RCU上报的Guard发生变化: 老的Guard=%d, 新的Guard=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->guard_type, pNewGBLogicDeviceInfo->guard_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The Guard reported by RCU change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->guard_type, pNewGBLogicDeviceInfo->guard_type);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE guard_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 点位状态 */
        if (pOldGBLogicDeviceInfo->status != pNewGBLogicDeviceInfo->status)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 前端上报点位状态发生变化: 老的状态=%d, 新的状态=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, Front-end reporting point status change: Old =%s,New =%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE status: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 2;
        }
    }

    if (1 == change_type) /* 以数据库为准，数据库到内存的比较 */
    {
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 数据库点位名称发生变化: 老的点位名称=%s, 新的点位名称=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point's name change: Old Poit Name=%s,New Poit Name=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE device_name 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 对应的媒体物理设备通道 */
        if (pOldGBLogicDeviceInfo->phy_mediaDeviceChannel != pNewGBLogicDeviceInfo->phy_mediaDeviceChannel)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE phy_mediaDeviceChannel: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 对应的控制物理设备索引 */
        if (pOldGBLogicDeviceInfo->phy_ctrlDeviceIndex != pNewGBLogicDeviceInfo->phy_ctrlDeviceIndex)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE phy_ctrlDeviceIndex: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 对应的控制物理设备通道 */
        if (pOldGBLogicDeviceInfo->phy_ctrlDeviceChannel != pNewGBLogicDeviceInfo->phy_ctrlDeviceChannel)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE phy_ctrlDeviceChannel: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* 录像类型*/
        if (pOldGBLogicDeviceInfo->record_type != pNewGBLogicDeviceInfo->record_type)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "前端逻辑设备信息发生变化, 逻辑设备ID=%s, 逻辑点位名称=%s, 数据库点位录像类型发生变化: 老的录像类型=%d, 新的录像类型=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->record_type, pNewGBLogicDeviceInfo->record_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point record type change: Old Poit Name=%s,New Poit Name=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->record_type, pNewGBLogicDeviceInfo->record_type);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE record_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : IsGBDeviceRegInfoHasChange
 功能描述  : 标准物理设备注册信息是否有变化
 输入参数  : int pos
                            int iChannel
                            char* pcManufacturer
                            char* pcModel
                            char* pcVersion
 输出参数  : 无
 返 回 值  :int
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int IsGBDeviceRegInfoHasChange(GBDevice_info_t * pGBDeviceInfo, int iDeviceType, char * pcLoginIP, int iLoginPort, int iRegInfoIndex)
{
    if (pGBDeviceInfo == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "IsGBDeviceRegInfoHasChange() exit---: Param Error \r\n");
        return 0;
    }

    /* 设备类型 */
    if (pGBDeviceInfo->device_type != iDeviceType)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "IsGBDeviceRegInfoHasChange() exit---: CHANGE 12 \r\n");
        return 1;
    }

    /* 设备登录端口 */
    if (pGBDeviceInfo->login_port > 0 && iLoginPort > 0)
    {
        if (pGBDeviceInfo->login_port != iLoginPort)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "IsGBDeviceRegInfoHasChange() exit---: CHANGE 1 \r\n");
            return 1;
        }
    }
    else if (pGBDeviceInfo->login_port <= 0 && iLoginPort > 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "IsGBDeviceRegInfoHasChange() exit---: CHANGE 2 \r\n");
        return 1;
    }

    /* 设备登录IP 地址 */
    if (NULL != pGBDeviceInfo->login_ip && NULL != pcLoginIP)
    {
        if (0 != sstrcmp(pGBDeviceInfo->login_ip, pcLoginIP))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "IsGBDeviceRegInfoHasChange() exit---: CHANGE 3 \r\n");
            return 1;
        }
    }
    else if (NULL == pGBDeviceInfo->login_ip && NULL != pcLoginIP)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "IsGBDeviceRegInfoHasChange() exit---: CHANGE 4 \r\n");
        return 1;
    }

    /* 设备注册索引 */
    if (pGBDeviceInfo->reg_info_index >= 0 && iRegInfoIndex >= 0)
    {
        if (pGBDeviceInfo->reg_info_index != iRegInfoIndex)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "IsGBDeviceRegInfoHasChange() exit---: CHANGE 5 \r\n");
            return 1;
        }
    }
    else if (pGBDeviceInfo->reg_info_index < 0 && iRegInfoIndex >= 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "IsGBDeviceRegInfoHasChange() exit---: CHANGE 6 \r\n");
        return 1;
    }

    //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "IsGBDeviceRegInfoHasChange() exit---: NO CHANGE \r\n");
    return 0;
}

/*****************************************************************************
 函 数 名  : SendExecuteDevicePresetMessage
 功能描述  : 发送执行预置位命令给前端设备
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             GBDevice_info_t* pGBDeviceInfo
             unsigned int uPresetID
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月10日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendExecuteDevicePresetMessage(GBLogicDevice_info_t* pGBLogicDeviceInfo, GBDevice_info_t * pGBDeviceInfo, unsigned int uPresetID)
{
    int i = 0;

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    BYTE b1 = 0xA5, b2 = 0x0F, b3 = 0x4D, b4 = 0x82, b5 = 0, b6 = uPresetID, b7 = 0, b8 = (b1 + b2 + b3 + b4 + b5 + b6 + b7) % 256;
    char sPTZCmd[128] = {0};

    if (NULL == pGBLogicDeviceInfo || NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendExecuteDevicePresetMessage() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pGBLogicDeviceInfo->device_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendExecuteDevicePresetMessage() exit---: device_id Error \r\n");
        return -1;
    }

    if (LOCK_STATUS_USER_LOCK == pGBLogicDeviceInfo->lock_status)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "发送执行预置位Message消息到前端设备失败, 点位已被用户锁定:逻辑点位ID=%s, 点位名称=%s, 前端IP地址=%s, 端口号=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to send message that notifies preset, point has been locked by the user :Logic ID=%s, name=%s, IP=%s, Port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        return -2;
    }
    else if (LOCK_STATUS_ROUTE_LOCK == pGBLogicDeviceInfo->lock_status)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "发送执行预置位Message消息到前端设备失败, 点位已被上级锁定:逻辑点位ID=%s, 点位名称=%s, 前端IP地址=%s, 端口号=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to send message that notifies preset, point has been locked by the superior :Logic ID=%s, name=%s, IP=%s, Port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        return -2;
    }

    /* 组建XML信息 */
    outPacket.SetRootTag("Control");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"34574");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

    AccNode = outPacket.CreateElement((char*)"PTZCmd");
    snprintf(sPTZCmd, 128, "%02X%02X%02X%02X%02X%02X%02X%02X", b1, b2, b3, b4, b5, b6, b7, b8);
    outPacket.SetElementValue(AccNode, sPTZCmd);

    /* 推送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBLogicDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送执行预置位命令Message消息到前端设备失败:逻辑点位ID=%s, 点位名称=%s, 前端设备ID=%s, IP地址=%s, 端口号=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to send message that notifies preset:Logic ID=%s, Name=%s, ID=%s, IP=%s, port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendExecuteDevicePresetMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送执行预置位命令Message消息到前端设备成功:逻辑点位ID=%s, 点位名称=%s, 前端设备ID=%s, IP地址=%s, 端口号=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Succeed to send message that notifies preset:Logic ID=%s, Name=%s, ID=%s, IP=%s, port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendExecuteDevicePresetMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendExecuteDevicePresetMessageToRoute
 功能描述  : 发送执行预置位命令给上级CMS
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             route_info_t * pRouteInfo
             unsigned int uPresetID
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月26日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendExecuteDevicePresetMessageToRoute(GBLogicDevice_info_t* pGBLogicDeviceInfo, route_info_t * pRouteInfo, unsigned int uPresetID)
{
    int i = 0;

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    BYTE b1 = 0xA5, b2 = 0x0F, b3 = 0x4D, b4 = 0x82, b5 = 0, b6 = uPresetID, b7 = 0, b8 = (b1 + b2 + b3 + b4 + b5 + b6 + b7) % 256;
    char sPTZCmd[128] = {0};

    if (NULL == pGBLogicDeviceInfo || NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendExecuteDevicePresetMessageToRoute() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pGBLogicDeviceInfo->device_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendExecuteDevicePresetMessageToRoute() exit---: device_id Error \r\n");
        return -1;
    }

    if (LOCK_STATUS_USER_LOCK == pGBLogicDeviceInfo->lock_status)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "发送执行预置位Message消息到上级CMS失败, 点位已被用户锁定:逻辑点位ID=%s, 点位名称=%s, 上级CMS IP地址=%s, 端口号=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to send message that notifies preset, point has been locked by the user :Logic ID=%s, name=%s, IP=%s, Port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_ip, pRouteInfo->server_port);
        return -2;
    }
    else if (LOCK_STATUS_ROUTE_LOCK == pGBLogicDeviceInfo->lock_status)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "发送执行预置位Message消息到上级CMS失败, 点位已被上级锁定:逻辑点位ID=%s, 点位名称=%s, 上级CMS IP地址=%s, 端口号=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to send message that notifies preset, point has been locked by the superior :Logic ID=%s, name=%s, IP=%s, Port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_ip, pRouteInfo->server_port);
        return -2;
    }

    /* 组建XML信息 */
    outPacket.SetRootTag("Control");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"34575");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

    AccNode = outPacket.CreateElement((char*)"PTZCmd");
    snprintf(sPTZCmd, 128, "%02X%02X%02X%02X%02X%02X%02X%02X", b1, b2, b3, b4, b5, b6, b7, b8);
    outPacket.SetElementValue(AccNode, sPTZCmd);

    /* 推送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBLogicDeviceInfo->device_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送执行预置位Message消息到上级CMS失败:逻辑点位ID=%s, 点位名称=%s, 上级CMS ID=%s, IP地址=%s, 端口号=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send message that notifies preset:Logic ID=%s, Name=%s, ID=%s, IP=%s, port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendExecuteDevicePresetMessageToRoute() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送执行预置位Message消息到上级CMS成功:逻辑点位ID=%s, 点位名称=%s, 上级CMS ID=%s, IP地址=%s, 端口号=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send message that notifies preset:Logic ID=%s, Name=%s, ID=%s, IP=%s, port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendExecuteDevicePresetMessageToRoute() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendDeviceControlAlarmMessage
 功能描述  : 发送报警输出控制命令
 输入参数  : char * device_id
             GBDevice_info_t * pGBDeviceInfo
             int iuCmdType
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月5日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendDeviceControlAlarmMessage(char * device_id, GBDevice_info_t * pGBDeviceInfo, int iuCmdType)
{
    int i = 0;

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (NULL == device_id || NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendDeviceControlAlarmMessage() exit---: Param Error \r\n");
        return -1;
    }

    /* 组建XML信息 */
    outPacket.SetRootTag("Control");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"44313");

    AccNode = outPacket.CreateElement((char*)"DeviceID");

    if (NULL != device_id)
    {
        outPacket.SetElementValue(AccNode, device_id);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    AccNode = outPacket.CreateElement((char*)"AlarmCmd");

    if (0 == iuCmdType)
    {
        outPacket.SetElementValue(AccNode, (char*)"Close");
    }
    else if (1 == iuCmdType)
    {
        outPacket.SetElementValue(AccNode, (char*)"Open");
    }
    else if (2 == iuCmdType)
    {
        outPacket.SetElementValue(AccNode, (char*)"ResetAlarm");
    }

    /* 推送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送报警输出控制命令Message消息到前端设备失败:前端设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send alarm output control command Message to the front-end equipment:前端设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceControlAlarmMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送报警输出控制命令Message消息到前端设备成功:前端设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send alarm output control command Message to the front-end equipment:前端设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendDeviceControlAlarmMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendQueryDeviceInfoMessage
 功能描述  : 发送设备信息查询消息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月19日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SendQueryDeviceInfoMessage(GBDevice_info_t * pGBDeviceInfo)
{
    int i = 0;

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendQueryDeviceInfoMessage() exit---: Param Error \r\n");
        return -1;
    }

    /* 组建XML信息 */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"DeviceInfo");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"1");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    /* 推送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送查询设备信息Message消息到设备失败:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send alarm output control command Message to the front-end equipment:前端设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQueryDeviceInfoMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送查询设备信息Message消息到设备成功:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Fail to send alarm output control command Message to the front-end equipment:前端设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQueryDeviceInfoMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendQueryDeviceCatalogMessage
 功能描述  : 发送设备目录查询消息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月19日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SendQueryDeviceCatalogMessage(GBDevice_info_t * pGBDeviceInfo)
{
    int i = 0;
    char strSN[32] = {0};
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    time_t now = time(NULL);

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendQueryDeviceCatalogMessage() exit---: Param Error \r\n");
        return -1;
    }

    pGBDeviceInfo->CataLogNumCount = 0;
    pGBDeviceInfo->CataLogSN++;
    pGBDeviceInfo->iGetCataLogStatus = 0;
    pGBDeviceInfo->iLastGetCataLogTime = now;

    /* 组建XML信息 */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"Catalog");

    AccNode = outPacket.CreateElement((char*)"SN");
    snprintf(strSN, 32, "%u", pGBDeviceInfo->CataLogSN);
    outPacket.SetElementValue(AccNode, strSN);

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    /* 推送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送查询设备目录Message消息到设备失败:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send query inventory message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQueryDeviceInfoMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送查询设备目录Message消息到设备成功:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send query inventory message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQueryDeviceInfoMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendQueryDeviceStatusMessage
 功能描述  : 发送设备状态查询消息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月19日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SendQueryDeviceStatusMessage(GBDevice_info_t * pGBDeviceInfo)
{
    int i = 0;

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendQueryDeviceStatusMessage() exit---: Param Error \r\n");
        return -1;
    }

    /* 组建XML信息 */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"DeviceStatus");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"1");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    /* 推送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送查询设备状态Message消息到设备失败:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send query status message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQueryDeviceStatusMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送查询设备状态Message消息到设备成功:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send query status message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQueryDeviceStatusMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendQueryAllOfflineLogicDeviceStatusMessage
 功能描述  : 查询所有掉线的逻辑设备点位状态信息
 输入参数  : GBDevice_info_t * pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年7月19日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendQueryAllOfflineLogicDeviceStatusMessage(GBDevice_info_t * pGBDeviceInfo)
{
    int iRet = 0;
    int index = -1;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    vector<string> DeviceIDVector;

    if (NULL == pGBDeviceInfo)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendQueryAllOfflineLogicDeviceStatusMessage() exit---: Param Error \r\n");
        return -1;
    }

    DeviceIDVector.clear();

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->phy_mediaDeviceIndex <= 0))
        {
            continue;
        }

        if (pGBLogicDeviceInfo->status > 0)
        {
            continue;
        }

        if (pGBLogicDeviceInfo->phy_mediaDeviceIndex == pGBDeviceInfo->id)
        {
            DeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送获取前端设备掉线点位状态信息: 设备ID=%s, IP地址=%s, 端口号=%d, 掉线的逻辑点位数=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (int)DeviceIDVector.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send to get the front-end equipment dropped points state information: ID=%s, IP=%s, port=%d, Online Logic point number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (int)DeviceIDVector.size());

    if (DeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)DeviceIDVector.size(); index++)
        {
            iRet |= SendQueryGBLogicDeviceStatusMessage((char*)DeviceIDVector[index].c_str(), pGBDeviceInfo);
        }
    }

    DeviceIDVector.clear();

    return iRet;
}

/*****************************************************************************
 函 数 名  : SendQueryGBLogicDeviceStatusMessage
 功能描述  : 发送查询逻辑设备状态信息
 输入参数  : char* device_id
             GBDevice_info_t * pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月19日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendQueryGBLogicDeviceStatusMessage(char* device_id, GBDevice_info_t * pGBDeviceInfo)
{
    int i = 0;

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendQueryGBLogicDeviceStatusMessage() exit---: Param Error \r\n");
        return -1;
    }

    /* 组建XML信息 */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"DeviceStatus");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"3256");

    AccNode = outPacket.CreateElement((char*)"DeviceID");

    if (NULL != device_id)
    {
        outPacket.SetElementValue(AccNode, device_id);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    /* 推送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送查询逻辑设备状态Message消息到设备失败:逻辑设备ID=%s, 设备ID=%s, IP地址=%s, 端口号=%d", device_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send query status message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQueryGBLogicDeviceStatusMessage() SIP_SendMessage Error:device_id=%s, dest_id=%s, dest_ip=%s, dest_port=%d \r\n", device_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送查询逻辑设备状态Message消息到设备成功:逻辑设备ID=%s, 设备ID=%s, IP地址=%s, 端口号=%d", device_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send query status message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQueryGBLogicDeviceStatusMessage() SIP_SendMessage OK:device_id=%s, dest_id=%s, dest_ip=%s, dest_port=%d \r\n", device_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendQueryDeviceGroupInfoMessage
 功能描述  : 发送获取逻辑设备分组配置消息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月12日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendQueryDeviceGroupInfoMessage(GBDevice_info_t * pGBDeviceInfo)
{
    int i = 0;
    char strSN[32] = {0};
    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendQueryDeviceGroupInfoMessage() exit---: Param Error \r\n");
        return -1;
    }

    pGBDeviceInfo->LogicDeviceGroupConfigCount = 0;
    pGBDeviceInfo->LogicDeviceGroupSN++;

    /* 组建XML信息 */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"LogicDeviceGroupConfig");

    AccNode = outPacket.CreateElement((char*)"SN");
    snprintf(strSN, 32, "%u", pGBDeviceInfo->LogicDeviceGroupSN);
    outPacket.SetElementValue(AccNode, strSN);

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    /* 推送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送查询设备分组Message消息到设备失败:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send query group message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQueryDeviceGroupInfoMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送查询设备分组Message消息到设备成功:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send query group message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQueryDeviceGroupInfoMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendQueryDeviceGroupMapInfoMessage
 功能描述  : 发送获取逻辑设备分组关系配置消息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月12日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendQueryDeviceGroupMapInfoMessage(GBDevice_info_t * pGBDeviceInfo)
{
    int i = 0;
    char strSN[32] = {0};
    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendQueryDeviceGroupMapInfoMessage() exit---: Param Error \r\n");
        return -1;
    }

    pGBDeviceInfo->LogicDeviceMapGroupConfigCount = 0;
    pGBDeviceInfo->LogicDeviceMapGroupSN++;

    /* 组建XML信息 */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"LogicDeviceMapGroupConfig");

    AccNode = outPacket.CreateElement((char*)"SN");
    snprintf(strSN, 32, "%u", pGBDeviceInfo->LogicDeviceMapGroupSN);
    outPacket.SetElementValue(AccNode, strSN);

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    /* 推送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送查询设备分组关系Message消息到设备失败:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send query group message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQueryDeviceGroupMapInfoMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送查询设备分组关系Message消息到设备成功:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send query group message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQueryDeviceGroupMapInfoMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendDeviceTeleBootMessage
 功能描述  : 发送设备远程重启命令
 输入参数  : GBDevice_info_t * pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月4日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendDeviceTeleBootMessage(GBDevice_info_t * pGBDeviceInfo)
{
    int i = 0;

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendDeviceTeleBootMessage() exit---: Param Error \r\n");
        return -1;
    }

    /* 组建XML信息 */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"17298");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    AccNode = outPacket.CreateElement((char*)"TeleBoot");
    outPacket.SetElementValue(AccNode, (char*)"Boot");

    /* 推送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送重启设备Message消息到设备失败:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send restart message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceTeleBootMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送重启设备Message消息到设备成功:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send restart message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendDeviceTeleBootMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendQueryDECDeviceMediaPortMessage
 功能描述  : 发送获取解码器媒体端口消息
 输入参数  : GBDevice_info_t * pGBDeviceInfo
             char* dc_id
             char* camera_id
             int iStreamType
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendQueryDECDeviceMediaPortMessage(GBDevice_info_t * pGBDeviceInfo, char* dc_id, char* camera_id, int iStreamType)
{
    int i = 0;
    char strStreamType[32] = {0};
    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (NULL == pGBDeviceInfo || NULL == dc_id || NULL == camera_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendQueryDECDeviceMediaPortMessage() exit---: Param Error \r\n");
        return -1;
    }

    /* 组建XML信息 */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"GetDECMediaPort");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"547");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    AccNode = outPacket.CreateElement((char*)"DECChannelID");
    outPacket.SetElementValue(AccNode, dc_id);

    AccNode = outPacket.CreateElement((char*)"CameraID");
    outPacket.SetElementValue(AccNode, camera_id);

    AccNode = outPacket.CreateElement((char*)"StreamType");
    snprintf(strStreamType, 32, "%d", iStreamType);
    outPacket.SetElementValue(AccNode, strStreamType);

    /* 推送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送查询解码器媒体端口Message消息到设备失败:设备ID=%s, IP地址=%s, 端口号=%d, 解码通道ID=%s, 逻辑点位Id=%s, 码流类型=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, dc_id, camera_id, iStreamType);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send query media port message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQueryDECDeviceMediaPortMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送查询解码器媒体端口Message消息到设备成功:设备ID=%s, IP地址=%s, 端口号=%d, 解码通道ID=%s, 逻辑点位Id=%s, 码流类型=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, dc_id, camera_id, iStreamType);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send query media port message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQueryDECDeviceMediaPortMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : UpdateGBDeviceRegStatus2DB
 功能描述  : 更新标准物理设备数据库注册状态
 输入参数  : int pos
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
int UpdateGBDeviceRegStatus2DB(GBDevice_info_t * pGBDeviceInfo, DBOper * pDevice_Srv_dboper)
{
    int iRet = 0;
    string strSQL = "";
    char strStatus[16] = {0};

    if (pGBDeviceInfo == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "UpdateGBDeviceRegStatus2DB() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "UpdateGBDeviceRegStatus2DB() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    /* 更新数据库 */
    strSQL.clear();
    strSQL = "UPDATE GBPhyDeviceConfig SET Status = ";
    snprintf(strStatus, 16, "%d", pGBDeviceInfo->reg_status);
    strSQL += strStatus;
    strSQL += ", DeviceIP = '";
    strSQL += pGBDeviceInfo->login_ip;
    strSQL += "' WHERE DeviceID like '";
    strSQL += pGBDeviceInfo->device_id;
    strSQL += "'";

    iRet = pDevice_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateGBDeviceRegStatus2DB() DB Oper Error:strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateGBDeviceRegStatus2DB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : UpdateGBDeviceLinkType2DB
 功能描述  : 更新设备的联网类型到数据库
 输入参数  : GBDevice_info_t* pGBDeviceInfo
                            DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月23日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int UpdateGBDeviceLinkType2DB(GBDevice_info_t * pGBDeviceInfo, DBOper * pDevice_Srv_dboper)
{
    int iRet = 0;
    string strSQL = "";
    char strLinkType[16] = {0};

    if (pGBDeviceInfo == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "UpdateGBDeviceLinkType2DB() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "UpdateGBDeviceLinkType2DB() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    /* 更新数据库 */
    strSQL.clear();
    strSQL = "UPDATE GBPhyDeviceConfig SET LinkType = ";
    snprintf(strLinkType, 16, "%d", pGBDeviceInfo->link_type);
    strSQL += strLinkType;
    strSQL += " WHERE DeviceID like '";
    strSQL += pGBDeviceInfo->device_id;
    strSQL += "'";

    iRet = pDevice_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateGBDeviceLinkType2DB() DB Oper Error:strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateGBDeviceLinkType2DB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : UpdateGBDeviceCMSID2DB
 功能描述  : 更新物理设备的CMSID到数据库
 输入参数  : char* device_id
             DBOper * pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月4日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int UpdateGBDeviceCMSID2DB(char* device_id, DBOper * pDevice_Srv_dboper)
{
    int iRet = 0;
    string strSQL = "";

    if (device_id == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "UpdateGBDeviceCMSID2DB() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "UpdateGBDeviceCMSID2DB() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    /* 更新数据库 */
    strSQL.clear();
    strSQL = "UPDATE GBPhyDeviceConfig SET CMSID = '";
    strSQL += local_cms_id_get();
    strSQL += "' WHERE DeviceID like '";
    strSQL += device_id;
    strSQL += "'";

    iRet = pDevice_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateGBDeviceCMSID2DB() DB Oper Error:strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateGBDeviceCMSID2DB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : UpdateGBDeviceInfo2DB
 功能描述  : 更新物理设备信息到数据库
 输入参数  : GBDevice_cfg_t& new_GBDevice_cfg
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年11月30日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
int UpdateGBDeviceInfo2DB(GBDevice_cfg_t & new_GBDevice_cfg, DBOper * pDevice_Srv_dboper)
{
    int iRet = 0;
    string strSQL = "";
    int record_count = 0;
    char strMaxCamera[16] = {0};
    char strMaxAlarm[16] = {0};
    GBDevice_cfg_t old_GBDevice_cfg;

    if (NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "UpdateGBDeviceInfo2DB() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from GBPhyDeviceConfig WHERE DeviceID like '";
    strSQL += new_GBDevice_cfg.device_id + "';";
    record_count = pDevice_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateGBDeviceInfo2DB() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateGBDeviceInfo2DB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "UpdateGBDeviceInfo2DB() exit---: No Record Count \r\n");
        return 0;
    }

    /* 获取数据库原有老数据 */
    int tmp_ivalue = 0;
    string tmp_svalue = "";

    /* 设备视频输入通道数 */
    tmp_ivalue = 0;
    pDevice_Srv_dboper->GetFieldValue("MaxCamera", tmp_ivalue);

    old_GBDevice_cfg.device_max_camera = tmp_ivalue;


    /* 设备报警输入通道数 */
    tmp_ivalue = 0;
    pDevice_Srv_dboper->GetFieldValue("MaxAlarm", tmp_ivalue);

    old_GBDevice_cfg.device_max_alarm = tmp_ivalue;

    /* 设备生产商 */
    tmp_svalue.clear();
    pDevice_Srv_dboper->GetFieldValue("Manufacturer", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        old_GBDevice_cfg.device_manufacturer = tmp_svalue;
    }

    /* 设备型号 */
    tmp_svalue.clear();
    pDevice_Srv_dboper->GetFieldValue("Model", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        old_GBDevice_cfg.device_model = tmp_svalue;
    }

    /* 设备版本 */
    tmp_svalue.clear();
    pDevice_Srv_dboper->GetFieldValue("Firmware", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        old_GBDevice_cfg.device_firmware = tmp_svalue;
    }

    /* 判断是否有变化 */
    if (old_GBDevice_cfg.device_max_camera == new_GBDevice_cfg.device_max_camera
        && old_GBDevice_cfg.device_max_alarm == new_GBDevice_cfg.device_max_alarm
        && old_GBDevice_cfg.device_manufacturer == new_GBDevice_cfg.device_manufacturer
        && old_GBDevice_cfg.device_model == new_GBDevice_cfg.device_model
        && old_GBDevice_cfg.device_firmware == new_GBDevice_cfg.device_firmware)
    {
        return 0;
    }

    /* 更新数据库 */
    strSQL.clear();
    strSQL = "UPDATE GBPhyDeviceConfig SET MaxCamera = ";
    snprintf(strMaxCamera, 16, "%d", new_GBDevice_cfg.device_max_camera);
    strSQL += strMaxCamera;
    strSQL += ",MaxAlarm = ";
    snprintf(strMaxAlarm, 16, "%d", new_GBDevice_cfg.device_max_alarm);
    strSQL += strMaxAlarm;
    strSQL += ",Model = '";
    strSQL += new_GBDevice_cfg.device_model;
    strSQL += "',Firmware = '";
    strSQL += new_GBDevice_cfg.device_firmware;
    strSQL += "',Manufacturer = '";
    strSQL += new_GBDevice_cfg.device_manufacturer;
    strSQL += "' WHERE DeviceID like '";
    strSQL += new_GBDevice_cfg.device_id;
    strSQL += "'";

    iRet = pDevice_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateGBDeviceInfo2DB() DB Oper Error: strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateGBDeviceInfo2DB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : UpdateGBLogicDeviceRegStatus2DB
 功能描述  : 更新标准逻辑设备数据库注册状态
 输入参数  : char* strDeviceID
                            int status
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
int UpdateGBLogicDeviceRegStatus2DB(char * strDeviceID, int status, DBOper * pDevice_Srv_dboper)
{
    int iRet = 0;
    string strSQL = "";
    char strStatus[16] = {0};

    if (strDeviceID == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "UpdateGBLogicDeviceRegStatus2DB() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "UpdateGBLogicDeviceRegStatus2DB() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    /* 更新数据库 */
    strSQL.clear();
    strSQL = "UPDATE GBLogicDeviceConfig SET Status = ";
    snprintf(strStatus, 16, "%d", status);
    strSQL += strStatus;
    strSQL += " WHERE DeviceID like '";
    strSQL += strDeviceID;
    strSQL += "'";

    iRet = pDevice_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateGBLogicDeviceRegStatus2DB() DB Oper Error:strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateGBLogicDeviceRegStatus2DB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : UpdateGBLogicDeviceXYParam2DB
 功能描述  : 更新逻辑设备的经纬度信息到数据库中
 输入参数  : char* strDeviceID
             double longitude
             double latitude
             char * strMapLayer
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月28日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int UpdateGBLogicDeviceXYParam2DB(char * strDeviceID, double longitude, double latitude, char * strMapLayer, DBOper * pDevice_Srv_dboper)
{
    int iRet = 0;
    string strSQL = "";
    char strTmp[64] = {0};

    if (strDeviceID == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "UpdateGBLogicDeviceXYParam2DB() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "UpdateGBLogicDeviceXYParam2DB() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    /* 更新数据库 */
    strSQL.clear();
    strSQL = "UPDATE GBLogicDeviceConfig SET ";

    strSQL += "Longitude = ";
    memset(strTmp, 0 , 64);
    snprintf(strTmp, 64, "%.16lf", longitude);
    strSQL += strTmp;

    strSQL += ",";

    /* 纬度 */
    strSQL += "Latitude = ";
    memset(strTmp, 0 , 64);
    snprintf(strTmp, 64, "%.16lf", latitude);
    strSQL += strTmp;

    /* 所属图层 */
    if (NULL != strMapLayer)
    {
        strSQL += ",";

        strSQL += "Resved2 = '";
        strSQL += strMapLayer;
        strSQL += "'";
    }
    else
    {
        strSQL += ",";

        strSQL += "Resved2 = ''";
    }

    strSQL += " WHERE DeviceID like '";
    strSQL += strDeviceID;
    strSQL += "'";

    iRet = pDevice_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "UpdateGBLogicDeviceXYParam2DB() strSQL=%s DB_Update iRet=%d \r\n", strSQL.c_str(), iRet);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateGBLogicDeviceXYParam2DB() DB Oper Error:strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "UpdateGBLogicDeviceXYParam2DB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : AddGBLogicDeviceInfoByGBDeviceInfo
 功能描述  : 根据标准物理设备信息添加逻辑设备信息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月20日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int AddGBLogicDeviceInfoByGBDeviceInfo(GBDevice_info_t * pGBDeviceInfo)
{
    int i = 0;
    int iLogicDeviceIndex = -1;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    char strDeviceType[4] = {0};
    char* pTmp = NULL;

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "AddGBLogicDeviceInfoByGBDeviceInfo() exit---: Param Error \r\n");
        return -1;
    }

    pGBLogicDeviceInfo = GBLogicDevice_info_find(pGBDeviceInfo->device_id);

    if (NULL != pGBLogicDeviceInfo) /* 已经存在，更新 */
    {
        pGBLogicDeviceInfo->status = 1;
        pGBLogicDeviceInfo->enable = 1;

        if (pGBLogicDeviceInfo->id <= 0)
        {
            pGBLogicDeviceInfo->id = crc32((unsigned char*)pGBLogicDeviceInfo->device_id, MAX_ID_LEN);
        }

        if ('\0' == pGBLogicDeviceInfo->cms_id[0])
        {
            osip_strncpy(pGBLogicDeviceInfo->cms_id, local_cms_id_get(), MAX_ID_LEN);
        }
    }
    else /* 不存在，添加 */
    {
        i = GBLogicDevice_info_init(&pGBLogicDeviceInfo);

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfoByGBDeviceInfo() exit---: GBLogic Device Info Init Error \r\n");
            return -1;
        }

        if ('\0' != pGBDeviceInfo->device_id[0])
        {
            osip_strncpy(pGBLogicDeviceInfo->device_id, pGBDeviceInfo->device_id, MAX_ID_LEN);

            pTmp = &pGBLogicDeviceInfo->device_id[10];
            osip_strncpy(strDeviceType, pTmp, 3);
            pGBLogicDeviceInfo->device_type = osip_atoi(strDeviceType);
        }
        else
        {
            pGBLogicDeviceInfo->device_type = EV9000_DEVICETYPE_IPC;
        }

        if (NULL != pGBLogicDeviceInfo->device_id)
        {
            pGBLogicDeviceInfo->id = crc32((unsigned char*)pGBLogicDeviceInfo->device_id, MAX_ID_LEN);
        }

        osip_strncpy(pGBLogicDeviceInfo->cms_id, local_cms_id_get(), MAX_ID_LEN);

        pGBLogicDeviceInfo->enable = 1;

        pGBLogicDeviceInfo->phy_mediaDeviceIndex = pGBDeviceInfo->id;

        pGBLogicDeviceInfo->phy_mediaDeviceChannel = 1;

        pGBLogicDeviceInfo->phy_ctrlDeviceIndex = pGBDeviceInfo->id;

        pGBLogicDeviceInfo->phy_ctrlDeviceChannel = 1;

        pGBLogicDeviceInfo->stream_count = 1;
        //pGBLogicDeviceInfo->record_type = 0;

        //pGBLogicDeviceInfo->owner = NULL;
        //pGBLogicDeviceInfo->civil_code = NULL;
        //pGBLogicDeviceInfo->block = NULL;
        //pGBLogicDeviceInfo->address = NULL;
        pGBLogicDeviceInfo->parental = 0;

        pGBLogicDeviceInfo->safety_way = 0;
        pGBLogicDeviceInfo->register_way = 1;

        //pGBLogicDeviceInfo->cert_num = NULL;
        pGBLogicDeviceInfo->certifiable = 0;
        pGBLogicDeviceInfo->error_code = 0;
        //pGBLogicDeviceInfo->end_time = NULL;

        pGBLogicDeviceInfo->secrecy = 0;

        osip_strncpy(pGBLogicDeviceInfo->ip_address, pGBDeviceInfo->login_ip, MAX_IP_LEN);
        pGBLogicDeviceInfo->port = pGBDeviceInfo->login_port;

        pGBLogicDeviceInfo->status = pGBDeviceInfo->reg_status;

        pGBLogicDeviceInfo->longitude = 0.0;
        pGBLogicDeviceInfo->latitude = 0.0;

        i = GBDevice_add(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER, pGBDeviceInfo);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfoByGBDeviceInfo() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_MASTER Error:i=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddGBLogicDeviceInfoByGBDeviceInfo() GBDevice_add:device_id=%s, stream_type=EV9000_STREAM_TYPE_MASTER OK:i=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }

        iLogicDeviceIndex = GBLogicDevice_info_add(pGBLogicDeviceInfo);
    }

    return iLogicDeviceIndex;
}

/*****************************************************************************
 函 数 名  : AddGBLogicDeviceInfo2DB
 功能描述  : 将逻辑设备信息添加到数据库
 输入参数  : int iLogicDeviceIndex
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月20日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int AddGBLogicDeviceInfo2DB(char * device_id, DBOper * pDevice_Srv_dboper)
{
    int iRet = 0;
    int record_count = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    //GBLogicDevice_info_t* pDBGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    string strQuerySQL = "";
    string strInsertSQL = "";
    string strUpdateSQL = "";
    char strDeviceIndex[64] = {0};
    char strTmp[16] = {0};
    char strTmp1[64] = {0};

    if (NULL == device_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "AddGBLogicDeviceInfo2DB() exit---: Param Error \r\n");
        return -1;
    }

    pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

    if ((NULL == pGBLogicDeviceInfo) || (NULL == pGBLogicDeviceInfo->device_id))
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DB() exit---: Get GBLogic Device Info Error:device_id=%s \r\n", device_id);
        return -1;
    }

    pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DB() exit---: GBDevice_info_get_by_stream_type Error \r\n");
        return -1;
    }

    /* 1、查询SQL 语句*/
    strQuerySQL.clear();
    strQuerySQL = "select * from GBLogicDeviceConfig WHERE DeviceID like '";
    strQuerySQL += pGBLogicDeviceInfo->device_id;
    strQuerySQL += "'";


    /* 2、插入SQL 语句*/
    strInsertSQL.clear();
    strInsertSQL = "insert into GBLogicDeviceConfig (ID,DeviceID,CMSID,DeviceName,Enable,CtrlEnable,MicEnable,FrameCount,AlarmLengthOfTime,DeviceType,PhyDeviceIndex,PhyDeviceChannel,CtrlDeviceIndex,CtrlDeviceChannel,StreamCount,RecordType,OtherRealm,Manufacturer,Model,Owner,Block,Address,Parental,ParentID,SafetyWay,RegisterWay,CertNum,Certifiable,ErrCode,EndTime,Secrecy,IPAddress,Port,Password,Status,Longitude,Latitude,Resved2,Resved1) values (";

    /* 逻辑设备索引*/
    memset(strDeviceIndex, 0 , 64);
    snprintf(strDeviceIndex, 64, "%u", pGBLogicDeviceInfo->id);
    strInsertSQL += strDeviceIndex;

    strInsertSQL += ",";

    /* 点位统一编号 */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->device_id;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 所属的CMS ID */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->cms_id;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 点位名称 */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->device_name;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 是否启用 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->enable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 是否可控 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->ctrl_enable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 是否支持对讲 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->mic_enable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 帧率 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->frame_count);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 报警时长 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 点位类型 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->device_type);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 对应的媒体物理设备索引 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceIndex);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 对应的媒体物理设备通道 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceChannel);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 对应的控制物理设备索引 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceIndex);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 对应的控制物理设备通道 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceChannel);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->stream_count);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 录像类型 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->record_type);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 是否属于其他域 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->other_realm);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 设备生产商 */
    strInsertSQL += "'";

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        strInsertSQL += pGBLogicDeviceInfo->manufacturer;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 设备型号 */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->model;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 设备归属 */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->owner;
    strInsertSQL += "'";

    strInsertSQL += ",";

#if 0
    /* 行政区域 */
    strInsertSQL += "'";

    if (NULL != pGBLogicDeviceInfo->civil_code)
    {
        strInsertSQL += pGBLogicDeviceInfo->civil_code;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";
#endif

    /* 警区 */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->block;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 安装地址 */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->address;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 是否有子设备 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->parental);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 父设备/区域/系统ID */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->parentID;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 信令安全模式*/
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->safety_way);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 注册方式 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->register_way);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 证书序列号*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->cert_num;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 证书有效标识 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->certifiable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 无效原因码 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->error_code);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 证书终止有效期*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->end_time;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 保密属性 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->secrecy);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* IP地址*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->ip_address;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 端口号 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->port);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 密码*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->password;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 点位状态 */
    memset(strTmp, 0 , 16);

    if (pGBLogicDeviceInfo->status > 0)
    {
        strInsertSQL += "1";
    }
    else
    {
        strInsertSQL += "0";
    }

    strInsertSQL += ",";

    /* 经度 */
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
    strInsertSQL += strTmp1;

    strInsertSQL += ",";

    /* 纬度 */
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
    strInsertSQL += strTmp1;

    strInsertSQL += ",";

    /* 所属图层 */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->map_layer;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 报警设备子类型 */
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
    strInsertSQL += strTmp1;

    strInsertSQL += ")";


    /* 3、更新SQL 语句*/
    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE GBLogicDeviceConfig SET ";

#if 0
    /* 索引 */
    strUpdateSQL += "ID = ";
    memset(strDeviceIndex, 0 , 64);
    snprintf(strDeviceIndex, 64, "%u", pGBLogicDeviceInfo->id);
    strUpdateSQL += strDeviceIndex;

    strUpdateSQL += ",";
#endif

    /* 所属的CMS ID */
    strUpdateSQL += "CMSID = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->cms_id;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    if (('\0' != pGBLogicDeviceInfo->device_name[0]
         && (0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"IP Camera", 9)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"IPCamera", 8)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"Camera", 6)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"IPC", 3)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"通道", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"固枪", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"动球", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"半球", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, pGBLogicDeviceInfo->device_id, MAX_ID_LEN)))
        || '\0' == pGBLogicDeviceInfo->device_name[0])
    {
        /* 如果前端上报的是 IP Camera，Camera，的名称，或者上报的为空，则不更新数据库，以数据库配置为准 */
    }
    else
    {
        /* 点位名称 */
        strUpdateSQL += "DeviceName = ";
        strUpdateSQL += "'";
        strUpdateSQL += pGBLogicDeviceInfo->device_name;
        strUpdateSQL += "'";

        strUpdateSQL += ",";
    }

    /* 是否启用 */
    strUpdateSQL += "Enable = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->enable);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 是否属于其他域 */
    strUpdateSQL += "OtherRealm = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->other_realm);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    if (pGBLogicDeviceInfo->ctrl_enable > 0)
    {
        /* 是否可控 */
        strUpdateSQL += "CtrlEnable = ";
        memset(strTmp, 0 , 16);
        snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->ctrl_enable);
        strUpdateSQL += strTmp;

        strUpdateSQL += ",";
    }

    if (NULL != pGBDeviceInfo
        && EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
        && 0 == pGBDeviceInfo->three_party_flag) /* 如果是自己的下级平台的点位，则需要更新点位可控标识*/
    {
        /* 是否支持对讲 */
        strUpdateSQL += "MicEnable = ";
        memset(strTmp, 0 , 16);
        snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->mic_enable);
        strUpdateSQL += strTmp;

        strUpdateSQL += ",";

        /* 帧率 */
        strUpdateSQL += "FrameCount = ";
        memset(strTmp, 0 , 16);
        snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->frame_count);
        strUpdateSQL += strTmp;

        strUpdateSQL += ",";

        /* 是否支持多码流 */
        strUpdateSQL += "StreamCount = ";
        memset(strTmp, 0 , 16);
        snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->stream_count);
        strUpdateSQL += strTmp;

        strUpdateSQL += ",";

        /* 告警时长 */
        strUpdateSQL += "AlarmLengthOfTime = ";
        memset(strTmp, 0 , 16);
        snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
        strUpdateSQL += strTmp;

        strUpdateSQL += ",";
    }
    else if (NULL != pGBDeviceInfo && EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)  /* 如果是媒体网关的点位，则需要更新点位双码流标识 */
    {
        /* 是否支持多码流 */
        strUpdateSQL += "StreamCount = ";
        memset(strTmp, 0 , 16);
        snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->stream_count);
        strUpdateSQL += strTmp;

        strUpdateSQL += ",";
    }

    /* 对应的媒体物理设备索引 */
    strUpdateSQL += "PhyDeviceIndex = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceIndex);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

#if 0
    /* 对应的媒体物理设备通道 */
    strUpdateSQL += "PhyDeviceChannel = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceChannel);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 对应的控制物理设备索引 */
    strUpdateSQL += "CtrlDeviceIndex = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceIndex);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 对应的控制物理设备通道 */
    strUpdateSQL += "CtrlDeviceChannel = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceChannel);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 是否支持多码流 */
    strUpdateSQL += "StreamCount = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->stream_count);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 录像类型 */
    strUpdateSQL += "RecordType = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->record_type);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";
#endif

    /* 设备生产商 */
    strUpdateSQL += "Manufacturer = ";
    strUpdateSQL += "'";

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        strUpdateSQL += pGBLogicDeviceInfo->manufacturer;
    }

    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 设备型号 */
    strUpdateSQL += "Model = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->model;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 设备归属 */
    strUpdateSQL += "Owner = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->owner;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

#if 0
    /* 行政区域 */
    strUpdateSQL += "CivilCode = ";
    strUpdateSQL += "'";

    if (NULL != pGBLogicDeviceInfo->civil_code)
    {
        strUpdateSQL += pGBLogicDeviceInfo->civil_code;
    }

    strUpdateSQL += "'";

    strUpdateSQL += ",";
#endif

    /* 警区 */
    strUpdateSQL += "Block = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->block;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 安装地址 */
    strUpdateSQL += "Address = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->address;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 是否有子设备 */
    strUpdateSQL += "Parental = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->parental);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 父设备/区域/系统ID */
    strUpdateSQL += "ParentID = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->parentID;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 信令安全模式*/
    strUpdateSQL += "SafetyWay = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->safety_way);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 注册方式 */
    strUpdateSQL += "RegisterWay = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->register_way);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 证书序列号*/
    strUpdateSQL += "CertNum = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->cert_num;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 证书有效标识 */
    strUpdateSQL += "Certifiable = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->certifiable);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 无效原因码 */
    strUpdateSQL += "ErrCode = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->error_code);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 证书终止有效期*/
    strUpdateSQL += "EndTime = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->end_time;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 保密属性 */
    strUpdateSQL += "Secrecy = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->secrecy);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* IP地址*/
    strUpdateSQL += "IPAddress = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->ip_address;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 端口号 */
    strUpdateSQL += "Port = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->port);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 密码*/
    strUpdateSQL += "Password = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->password;
    strUpdateSQL += "'";

    /* 点位状态 */
    strUpdateSQL += ",";

    strUpdateSQL += "Status = ";
    memset(strTmp, 0 , 16);

    if (pGBLogicDeviceInfo->status > 0)
    {
        strUpdateSQL += "1";
    }
    else
    {
        strUpdateSQL += "0";
    }

    if (NULL != pGBDeviceInfo
        && EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
        && 0 == pGBDeviceInfo->three_party_flag) /* 如果是下级平台的点位，则需要比较经纬度 */
    {
        /* 经度 */
        if (pGBLogicDeviceInfo->longitude > 0)
        {
            strUpdateSQL += ",";

            strUpdateSQL += "Longitude = ";
            memset(strTmp1, 0 , 64);
            snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
            strUpdateSQL += strTmp1;
        }

        /* 纬度 */
        if (pGBLogicDeviceInfo->latitude > 0)
        {
            strUpdateSQL += ",";

            strUpdateSQL += "Latitude = ";
            memset(strTmp1, 0 , 64);
            snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
            strUpdateSQL += strTmp1;
        }

        /* 所属图层 */
        if (pGBLogicDeviceInfo->map_layer[0] != '\0')
        {
            strUpdateSQL += ",";

            strUpdateSQL += "Resved2 = ";
            strUpdateSQL += "'";
            strUpdateSQL += pGBLogicDeviceInfo->map_layer;
            strUpdateSQL += "'";
        }
    }

    strUpdateSQL += ",";

    /* 报警设备子类型 */
    strUpdateSQL += "Resved1 = ";
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
    strUpdateSQL += strTmp1;

    strUpdateSQL += " WHERE DeviceID like '";
    strUpdateSQL += pGBLogicDeviceInfo->device_id;
    strUpdateSQL += "'";

    /* 查询数据库 */
    record_count = pDevice_Srv_dboper->DB_Select(strQuerySQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddGBLogicDeviceInfo2DB() DB Select:record_count=%d,DeviceID=%s \r\n", record_count, pGBLogicDeviceInfo->device_id);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DB() DB Oper Error:strQuerySQL=%s, record_count=%d \r\n", strQuerySQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        iRet = pDevice_Srv_dboper->DB_Insert(strQuerySQL.c_str(), strUpdateSQL.c_str(), strInsertSQL.c_str(), 1);

        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddGBLogicDeviceInfo2DB() DB Insert:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DB() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddGBLogicDeviceInfo2DB() DB Insert OK:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);

            /* 成功之后需要读取其Index 填到逻辑设备结构中 */
            iRet = pDevice_Srv_dboper->DB_Select(strQuerySQL.c_str(), 1);

            if (iRet > 0)
            {
                unsigned int tmp_uivalue = 0;
                pDevice_Srv_dboper->GetFieldValue("ID", tmp_uivalue);

                if (tmp_uivalue > 0)
                {
                    pGBLogicDeviceInfo->id = tmp_uivalue;
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DB() Get ID Error:tmp_uivalue=%u \r\n", tmp_uivalue);
                }
            }
            else if (iRet == 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DB() DB Select:strSQL=%s, No Count Select, iRet=%d \r\n", strQuerySQL.c_str(), iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DB() DB Oper Error:strSQL=%s, iRet=%d \r\n", strQuerySQL.c_str(), iRet);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
            }
        }
    }
    else
    {
        iRet = pDevice_Srv_dboper->DB_Update(strUpdateSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DB() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        }
        else
        {
            if (0 == pGBLogicDeviceInfo->enable)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "更新逻辑设备禁用标识到数据库:逻辑设备ID=%s, 逻辑点位名称=%s, 禁用标识=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBLogicDeviceInfo->enable);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Update logic device disable identification in database:logic device ID=%s, logic point name =%s, Disable identification =%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBLogicDeviceInfo->enable);

            }

            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddGBLogicDeviceInfo2DB() DB Update OK:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
        }
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : AddGBLogicDeviceInfo2DBForRoute
 功能描述  : 将逻辑设备信息添加到数据库
 输入参数  : char * device_id
             DBOper * pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月10日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddGBLogicDeviceInfo2DBForRoute(char * device_id, DBOper * pDevice_Srv_dboper)
{
    int iRet = 0;
    int record_count = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    string strQuerySQL = "";
    string strInsertSQL = "";
    string strUpdateSQL = "";
    char strDeviceIndex[64] = {0};
    char strTmp[16] = {0};
    char strTmp1[64] = {0};

    if (NULL == device_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "AddGBLogicDeviceInfo2DBForRoute() exit---: Param Error \r\n");
        return -1;
    }

    pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

    if ((NULL == pGBLogicDeviceInfo) || ('\0' == pGBLogicDeviceInfo->device_id[0]))
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DBForRoute() exit---: Get GBLogic Device Info Error:device_id=%s \r\n", device_id);
        return -1;
    }

    /* 1、查询SQL 语句*/
    strQuerySQL.clear();
    strQuerySQL = "select * from GBLogicDeviceConfig WHERE DeviceID like '";
    strQuerySQL += pGBLogicDeviceInfo->device_id;
    strQuerySQL += "'";


    /* 2、插入SQL 语句*/
    strInsertSQL.clear();
    strInsertSQL = "insert into GBLogicDeviceConfig (ID,DeviceID,CMSID,DeviceName,Enable,CtrlEnable,MicEnable,FrameCount,AlarmLengthOfTime,DeviceType,PhyDeviceIndex,PhyDeviceChannel,CtrlDeviceIndex,CtrlDeviceChannel,StreamCount,RecordType,OtherRealm,Manufacturer,Model,Owner,Block,Address,Parental,ParentID,SafetyWay,RegisterWay,CertNum,Certifiable,ErrCode,EndTime,Secrecy,IPAddress,Port,Password,Status,Longitude,Latitude,Resved2,Resved1) values (";

    /* 逻辑设备索引*/
    memset(strDeviceIndex, 0 , 64);
    snprintf(strDeviceIndex, 64, "%u", pGBLogicDeviceInfo->id);
    strInsertSQL += strDeviceIndex;

    strInsertSQL += ",";

    /* 点位统一编号 */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->device_id;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 所属的CMS ID */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->cms_id;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 点位名称 */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->device_name;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 是否启用 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->enable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 是否可控 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->ctrl_enable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 是否支持对讲 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->mic_enable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 帧率 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->frame_count);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 报警时长 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 点位类型 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->device_type);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 对应的媒体物理设备索引 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceIndex);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 对应的媒体物理设备通道 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceChannel);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 对应的控制物理设备索引 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceIndex);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 对应的控制物理设备通道 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceChannel);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->stream_count);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 录像类型 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->record_type);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 是否属于其他域 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->other_realm);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 设备生产商 */
    strInsertSQL += "'";

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        strInsertSQL += pGBLogicDeviceInfo->manufacturer;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 设备型号 */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->model;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 设备归属 */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->owner;
    strInsertSQL += "'";

    strInsertSQL += ",";

#if 0
    /* 行政区域 */
    strInsertSQL += "'";

    if (NULL != pGBLogicDeviceInfo->civil_code)
    {
        strInsertSQL += pGBLogicDeviceInfo->civil_code;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";
#endif

    /* 警区 */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->block;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 安装地址 */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->address;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 是否有子设备 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->parental);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 父设备/区域/系统ID */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->parentID;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 信令安全模式*/
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->safety_way);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 注册方式 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->register_way);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 证书序列号*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->cert_num;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 证书有效标识 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->certifiable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 无效原因码 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->error_code);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 证书终止有效期*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->end_time;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 保密属性 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->secrecy);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* IP地址*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->ip_address;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 端口号 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->port);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* 密码*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->password;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 点位状态 */
    memset(strTmp, 0 , 16);

    if (pGBLogicDeviceInfo->status > 0)
    {
        strInsertSQL += "1";
    }
    else
    {
        strInsertSQL += "0";
    }

    strInsertSQL += ",";

    /* 经度 */
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
    strInsertSQL += strTmp1;

    strInsertSQL += ",";

    /* 纬度 */
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
    strInsertSQL += strTmp1;

    strInsertSQL += ",";

    /* 所属图层 */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->map_layer;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* 报警设备子类型 */
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
    strInsertSQL += strTmp1;

    strInsertSQL += ")";


    /* 3、更新SQL 语句*/
    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE GBLogicDeviceConfig SET ";

#if 0
    /* 索引 */
    strUpdateSQL += "ID = ";
    memset(strDeviceIndex, 0 , 64);
    snprintf(strDeviceIndex, 64, "%u", pGBLogicDeviceInfo->id);
    strUpdateSQL += strDeviceIndex;

    strUpdateSQL += ",";
#endif

    /* 所属的CMS ID */
    strUpdateSQL += "CMSID = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->cms_id;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    if (('\0' != pGBLogicDeviceInfo->device_name[0]
         && (0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"IP Camera", 9)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"IPCamera", 8)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"Camera", 6)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"IPC", 3)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"通道", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"固枪", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"动球", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"半球", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, pGBLogicDeviceInfo->device_id, MAX_ID_LEN)))
        || '\0' == pGBLogicDeviceInfo->device_name[0])
    {
        /* 如果前端上报的是 IP Camera，Camera，的名称，或者上报的为空，则不更新数据库，以数据库配置为准 */
    }
    else
    {
        /* 点位名称 */
        strUpdateSQL += "DeviceName = ";
        strUpdateSQL += "'";
        strUpdateSQL += pGBLogicDeviceInfo->device_name;
        strUpdateSQL += "'";

        strUpdateSQL += ",";
    }

    /* 是否启用 */
    strUpdateSQL += "Enable = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->enable);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 是否属于其他域 */
    strUpdateSQL += "OtherRealm = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->other_realm);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 是否可控 */
    strUpdateSQL += "CtrlEnable = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->ctrl_enable);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 是否支持对讲 */
    strUpdateSQL += "MicEnable = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->mic_enable);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 帧率 */
    strUpdateSQL += "FrameCount = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->frame_count);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 是否支持多码流 */
    strUpdateSQL += "StreamCount = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->stream_count);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 告警时长 */
    strUpdateSQL += "AlarmLengthOfTime = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 对应的媒体物理设备索引 */
    strUpdateSQL += "PhyDeviceIndex = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceIndex);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

#if 0
    /* 对应的媒体物理设备通道 */
    strUpdateSQL += "PhyDeviceChannel = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceChannel);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 对应的控制物理设备索引 */
    strUpdateSQL += "CtrlDeviceIndex = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceIndex);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 对应的控制物理设备通道 */
    strUpdateSQL += "CtrlDeviceChannel = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceChannel);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 是否支持多码流 */
    strUpdateSQL += "StreamCount = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->stream_count);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 录像类型 */
    strUpdateSQL += "RecordType = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->record_type);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";
#endif

    /* 设备生产商 */
    strUpdateSQL += "Manufacturer = ";
    strUpdateSQL += "'";

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        strUpdateSQL += pGBLogicDeviceInfo->manufacturer;
    }

    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 设备型号 */
    strUpdateSQL += "Model = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->model;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 设备归属 */
    strUpdateSQL += "Owner = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->owner;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

#if 0
    /* 行政区域 */
    strUpdateSQL += "CivilCode = ";
    strUpdateSQL += "'";

    if (NULL != pGBLogicDeviceInfo->civil_code)
    {
        strUpdateSQL += pGBLogicDeviceInfo->civil_code;
    }

    strUpdateSQL += "'";

    strUpdateSQL += ",";
#endif

    /* 警区 */
    strUpdateSQL += "Block = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->block;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 安装地址 */
    strUpdateSQL += "Address = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->address;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 是否有子设备 */
    strUpdateSQL += "Parental = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->parental);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 父设备/区域/系统ID */
    strUpdateSQL += "ParentID = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->parentID;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 信令安全模式*/
    strUpdateSQL += "SafetyWay = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->safety_way);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 注册方式 */
    strUpdateSQL += "RegisterWay = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->register_way);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 证书序列号*/
    strUpdateSQL += "CertNum = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->cert_num;

    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 证书有效标识 */
    strUpdateSQL += "Certifiable = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->certifiable);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 无效原因码 */
    strUpdateSQL += "ErrCode = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->error_code);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 证书终止有效期*/
    strUpdateSQL += "EndTime = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->end_time;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 保密属性 */
    strUpdateSQL += "Secrecy = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->secrecy);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* IP地址*/
    strUpdateSQL += "IPAddress = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->ip_address;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* 端口号 */
    strUpdateSQL += "Port = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->port);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* 密码*/
    strUpdateSQL += "Password = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->password;
    strUpdateSQL += "'";

    /* 点位状态 */
    strUpdateSQL += ",";

    strUpdateSQL += "Status = ";
    memset(strTmp, 0 , 16);

    if (pGBLogicDeviceInfo->status > 0)
    {
        strUpdateSQL += "1";
    }
    else
    {
        strUpdateSQL += "0";
    }

    /* 经度 */
    if (pGBLogicDeviceInfo->longitude > 0)
    {
        strUpdateSQL += ",";

        strUpdateSQL += "Longitude = ";
        memset(strTmp1, 0 , 64);
        snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
        strUpdateSQL += strTmp1;
    }

    /* 纬度 */
    if (pGBLogicDeviceInfo->longitude > 0)
    {
        strUpdateSQL += ",";

        strUpdateSQL += "Latitude = ";
        memset(strTmp1, 0 , 64);
        snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
        strUpdateSQL += strTmp1;

    }

    /* 所属图层 */
    if (pGBLogicDeviceInfo->map_layer[0] != '\0')
    {
        strUpdateSQL += ",";

        strUpdateSQL += "Resved2 = ";
        strUpdateSQL += "'";
        strUpdateSQL += pGBLogicDeviceInfo->map_layer;
        strUpdateSQL += "'";

        strUpdateSQL += ",";
    }

    /* 报警设备子类型 */
    strUpdateSQL += "Resved1 = ";
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
    strUpdateSQL += strTmp1;

    strUpdateSQL += " WHERE DeviceID like '";
    strUpdateSQL += pGBLogicDeviceInfo->device_id;
    strUpdateSQL += "'";

    /* 查询数据库 */
    record_count = pDevice_Srv_dboper->DB_Select(strQuerySQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddGBLogicDeviceInfo2DBForRoute() DB Select:record_count=%d,DeviceID=%s \r\n", record_count, pGBLogicDeviceInfo->device_id);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DBForRoute() DB Oper Error:strQuerySQL=%s, record_count=%d \r\n", strQuerySQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DBForRoute() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        iRet = pDevice_Srv_dboper->DB_Insert(strQuerySQL.c_str(), strUpdateSQL.c_str(), strInsertSQL.c_str(), 1);

        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddGBLogicDeviceInfo2DB() DB Insert:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DBForRoute() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DBForRoute() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddGBLogicDeviceInfo2DBForRoute() DB Insert OK:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);

            /* 成功之后需要读取其Index 填到逻辑设备结构中 */
            iRet = pDevice_Srv_dboper->DB_Select(strQuerySQL.c_str(), 1);

            if (iRet > 0)
            {
                unsigned int tmp_uivalue = 0;
                pDevice_Srv_dboper->GetFieldValue("ID", tmp_uivalue);

                if (tmp_uivalue > 0)
                {
                    pGBLogicDeviceInfo->id = tmp_uivalue;
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DBForRoute() Get ID Error:tmp_uivalue=%u \r\n", tmp_uivalue);
                }
            }
            else if (iRet == 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DBForRoute() DB Select:strSQL=%s, No Count Select, iRet=%d \r\n", strQuerySQL.c_str(), iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DBForRoute() DB Oper Error:strSQL=%s, iRet=%d \r\n", strQuerySQL.c_str(), iRet);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DBForRoute() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
            }
        }
    }
    else
    {
        iRet = pDevice_Srv_dboper->DB_Update(strUpdateSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DBForRoute() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddGBLogicDeviceInfo2DBForRoute() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        }
        else
        {
            if (0 == pGBLogicDeviceInfo->enable)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "更新逻辑设备禁用标识到数据库:逻辑设备ID=%s, 逻辑点位名称=%s, 禁用标识=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBLogicDeviceInfo->enable);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Update logic device disable identification in database:logic device ID=%s, logic point name =%s, Disable identification =%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBLogicDeviceInfo->enable);

            }

            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddGBLogicDeviceInfo2DBForRoute() DB Update OK:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
        }
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : CheckIsGBLogicDeviceInfoInDB
 功能描述  : 检查逻辑设备信息是否存在于数据库
 输入参数  : char* device_id
                            DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月8日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
int CheckIsGBLogicDeviceInfoInDB(char * device_id, DBOper * pDevice_Srv_dboper)
{
    int iRet = 0;
    int record_count = 0;
    string strQuerySQL = "";

    if ((NULL == device_id) || (NULL == pDevice_Srv_dboper))
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "CheckIsGBLogicDeviceInfoInDB() exit---: Param Error \r\n");
        return -1;
    }

    /* 1、查询SQL 语句*/
    strQuerySQL.clear();
    strQuerySQL = "select * from GBLogicDeviceConfig WHERE DeviceID like '";
    strQuerySQL += device_id;
    strQuerySQL += "'";

    /* 查询数据库 */
    record_count = pDevice_Srv_dboper->DB_Select(strQuerySQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "CheckIsGBLogicDeviceInfoInDB() DB Oper Error:strQuerySQL=%s, record_count=%d \r\n", strQuerySQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "CheckIsGBLogicDeviceInfoInDB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : GetGBLogicDeviceRecordTypeFromDB
 功能描述  : 获取逻辑设备的录像类型
 输入参数  : char* device_id
                            DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月16日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GetGBLogicDeviceRecordTypeFromDB(char * device_id, DBOper * pDevice_Srv_dboper)
{
    int record_count = 0;
    string strQuerySQL = "";
    int tmp_ivalue = 0;

    if ((NULL == device_id) || (NULL == pDevice_Srv_dboper))
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GetGBLogicDeviceRecordTypeFromDB() exit---: Param Error \r\n");
        return -1;
    }

    /* 1、查询SQL 语句*/
    strQuerySQL.clear();
    strQuerySQL = "select RecordType from GBLogicDeviceConfig WHERE DeviceID like '";
    strQuerySQL += device_id;
    strQuerySQL += "'";

    /* 查询数据库 */
    record_count = pDevice_Srv_dboper->DB_Select(strQuerySQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GetGBLogicDeviceRecordTypeFromDB() DB Oper Error:strQuerySQL=%s, record_count=%d \r\n", strQuerySQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GetGBLogicDeviceRecordTypeFromDB() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        return 0;
    }

    /* 录像类型 */
    tmp_ivalue = 0;
    pDevice_Srv_dboper->GetFieldValue(0, tmp_ivalue);

    return tmp_ivalue;
}

/*****************************************************************************
 函 数 名  : SendDeviceOffLineAlarmToAllClientUser
 功能描述  : 发送设备掉线告警给在线客户端用户
 输入参数  : char* device_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月7日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SendDeviceOffLineAlarmToAllClientUser(char * device_id)
{
    int iRet = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    unsigned int uType = EV9000_ALARM_LOGIC_DEVICE_ERROR;

    if (NULL == device_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendDeviceOffLineAlarmToAllClientUser() exit---: Param Error \r\n");
        return -1;
    }

    pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

    if (NULL == pGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceOffLineAlarmToAllClientUser() exit---: Get Device Info Error:device_id=%s \r\n", device_id);
        return -1;
    }

    iRet = SystemFaultAlarm(pGBLogicDeviceInfo->id, device_id, uType, (char*)"2", (char*)"0x01400002", "前端逻辑设备掉线:点位ID=%s,点位名称=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);
    iRet = EnSystemFaultAlarm(pGBLogicDeviceInfo->id, device_id, uType, (char*)"2", (char*)"0x01400002", "The front end logic device off line:device ID=%s,device name=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);

    return iRet;
}

/*****************************************************************************
 函 数 名  : SendDeviceNoStreamAlarmToAllClientUser
 功能描述  : 发送设备无码流告警给在线客户端用户
 输入参数  : char * device_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月22日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendDeviceNoStreamAlarmToAllClientUser(char * device_id)
{
    int iRet = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    unsigned int uType = EV9000_ALARM_LOGIC_DEVICE_ERROR;

    if (NULL == device_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendDeviceNoStreamAlarmToAllClientUser() exit---: Param Error \r\n");
        return -1;
    }

    pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

    if (NULL == pGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceNoStreamAlarmToAllClientUser() exit---: Get Device Info Error:device_id=%s \r\n", device_id);
        return -1;
    }

    iRet = SystemFaultAlarm(pGBLogicDeviceInfo->id, device_id, uType, (char*)"2", (char*)"0x01400002", "前端逻辑没有码流:点位ID=%s,点位名称=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);
    iRet = EnSystemFaultAlarm(pGBLogicDeviceInfo->id, device_id, uType, (char*)"2", (char*)"0x01400002", "The front end logic device no stream:device ID=%s,device name=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);

    return iRet;
}

/*****************************************************************************
 函 数 名  : SendIntelligentDeviceOffLineAlarmToAllClientUser
 功能描述  : 发送智能分析点位掉线信息给用户
 输入参数  : char* device_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月23日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendIntelligentDeviceOffLineAlarmToAllClientUser(char * device_id)
{
    int iRet = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    unsigned int uType = EV9000_ALARM_LOGIC_DEVICE_ERROR;

    if (NULL == device_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendIntelligentDeviceOffLineAlarmToAllClientUser() exit---: Param Error \r\n");
        return -1;
    }

    pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

    if (NULL == pGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendIntelligentDeviceOffLineAlarmToAllClientUser() exit---: Get Device Info Error:device_id=%s \r\n", device_id);
        return -1;
    }

    iRet = SystemFaultAlarm(pGBLogicDeviceInfo->id, device_id, uType, (char*)"2", (char*)"0x01400002", "前端逻辑设备智能分析状态掉线:点位ID=%s,点位名称=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);
    iRet = EnSystemFaultAlarm(pGBLogicDeviceInfo->id, device_id, uType, (char*)"2", (char*)"0x01400002", "The front end logic device intelligent status off:device ID=%s,device name=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);

    return iRet;
}

/*****************************************************************************
 函 数 名  : SendGBPhyDeviceOffLineAlarmToAllClientUser
 功能描述  : 发送物理设备故障信息给用户
 输入参数  : unsigned int uType
             unsigned int device_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月22日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendGBPhyDeviceOffLineAlarmToAllClientUser(unsigned int uType, unsigned int device_index)
{
    int iRet = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;

    pGBDeviceInfo = GBDevice_info_find_by_device_index(device_index);

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendGBPhyDeviceOffLineAlarmToAllClientUser() exit---: GBDevice_info_find_by_device_index Error:device_index=%d \r\n", device_index);
        return -1;
    }

    iRet = SystemFaultAlarm(device_index, pGBDeviceInfo->device_id, uType, (char*)"2", (char*)"0x01400002", "前端物理设备掉线:物理设备 ID=%s, 物理设备 IP地址=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
    iRet = EnSystemFaultAlarm(device_index, pGBDeviceInfo->device_id, uType, (char*)"2", (char*)"0x01400002", "The front end device off line:device ID=%s, device IP addr=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);

    return iRet;
}

/*****************************************************************************
 函 数 名  : SendAllGBLogicDeviceStatusOffAlarmToAllClientUser
 功能描述  : 根据物理设备索引，发送所有下面的逻辑设备下线告警给在线客户端
 输入参数  : int device_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月7日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int SendAllGBLogicDeviceStatusOffAlarmToAllClientUser(int device_index)
{
    int iRet = 0;
    int index = -1;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    vector<string> DeviceIDVector;

    if (device_index <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendAllGBLogicDeviceStatusOffAlarmToAllClientUser() exit---: Param Error \r\n");
        return -1;
    }

    DeviceIDVector.clear();

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->phy_mediaDeviceIndex <= 0))
        {
            continue;
        }

        if (pGBLogicDeviceInfo->phy_mediaDeviceIndex == device_index)
        {
            DeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    if (DeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)DeviceIDVector.size(); index++)
        {
            iRet = SendDeviceOffLineAlarmToAllClientUser((char*)DeviceIDVector[index].c_str());
        }
    }

    DeviceIDVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser
 功能描述  : 根据物理设备索引，发送所有下面的逻辑设备智能分析状态下线告警给
             在线客户端
 输入参数  : int device_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月23日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser(int device_index)
{
    int iRet = 0;
    int index = -1;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    vector<string> DeviceIDVector;
    GBDevice_info_t* pGBDeviceInfo = NULL;

    if (device_index <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser() exit---: Param Error \r\n");
        return -1;
    }

    DeviceIDVector.clear();

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->phy_mediaDeviceIndex <= 0))
        {
            continue;
        }

        pGBDeviceInfo = GBDevice_info_get_by_stream_type2(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_INTELLIGENCE);

        if (NULL != pGBDeviceInfo)
        {
            if (pGBDeviceInfo->id == device_index)
            {
                DeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
            }
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    if (DeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)DeviceIDVector.size(); index++)
        {
            iRet = SendIntelligentDeviceOffLineAlarmToAllClientUser((char*)DeviceIDVector[index].c_str());
        }
    }

    DeviceIDVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : SendQuerySubCMSDBIPMessage
 功能描述  : 发送获取下级CMS IP地址消息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月10日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendQuerySubCMSDBIPMessage(GBDevice_info_t * pGBDeviceInfo)
{
    int i = 0;

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendQuerySubCMSDBIPMessage() exit---: Param Error \r\n");
        return -1;
    }

    /* 组建XML信息 */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"GetDBIP");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"123");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    /* 推送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送查询下级CMS 数据库IP地址Message消息到设备失败:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendQuerySubCMSDBIPMessage error:dest_id=%s, dest_ip=%s, dest_port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQuerySubCMSDBIPMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送查询下级CMS 数据库IP地址Message消息到设备成功:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendQuerySubCMSDBIPMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQuerySubCMSDBIPMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendExecuteDevicePresetMessageToSubCMS
 功能描述  : 发送执行预置位消息给下级CMS
 输入参数  : char* strPresetID
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月28日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendExecuteDevicePresetMessageToSubCMS(char* strPresetID)
{
    int i = 0;
    int index = -1;
    GBDevice_info_t * pGBDeviceInfo = NULL;
    GBDevice_Info_Iterator Itr;

    CPacket outPacket;
    DOMElement* AccNode = NULL;
    vector<string> DeviceIDVector;

    DeviceIDVector.clear();

    GBDEVICE_SMUTEX_LOCK();

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        GBDEVICE_SMUTEX_UNLOCK();
        return NULL;
    }

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        pGBDeviceInfo = Itr->second;

        if (NULL == pGBDeviceInfo || 0 == pGBDeviceInfo->reg_status)
        {
            continue;
        }

        if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type && 1 == pGBDeviceInfo->link_type)
        {
            DeviceIDVector.push_back(pGBDeviceInfo->device_id);
        }
    }

    GBDEVICE_SMUTEX_UNLOCK();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送执行预置位消息到所有下级CMS: 下级CMS数=%d", (int)DeviceIDVector.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendExcuteResetMessageToAllSubCMS: SubCMSSum=%d", (int)DeviceIDVector.size());

    if (DeviceIDVector.size() > 0)
    {
        /* 组建XML信息 */
        outPacket.SetRootTag("Control");
        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"ExecutePreset");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, (char*)"4589");

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

        AccNode = outPacket.CreateElement((char*)"PresetID");
        outPacket.SetElementValue(AccNode, strPresetID);

        for (index = 0; index < (int)DeviceIDVector.size(); index++)
        {
            pGBDeviceInfo = GBDevice_info_find((char*)DeviceIDVector[index].c_str());

            if (NULL != pGBDeviceInfo)
            {
                /* 推送消息 */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送执行预置位Message消息到下级CMS失败:下级CMS ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendExecuteDevicePresetMessageToSubCMS Error:dest_id=%s, dest_ip=%s, dest_port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendExecuteDevicePresetMessageToSubCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送执行预置位Message消息到下级CMS成功:下级CMS ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendExecuteDevicePresetMessageToSubCMS OK:dest_id=%s, dest_ip=%s, dest_port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendExecuteDevicePresetMessageToSubCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
            }
        }
    }

    DeviceIDVector.clear();

    return i;
}

/*****************************************************************************
 函 数 名  : SendQuerySubCMSTopologyPhyDeviceConfigMessage
 功能描述  : 发送获取下级CMS的拓扑物理设备配置表的消息
 输入参数  : GBDevice_info_t* pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月27日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendQuerySubCMSTopologyPhyDeviceConfigMessage(GBDevice_info_t * pGBDeviceInfo)
{
    int i = 0;

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendQuerySubCMSTopologyPhyDeviceConfigMessage() exit---: Param Error \r\n");
        return -1;
    }

    /* 组建XML信息 */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"TopologyPhyDeviceConfig");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"223");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    /* 推送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送查询下级CMS拓扑物理设备配置Message消息到设备失败:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendQuerySubCMSToPologyPhysicalDeviceMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQuerySubCMSDBIPMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送查询下级CMS拓扑物理设备配置Message消息到设备成功:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendQuerySubCMSToPologyPhysicalDeviceMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQuerySubCMSDBIPMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendNotifyRestartMessageToSubCMS
 功能描述  : CMS通知下级CMS本级CMS重启命令
 输入参数  : GBDevice_info_t* pGBDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月5日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendNotifyRestartMessageToSubCMS(GBDevice_info_t * pGBDeviceInfo)
{
    int i = 0;

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendNotifyRestartMessageToSubCMS() exit---: Param Error \r\n");
        return -1;
    }

    /* 组建XML信息 */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"CMSRestart");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"23456");

    AccNode = outPacket.CreateElement((char*)"CMSID");
    outPacket.SetElementValue(AccNode, local_cms_id_get());

    /* 推送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送本级CMS重启通知命令Message消息到设备失败:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendNotifyRestartMessageToSubCMS Error:dest_id=%s, dest_ip=%s, dest_port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyRestartMessageToSubCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送本级CMS重启通知命令Message消息到设备成功:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyRestartMessageToSubCMS OK:dest_id=%s, dest_ip=%s, dest_port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendNotifyRestartMessageToSubCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendNotifyRestartMessageToAllSubCMS
 功能描述  : 发送重启消息到所有下级互连CMS
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月9日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendNotifyRestartMessageToAllSubCMS()
{
    int i = 0;

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    GBDevice_Info_Iterator Itr;

    GBDevice_info_t* pProcGBDeviceInfo = NULL;
    needtoproc_GBDeviceinfo_queue needProc;

    /* 组建XML信息 */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"CMSRestart");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"23456");

    AccNode = outPacket.CreateElement((char*)"CMSID");
    outPacket.SetElementValue(AccNode, local_cms_id_get());

    needProc.clear();

    GBDEVICE_SMUTEX_LOCK();

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        GBDEVICE_SMUTEX_UNLOCK();
        return 0;
    }

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        pProcGBDeviceInfo = Itr->second;

        if ((NULL == pProcGBDeviceInfo) || (pProcGBDeviceInfo->id <= 0))
        {
            continue;
        }

        if (pProcGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER && pProcGBDeviceInfo->three_party_flag == 0) /* 非第三方平台 */
        {
            needProc.push_back(pProcGBDeviceInfo);
        }
    }

    GBDEVICE_SMUTEX_UNLOCK();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送重启消息到所有下级CMS: 下级CMS数=%d", (int)needProc.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyRestartMessageToAllSubCMS: SubCMS=%d", (int)needProc.size());

    if ((int)needProc.size() == 0)
    {
        return 0;
    }

    while (!needProc.empty())
    {
        pProcGBDeviceInfo = (GBDevice_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pProcGBDeviceInfo)
        {
            /* 推送消息 */
            i |= SIP_SendMessage(NULL, local_cms_id_get(), pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->strRegServerIP, pProcGBDeviceInfo->iRegServerPort, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送本级CMS重启通知命令Message消息到下级CMS失败:下级CMS ID=%s, IP地址=%s, 端口号=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendNotifyRestartMessageToAllSubCMS Error:dest_id=%s, dest_ip=%s, dest_port=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyRestartMessageToAllSubCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送本级CMS重启通知命令Message消息到下级CMS成功:下级CMS ID=%s, IP地址=%s, 端口号=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyRestartMessageToAllSubCMS OK:dest_id=%s, dest_ip=%s, dest_port=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendNotifyRestartMessageToAllSubCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
            }
        }
    }

    needProc.clear();

    return 1;
}

/*****************************************************************************
 函 数 名  : SendTSUInfoMessageToDEC
 功能描述  : 发送TSU信息给DEC
 输入参数  : char* callee_id
             char* local_ip
             int local_port
             char* remote_ip
             int remote_port
             char* strSN
             char* device_id
             char* tsu_ip
             int tsu_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月15日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendTSUInfoMessageToDEC(char* callee_id, char* local_ip, int local_port, char* remote_ip, int remote_port, char* strSN, char* device_id, char* tsu_ip, int tsu_port)
{
    int i = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    char strPort[32] = {0};

    if (NULL == callee_id || NULL == local_ip || NULL == remote_ip || NULL == strSN || NULL == device_id || local_port <= 0 || remote_port <= 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendTSUInfoMessageToDEC() exit---: Param Error \r\n");
        return -1;
    }

    /* 组建XML信息 */
    outPacket.SetRootTag("Response");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"GetTSUIPAndPort");

    AccNode = outPacket.CreateElement((char*)"SN");

    if (NULL != strSN)
    {
        outPacket.SetElementValue(AccNode, strSN);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    AccNode = outPacket.CreateElement((char*)"DeviceID");

    if (NULL != device_id)
    {
        outPacket.SetElementValue(AccNode, device_id);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    AccNode = outPacket.CreateElement((char*)"Result");
    outPacket.SetElementValue(AccNode, (char*)"OK");

    AccNode = outPacket.CreateElement((char*)"TSUIP");

    if (NULL != tsu_ip)
    {
        outPacket.SetElementValue(AccNode, tsu_ip);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }


    AccNode = outPacket.CreateElement((char*)"TSUPort");
    snprintf(strPort, 32, "%d", tsu_port);
    outPacket.SetElementValue(AccNode, strPort);

    /* 发送消息给上级CMS */
    i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, local_ip, local_port, remote_ip, remote_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送TSU IP地址信息Message消息到解码器失败:解码器ID=%s, IP地址=%s, 端口号=%d", callee_id, remote_ip, remote_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendTSUInfoMessageToDEC Error:dest_id=%s, dest_ip=%s, dest_port=%d", callee_id, remote_ip, remote_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendTSUInfoMessageToDEC() SIP_SendMessage Error :dest_id=%s, dest_ip=%s, dest_port=%d \r\n", callee_id, remote_ip, remote_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送TSU IP地址信息Message消息到解码器成功:解码器ID=%s, IP地址=%s, 端口号=%d", callee_id, remote_ip, remote_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendTSUInfoMessageToDEC OK:dest_id=%s, dest_ip=%s, dest_port=%d", callee_id, remote_ip, remote_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendTSUInfoMessageToDEC() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", callee_id, remote_ip, remote_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendSubscribeMessageToSubGBDevice
 功能描述  : 发送订阅消息给下级国标设备
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             int iFlag
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月16日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendSubscribeMessageToSubGBDevice(GBDevice_info_t* pGBDeviceInfo, int iFlag)
{
    int i = 0;
    int index = -1;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    char strSN[128] = {0};

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendSubscribeMessageToSubGBDevice() exit---: Param Error \r\n");
        return -1;
    }

    if (pGBDeviceInfo->reg_status == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendSubscribeMessageToSubGBDevice() exit---: GBDevice Info Not Register \r\n");
        return -1;
    }

    /* 组建XML信息 */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"Catalog");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"17430");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    if (0 == iFlag) /* 初始订阅 */
    {
        subscribe_event_id++;
        pGBDeviceInfo->catalog_subscribe_event_id = subscribe_event_id;
        snprintf(strSN, 128, "%u", pGBDeviceInfo->catalog_subscribe_event_id);
        index = SIP_SendSubscribe(strSN, local_cms_id_get(), pGBDeviceInfo->device_id, (char*)"Catalog", pGBDeviceInfo->catalog_subscribe_event_id, local_subscribe_expires_get(), pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());
        pGBDeviceInfo->catalog_subscribe_dialog_index = index;
    }
    else if (1 == iFlag) /* 刷新订阅 */
    {
        i = SIP_SubscribeRefresh(pGBDeviceInfo->catalog_subscribe_dialog_index);
    }
    else if (2 == iFlag) /* 取消订阅 */
    {
        i = SIP_UnSubscribe(pGBDeviceInfo->catalog_subscribe_dialog_index);
    }

    pGBDeviceInfo->catalog_subscribe_flag = 0;
    pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get();

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送订阅Subscribe消息到前端设备失败:前端设备ID=%s, IP地址=%s, 端口号=%d, subscribe_event_id=%d, Flag=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->catalog_subscribe_event_id, iFlag);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendSubscribeMessageToSubGBDevice Error:ID=%s, IP=%s, port=%d, subscribe_event_id=%d, Flag=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->catalog_subscribe_event_id, iFlag);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendSubscribeMessageToSubGBDevice() SIP_SendSubscribe Error \r\n");
    }
    else if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送订阅Subscribe消息到前端设备成功:前端设备ID=%s, IP地址=%s, 端口号=%d, subscribe_event_id=%d, Flag=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->catalog_subscribe_event_id, iFlag);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendSubscribeMessageToSubGBDevice OK:ID=%s, IP=%s, port=%d, subscribe_event_id=%d, Flag=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->catalog_subscribe_event_id, iFlag);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendSubscribeMessageToSubGBDevice() SIP_SendSubscribe OK \r\n");
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendRCUDeviceStatusToSubCMS
 功能描述  : 发送RCU设备状态到下级CMS
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendRCUDeviceStatusToSubCMS(GBLogicDevice_info_t* pGBLogicDeviceInfo, DBOper* ptDBoper)
{
    int i = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;

    int record_count = 0;
    string strSQL = "";
    char strGBLogicDeviceIndex[32] = {0};
    int device_index = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    vector<unsigned int> DeviceIndexVector;
    int while_count = 0;
    int index = 0;

    char strGuard[32] = {0};
    char strAlarmPriority[32] = {0};
    char strStatus[32] = {0};


    if (NULL == pGBLogicDeviceInfo || NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendRCUDeviceStatusToSubCMS() exit---: Param Error \r\n");
        return -1;
    }

    /* 获取该点推送的下级CMS设备 */
    strSQL.clear();
    strSQL = "select GBPhyDeviceIndex from GBPhyDevicePermConfig WHERE DeviceIndex = "; /* 查询权限表，获取到对应的物理设备 */
    snprintf(strGBLogicDeviceIndex, 32, "%u", pGBLogicDeviceInfo->id);
    strSQL += strGBLogicDeviceIndex;

    record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendRCUDeviceStatusToSubCMS() record_count=%d, DeviceIndex=%d \r\n", record_count, device_index);

    if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendRCUDeviceStatusToSubCMS() exit---: No Record Count \r\n");
        return 0;
    }
    else if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendRCUDeviceStatusToSubCMS() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendRCUDeviceStatusToSubCMS() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return -1;
    }

    /* 组建消息 */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"RCUDeviceStatus");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"132");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

    /*状态*/
    AccNode = outPacket.CreateElement((char*)"Status");
    snprintf(strStatus, 32, "%u", pGBLogicDeviceInfo->status);
    outPacket.SetElementValue(AccNode, strStatus);

    /*RCU上报的报警级别*/
    AccNode = outPacket.CreateElement((char*)"AlarmPriority");
    snprintf(strAlarmPriority, 32, "%u", pGBLogicDeviceInfo->AlarmPriority);
    outPacket.SetElementValue(AccNode, strAlarmPriority);

    /*RCU上报的Guard */
    AccNode = outPacket.CreateElement((char*)"Guard");
    snprintf(strGuard, 32, "%u", pGBLogicDeviceInfo->guard_type);
    outPacket.SetElementValue(AccNode, strGuard);

    /* RCU上报的Value */
    AccNode = outPacket.CreateElement((char*)"Value");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->Value);

    /* RCU上报的Unit */
    AccNode = outPacket.CreateElement((char*)"Unit");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->Unit);

    /* 循环读取数据库 */
    do
    {
        unsigned int tmp_uivalue = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "SendRCUDeviceStatusToSubCMS() While Count=%d \r\n", while_count);
        }

        ptDBoper->GetFieldValue(0, tmp_uivalue);

        if (tmp_uivalue <= 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendRCUDeviceStatusToSubCMS() DeviceIndex Error \r\n");
            continue;
        }

        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddAllGBLogicDeviceIDToVector() DeviceIndex=%u \r\n", tmp_uivalue);

        i = AddDeviceIndexToDeviceIndexVector(DeviceIndexVector, tmp_uivalue);
    }
    while (ptDBoper->MoveNext() >= 0);

    for (index = 0; index < record_count; index++)
    {
        device_index = DeviceIndexVector[index];

        /* 获取物理设备信息 */
        pGBDeviceInfo = GBDevice_info_find_by_device_index(device_index);

        if (NULL == pGBDeviceInfo)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendRCUDeviceStatusToSubCMS() exit---: GBDevice Info Find Error:device_index=%u \r\n", device_index);
            continue;
        }

        if (EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendRCUDeviceStatusToSubCMS() exit---: GBDevice Type Error:Device ID=%s, device_type=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->device_type);
            continue;
        }

        if (pGBDeviceInfo->reg_status <= 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendRCUDeviceStatusToSubCMS() exit---: GBDevice reg_status Error:Device ID=%s, reg_status=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->reg_status);
            continue;
        }

        if (1 == pGBDeviceInfo->three_party_flag)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendRCUDeviceStatusToSubCMS() exit---: CMS Is Not Owner:Device ID=%s \r\n", pGBDeviceInfo->device_id);
            continue;
        }

        i |= SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送通知RCU点位状态变化Message消息到下级CMS失败:下级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑点位ID=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendRCUDeviceStatusToSubCMS Error:CMS ID=%s, IP=%s, port=%d, ID=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendRCUDeviceStatusToSubCMS() SIP_SendMessage Error \r\n");
            continue;
        }
        else if (i == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送通知RCU点位状态变化Message消息到下级CMS成功:下级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑点位ID=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendRCUDeviceStatusToSubCMS OK:CMS ID=%s, IP=%s, port=%d, ID=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id);
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendRCUDeviceStatusToSubCMS() SIP_SendMessage OK \r\n");
        }
    }

    return 1;
}

/*****************************************************************************
 函 数 名  : SendRCUDeviceStatusToRouteCMS
 功能描述  : 发送设备状态给所有上级CMS
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月21日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendRCUDeviceStatusToRouteCMS(GBLogicDevice_info_t* pGBLogicDeviceInfo, DBOper* pDboper)
{
    int i = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;

    char strGuard[32] = {0};
    char strAlarmPriority[32] = {0};
    char strStatus[32] = {0};

    if (NULL == pGBLogicDeviceInfo)
    {
        return -1;
    }

    /* 组建消息 */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"RCUDeviceStatus");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"132");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

    /*状态*/
    AccNode = outPacket.CreateElement((char*)"Status");
    snprintf(strStatus, 32, "%u", pGBLogicDeviceInfo->status);
    outPacket.SetElementValue(AccNode, strStatus);

    /*RCU上报的报警级别*/
    AccNode = outPacket.CreateElement((char*)"AlarmPriority");
    snprintf(strAlarmPriority, 32, "%u", pGBLogicDeviceInfo->AlarmPriority);
    outPacket.SetElementValue(AccNode, strAlarmPriority);

    /*RCU上报的Guard */
    AccNode = outPacket.CreateElement((char*)"Guard");
    snprintf(strGuard, 32, "%u", pGBLogicDeviceInfo->guard_type);
    outPacket.SetElementValue(AccNode, strGuard);

    /* RCU上报的Value */
    AccNode = outPacket.CreateElement((char*)"Value");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->Value);

    /* RCU上报的Unit */
    AccNode = outPacket.CreateElement((char*)"Unit");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->Unit);

    i = SendMessageToOwnerRouteCMSExceptMMS2(pGBLogicDeviceInfo->id, NULL, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length(), pDboper);

    if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送RCU设备状态变化Message消息到本地上级CMS失败");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Sent RCU device status change message  to the local superior CMS failure");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendRCUDeviceStatusToRouteCMS() SendMessageToOwnerRouteCMSExceptMMS2 Error \r\n");
        return -1;
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送RCU设备状态变化Message消息到本地上级CMS成功");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Sent RCU device status change message  to the local superior CMS success");
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendRCUDeviceStatusToRouteCMS() SendMessageToOwnerRouteCMSExceptMMS2 OK \r\n");
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendAllGBLogicDeviceStatusProc
 功能描述  : 发送所有逻辑设备状态信息给在线客户端以及上级CMS
 输入参数  : int device_index
             int status
             int enable
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月30日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
int SendAllGBLogicDeviceStatusProc(int device_index, int status, int enable, DBOper* ptDBoper)
{
    int iRet = 0;
    GBLogicDevice_Info_Iterator Itr;
    vector<string> DeviceIDVector;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (device_index <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendAllGBLogicDeviceStatusProc() exit---: Param Error \r\n");
        return -1;
    }

    DeviceIDVector.clear();

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->phy_mediaDeviceIndex <= 0))
        {
            continue;
        }

        if (pGBLogicDeviceInfo->phy_mediaDeviceIndex == device_index)
        {
            DeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    if (DeviceIDVector.size() > 0)
    {
        /* 发送所有设备状态消息给在线用户  */
        iRet = SendAllDeviceStatusToAllClientUser(DeviceIDVector, status);
        printf("SendAllGBLogicDeviceStatusProc() SendAllDeviceStatusToAllClientUser() Exit--- \r\n");

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "SendAllGBLogicDeviceStatusProc() SendAllDeviceStatusToAllClientUser Error:iRet=%d \r\n", iRet);
        }
        else if (iRet > 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "SendAllGBLogicDeviceStatusProc() SendAllDeviceStatusToAllClientUser OK:iRet=%d \r\n", iRet);
        }

        if (0 == enable)
        {
            /* 发送所有删除消息给上级CMS  */
            iRet = SendAllDeviceCatalogToRouteCMS(DeviceIDVector, 1, ptDBoper);
            printf("SendAllGBLogicDeviceStatusProc() SendAllDeviceCatalogToRouteCMS() Exit--- \r\n");

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "SendAllGBLogicDeviceStatusProc() SendAllDeviceCatalogToRouteCMS Error:iRet=%d \r\n", iRet);
            }
            else if (iRet > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "SendAllGBLogicDeviceStatusProc() SendAllDeviceCatalogToRouteCMS OK:iRet=%d \r\n", iRet);
            }

            /* 发送所有删除消息给下级CMS  */
            iRet = SendAllNotifyCatalogToSubCMS(DeviceIDVector, 1, ptDBoper);
            printf("SendAllGBLogicDeviceStatusProc() SendAllNotifyCatalogToSubCMS() Exit--- \r\n");

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "SendAllGBLogicDeviceStatusProc() SendAllNotifyCatalogToSubCMS Error:iRet=%d \r\n", iRet);
            }
            else if (iRet > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "SendAllGBLogicDeviceStatusProc() SendAllNotifyCatalogToSubCMS OK:iRet=%d \r\n", iRet);
            }
        }
        else
        {
            /* 发送所有设备状态消息给上级CMS  */
            iRet = SendAllDeviceStatusToRouteCMS(DeviceIDVector, status);
            printf("SendAllGBLogicDeviceStatusProc() SendAllDeviceStatusToRouteCMS() Exit--- \r\n");

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "SendAllGBLogicDeviceStatusProc() SendAllDeviceStatusToRouteCMS Error:iRet=%d \r\n", iRet);
            }
            else if (iRet > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "SendAllGBLogicDeviceStatusProc() SendAllDeviceStatusToRouteCMS OK:iRet=%d \r\n", iRet);
            }

            /* 发送所有设备状态消息给下级CMS  */
            iRet = SendAllDeviceStatusToSubCMS(DeviceIDVector, status, ptDBoper);
            printf("SendAllGBLogicDeviceStatusProc() SendAllDeviceStatusToSubCMS() Exit--- \r\n");

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendAllGBLogicDeviceStatusProc() SendAllDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, status, iRet);
            }
            else if (iRet > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendAllGBLogicDeviceStatusProc() SendAllDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, status, iRet);
            }
        }
    }

    DeviceIDVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : SendAllIntelligentGBLogicDeviceStatusProc
 功能描述  : 发送所有智能分析逻辑设备点位状态给客户端
 输入参数  : int device_index
             int status
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月23日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendAllIntelligentGBLogicDeviceStatusProc(int device_index, int status, DBOper* ptDBoper)
{
    int iRet = 0;
    int index = -1;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    vector<string> DeviceIDVector;
    GBDevice_info_t* pGBDeviceInfo = NULL;

    if (device_index <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendAllIntelligentGBLogicDeviceStatusProc() exit---: Param Error \r\n");
        return -1;
    }

    DeviceIDVector.clear();

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->phy_mediaDeviceIndex <= 0))
        {
            continue;
        }

        pGBDeviceInfo = GBDevice_info_get_by_stream_type2(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_INTELLIGENCE);

        if (NULL != pGBDeviceInfo)
        {
            if (pGBDeviceInfo->id == device_index)
            {
                DeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
            }
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    if (DeviceIDVector.size() > 0)
    {
        if (status == 1)
        {
            for (index = 0; index < (int)DeviceIDVector.size(); index++)
            {
                /* 发送设备状态变化消息  */
                iRet = SendDeviceStatusMessageProc(pGBLogicDeviceInfo, 4, ptDBoper);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendAllIntelligentGBLogicDeviceStatusProc() SendDeviceStatusMessageProc ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendAllIntelligentGBLogicDeviceStatusProc() SendDeviceStatusMessageProc OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, iRet);
                }
            }
        }
        else
        {
            for (index = 0; index < (int)DeviceIDVector.size(); index++)
            {
                pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

                if (NULL != pGBLogicDeviceInfo)
                {
                    /* 发送设备状态变化消息  */
                    iRet = SendDeviceStatusMessageProc(pGBLogicDeviceInfo, pGBLogicDeviceInfo->status, ptDBoper);

                    if (0 != iRet)
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendAllIntelligentGBLogicDeviceStatusProc() SendDeviceStatusMessageProc ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendAllIntelligentGBLogicDeviceStatusProc() SendDeviceStatusMessageProc OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, iRet);
                    }
                }

            }
        }
    }

    DeviceIDVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : SendDeviceStatusToSubCMS
 功能描述  : 发送设备状态到下级CMS
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int status
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月12日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendDeviceStatusToSubCMS(GBLogicDevice_info_t* pGBLogicDeviceInfo, int status, DBOper* ptDBoper)
{
    int i = 0;
    CPacket outPacket1;
    DOMElement* AccNode1 = NULL;
    string strStatus1 = "";

    int record_count = 0;
    string strSQL = "";
    char strGBLogicDeviceIndex[32] = {0};
    int device_index = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    vector<unsigned int> DeviceIndexVector;
    int while_count = 0;
    int index = 0;

    if (NULL == pGBLogicDeviceInfo || NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendDeviceStatusToSubCMS() exit---: Param Error \r\n");
        return -1;
    }

    /* 获取该点推送的下级CMS设备 */
    strSQL.clear();
    strSQL = "select GBPhyDeviceIndex from GBPhyDevicePermConfig WHERE DeviceIndex = "; /* 查询权限表，获取到对应的物理设备 */
    snprintf(strGBLogicDeviceIndex, 32, "%u", pGBLogicDeviceInfo->id);
    strSQL += strGBLogicDeviceIndex;

    record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendDeviceStatusToSubCMS() record_count=%d, DeviceIndex=%d \r\n", record_count, device_index);

    if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendDeviceStatusToSubCMS() exit---: No Record Count \r\n");
        return 0;
    }
    else if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceStatusToSubCMS() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceStatusToSubCMS() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return -1;
    }

    if (0 == status)
    {
        strStatus1 = "OFF";
    }
    else if (1 == status)
    {
        strStatus1 = "ON";
    }
    else if (2 == status)
    {
        strStatus1 = "NOVIDEO";
    }
    else if (4 == status)
    {
        strStatus1 = "INTELLIGENT";
    }
    else if (5 == status)
    {
        strStatus1 = "CLOSE";
    }
    else if (6 == status)
    {
        strStatus1 = "APART";
    }
    else if (7 == status)
    {
        strStatus1 = "ALARM";
    }

    /* 回复响应,组建消息 */
    outPacket1.SetRootTag("Notify");
    AccNode1 = outPacket1.CreateElement((char*)"CmdType");
    outPacket1.SetElementValue(AccNode1, (char*)"DeviceStatus");

    AccNode1 = outPacket1.CreateElement((char*)"SN");
    outPacket1.SetElementValue(AccNode1, (char*)"132");

    AccNode1 = outPacket1.CreateElement((char*)"DeviceID");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->device_id);

    AccNode1 = outPacket1.CreateElement((char*)"Status");
    outPacket1.SetElementValue(AccNode1, (char*)strStatus1.c_str());

    /* 循环读取数据库 */
    do
    {
        unsigned int tmp_uivalue = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "SendDeviceStatusToSubCMS() While Count=%d \r\n", while_count);
        }

        ptDBoper->GetFieldValue(0, tmp_uivalue);

        if (tmp_uivalue <= 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceStatusToSubCMS() DeviceIndex Error \r\n");
            continue;
        }

        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "AddAllGBLogicDeviceIDToVector() DeviceIndex=%u \r\n", tmp_uivalue);

        i = AddDeviceIndexToDeviceIndexVector(DeviceIndexVector, tmp_uivalue);
    }
    while (ptDBoper->MoveNext() >= 0);

    for (index = 0; index < record_count; index++)
    {
        device_index = DeviceIndexVector[index];

        /* 获取物理设备信息 */
        pGBDeviceInfo = GBDevice_info_find_by_device_index(device_index);

        if (NULL == pGBDeviceInfo)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceStatusToSubCMS() exit---: GBDevice Info Find Error:device_index=%u \r\n", device_index);
            continue;
        }

        if (EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceStatusToSubCMS() exit---: GBDevice Type Error:Device ID=%s, device_type=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->device_type);
            continue;
        }

        if (pGBDeviceInfo->reg_status <= 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceStatusToSubCMS() exit---: GBDevice reg_status Error:Device ID=%s, reg_status=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->reg_status);
            continue;
        }

        if (1 == pGBDeviceInfo->three_party_flag)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceStatusToSubCMS() exit---: CMS Is Not Owner:Device ID=%s \r\n", pGBDeviceInfo->device_id);
            continue;
        }

        i |= SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket1.GetXml(NULL).c_str(), outPacket1.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送通知点位状态变化Message消息到下级CMS失败:下级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑点位ID=%s, 状态=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strStatus1.c_str());
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendDeviceStatusToSubCMS Error:CMS ID=%s, IP=%s, port=%d, ID=%s, status=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strStatus1.c_str());
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceStatusToSubCMS() SIP_SendMessage Error \r\n");
            continue;
        }
        else if (i == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送通知点位状态变化Message消息到下级CMS成功:下级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑点位ID=%s, 状态=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strStatus1.c_str());
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendDeviceStatusToSubCMS OK:CMS ID=%s, IP=%s, port=%d, ID=%s, status=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strStatus1.c_str());
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendDeviceStatusToSubCMS() SIP_SendMessage OK \r\n");
        }
    }

    return 1;
}

/*****************************************************************************
 函 数 名  : SendAllDeviceStatusToSubCMS
 功能描述  : 发送所有设备状态到下级CMS
 输入参数  : vector<string>& DeviceIDVector
             int status
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月22日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendAllDeviceStatusToSubCMS(vector<string>& DeviceIDVector, int status, DBOper* ptDBoper)
{
    int i = 0;
    int index = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    for (index = 0; index < (int)DeviceIDVector.size(); index++)
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

        if (NULL == pGBLogicDeviceInfo)
        {
            continue;
        }

        i = SendDeviceStatusToSubCMS(pGBLogicDeviceInfo, status, ptDBoper);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SendDeviceStatusMessageProc
 功能描述  : 发送设备状态变化消息处理
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int status
             DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月8日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendDeviceStatusMessageProc(GBLogicDevice_info_t* pGBLogicDeviceInfo, int status, DBOper* pDboper)
{
    int i = 0;

    if (NULL == pGBLogicDeviceInfo || NULL == pDboper)
    {
        return -1;
    }

    /* 发送设备状态消息给客户端 */
    i = SendDeviceStatusToAllClientUser(pGBLogicDeviceInfo->device_id, status, pDboper);

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "SendDeviceStatusMessageProc() SendDeviceStatusToAllClientUser Error:i=%d \r\n", i);
    }
    else if (i > 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "SendDeviceStatusMessageProc() SendDeviceStatusToAllClientUser OK:i=%d \r\n", i);
    }

    /* 发送设备状态消息给上级CMS  */
    i = SendDeviceStatusToRouteCMS(pGBLogicDeviceInfo->id, pGBLogicDeviceInfo->device_id, status, pDboper);

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceStatusMessageProc() SendDeviceStatusToRouteCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
    }
    else if (i > 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendDeviceStatusMessageProc() SendDeviceStatusToRouteCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
    }

    /* 发送设备状态消息给下级CMS  */
    i = SendDeviceStatusToSubCMS(pGBLogicDeviceInfo, status, pDboper);

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceStatusMessageProc() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
    }
    else if (i > 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendDeviceStatusMessageProc() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SendNotifyGroupMapCatalogTo3PartyRouteCMS
 功能描述  : 发送点位分组关系变化消息到第三方平台
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int iEvent
             DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendNotifyGroupMapCatalogTo3PartyRouteCMS(GBLogicDevice_info_t* pGBLogicDeviceInfo, int iEvent, DBOper* pDboper)
{
    int i = 0;
    string strEvent = "";
    CPacket outPacket2;
    DOMElement* AccNode2 = NULL;
    DOMElement* ItemAccNode2 = NULL;
    DOMElement* ItemInfoNode2 = NULL;

    char strParental[16] = {0};
    char strSafetyWay[16] = {0};
    char strRegisterWay[16] = {0};
    char strCertifiable[16] = {0};
    char strErrCode[16] = {0};
    char strSecrecy[16] = {0};
    char strPort[16] = {0};
    char strLongitude[64] = {0};
    char strLatitude[64] = {0};
    char strPTZType[16] = {0};

    if (NULL == pGBLogicDeviceInfo)
    {
        return -1;
    }

    if (0 == iEvent)
    {
        strEvent = "ADD";
    }
    else if (1 == iEvent)
    {
        strEvent = "DEL";
    }
    else if (2 == iEvent)
    {
        strEvent = "UPDATE";
    }
    else
    {
        return -1;
    }

    /* 发送给第三方平台 */
    outPacket2.SetRootTag("Notify");
    AccNode2 = outPacket2.CreateElement((char*)"CmdType");
    outPacket2.SetElementValue(AccNode2, (char*)"Catalog");

    AccNode2 = outPacket2.CreateElement((char*)"SN");
    outPacket2.SetElementValue(AccNode2, (char*)"267");

    AccNode2 = outPacket2.CreateElement((char*)"DeviceID");
    outPacket2.SetElementValue(AccNode2, local_cms_id_get());

    AccNode2 = outPacket2.CreateElement((char*)"SumNum");
    outPacket2.SetElementValue(AccNode2, (char*)"1");

    AccNode2 = outPacket2.CreateElement((char*)"DeviceList");
    outPacket2.SetElementAttr(AccNode2, (char*)"Num", (char*)"1");

    outPacket2.SetCurrentElement(AccNode2);
    ItemAccNode2 = outPacket2.CreateElement((char*)"Item");
    outPacket2.SetCurrentElement(ItemAccNode2);

    AccNode2 = outPacket2.CreateElement((char*)"Event");
    outPacket2.SetElementValue(AccNode2, (char*)strEvent.c_str());

    /* 设备统一编号 */
    AccNode2 = outPacket2.CreateElement((char*)"DeviceID");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->device_id);

    /* 点位名称 */
    AccNode2 = outPacket2.CreateElement((char*)"Name");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->device_name);

    /* 设备生产商 */
    AccNode2 = outPacket2.CreateElement((char*)"Manufacturer");

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->manufacturer);
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, (char*)"");
    }

    /* 设备型号 */
    AccNode2 = outPacket2.CreateElement((char*)"Model");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->model);

    /* 设备归属 */
    AccNode2 = outPacket2.CreateElement((char*)"Owner");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->owner);

    /* 行政区域 */
    AccNode2 = outPacket2.CreateElement((char*)"CivilCode");

    if ('\0' == pGBLogicDeviceInfo->civil_code[0])
    {
        outPacket2.SetElementValue(AccNode2, local_civil_code_get());
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->civil_code);
    }

    /* 警区 */
    AccNode2 = outPacket2.CreateElement((char*)"Block");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->block);

    /* 安装地址 */
    AccNode2 = outPacket2.CreateElement((char*)"Address");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->address);

    /* 是否有子设备 */
    AccNode2 = outPacket2.CreateElement((char*)"Parental");
    snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
    outPacket2.SetElementValue(AccNode2, strParental);

    /* 父设备/区域/系统ID, 和其他平台对接的时候，统一使用本级CMS ID */
    AccNode2 = outPacket2.CreateElement((char*)"ParentID");

    if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
    {
        outPacket2.SetElementValue(AccNode2, local_cms_id_get());
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->virtualParentID);
    }

    /* 信令安全模式*/
    AccNode2 = outPacket2.CreateElement((char*)"SafetyWay");
    snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
    outPacket2.SetElementValue(AccNode2, strSafetyWay);

    /* 注册方式 */
    AccNode2 = outPacket2.CreateElement((char*)"RegisterWay");
    snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
    outPacket2.SetElementValue(AccNode2, strRegisterWay);

    /* 证书序列号*/
    AccNode2 = outPacket2.CreateElement((char*)"CertNum");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->cert_num);

    /* 证书有效标识 */
    AccNode2 = outPacket2.CreateElement((char*)"Certifiable");
    snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
    outPacket2.SetElementValue(AccNode2, strCertifiable);

    /* 无效原因码 */
    AccNode2 = outPacket2.CreateElement((char*)"ErrCode");
    snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
    outPacket2.SetElementValue(AccNode2, strErrCode);

    /* 证书终止有效期*/
    AccNode2 = outPacket2.CreateElement((char*)"EndTime");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->end_time);

    /* 保密属性 */
    AccNode2 = outPacket2.CreateElement((char*)"Secrecy");
    snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
    outPacket2.SetElementValue(AccNode2, strSecrecy);

    /* IP地址*/
    AccNode2 = outPacket2.CreateElement((char*)"IPAddress");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->ip_address);

    /* 端口号 */
    AccNode2 = outPacket2.CreateElement((char*)"Port");
    snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
    outPacket2.SetElementValue(AccNode2, strPort);

    /* 密码*/
    AccNode2 = outPacket2.CreateElement((char*)"Password");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->password);

    /* 点位状态 */
    AccNode2 = outPacket2.CreateElement((char*)"Status");

    if (1 == pGBLogicDeviceInfo->status)
    {
        outPacket2.SetElementValue(AccNode2, (char*)"ON");
    }
    else if (2 == pGBLogicDeviceInfo->status)
    {
        outPacket2.SetElementValue(AccNode2, (char*)"VLOST");
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, (char*)"OFF");
    }

    /* 经度 */
    AccNode2 = outPacket2.CreateElement((char*)"Longitude");
    snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
    outPacket2.SetElementValue(AccNode2, strLongitude);

    /* 纬度 */
    AccNode2 = outPacket2.CreateElement((char*)"Latitude");
    snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
    outPacket2.SetElementValue(AccNode2, strLatitude);

    /* 扩展的Info字段 */
    outPacket2.SetCurrentElement(ItemAccNode2);
    ItemInfoNode2 = outPacket2.CreateElement((char*)"Info");
    outPacket2.SetCurrentElement(ItemInfoNode2);

    /* 是否可控 */
    AccNode2 = outPacket2.CreateElement((char*)"PTZType");

    if (pGBLogicDeviceInfo->ctrl_enable <= 0)
    {
        snprintf(strPTZType, 16, "%u", 3);
    }
    else
    {
        snprintf(strPTZType, 16, "%u", pGBLogicDeviceInfo->ctrl_enable);
    }

    outPacket2.SetElementValue(AccNode2, strPTZType);

    i = SendNotifyTo3PartyRouteCMS2(pGBLogicDeviceInfo->id, NULL, (char*)outPacket2.GetXml(NULL).c_str(), outPacket2.GetXml(NULL).length(), pDboper);

    if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送目录分组关系变化Notify消息到第三方上级CMS失败");
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyGroupMapCatalogTo3PartyRouteCMS() SendNotifyTo3PartyRouteCMS2 Error \r\n");
        return -1;
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送目录分组关系变化Notify消息到第三方上级CMS成功");
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendNotifyGroupMapCatalogTo3PartyRouteCMS() SendNotifyTo3PartyRouteCMS2 OK \r\n");
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendNotifyCatalogToSubCMS
 功能描述  : 发送点位变化消息给下级CMS平台
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int iEvent
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月12日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendNotifyCatalogToSubCMS(GBLogicDevice_info_t* pGBLogicDeviceInfo, int iEvent, DBOper* ptDBoper)
{
    int i = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    string strEvent = "";
    DOMElement* ItemAccNode = NULL;
    DOMElement* ItemInfoNode = NULL;

    int record_count = 0;
    string strSQL = "";
    char strGBLogicDeviceIndex[32] = {0};
    int device_index = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    vector<unsigned int> DeviceIndexVector;
    int while_count = 0;
    int index = 0;

    char strID[64] = {0};
    char strParental[16] = {0};
    char strSafetyWay[16] = {0};
    char strRegisterWay[16] = {0};
    char strCertifiable[16] = {0};
    char strErrCode[16] = {0};
    char strSecrecy[16] = {0};
    char strPort[16] = {0};
    char strLongitude[64] = {0};
    char strLatitude[64] = {0};
    char strAlarmDeviceSubType[64] = {0};
    char strStreamCount[16] = {0};
    char strFrameCount[16] = {0};
    char strAlarmLengthOfTime[16] = {0};
    char strAlarmPriority[32] = {0};/*2016.10.10 add for RCU*/
    char strGuard[32] = {0};/*2016.10.10 add for RCU*/
    char strPTZType[16] = {0};

    if (NULL == pGBLogicDeviceInfo || NULL == ptDBoper)
    {
        return -1;
    }

    /* 获取该点推送的下级CMS设备 */
    strSQL.clear();
    strSQL = "select GBPhyDeviceIndex from GBPhyDevicePermConfig WHERE DeviceIndex = "; /* 查询权限表，获取到对应的物理设备 */
    snprintf(strGBLogicDeviceIndex, 32, "%u", pGBLogicDeviceInfo->id);
    strSQL += strGBLogicDeviceIndex;

    record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendNotifyCatalogToSubCMS() record_count=%d, DeviceIndex=%d \r\n", record_count, pGBLogicDeviceInfo->id);

    if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendNotifyCatalogToSubCMS() exit---: No Record Count \r\n");
        return 0;
    }
    else if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogToSubCMS() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogToSubCMS() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return -1;
    }

    if (0 == iEvent)
    {
        strEvent = "ADD";
    }
    else if (1 == iEvent)
    {
        strEvent = "DEL";
    }
    else if (2 == iEvent)
    {
        strEvent = "UPDATE";
    }
    else
    {
        return -1;
    }

    /* 回复响应,组建消息 */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"Catalog");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"154");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, local_cms_id_get());

    AccNode = outPacket.CreateElement((char*)"SumNum");
    outPacket.SetElementValue(AccNode, (char*)"1");

    AccNode = outPacket.CreateElement((char*)"DeviceList");
    outPacket.SetElementAttr(AccNode, (char*)"Num", (char*)"1");

    outPacket.SetCurrentElement(AccNode);
    ItemAccNode = outPacket.CreateElement((char*)"Item");
    outPacket.SetCurrentElement(ItemAccNode);

    AccNode = outPacket.CreateElement((char*)"Event");
    outPacket.SetElementValue(AccNode, (char*)strEvent.c_str());

    /*RCU上报的报警级别*/
    AccNode = outPacket.CreateElement((char*)"AlarmPriority");
    snprintf(strAlarmPriority, 32, "%d", pGBLogicDeviceInfo->AlarmPriority);
    outPacket.SetElementValue(AccNode, strAlarmPriority);

    /*RCU上报的Guard*/
    AccNode = outPacket.CreateElement((char*)"Guard");
    snprintf(strGuard, 32, "%d", pGBLogicDeviceInfo->guard_type);
    outPacket.SetElementValue(AccNode, strGuard);

    /* RCU上报的Value */
    AccNode = outPacket.CreateElement((char*)"Value");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->Value);

    /* RCU上报的Unit */
    AccNode = outPacket.CreateElement((char*)"Unit");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->Unit);

    /* 设备索引 */
    AccNode = outPacket.CreateElement((char*)"ID");
    snprintf(strID, 64, "%u", pGBLogicDeviceInfo->id);
    outPacket.SetElementValue(AccNode, strID);

    /* 设备统一编号 */
    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

    /* 点位名称 */
    AccNode = outPacket.CreateElement((char*)"Name");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

    /* 是否启用*/
    AccNode = outPacket.CreateElement((char*)"Enable");

    if (0 == pGBLogicDeviceInfo->enable)
    {
        outPacket.SetElementValue(AccNode, (char*)"0");
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"1");
    }

    /* 是否可控 */
    AccNode = outPacket.CreateElement((char*)"CtrlEnable");

    if (1 == pGBLogicDeviceInfo->ctrl_enable)
    {
        outPacket.SetElementValue(AccNode, (char*)"Enable");
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"Disable");
    }

    /* 是否支持对讲 */
    AccNode = outPacket.CreateElement((char*)"MicEnable");

    if (0 == pGBLogicDeviceInfo->mic_enable)
    {
        outPacket.SetElementValue(AccNode, (char*)"Disable");
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"Enable");
    }

    /* 帧率 */
    AccNode = outPacket.CreateElement((char*)"FrameCount");
    snprintf(strFrameCount, 16, "%d", pGBLogicDeviceInfo->frame_count);
    outPacket.SetElementValue(AccNode, strFrameCount);

    /* 是否支持多码流 */
    AccNode = outPacket.CreateElement((char*)"StreamCount");
    snprintf(strStreamCount, 16, "%d", pGBLogicDeviceInfo->stream_count);
    outPacket.SetElementValue(AccNode, strStreamCount);

    /* 告警时长 */
    AccNode = outPacket.CreateElement((char*)"AlarmLengthOfTime");
    snprintf(strAlarmLengthOfTime, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
    outPacket.SetElementValue(AccNode, strAlarmLengthOfTime);

    /* 设备生产商 */
    AccNode = outPacket.CreateElement((char*)"Manufacturer");

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->manufacturer);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    /* 设备型号 */
    AccNode = outPacket.CreateElement((char*)"Model");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->model);

    /* 设备归属 */
    AccNode = outPacket.CreateElement((char*)"Owner");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->owner);

    /* 行政区域 */
    AccNode = outPacket.CreateElement((char*)"CivilCode");

    if ('\0' == pGBLogicDeviceInfo->civil_code[0])
    {
        outPacket.SetElementValue(AccNode, local_civil_code_get());
    }
    else
    {
        outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->civil_code);
    }

    /* 警区 */
    AccNode = outPacket.CreateElement((char*)"Block");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->block);

    /* 安装地址 */
    AccNode = outPacket.CreateElement((char*)"Address");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->address);

    /* 是否有子设备 */
    AccNode = outPacket.CreateElement((char*)"Parental");
    snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
    outPacket.SetElementValue(AccNode, strParental);

    /* 父设备/区域/系统ID, 和其他平台对接的时候，统一使用本级CMS ID */
    AccNode = outPacket.CreateElement((char*)"ParentID");

    if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
    {
        outPacket.SetElementValue(AccNode, local_cms_id_get());
    }
    else
    {
        outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->virtualParentID);
    }

    /* 信令安全模式*/
    AccNode = outPacket.CreateElement((char*)"SafetyWay");
    snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
    outPacket.SetElementValue(AccNode, strSafetyWay);

    /* 注册方式 */
    AccNode = outPacket.CreateElement((char*)"RegisterWay");
    snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
    outPacket.SetElementValue(AccNode, strRegisterWay);

    /* 证书序列号*/
    AccNode = outPacket.CreateElement((char*)"CertNum");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->cert_num);

    /* 证书有效标识 */
    AccNode = outPacket.CreateElement((char*)"Certifiable");
    snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
    outPacket.SetElementValue(AccNode, strCertifiable);

    /* 无效原因码 */
    AccNode = outPacket.CreateElement((char*)"ErrCode");
    snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
    outPacket.SetElementValue(AccNode, strErrCode);

    /* 证书终止有效期*/
    AccNode = outPacket.CreateElement((char*)"EndTime");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->end_time);

    /* 保密属性 */
    AccNode = outPacket.CreateElement((char*)"Secrecy");
    snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
    outPacket.SetElementValue(AccNode, strSecrecy);

    /* IP地址*/
    AccNode = outPacket.CreateElement((char*)"IPAddress");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->ip_address);

    /* 端口号 */
    AccNode = outPacket.CreateElement((char*)"Port");
    snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
    outPacket.SetElementValue(AccNode, strPort);

    /* 密码*/
    AccNode = outPacket.CreateElement((char*)"Password");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->password);

    /* 点位状态 */
    AccNode = outPacket.CreateElement((char*)"Status");

    if (1 == pGBLogicDeviceInfo->status)
    {
        if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
        {
            outPacket.SetElementValue(AccNode, (char*)"INTELLIGENT");
        }
        else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
        {
            outPacket.SetElementValue(AccNode, (char*)"CLOSE");
        }
        else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
        {
            outPacket.SetElementValue(AccNode, (char*)"APART");
        }
        else
        {
            outPacket.SetElementValue(AccNode, (char*)"ON");
        }
    }
    else if (2 == pGBLogicDeviceInfo->status)
    {
        outPacket.SetElementValue(AccNode, (char*)"NOVIDEO");
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"OFF");
    }

    /* 经度 */
    AccNode = outPacket.CreateElement((char*)"Longitude");
    snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
    outPacket.SetElementValue(AccNode, strLongitude);

    /* 纬度 */
    AccNode = outPacket.CreateElement((char*)"Latitude");
    snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
    outPacket.SetElementValue(AccNode, strLatitude);

    /* 所属图层 */
    AccNode = outPacket.CreateElement((char*)"MapLayer");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->map_layer);

    /* 报警设备子类型 */
    AccNode = outPacket.CreateElement((char*)"ChlType");
    snprintf(strAlarmDeviceSubType, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
    outPacket.SetElementValue(AccNode, strAlarmDeviceSubType);

    /* 所属的CMS ID */
    AccNode = outPacket.CreateElement((char*)"CMSID");
    outPacket.SetElementValue(AccNode, local_cms_id_get());

    /* 扩展的Info字段 */
    outPacket.SetCurrentElement(ItemAccNode);
    ItemInfoNode = outPacket.CreateElement((char*)"Info");
    outPacket.SetCurrentElement(ItemInfoNode);

    /* 是否可控 */
    AccNode = outPacket.CreateElement((char*)"PTZType");

    if (pGBLogicDeviceInfo->ctrl_enable <= 0)
    {
        snprintf(strPTZType, 16, "%u", 3);
    }
    else
    {
        snprintf(strPTZType, 16, "%u", pGBLogicDeviceInfo->ctrl_enable);
    }

    outPacket.SetElementValue(AccNode, strPTZType);

    /* 循环读取数据库 */
    do
    {
        unsigned int tmp_uivalue = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "SendNotifyCatalogToSubCMS() While Count=%d \r\n", while_count);
        }

        ptDBoper->GetFieldValue(0, tmp_uivalue);

        if (tmp_uivalue <= 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogToSubCMS() DeviceIndex Error \r\n");
            continue;
        }

        i = AddDeviceIndexToDeviceIndexVector(DeviceIndexVector, tmp_uivalue);
    }
    while (ptDBoper->MoveNext() >= 0);

    for (index = 0; index < record_count; index++)
    {
        device_index = DeviceIndexVector[index];

        /* 获取物理设备信息 */
        pGBDeviceInfo = GBDevice_info_find_by_device_index(device_index);

        if (NULL == pGBDeviceInfo)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogToSubCMS() exit---: GBDevice Info Find Error:device_index=%u \r\n", device_index);
            continue;
        }

        if (EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogToSubCMS() exit---: GBDevice Type Error:Device ID=%s, device_type=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->device_type);
            continue;
        }

        if (pGBDeviceInfo->reg_status <= 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogToSubCMS() exit---: GBDevice reg_status Error:Device ID=%s, reg_status=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->reg_status);
            continue;
        }

        if (1 == pGBDeviceInfo->three_party_flag)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogToSubCMS() exit---: CMS Is Not Owner:Device ID=%s \r\n", pGBDeviceInfo->device_id);
            continue;
        }

        i |= SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送通知目录变化Message消息到下级CMS失败:下级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑点位ID=%s, 事件=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strEvent.c_str());
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendNotifyCatalogToSubCMS Error:CMS ID=%s, IP=%s, port=%d, ID=%s, event=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strEvent.c_str());
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogToSubCMS() SIP_SendMessage Error \r\n");
            continue;
        }
        else if (i == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送通知目录变化Message消息到下级CMS成功:下级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑点位ID=%s, 事件=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strEvent.c_str());
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyCatalogToSubCMS OK:CMS ID=%s, IP=%s, port=%d, ID=%s, event=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strEvent.c_str());
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendNotifyCatalogToSubCMS() SIP_SendMessage OK \r\n");
        }
    }

    return 1;
}

/*****************************************************************************
 函 数 名  : SendAllNotifyCatalogToSubCMS
 功能描述  : 发送所有点位Catalog到下级平台
 输入参数  : vector<string>& DeviceIDVector
             int iEvent
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月22日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendAllNotifyCatalogToSubCMS(vector<string>& DeviceIDVector, int iEvent, DBOper* ptDBoper)
{
    int i = 0;
    int index = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    for (index = 0; index < (int)DeviceIDVector.size(); index++)
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

        if (NULL == pGBLogicDeviceInfo)
        {
            continue;
        }

        i = SendNotifyCatalogToSubCMS(pGBLogicDeviceInfo, iEvent, ptDBoper);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SendNotifyCatalogToRouteCMS
 功能描述  : 发送目录变化消息给上级平台
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int iEvent
             DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月12日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendNotifyCatalogToRouteCMS(GBLogicDevice_info_t* pGBLogicDeviceInfo, int iEvent, DBOper* pDboper)
{
    int i = 0;
    string strEvent = "";
    CPacket outPacket1;
    DOMElement* AccNode1 = NULL;
    DOMElement* ItemAccNode1 = NULL;
    DOMElement* ItemInfoNode1 = NULL;

    CPacket outPacket2;
    DOMElement* AccNode2 = NULL;
    DOMElement* ItemAccNode2 = NULL;
    DOMElement* ItemInfoNode2 = NULL;

    char strID[64] = {0};
    char strParental[16] = {0};
    char strSafetyWay[16] = {0};
    char strRegisterWay[16] = {0};
    char strCertifiable[16] = {0};
    char strErrCode[16] = {0};
    char strSecrecy[16] = {0};
    char strPort[16] = {0};
    char strLongitude[64] = {0};
    char strLatitude[64] = {0};
    char strAlarmDeviceSubType[64] = {0};
    char strStreamCount[16] = {0};
    char strFrameCount[16] = {0};
    char strAlarmLengthOfTime[16] = {0};
    char strAlarmPriority[32] = {0};/*2016.10.10 add for RCU*/
    char strGuard[32] = {0};/*2016.10.10 add for RCU*/
    char strPTZType[16] = {0};

    if (NULL == pGBLogicDeviceInfo)
    {
        return -1;
    }

    if (0 == iEvent)
    {
        strEvent = "ADD";
    }
    else if (1 == iEvent)
    {
        strEvent = "DEL";
    }
    else if (2 == iEvent)
    {
        strEvent = "UPDATE";
    }
    else
    {
        return -1;
    }

    /* 回复响应,组建消息 */
    outPacket1.SetRootTag("Notify");
    AccNode1 = outPacket1.CreateElement((char*)"CmdType");
    outPacket1.SetElementValue(AccNode1, (char*)"Catalog");

    AccNode1 = outPacket1.CreateElement((char*)"SN");
    outPacket1.SetElementValue(AccNode1, (char*)"166");

    AccNode1 = outPacket1.CreateElement((char*)"DeviceID");
    outPacket1.SetElementValue(AccNode1, local_cms_id_get());

    AccNode1 = outPacket1.CreateElement((char*)"SumNum");
    outPacket1.SetElementValue(AccNode1, (char*)"1");

    AccNode1 = outPacket1.CreateElement((char*)"DeviceList");
    outPacket1.SetElementAttr(AccNode1, (char*)"Num", (char*)"1");

    outPacket1.SetCurrentElement(AccNode1);
    ItemAccNode1 = outPacket1.CreateElement((char*)"Item");
    outPacket1.SetCurrentElement(ItemAccNode1);

    AccNode1 = outPacket1.CreateElement((char*)"Event");
    outPacket1.SetElementValue(AccNode1, (char*)strEvent.c_str());

    /* 设备索引 */
    AccNode1 = outPacket1.CreateElement((char*)"ID");
    snprintf(strID, 64, "%u", pGBLogicDeviceInfo->id);
    outPacket1.SetElementValue(AccNode1, strID);

    /* 设备统一编号 */
    AccNode1 = outPacket1.CreateElement((char*)"DeviceID");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->device_id);

    /* 点位名称 */
    AccNode1 = outPacket1.CreateElement((char*)"Name");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->device_name);

    /* 是否启用*/
    AccNode1 = outPacket1.CreateElement((char*)"Enable");

    if (0 == pGBLogicDeviceInfo->enable)
    {
        outPacket1.SetElementValue(AccNode1, (char*)"0");
    }
    else
    {
        outPacket1.SetElementValue(AccNode1, (char*)"1");
    }

    /* 是否可控 */
    AccNode1 = outPacket1.CreateElement((char*)"CtrlEnable");

    if (1 == pGBLogicDeviceInfo->ctrl_enable)
    {
        outPacket1.SetElementValue(AccNode1, (char*)"Enable");
    }
    else
    {
        outPacket1.SetElementValue(AccNode1, (char*)"Disable");
    }

    /* 是否支持对讲 */
    AccNode1 = outPacket1.CreateElement((char*)"MicEnable");

    if (0 == pGBLogicDeviceInfo->mic_enable)
    {
        outPacket1.SetElementValue(AccNode1, (char*)"Disable");
    }
    else
    {
        outPacket1.SetElementValue(AccNode1, (char*)"Enable");
    }

    /* 帧率 */
    AccNode1 = outPacket1.CreateElement((char*)"FrameCount");
    snprintf(strFrameCount, 16, "%d", pGBLogicDeviceInfo->frame_count);
    outPacket1.SetElementValue(AccNode1, strFrameCount);

    /* 是否支持多码流 */
    AccNode1 = outPacket1.CreateElement((char*)"StreamCount");
    snprintf(strStreamCount, 16, "%d", pGBLogicDeviceInfo->stream_count);
    outPacket1.SetElementValue(AccNode1, strStreamCount);

    /* 告警时长 */
    AccNode1 = outPacket1.CreateElement((char*)"AlarmLengthOfTime");
    snprintf(strAlarmLengthOfTime, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
    outPacket1.SetElementValue(AccNode1, strAlarmLengthOfTime);

    /* 设备生产商 */
    AccNode1 = outPacket1.CreateElement((char*)"Manufacturer");

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->manufacturer);
    }
    else
    {
        outPacket1.SetElementValue(AccNode1, (char*)"");
    }

    /* 设备型号 */
    AccNode1 = outPacket1.CreateElement((char*)"Model");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->model);

    /* 设备归属 */
    AccNode1 = outPacket1.CreateElement((char*)"Owner");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->owner);

    /* 行政区域 */
    AccNode1 = outPacket1.CreateElement((char*)"CivilCode");

    if ('\0' == pGBLogicDeviceInfo->civil_code[0])
    {
        outPacket1.SetElementValue(AccNode1, local_civil_code_get());
    }
    else
    {
        outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->civil_code);
    }

    /* 警区 */
    AccNode1 = outPacket1.CreateElement((char*)"Block");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->block);

    /* 安装地址 */
    AccNode1 = outPacket1.CreateElement((char*)"Address");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->address);

    /* 是否有子设备 */
    AccNode1 = outPacket1.CreateElement((char*)"Parental");
    snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
    outPacket1.SetElementValue(AccNode1, strParental);

    /* 父设备/区域/系统ID, 和其他平台对接的时候，统一使用本级CMS ID */
    AccNode1 = outPacket1.CreateElement((char*)"ParentID");

    if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
    {
        outPacket1.SetElementValue(AccNode1, local_cms_id_get());
    }
    else
    {
        outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->virtualParentID);
    }

    /* 信令安全模式*/
    AccNode1 = outPacket1.CreateElement((char*)"SafetyWay");
    snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
    outPacket1.SetElementValue(AccNode1, strSafetyWay);

    /* 注册方式 */
    AccNode1 = outPacket1.CreateElement((char*)"RegisterWay");
    snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
    outPacket1.SetElementValue(AccNode1, strRegisterWay);

    /* 证书序列号*/
    AccNode1 = outPacket1.CreateElement((char*)"CertNum");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->cert_num);

    /* 证书有效标识 */
    AccNode1 = outPacket1.CreateElement((char*)"Certifiable");
    snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
    outPacket1.SetElementValue(AccNode1, strCertifiable);

    /* 无效原因码 */
    AccNode1 = outPacket1.CreateElement((char*)"ErrCode");
    snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
    outPacket1.SetElementValue(AccNode1, strErrCode);

    /* 证书终止有效期*/
    AccNode1 = outPacket1.CreateElement((char*)"EndTime");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->end_time);

    /* 保密属性 */
    AccNode1 = outPacket1.CreateElement((char*)"Secrecy");
    snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
    outPacket1.SetElementValue(AccNode1, strSecrecy);

    /* IP地址*/
    AccNode1 = outPacket1.CreateElement((char*)"IPAddress");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->ip_address);

    /* 端口号 */
    AccNode1 = outPacket1.CreateElement((char*)"Port");
    snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
    outPacket1.SetElementValue(AccNode1, strPort);

    /* 密码*/
    AccNode1 = outPacket1.CreateElement((char*)"Password");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->password);

    /* 点位状态 */
    AccNode1 = outPacket1.CreateElement((char*)"Status");

    if (1 == pGBLogicDeviceInfo->status)
    {
        if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
        {
            outPacket1.SetElementValue(AccNode1, (char*)"INTELLIGENT");
        }
        else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
        {
            outPacket1.SetElementValue(AccNode1, (char*)"CLOSE");
        }
        else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
        {
            outPacket1.SetElementValue(AccNode1, (char*)"APART");
        }
        else
        {
            outPacket1.SetElementValue(AccNode1, (char*)"ON");
        }
    }
    else if (2 == pGBLogicDeviceInfo->status)
    {
        outPacket1.SetElementValue(AccNode1, (char*)"NOVIDEO");
    }
    else
    {
        outPacket1.SetElementValue(AccNode1, (char*)"OFF");
    }

    /* 经度 */
    AccNode1 = outPacket1.CreateElement((char*)"Longitude");
    snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
    outPacket1.SetElementValue(AccNode1, strLongitude);

    /* 纬度 */
    AccNode1 = outPacket1.CreateElement((char*)"Latitude");
    snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
    outPacket1.SetElementValue(AccNode1, strLatitude);

    /* 所属图层 */
    AccNode1 = outPacket1.CreateElement((char*)"MapLayer");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->map_layer);

    /* 报警设备子类型 */
    AccNode1 = outPacket1.CreateElement((char*)"ChlType");
    snprintf(strAlarmDeviceSubType, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
    outPacket1.SetElementValue(AccNode1, strAlarmDeviceSubType);

    /* 所属的CMS ID */
    AccNode1 = outPacket1.CreateElement((char*)"CMSID");
    outPacket1.SetElementValue(AccNode1, local_cms_id_get());

    /*RCU上报的报警级别*/
    AccNode1 = outPacket1.CreateElement((char*)"AlarmPriority");
    snprintf(strAlarmPriority, 32, "%u", pGBLogicDeviceInfo->AlarmPriority);
    outPacket1.SetElementValue(AccNode1, strAlarmPriority);

    /*RCU上报的Guard */
    AccNode1 = outPacket1.CreateElement((char*)"Guard");
    snprintf(strGuard, 32, "%u", pGBLogicDeviceInfo->guard_type);
    outPacket1.SetElementValue(AccNode1, strGuard);

    /* RCU上报的Value */
    AccNode1 = outPacket1.CreateElement((char*)"Value");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->Value);

    /* RCU上报的Unit */
    AccNode1 = outPacket1.CreateElement((char*)"Unit");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->Unit);

    /* 扩展的Info字段 */
    outPacket1.SetCurrentElement(ItemAccNode1);
    ItemInfoNode1 = outPacket1.CreateElement((char*)"Info");
    outPacket1.SetCurrentElement(ItemInfoNode1);

    /* 是否可控 */
    AccNode1 = outPacket1.CreateElement((char*)"PTZType");

    if (pGBLogicDeviceInfo->ctrl_enable <= 0)
    {
        snprintf(strPTZType, 16, "%u", 3);
    }
    else
    {
        snprintf(strPTZType, 16, "%u", pGBLogicDeviceInfo->ctrl_enable);
    }

    outPacket1.SetElementValue(AccNode1, strPTZType);

    i = SendMessageToOwnerRouteCMS(pGBLogicDeviceInfo->id, NULL, (char*)outPacket1.GetXml(NULL).c_str(), outPacket1.GetXml(NULL).length(), pDboper);

    if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送目录变化Message消息到本地上级CMS失败");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendNotifyCatalogToRouteCMS Error");
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogEventToRouteCMS() SendNotifyCatalogToRouteCMS Error \r\n");
        return -1;
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送目录变化Message消息到本地上级CMS成功");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyCatalogToRouteCMS OK");
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendNotifyCatalogEventToRouteCMS() SendNotifyCatalogToRouteCMS OK \r\n");
    }

    /* 发送给第三方平台 */
    outPacket2.SetRootTag("Notify");
    AccNode2 = outPacket2.CreateElement((char*)"CmdType");
    outPacket2.SetElementValue(AccNode2, (char*)"Catalog");

    AccNode2 = outPacket2.CreateElement((char*)"SN");
    outPacket2.SetElementValue(AccNode2, (char*)"167");

    AccNode2 = outPacket2.CreateElement((char*)"DeviceID");
    outPacket2.SetElementValue(AccNode2, local_cms_id_get());

    AccNode2 = outPacket2.CreateElement((char*)"SumNum");
    outPacket2.SetElementValue(AccNode2, (char*)"1");

    AccNode2 = outPacket2.CreateElement((char*)"DeviceList");
    outPacket2.SetElementAttr(AccNode2, (char*)"Num", (char*)"1");

    outPacket2.SetCurrentElement(AccNode2);
    ItemAccNode2 = outPacket2.CreateElement((char*)"Item");
    outPacket2.SetCurrentElement(ItemAccNode2);

    AccNode2 = outPacket2.CreateElement((char*)"Event");
    outPacket2.SetElementValue(AccNode2, (char*)strEvent.c_str());

    /* 设备统一编号 */
    AccNode2 = outPacket2.CreateElement((char*)"DeviceID");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->device_id);

    /* 点位名称 */
    AccNode2 = outPacket2.CreateElement((char*)"Name");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->device_name);

    /* 设备生产商 */
    AccNode2 = outPacket2.CreateElement((char*)"Manufacturer");

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->manufacturer);
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, (char*)"");
    }

    /* 设备型号 */
    AccNode2 = outPacket2.CreateElement((char*)"Model");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->model);

    /* 设备归属 */
    AccNode2 = outPacket2.CreateElement((char*)"Owner");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->owner);

    /* 行政区域 */
    AccNode2 = outPacket2.CreateElement((char*)"CivilCode");

    if ('\0' == pGBLogicDeviceInfo->civil_code[0])
    {
        outPacket2.SetElementValue(AccNode2, local_civil_code_get());
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->civil_code);
    }

    /* 警区 */
    AccNode2 = outPacket2.CreateElement((char*)"Block");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->block);

    /* 安装地址 */
    AccNode2 = outPacket2.CreateElement((char*)"Address");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->address);

    /* 是否有子设备 */
    AccNode2 = outPacket2.CreateElement((char*)"Parental");
    snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
    outPacket2.SetElementValue(AccNode2, strParental);

    /* 父设备/区域/系统ID, 和其他平台对接的时候，统一使用本级CMS ID */
    AccNode2 = outPacket2.CreateElement((char*)"ParentID");

    if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
    {
        outPacket2.SetElementValue(AccNode2, local_cms_id_get());
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->virtualParentID);
    }

    /* 信令安全模式*/
    AccNode2 = outPacket2.CreateElement((char*)"SafetyWay");
    snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
    outPacket2.SetElementValue(AccNode2, strSafetyWay);

    /* 注册方式 */
    AccNode2 = outPacket2.CreateElement((char*)"RegisterWay");
    snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
    outPacket2.SetElementValue(AccNode2, strRegisterWay);

    /* 证书序列号*/
    AccNode2 = outPacket2.CreateElement((char*)"CertNum");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->cert_num);

    /* 证书有效标识 */
    AccNode2 = outPacket2.CreateElement((char*)"Certifiable");
    snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
    outPacket2.SetElementValue(AccNode2, strCertifiable);

    /* 无效原因码 */
    AccNode2 = outPacket2.CreateElement((char*)"ErrCode");
    snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
    outPacket2.SetElementValue(AccNode2, strErrCode);

    /* 证书终止有效期*/
    AccNode2 = outPacket2.CreateElement((char*)"EndTime");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->end_time);

    /* 保密属性 */
    AccNode2 = outPacket2.CreateElement((char*)"Secrecy");
    snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
    outPacket2.SetElementValue(AccNode2, strSecrecy);

    /* IP地址*/
    AccNode2 = outPacket2.CreateElement((char*)"IPAddress");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->ip_address);

    /* 端口号 */
    AccNode2 = outPacket2.CreateElement((char*)"Port");
    snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
    outPacket2.SetElementValue(AccNode2, strPort);

    /* 密码*/
    AccNode2 = outPacket2.CreateElement((char*)"Password");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->password);

    /* 点位状态 */
    AccNode2 = outPacket2.CreateElement((char*)"Status");

    if (1 == pGBLogicDeviceInfo->status)
    {
        outPacket2.SetElementValue(AccNode2, (char*)"ON");
    }
    else if (2 == pGBLogicDeviceInfo->status)
    {
        outPacket2.SetElementValue(AccNode2, (char*)"VLOST");
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, (char*)"OFF");
    }

    /* 经度 */
    AccNode2 = outPacket2.CreateElement((char*)"Longitude");
    snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
    outPacket2.SetElementValue(AccNode2, strLongitude);

    /* 纬度 */
    AccNode2 = outPacket2.CreateElement((char*)"Latitude");
    snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
    outPacket2.SetElementValue(AccNode2, strLatitude);

    /* 扩展的Info字段 */
    outPacket2.SetCurrentElement(ItemAccNode2);
    ItemInfoNode2 = outPacket2.CreateElement((char*)"Info");
    outPacket2.SetCurrentElement(ItemInfoNode2);

    /* 是否可控 */
    AccNode2 = outPacket2.CreateElement((char*)"PTZType");

    if (pGBLogicDeviceInfo->ctrl_enable <= 0)
    {
        snprintf(strPTZType, 16, "%u", 3);
    }
    else
    {
        snprintf(strPTZType, 16, "%u", pGBLogicDeviceInfo->ctrl_enable);
    }

    outPacket2.SetElementValue(AccNode2, strPTZType);

    i = SendNotifyTo3PartyRouteCMS2(pGBLogicDeviceInfo->id, NULL, (char*)outPacket2.GetXml(NULL).c_str(), outPacket2.GetXml(NULL).length(), pDboper);

    if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送目录变化Message消息到第三方上级CMS失败");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendNotifyCatalogToRouteCMS Error");
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogToRouteCMS() SendNotifyTo3PartyRouteCMS2 Error \r\n");
        return -1;
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送目录变化Message消息到第三方上级CMS成功");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyCatalogToRouteCMS OK");
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendNotifyCatalogToRouteCMS() SendNotifyTo3PartyRouteCMS2 OK \r\n");
    }

    return i;
}

/*****************************************************************************
 函 数 名  : SendNotifyCatalogMessageToAllRoute
 功能描述  : 发送Catalog变化通知事件消息处理
 输入参数  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int iEvent
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月8日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendNotifyCatalogMessageToAllRoute(GBLogicDevice_info_t* pGBLogicDeviceInfo, int iEvent, DBOper* ptDBoper)
{
    int i = 0;

    if (NULL == pGBLogicDeviceInfo || NULL == ptDBoper)
    {
        return -1;
    }

    /* 发送添加消息给上级CMS  */
    i |= SendNotifyCatalogToRouteCMS(pGBLogicDeviceInfo, iEvent, ptDBoper);

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "SendNotifyCatalogMessageToAllRoute() SendNotifyCatalogToRouteCMS Error:iRet=%d \r\n", i);
    }
    else if (i > 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "SendNotifyCatalogMessageToAllRoute() SendNotifyCatalogToRouteCMS OK:iRet=%d \r\n", i);
    }

    /* 发送添加消息给下级CMS  */
    i |= SendNotifyCatalogToSubCMS(pGBLogicDeviceInfo, iEvent, ptDBoper);

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "SendNotifyCatalogMessageToAllRoute() SendNotifyCatalogToSubCMS Error:iRet=%d \r\n", i);
    }
    else if (i > 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "SendNotifyCatalogMessageToAllRoute() SendNotifyCatalogToSubCMS OK:iRet=%d \r\n", i);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : ExecuteDevicePresetByPresetID
 功能描述  : 根据预置位ID执行预置位
 输入参数  : unsigned int iPresetID
                            DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月10日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ExecuteDevicePresetByPresetID(unsigned int uPresetID, DBOper * pDevice_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    int record_count = 0;
    string strSQL = "";
    char strPresetID[32] = {0};
    GBLogicDevice_info_t* pDestGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pDestGBDeviceInfo = NULL;
    int while_count = 0;

    int iCalleeRoutePos = 0;
    route_info_t* pCalleeRouteInfo = NULL;

    if (uPresetID <= 0 || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG,  "ExecuteDevicePresetByPresetID() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "ExecuteDevicePresetByPresetID() PresetID=%u \r\n", uPresetID);

    snprintf(strPresetID, 32, "%u", uPresetID);

    /* 根据Preset ID，查询预置位表，获取预置位的具体数据 */
    strSQL.clear();
    strSQL = "select * from PresetConfig WHERE ID = ";
    strSQL += strPresetID;

    record_count = pDevice_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetID() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetID() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "ExecuteDevicePresetByPresetID() exit---: No Record Count \r\n");

        /* 可能是同级中从CMS中的预置位，需要发送给从CMS */
        iRet = SendExecuteDevicePresetMessageToSubCMS(strPresetID);

        return iRet;
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "ExecuteDevicePresetByPresetID() record_count=%d\r\n", record_count);

    /* 循环获取轮巡动作数据 */
    do
    {
        unsigned int uDeviceIndex = 0;
        unsigned int uPresetID = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "ExecuteDevicePresetByPresetID() While Count=%d \r\n", while_count);
        }

        uDeviceIndex = 0;
        pDevice_Srv_dboper->GetFieldValue("DeviceIndex", uDeviceIndex);

        uPresetID = 0;
        pDevice_Srv_dboper->GetFieldValue("PresetID", uPresetID);

        /* 获取目的端的设备信息 */
        pDestGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(uDeviceIndex);

        if (NULL != pDestGBLogicDeviceInfo)
        {
            /* 根据逻辑设备所属域进行判断，决定消息走向 */
            if (1 == pDestGBLogicDeviceInfo->other_realm)
            {
                /* 查找上级路由信息 */
                iCalleeRoutePos = route_info_find(pDestGBLogicDeviceInfo->cms_id);

                if (iCalleeRoutePos >= 0)
                {
                    pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

                    if (NULL != pCalleeRouteInfo)
                    {
                        i = SendExecuteDevicePresetMessageToRoute(pDestGBLogicDeviceInfo, pCalleeRouteInfo, uPresetID);

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "执行点位预置位, 发送到上级CMS失败:点位ID=%s, 点位名称=%s, 预置位ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ExcutePointPreset, SendToSuperiorCMS Error:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行点位预置位, 发送到上级CMS成功:点位ID=%s, 点位名称=%s, 预置位ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ExcutePointPreset, SendToSuperiorCMS OK:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);

                            /* 更新自动归位预置位信息，执行预置位之后需要自动归为操作 */
                            iRet = preset_auto_back_update(uDeviceIndex);

                            if (iRet < 0)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetID() preset_auto_back_update Error:DeviceIndex=%u\r\n", uDeviceIndex);
                            }
                            else if (iRet > 0)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteDevicePresetByPresetID() preset_auto_back_update OK:DeviceIndex=%u\r\n", uDeviceIndex);
                            }
                        }
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetID() Get Dest Route Info Error, cms_id=%s\r\n", pDestGBLogicDeviceInfo->cms_id);
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetID() Find Dest Route Info Error, cms_id=%s\r\n", pDestGBLogicDeviceInfo->cms_id);
                }
            }
            else
            {
                pDestGBDeviceInfo = GBDevice_info_get_by_stream_type(pDestGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

                if (NULL != pDestGBDeviceInfo)
                {
                    i = SendExecuteDevicePresetMessage(pDestGBLogicDeviceInfo, pDestGBDeviceInfo, uPresetID);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "执行点位预置位, 发送到前端失败:点位ID=%s, 点位名称=%s, 预置位ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ExcutePointPreset, SendToSuperiorCMS Error:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行点位预置位, 发送到前端成功:点位ID=%s, 点位名称=%s, 预置位ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ExcutePointPreset, SendToSuperiorCMS OK:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);

                        /* 更新自动归位预置位信息，执行预置位之后需要自动归为操作 */
                        iRet = preset_auto_back_update(uDeviceIndex);

                        if (iRet < 0)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetID() preset_auto_back_update Error:DeviceIndex=%u\r\n", uDeviceIndex);
                        }
                        else if (iRet > 0)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteDevicePresetByPresetID() preset_auto_back_update OK:DeviceIndex=%u\r\n", uDeviceIndex);
                        }
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetID() Get DestGBDeviceInfo Error:device_id=%s \r\n", pDestGBLogicDeviceInfo->device_id);
                }
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetID() Find Dest GBLogic Device Info Error, DeviceIndex=%u\r\n", uDeviceIndex);
        }
    }
    while (pDevice_Srv_dboper->MoveNext() >= 0);

    return i;
}

/*****************************************************************************
 函 数 名  : ExecuteDeviceAlarmRecord
 功能描述  : 通知TSU执行报警录像
 输入参数  : unsigned int uDeviceIndex
             int flag:0标识停止报警录像，1，标识开始报警录像
             DBOper * pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月3日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ExecuteDeviceAlarmRecord(unsigned int uDeviceIndex, int flag, DBOper * pDevice_Srv_dboper)
{
    int i = 0;
    record_info_t* pRecordInfo = NULL;
    int record_info_index = -1;
    cr_t* pCrData = NULL;

    record_time_sched_t* pRecordTimeSched = NULL;
    int record_status = 0;

    if (uDeviceIndex <= 0 || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG,  "ExecuteDeviceAlarmRecord() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "ExecuteDeviceAlarmRecord() DeviceIndex=%u, flag=%d \r\n", uDeviceIndex, flag);

    record_info_index = record_info_find_by_stream_type(uDeviceIndex, EV9000_STREAM_TYPE_MASTER);

    if (record_info_index < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "ExecuteDeviceAlarmRecord() exit---: record_info_find_by_stream_type Error:DeviceIndex=%u \r\n", uDeviceIndex);
        return -1;
    }

    pRecordInfo = record_info_get(record_info_index);

    if (NULL == pRecordInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "ExecuteDeviceAlarmRecord() exit---: record_info_get Error:record_info_index=%d \r\n", record_info_index);
        return -1;
    }

    if (pRecordInfo->record_cr_index < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "ExecuteDeviceAlarmRecord() exit---: record_cr_index Error \r\n");
        return -1;
    }

    pCrData = call_record_get(pRecordInfo->record_cr_index);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDeviceAlarmRecord() exit---: Get Record Srv Error:record_cr_index=%d \r\n", pRecordInfo->record_cr_index);
        return -1;
    }

    if (0 == pRecordInfo->uID
        && EV9000_RECORD_TYPE_NORMAL == pRecordInfo->record_type) /* 报警录像增加的录像，非普通配置的录像, 只需要调用设置报警录像接口 */
    {
        if (0 == flag) /* 停止报警录像 */
        {
            i = notify_tsu_set_alarm_record(pCrData->tsu_ip, pCrData->task_id, flag);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record Error: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record OK: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
            }

            /* 已经开始的报警录像的需要停止报警录像 */
            if (1 == pRecordInfo->iTSUAlarmRecordStatus)
            {
                pRecordInfo->iTSUAlarmRecordStatus = 0;
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record: TSU Already Stopped Alarm Record:tsu_ip=%s, task_id=%s, flag=%d", pCrData->tsu_ip, pCrData->task_id, flag);
            }

            /* 通知TSU暂停录像 */
            i = notify_tsu_pause_record(pCrData->tsu_ip, pCrData->task_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDeviceAlarmRecord() notify_tsu_pause_record Error: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteDeviceAlarmRecord() notify_tsu_pause_record OK: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
            }
        }
        else if (1 == flag) /* 开始报警录像 */
        {
            /* 通知TSU恢复录像 */
            i = notify_tsu_resume_record(pCrData->tsu_ip, pCrData->task_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDeviceAlarmRecord() notify_tsu_resume_record Error: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteDeviceAlarmRecord() notify_tsu_resume_record OK: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
            }

            i = notify_tsu_set_alarm_record(pCrData->tsu_ip, pCrData->task_id, flag);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record Error: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record OK: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
            }

            /* 已经停止报警录像的需要开始报警录像 */
            if (0 == pRecordInfo->iTSUAlarmRecordStatus)
            {
                pRecordInfo->iTSUAlarmRecordStatus = 1;
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record: TSU Already Start Alarm Record:tsu_ip=%s, task_id=%s, flag=%d", pCrData->tsu_ip, pCrData->task_id, flag);
            }
        }
    }
    else /* 配置的普通录像 */
    {
        if (1 == pRecordInfo->TimeOfAllWeek) /* 全天录像的情况下，只需要调用设置报警录像接口 */
        {
            i = notify_tsu_set_alarm_record(pCrData->tsu_ip, pCrData->task_id, flag);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record Error: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record OK: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
            }

            if (0 == flag) /* 停止报警录像 */
            {
                /* 已经开始的报警录像的需要停止报警录像 */
                if (1 == pRecordInfo->iTSUAlarmRecordStatus)
                {
                    pRecordInfo->iTSUAlarmRecordStatus = 0;
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record: TSU Already Stopped Alarm Record:tsu_ip=%s, task_id=%s, flag=%d", pCrData->tsu_ip, pCrData->task_id, flag);
                }
            }
            else if (1 == flag) /* 开始报警录像 */
            {
                /* 已经停止报警录像的需要开始报警录像 */
                if (0 == pRecordInfo->iTSUAlarmRecordStatus)
                {
                    pRecordInfo->iTSUAlarmRecordStatus = 1;
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record: TSU Already Start Alarm Record:tsu_ip=%s, task_id=%s, flag=%d", pCrData->tsu_ip, pCrData->task_id, flag);
                }
            }
        }
        else /* 非全天录像的情况下，需要判断该时段是否正在录像 */
        {
            if (0 == flag) /* 停止录像 */
            {
                i = notify_tsu_set_alarm_record(pCrData->tsu_ip, pCrData->task_id, flag);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record Error: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record OK: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
                }

                /* 已经开始的报警录像的需要停止报警录像 */
                if (1 == pRecordInfo->iTSUAlarmRecordStatus)
                {
                    pRecordInfo->iTSUAlarmRecordStatus = 0;
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record: TSU Already Stopped Alarm Record:tsu_ip=%s, task_id=%s, flag=%d", pCrData->tsu_ip, pCrData->task_id, flag);
                }

                /* 判断是否需要通知TSU暂停录像 */
                pRecordTimeSched = record_time_sched_get(pRecordInfo->uID);

                if (NULL == pRecordTimeSched)
                {
                    i = notify_tsu_pause_record(pCrData->tsu_ip, pCrData->task_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDeviceAlarmRecord() notify_tsu_pause_record Error: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteDeviceAlarmRecord() notify_tsu_pause_record OK: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
                    }
                }
                else
                {
                    record_status = get_record_status_from_record_time_config(pRecordTimeSched->pDayOfWeekTimeList);

                    if (0 == record_status)
                    {
                        i = notify_tsu_pause_record(pCrData->tsu_ip, pCrData->task_id);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDeviceAlarmRecord() notify_tsu_pause_record Error: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteDeviceAlarmRecord() notify_tsu_pause_record OK: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
                        }
                    }
                }
            }
            else if (1 == flag) /* 开始录像 */
            {
                /* 判断是否需要通知TSU恢复录像 */
                pRecordTimeSched = record_time_sched_get(pRecordInfo->uID);

                if (NULL == pRecordTimeSched)
                {
                    i = notify_tsu_resume_record(pCrData->tsu_ip, pCrData->task_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDeviceAlarmRecord() notify_tsu_resume_record Error: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteDeviceAlarmRecord() notify_tsu_resume_record OK: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
                    }
                }
                else
                {
                    record_status = get_record_status_from_record_time_config(pRecordTimeSched->pDayOfWeekTimeList);

                    if (0 == record_status)
                    {
                        i = notify_tsu_resume_record(pCrData->tsu_ip, pCrData->task_id);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDeviceAlarmRecord() notify_tsu_resume_record Error: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteDeviceAlarmRecord() notify_tsu_resume_record OK: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
                        }
                    }
                }

                i = notify_tsu_set_alarm_record(pCrData->tsu_ip, pCrData->task_id, flag);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record Error: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record OK: tsu_ip=%s, task_id=%s, flag=%d, i=%d", pCrData->tsu_ip, pCrData->task_id, flag, i);
                }

                /* 已经停止的需要开始录像 */
                if (0 == pRecordInfo->iTSUAlarmRecordStatus)
                {
                    pRecordInfo->iTSUAlarmRecordStatus = 1;
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record: TSU Already Start Alarm Record:tsu_ip=%s, task_id=%s, flag=%d", pCrData->tsu_ip, pCrData->task_id, flag);
                }
            }
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : ExecuteDevicePresetByPresetIDAndDeviceIndex
 功能描述  : 根据预置位ID和逻辑设备索引执行预置位
 输入参数  : unsigned int uPresetID
             unsigned int uDeviceIndex
             DBOper * pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月11日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ExecuteDevicePresetByPresetIDAndDeviceIndex(unsigned int uPresetID, unsigned int uDeviceIndex, DBOper * pDevice_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    int record_count = 0;
    string strSQL = "";
    char strPresetID[32] = {0};
    char strDeviceIndex[32] = {0};
    GBLogicDevice_info_t* pDestGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pDestGBDeviceInfo = NULL;
    int while_count = 0;

    int iCalleeRoutePos = 0;
    route_info_t* pCalleeRouteInfo = NULL;

    if (uPresetID <= 0 || uDeviceIndex <= 0 || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG,  "ExecuteDevicePresetByPresetIDAndDeviceIndex() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetIDAndDeviceIndex() PresetID=%u \r\n", uPresetID);

    snprintf(strPresetID, 32, "%u", uPresetID);
    snprintf(strDeviceIndex, 32, "%u", uDeviceIndex);

    /* 根据Preset ID，查询预置位表，获取预置位的具体数据 */
    strSQL.clear();
    strSQL = "select * from PresetConfig WHERE PresetID = ";
    strSQL += strPresetID;
    strSQL += " and DeviceIndex=";
    strSQL += strDeviceIndex;

    record_count = pDevice_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetIDAndDeviceIndex() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetIDAndDeviceIndex() ErrorMsg=%s\r\n", pDevice_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "ExecuteDevicePresetByPresetIDAndDeviceIndex() exit---: No Record Count \r\n");
        return iRet;
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetIDAndDeviceIndex() record_count=%d\r\n", record_count);

    /* 循环获取轮巡动作数据 */
    do
    {
        unsigned int uDeviceIndex = 0;
        unsigned int uPresetID = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "ExecuteDevicePresetByPresetIDAndDeviceIndex() While Count=%d \r\n", while_count);
        }

        uDeviceIndex = 0;
        pDevice_Srv_dboper->GetFieldValue("DeviceIndex", uDeviceIndex);

        uPresetID = 0;
        pDevice_Srv_dboper->GetFieldValue("PresetID", uPresetID);

        /* 获取目的端的设备信息 */
        pDestGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(uDeviceIndex);

        if (NULL != pDestGBLogicDeviceInfo)
        {
            /* 根据逻辑设备所属域进行判断，决定消息走向 */
            if (1 == pDestGBLogicDeviceInfo->other_realm)
            {
                /* 查找上级路由信息 */
                iCalleeRoutePos = route_info_find(pDestGBLogicDeviceInfo->cms_id);

                if (iCalleeRoutePos >= 0)
                {
                    pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

                    if (NULL != pCalleeRouteInfo)
                    {
                        i = SendExecuteDevicePresetMessageToRoute(pDestGBLogicDeviceInfo, pCalleeRouteInfo, uPresetID);

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "执行点位预置位, 发送到上级CMS失败:点位ID=%s, 点位名称=%s, 预置位ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ExcutePointPreset, SendToSuperiorCMS Error:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行点位预置位, 发送到上级CMS成功:点位ID=%s, 点位名称=%s, 预置位ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ExcutePointPreset, SendToSuperiorCMS OK:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);

                            /* 更新自动归位预置位信息，执行预置位之后需要自动归为操作 */
                            iRet = preset_auto_back_update(uDeviceIndex);

                            if (iRet < 0)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetIDAndDeviceIndex() preset_auto_back_update Error:DeviceIndex=%u\r\n", uDeviceIndex);
                            }
                            else if (iRet > 0)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteDevicePresetByPresetIDAndDeviceIndex() preset_auto_back_update OK:DeviceIndex=%u\r\n", uDeviceIndex);
                            }
                        }
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetIDAndDeviceIndex() Get Dest Route Info Error, cms_id=%s\r\n", pDestGBLogicDeviceInfo->cms_id);
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetIDAndDeviceIndex() Find Dest Route Info Error, cms_id=%s\r\n", pDestGBLogicDeviceInfo->cms_id);
                }
            }
            else
            {
                pDestGBDeviceInfo = GBDevice_info_get_by_stream_type(pDestGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

                if (NULL != pDestGBDeviceInfo)
                {
                    i = SendExecuteDevicePresetMessage(pDestGBLogicDeviceInfo, pDestGBDeviceInfo, uPresetID);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "执行点位预置位, 发送到前端失败:点位ID=%s, 点位名称=%s, 预置位ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ExcutePointPreset, SendToSuperiorCMS Error:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行点位预置位, 发送到前端成功:点位ID=%s, 点位名称=%s, 预置位ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ExcutePointPreset, SendToSuperiorCMS OK:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);

                        /* 更新自动归位预置位信息，执行预置位之后需要自动归为操作 */
                        iRet = preset_auto_back_update(uDeviceIndex);

                        if (iRet < 0)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetIDAndDeviceIndex() preset_auto_back_update Error:DeviceIndex=%u\r\n", uDeviceIndex);
                        }
                        else if (iRet > 0)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteDevicePresetByPresetIDAndDeviceIndex() preset_auto_back_update OK:DeviceIndex=%u\r\n", uDeviceIndex);
                        }
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetIDAndDeviceIndex() Get DestGBDeviceInfo Error:device_id=%s \r\n", pDestGBLogicDeviceInfo->device_id);
                }
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteDevicePresetByPresetIDAndDeviceIndex() Find Dest GBLogic Device Info Error, DeviceIndex=%u\r\n", uDeviceIndex);
        }
    }
    while (pDevice_Srv_dboper->MoveNext() >= 0);

    return i;
}

/*****************************************************************************
 函 数 名  : ExecuteControlRCUDevice
 功能描述  : 执行RCU报警输出点位控制
 输入参数  : unsigned int uDeviceIndex
             char* ctrl_value
             DBOper * pDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年1月3日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ExecuteControlRCUDevice(unsigned int uDeviceIndex, char* ctrl_value, DBOper * pDBoper)
{
    int iRet = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pDestGBDeviceInfo = NULL;
    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (uDeviceIndex <= 0 || NULL == ctrl_value || NULL == pDBoper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG,  "ExecuteControlRCUDevice() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    if (ctrl_value[0] == '\0')
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "ExecuteControlRCUDevice() exit---: ctrl_value Error \r\n");
        return -1;
    }

    pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(uDeviceIndex);

    if (NULL == pGBLogicDeviceInfo)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送控制RCU设备报警输出Message消息到前端设备失败:逻辑点位索引=%u, 原因=获取报警点位信息失败", uDeviceIndex);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "ExecuteControlRCUDevice() exit---: GBLogicDevice_info_find_by_device_index Error:uDeviceIndex=%u \r\n", uDeviceIndex);
        return -1;
    }

    pDestGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

    if (NULL != pDestGBDeviceInfo)
    {
        /* 组建XML信息 */
        outPacket.SetRootTag("Control");
        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"LogicDeviceControl");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, (char*)"3476");

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

        AccNode = outPacket.CreateElement((char*)"Value");
        outPacket.SetElementValue(AccNode, ctrl_value);

        /* 推送消息 */
        iRet = SIP_SendMessage(NULL, local_cms_id_get(), pGBLogicDeviceInfo->device_id, pDestGBDeviceInfo->strRegServerIP, pDestGBDeviceInfo->iRegServerPort, pDestGBDeviceInfo->login_ip, pDestGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (iRet != 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送控制RCU设备报警输出Message消息到前端设备失败:逻辑点位ID=%s, 点位名称=%s, 前端设备ID=%s, IP地址=%s, 端口号=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->login_ip, pDestGBDeviceInfo->login_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to send message that control RCU:Logic ID=%s, Name=%s, ID=%s, IP=%s, port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->login_ip, pDestGBDeviceInfo->login_port);

            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteControlRCUDevice() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->login_ip, pDestGBDeviceInfo->login_port);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送控制RCU设备报警输出Message消息到前端设备成功:逻辑点位ID=%s, 点位名称=%s, 前端设备ID=%s, IP地址=%s, 端口号=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->login_ip, pDestGBDeviceInfo->login_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Succeed to send message that control RCU:Logic ID=%s, Name=%s, ID=%s, IP=%s, port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->login_ip, pDestGBDeviceInfo->login_port);
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteControlRCUDevice() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->login_ip, pDestGBDeviceInfo->login_port);
        }
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送控制RCU设备报警输出Message消息到前端设备失败:逻辑点位ID=%s, 点位名称=%s, 原因=获取报警点位对应的物理设备信息失败", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteControlRCUDevice() Get DestGBDeviceInfo Error:device_id=%s \r\n", pGBLogicDeviceInfo->device_id);
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : ShowGBDeviceInfo
 功能描述  : 查看物理设备信息
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月27日 星期六
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void ShowGBDeviceInfo(int sock, int type)
{
    char strLine[] = "\r------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rDevice Index Device ID            Status   Login IP        LoginPort Manufacturer         TransType 3Party LastKeepAliveTime   KeepAliveExpire KeepAliveCount RegEthName RegServerIP     Subscribe UAS\r\n";
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBDevice_Info_Iterator Itr;
    char rbuf[256] = {0};

    time_t utc_time;
    struct tm local_time = { 0 };
    char str_date[12] = {0};
    char str_time[12] = {0};
    char str_head[32] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        return;
    }

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        pGBDeviceInfo = Itr->second;

        if (NULL == pGBDeviceInfo)
        {
            continue;
        }

        if (type <= 1)
        {
            if (type != pGBDeviceInfo->reg_status)
            {
                continue;
            }
        }

        utc_time = pGBDeviceInfo->last_keep_alive_time;
        localtime_r(&utc_time, &local_time);

        strftime(str_date, sizeof(str_date), "%Y-%m-%d", &local_time);
        strftime(str_time, sizeof(str_time), "%H:%M:%S", &local_time);
        snprintf(str_head, 32, "%s %s", str_date, str_time);

        if (0 == pGBDeviceInfo->reg_status)
        {
            snprintf(rbuf, 256, "\r%-12d %-20s %-8s %-15s %-9d %-20s %-9d %-6d %-19s %-15d %-14d %-10s %-15s %-9d %-3d\r\n", pGBDeviceInfo->id, pGBDeviceInfo->device_id, (char*)"Off Line", pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->manufacturer, pGBDeviceInfo->trans_protocol, pGBDeviceInfo->three_party_flag, str_head, pGBDeviceInfo->keep_alive_expires, pGBDeviceInfo->keep_alive_expires_count, pGBDeviceInfo->strRegServerEthName, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->catalog_subscribe_flag, pGBDeviceInfo->reg_info_index);
        }
        else if (1 == pGBDeviceInfo->reg_status)
        {
            snprintf(rbuf, 256, "\r%-12d %-20s %-8s %-15s %-9d %-20s %-9d %-6d %-19s %-15d %-14d %-10s %-15s %-9d %-3d\r\n", pGBDeviceInfo->id, pGBDeviceInfo->device_id, (char*)"On Line", pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->manufacturer, pGBDeviceInfo->trans_protocol, pGBDeviceInfo->three_party_flag, str_head, pGBDeviceInfo->keep_alive_expires, pGBDeviceInfo->keep_alive_expires_count, pGBDeviceInfo->strRegServerEthName, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->catalog_subscribe_flag, pGBDeviceInfo->reg_info_index);
        }
        else
        {
            snprintf(rbuf, 256, "\r%-12d %-20s %-8s %-15s %-9d %-20s %-9d %-6d %-19s %-15d %-14d %-10s %-15s %-9d %-3d\r\n", pGBDeviceInfo->id, pGBDeviceInfo->device_id, (char*)"Unknow", pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->manufacturer, pGBDeviceInfo->trans_protocol, pGBDeviceInfo->three_party_flag, str_head, pGBDeviceInfo->keep_alive_expires, pGBDeviceInfo->keep_alive_expires_count, pGBDeviceInfo->strRegServerEthName, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->catalog_subscribe_flag, pGBDeviceInfo->reg_info_index);
        }

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

/*****************************************************************************
 函 数 名  : ShowLogicDeviceInfo
 功能描述  : 查看逻辑设备信息
 输入参数  : int sock
             int type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年12月5日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void ShowLogicDeviceInfo(int sock, int type)
{
    char strLine[] = "\r-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rDeviceIndex DeviceID             DeviceName                               Status    Enable Ctrl StreamCo MIndex SIndex IIndex RcdType CivilCode VirtualParentID      RealmFlag SHChnl\r\n";
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    char rbuf[256] = {0};
    int iMasterGBDeviceIndex = -1;
    int iSlaveGBDeviceIndex = -1;
    int iIntellGBDeviceIndex = -1;

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        return;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if (NULL == pGBLogicDeviceInfo)
        {
            continue;
        }

        /* 获取主流物理设备信息 */
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL == pGBDeviceInfo)
        {
            iMasterGBDeviceIndex = -1;
        }
        else
        {
            if (pGBDeviceInfo->id != pGBLogicDeviceInfo->phy_mediaDeviceIndex)
            {
                iMasterGBDeviceIndex = 0;
            }
            else
            {
                iMasterGBDeviceIndex = pGBDeviceInfo->id;
            }
        }

        /* 获取从流物理设备信息 */
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_SLAVE);

        if (NULL == pGBDeviceInfo)
        {
            iSlaveGBDeviceIndex = -1;
        }
        else
        {
            iSlaveGBDeviceIndex = pGBDeviceInfo->id;
        }

        /* 获取智能分析流物理设备信息 */
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_INTELLIGENCE);

        if (NULL == pGBDeviceInfo)
        {
            iIntellGBDeviceIndex = -1;
        }
        else
        {
            iIntellGBDeviceIndex = pGBDeviceInfo->id;
        }

        if (type <= 2)
        {
            if (type != pGBLogicDeviceInfo->status)
            {
                continue;
            }
        }
        else if (type == 3)
        {
            if (INTELLIGENT_STATUS_NULL == pGBLogicDeviceInfo->intelligent_status)
            {
                continue;
            }
        }
        else if (type == 4)
        {
            if (EV9000_DEVICETYPE_ALARMINPUT != pGBLogicDeviceInfo->device_type
                && EV9000_DEVICETYPE_ALARMOUTPUT != pGBLogicDeviceInfo->device_type)
            {
                continue;
            }
        }

        if (0 == pGBLogicDeviceInfo->status)
        {
            snprintf(rbuf, 256, "\r%-11u %-20s %-40s %-9s %-6d %-4d %-8d %-6d %-6d %-6d %-7d %-9s %-20s %-9d %-6u\r\n", pGBLogicDeviceInfo->id, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, (char*)"Off Line", pGBLogicDeviceInfo->enable, pGBLogicDeviceInfo->ctrl_enable, pGBLogicDeviceInfo->stream_count, iMasterGBDeviceIndex, iSlaveGBDeviceIndex, iIntellGBDeviceIndex, pGBLogicDeviceInfo->record_type, pGBLogicDeviceInfo->civil_code, pGBLogicDeviceInfo->virtualParentID, pGBLogicDeviceInfo->other_realm, pGBLogicDeviceInfo->shdb_channel_id);
        }
        else if (1 == pGBLogicDeviceInfo->status)
        {
            if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
            {
                snprintf(rbuf, 256, "\r%-11u %-20s %-40s %-9s %-6d %-4d %-8d %-6d %-6d %-6d %-7d %-9s %-20s %-9d %-6u\r\n", pGBLogicDeviceInfo->id, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, (char*)"Intell", pGBLogicDeviceInfo->enable, pGBLogicDeviceInfo->ctrl_enable, pGBLogicDeviceInfo->stream_count, iMasterGBDeviceIndex, iSlaveGBDeviceIndex, iIntellGBDeviceIndex, pGBLogicDeviceInfo->record_type, pGBLogicDeviceInfo->civil_code, pGBLogicDeviceInfo->virtualParentID, pGBLogicDeviceInfo->other_realm, pGBLogicDeviceInfo->shdb_channel_id);
            }
            else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
            {
                snprintf(rbuf, 256, "\r%-11u %-20s %-40s %-9s %-6d %-4d %-8d %-6d %-6d %-6d %-7d %-9s %-20s %-9d %-6u\r\n", pGBLogicDeviceInfo->id, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, (char*)"Close", pGBLogicDeviceInfo->enable, pGBLogicDeviceInfo->ctrl_enable, pGBLogicDeviceInfo->stream_count, iMasterGBDeviceIndex, iSlaveGBDeviceIndex, iIntellGBDeviceIndex, pGBLogicDeviceInfo->record_type, pGBLogicDeviceInfo->civil_code, pGBLogicDeviceInfo->virtualParentID, pGBLogicDeviceInfo->other_realm, pGBLogicDeviceInfo->shdb_channel_id);
            }
            else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
            {
                snprintf(rbuf, 256, "\r%-11u %-20s %-40s %-9s %-6d %-4d %-8d %-6d %-6d %-6d %-7d %-9s %-20s %-9d %-6u\r\n", pGBLogicDeviceInfo->id, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, (char*)"Apart", pGBLogicDeviceInfo->enable, pGBLogicDeviceInfo->ctrl_enable, pGBLogicDeviceInfo->stream_count, iMasterGBDeviceIndex, iSlaveGBDeviceIndex, iIntellGBDeviceIndex, pGBLogicDeviceInfo->record_type, pGBLogicDeviceInfo->civil_code, pGBLogicDeviceInfo->virtualParentID, pGBLogicDeviceInfo->other_realm, pGBLogicDeviceInfo->shdb_channel_id);
            }
            else
            {
                snprintf(rbuf, 256, "\r%-11u %-20s %-40s %-9s %-6d %-4d %-8d %-6d %-6d %-6d %-7d %-9s %-20s %-9d %-6u\r\n", pGBLogicDeviceInfo->id, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, (char*)"On Line", pGBLogicDeviceInfo->enable, pGBLogicDeviceInfo->ctrl_enable, pGBLogicDeviceInfo->stream_count, iMasterGBDeviceIndex, iSlaveGBDeviceIndex, iIntellGBDeviceIndex, pGBLogicDeviceInfo->record_type, pGBLogicDeviceInfo->civil_code, pGBLogicDeviceInfo->virtualParentID, pGBLogicDeviceInfo->other_realm, pGBLogicDeviceInfo->shdb_channel_id);
            }
        }
        else if (2 == pGBLogicDeviceInfo->status)
        {
            snprintf(rbuf, 256, "\r%-11u %-20s %-40s %-9s %-6d %-4d %-11d %-8d %-10d %-11d %-7d %-9s %-20s %-9d %-6u\r\n", pGBLogicDeviceInfo->id, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, (char*)"No Stream", pGBLogicDeviceInfo->enable, pGBLogicDeviceInfo->ctrl_enable, pGBLogicDeviceInfo->stream_count, iMasterGBDeviceIndex, iSlaveGBDeviceIndex, iIntellGBDeviceIndex, pGBLogicDeviceInfo->record_type, pGBLogicDeviceInfo->civil_code, pGBLogicDeviceInfo->virtualParentID, pGBLogicDeviceInfo->other_realm, pGBLogicDeviceInfo->shdb_channel_id);
        }
        else
        {
            snprintf(rbuf, 256, "\r%-11u %-20s %-40s %-9s %-6d %-4d %-11d %-8d %-10d %-11d %-7d %-9s %-20s %-9d %-6u\r\n", pGBLogicDeviceInfo->id, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, (char*)"Unknow", pGBLogicDeviceInfo->enable, pGBLogicDeviceInfo->ctrl_enable, pGBLogicDeviceInfo->stream_count, iMasterGBDeviceIndex, iSlaveGBDeviceIndex, iIntellGBDeviceIndex, pGBLogicDeviceInfo->record_type, pGBLogicDeviceInfo->civil_code, pGBLogicDeviceInfo->virtualParentID, pGBLogicDeviceInfo->other_realm, pGBLogicDeviceInfo->shdb_channel_id);
        }

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

/*****************************************************************************
 函 数 名  : checkNumberOfGBDeviceInfo
 功能描述  : 检查注册的设备总数
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月17日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int checkNumberOfGBDeviceInfo()
{
    int iSum = 0;

    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBDevice_Info_Iterator Itr;

    GBDEVICE_SMUTEX_LOCK();

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        GBDEVICE_SMUTEX_UNLOCK();
        return 0;
    }

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        pGBDeviceInfo = Itr->second;

        if ((NULL == pGBDeviceInfo) || (pGBDeviceInfo->id <= 0))
        {
            continue;
        }

        iSum++;
    }

    GBDEVICE_SMUTEX_UNLOCK();
    return iSum;
}

/*****************************************************************************
 函 数 名  : SendNotifyCatalogMessageToSubCMS
 功能描述  : 发送点位信息给下级CMS
 输入参数  : char* strCMSID
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月9日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendNotifyCatalogMessageToSubCMS(char* strCMSID)
{
    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;

    if (NULL == strCMSID || strCMSID[0] == '\0')
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendNotifyCatalogMessageToSubCMS() exit---: Param Error \r\n");
        return -1;
    }

    pGBDeviceInfo = GBDevice_info_find(strCMSID);

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogMessageToSubCMS() exit---: GBDevice Info Find Error:CMSID=%s \r\n", strCMSID);
        return -1;
    }

    if (EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogMessageToSubCMS() exit---: GBDevice Type Error:CMSID=%s, device_type=%d \r\n", strCMSID, pGBDeviceInfo->device_type);
        return -1;
    }

    if (pGBDeviceInfo->reg_status <= 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogMessageToSubCMS() exit---: GBDevice reg_status Error:CMSID=%s, reg_status=%d \r\n", strCMSID, pGBDeviceInfo->reg_status);
        return -1;
    }

    if (1 == pGBDeviceInfo->three_party_flag)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogMessageToSubCMS() exit---: CMS Is Not Owner:CMSID=%s \r\n", strCMSID);
        return -1;
    }

    i = GetGBDeviceListAndSendNotifyCatalogToSubCMS(pGBDeviceInfo, &g_DBOper);

    return i;
}

/*****************************************************************************
 函 数 名  : GetGBDeviceListAndSendNotifyCatalogToSubCMS
 功能描述  : 获取要推送给下级CMS的点位信息，推送给下级CMS
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             DBOper* pDevice_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月9日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GetGBDeviceListAndSendNotifyCatalogToSubCMS(GBDevice_info_t* pGBDeviceInfo, DBOper* pDevice_Srv_dboper)
{
    int i = 0;
    int index = 0;
    int record_count = 0; /* 记录数 */
    int send_count = 0;   /* 发送的次数 */
    int query_count = 0;  /* 查询数据统计 */
    DOMElement* ListAccNode = NULL;
    char strSN[128] = {0};

    string strSQL = "";
    vector<string> DeviceIDVector;

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GetGBDeviceListAndSendNotifyCatalogToSubCMS() exit---: GBDevice Info Error \r\n");
        return -1;
    }

    if (NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GetGBDeviceListAndSendNotifyCatalogToSubCMS() exit---: Param Error \r\n");
        return -1;
    }

    DeviceIDVector.clear();

    /* 添加所有的逻辑设备 */
    i = AddAllGBLogicDeviceIDToVectorForSubCMS(DeviceIDVector, pGBDeviceInfo->id, pDevice_Srv_dboper);

    /* 4、获取容器中的设备个数 */
    record_count = DeviceIDVector.size();

    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetGBDeviceListAndSendNotifyCatalogToSubCMS() record_count=%d, pGBDeviceInfo->id=%d \r\n", record_count, pGBDeviceInfo->id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "推送点位到下级CMS:下级CMS ID=%s, 下级CMS IP=%s, 下级CMS Port=%d, 推送的点位总数=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, record_count);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send notify catalog message to sub cms:Sub CMS ID=%s, Sub CMS IP=%s, Sub CMS Port=%d, record count=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, record_count);

    /* 5、如果记录数为0 */
    if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "GetGBDeviceListAndSendNotifyCatalogToSubCMS() exit---: No Record Count \r\n");
        return i;
    }

    notify_catalog_sn++;
    snprintf(strSN, 128, "%u", notify_catalog_sn);

    /* 6、循环查找容器，读取用户的设备信息，加入xml中 */
    CPacket* pOutPacket = NULL;

    for (index = 0; index < record_count; index++)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "RouteGetGBDeviceListAndSendCataLogToCMS() DeviceIndex=%u \r\n", device_index);

        /* 如果记录数大于4，则要分次发送 */
        query_count++;

        /* 创建XML头部 */
        i = CreateGBLogicDeviceCatalogResponseXMLHeadForRoute(&pOutPacket, query_count, record_count, strSN, local_cms_id_get(), &ListAccNode);

        /* 加入Item 值 */
        i = AddLogicDeviceInfoToXMLItemForRoute(pOutPacket, ListAccNode, (char*)DeviceIDVector[index].c_str(), 0, pDevice_Srv_dboper);

        if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 0) || (query_count == record_count))
        {
            if (NULL != pOutPacket)
            {
                send_count++;

                /* 发送消息给下级CMS */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送推送点位Message消息到下级CMS失败:下级CMS ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendNotifyCatalogToSubCMS Error:SubCMS ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GetGBDeviceListAndSendNotifyCatalogToSubCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送推送点位Message消息到下级CMS成功:下级CMS ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyCatalogToSubCMS Error:SubCMS ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GetGBDeviceListAndSendNotifyCatalogToSubCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }

                delete pOutPacket;
                pOutPacket = NULL;
            }
        }
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GetGBDeviceListAndSendNotifyCatalogToSubCMS Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : AddAllSubCMSIPToVector
 功能描述  : 将下级CMS的所有IP地址加入到容器中
 输入参数  : vector<string>& SubCmsIPVector
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月18日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void AddAllSubCMSIPToVector(vector<string>& SubCmsIPVector)
{
    GBDevice_Info_Iterator Itr;

    GBDevice_info_t* pProcGBDeviceInfo = NULL;

    GBDEVICE_SMUTEX_LOCK();

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        GBDEVICE_SMUTEX_UNLOCK();
        return;
    }

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        pProcGBDeviceInfo = Itr->second;

        if ((NULL == pProcGBDeviceInfo) || (pProcGBDeviceInfo->id <= 0))
        {
            continue;
        }

        if (EV9000_DEVICETYPE_SIPSERVER != pProcGBDeviceInfo->device_type)
        {
            continue;
        }

        if (pProcGBDeviceInfo->reg_status <= 0)
        {
            continue;
        }

        if (pProcGBDeviceInfo->reg_info_index < 0)
        {
            continue;
        }

        if (pProcGBDeviceInfo->login_ip[0] == '\0')
        {
            continue;
        }

        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddAllSubCMSIPToVector() SubCmsIPVector:login_ip=%s \r\n", pProcGBDeviceInfo->login_ip);
        SubCmsIPVector.push_back(pProcGBDeviceInfo->login_ip);
    }

    GBDEVICE_SMUTEX_UNLOCK();

    return;
}

/*****************************************************************************
 函 数 名  : IsIPInSubCMS
 功能描述  : 查看IP地址是否是下级CMS的IP地址
 输入参数  : vector<string>& SubCmsIPVector
             char* ip_addr
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月18日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int IsIPInSubCMS(vector<string>& SubCmsIPVector, char* ip_addr)
{
    int index = 0;

    if (NULL == ip_addr)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "IsIPInSubCMS() ip_addr Error \r\n");
        return -1;
    }

    if (SubCmsIPVector.size() <= 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "IsIPInSubCMS() SubCmsIPVector size Zero \r\n");
        return 0;
    }

    for (index = 0; index < (int)SubCmsIPVector.size(); index++)
    {
        if (0 == sstrcmp((char*)SubCmsIPVector[index].c_str(), ip_addr))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "IsIPInSubCMS() SubCmsIPVector:index=%d, IP=%s \r\n", index, (char*)SubCmsIPVector[index].c_str());
            return 1;
        }
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "IsIPInSubCMS() SubCmsIPVector Not Found \r\n");
    return 0;
}

/*****************************************************************************
 函 数 名  : checkGBDeviceIsOnline
 功能描述  : 通过ping监测前端设备是否在线
 输入参数  : char* device_ip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月3日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int checkGBDeviceIsOnline(char* device_ip)
{
    if (NULL == device_ip)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "checkGBDeviceIsOnline() device_ip Error \r\n");
        return -1;
    }

    CPing  tmpcPingClass(device_ip, 0);

    if (0 != tmpcPingClass.ping(5))
    {
        if (tmpcPingClass.GetTimeOut() <= 2)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "checkGBDeviceIsOnline() device_ip=%s, Network OK \r\n", device_ip);
            return 1;
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "checkGBDeviceIsOnline() device_ip=%s, Network Error \r\n", device_ip);
            return 0;
        }
    }
    else
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "checkGBDeviceIsOnline() device_ip=%s, Ping Error \r\n", device_ip);
        return 0;
    }
}

/*****************************************************************************
 函 数 名  : RemoveGBLogicDeviceLockInfoByUserInfo
 功能描述  : 根据用户锁定信息移除逻辑点位的锁定信息
 输入参数  : user_info_t* pUserInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年4月7日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int RemoveGBLogicDeviceLockInfoByUserInfo(user_info_t* pUserInfo)
{
    int i = 0;
    int index = 0;
    unsigned int device_index = 0;

    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    vector<unsigned int> DeviceIndexVector;

    if (NULL == pUserInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "RemoveGBLogicDeviceLockInfoByUserInfo() UserInfo Error \r\n");
        return -1;
    }

    DeviceIndexVector.clear();

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return 0;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->id <= 0))
        {
            continue;
        }

        if (LOCK_STATUS_USER_LOCK != pGBLogicDeviceInfo->lock_status)
        {
            continue;
        }

        if (NULL == pGBLogicDeviceInfo->pLockUserInfo)
        {
            pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
            continue;
        }

        if (pGBLogicDeviceInfo->pLockUserInfo != pUserInfo)
        {
            continue;
        }

        pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
        pGBLogicDeviceInfo->pLockUserInfo = NULL;

        DeviceIndexVector.push_back(pGBLogicDeviceInfo->id);
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    for (index = 0; index < (int)DeviceIndexVector.size(); index++)
    {
        device_index = DeviceIndexVector[index];

        i = device_auto_unlock_remove(device_index);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "用户注销登录, 解除锁定点位失败:用户ID=%s, IP地址=%s, 端口号=%d, 逻辑设备device_index=%u, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, device_index, (char*)"从自动解锁队列移除失败");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "User device control command processing, unlock point failed:User ID=%s, User IP=%s, Port=%d, device_index=%u, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, device_index, (char*)"Remove from the list of auto unlock failed.");
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "RemoveGBLogicDeviceLockInfoByUserInfo() device_auto_unlock_remove Error \r\n");
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户注销登录, 解除锁定点位成功:用户ID=%s, IP地址=%s, 端口号=%d, 逻辑设备device_index=%u", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, device_index);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User device control command processing, unlock point sucessfully:User ID=%s, User IP=%s, Port=%d, device_index=%u, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, device_index, (char*)"Remove from the list of auto unlock failed.");
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "RemoveGBLogicDeviceLockInfoByUserInfo() device_auto_unlock_remove OK \r\n");
        }
    }

    DeviceIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : RemoveGBLogicDeviceLockInfoByRouteInfo
 功能描述  : 根据上级平台锁定信息移除逻辑点位的锁定信息
 输入参数  : route_info_t* pRouteInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年4月7日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int RemoveGBLogicDeviceLockInfoByRouteInfo(route_info_t* pRouteInfo)
{
    int i = 0;
    int index = 0;
    unsigned int device_index = 0;

    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    vector<unsigned int> DeviceIndexVector;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "RemoveGBLogicDeviceLockInfoByRouteInfo() RouteInfo Error \r\n");
        return -1;
    }

    DeviceIndexVector.clear();

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return 0;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->id <= 0))
        {
            continue;
        }

        if (LOCK_STATUS_ROUTE_LOCK != pGBLogicDeviceInfo->lock_status)
        {
            continue;
        }

        if (NULL == pGBLogicDeviceInfo->pLockRouteInfo)
        {
            pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
            continue;
        }

        if (pGBLogicDeviceInfo->pLockRouteInfo != pRouteInfo)
        {
            continue;
        }

        pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
        pGBLogicDeviceInfo->pLockRouteInfo = NULL;

        DeviceIndexVector.push_back(pGBLogicDeviceInfo->id);
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    for (index = 0; index < (int)DeviceIndexVector.size(); index++)
    {
        device_index = DeviceIndexVector[index];

        i = device_auto_unlock_remove(device_index);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级平台连接失败, 解除锁定点位失败:上级CMS ID=%s, 上级CMS IP地址=%s, 上级CMS 端口号=%d, 逻辑设备device_index=%u, 原因=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_index, (char*)"从自动解锁队列移除失败");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "User device control command processing, unlock point failed:User ID=%s, User IP=%s, Port=%d, device_index=%u, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_index, (char*)"Remove from the list of auto unlock failed.");
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "RemoveGBLogicDeviceLockInfoByRouteInfo() device_auto_unlock_remove Error \r\n");
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级平台连接失败, 解除锁定点位成功:上级CMS ID=%s, 上级CMS IP地址=%s, 上级CMS 端口号=%d, 逻辑设备device_index=%u", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_index);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User device control command processing, unlock point successfully:User ID=%s, User IP=%s, Port=%d, device_index=%u, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_index, (char*)"Remove from the list of auto unlock failed.");
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "RemoveGBLogicDeviceLockInfoByRouteInfo() device_auto_unlock_remove OK \r\n");
        }
    }

    DeviceIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : GBLogicDeviceConfig_db_refresh_proc
 功能描述  : 设置逻辑点位数据库更新操作标识
 输入参数  : DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月23日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GBLogicDeviceConfig_db_refresh_proc()
{
    if (1 == db_GBLogicDeviceInfo_reload_mark) /* 正在执行 */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "逻辑点位数据库信息正在同步");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Logic point database information are synchronized");
        return 0;
    }

    db_GBLogicDeviceInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 函 数 名  : check_GBLogicDeviceConfig_need_to_reload
 功能描述  : 检查是否需要同步逻辑点位配置表
 输入参数  : DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月23日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void check_GBLogicDeviceConfig_need_to_reload(DBOper* pDboper)
{
    /* 检查是否需要更新数据库标识 */
    if (!db_GBLogicDeviceInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步逻辑点位数据库信息: 开始---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization logic point database information: begain---");
    printf("\r\n\r\ncheck_GBLogicDeviceConfig_need_to_reload() :begain---, time=%d \r\n", time(NULL));

    /* 设置逻辑设备队列的删除标识 */
    set_GBLogicDevice_info_list_del_mark(3);

    /* 将逻辑设备数据库中的变化数据同步到内存 */
    check_GBLogicDevice_info_from_db_to_list(pDboper);

    /* 删除多余的逻辑设备信息 */
    delete_GBLogicDevice_info_from_list_by_mark();
    printf("check_GBLogicDeviceConfig_need_to_reload() :end---, time=%d \r\n", time(NULL));

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步逻辑点位数据库信息: 结束---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization logic point database information: end---");
    db_GBLogicDeviceInfo_reload_mark = 0;

    return;
}

/*****************************************************************************
 函 数 名  : GBDeviceConfig_db_refresh_proc
 功能描述  : 设置物理设备数据库更新操作标识
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月23日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GBDeviceConfig_db_refresh_proc()
{
    if (1 == db_GBDeviceInfo_reload_mark) /* 正在执行 */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "标准物理设备数据库信息正在同步");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Standard physical device database information are synchronized");
        return 0;
    }

    db_GBDeviceInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 函 数 名  : check_GBLogicDeviceConfig_need_to_reload
 功能描述  : 检查是否需要同步物理设备配置表
 输入参数  : DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月23日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void check_GBDeviceConfig_need_to_reload(DBOper* pDboper)
{
    /* 检查是否需要更新数据库标识 */
    if (!db_GBDeviceInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步标准物理设备数据库信息: 开始---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronous standard physical device database information: begian---");
    printf("\r\n\r\ncheck_GBDeviceConfig_need_to_reload() :begain---, time=%d \r\n", time(NULL));

    /* 设置物理设备队列的删除标识 */
    set_GBDevice_info_list_del_mark(1);

    /* 将物理设备数据库中的变化数据同步到内存 */
    check_GBDevice_info_from_db_to_list(pDboper);

    /* 删除多余的物理设备信息 */
    delete_GBDevice_info_from_list_by_mark();
    printf("check_GBDeviceConfig_need_to_reload() :end---, time=%d \r\n", time(NULL));

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步标准物理设备数据库信息: 结束---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronous standard physical device database information: end---");
    db_GBDeviceInfo_reload_mark = 0;

    return;
}

#if DECS("ZRV设备信息队列")
/*****************************************************************************
 函 数 名  : ZRVDevice_info_init
 功能描述  : ZRV设备结构初始化
 输入参数  : ZRVDevice_info_t ** ZRVDevice_info
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ZRVDevice_info_init(ZRVDevice_info_t** ZRVDevice_info)
{
    *ZRVDevice_info = new ZRVDevice_info_t;

    if (*ZRVDevice_info == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "ZRVDevice_info_init() exit---: *ZRVDevice_info Smalloc Error \r\n");
        return -1;
    }

    (*ZRVDevice_info)->id = 0;
    (*ZRVDevice_info)->device_ip[0] = '\0';
    (*ZRVDevice_info)->reg_status = 0;

    (*ZRVDevice_info)->tcp_sock = -1;

    (*ZRVDevice_info)->last_register_time = 0;
    (*ZRVDevice_info)->register_expires = 0;
    (*ZRVDevice_info)->register_expires_count = 0;

    (*ZRVDevice_info)->strRegServerEthName[0] = '\0';
    (*ZRVDevice_info)->strRegServerIP[0] = '\0';

    (*ZRVDevice_info)->del_mark = 0;

    return 0;
}

/*****************************************************************************
 函 数 名  : ZRVDevice_info_free
 功能描述  : ZRV设备结构释放
 输入参数  : ZRVDevice_info_t * ZRVDevice_info
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void ZRVDevice_info_free(ZRVDevice_info_t* ZRVDevice_info)
{
    if (ZRVDevice_info == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "ZRVDevice_info_free() exit---: Param Error \r\n");
        return;
    }

    ZRVDevice_info->id = 0;
    memset(ZRVDevice_info->device_ip, 0, MAX_IP_LEN);
    ZRVDevice_info->reg_status = 0;

    ZRVDevice_info->tcp_sock = -1;

    ZRVDevice_info->last_register_time = 0;
    ZRVDevice_info->register_expires = 0;
    ZRVDevice_info->register_expires_count = 0;

    memset(ZRVDevice_info->strRegServerEthName, 0, MAX_IP_LEN);
    memset(ZRVDevice_info->strRegServerIP, 0, MAX_IP_LEN);

    ZRVDevice_info->del_mark = 0;

    return;
}

/*****************************************************************************
 函 数 名  : ZRVDevice_info_list_init
 功能描述  : ZRV设备信息队列初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ZRVDevice_info_list_init()
{
    g_ZRVDeviceInfoMap.clear();

#ifdef MULTI_THR
    g_ZRVDeviceInfoMapLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_ZRVDeviceInfoMapLock)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "ZRVDevice_info_list_init() exit---: ZRVDevice Info Map Lock Init Error \r\n");
        return -1;
    }

#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : ZRVDevice_info_list_free
 功能描述  : ZRV设备队列释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void ZRVDevice_info_list_free()
{
    ZRVDevice_Info_Iterator Itr;
    ZRVDevice_info_t* ZRVDevice_info = NULL;

    for (Itr = g_ZRVDeviceInfoMap.begin(); Itr != g_ZRVDeviceInfoMap.end(); Itr++)
    {
        ZRVDevice_info = Itr->second;

        if (NULL != ZRVDevice_info)
        {
            ZRVDevice_info_free(ZRVDevice_info);
            delete ZRVDevice_info;
            ZRVDevice_info = NULL;
        }
    }

    g_ZRVDeviceInfoMap.clear();

#ifdef MULTI_THR

    if (NULL != g_ZRVDeviceInfoMapLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_ZRVDeviceInfoMapLock);
        g_ZRVDeviceInfoMapLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 函 数 名  : ZRVDevice_info_list_lock
 功能描述  : ZRV设备队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ZRVDevice_info_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_ZRVDeviceInfoMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "ZRVDevice_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_ZRVDeviceInfoMapLock);

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : ZRVDevice_info_list_unlock
 功能描述  : ZRV设备队列解锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ZRVDevice_info_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_ZRVDeviceInfoMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "ZRVDevice_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_ZRVDeviceInfoMapLock);

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_ZRVDevice_info_list_lock
 功能描述  : ZRV设备队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int debug_ZRVDevice_info_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_ZRVDeviceInfoMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "debug_ZRVDevice_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_ZRVDeviceInfoMapLock, file, line, func);

    iZRVDeviceInfoLockCount++;

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_ZRVDevice_info_list_lock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iZRVDeviceInfoLockCount != iZRVDeviceInfoUnLockCount + 1)
        {
            //printf("\r\n**********%s:%d:%s:debug_ZRVDevice_info_list_lock:iRet=%d, iZRVDeviceInfoLockCount=%lld**********\r\n", file, line, func, iRet, iZRVDeviceInfoLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_ZRVDevice_info_list_lock:iRet=%d, iZRVDeviceInfoLockCount=%lld", file, line, func, iRet, iZRVDeviceInfoLockCount);
        }
    }

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_ZRVDevice_info_list_unlock
 功能描述  : ZRV设备队列解锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int debug_ZRVDevice_info_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_ZRVDeviceInfoMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "debug_ZRVDevice_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_ZRVDeviceInfoMapLock, file,  line, func);

    iZRVDeviceInfoUnLockCount++;

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_ZRVDevice_info_list_unlock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iZRVDeviceInfoLockCount != iZRVDeviceInfoUnLockCount)
        {
            //printf("\r\n**********%s:%d:%s:debug_ZRVDevice_info_list_unlock:iRet=%d, iZRVDeviceInfoUnLockCount=%lld**********\r\n", file, line, func, iRet, iZRVDeviceInfoUnLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_ZRVDevice_info_list_unlock:iRet=%d, iZRVDeviceInfoUnLockCount=%lld", file, line, func, iRet, iZRVDeviceInfoUnLockCount);
        }
    }

#endif

    return iRet;
}

/*****************************************************************************
 函 数 名  : ZRVDevice_info_add
 功能描述  : 添加ZRV设备信息到队列中
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ZRVDevice_info_add(ZRVDevice_info_t* pZRVDeviceInfo)
{
    if (pZRVDeviceInfo == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "ZRVDevice_info_add() exit---: Param Error \r\n");
        return -1;
    }

    ZRVDEVICE_SMUTEX_LOCK();

    g_ZRVDeviceInfoMap[pZRVDeviceInfo->device_ip] = pZRVDeviceInfo;

    ZRVDEVICE_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 函 数 名  : ZRVDevice_info_remove
 功能描述  : 从队列中移除ZRV设备信息
 输入参数  : char* device_ip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ZRVDevice_info_remove(char* device_ip)
{
    if (NULL == device_ip)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "ZRVDevice_info_remove() exit---: Param Error \r\n");
        return -1;
    }

    ZRVDEVICE_SMUTEX_LOCK();
    g_ZRVDeviceInfoMap.erase(device_ip);
    ZRVDEVICE_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : ZRVDevice_info_find
 功能描述  : 从队列中查找ZRV设备
 输入参数  : char* device_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
ZRVDevice_info_t* ZRVDevice_info_find(char* device_ip)
{
    ZRVDevice_info_t* pZRVDeviceInfo = NULL;
    ZRVDevice_Info_Iterator Itr;

    if (NULL == device_ip)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "ZRVDevice_info_find() exit---: Param Error \r\n");
        return NULL;
    }

    ZRVDEVICE_SMUTEX_LOCK();

    if (g_ZRVDeviceInfoMap.size() <= 0)
    {
        ZRVDEVICE_SMUTEX_UNLOCK();
        return NULL;
    }

    Itr = g_ZRVDeviceInfoMap.find(device_ip);

    if (Itr == g_ZRVDeviceInfoMap.end())
    {
        ZRVDEVICE_SMUTEX_UNLOCK();
        return NULL;
    }
    else
    {
        pZRVDeviceInfo = Itr->second;
        ZRVDEVICE_SMUTEX_UNLOCK();
        return pZRVDeviceInfo;
    }

    ZRVDEVICE_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : ZRVDevice_info_find_by_device_index
 功能描述  : 通过物理设备TCP Sock获取物理设备信息
 输入参数  : int tcp_sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
ZRVDevice_info_t* ZRVDevice_info_find_by_tcp_sock(int tcp_sock)
{
    ZRVDevice_info_t* pZRVDeviceInfo = NULL;
    ZRVDevice_Info_Iterator Itr;

    if (tcp_sock < 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "ZRVDevice_info_find_by_device_index() exit---: Param Error \r\n");
        return NULL;
    }

    ZRVDEVICE_SMUTEX_LOCK();

    if (g_ZRVDeviceInfoMap.size() <= 0)
    {
        ZRVDEVICE_SMUTEX_UNLOCK();
        return NULL;
    }

    for (Itr = g_ZRVDeviceInfoMap.begin(); Itr != g_ZRVDeviceInfoMap.end(); Itr++)
    {
        pZRVDeviceInfo = Itr->second;

        if ((NULL == pZRVDeviceInfo) || (pZRVDeviceInfo->id <= 0))
        {
            continue;
        }

        if (pZRVDeviceInfo->device_ip[0] =='\0')
        {
            continue;
        }

        if (pZRVDeviceInfo->tcp_sock <= 0 || pZRVDeviceInfo->reg_status <= 0)
        {
            continue;
        }

        if (pZRVDeviceInfo->id == tcp_sock)
        {
            pZRVDeviceInfo = Itr->second;
            ZRVDEVICE_SMUTEX_UNLOCK();
            return pZRVDeviceInfo;
        }
    }

    ZRVDEVICE_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : scan_ZRVDevice_info_list_for_expires
 功能描述  : 扫描物理设备队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void scan_ZRVDevice_info_list_for_expires()
{
    int iRet = -1;
    ZRVDevice_Info_Iterator Itr;

    ZRVDevice_info_t* pProcZRVDeviceInfo = NULL;
    needtoproc_ZRVDeviceinfo_queue needProc;

    needProc.clear();

    ZRVDEVICE_SMUTEX_LOCK();

    if (g_ZRVDeviceInfoMap.size() <= 0)
    {
        ZRVDEVICE_SMUTEX_UNLOCK();
        return;
    }

    for (Itr = g_ZRVDeviceInfoMap.begin(); Itr != g_ZRVDeviceInfoMap.end(); Itr++)
    {
        pProcZRVDeviceInfo = Itr->second;

        if ((NULL == pProcZRVDeviceInfo) || (pProcZRVDeviceInfo->id <= 0))
        {
            continue;
        }

        if (pProcZRVDeviceInfo->reg_status == 0)
        {
            continue;
        }

        if (pProcZRVDeviceInfo->tcp_sock < 0)
        {
            continue;
        }

        pProcZRVDeviceInfo->register_expires_count--;

        if (pProcZRVDeviceInfo->register_expires_count <= 0)
        {
            needProc.push_back(pProcZRVDeviceInfo);
            continue;
        }
    }

    ZRVDEVICE_SMUTEX_UNLOCK();

    /* 处理需要发送注销消息的 */
    while (!needProc.empty())
    {
        pProcZRVDeviceInfo = (ZRVDevice_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pProcZRVDeviceInfo)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ZRV设备保活消息超时, 主动注销登录:ZRV设备IP地址=%s, TCP Socket=%d", pProcZRVDeviceInfo->device_ip, pProcZRVDeviceInfo->tcp_sock);
            pProcZRVDeviceInfo->reg_status = 0;
            pProcZRVDeviceInfo->tcp_sock = -1;
        }
    }

    needProc.clear();

    return;
}

/*****************************************************************************
 函 数 名  : free_zrv_device_info_by_tcp_socket
 功能描述  : 释放ZRV设备里面的TCP Socket链接
 输入参数  : int tcp_socket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int free_zrv_device_info_by_tcp_socket(int tcp_socket)
{
    int i = 0;
    ZRVDevice_info_t* pTmpZRVDeviceInfo = NULL;
    ZRVDevice_info_t* pZRVDeviceInfo = NULL;
    ZRVDevice_Info_Iterator Itr;

    if (tcp_socket < 0)
    {
        return -1;
    }

    ZRVDEVICE_SMUTEX_LOCK();

    if (g_ZRVDeviceInfoMap.size() <= 0)
    {
        ZRVDEVICE_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_ZRVDeviceInfoMap.begin(); Itr != g_ZRVDeviceInfoMap.end(); Itr++)
    {
        pTmpZRVDeviceInfo = Itr->second;

        if ((NULL == pTmpZRVDeviceInfo) || (pTmpZRVDeviceInfo->device_ip[0] == '\0'))
        {
            continue;
        }

        if (pTmpZRVDeviceInfo->tcp_sock >= 0)
        {
            if (pTmpZRVDeviceInfo->tcp_sock == tcp_socket)
            {
                pTmpZRVDeviceInfo->tcp_sock = -1;
                pTmpZRVDeviceInfo->reg_status = 0;
                pZRVDeviceInfo = pTmpZRVDeviceInfo;
                break;
            }
        }
    }

    ZRVDEVICE_SMUTEX_UNLOCK();

    if (NULL != pZRVDeviceInfo)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ZRV设备TCP连接断开, 主动注销登录:ZRV设备IP地址=%s, TCP Socket=%d", pZRVDeviceInfo->device_ip, tcp_socket);
        SetZRVDeviceToConfigTable(pZRVDeviceInfo, &g_DBOper);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : is_zrv_device_tcp_socket_in_use
 功能描述  : ZRV设备的TCP Soceket 是否使用了
 输入参数  : int tcp_socket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int is_zrv_device_tcp_socket_in_use(int tcp_socket)
{
    ZRVDevice_info_t* pTmpZRVDeviceInfo = NULL;
    ZRVDevice_Info_Iterator Itr;

    if (tcp_socket < 0)
    {
        return 0;
    }

    ZRVDEVICE_SMUTEX_LOCK();

    if (g_ZRVDeviceInfoMap.size() <= 0)
    {
        ZRVDEVICE_SMUTEX_UNLOCK();
        return 0;
    }

    for (Itr = g_ZRVDeviceInfoMap.begin(); Itr != g_ZRVDeviceInfoMap.end(); Itr++)
    {
        pTmpZRVDeviceInfo = Itr->second;

        if ((NULL == pTmpZRVDeviceInfo) || (pTmpZRVDeviceInfo->device_ip[0] == '\0'))
        {
            continue;
        }

        if (pTmpZRVDeviceInfo->tcp_sock >= 0)
        {
            if (pTmpZRVDeviceInfo->tcp_sock == tcp_socket)
            {
                ZRVDEVICE_SMUTEX_UNLOCK();
                return 1;
            }
        }
    }

    ZRVDEVICE_SMUTEX_UNLOCK();
    return 0;
}

int GetZRVDeviceIPForAssginTask(vector<string>& ZRVDeviceIP)
{
    ZRVDevice_info_t* pTmpZRVDeviceInfo = NULL;
    ZRVDevice_Info_Iterator Itr;

    ZRVDEVICE_SMUTEX_LOCK();

    if (g_ZRVDeviceInfoMap.size() <= 0)
    {
        ZRVDEVICE_SMUTEX_UNLOCK();
        return 0;
    }

    for (Itr = g_ZRVDeviceInfoMap.begin(); Itr != g_ZRVDeviceInfoMap.end(); Itr++)
    {
        pTmpZRVDeviceInfo = Itr->second;

        if ((NULL == pTmpZRVDeviceInfo) || (pTmpZRVDeviceInfo->device_ip[0] == '\0'))
        {
            continue;
        }

        if (pTmpZRVDeviceInfo->reg_status <= 0 || pTmpZRVDeviceInfo->tcp_sock < 0)
        {
            continue;
        }

        ZRVDeviceIP.push_back(pTmpZRVDeviceInfo->device_ip);
    }

    ZRVDEVICE_SMUTEX_UNLOCK();
    return 0;
}

int AddAssignCompressTaskToPerZRVDevice(vector<string>& CompressTaskID, int task_count, int task_begin_index, int task_end_index, vector<string>& PerCompressTaskID)
{
    int task_index = 0;
    //SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "分配压缩任务信息到ZRV设备, 任务总数=%d, 每台ZRV设备分配的任务数=%d, ZRV设备索引device_index=%d", task_count, per_task_count_zrvdevice, device_index);
    PerCompressTaskID.clear();

    for (task_index = task_begin_index; task_index < task_end_index; task_index++)
    {
        if (task_index < task_count)
        {
            //SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "分配压缩任务信息到ZRV设备, ZRV设备索引device_index=%d, 任务索引task_index=%d, CompressTaskID=%s", device_index, task_index, (char*)CompressTaskID[task_index].c_str());
            PerCompressTaskID.push_back(CompressTaskID[task_index]);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : ShowZRVDeviceInfo
 功能描述  : 查看ZRV设备信息
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void ShowZRVDeviceInfo(int sock, int type)
{
    char strLine[] = "\r----------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rDevice Index Login IP        Status TCPSocket LastRegisterTime    RegisterExpire RegisterCount\r\n";
    ZRVDevice_info_t* pZRVDeviceInfo = NULL;
    ZRVDevice_Info_Iterator Itr;
    char rbuf[256] = {0};

    time_t utc_time;
    struct tm local_time = { 0 };
    char str_date[12] = {0};
    char str_time[12] = {0};
    char str_head[32] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if (g_ZRVDeviceInfoMap.size() <= 0)
    {
        return;
    }

    for (Itr = g_ZRVDeviceInfoMap.begin(); Itr != g_ZRVDeviceInfoMap.end(); Itr++)
    {
        pZRVDeviceInfo = Itr->second;

        if (NULL == pZRVDeviceInfo)
        {
            continue;
        }

        if (type <= 1)
        {
            if (type != pZRVDeviceInfo->reg_status)
            {
                continue;
            }
        }

        utc_time = pZRVDeviceInfo->last_register_time;
        localtime_r(&utc_time, &local_time);

        strftime(str_date, sizeof(str_date), "%Y-%m-%d", &local_time);
        strftime(str_time, sizeof(str_time), "%H:%M:%S", &local_time);
        snprintf(str_head, 32, "%s %s", str_date, str_time);

        snprintf(rbuf, 256, "\r%-12d %-15s %-6d %-9d %-19s %-14d %-13d\r\n", pZRVDeviceInfo->id, pZRVDeviceInfo->device_ip, pZRVDeviceInfo->reg_status, pZRVDeviceInfo->tcp_sock, str_head, pZRVDeviceInfo->register_expires, pZRVDeviceInfo->register_expires_count);

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

unsigned int SetZRVDeviceToConfigTable(ZRVDevice_info_t* pZRVDeviceInfo, DBOper* ptDBoper)
{
    int iRet = 0;
    int record_count = -1;
    string strSQL = "";
    string strUpdateSQL = "";
    string strInsertSQL = "";

    char strStatus[32] = {0};
    char strExpires[32] = {0};

    char strDeviceIndex[16] = {0};
    unsigned int iDeviceIndex = 0;
    int iTmpStatus = 0;
    int iTmpExpires = 0;

    if (NULL == pZRVDeviceInfo || NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SetZRVDeviceToConfigTable() exit---:  Param Error \r\n");
        return 0;
    }

    if (pZRVDeviceInfo->device_ip[0] == '\0')
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "SetZRVDeviceToConfigTable() exit---:  device_ip NULL \r\n");
        return 0;
    }

    snprintf(strStatus, 32, "%d", pZRVDeviceInfo->reg_status);
    snprintf(strExpires, 32, "%d", pZRVDeviceInfo->register_expires);

    /* 查找数据库的数据 */
    strSQL.clear();

    strSQL = "select * from ZRVDeviceInfo";
    strSQL += " WHERE DeviceIP like '";
    strSQL += pZRVDeviceInfo->device_ip;
    strSQL += "'";

    record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "SetZRVDeviceToConfigTable() record_count=%d, strSQL=%s \r\n", record_count, strSQL.c_str());

    if (record_count < 0) /* 数据库错误 */
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetZRVDeviceToConfigTable() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetZRVDeviceToConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return 0;
    }
    else if (record_count == 0) /* 没有记录 */
    {
        strInsertSQL.clear();
        strInsertSQL = "insert into ZRVDeviceInfo (DeviceIP,Status,Expires) values (";

        strInsertSQL += "'";
        strInsertSQL += pZRVDeviceInfo->device_ip;
        strInsertSQL += "'";
        strInsertSQL += ",";

        strInsertSQL += strStatus;
        strInsertSQL += ",";

        strInsertSQL += strExpires;
        strInsertSQL += ")";

        iRet = ptDBoper->DB_Insert("", "", strInsertSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetZRVDeviceToConfigTable() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetZRVDeviceToConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
            return 0;
        }

        record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

        if (record_count < 0) /* 数据库错误 */
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetZRVDeviceToConfigTable() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetZRVDeviceToConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
            return 0;
        }
        else if (record_count == 0) /* 没有查到记录 */
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "SetZRVDeviceToConfigTable() exit---: No Record Count \r\n");
            return 0;
        }

        /* 获取数据库记录信息 */
        ptDBoper->GetFieldValue("ID", iDeviceIndex);
    }
    else
    {
        /* 获取数据库记录信息 */
        ptDBoper->GetFieldValue("ID", iDeviceIndex);
        ptDBoper->GetFieldValue("Status", iTmpStatus);
        ptDBoper->GetFieldValue("Expires", iTmpExpires);

        if (iTmpStatus != pZRVDeviceInfo->reg_status
            || iTmpExpires != pZRVDeviceInfo->register_expires) /* 有变化，更新到数据库 */
        {
            strUpdateSQL.clear();
            strUpdateSQL = "UPDATE ZRVDeviceInfo SET";

            strUpdateSQL += " Status = ";
            strUpdateSQL += strStatus;
            strUpdateSQL += ",";

            strUpdateSQL += " Expires = ";
            strUpdateSQL += strExpires;

            strUpdateSQL += " WHERE DeviceIP like '";
            strUpdateSQL += pZRVDeviceInfo->device_ip;
            strUpdateSQL += "'";

            iRet = ptDBoper->DB_Update(strUpdateSQL.c_str(), 1);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetZRVDeviceToConfigTable() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetZRVDeviceToConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
                return 0;
            }
        }
    }

    return iDeviceIndex;
}

/*****************************************************************************
 函 数 名  : get_min_task_zrv_device_count
 功能描述  : 获取任务数最小的ZRV设备
 输入参数  : char* platform_ip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年9月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
char* get_zrv_device_ip_by_min_task_count(char* platform_ip)
{
    int pos = 0;
    int device_index = 0;
    int min_task_count = -1;
    int task_count = 0;
    ZRVDevice_info_t* pZRVDeviceInfo = NULL;
    ZRVDevice_Info_Iterator Itr;
    static char device_ip[MAX_IP_LEN] = {0};

    vector<string> ZRVDeviceIP;
    ZRVDeviceIP.clear();

    if (NULL == platform_ip || platform_ip[0] == '\0')
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "get_zrv_device_ip_by_min_task_count() pcPlatformIP Error\r\n");
        return NULL;
    }

    ZRVDEVICE_SMUTEX_LOCK();

    if (g_ZRVDeviceInfoMap.size() <= 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "get_zrv_device_ip_by_min_task_count() g_ZRVDeviceInfoMap Error\r\n");
        ZRVDEVICE_SMUTEX_UNLOCK();
        return NULL;
    }

    for (Itr = g_ZRVDeviceInfoMap.begin(); Itr != g_ZRVDeviceInfoMap.end(); Itr++)
    {
        pZRVDeviceInfo = Itr->second;

        if ((NULL == pZRVDeviceInfo) || (pZRVDeviceInfo->id <= 0))
        {
            continue;
        }

        if (pZRVDeviceInfo->device_ip[0] =='\0')
        {
            continue;
        }

        if (pZRVDeviceInfo->tcp_sock <= 0 || pZRVDeviceInfo->reg_status <= 0)
        {
            continue;
        }

        ZRVDeviceIP.push_back(pZRVDeviceInfo->device_ip);
    }

    ZRVDEVICE_SMUTEX_UNLOCK();

    if ((int)ZRVDeviceIP.size() <= 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "get_zrv_device_ip_by_min_task_count() ZRVDeviceIP Error\r\n");
        return NULL;
    }

    for (device_index = 0; device_index < (int)ZRVDeviceIP.size(); device_index++)
    {
        task_count = 0;
        task_count = GetCurrentCompressTaskCountByZRVDeviceIP(platform_ip, (char*)ZRVDeviceIP[device_index].c_str());

        if (min_task_count < task_count)
        {
            min_task_count = task_count;
            memset(device_ip, 0, MAX_IP_LEN);
            osip_strncpy(device_ip, (char*)ZRVDeviceIP[device_index].c_str(), MAX_IP_LEN);
        }
    }

    ZRVDeviceIP.clear();

    if (device_ip[0] != '\0')
    {
        return device_ip;
    }
    else
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "get_zrv_device_ip_by_min_task_count() device_ip Error\r\n");
        return NULL;
    }
}

#endif
