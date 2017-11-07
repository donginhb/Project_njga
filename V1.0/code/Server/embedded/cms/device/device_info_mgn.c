
/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
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
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern DBOper g_DBOper;
extern int cms_run_status;  /* 0:û������,1:�������� */

/*----------------------------------------------*
 * �ⲿ����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ȫ�ֱ���                                     *
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
 * ģ�鼶����                                   *
 *----------------------------------------------*/
GBDevice_Info_MAP g_GBDeviceInfoMap;              /* ��׼�����豸��Ϣ���� */
#ifdef MULTI_THR
osip_mutex_t* g_GBDeviceInfoMapLock = NULL;
#endif

GBLogicDevice_Info_MAP g_GBLogicDeviceInfoMap;    /* ��׼�߼��豸��Ϣ���� */
#ifdef MULTI_THR
osip_mutex_t* g_GBLogicDeviceInfoMapLock = NULL;
#endif

ZRVDevice_Info_MAP g_ZRVDeviceInfoMap;              /* ZRV�豸��Ϣ���� */
#ifdef MULTI_THR
osip_mutex_t* g_ZRVDeviceInfoMapLock = NULL;
#endif

int db_GBLogicDeviceInfo_reload_mark = 0; /* �߼��豸���ݿ���±�ʶ:0:����Ҫ���£�1:��Ҫ�������ݿ� */
int db_GBDeviceInfo_reload_mark = 0;      /* ��׼�����豸���ݿ���±�ʶ:0:����Ҫ���£�1:��Ҫ�������ݿ� */

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

#if DECS("�߼��豸����")
/*****************************************************************************
 �� �� ��  : LogicDeviceGroup_init
 ��������  : �߼��豸������Ϣ�ṹ��ʼ��
 �������  : LogicDeviceGroup_t** logic_device_group
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : LogicDeviceGroup_free
 ��������  : �߼��豸������Ϣ�ṹ�ͷ�
 �������  : LogicDeviceGroup_t* logic_device_group
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : AddLogicDeviceGroup
 ��������  : ����߼��豸������Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
             char* group_id
             char* cms_id
             char* group_name
             int sort_id
             char* parent_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : ModifyLogicDeviceGroup
 ��������  : �޸��߼��豸������Ϣ
 �������  : LogicDeviceGroup_t* pLogicDeviceGroup
             char* group_name
             int sort_id
             char* parent_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : GetLogicDeviceGroup
 ��������  : ��ȡ�߼��豸������Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
             char* group_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : SetLogicDeviceGroupChangeFlag
 ��������  : �����߼��豸������Ϣ���޸ı�ʶ
 �������  : GBDevice_info_t* pGBDeviceInfo
             int iChangeFlag
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "���÷�����Ϣɾ����ʶ:����ID=%s, ��������=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Setting the deleted mark of group identification information:group_ID=%s, group_name=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : SynLogicDeviceGroupInfoToDB
 ��������  : ͬ���߼��豸������Ϣ�����ݿ�
 �������  : GBDevice_info_t* pGBDeviceInfo
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

        if (0 == pLogicDeviceGroup->iChangeFlag || 1 == pLogicDeviceGroup->iChangeFlag) /* ���� */
        {
            iRet = InsertLogicDeviceGroupConfig(pLogicDeviceGroup, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceGroupInfoToDB() InsertLogicDeviceGroupConfig Error:GroupID=%s, CMSID=%s, iRet=%d \r\n", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->CMSID, iRet);
            }

            if (0 == pLogicDeviceGroup->iChangeFlag) /* û�б仯 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϱ��ķ�����Ϣû�б仯:����ID=%s, ��������=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Reported no change in the information packet:group_ID=%s, group_name=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            }
            else if (1 == pLogicDeviceGroup->iChangeFlag) /* ���� */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "���ӷ�����Ϣ�����ݿ�:����ID=%s, ��������=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Increasing the packet information to the database:group_ID=%s, group_name=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            }

            pLogicDeviceGroup->iChangeFlag = 0;
        }
        else if (2 == pLogicDeviceGroup->iChangeFlag) /* �޸� */
        {
            iRet = UpdateLogicDeviceGroupConfig(pLogicDeviceGroup, pDevice_Srv_dboper);
            pLogicDeviceGroup->iChangeFlag = 0;

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceGroupInfoToDB() UpdateLogicDeviceGroupConfig Error:GroupID=%s, CMSID=%s, iRet=%d \r\n", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->CMSID, iRet);
            }
            else if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�޸ķ�����Ϣ�����ݿ�:����ID=%s, ��������=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Modify group information to the database:GroupID=%s, GroupName=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            }
        }
        else if (3 == pLogicDeviceGroup->iChangeFlag) /* ɾ�� */
        {
            iRet = DeleteLogicDeviceGroupConfig(pLogicDeviceGroup, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceGroupInfoToDB() DeleteLogicDeviceGroupConfig Error:GroupID=%s, CMSID=%s, iRet=%d \r\n", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->CMSID, iRet);
            }
            else if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�����ݿ�ɾ��������Ϣ:����ID=%s, ��������=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Remove the group of information from the database:GropuID=%s, GroupName=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : SynLogicDeviceGroupInfoToDB2
 ��������  : ͬ�������߼��豸������Ϣ�����ݿ�
 �������  : LogicDeviceGroup_t* pLogicDeviceGroup
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SynLogicDeviceGroupInfoToDB2(LogicDeviceGroup_t* pLogicDeviceGroup, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;

    if (pLogicDeviceGroup == NULL || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SynLogicDeviceGroupInfoToDB2() exit---: Param Error \r\n");
        return -1;
    }

    if (0 == pLogicDeviceGroup->iChangeFlag || 1 == pLogicDeviceGroup->iChangeFlag) /* ���� */
    {
        iRet = InsertLogicDeviceGroupConfig(pLogicDeviceGroup, pDevice_Srv_dboper);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceGroupInfoToDB2() InsertLogicDeviceGroupConfig Error:GroupID=%s, CMSID=%s, iRet=%d \r\n", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->CMSID, iRet);
        }

        if (0 == pLogicDeviceGroup->iChangeFlag) /* û�б仯 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϱ��ķ�����Ϣû�б仯:����ID=%s, ��������=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Reported no change in the information packet:group_ID=%s, group_name=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
        }
        else if (1 == pLogicDeviceGroup->iChangeFlag) /* ���� */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "���ӷ�����Ϣ�����ݿ�:����ID=%s, ��������=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Increasing the packet information to the database:group_ID=%s, group_name=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
        }

        pLogicDeviceGroup->iChangeFlag = 0;
    }
    else if (2 == pLogicDeviceGroup->iChangeFlag) /* �޸� */
    {
        iRet = UpdateLogicDeviceGroupConfig(pLogicDeviceGroup, pDevice_Srv_dboper);
        pLogicDeviceGroup->iChangeFlag = 0;

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceGroupInfoToDB2() UpdateLogicDeviceGroupConfig Error:GroupID=%s, CMSID=%s, iRet=%d \r\n", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->CMSID, iRet);
        }
        else if (iRet > 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�޸ķ�����Ϣ�����ݿ�:����ID=%s, ��������=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Modify group information to the database:GroupID=%s, GroupName=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
        }
    }
    else if (3 == pLogicDeviceGroup->iChangeFlag) /* ɾ�� */
    {
        iRet = DeleteLogicDeviceGroupConfig(pLogicDeviceGroup, pDevice_Srv_dboper);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceGroupInfoToDB2() DeleteLogicDeviceGroupConfig Error:GroupID=%s, CMSID=%s, iRet=%d \r\n", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->CMSID, iRet);
        }
        else if (iRet > 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�����ݿ�ɾ��������Ϣ:����ID=%s, ��������=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Remove the group of information from the database:GropuID=%s, GroupName=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : DelLogicDeviceGroupInfo
 ��������  : ɾ��������߼��豸������Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

        if (3 == pLogicDeviceGroup->iChangeFlag) /* ɾ�� */
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
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���ڴ���ɾ��������Ϣ:����ID=%s, ��������=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
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
 �� �� ��  : DelLogicDeviceGroupInfo2
 ��������  : ɾ������������߼��豸������Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
             LogicDeviceGroup_t* pLogicDeviceGroup
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int DelLogicDeviceGroupInfo2(GBDevice_info_t* pGBDeviceInfo, LogicDeviceGroup_t* pLogicDeviceGroup)
{
    if (pGBDeviceInfo == NULL || pLogicDeviceGroup == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "DelLogicDeviceGroupInfo() exit---: Param Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���ڴ���ɾ��������Ϣ:����ID=%s, ��������=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Remove the grouppacket of information from the database:GroupID=%s, GroupName=%s", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->Name);
    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DelLogicDeviceGroupInfo() osip_list_remove:GroupID=%s, CMSID=%s \r\n", pLogicDeviceGroup->GroupID, pLogicDeviceGroup->CMSID);

    pGBDeviceInfo->LogicDeviceGroupList.erase(pLogicDeviceGroup->GroupID);
    LogicDeviceGroup_free(pLogicDeviceGroup);
    pLogicDeviceGroup = NULL;

    return 0;
}


/*****************************************************************************
 �� �� ��  : InsertLogicDeviceGroupConfig
 ��������  : �������ݵ��߼��豸�������ñ�
 �������  : LogicDeviceGroup_t* pLogicDeviceGroup
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��13�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* 1����ѯSQL��� */
    strQuerySQL.clear();
    strQuerySQL = "select * from LogicDeviceGroupConfig WHERE GroupID like '";
    strQuerySQL += pLogicDeviceGroup->GroupID;
    strQuerySQL += "'";

    /* ����SQL��� */
    strInsertSQL.clear();
    strInsertSQL = "insert into LogicDeviceGroupConfig (GroupID,CMSID,Name,SortID,ParentID) values (";

    /* ���� */
    strInsertSQL += "'";
    strInsertSQL += pLogicDeviceGroup->GroupID;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ������CMS ID */
    strInsertSQL += "'";
    strInsertSQL += pLogicDeviceGroup->CMSID;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ������ */
    strInsertSQL += "'";
    strInsertSQL += pLogicDeviceGroup->Name;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ͬһ���ڵ����������ţ�Ĭ��0������ */
    strInsertSQL += strSortID;

    strInsertSQL += ",";

    /* ���ڵ��� */
    strInsertSQL += "'";
    strInsertSQL += pLogicDeviceGroup->ParentID;
    strInsertSQL += "'";

    strInsertSQL += ")";

    /* ��ѯ���ݿ� */
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
 �� �� ��  : UpdateLogicDeviceGroupConfig
 ��������  : �������ݵ��߼��豸�������ñ�
 �������  : LogicDeviceGroup_t* pLogicDeviceGroup
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ����SQL��� */
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
 �� �� ��  : DeleteLogicDeviceGroupConfig
 ��������  : �����ݿ���ɾ���߼��豸������Ϣ
 �������  : LogicDeviceGroup_t* pLogicDeviceGroup
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ɾ��SQL��� */
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
 �� �� ��  : AddLogicDeviceGroupConfigToDeviceInfo
 ��������  : ����߼��豸������Ϣ�������豸��
 �������  : GBDevice_info_t* pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

        /* ��ӵ����� */
        ret = DeviceGroupConfigInfoProc(pGBDeviceInfo, (char*)strGroupID.c_str(), (char*)strName.c_str(), (char*)strParentID.c_str(), iSortID, 1, pDevice_Srv_dboper, 0);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddLogicDeviceGroupConfigToDeviceInfo() DeviceGroupConfigInfoProc:GroupID=%s, Name=%s, SortID=%d, ParentID=%s, i=%d", (char*)strGroupID.c_str(), (char*)strName.c_str(), iSortID, (char*)strParentID.c_str(), ret);
    }
    while (pDevice_Srv_dboper->MoveNext() >= 0);

    return ret;

}
#endif

#if DECS("�߼��豸�����ϵ")
/*****************************************************************************
 �� �� ��  : LogicDeviceMapGroup_init
 ��������  : �߼��豸�����ϵ��Ϣ�ṹ��ʼ��
 �������  : LogicDeviceMapGroup_t** logic_device_map_group
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : LogicDeviceGroup_free
 ��������  : �߼��豸������Ϣ�ṹ�ͷ�
 �������  : LogicDeviceMapGroup_t* logic_device_map_group
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : AddLogicDeviceMapGroup
 ��������  : ����߼��豸�����ϵ��Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
             char* group_id
             unsigned int device_index
             char* cms_id
             int sort_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : ModifyLogicDeviceMapGroup
 ��������  : �޸��߼��豸�����ϵ
 �������  : LogicDeviceMapGroup_t* pLogicDeviceMapGroup
             char* group_id
             char* cms_id
             int sort_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : GetLogicDeviceMapGroup
 ��������  : ��ȡ�߼��豸�����ϵ��Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
             unsigned int device_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : SetLogicDeviceMapGroupChangeFlag
 ��������  : �����߼��豸�����ϵ��Ϣ���޸ı�ʶ
 �������  : GBDevice_info_t* pGBDeviceInfo
             int iChangeFlag
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "���÷����ϵ��Ϣɾ����ʶ:����ID=%s, �߼��豸����=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Setting group relation information deletion mark:GroupID=%s,LogicDeviceIndex=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : SynLogicDeviceGroupInfoToDB
 ��������  : ͬ���߼��豸������Ϣ�����ݿ�
 �������  : GBDevice_info_t* pGBDeviceInfo
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

        if (0 == pLogicDeviceMapGroup->iChangeFlag || 1 == pLogicDeviceMapGroup->iChangeFlag) /* ���� */
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

            /* ����ԭ���ϼ��Ѿ�����λ�����ˣ���Ҫɾ���� */
            iRet = DeleteExcessLogicDeviceMapGroupConfig(pLogicDeviceMapGroup, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceMapGroupInfoToDB() DeleteExcessLogicDeviceMapGroupConfig Error:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
            }

            if (0 == pLogicDeviceMapGroup->iChangeFlag) /* û�б仯 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϱ��ķ����ϵ��Ϣû�б仯:����ID=%s, �߼��豸����=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Group information reporting  has not changed:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            }
            else if (1 == pLogicDeviceMapGroup->iChangeFlag) /* ���� */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ӷ����ϵ��Ϣ�����ݿ�:����ID=%s, �߼��豸����=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Increasing the group information to the database:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            }

            pLogicDeviceMapGroup->iChangeFlag = 0;
        }
        else if (2 == pLogicDeviceMapGroup->iChangeFlag) /* �޸� */
        {
            iRet = UpdateLogicDeviceMapGroupConfig(pLogicDeviceMapGroup, pDevice_Srv_dboper);
            pLogicDeviceMapGroup->iChangeFlag = 0;

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceMapGroupInfoToDB() UpdateLogicDeviceMapGroupConfig Error:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
            }
            else if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�޸ķ����ϵ��Ϣ�����ݿ�:����ID=%s, �߼��豸����=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Modifying group information into database:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            }
        }
        else if (3 == pLogicDeviceMapGroup->iChangeFlag) /* ɾ�� */
        {
            iRet = DeleteLogicDeviceMapGroupConfig(pLogicDeviceMapGroup, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceMapGroupInfoToDB() DeleteLogicDeviceMapGroupConfig Error:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
            }
            else if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ɾ�������ϵ��Ϣ�����ݿ�:����ID=%s, �߼��豸����=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Delete group relationship information from the database:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : SynLogicDeviceMapGroupInfoToDB2
 ��������  : ͬ��������λ���߼��豸������Ϣ�����ݿ�
 �������  : LogicDeviceMapGroup_t* pLogicDeviceMapGroup
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SynLogicDeviceMapGroupInfoToDB2(LogicDeviceMapGroup_t* pLogicDeviceMapGroup, DBOper* pDevice_Srv_dboper)
{
    int iRet = 0;

    if (pLogicDeviceMapGroup == NULL || NULL == pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SynLogicDeviceMapGroupInfoToDB2() exit---: Param Error \r\n");
        return -1;
    }

    if (0 == pLogicDeviceMapGroup->iChangeFlag || 1 == pLogicDeviceMapGroup->iChangeFlag) /* ���� */
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

        /* ����ԭ���ϼ��Ѿ�����λ�����ˣ���Ҫɾ���� */
        iRet = DeleteExcessLogicDeviceMapGroupConfig(pLogicDeviceMapGroup, pDevice_Srv_dboper);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceMapGroupInfoToDB() DeleteExcessLogicDeviceMapGroupConfig Error:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
        }

        if (0 == pLogicDeviceMapGroup->iChangeFlag) /* û�б仯 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϱ��ķ����ϵ��Ϣû�б仯:����ID=%s, �߼��豸����=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Group information reporting  has not changed:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
        }
        else if (1 == pLogicDeviceMapGroup->iChangeFlag) /* ���� */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ӷ����ϵ��Ϣ�����ݿ�:����ID=%s, �߼��豸����=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Increasing the group information to the database:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
        }

        pLogicDeviceMapGroup->iChangeFlag = 0;
    }
    else if (2 == pLogicDeviceMapGroup->iChangeFlag) /* �޸� */
    {
        iRet = UpdateLogicDeviceMapGroupConfig(pLogicDeviceMapGroup, pDevice_Srv_dboper);
        pLogicDeviceMapGroup->iChangeFlag = 0;

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceMapGroupInfoToDB() UpdateLogicDeviceMapGroupConfig Error:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
        }
        else if (iRet > 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�޸ķ����ϵ��Ϣ�����ݿ�:����ID=%s, �߼��豸����=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Modifying group information into database:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
        }
    }
    else if (3 == pLogicDeviceMapGroup->iChangeFlag) /* ɾ�� */
    {
        iRet = DeleteLogicDeviceMapGroupConfig(pLogicDeviceMapGroup, pDevice_Srv_dboper);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SynLogicDeviceMapGroupInfoToDB() DeleteLogicDeviceMapGroupConfig Error:GroupID=%s, CMSID=%s, DeviceIndex=%u, iRet=%d \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex, iRet);
        }
        else if (iRet > 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ɾ�������ϵ��Ϣ�����ݿ�:����ID=%s, �߼��豸����=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Delete group relationship information from the database:Group_ID=%s, LogicDevice_index=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
        }
    }

    return 0;
}


/*****************************************************************************
 �� �� ��  : DelLogicDeviceGroupInfo
 ��������  : ɾ��������߼��豸������Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

        if (3 == pLogicDeviceMapGroup->iChangeFlag) /* ɾ�� */
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
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���ڴ���ɾ�������ϵ��Ϣ:����ID=%s, �߼��豸����=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
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
 �� �� ��  : DelLogicDeviceMapGroupInfo2
 ��������  : ɾ��ĳһ��������߼��豸������Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
             LogicDeviceMapGroup_t* pLogicDeviceMapGroup
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int DelLogicDeviceMapGroupInfo2(GBDevice_info_t* pGBDeviceInfo, LogicDeviceMapGroup_t* pLogicDeviceMapGroup)
{
    if (NULL == pGBDeviceInfo || NULL == pLogicDeviceMapGroup)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "DelLogicDeviceGroupInfo() exit---: Param Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���ڴ���ɾ�������ϵ��Ϣ:����ID=%s, �߼��豸����=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Delete group relationship information from memory:Group_ID=%s, LogicDeviceIndex=%u", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->DeviceIndex);
    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DelLogicDeviceGroupInfo() osip_list_remove:GroupID=%s, CMSID=%s, DeviceIndex=%u \r\n", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, pLogicDeviceMapGroup->DeviceIndex);

    pGBDeviceInfo->LogicDeviceMapGroupList.erase(pLogicDeviceMapGroup->DeviceIndex);
    LogicDeviceMapGroup_free(pLogicDeviceMapGroup);
    pLogicDeviceMapGroup = NULL;

    return 0;
}

/*****************************************************************************
 �� �� ��  : InsertLogicDeviceMapGroupConfig
 ��������  : �������ݵ��߼��豸�����ϵ���ñ�
 �������  : LogicDeviceMapGroup_t* pLogicDeviceMapGroup
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��13�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* 1����ѯSQL��� */
    strQuerySQL.clear();
    strQuerySQL = "select * from LogicDeviceMapGroupConfig WHERE GroupID like '";
    strQuerySQL += pLogicDeviceMapGroup->GroupID;
    strQuerySQL += "' and CMSID like '";
    strQuerySQL += pLogicDeviceMapGroup->CMSID;
    strQuerySQL += "'";
    strQuerySQL += " and DeviceIndex = ";
    strQuerySQL += strDeviceIndex;

    /* ����SQL��� */
    strInsertSQL.clear();
    strInsertSQL = "insert into LogicDeviceMapGroupConfig (GroupID,DeviceIndex,CMSID,SortID) values (";

    /* ���� */
    strInsertSQL += "'";
    strInsertSQL += pLogicDeviceMapGroup->GroupID;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* �߼��豸����*/
    strInsertSQL += strDeviceIndex;

    strInsertSQL += ",";

    /* ������CMS ID */
    strInsertSQL += "'";
    strInsertSQL += pLogicDeviceMapGroup->CMSID;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ͬһ���ڵ����������ţ�Ĭ��0������ */
    strInsertSQL += strSortID;

    strInsertSQL += ")";

    /* ��ѯ���ݿ� */
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
 �� �� ��  : UpdateLogicDeviceMapGroupConfig
 ��������  : �������ݵ��߼��豸�����ϵ���ñ�
 �������  : LogicDeviceGroup_t* pLogicDeviceGroup
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ����SQL��� */
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
 �� �� ��  : DeleteLogicDeviceMapGroupConfig
 ��������  : �����ݿ���ɾ���߼��豸�����ϵ��Ϣ
 �������  : LogicDeviceMapGroup_t* pLogicDeviceMapGroup
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ɾ��SQL��� */
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
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�����ݿ���ɾ���߼��豸�����ϵ:����ID=%s, CMS ID=%s, �߼��豸����=%s", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, strDeviceIndex);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Deleted logical device group relations from the database :Group ID=%s, CMS ID=%s, Logic Device Index=%s", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, strDeviceIndex);
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : DeleteExcessLogicDeviceMapGroupConfig
 ��������  : ɾ�����ϼ�CMS���õĶ���ķ����ϵ
 �������  : LogicDeviceMapGroup_t* pLogicDeviceMapGroup
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��7��13��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ɾ��SQL��� */
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
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�����ݿ���ɾ����������߼��豸�����ϵ:����ID<>%s����CMS ID<>%s, �߼��豸����=%s", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, strDeviceIndex);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Delete redundant grouping of logical device from the database:Group ID<>%s or CMS ID<>%s, Logic Device Index=%s", pLogicDeviceMapGroup->GroupID, pLogicDeviceMapGroup->CMSID, strDeviceIndex);
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : AddLogicDeviceGroupMapConfigToDeviceInfo
 ��������  : ����߼��豸�����ϵ��Ϣ�������豸��
 �������  : GBDevice_info_t* pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

        /* ��ӵ����� */
        ret = DeviceGroupMapConfigInfoProc(pGBDeviceInfo, (char*)strGroupID.c_str(), uDeviceIndex, iSortID, 1, pDevice_Srv_dboper, 0);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "AddLogicDeviceGroupMapConfigToDeviceInfo() DeviceGroupMapConfigInfoProc:GroupID=%s, DeviceIndex=%u, SortID=%d, i=%d", (char*)strGroupID.c_str(), uDeviceIndex, iSortID, ret);
    }
    while (pDevice_Srv_dboper->MoveNext() >= 0);

    return ret;

}
#endif

#if DECS("��׼�����豸��Ϣ����")
/*****************************************************************************
 �� �� ��  : GBDevice_info_init
 ��������  : ��׼�����豸�ṹ��ʼ��
 �������  : GBDevice_info_t ** GBDevice_info
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

    /* �¼��ϱ����߼��豸������Ϣ���г�ʼ�� */
    (*GBDevice_info)->LogicDeviceGroupList.clear();

    /* �¼��ϱ����߼��豸�����ϵ��Ϣ���г�ʼ�� */
    (*GBDevice_info)->LogicDeviceMapGroupList.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : GBDevice_info_free
 ��������  : ��׼�����豸�ṹ�ͷ�
 �������  : GBDevice_info_t * GBDevice_info
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

    /* �¼��ϱ����߼��豸������Ϣ�����ͷ� */
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

    /* �¼��ϱ����߼��豸�����ϵ��Ϣ�����ͷ� */
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
 �� �� ��  : GBDevice_info_list_init
 ��������  : ��׼�����豸��Ϣ���г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBDevice_info_list_free
 ��������  : ��׼�����豸�����ͷ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBDevice_info_list_lock
 ��������  : ��׼�����豸��������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBDevice_info_list_unlock
 ��������  : ��׼�����豸���н���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : debug_GBDevice_info_list_lock
 ��������  : ��׼�����豸��������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : debug_GBDevice_info_list_unlock
 ��������  : ��׼�����豸���н���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBDevice_info_add
 ��������  : ��ӱ�׼�����豸��Ϣ��������
 �������  : char* device_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBDevice_info_remove
 ��������  : �Ӷ������Ƴ���׼�����豸��Ϣ
 �������  : int pos
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBDevice_info_find
 ��������  : �Ӷ����в��ұ�׼�����豸
 �������  : char* device_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��16��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : Get_GBDevice_Index_By_Device_ID
 ��������  : ���������豸��ID��ȡ�����豸������
 �������  : char* device_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��22��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBDevice_info_find_by_device_index
 ��������  : ͨ�������豸������ȡ�����豸��Ϣ
 �������  : int device_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��29��
    ��    ��   : �û�·����Ϣ����
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBDevice_info_find_by_ip_and_port
 ��������  : ͨ�������豸��IP�Ͷ˿ڲ��������豸
 �������  : char* login_ip
             int login_port
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��9�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBDevice_info_find_by_id_ip_and_port
 ��������  : ͨ�������豸��ID��IP�Ͷ˿ڲ��������豸
 �������  : char* device_id
             char* login_ip
             int login_port
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��9�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : AddLogicDeviceGroupAndMapDataDeviceInfo
 ��������  : ����߼��豸����ͷ����ϵ��Ϣ�������豸��Ϣ��
 �������  : DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��18�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : scan_GBDevice_info_list_for_subscribe
 ��������  : ɨ���׼�����豸���У����Ͷ�����Ϣ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��6��16�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

        if (EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type) /* Ŀǰ�����ڵ�����ƽ̨���Է����� */
        {
            continue;
        }

        if (pGBDeviceInfo->three_party_flag == 0) /* �ǵ�������ʱ�������� */
        {
            continue;
        }

        if (pGBDeviceInfo->reg_status == 0
            && pGBDeviceInfo->catalog_subscribe_flag == 1) /* ��Ҫȥ���� */
        {
            needUnSubscribe.push_back(pGBDeviceInfo);
            continue;
        }
        else if (pGBDeviceInfo->reg_status == 1
                 && pGBDeviceInfo->catalog_subscribe_flag == 0) /* ��Ҫ������ */
        {
            pGBDeviceInfo->catalog_subscribe_interval--;

            if (pGBDeviceInfo->catalog_subscribe_interval <= 0)
            {
                needSubscribe.push_back(pGBDeviceInfo);
                continue;
            }
        }
        else if (pGBDeviceInfo->reg_status == 1
                 && pGBDeviceInfo->catalog_subscribe_flag == 1) /* ��Ҫ����ˢ�¶��� */
        {
            pGBDeviceInfo->catalog_subscribe_expires_count--;

            if (pGBDeviceInfo->catalog_subscribe_expires_count <= (pGBDeviceInfo->catalog_subscribe_expires) / 2)
            {
                needRefresh.push_back(pGBDeviceInfo); /* ����ˢ�¶��� */
                continue;
            }
        }
    }

    GBDEVICE_SMUTEX_UNLOCK();

    /* ������Ҫ���Ͷ�����Ϣ�� */
    while (!needSubscribe.empty())
    {
        pSubscribeGBDeviceInfo = (GBDevice_info_t*) needSubscribe.front();
        needSubscribe.pop_front();

        if (NULL != pSubscribeGBDeviceInfo)
        {
            /* ���ͳ�ʼ���� */
            iRet = SendSubscribeMessageToSubGBDevice(pSubscribeGBDeviceInfo, 0);

            if (iRet != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���ͳ�ʼ������Ϣ��ǰ���豸ʧ��:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pSubscribeGBDeviceInfo->device_id, pSubscribeGBDeviceInfo->login_ip, pSubscribeGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendSubscribeMessageToSubGBDevice Error:SubGBDeviceID=%s, IP=%s, port=%d", pSubscribeGBDeviceInfo->device_id, pSubscribeGBDeviceInfo->login_ip, pSubscribeGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "scan_GBDevice_info_list_for_subscribe() SendSubscribeMessageToSubGBDevice Error \r\n");
                continue;
            }
            else if (iRet == 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ͳ�ʼ������Ϣ��ǰ���豸�ɹ�:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pSubscribeGBDeviceInfo->device_id, pSubscribeGBDeviceInfo->login_ip, pSubscribeGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendSubscribeMessageToSubGBDevice Ok:SubGBDeviceID=%s, IP=%s, port=%d", pSubscribeGBDeviceInfo->device_id, pSubscribeGBDeviceInfo->login_ip, pSubscribeGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "scan_GBDevice_info_list_for_subscribe() SendSubscribeMessageToSubGBDevice OK \r\n");
            }
        }
    }

    needSubscribe.clear();

    /* ������Ҫ����ˢ��ע����Ϣ�� */
    while (!needRefresh.empty())
    {
        pRefreshGBDeviceInfo = (GBDevice_info_t*) needRefresh.front();
        needRefresh.pop_front();

        if (NULL != pRefreshGBDeviceInfo)
        {
            /* ����ˢ�¶��� */
            iRet = SendSubscribeMessageToSubGBDevice(pRefreshGBDeviceInfo, 1);

            if (iRet != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ˢ�¶�����Ϣ��ǰ���豸ʧ��:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pRefreshGBDeviceInfo->device_id, pRefreshGBDeviceInfo->login_ip, pRefreshGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fial to send the message of refreshing the subscribe to the front-end equipment :ID=%s, IP=%s, port=%d", pRefreshGBDeviceInfo->device_id, pRefreshGBDeviceInfo->login_ip, pRefreshGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "scan_GBDevice_info_list_for_subscribe() SendSubscribeMessageToSubGBDevice Error \r\n");
                continue;
            }
            else if (iRet == 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ˢ�¶�����Ϣ��ǰ���豸�ɹ�:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pRefreshGBDeviceInfo->device_id, pRefreshGBDeviceInfo->login_ip, pRefreshGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send the message of refreshing the subscribe to the front-end equipment :ID=%s, IP=%s, port=%d", pRefreshGBDeviceInfo->device_id, pRefreshGBDeviceInfo->login_ip, pRefreshGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "scan_GBDevice_info_list_for_subscribe() SendSubscribeMessageToSubGBDevice OK \r\n");
            }
        }
    }

    needRefresh.clear();

    /* ������Ҫ����ȥ������Ϣ�� */
    while (!needUnSubscribe.empty())
    {
        pUnSubscribeGBDeviceInfo = (GBDevice_info_t*) needUnSubscribe.front();
        needUnSubscribe.pop_front();

        if (NULL != pUnSubscribeGBDeviceInfo)
        {
            /* ����ȡ������ */
            iRet = SendSubscribeMessageToSubGBDevice(pUnSubscribeGBDeviceInfo, 2);

            if (iRet != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ȡ��������Ϣ��ǰ���豸ʧ��:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pUnSubscribeGBDeviceInfo->device_id, pUnSubscribeGBDeviceInfo->login_ip, pUnSubscribeGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fial to send the message of refreshing the subscribe to the front-end equipment :ID=%s, IP=%s, port=%d", pRefreshGBDeviceInfo->device_id, pRefreshGBDeviceInfo->login_ip, pRefreshGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "scan_GBDevice_info_list_for_subscribe() SendSubscribeMessageToSubGBDevice Error \r\n");
                continue;
            }
            else if (iRet == 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ȡ��������Ϣ��ǰ���豸�ɹ�:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pUnSubscribeGBDeviceInfo->device_id, pUnSubscribeGBDeviceInfo->login_ip, pUnSubscribeGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send the message of refreshing the subscribe to the front-end equipment :ID=%s, IP=%s, port=%d", pRefreshGBDeviceInfo->device_id, pRefreshGBDeviceInfo->login_ip, pRefreshGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "scan_GBDevice_info_list_for_subscribe() SendSubscribeMessageToSubGBDevice OK \r\n");
            }
        }
    }

    needUnSubscribe.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : scan_GBDevice_info_list_for_expires
 ��������  : ɨ�������豸����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��7��6�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

        /* û����󱣻�ʱ��Ĺ��˵� */
        if (pProcGBDeviceInfo->last_keep_alive_time <= 0)
        {
            continue;
        }

        /* ����ʱ��ļ�����Ϸ��Ĺ��˵� */
        if (pProcGBDeviceInfo->keep_alive_expires <= 0)
        {
            continue;
        }

        pProcGBDeviceInfo->keep_alive_expires_count = pProcGBDeviceInfo->keep_alive_expires_count - 10;

        if (pProcGBDeviceInfo->keep_alive_expires_count <= 0)
        {
            /* ͨ��ping���һ��ǰ���Ƿ����� */
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
                //SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ǰ���豸������Ϣ��ʱ, ���ǿ���pingͨ��ȡ��ע������:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
                //EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Front end device keep-alive message timeout, but ping OK, cancel log off:front-end device ID=%s, IP address=%s, Port number=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
            }
        }
    }

    GBDEVICE_SMUTEX_UNLOCK();

    /* ������Ҫ����ע����Ϣ�� */
    while (!needProc.empty())
    {
        pProcGBDeviceInfo = (GBDevice_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pProcGBDeviceInfo)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ǰ���豸������Ϣ��ʱ, ����ע����¼:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
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
 �� �� ��  : check_GBDevice_info_from_db_to_list
 ��������  : ���������豸������Ϣ���ڴ���
 �������  : DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��9��7��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ������Ҫ���Ͷ�����Ϣ�� */
    while (!needProc.empty())
    {
        pProcGBDeviceInfo = (GBDevice_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pProcGBDeviceInfo)
        {
            /* ����Э�� */
            if (GetDevCfg(pdboper, pProcGBDeviceInfo->device_id, GBDevice_cfg) > 0)
            {
                /* ����ɾ�� */
                pProcGBDeviceInfo->del_mark = 0;

                if ((EV9000_DEVICETYPE_SIPSERVER == pProcGBDeviceInfo->device_type && 0 == pProcGBDeviceInfo->three_party_flag)
                    || EV9000_DEVICETYPE_DECODER == pProcGBDeviceInfo->device_type
                    || EV9000_DEVICETYPE_MGWSERVER == pProcGBDeviceInfo->device_type)
                {

                }
                else
                {
                    /* ���䷽ʽ���¸�ֵ */
                    if (pProcGBDeviceInfo->trans_protocol != GBDevice_cfg.trans_protocol)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ�˱�׼�����豸��Ϣ�����仯, �����豸ID=%s, IP��ַ=%s, ���䷽ʽ�����仯: �ϵĴ��䷽ʽ=%d, �µĴ��䷽ʽ=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->trans_protocol, GBDevice_cfg.trans_protocol);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "The front standard physical device information change ,ID=%s, IP=%s, Transmission mode change: old transmission mode change=%d, new transmission mode change=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->trans_protocol, GBDevice_cfg.trans_protocol);

                        pProcGBDeviceInfo->trans_protocol = GBDevice_cfg.trans_protocol;
                        DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "check_GBDevice_info_from_db_to_list() trans_protocol changed:device_id=%s, trans_protocol=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->trans_protocol);
                    }
                }

                /* ���ñ�ʶ */
                if (0 == GBDevice_cfg.enable)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ�˱�׼�����豸��Ϣ�����仯, �����豸ID=%s, IP��ַ=%s, �豸������, ����������豸������ҵ��", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "The front standard physical device information change ,ID=%s, IP=%s, Equipment is disabled, all the business of this device will be removed", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip);


                    /* ɾ�����˽ṹ����Ϣ */
                    iRet = DeleteTopologyPhyDeviceInfoFromDB(pProcGBDeviceInfo->device_id, pdboper);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "check_GBDevice_info_from_db_to_list() DeleteTopologyPhyDeviceInfoFromDB:device_id=%s\r\n", pProcGBDeviceInfo->device_id);

                    SIP_UASRemoveRegisterInfo(pProcGBDeviceInfo->reg_info_index);

                    /* ��������˽ṹ��������豸���ͣ���ô��Ҫɾ�����˽ṹ����Ϣ�����ϱ����ϼ�CMS */
                    if (EV9000_DEVICETYPE_DECODER == pProcGBDeviceInfo->device_type
                        || EV9000_DEVICETYPE_SIPSERVER == pProcGBDeviceInfo->device_type
                        || EV9000_DEVICETYPE_MGWSERVER == pProcGBDeviceInfo->device_type)
                    {
                        /* ɾ�����˽ṹ����Ϣ */
                        i = DeleteTopologyPhyDeviceInfoFromDB(pProcGBDeviceInfo->device_id, pdboper);
                    }

                    /* ���ҵ�λ��ҵ�񣬲�ֹͣ����ҵ�� */
                    if (EV9000_DEVICETYPE_DECODER == pProcGBDeviceInfo->device_type) /* �������������в���Ϣֹͣҵ�� */
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
                    else if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pProcGBDeviceInfo->device_type) /* IVS�������С����в���Ϣֹͣҵ�� */
                    {
                        i = StopAllServiceTaskByCallerIPAndPort(pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port); /* �����������Ƶ�� */

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_GBDevice_info_from_db_to_list() StopAllServiceTaskByCallerIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "check_GBDevice_info_from_db_to_list() StopAllServiceTaskByCallerIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }

                        i = StopAllServiceTaskByCalleeIPAndPort(pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port); /* �������͵����ܷ���Ƶ�� */

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "check_GBDevice_info_from_db_to_list() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "check_GBDevice_info_from_db_to_list() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, i);
                        }
                    }
                    else if (EV9000_DEVICETYPE_SIPSERVER == pProcGBDeviceInfo->device_type) /* �¼�CMS */
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
                        /* ֪ͨ�ͻ��ˣ��߼��豸���ܷ������� */
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
                        /* ֪ͨ�ͻ��ˣ��߼��豸���� */
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
                        /* �������ܷ����豸״̬��Ϣ���ͻ��� */
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
                        /* �����豸״̬��Ϣ���ͻ��� */
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

                    /* �Ƴ��߼��豸 */
                    if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pProcGBDeviceInfo->device_type)
                    {
                        //�����߼��豸�����ܷ���״̬
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
                        //�Ƴ������Ӧ���߼��豸
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

                    /* �����豸ҵ���߳� */
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

                    /* �ڴ�ɾ���� */
                    pProcGBDeviceInfo->del_mark = 1;
                }
            }
        }
    }

    needProc.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : set_GBDevice_info_list_del_mark
 ��������  : ���������豸ɾ����ʶ
 �������  : int del_mark
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��6��23��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : delete_GBLogicDevice_info_from_list_by_mark
 ��������  : ����ɾ����ʶ��ɾ�������豸��Ϣ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��7��23�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ɾ����ʶ, ɾ��������ı�׼�����豸��Ϣ: ɾ���ı�׼�����豸����=%d", (int)DeviceIDVector.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "According to delete mark, delete the redundant standard physical device information: the sum of standard physical device is deleted =%d", (int)DeviceIDVector.size());

    if (DeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)DeviceIDVector.size(); index++)
        {
            pGBDeviceInfo = GBDevice_info_find((char*)DeviceIDVector[index].c_str());

            if (NULL != pGBDeviceInfo)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ɾ����ʶ, ɾ��������ı�׼�����豸��Ϣ�ɹ�:��׼�����豸ID=%s, IP��ַ=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
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

#if DECS("��׼�߼��豸��Ϣ����")

/*****************************************************************************
 �� �� ��  : GBDevice_init
 ��������  : �߼��豸�е������豸�ṹ��ʼ��
 �������  : GBDevice_t* gb_device
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��26�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBDevice_free
 ��������  : �߼��豸�е������豸�ṹ�ͷ�
 �������  : GBDevice_t* gb_device
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��26�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBDevice_add
 ��������  : �߼��豸�е������豸���
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int stream_type
             GBDevice_info_t* pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��26�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBDevice_remove
 ��������  : �߼��豸�е������豸�Ƴ�
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int stream_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��4�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBDevice_get_by_stream_type
 ��������  : ����ý�������ͻ�ȡ�߼��豸�е������豸
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int stream_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��26�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBDevice_info_get_by_stream_type
 ��������  : ����ý�������ͻ�ȡ�����豸��Ϣ
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int stream_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��26�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBDevice_info_get_by_stream_type2
 ��������  : ����ý�������ͻ�ȡ�����豸��Ϣ
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int stream_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��10��23�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBLogicDevice_info_init
 ��������  : ��׼�߼��豸�ṹ��ʼ��
 �������  : GBLogicDevice_info_t ** GBLogicDevice_info
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

    /* ��Ӧ�ı�׼�����豸���г�ʼ�� */
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
 �� �� ��  : GBLogicDevice_info_free
 ��������  : ��׼�߼��豸�ṹ�ͷ�
 �������  : GBLogicDevice_info_t * GBLogicDevice_info
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBLogicDevice_info_list_init
 ��������  : ��׼�߼��豸��Ϣ���г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBLogicDevice_info_list_free
 ��������  : ��׼�߼��豸�����ͷ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBLogicDevice_info_list_lock
 ��������  : ��׼�߼��豸��������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBLogicDevice_info_list_unlock
 ��������  : ��׼�߼��豸���н���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : debug_GBLogicDevice_info_list_lock
 ��������  : ��׼�߼��豸��������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : debug_GBLogicDevice_info_list_unlock
 ��������  : ��׼�߼��豸���н���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBLogicDevice_info_add
 ��������  : ����߼��豸��Ϣ������
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��10�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBLogicDevice_info_remove
 ��������  : �Ӷ������Ƴ���׼�߼��豸��Ϣ
 �������  : char* device_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBLogicDevice_info_find
 ��������  : �Ӷ����в��ұ�׼�߼��豸
 �������  : char* device_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��16��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBLogicDevice_info_find_by_device_index
 ��������  : �����豸�����Ӷ����в��ұ�׼�߼��豸
 �������  : int device_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��27��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBLogicDevice_info_find_by_device_index2
 ��������  : �����豸�����Ӷ����в��ұ�׼�߼��豸
 �������  : unsigned int device_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��6��14�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : GBLogicDevice_info_update
 ��������  : ���±�׼�߼��豸��Ϣ
 �������  : GBLogicDevice_info_t* pOldGBLogicDeviceInfo
                            GBLogicDevice_info_t* pNewGBLogicDeviceInfo
                            int change_type:0:�ڴ浽���ݿ�ĸ��£�1:���ݿ⵽�ڴ�ĸ���
 �������  : ��
 �� �� ֵ  :int
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

    if (0 == change_type) /* ��ǰ���ϱ�Ϊ׼���ڴ浽���ݿ�ıȽ� */
    {
        if (('\0' != pNewGBLogicDeviceInfo->device_name[0]
             && (0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IP Camera", 9)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPCamera", 8)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"Camera", 6)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPC", 3)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"ͨ��", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"��ǹ", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"����", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"����", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_id, MAX_ID_LEN)))
            || '\0' == pNewGBLogicDeviceInfo->device_name[0])
        {
            /* ���ǰ���ϱ����� IP Camera��Camera�������ƣ������ϱ���Ϊ�գ��򲻸��£������ݿ�����Ϊ׼ */
        }
        else
        {
            /* ��λ���� */
            if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
            {
                memset(pOldGBLogicDeviceInfo->device_name, 0, MAX_128CHAR_STRING_LEN + 4);
                osip_strncpy(pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name, MAX_128CHAR_STRING_LEN);
            }
        }

        /* �Ƿ����� */
        pOldGBLogicDeviceInfo->enable = pNewGBLogicDeviceInfo->enable;

        /* �����������ͨ������Ҫ���±����豸������ */
        pOldGBLogicDeviceInfo->alarm_device_sub_type = pNewGBLogicDeviceInfo->alarm_device_sub_type;

        /* �Ƿ������������� */
        pOldGBLogicDeviceInfo->other_realm = pNewGBLogicDeviceInfo->other_realm;

        /* �Ƿ�ɿأ����ǰ���ϱ��Ĵ���0,����ǰ���ϱ���Ϊ׼ */
        if (pNewGBLogicDeviceInfo->ctrl_enable > 0)
        {
            pOldGBLogicDeviceInfo->ctrl_enable = pNewGBLogicDeviceInfo->ctrl_enable;
        }
    }

    if (1 == change_type) /* �����ݿ�Ϊ׼�����ݿ⵽�ڴ�ĸ��� */
    {
        /* ��λ���� */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
        {
            memset(pOldGBLogicDeviceInfo->device_name, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name, MAX_128CHAR_STRING_LEN);
        }

        /* �Ƿ����� */
        pOldGBLogicDeviceInfo->enable = pNewGBLogicDeviceInfo->enable;

        /* �Ƿ�ɿ� */
        pOldGBLogicDeviceInfo->ctrl_enable = pNewGBLogicDeviceInfo->ctrl_enable;
        pOldGBLogicDeviceInfo->mic_enable = pNewGBLogicDeviceInfo->mic_enable;
        pOldGBLogicDeviceInfo->frame_count = pNewGBLogicDeviceInfo->frame_count;
        pOldGBLogicDeviceInfo->alarm_duration = pNewGBLogicDeviceInfo->alarm_duration;
    }

    if (NULL != pGBDeviceInfo
        && EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
        && 0 == pGBDeviceInfo->three_party_flag) /* ������¼�����ƽ̨�ĵ�λ������Ҫ�ȽϽ��ñ�ʶ*/
    {
        pOldGBLogicDeviceInfo->mic_enable = pNewGBLogicDeviceInfo->mic_enable;
        pOldGBLogicDeviceInfo->frame_count = pNewGBLogicDeviceInfo->frame_count;
        pOldGBLogicDeviceInfo->alarm_duration = pNewGBLogicDeviceInfo->alarm_duration;
    }

    /* �Ƿ�֧�ֶ����� */
    if (1 == change_type) /* �����ݿ�Ϊ׼�����ݿ⵽�ڴ�ĸ��� */
    {
        pOldGBLogicDeviceInfo->stream_count = pNewGBLogicDeviceInfo->stream_count;
    }

    if (NULL != pGBDeviceInfo
        && ((EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type && 0 == pGBDeviceInfo->three_party_flag)
            || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)) /* ������¼�ƽ̨�ĵ�λ������Ҫ�Ƚ��Ƿ�֧�ֶ�������ʶ*/
    {
        pOldGBLogicDeviceInfo->stream_count = pNewGBLogicDeviceInfo->stream_count;
    }

    if (0 == change_type) /* ���ڴ�Ϊ׼���ڴ浽���ݿ�ĸ��� */
    {
        /* ��Ӧ��ý�������豸���� */
        pOldGBLogicDeviceInfo->phy_mediaDeviceIndex = pNewGBLogicDeviceInfo->phy_mediaDeviceIndex;
    }

    if (1 == change_type) /* �����ݿ�Ϊ׼�����ݿ⵽�ڴ�ĸ��� */
    {
        /* ��Ӧ��ý�������豸ͨ�� */
        pOldGBLogicDeviceInfo->phy_mediaDeviceChannel = pNewGBLogicDeviceInfo->phy_mediaDeviceChannel;

        /* ��Ӧ�Ŀ��������豸���� */
        pOldGBLogicDeviceInfo->phy_ctrlDeviceIndex = pNewGBLogicDeviceInfo->phy_ctrlDeviceIndex;

        /* ��Ӧ�Ŀ��������豸ͨ�� */
        pOldGBLogicDeviceInfo->phy_ctrlDeviceChannel = pNewGBLogicDeviceInfo->phy_ctrlDeviceChannel;
    }

    if (0 == change_type) /* ��ǰ���ϱ�Ϊ׼���ڴ浽���ݿ�ıȽ� */
    {
        /* �豸������ */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->manufacturer, pOldGBLogicDeviceInfo->manufacturer))
        {
            memset(pOldGBLogicDeviceInfo->manufacturer, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->manufacturer, pNewGBLogicDeviceInfo->manufacturer, MAX_128CHAR_STRING_LEN);
        }

        /* �豸�ͺ� */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->model, pOldGBLogicDeviceInfo->model))
        {
            memset(pOldGBLogicDeviceInfo->model, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->model, pNewGBLogicDeviceInfo->model, MAX_128CHAR_STRING_LEN);
        }

        /* �豸���� */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->owner, pOldGBLogicDeviceInfo->owner))
        {
            memset(pOldGBLogicDeviceInfo->owner, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->owner, pNewGBLogicDeviceInfo->owner, MAX_128CHAR_STRING_LEN);
        }

#if 0

        /* �������� */
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

        /* ���� */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->block, pOldGBLogicDeviceInfo->block))
        {
            memset(pOldGBLogicDeviceInfo->block, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->block, pNewGBLogicDeviceInfo->block, MAX_128CHAR_STRING_LEN);
        }

        /* ��װ��ַ */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->address, pOldGBLogicDeviceInfo->address))
        {
            memset(pOldGBLogicDeviceInfo->address, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->address, pNewGBLogicDeviceInfo->address, MAX_128CHAR_STRING_LEN);
        }

        /* �Ƿ������豸 */
        pOldGBLogicDeviceInfo->parental = pNewGBLogicDeviceInfo->parental;

        /* ���豸/����/ϵͳID */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->parentID, pOldGBLogicDeviceInfo->parentID))
        {
            memset(pOldGBLogicDeviceInfo->parentID, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->parentID, pNewGBLogicDeviceInfo->parentID, MAX_128CHAR_STRING_LEN);
        }

        /* ���ȫģʽ*/
        pOldGBLogicDeviceInfo->safety_way = pNewGBLogicDeviceInfo->safety_way;

        /* ע�᷽ʽ */
        pOldGBLogicDeviceInfo->register_way = pNewGBLogicDeviceInfo->register_way;

        /* ֤�����к�*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->cert_num, pOldGBLogicDeviceInfo->cert_num))
        {
            memset(pOldGBLogicDeviceInfo->cert_num, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->cert_num, pNewGBLogicDeviceInfo->cert_num, MAX_128CHAR_STRING_LEN);
        }

        /* ֤����Ч��ʶ */
        pOldGBLogicDeviceInfo->certifiable = pNewGBLogicDeviceInfo->certifiable;

        /* ��Чԭ���� */
        pOldGBLogicDeviceInfo->error_code = pNewGBLogicDeviceInfo->error_code;

        /* ֤����ֹ��Ч��*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->end_time, pOldGBLogicDeviceInfo->end_time))
        {
            memset(pOldGBLogicDeviceInfo->end_time, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->end_time, pNewGBLogicDeviceInfo->end_time, MAX_128CHAR_STRING_LEN);
        }

        /* �������� */
        pOldGBLogicDeviceInfo->secrecy = pNewGBLogicDeviceInfo->secrecy;

        /* IP��ַ*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->ip_address, pOldGBLogicDeviceInfo->ip_address))
        {
            memset(pOldGBLogicDeviceInfo->ip_address, 0, MAX_IP_LEN);
            osip_strncpy(pOldGBLogicDeviceInfo->ip_address, pNewGBLogicDeviceInfo->ip_address, MAX_IP_LEN);
        }

        /* �˿ں�*/
        pOldGBLogicDeviceInfo->port = pNewGBLogicDeviceInfo->port;

        /* ����*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->password, pOldGBLogicDeviceInfo->password))
        {
            memset(pOldGBLogicDeviceInfo->password, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->password, pNewGBLogicDeviceInfo->password, MAX_128CHAR_STRING_LEN);
        }

        /* ��λ״̬ */
        pOldGBLogicDeviceInfo->status = pNewGBLogicDeviceInfo->status;
    }

    if (1 == change_type) /* �����ݿ�Ϊ׼�����ݿ⵽�ڴ�ĸ��� */
    {
        /* ���� */
        if (pNewGBLogicDeviceInfo->longitude != pOldGBLogicDeviceInfo->longitude)
        {
            pOldGBLogicDeviceInfo->longitude = pNewGBLogicDeviceInfo->longitude;
        }

        /* γ�� */
        if (pNewGBLogicDeviceInfo->latitude != pOldGBLogicDeviceInfo->latitude)
        {
            pOldGBLogicDeviceInfo->latitude = pNewGBLogicDeviceInfo->latitude;
        }

        /*  ������ͼ�� */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->map_layer, pOldGBLogicDeviceInfo->map_layer))
        {
            memset(pOldGBLogicDeviceInfo->map_layer, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->map_layer, pNewGBLogicDeviceInfo->map_layer, MAX_128CHAR_STRING_LEN);
        }

        /* ӥ�������Ӧ��Ԥ��ID */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->strResved2, pOldGBLogicDeviceInfo->strResved2))
        {
            memset(pOldGBLogicDeviceInfo->strResved2, 0, MAX_32CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->strResved2, pNewGBLogicDeviceInfo->strResved2, MAX_32CHAR_STRING_LEN);
        }
    }

    if (NULL != pGBDeviceInfo
        && EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
        && 0 == pGBDeviceInfo->three_party_flag) /* ������¼�ƽ̨�ĵ�λ������Ҫ�ȽϾ�γ�� */
    {
        /* ���� */
        if (pNewGBLogicDeviceInfo->longitude > 0 && pNewGBLogicDeviceInfo->longitude != pOldGBLogicDeviceInfo->longitude)
        {
            pOldGBLogicDeviceInfo->longitude = pNewGBLogicDeviceInfo->longitude;
        }

        /* γ�� */
        if (pNewGBLogicDeviceInfo->latitude > 0 && pNewGBLogicDeviceInfo->latitude != pOldGBLogicDeviceInfo->latitude)
        {
            pOldGBLogicDeviceInfo->latitude = pNewGBLogicDeviceInfo->latitude;
        }

        /*  ������ͼ�� */
        if (pNewGBLogicDeviceInfo->map_layer[0] != '\0' && 0 != sstrcmp(pNewGBLogicDeviceInfo->map_layer, pOldGBLogicDeviceInfo->map_layer))
        {
            memset(pOldGBLogicDeviceInfo->map_layer, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->map_layer, pNewGBLogicDeviceInfo->map_layer, MAX_128CHAR_STRING_LEN);
        }
    }

    /* ���� */
    pOldGBLogicDeviceInfo->device_type = pNewGBLogicDeviceInfo->device_type;

#if 0
    /* ����*/
    pOldGBLogicDeviceInfo->id = pNewGBLogicDeviceInfo->id;
#endif

    /*  ������CMSID */
    if (0 != sstrcmp(pNewGBLogicDeviceInfo->cms_id, pOldGBLogicDeviceInfo->cms_id))
    {
        memset(pOldGBLogicDeviceInfo->cms_id, 0, MAX_ID_LEN + 4);
        osip_strncpy(pOldGBLogicDeviceInfo->cms_id, pNewGBLogicDeviceInfo->cms_id, MAX_ID_LEN);
    }

    /* ¼������*/
    if (1 == change_type) /* �����ݿ�Ϊ׼�����ݿ⵽�ڴ�ĸ��� */
    {
        pOldGBLogicDeviceInfo->record_type = pNewGBLogicDeviceInfo->record_type;
    }

#if 1/*2016.10.10 add for RCU*/

    if (0 == change_type) /* �ڴ�ĸ��� */
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

    if (pOldGBLogicDeviceInfo->stream_count > 1) /* ���Ӵ����������豸 */
    {
        /* ���������豸 */
        pSlaveGBDevice = GBDevice_get_by_stream_type(pOldGBLogicDeviceInfo, EV9000_STREAM_TYPE_SLAVE);

        if (NULL != pSlaveGBDevice) /* �Ѿ����ڣ��ȽϿ��Ƿ�һ�� */
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
        else /* �����ڣ�ֱ����� */
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

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���豸��λ����:�߼��豸ID=%s, �߼���λ����=%s, ���Ӵ��������豸", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front-end logic device info update:device ID=%s, logic point name=%s, add slave stream device", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name);
        }
    }
    else if (pOldGBLogicDeviceInfo->stream_count <= 1)/* ȥ�������������豸 */
    {
        /* ���������豸 */
        pSlaveGBDevice = GBDevice_get_by_stream_type(pOldGBLogicDeviceInfo, EV9000_STREAM_TYPE_SLAVE);

        if (NULL != pSlaveGBDevice) /* �Ѿ����� */
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

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���豸��λ����:�߼��豸ID=%s, �߼���λ����=%s, ɾ�����������豸", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front-end logic device info update:device ID=%s, logic point name=%s, remove slave stream device", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name);
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : GBLogicDevice_info_update_for_route
 ��������  : ���±�׼�߼��豸��Ϣ
 �������  : GBLogicDevice_info_t* pOldGBLogicDeviceInfo
             GBLogicDevice_info_t* pNewGBLogicDeviceInfo
             int change_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��10��10��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GBLogicDevice_info_update_for_route(GBLogicDevice_info_t* pOldGBLogicDeviceInfo, GBLogicDevice_info_t* pNewGBLogicDeviceInfo, int change_type)
{
    if (NULL == pOldGBLogicDeviceInfo || NULL == pNewGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBLogicDevice_info_update() exit---: Param Error \r\n");
        return -1;
    }

    if (0 == change_type) /* ��ǰ���ϱ�Ϊ׼���ڴ浽���ݿ�ıȽ� */
    {
        if (('\0' != pNewGBLogicDeviceInfo->device_name[0]
             && (0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IP Camera", 9)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPCamera", 8)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"Camera", 6)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPC", 3)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"ͨ��", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"��ǹ", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"����", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"����", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_id, MAX_ID_LEN)))
            || '\0' == pNewGBLogicDeviceInfo->device_name[0])
        {
            /* ���ǰ���ϱ����� IP Camera��Camera�������ƣ������ϱ���Ϊ�գ��򲻸��£������ݿ�����Ϊ׼ */
        }
        else
        {
            /* ��λ���� */
            if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
            {
                memset(pOldGBLogicDeviceInfo->device_name, 0, MAX_128CHAR_STRING_LEN + 4);
                osip_strncpy(pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name, MAX_128CHAR_STRING_LEN);
            }
        }

        /* �Ƿ����� */
        pOldGBLogicDeviceInfo->enable = pNewGBLogicDeviceInfo->enable;

        /* �����������ͨ������Ҫ���±����豸������ */
        pOldGBLogicDeviceInfo->alarm_device_sub_type = pNewGBLogicDeviceInfo->alarm_device_sub_type;

        /* �Ƿ������������� */
        pOldGBLogicDeviceInfo->other_realm = pNewGBLogicDeviceInfo->other_realm;
    }

    if (1 == change_type) /* �����ݿ�Ϊ׼�����ݿ⵽�ڴ�ĸ��� */
    {
        /* ��λ���� */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
        {
            memset(pOldGBLogicDeviceInfo->device_name, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name, MAX_128CHAR_STRING_LEN);
        }
    }

    if (0 == change_type) /* ��ǰ���ϱ�Ϊ׼���ڴ浽���ݿ�ıȽ� */
    {
        pOldGBLogicDeviceInfo->ctrl_enable = pNewGBLogicDeviceInfo->ctrl_enable;
        pOldGBLogicDeviceInfo->mic_enable = pNewGBLogicDeviceInfo->mic_enable;
        pOldGBLogicDeviceInfo->frame_count = pNewGBLogicDeviceInfo->frame_count;
        pOldGBLogicDeviceInfo->alarm_duration = pNewGBLogicDeviceInfo->alarm_duration;
        pOldGBLogicDeviceInfo->stream_count = pNewGBLogicDeviceInfo->stream_count;
        /* ��Ӧ��ý�������豸���� */
        pOldGBLogicDeviceInfo->phy_mediaDeviceIndex = pNewGBLogicDeviceInfo->phy_mediaDeviceIndex;
    }

    if (1 == change_type) /* �����ݿ�Ϊ׼�����ݿ⵽�ڴ�ĸ��� */
    {
        /* ��Ӧ��ý�������豸ͨ�� */
        pOldGBLogicDeviceInfo->phy_mediaDeviceChannel = pNewGBLogicDeviceInfo->phy_mediaDeviceChannel;

        /* ��Ӧ�Ŀ��������豸���� */
        pOldGBLogicDeviceInfo->phy_ctrlDeviceIndex = pNewGBLogicDeviceInfo->phy_ctrlDeviceIndex;

        /* ��Ӧ�Ŀ��������豸ͨ�� */
        pOldGBLogicDeviceInfo->phy_ctrlDeviceChannel = pNewGBLogicDeviceInfo->phy_ctrlDeviceChannel;
    }

    if (0 == change_type) /* ��ǰ���ϱ�Ϊ׼���ڴ浽���ݿ�ıȽ� */
    {
        /* �豸������ */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->manufacturer, pOldGBLogicDeviceInfo->manufacturer))
        {
            memset(pOldGBLogicDeviceInfo->manufacturer, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->manufacturer, pNewGBLogicDeviceInfo->manufacturer, MAX_128CHAR_STRING_LEN);
        }

        /* �豸�ͺ� */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->model, pOldGBLogicDeviceInfo->model))
        {
            memset(pOldGBLogicDeviceInfo->model, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->model, pNewGBLogicDeviceInfo->model, MAX_128CHAR_STRING_LEN);
        }

        /* �豸���� */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->owner, pOldGBLogicDeviceInfo->owner))
        {
            memset(pOldGBLogicDeviceInfo->owner, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->owner, pNewGBLogicDeviceInfo->owner, MAX_128CHAR_STRING_LEN);
        }

#if 0

        /* �������� */
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

        /* ���� */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->block, pOldGBLogicDeviceInfo->block))
        {
            memset(pOldGBLogicDeviceInfo->block, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->block, pNewGBLogicDeviceInfo->block, MAX_128CHAR_STRING_LEN);
        }

        /* ��װ��ַ */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->address, pOldGBLogicDeviceInfo->address))
        {
            memset(pOldGBLogicDeviceInfo->address, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->address, pNewGBLogicDeviceInfo->address, MAX_128CHAR_STRING_LEN);
        }

        /* �Ƿ������豸 */
        pOldGBLogicDeviceInfo->parental = pNewGBLogicDeviceInfo->parental;

        /* ���豸/����/ϵͳID */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->parentID, pOldGBLogicDeviceInfo->parentID))
        {
            memset(pOldGBLogicDeviceInfo->parentID, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->parentID, pNewGBLogicDeviceInfo->parentID, MAX_128CHAR_STRING_LEN);
        }

        /* ���ȫģʽ*/
        pOldGBLogicDeviceInfo->safety_way = pNewGBLogicDeviceInfo->safety_way;

        /* ע�᷽ʽ */
        pOldGBLogicDeviceInfo->register_way = pNewGBLogicDeviceInfo->register_way;

        /* ֤�����к�*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->cert_num, pOldGBLogicDeviceInfo->cert_num))
        {
            memset(pOldGBLogicDeviceInfo->cert_num, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->cert_num, pNewGBLogicDeviceInfo->cert_num, MAX_128CHAR_STRING_LEN);
        }

        /* ֤����Ч��ʶ */
        pOldGBLogicDeviceInfo->certifiable = pNewGBLogicDeviceInfo->certifiable;

        /* ��Чԭ���� */
        pOldGBLogicDeviceInfo->error_code = pNewGBLogicDeviceInfo->error_code;

        /* ֤����ֹ��Ч��*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->end_time, pOldGBLogicDeviceInfo->end_time))
        {
            memset(pOldGBLogicDeviceInfo->end_time, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->end_time, pNewGBLogicDeviceInfo->end_time, MAX_128CHAR_STRING_LEN);
        }

        /* �������� */
        pOldGBLogicDeviceInfo->secrecy = pNewGBLogicDeviceInfo->secrecy;

        /* IP��ַ*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->ip_address, pOldGBLogicDeviceInfo->ip_address))
        {
            memset(pOldGBLogicDeviceInfo->ip_address, 0, MAX_IP_LEN);
            osip_strncpy(pOldGBLogicDeviceInfo->ip_address, pNewGBLogicDeviceInfo->ip_address, MAX_IP_LEN);
        }

        /* �˿ں�*/
        pOldGBLogicDeviceInfo->port = pNewGBLogicDeviceInfo->port;

        /* ����*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->password, pOldGBLogicDeviceInfo->password))
        {
            memset(pOldGBLogicDeviceInfo->password, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->password, pNewGBLogicDeviceInfo->password, MAX_128CHAR_STRING_LEN);
        }

        /* ��λ״̬ */
        pOldGBLogicDeviceInfo->status = pNewGBLogicDeviceInfo->status;
    }

    if (0 == change_type) /* ��ǰ���ϱ�Ϊ׼���ڴ浽���ݿ�ıȽ� */
    {
        /* ���� */
        if (pNewGBLogicDeviceInfo->longitude > 0 && pNewGBLogicDeviceInfo->longitude != pOldGBLogicDeviceInfo->longitude)
        {
            pOldGBLogicDeviceInfo->longitude = pNewGBLogicDeviceInfo->longitude;
        }

        /* γ�� */
        if (pNewGBLogicDeviceInfo->latitude > 0 && pNewGBLogicDeviceInfo->latitude != pOldGBLogicDeviceInfo->latitude)
        {
            pOldGBLogicDeviceInfo->latitude = pNewGBLogicDeviceInfo->latitude;
        }

        /*  ������ͼ�� */
        if (pNewGBLogicDeviceInfo->map_layer[0] != '\0' && 0 != sstrcmp(pNewGBLogicDeviceInfo->map_layer, pOldGBLogicDeviceInfo->map_layer))
        {
            memset(pOldGBLogicDeviceInfo->map_layer, 0, MAX_128CHAR_STRING_LEN + 4);
            osip_strncpy(pOldGBLogicDeviceInfo->map_layer, pNewGBLogicDeviceInfo->map_layer, MAX_128CHAR_STRING_LEN);
        }
    }

    /* ���� */
    pOldGBLogicDeviceInfo->device_type = pNewGBLogicDeviceInfo->device_type;

#if 0
    /* ����*/
    pOldGBLogicDeviceInfo->id = pNewGBLogicDeviceInfo->id;
#endif

    /*  ������CMSID */
    if (0 != sstrcmp(pNewGBLogicDeviceInfo->cms_id, pOldGBLogicDeviceInfo->cms_id))
    {
        memset(pOldGBLogicDeviceInfo->cms_id, 0, MAX_ID_LEN + 4);
        osip_strncpy(pOldGBLogicDeviceInfo->cms_id, pNewGBLogicDeviceInfo->cms_id, MAX_ID_LEN);
    }

    /* ¼������*/
    if (1 == change_type) /* �����ݿ�Ϊ׼�����ݿ⵽�ڴ�ĸ��� */
    {
        pOldGBLogicDeviceInfo->record_type = pNewGBLogicDeviceInfo->record_type;
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : Get_GBLogicDevice_Index_By_Device_ID
 ��������  : �����߼��豸��ID��ȡ�����豸������
 �������  : char* device_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��22��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : AddAllGBLogicDeviceIDToVectorForDevice
 ��������  : ��������߼��豸��ID ��Ϣ��������
 �������  : vector<string>& DeviceIDVector
             int device_type
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��10�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ��������ܷ�������������豸�����˵�������λ */
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
        //��DeviceIndex��������
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
 �� �� ��  : AddAllGBLogicDeviceIDToVectorForRoute
 ��������  : ������λ�ϱ���������߼��豸��ID ��Ϣ��������
 �������  : vector<string>& DeviceIDVector
             int route_index
             int iThreePartyFlag
             int link_type
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��7��22�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    if (iThreePartyFlag) /* ������ƽ̨ */
    {
        strSQL.clear();
        strSQL = "select GDC.DeviceID from GBLogicDeviceConfig as GDC, RouteDevicePermConfig as RDPC WHERE GDC.ID = RDPC.DeviceIndex and GDC.Enable=1 and RDPC.RouteIndex = "; /* ��ѯȨ�ޱ� */
        snprintf(strRouteIndex, 16, "%u", route_index);
        strSQL += strRouteIndex;

        record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddAllGBLogicDeviceIDToVectorForRoute() record_count=%d, route_index=%d \r\n", record_count, route_index);

        if (record_count <= 0) /* ����ԭ����, ��ȡ���е��߼���λ */
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
        if (1 == link_type) /* ͬ��������£���Ҫ�ϱ�����ǽͨ��,Ȩ�޲���Ч */
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
            strSQL = "select GDC.DeviceID from GBLogicDeviceConfig as GDC, RouteDevicePermConfig as RDPC WHERE GDC.ID = RDPC.DeviceIndex and RDPC.RouteIndex = "; /* ��ѯȨ�ޱ� */
            snprintf(strRouteIndex, 16, "%u", route_index);
            strSQL += strRouteIndex;

            record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddAllGBLogicDeviceIDToVectorForRoute() record_count=%d, route_index=%d \r\n", record_count, route_index);

            if (record_count <= 0) /* ����ԭ����, ��ȡ���е��߼���λ */
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
        //��DeviceIndex��������
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
 �� �� ��  : AddAllGBLogicDeviceIDToVectorForUser
 ��������  : �û���ȡ��λ��ʱ��������е�λDeviceID������
 �������  : vector<string>& DeviceIDVector
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��5��25��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
        //��DeviceIndex��������
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
 �� �� ��  : AddRCUGBLogicDeviceIDToVectorForUser
 ��������  : �û���ȡRCU��λ��ʱ���������RCU��λDeviceID������
 �������  : vector<string>& DeviceIDVector
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��5��25��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
        //��DeviceIndex��������
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
 �� �� ��  : AddAllGBLogicDeviceIDToVectorForSubCMS
 ��������  : �����¼�CMS�ĵ�λȨ�޻�ȡ��λ���index������
 �������  : vector<string>& DeviceIDVector
             int device_index
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��10��9��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
    strSQL = "select GDC.DeviceID from GBLogicDeviceConfig as GDC, GBPhyDevicePermConfig as GDPC WHERE GDC.ID = GDPC.DeviceIndex and GDPC.GBPhyDeviceIndex = "; /* ��ѯȨ�ޱ� */
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
        //��DeviceIndex��������
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
 �� �� ��  : AddAllGBLogicDeviceIDToVector
 ��������  : ������е�λindex������
 �������  : vector<string>& DeviceIDVector
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��7��23�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
        //��DeviceIndex��������
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
 �� �� ��  : SetGBLogicDeviceInfoDelFlag
 ��������  : ���������豸����Ϣ���������߼�ͨ����ɾ����ʶ
 �������  : GBDevice_info_t* pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��12��23�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

        if (pGBLogicDeviceInfo->enable == 0) /* �Ѿ����õĲ���Ҫ�ظ����� */
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
 �� �� ��  : SetGBLogicDeviceInfoEnableFlagByDelMark
 ��������  : ����ɾ����ʶ�����߼��豸�Ľ��ñ�ʶ
 �������  : GBDevice_info_t* pGBDeviceInfo
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��12��23�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

        if (pGBLogicDeviceInfo->enable == 0) /* �Ѿ����õĲ���Ҫ�ظ����� */
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

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���豸Ŀ¼��ѯ��Ӧ��Ϣ:����ɾ����ʶɾ���߼���λ�����ܷ�����ʶ, �߼��豸ID=%s", pGBLogicDeviceInfo->device_id);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end device directory query response message:Delete logic point's intelligent analysis identification based on the deletion identification, logic device ID=%s", pGBLogicDeviceInfo->device_id);
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO,  "SetGBLogicDeviceInfoEnableFlagByDelMark(): Disable Device Intelligent Status: device_id=%s \r\n", pGBLogicDeviceInfo->device_id);

            /* �����豸״̬�仯��Ϣ  */
            iRet = SendDeviceStatusMessageProc(pGBLogicDeviceInfo, pGBLogicDeviceInfo->status, pDevice_Srv_dboper);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMark() SendDeviceStatusMessageProc ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SetGBLogicDeviceInfoEnableFlagByDelMark() SendDeviceStatusMessageProc OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, iRet);
            }

            /* �����豸״̬�澯��Ϣ�������û�  */
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

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���豸Ŀ¼��ѯ��Ӧ��Ϣ:����ɾ����ʶ�����߼��豸�Ľ��ñ�ʶ, �߼��豸ID=%s", pGBLogicDeviceInfo->device_id);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end device directory query response message:Set up the logical device's disabled identification based on the deletion identification,logic device ID=%s", pGBLogicDeviceInfo->device_id);
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO,  "SetGBLogicDeviceInfoEnableFlagByDelMark(): Disable Device: device_id=%s \r\n", pGBLogicDeviceInfo->device_id);

            /* �����豸״̬��Ϣ�������û�  */
            iRet = SendDeviceStatusToAllClientUser((char*)MasterDeviceIDVector[index].c_str(), 0, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "SetGBLogicDeviceInfoEnableFlagByDelMark() SendDeviceStatusToAllClientUser Error:iRet=%d \r\n", iRet);
            }
            else if (iRet > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "SetGBLogicDeviceInfoEnableFlagByDelMark() SendDeviceStatusToAllClientUser OK:iRet=%d \r\n", iRet);
            }

            /* ����Catalog�仯֪ͨ�¼���Ϣ  */
            iRet = SendNotifyCatalogMessageToAllRoute(pGBLogicDeviceInfo, 1, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "SetGBLogicDeviceInfoEnableFlagByDelMark() SendNotifyCatalogMessageToAllRoute Error:iRet=%d \r\n", iRet);
            }
            else if (iRet > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "SetGBLogicDeviceInfoEnableFlagByDelMark() SendNotifyCatalogMessageToAllRoute OK:iRet=%d \r\n", iRet);
            }

            /* �����豸״̬�澯��Ϣ�������û�  */
            iRet = SendDeviceOffLineAlarmToAllClientUser((char*)MasterDeviceIDVector[index].c_str());

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMark() SendDeviceOffLineAlarmToAllClientUser Error:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SetGBLogicDeviceInfoEnableFlagByDelMark() SendDeviceOffLineAlarmToAllClientUser OK:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }

            /* ���������߼��豸��λҵ��ֹͣ*/
            iRet = StopAllServiceTaskByLogicDeviceID((char*)MasterDeviceIDVector[index].c_str());

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMark() StopAllServiceTaskByLogicDeviceID Error:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SetGBLogicDeviceInfoEnableFlagByDelMark() StopAllServiceTaskByLogicDeviceID OK:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }

            /* ���������߼��豸��λ��Ƶ�Խ�ҵ��ֹͣ */
            iRet = StopAudioServiceTaskByLogicDeviceID((char*)MasterDeviceIDVector[index].c_str());

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMark() StopAudioServiceTaskByLogicDeviceID Error:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SetGBLogicDeviceInfoEnableFlagByDelMark() StopAudioServiceTaskByLogicDeviceID OK:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }

            /* ͬ�������ݿ� */
            iRet = AddGBLogicDeviceInfo2DB((char*)MasterDeviceIDVector[index].c_str(), pDevice_Srv_dboper);
        }
    }

    MasterDeviceIDVector.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : GBLogicDeviceCatalogInfoProc
 ��������  : �����豸�ϱ����߼�ͨ����Ϣ����
 �������  : GBDevice_info_t* pGBDeviceInfo
             GBLogicDevice_info_t* pNewGBLogicDeviceInfo
             GBLogicDevice_info_t* pOldGBLogicDeviceInfo
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��12��23�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ���������豸 */
    pMasterGBDevice = GBDevice_get_by_stream_type(pOldGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

    if (NULL != pMasterGBDevice) /* �Ѿ����ڣ��ȽϿ��Ƿ�һ�� */
    {
        pMasterGBDeviceInfo = pMasterGBDevice->ptGBDeviceInfo;

        if (NULL != pMasterGBDeviceInfo)
        {
            if (0 != sstrcmp(pMasterGBDeviceInfo->device_id, pGBDeviceInfo->device_id))
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ǰ���豸Ŀ¼��ѯ��Ӧ��Ϣ:�ϱ����߼��豸ID=%s, �߼���λ����=%s, ���߼��豸�����ϱ������������豸��һ�£����ܸ��߼���λID�����ظ�����,�ϵ������豸ID=%s, IP=%s, �µ������豸ID=%s, IP=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pMasterGBDeviceInfo->device_id, pMasterGBDeviceInfo->login_ip, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Front end device directory query response message:Logic device state change:device ID=%s, old state =%d, new state=%d; intelligent analysis old state=%d,new state=%d; alarm old state =%d,New state=%d", pNewGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status, pOldGBLogicDeviceInfo->intelligent_status, pNewGBLogicDeviceInfo->intelligent_status, pOldGBLogicDeviceInfo->alarm_status, pNewGBLogicDeviceInfo->alarm_status);

                pMasterGBDevice->ptGBDeviceInfo = pGBDeviceInfo;
            }
        }
        else
        {
            pMasterGBDevice->ptGBDeviceInfo = pGBDeviceInfo;
        }
    }
    else /* �����ڣ�ֱ����� */
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

    /* ״̬�仯��Ҫ֪ͨ���ͻ��� */
    if (pOldGBLogicDeviceInfo->status != pNewGBLogicDeviceInfo->status
        || pOldGBLogicDeviceInfo->intelligent_status != pNewGBLogicDeviceInfo->intelligent_status
        || pOldGBLogicDeviceInfo->alarm_status != pNewGBLogicDeviceInfo->alarm_status)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���豸Ŀ¼��ѯ��Ӧ��Ϣ, �߼��豸״̬�仯:�豸ID=%s, ��״̬=%d, ��״̬=%d; ���ܷ�����״̬=%d,��״̬=%d; ������״̬=%d,��״̬=%d", pNewGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status, pOldGBLogicDeviceInfo->intelligent_status, pNewGBLogicDeviceInfo->intelligent_status, pOldGBLogicDeviceInfo->alarm_status, pNewGBLogicDeviceInfo->alarm_status);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end device directory query response message:Reported logic device ID=%s, logic point name =%s, New disabled identification=%d, Old disable identification=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->enable, pOldGBLogicDeviceInfo->enable);

        if (1 == pNewGBLogicDeviceInfo->status && INTELLIGENT_STATUS_ON == pOldGBLogicDeviceInfo->intelligent_status)
        {
            /* �����豸״̬�仯��Ϣ */
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
            /* �����豸״̬�仯��Ϣ */
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
            /* �����豸״̬�仯��Ϣ */
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
            /* �����豸״̬�仯��Ϣ */
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

    if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type) /* �����CMS������ */
    {
        if (pOldGBLogicDeviceInfo->status == 1 && (pNewGBLogicDeviceInfo->status == 0 || pNewGBLogicDeviceInfo->status == 2))
        {
            if (pOldGBLogicDeviceInfo->status == 1 && pNewGBLogicDeviceInfo->status == 0)
            {
                /* ���͸澯��Ϣ���ͻ��� */
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
                /* ���͸澯��Ϣ���ͻ��� */
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
                /* ֹͣ��Ƶ�Խ�ҵ�� */
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
            /* ���͸澯��Ϣ���ͻ��� */
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
                /* ���͸澯��Ϣ���ͻ��� */
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
                /* ���͸澯��Ϣ���ͻ��� */
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

            if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_DECODER) /* ������ͨ����������IDֹͣҵ�� */
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
                    /* ֹͣ��Ƶ�Խ�ҵ�� */
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

    /* �鿴�豸��Ϣ�Ƿ��б仯 */
    change_flag = IsGBLogicDeviceInfoHasChange(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo, 0);

    if (1 == change_flag) /* �ڴ����Ƿ��б仯 */
    {
        change_flag_RCU = IsGBLogicDeviceInfoHasChangeForRCU(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo);

        /* �����ڴ� */
        i = GBLogicDevice_info_update(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo, 0);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() GBLogicDevice_info_update ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() GBLogicDevice_info_update OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }

        if (1 == change_flag_RCU) /* �ڴ���RCU�Ƿ��б仯 */
        {
            /* ����RCU�豸״̬��Ϣ���ͻ��� */
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

        /* ����Catalog�仯֪ͨ��Ϣ  */
        i = SendNotifyCatalogMessageToAllRoute(pOldGBLogicDeviceInfo, 2, pDevice_Srv_dboper);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "GBLogicDeviceCatalogInfoProc() SendNotifyCatalogMessageToAllRoute Error:iRet=%d \r\n", i);
        }
        else if (i > 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "GBLogicDeviceCatalogInfoProc() SendNotifyCatalogMessageToAllRoute OK:iRet=%d \r\n", i);
        }

        /* �������ݿ� */
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
    else if (2 == change_flag) /* ����״̬�����˱仯, �Ͳ���Ҫ�ڷ��ͱ仯Catalog�� */
    {
        /* �����ڴ� */
        i = GBLogicDevice_info_update(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo, 0);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProc() GBLogicDevice_info_update ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProc() GBLogicDevice_info_update OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }

        /* �������ݿ� */
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
        /* �����ݿ����Ƿ�����ǰ���ϱ�Ϊ׼�����ݷ����˸ı䣬��Ҫͬ�������ݿ� */
        i = load_GBLogicDevice_info_from_db_by_device_id(pDevice_Srv_dboper, pOldGBLogicDeviceInfo->device_id, &pDBGBLogicDeviceInfo);

        if (i == 0)
        {
            /* �������ݿ� */
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
            /* �����ݿ��е������Ƿ��б仯 */
            change_flag = IsGBLogicDeviceInfoHasChange(pDBGBLogicDeviceInfo, pOldGBLogicDeviceInfo, 0);

            if (1 == change_flag)
            {
                /* ���ڴ���µ����ݿ� */
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
 �� �� ��  : GBLogicDeviceCatalogInfoProcForRoute
 ��������  : �ϼ�CMS�����߼�ͨ����λ��Ϣ����
 �������  : GBLogicDevice_info_t* pNewGBLogicDeviceInfo
             GBLogicDevice_info_t* pOldGBLogicDeviceInfo
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��10��10��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ״̬�仯��Ҫ֪ͨ���ͻ��� */
    if (pOldGBLogicDeviceInfo->status != pNewGBLogicDeviceInfo->status
        || pOldGBLogicDeviceInfo->intelligent_status != pNewGBLogicDeviceInfo->intelligent_status
        || pOldGBLogicDeviceInfo->alarm_status != pNewGBLogicDeviceInfo->alarm_status)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMS���͵�λ��Ϣ, �߼��豸״̬�仯:�豸ID=%s, ��״̬=%d, ��״̬=%d; ���ܷ�����״̬=%d,��״̬=%d; ������״̬=%d,��״̬=%d", pNewGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status, pOldGBLogicDeviceInfo->intelligent_status, pNewGBLogicDeviceInfo->intelligent_status, pOldGBLogicDeviceInfo->alarm_status, pNewGBLogicDeviceInfo->alarm_status);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Superior CMS push point message:logic device status change:device ID=%s, old state=%d, new state=%d; intelligent analysis old state=%d,new state=%d; alarm old state=%d,new state=%d", pNewGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status, pOldGBLogicDeviceInfo->intelligent_status, pNewGBLogicDeviceInfo->intelligent_status, pOldGBLogicDeviceInfo->alarm_status, pNewGBLogicDeviceInfo->alarm_status);

        if (1 == pNewGBLogicDeviceInfo->status && INTELLIGENT_STATUS_ON == pOldGBLogicDeviceInfo->intelligent_status)
        {
            /* �����豸״̬��Ϣ���ͻ��� */
            i = SendDeviceStatusToAllClientUser(pNewGBLogicDeviceInfo->device_id, 4, pDevice_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 4, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 4, i);
            }

            /* �����豸״̬��Ϣ���¼�CMS  */
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
            /* �����豸״̬��Ϣ���ͻ��� */
            i = SendDeviceStatusToAllClientUser(pNewGBLogicDeviceInfo->device_id, 5, pDevice_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 5, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 5, i);
            }

            /* �����豸״̬��Ϣ���¼�CMS  */
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
            /* �����豸״̬��Ϣ���ͻ��� */
            i = SendDeviceStatusToAllClientUser(pNewGBLogicDeviceInfo->device_id, 6, pDevice_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 6, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, 6, i);
            }

            /* �����豸״̬��Ϣ���¼�CMS  */
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
            /* �����豸״̬��Ϣ���ͻ��� */
            i = SendDeviceStatusToAllClientUser(pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, pDevice_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser ERROR:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, pNewGBLogicDeviceInfo->status, i);
            }

            /* �����豸״̬��Ϣ���¼�CMS  */
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

    /* ���͸澯��Ϣ */
    if (pOldGBLogicDeviceInfo->status == 1
        && (pNewGBLogicDeviceInfo->status == 0 || pNewGBLogicDeviceInfo->status == 2))
    {
        if (pOldGBLogicDeviceInfo->status == 1 && pNewGBLogicDeviceInfo->status == 0)
        {
            /* ���͸澯��Ϣ���ͻ��� */
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
            /* ���͸澯��Ϣ���ͻ��� */
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
            /* ֹͣ��Ƶ�Խ�ҵ�� */
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
        /* ���͸澯��Ϣ���ͻ��� */
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

    /* �鿴�豸��Ϣ�Ƿ��б仯 */
    change_flag = IsGBLogicDeviceInfoHasChangeForRoute(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo, 0);

    if (1 == change_flag) /* �ڴ����Ƿ��б仯 */
    {
        change_flag_RCU = IsGBLogicDeviceInfoHasChangeForRCU(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo);

        /* �����ڴ� */
        i = GBLogicDevice_info_update_for_route(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo, 0);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() GBLogicDevice_info_update ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() GBLogicDevice_info_update OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }

        if (1 == change_flag_RCU) /* �ڴ���RCU�Ƿ��б仯 */
        {
            /* ����RCU�豸״̬��Ϣ���ͻ��� */
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

        /* ���͸�����Ϣ���¼�CMS  */
        i = SendNotifyCatalogToSubCMS(pOldGBLogicDeviceInfo, 2, pDevice_Srv_dboper);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "GBLogicDeviceCatalogInfoProcForRoute() SendNotifyCatalogToSubCMS Error:iRet=%d \r\n", i);
        }
        else if (i > 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "GBLogicDeviceCatalogInfoProcForRoute() SendNotifyCatalogToSubCMS OK:iRet=%d \r\n", i);
        }

        /* �������ݿ� */
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
    else if (2 == change_flag) /* ����״̬�����˱仯, �Ͳ���Ҫ�ڷ��ͱ仯Catalog�� */
    {
        /* �����ڴ� */
        i = GBLogicDevice_info_update_for_route(pOldGBLogicDeviceInfo, pNewGBLogicDeviceInfo, 0);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBLogicDeviceCatalogInfoProcForRoute() GBLogicDevice_info_update ERROR:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBLogicDeviceCatalogInfoProcForRoute() GBLogicDevice_info_update OK:device_id=%s, iRet=%d \r\n", pNewGBLogicDeviceInfo->device_id, i);
        }

        /* �������ݿ� */
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
        /* �����ݿ����Ƿ����û������˵ı仯������, ��Ҫ���µ��ڴ�*/
        i = load_GBLogicDevice_info_from_db_by_device_id(pDevice_Srv_dboper, pOldGBLogicDeviceInfo->device_id, &pDBGBLogicDeviceInfo);

        if (i == 0)
        {
            /* �������ݿ� */
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
            /* �����ݿ��е������Ƿ��б仯 */
            if (IsGBLogicDeviceInfoHasChangeForRoute(pOldGBLogicDeviceInfo, pDBGBLogicDeviceInfo, 0))
            {
                /* ���ڴ���µ����ݿ� */
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
 �� �� ��  : IntelligentAnalysisGBLogicDeviceCatalogInfoProc
 ��������  : ���ܷ����豸��Catalog����
 �������  : GBDevice_info_t * pGBDeviceInfo
             char* device_id
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��8��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �ɵ��߼��豸 */
    pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

    if (NULL == pGBLogicDeviceInfo)
    {
        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "ǰ���豸Ŀ¼��ѯ��Ӧ��Ϣ:�ϱ����߼��豸ID=%s, ǰ���豸��������Ϊ�����豸���ϱ��ķ�����λ�����Ѿ����ڵ��߼���λ", device_id);
        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Front-end device directory search response message:logic device reported ID=%s, front-end device is intelligent analysis device��analysis point reported is not existed logic device.", device_id);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "IntelligentAnalysisGBLogicDeviceCatalogInfoProc() exit---: Find GBLogicDevice Info Error \r\n");
        return -1;
    }

    if (0 == pGBLogicDeviceInfo->enable)
    {
        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "ǰ���豸Ŀ¼��ѯ��Ӧ��Ϣ:�ϱ����߼��豸ID=%s, ǰ���豸��������Ϊ�����豸���ϱ��ķ�����λ�Ѿ�������", device_id);
        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Front-end device directory search response message:logic device reported ID=%s, front-end device is intelligent analysis device��analysis point reported is disabled", device_id);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "IntelligentAnalysisGBLogicDeviceCatalogInfoProc() exit---: Find GBLogicDevice Info Error \r\n");
        return -1;
    }

    if (2 == pGBLogicDeviceInfo->del_mark)
    {
        /* �Ƴ�ɾ����ʶ */
        pGBLogicDeviceInfo->del_mark = 0;
    }

    if (INTELLIGENT_STATUS_NULL == pGBLogicDeviceInfo->intelligent_status)
    {
        pGBLogicDeviceInfo->intelligent_status = INTELLIGENT_STATUS_ON;

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ�����ܷ����豸Ŀ¼��ѯ��Ӧ��Ϣ:ǰ�������豸ID=%s, IP=%s:�ϱ����߼��豸ID=%s, ���ܷ���״̬=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->intelligent_status);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front-end intelligent analysis device directory search response message:front-end physical device ID=%s, IP=%s:logic device reported ID=%s, intelligent analysis status=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->intelligent_status);


        if (1 == pGBLogicDeviceInfo->status)
        {
            /* �����豸״̬�仯��Ϣ */
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

    /* �����������豸 */
    pIntelligentGBDevice = GBDevice_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_INTELLIGENCE);

    if (NULL != pIntelligentGBDevice) /* �Ѿ����ڣ��ȽϿ��Ƿ�һ�� */
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
    else /* �����ڣ�ֱ����� */
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
 �� �� ��  : CivilCodeGBLogicDeviceCatalogInfoProc
 ��������  : �¼��ϱ�������������֯����
 �������  : GBDevice_info_t * pGBDeviceInfo
             char* civil_code
             char* civil_name
             char* parent_id
             int iEvent
             DBOper* pDboper
             int iNeedToSync
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��9��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int CivilCodeGBLogicDeviceCatalogInfoProc(GBDevice_info_t * pGBDeviceInfo, char* civil_code, char* civil_name, char* parent_id, int iEvent, DBOper* pDboper, int iNeedToSync)
{
    int i = 0;
    int iCivilCodeLen = 0;
    int iParentIDLen = 0;
    string strGroupID = "";
    string strParentID = "";
    char strParentCodePrex[10] = {0}; /* ����ǰ׺  */

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

    if (2 == iCivilCodeLen) /* ʡ����֯ */
    {
        strGroupID = civil_code;
        strGroupID += "000000000000000000000000000000";

        /* �ϼ�ȫ0 */
        strParentID = "00000000000000000000000000000000";
    }
    else if (4 == iCivilCodeLen) /* �м���֯ */
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
    else if (6 == iCivilCodeLen) /* ������֯ */
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
    else if (8 == iCivilCodeLen) /* �ļ���֯ */
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
    else if (10 == iCivilCodeLen) /* �弶��֯ */
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
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ǰ���豸�ϱ�������������봦��:����������볤�Ȳ��Ϸ�:����=%d", iCivilCodeLen);
            return -1;
        }
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ǰ���豸�ϱ�������������봦��:ת����ķ���ID=%s, ��������=%s, �ϼ�����ID=%s", (char*)strGroupID.c_str(), civil_name, (char*)strParentID.c_str());

    i = DeviceGroupConfigInfoProc(pGBDeviceInfo, (char*)strGroupID.c_str(), civil_name, (char*)strParentID.c_str(), 0, iEvent, pDboper, iNeedToSync);

    return i;
}

/*****************************************************************************
 �� �� ��  : GroupCodeGBLogicDeviceCatalogInfoProc
 ��������  : �¼��ϱ���ҵ�����������֯����
 �������  : GBDevice_info_t * pGBDeviceInfo
             char* group_id
             char* group_name
             char* parent_id
             int iEvent
             DBOper* pDboper
             int iNeedToSync
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��9��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GroupCodeGBLogicDeviceCatalogInfoProc(GBDevice_info_t * pGBDeviceInfo, char* group_id, char* group_name, char* parent_id, int iEvent, DBOper* pDboper, int iNeedToSync)
{
    int i = 0;
    int iParentIDLen = 0;
    string strGroupID = "";
    string strParentID = "";
    char strGroupIDSuffix[8] = {0};  /* ����ID��׺,ȡ����ĺ�6λ */
    char strGroupIDPrex[12] = {0};   /* ����IDǰ׺,ȡ�����ǰ8λ  */
    char strParentIDSuffix[8] = {0}; /* ������ID��׺,ȡ����ĺ�6λ */
    char strParentIDPrex[12] = {0};  /* ����IDǰ׺,ȡ�����ǰ8λ  */

    if (NULL == pGBDeviceInfo || NULL == group_id || NULL == parent_id || NULL == pDboper)
    {
        return -1;
    }

    if (group_id[0] == '\0')
    {
        return -1;
    }

    /* ȡ������ĺ�6λ */
    osip_strncpy(strGroupIDSuffix, &group_id[14], 6);

    /* ȡ�������ǰ8λ */
    osip_strncpy(strGroupIDPrex, &group_id[0], 8);

    iParentIDLen = strlen(parent_id);

    if (2 == iParentIDLen) /* ʡ����֯ */
    {
        /* ������� */
        strGroupID = parent_id;
        strGroupID += "000000";
        strGroupID += strGroupIDSuffix;
        strGroupID += "000000000000000000";

        /* �ϼ�Ϊʡ�� */
        strParentID = parent_id;
        strParentID += "000000000000000000000000000000";
    }
    else if (4 == iParentIDLen) /* �м���֯ */
    {
        /* ������� */
        strGroupID = parent_id;
        strGroupID += "0000";
        strGroupID += strGroupIDSuffix;
        strGroupID += "000000000000000000";

        /* �ϼ�Ϊ�м� */
        strParentID = parent_id;
        strParentID += "0000000000000000000000000000";
    }
    else if (6 == iParentIDLen) /* ������֯ */
    {
        /* ������� */
        strGroupID = parent_id;
        strGroupID += "00";
        strGroupID += strGroupIDSuffix;
        strGroupID += "000000000000000000";

        /* �ϼ�Ϊ���� */
        strParentID = parent_id;
        strParentID += "00000000000000000000000000";
    }
    else if (8 == iParentIDLen) /* �ļ���֯ */
    {
        /* ������� */
        strGroupID = parent_id;
        strGroupID += strGroupIDSuffix;
        strGroupID += "000000000000000000";

        /* �ϼ�Ϊ�ɳ��� */
        strParentID = parent_id;
        strParentID += "000000000000000000000000";
    }
    else if (10 == iParentIDLen) /* �弶��֯ */
    {
        /* ������� */
        strGroupID = parent_id;
        strGroupID += strGroupIDSuffix;
        strGroupID += "0000000000000000";

        /* �ϼ�Ϊ�弶��֯ */
        strParentID = parent_id;
        strParentID += "0000000000000000000000";
    }
    else if (20 == iParentIDLen) /* ҵ����� */
    {
        /* ������� */
        strGroupID = strGroupIDPrex;
        strGroupID += strGroupIDSuffix;
        strGroupID += "000000000000000000";

        /* �ϼ�������� */
        /* ȡ������ĺ�6λ */
        osip_strncpy(strParentIDSuffix, &parent_id[14], 6);
        /* ȡ�������ǰ8λ */
        osip_strncpy(strParentIDPrex, &parent_id[0], 8);

        strParentID = strParentIDPrex;
        strParentID += strParentIDSuffix;
        strParentID += "000000000000000000";
    }
    else
    {
        /* ������� */
        strGroupID = strGroupIDPrex;
        strGroupID += strGroupIDSuffix;
        strGroupID += "000000000000000000";

        if (1 == iEvent)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ǰ���豸�ϱ���ҵ�������봦��:ҵ�������볤�Ȳ��Ϸ�:����=%d", iParentIDLen);
            return -1;
        }
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ǰ���豸�ϱ���ҵ�������봦��:ת����ķ���ID=%s, ��������=%s, �ϼ�����ID=%s", (char*)strGroupID.c_str(), group_name, (char*)strParentID.c_str());

    i = DeviceGroupConfigInfoProc(pGBDeviceInfo, (char*)strGroupID.c_str(), group_name, (char*)strParentID.c_str(), 0, iEvent, pDboper, iNeedToSync);

    return i;
}

/*****************************************************************************
 �� �� ��  : DeviceGroupConfigInfoProc
 ��������  : �¼��ϱ��ķ�����Ϣ����
 �������  : GBDevice_info_t * pGBDeviceInfo
             char* group_id
             char* group_name
             char* parent_id
             int sort_id
             int iEvent
             DBOper* pDboper
             int iNeedToSync
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��9��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �����߼��豸������Ϣ�����Ƿ���� */
    pLogicDeviceGroup = GetLogicDeviceGroup(pGBDeviceInfo, group_id);

    if (1 == iEvent) /* ��ӻ��߸��� */
    {
        if (NULL == pLogicDeviceGroup) /* �����ڣ���� */
        {
            /* ���뵽���� */
            i = AddLogicDeviceGroup(pGBDeviceInfo, group_id, pGBDeviceInfo->device_id, group_name, sort_id, parent_id);

            if (i >= 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ǰ���豸�ϱ����߼��豸������Ϣ����:����ID=%s, ��������=%s, �ϼ�����ID=%s, ��ӵ���������ɹ�", group_id, group_name, parent_id);
                iChangeFlag = 1;
            }
        }
        else /* ���ڣ����Ƿ��б仯 */
        {
            i = ModifyLogicDeviceGroup(pLogicDeviceGroup, group_name, sort_id, parent_id);

            if (i == 1)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ǰ���豸�ϱ����߼��豸������Ϣ�仯����:����ID=%s, ��������=%s, �ϼ�����ID=%s, �޸ķ�����Ϣ�ɹ�", group_id, group_name, parent_id);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "DeviceGroupConfigInfoProc() GroupID=%s, Name=%s, CMSID=%s, ModifyLogicDeviceGroup:i=%d \r\n", group_id, group_name, pGBDeviceInfo->device_id, i);
                iChangeFlag = 1;
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ǰ���豸�ϱ����߼��豸������Ϣû�б仯:����ID=%s, ��������=%s, �ϼ�����ID=%s", group_id, group_name, parent_id);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "DeviceGroupConfigInfoProc() GroupID=%s, Name=%s, CMSID=%s, No Change \r\n", group_id, group_name, pGBDeviceInfo->device_id);
            }
        }
    }
    else if (2 == iEvent) /* ɾ�� */
    {
        if (NULL == pLogicDeviceGroup) /* ������ */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���豸�ϱ����߼��豸������Ϣɾ������:����ID=%s, ��������=%s, �ϼ�����ID=%s, ������Ϣ������", group_id, group_name, parent_id);
        }
        else /* ���ڣ����Ƿ��б仯 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���豸�ϱ����߼��豸������Ϣɾ��:����ID=%s, ��������=%s, �ϼ�����ID=%s", group_id, group_name, parent_id);
            pLogicDeviceGroup->iChangeFlag = 3;
            iChangeFlag = 1;
        }
    }

    if (1 == iNeedToSync && 1 == iChangeFlag) /* �Ƿ���Ҫͬ�������ݿ� */
    {
        /* ���仯ͬ�������ݿ� */
        i = SynLogicDeviceGroupInfoToDB2(pLogicDeviceGroup, pDboper);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "DeviceGroupConfigInfoProc() SynLogicDeviceGroupInfoToDB2:i=%d \r\n", i);

        if (2 == iEvent) /* ɾ�� */
        {
            /* ɾ���ڴ��ж������Ϣ */
            i = DelLogicDeviceGroupInfo2(pGBDeviceInfo, pLogicDeviceGroup);
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "DeviceGroupConfigInfoProc() DelLogicDeviceGroupInfo2:i=%d \r\n", i);
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : DeviceGroupMapConfigInfoProc
 ��������  : �¼��ϱ��ķ����ϵ��Ϣ����
 �������  : GBDevice_info_t* pGBDeviceInfo
             char* group_id
             unsigned int device_index
             int sort_id
             int iEvent
             DBOper* pDboper
             int iNeedToSync
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    if (1 == iEvent) /* ��ӻ��߸��� */
    {
        /* �����߼��豸�����ϵ��Ϣ�����Ƿ���� */
        pLogicDeviceMapGroup = GetLogicDeviceMapGroup(pGBDeviceInfo, device_index);

        if (NULL == pLogicDeviceMapGroup) /* �����ڣ���� */
        {
            /* ���뵽���� */
            i = AddLogicDeviceMapGroup(pGBDeviceInfo, group_id, device_index, pGBDeviceInfo->device_id, sort_id);

            if (i >= 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ǰ���豸�ϱ����߼��豸�����ϵ��Ϣ����:����ID=%s, �߼��豸����=%u, ��ӵ����������ϵ�ɹ�", group_id, device_index);
                iChangeFlag = 1;
            }
        }
        else /* ���ڣ����Ƿ��б仯 */
        {
            i = ModifyLogicDeviceMapGroup(pLogicDeviceMapGroup, group_id, pGBDeviceInfo->device_id, sort_id);

            if (i == 1)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ǰ���豸�ϱ����߼��豸�����ϵ��Ϣ�仯����:����ID=%s, �߼��豸����=%u, �޸ķ����ϵ��Ϣ�ɹ�", group_id, device_index);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "DeviceGroupMapConfigInfoProc() GroupID=%s, device_index=%u, ModifyLogicDeviceGroup:i=%d \r\n", group_id, device_index, i);
                iChangeFlag = 1;
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ǰ���豸�ϱ����߼��豸�����ϵ��Ϣû�б仯:����ID=%s, �߼��豸����=%u", group_id, device_index);
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO,  "DeviceGroupMapConfigInfoProc() GroupID=%s, device_index=%u, No Change \r\n", group_id, device_index);
            }
        }
    }
    else if (2 == iEvent) /* ɾ�� */
    {
        /* �����߼��豸�����ϵ��Ϣ�����Ƿ���� */
        pLogicDeviceMapGroup = GetLogicDeviceMapGroup(pGBDeviceInfo, device_index);

        if (NULL == pLogicDeviceMapGroup) /* ������ */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���豸�ϱ����߼��豸�����ϵ��Ϣɾ������:����ID=%s, �߼��豸����=%u, �����ϵ��Ϣ������", group_id, device_index);
        }
        else /* ���ڣ����Ƿ��б仯 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���豸�ϱ����߼��豸�����ϵ��Ϣɾ��:����ID=%s, �߼��豸����=%u", group_id, device_index);
            pLogicDeviceMapGroup->iChangeFlag = 3;
            iChangeFlag = 1;
        }
    }

    if (1 == iNeedToSync && 1 == iChangeFlag) /* �Ƿ���Ҫͬ�������ݿ� */
    {
        /* ���仯ͬ�������ݿ� */
        i = SynLogicDeviceMapGroupInfoToDB2(pLogicDeviceMapGroup, pDboper);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "DeviceGroupMapConfigInfoProc() SynLogicDeviceMapGroupInfoToDB2:i=%d \r\n", i);

        if (2 == iEvent)
        {
            /* ɾ���ڴ��ж������Ϣ */
            i = DelLogicDeviceMapGroupInfo2(pGBDeviceInfo, pLogicDeviceMapGroup);
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "DeviceGroupMapConfigInfoProc() DelLogicDeviceMapGroupInfo2:i=%d \r\n", i);
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : set_GBLogicDevice_info_list_del_mark
 ��������  : �����߼��豸��Ϣɾ����ʶ
 �������  : int del_mark
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��7��23�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : delete_GBLogicDevice_info_from_list_by_mark
 ��������  : ����ɾ����ʶ��ɾ���߼��豸��Ϣ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��7��23�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ɾ����ʶ, ɾ����������߼��豸��λ��Ϣ: ɾ�����߼���λ����=%d", (int)DeviceIDVector.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "According to delete mark, delete the redundant standard physical device information: Delete logic point total = %d", (int)DeviceIDVector.size());

    if (DeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)DeviceIDVector.size(); index++)
        {
            pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

            if (NULL != pGBLogicDeviceInfo)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ɾ����ʶ, ɾ����������߼��豸��λ��Ϣ�ɹ�:�߼��豸ID=%s, ��λ����=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);
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
 �� �� ��  : set_all_vms_nvr_GBLogicDevice_info_enable_mark
 ��������  : ����VMS
 �������  : char* device_id
             int enable_mark
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��4��21��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : AddCivilCodeToGBLogicDeviceInfo
 ��������  : ����߼���λ������������Ϣ
 �������  : DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��9��6��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

        /* ������ĵ�λ�Ͳ���������֯������ */
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

            /* ������ĵ�λ�Ͳ���������֯������ */
            if (pGBLogicDeviceInfo->other_realm == 1)
            {
                continue;
            }

            pPrimaryGroup = GetPrimaryGroupInfoByGBLogicDeviceIndex(pGBLogicDeviceInfo->id, pDevice_Srv_dboper);

            /* ��֯���� */
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

            /* ������֯���� */
            if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
            {
                if (NULL != pPrimaryGroup)
                {
                    if (pPrimaryGroup->group_code[0] != '\0')
                    {
                        if (0 == sstrcmp(pPrimaryGroup->group_code, pPrimaryGroup->civil_code)) /* group_code �� civil_codeһ������ʾֱ�ӹ��������������棬û��ҵ����� */
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
                        if (0 == sstrcmp(pPrimaryGroup->group_code, pPrimaryGroup->civil_code)) /* group_code �� civil_codeһ������ʾֱ�ӹ��������������棬û��ҵ����� */
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

            if (1 == iChangeFlag)/* ���͵�λ��Ϣ�仯���ϼ�CMS */
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
                    /* ���ͱ仯֪ͨ������ƽ̨ */
                    i = SendNotifyGroupMapCatalogTo3PartyRouteCMS(pGBLogicDeviceInfo, 2, pDevice_Srv_dboper);

                    if (i > 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�߼���λ�����ϵ�����仯, �����߼��豸�仯֪ͨ���ϼ�ƽ̨:�߼��豸ID=%s", pGBLogicDeviceInfo->device_id);
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
 �� �� ��  : GBLogicDeviceCatalogGroupMapProc
 ��������  : �߼���λ�����ϵ����
 �������  : GBDevice_info_t * pGBDeviceInfo
             GBLogicDevice_info_t* pGBLogicDeviceInfo
             char* parent_id
             DBOper* pDevice_Srv_dboper
             int iNeedToSync
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��15��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GBLogicDeviceCatalogGroupMapProc(GBDevice_info_t * pGBDeviceInfo, GBLogicDevice_info_t* pGBLogicDeviceInfo, char* parent_id, DBOper* pDevice_Srv_dboper, int iNeedToSync)
{
    int i = 0;
    int iParentIDLen = 0;
    string strParentID = "";
    char strParentIDSuffix[8] = {0}; /* ������ID��׺,ȡ����ĺ�6λ */
    char strParentIDPrex[12] = {0};  /* ������IDǰ׺,ȡ�����ǰ8λ  */

    if (NULL == pGBDeviceInfo || NULL == pGBLogicDeviceInfo || NULL == parent_id || NULL == pDevice_Srv_dboper)
    {
        return -1;
    }

    if (parent_id[0] == '\0') /* ������ڵ��ǿգ����ʾû�з���,��ʶɾ��  */
    {
        i = DeviceGroupMapConfigInfoProc(pGBDeviceInfo, parent_id, pGBLogicDeviceInfo->id, 0, 2, pDevice_Srv_dboper, iNeedToSync);
    }
    else
    {
        iParentIDLen = strlen(parent_id);

        if (2 == iParentIDLen) /* ʡ����֯ */
        {
            /* �ϼ�Ϊʡ�� */
            strParentID = parent_id;
            strParentID += "000000000000000000000000000000";
        }
        else if (4 == iParentIDLen) /* �м���֯ */
        {
            /* �ϼ�Ϊ�м� */
            strParentID = parent_id;
            strParentID += "0000000000000000000000000000";
        }
        else if (6 == iParentIDLen) /* ������֯ */
        {
            /* �ϼ�Ϊ���� */
            strParentID = parent_id;
            strParentID += "00000000000000000000000000";
        }
        else if (8 == iParentIDLen) /* �ļ���֯ */
        {
            /* �ϼ�Ϊ�ɳ��� */
            strParentID = parent_id;
            strParentID += "000000000000000000000000";
        }
        else if (10 == iParentIDLen) /* �弶��֯ */
        {
            /* �ϼ�Ϊ�弶��֯ */
            strParentID = parent_id;
            strParentID += "0000000000000000000000";
        }
        else if (20 == iParentIDLen) /* ҵ����� */
        {
            /* �ϼ�������� */
            /* ȡ������ĺ�6λ */
            osip_strncpy(strParentIDSuffix, &parent_id[14], 6);
            /* ȡ�������ǰ8λ */
            osip_strncpy(strParentIDPrex, &parent_id[0], 8);

            strParentID = strParentIDPrex;
            strParentID += strParentIDSuffix;
            strParentID += "000000000000000000";
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ǰ���豸�ϱ��ķ����ϵ����:��λ�ĸ��ڵ�ҵ�������볤�Ȳ��Ϸ�:����=%d", iParentIDLen);
            return -1;
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ǰ���豸�ϱ��ķ����ϵ����:ת����ĸ��ڵ����ID=%s,��λID=%s", (char*)strParentID.c_str(), pGBLogicDeviceInfo->device_id);

        /* ��ԭ�е�ɾ���� */
        i = DeviceGroupMapConfigInfoProc(pGBDeviceInfo, (char*)strParentID.c_str(), pGBLogicDeviceInfo->id, 0, 2, pDevice_Srv_dboper, 0);

        /* �����µ� */
        i = DeviceGroupMapConfigInfoProc(pGBDeviceInfo, (char*)strParentID.c_str(), pGBLogicDeviceInfo->id, 0, 1, pDevice_Srv_dboper, iNeedToSync);
    }

    return i;
}
#endif

/*****************************************************************************
 �� �� ��  : load_db_data_to_LogicDevice_info_list_by_device_id
 ��������  : �����߼��豸DeviceID�����ݿ��м����߼��豸��Ϣ
 �������  : DBOper * ptDBOper
             char* device_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��5��25��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �豸����*/
    tmp_uivalue = 0;
    ptDBOper->GetFieldValue("ID", tmp_uivalue);

    pGBLogicDeviceInfo->id = tmp_uivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->id:%u", pGBLogicDeviceInfo->id);


    /* ��λͳһ��� */
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

    /* ������CMS ͳһ��� */
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


    /* ��λ���� */
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


    /* �Ƿ����� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Enable", tmp_ivalue);

    pGBLogicDeviceInfo->enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->enable:%d", pGBLogicDeviceInfo->enable);


    /* �豸����*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("DeviceType", tmp_ivalue);

    pGBLogicDeviceInfo->device_type = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->device_type:%d", pGBLogicDeviceInfo->device_type);


    /* �����豸������ */
    tmp_uivalue = 0;
    ptDBOper->GetFieldValue("Resved1", tmp_uivalue);

    pGBLogicDeviceInfo->alarm_device_sub_type = tmp_uivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->alarm_device_sub_type:%u", pGBLogicDeviceInfo->alarm_device_sub_type);


    /* �Ƿ�ɿ�*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("CtrlEnable", tmp_ivalue);

    pGBLogicDeviceInfo->ctrl_enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->ctrl_enable:%d", pGBLogicDeviceInfo->ctrl_enable);


    /* �Ƿ�֧�ֶԽ���Ĭ��ֵ0 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("MicEnable", tmp_ivalue);

    pGBLogicDeviceInfo->mic_enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->mic_enable:%d", pGBLogicDeviceInfo->mic_enable);


    /* ֡�ʣ�Ĭ��25 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("FrameCount", tmp_ivalue);

    pGBLogicDeviceInfo->frame_count = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->frame_count:%d", pGBLogicDeviceInfo->frame_count);


    /* �澯ʱ�� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("AlarmLengthOfTime", tmp_ivalue);

    pGBLogicDeviceInfo->alarm_duration = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->alarm_duration:%d", pGBLogicDeviceInfo->alarm_duration);


    /* ��Ӧ��ý�������豸����*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("PhyDeviceIndex", tmp_ivalue);

    pGBLogicDeviceInfo->phy_mediaDeviceIndex = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->phy_mediaDeviceIndex:%d", pGBLogicDeviceInfo->phy_mediaDeviceIndex);


    /* ��Ӧ��ý�������豸ͨ�� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("PhyDeviceChannel", tmp_ivalue);

    pGBLogicDeviceInfo->phy_mediaDeviceChannel = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->phy_mediaDeviceChannel:%d", pGBLogicDeviceInfo->phy_mediaDeviceChannel);


    /* ��Ӧ�Ŀ��������豸���� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("CtrlDeviceIndex", tmp_ivalue);

    pGBLogicDeviceInfo->phy_ctrlDeviceIndex = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->phy_ctrlDeviceIndex:%d", pGBLogicDeviceInfo->phy_ctrlDeviceIndex);


    /* ��Ӧ�Ŀ��������豸ͨ�� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("CtrlDeviceChannel", tmp_ivalue);

    pGBLogicDeviceInfo->phy_ctrlDeviceChannel = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->phy_ctrlDeviceChannel:%d", pGBLogicDeviceInfo->phy_ctrlDeviceChannel);


    /* �Ƿ�֧�ֶ����� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("StreamCount", tmp_ivalue);

    pGBLogicDeviceInfo->stream_count = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->stream_count:%d", pGBLogicDeviceInfo->stream_count);


    /* ¼������ */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("RecordType", tmp_ivalue);

    pGBLogicDeviceInfo->record_type = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->record_type:%d", pGBLogicDeviceInfo->record_type);


    /* �豸������ */
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


    /* �豸�ͺ� */
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


    /* �豸���� */
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
    /* �������� */
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

    /* ���� */
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


    /* ��װ��ַ */
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


    /* �Ƿ������豸 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Parental", tmp_ivalue);

    pGBLogicDeviceInfo->parental = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->parental:%d", pGBLogicDeviceInfo->parental);


    /* ���豸/����/ϵͳID */
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


    /* ���ȫģʽ*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("SafetyWay", tmp_ivalue);

    pGBLogicDeviceInfo->safety_way = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->safety_way:%d", pGBLogicDeviceInfo->safety_way);


    /* ע�᷽ʽ */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("RegisterWay", tmp_ivalue);

    pGBLogicDeviceInfo->register_way = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->safety_way:%d", pGBLogicDeviceInfo->register_way);


    /* ֤�����к�*/
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


    /* ֤����Ч��ʶ */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Certifiable", tmp_ivalue);

    pGBLogicDeviceInfo->certifiable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->certifiable:%d", pGBLogicDeviceInfo->certifiable);


    /* ��Чԭ���� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("ErrCode", tmp_ivalue);

    pGBLogicDeviceInfo->error_code = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->error_code:%d", pGBLogicDeviceInfo->error_code);


    /* ֤����ֹ��Ч��*/
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


    /* �������� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Secrecy", tmp_ivalue);

    pGBLogicDeviceInfo->secrecy = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->error_code:%d", pGBLogicDeviceInfo->secrecy);


    /* IP��ַ*/
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


    /* �˿ں� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Port", tmp_ivalue);

    pGBLogicDeviceInfo->port = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->port:%d", pGBLogicDeviceInfo->port);


    /* ����*/
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


    /* ״̬ */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Status", tmp_ivalue);

    pGBLogicDeviceInfo->status = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->status:%d", pGBLogicDeviceInfo->status);


    /* ���� */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Longitude", tmp_svalue);

    pGBLogicDeviceInfo->longitude = strtod(tmp_svalue.c_str(), (char**) NULL);
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id() pGBLogicDeviceInfo->longitude:%f", pGBLogicDeviceInfo->longitude);


    /* γ�� */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Latitude", tmp_svalue);

    pGBLogicDeviceInfo->latitude = strtod(tmp_svalue.c_str(), (char**) NULL);
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_db_data_to_LogicDevice_info_list_by_device_id:pGBLogicDeviceInfo->latitude : %f \r\n", pGBLogicDeviceInfo->latitude);

    /* ������ͼ�� */
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

    /* ӥ�������Ӧ��Ԥ��ID */
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

    /* ���������豸��Ϣ */
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

    /* �����߼��豸���� */
    if (GBLogicDevice_info_add(pGBLogicDeviceInfo) < 0)
    {
        GBLogicDevice_info_free(pGBLogicDeviceInfo);
        osip_free(pGBLogicDeviceInfo);
        pGBLogicDeviceInfo = NULL;
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "load_db_data_to_LogicDevice_info_list_by_device_id() GBLogicDevice Info Add Error");
        return -1;
    }

    /* ���ͱ仯���ϼ�Route */
    iRet = SendNotifyCatalogToRouteCMS(pGBLogicDeviceInfo, 0, ptDBOper);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "load_db_data_to_LogicDevice_info_list_by_device_id() SendNotifyCatalogToRouteCMS Error:device_id=%s, i=%d \r\n", pGBLogicDeviceInfo->device_id, iRet);
    }
    else if (iRet > 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "load_db_data_to_LogicDevice_info_list_by_device_id() SendNotifyCatalogToRouteCMS OK:device_id=%s, i=%d \r\n", pGBLogicDeviceInfo->device_id, iRet);
    }

    /* ���������Ϣ���¼�CMS  */
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
 �� �� ��  : load_GBLogicDevice_info_from_db_by_device_id
 ��������  : ͨ��DeviceID��ȡ���ݿ���߼��豸��Ϣ
 �������  : DBOper * ptDBOper
             char* device_id
             GBLogicDevice_info_t** pGBLogicDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��5��26��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �豸����*/
    tmp_uivalue = 0;
    ptDBOper->GetFieldValue("ID", tmp_uivalue);

    (*pGBLogicDeviceInfo)->id = tmp_uivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->id:%u", pGBLogicDeviceInfo->id);


    /* ��λͳһ��� */
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


    /* ������CMS ͳһ��� */
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


    /* ��λ���� */
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


    /* �Ƿ����� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Enable", tmp_ivalue);

    (*pGBLogicDeviceInfo)->enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->enable:%d", pGBLogicDeviceInfo->enable);


    /* �豸����*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("DeviceType", tmp_ivalue);

    (*pGBLogicDeviceInfo)->device_type = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->device_type:%d", pGBLogicDeviceInfo->device_type);


    /* �����豸������ */
    tmp_uivalue = 0;
    ptDBOper->GetFieldValue("Resved1", tmp_uivalue);

    (*pGBLogicDeviceInfo)->alarm_device_sub_type = tmp_uivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->alarm_device_sub_type:%u", pGBLogicDeviceInfo->alarm_device_sub_type);


    /* �Ƿ�ɿ�*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("CtrlEnable", tmp_ivalue);

    (*pGBLogicDeviceInfo)->ctrl_enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->ctrl_enable:%d", pGBLogicDeviceInfo->ctrl_enable);


    /* �Ƿ�֧�ֶԽ���Ĭ��ֵ0 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("MicEnable", tmp_ivalue);

    (*pGBLogicDeviceInfo)->mic_enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->mic_enable:%d", pGBLogicDeviceInfo->mic_enable);


    /* ֡�ʣ�Ĭ��25 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("FrameCount", tmp_ivalue);

    (*pGBLogicDeviceInfo)->frame_count = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->frame_count:%d", pGBLogicDeviceInfo->frame_count);


    /* �澯ʱ�� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("AlarmLengthOfTime", tmp_ivalue);

    (*pGBLogicDeviceInfo)->alarm_duration = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->alarm_duration:%d", pGBLogicDeviceInfo->alarm_duration);


    /* ��Ӧ��ý�������豸����*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("PhyDeviceIndex", tmp_ivalue);

    (*pGBLogicDeviceInfo)->phy_mediaDeviceIndex = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->phy_mediaDeviceIndex:%d", pGBLogicDeviceInfo->phy_mediaDeviceIndex);


    /* ��Ӧ��ý�������豸ͨ�� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("PhyDeviceChannel", tmp_ivalue);

    (*pGBLogicDeviceInfo)->phy_mediaDeviceChannel = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->phy_mediaDeviceChannel:%d", pGBLogicDeviceInfo->phy_mediaDeviceChannel);


    /* ��Ӧ�Ŀ��������豸���� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("CtrlDeviceIndex", tmp_ivalue);

    (*pGBLogicDeviceInfo)->phy_ctrlDeviceIndex = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->phy_ctrlDeviceIndex:%d", pGBLogicDeviceInfo->phy_ctrlDeviceIndex);


    /* ��Ӧ�Ŀ��������豸ͨ�� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("CtrlDeviceChannel", tmp_ivalue);

    (*pGBLogicDeviceInfo)->phy_ctrlDeviceChannel = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->phy_ctrlDeviceChannel:%d", pGBLogicDeviceInfo->phy_ctrlDeviceChannel);


    /* �Ƿ�֧�ֶ����� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("StreamCount", tmp_ivalue);

    (*pGBLogicDeviceInfo)->stream_count = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->stream_count:%d", pGBLogicDeviceInfo->stream_count);


    /* ¼������ */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("RecordType", tmp_ivalue);

    (*pGBLogicDeviceInfo)->record_type = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->record_type:%d", pGBLogicDeviceInfo->record_type);


    /* �豸������ */
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


    /* �豸�ͺ� */
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


    /* �豸���� */
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
    /* �������� */
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

    /* ���� */
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


    /* ��װ��ַ */
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


    /* �Ƿ������豸 */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Parental", tmp_ivalue);

    (*pGBLogicDeviceInfo)->parental = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->parental:%d", pGBLogicDeviceInfo->parental);


    /* ���豸/����/ϵͳID */
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


    /* ���ȫģʽ*/
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("SafetyWay", tmp_ivalue);

    (*pGBLogicDeviceInfo)->safety_way = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->safety_way:%d", pGBLogicDeviceInfo->safety_way);


    /* ע�᷽ʽ */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("RegisterWay", tmp_ivalue);

    (*pGBLogicDeviceInfo)->register_way = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->safety_way:%d", pGBLogicDeviceInfo->register_way);


    /* ֤�����к�*/
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


    /* ֤����Ч��ʶ */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Certifiable", tmp_ivalue);

    (*pGBLogicDeviceInfo)->certifiable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->certifiable:%d", pGBLogicDeviceInfo->certifiable);


    /* ��Чԭ���� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("ErrCode", tmp_ivalue);

    (*pGBLogicDeviceInfo)->error_code = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->error_code:%d", pGBLogicDeviceInfo->error_code);


    /* ֤����ֹ��Ч��*/
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


    /* �������� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Secrecy", tmp_ivalue);

    (*pGBLogicDeviceInfo)->secrecy = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->error_code:%d", pGBLogicDeviceInfo->secrecy);


    /* IP��ַ*/
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


    /* �˿ں� */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Port", tmp_ivalue);

    (*pGBLogicDeviceInfo)->port = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->port:%d", pGBLogicDeviceInfo->port);


    /* ����*/
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


    /* ״̬ */
    tmp_ivalue = 0;
    ptDBOper->GetFieldValue("Status", tmp_ivalue);

    (*pGBLogicDeviceInfo)->status = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->status:%d", pGBLogicDeviceInfo->status);


    /* ���� */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Longitude", tmp_svalue);

    (*pGBLogicDeviceInfo)->longitude = strtod(tmp_svalue.c_str(), (char**) NULL);
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id() pGBLogicDeviceInfo->longitude:%f", pGBLogicDeviceInfo->longitude);


    /* γ�� */
    tmp_svalue.clear();
    ptDBOper->GetFieldValue("Latitude", tmp_svalue);

    (*pGBLogicDeviceInfo)->latitude = strtod(tmp_svalue.c_str(), (char**) NULL);
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "load_GBLogicDevice_info_from_db_by_device_id:pGBLogicDeviceInfo->latitude : %f \r\n", pGBLogicDeviceInfo->latitude);

    /* ������ͼ�� */
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

    /* ӥ�������Ӧ��Ԥ��ID */
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

    /* ���������豸��Ϣ */
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
 �� �� ��  : check_GBLogicDevice_info_from_db
 ��������  : ����߼��豸���ݿ��е��������ݿ��Ƿ��б仯������б仯����ͬ��
             ���ڴ���
 �������  : DBOper* pdboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��7��23�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_GBLogicDevice_info_from_db_to_list(DBOper* pdboper)
{
    //int i = 0;
    int iRet = 0;
    int index = 0;
    int record_count = 0; /* ��¼�� */
    int change_flag = 0;

    vector<string> DeviceIDVector;

    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_info_t* pDBGBLogicDeviceInfo = NULL;

    DeviceIDVector.clear();

    /* 1��������е��߼��豸 */
    iRet = AddAllGBLogicDeviceIDToVector(DeviceIDVector, pdboper);

    /* 2����ȡ�����е��豸���� */
    record_count = DeviceIDVector.size();

    printf("check_GBLogicDevice_info_from_db() DB Record:record_count=%d \r\n", record_count);

    /* 3�������¼��Ϊ0 */
    if (record_count == 0)
    {
        return record_count;
    }

    for (index = 0; index < record_count; index++)
    {
        /* ����Index ��ȡ�߼��豸��Ϣ��������ֻ�����������豸����û�����ߣ����ݿ���ڴ��ж�û�е� */
        pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

        if (NULL != pGBLogicDeviceInfo)
        {
            pGBLogicDeviceInfo->del_mark = 0;

            /* �������ݿ����б仯����Ҫ���µ��ڴ� */
            iRet = load_GBLogicDevice_info_from_db_by_device_id(pdboper, (char*)DeviceIDVector[index].c_str(), &pDBGBLogicDeviceInfo);

            if (iRet > 0)
            {
                change_flag = IsGBLogicDeviceInfoHasChange(pGBLogicDeviceInfo, pDBGBLogicDeviceInfo, 1);

                if (change_flag) /* �����ݿ����Ƿ��б仯 */
                {
                    /* �����ڴ� */
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
                        /* ���ͱ仯���ϼ�Route */
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
                            /* ���ͱ仯���ϼ�Route */
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

    /* ��ȡ�Ϻ��ر��Ӧ��ͨ����Ϣ */
    iRet = check_shdb_channel_info_from_db(pdboper);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : check_shdb_channel_info_from_db
 ��������  : �����Ϻ��ر��Ӧ��ͨ��ID���߼��豸
 �������  : DBOper * pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��5��23��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

        /* ͨ������ */
        iChannelID = 0;
        pDboper->GetFieldValue("ChannelID", iChannelID);

        /* �豸���� */
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
 �� �� ��  : SetGBLogicDeviceStatus
 ��������  : ���ñ�׼�߼��豸״̬
 �������  : int device_index
             int status
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��19�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
            /* �������ݿ� */
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
 �� �� ��  : RemoveGBLogicDevice
 ��������  : ���������豸ID�Ƴ��߼��豸
 �������  : int device_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��10�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : SetGBLogicDeviceIntelligentStatus
 ��������  : �����߼��豸�����ܷ���״̬
 �������  : int device_index
             intelligent_status_t status
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��10��23�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : IsGBLogicDeviceInfoHasChangeForRCU
 ��������  : ��׼�߼��豸��Ϣ�Ƿ��б仯
 �������  : GBLogicDevice_info_t* pOldGBLogicDeviceInfo
             GBLogicDevice_info_t* pNewGBLogicDeviceInfo
             int change_type:0:�ڴ浽���ݿ�ıȽϣ�1:���ݿ⵽�ڴ�ıȽ�
 �������  : ��
 �� �� ֵ  :int
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ��RCU�߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, RCU�ϱ���AlarmPriority�����仯: �ϵ�AlarmPriority=%d, �µ�AlarmPriority=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->AlarmPriority, pNewGBLogicDeviceInfo->AlarmPriority);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end RCU logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The AlarmPriority reported by RCU change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->AlarmPriority, pNewGBLogicDeviceInfo->AlarmPriority);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRCU() exit---: CHANGE AlarmPriority: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* RCU Guard */
        if (pOldGBLogicDeviceInfo->guard_type != pNewGBLogicDeviceInfo->guard_type)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ��RCU�߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, RCU�ϱ���Guard�����仯: �ϵ�Guard=%d, �µ�Guard=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->guard_type, pNewGBLogicDeviceInfo->guard_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front RCU end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The Guard reported by RCU change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->guard_type, pNewGBLogicDeviceInfo->guard_type);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRCU() exit---: CHANGE AlarmPriority: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "IsGBLogicDeviceInfoHasChangeForRCU() exit---: NO CHANGE \r\n");
    return 0;
}

/*****************************************************************************
 �� �� ��  : IsGBLogicDeviceInfoHasChange
 ��������  : ��׼�߼��豸��Ϣ�Ƿ��б仯
 �������  : GBLogicDevice_info_t* pOldGBLogicDeviceInfo
             GBLogicDevice_info_t* pNewGBLogicDeviceInfo
             int change_type:0:�ڴ浽���ݿ�ıȽϣ�1:���ݿ⵽�ڴ�ıȽ�
 �������  : ��
 �� �� ֵ  :int
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

    if (0 == change_type) /* ��ǰ���ϱ�Ϊ׼���ڴ浽���ݿ�ıȽ� */
    {
        /* ��λ���� */
        if (('\0' != pNewGBLogicDeviceInfo->device_name[0]
             && (0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IP Camera", 9)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPCamera", 8)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"Camera", 6)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPC", 3)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"ͨ��", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"��ǹ", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"����", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"����", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_id, MAX_ID_LEN)))
            || '\0' == pNewGBLogicDeviceInfo->device_name[0])
        {
            /* ���ǰ���ϱ����� IP Camera��Camera�������ƣ������ϱ���Ϊ�գ���ȥ�Ƚϣ������ݿ�����Ϊ׼ */
        }
        else
        {
            if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ǰ���ϱ��ĵ�λ���Ʒ����仯: �ϵĵ�λ����=%s, �µĵ�λ����=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point's name change: Old Poit Name=%s,New Poit Name=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE device_name 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }
        }

        /* �Ƿ����� */
        if (pOldGBLogicDeviceInfo->enable != pNewGBLogicDeviceInfo->enable)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ǰ���ϱ��ĵ�λ���ñ�ʶ�����仯: �ϵĽ��ñ�ʶ=%d, �µĽ��ñ�ʶ=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->enable, pNewGBLogicDeviceInfo->enable);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point's disable mark change: Old Poit Mark=%s,New Poit Mark=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->enable, pNewGBLogicDeviceInfo->enable);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �����������ͨ������Ҫ���±����豸������ */
        if (pOldGBLogicDeviceInfo->alarm_device_sub_type != pNewGBLogicDeviceInfo->alarm_device_sub_type)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ǰ���ϱ��ĵ�λ�����豸���ͷ����仯: �ϵı����豸����=%u, �µı����豸����=%u", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_device_sub_type, pNewGBLogicDeviceInfo->alarm_device_sub_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point's alarm type change: Old alarm type=%s,New alarm type=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name,  pOldGBLogicDeviceInfo->alarm_device_sub_type, pNewGBLogicDeviceInfo->alarm_device_sub_type);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE alarm_device_sub_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �Ƿ������������� */
        if (pOldGBLogicDeviceInfo->other_realm != pNewGBLogicDeviceInfo->other_realm)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ǰ���ϱ��ĵ�λ�Ƿ���������������ʶ�����仯: �ϵ��Ƿ���������������ʶ=%d, �µ��Ƿ���������������ʶ=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->other_realm, pNewGBLogicDeviceInfo->other_realm);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, Front-end reporting point whether belong to other domain name change: Old =%s,New =%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->other_realm, pNewGBLogicDeviceInfo->other_realm);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE other_realm: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ��Ӧ��ý�������豸���� */
        if (pOldGBLogicDeviceInfo->phy_mediaDeviceIndex != pNewGBLogicDeviceInfo->phy_mediaDeviceIndex)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE phy_mediaDeviceIndex: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �豸������ */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->manufacturer, pOldGBLogicDeviceInfo->manufacturer))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE manufacturer 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �豸�ͺ� */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->model, pOldGBLogicDeviceInfo->model))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE model 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �豸���� */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->owner, pOldGBLogicDeviceInfo->owner))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE owner 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

#if 0

        /* �������� */
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

        /* ���� */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->block, pOldGBLogicDeviceInfo->block))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE block 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ��װ��ַ */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->address, pOldGBLogicDeviceInfo->address))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE address 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �Ƿ������豸 */
        if (pOldGBLogicDeviceInfo->parental != pNewGBLogicDeviceInfo->parental)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE parental: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ���豸/����/ϵͳID */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->parentID, pOldGBLogicDeviceInfo->parentID))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE parentID 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ���ȫģʽ*/
        if (pOldGBLogicDeviceInfo->safety_way != pNewGBLogicDeviceInfo->safety_way)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE safety_way: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ע�᷽ʽ */
        if (pOldGBLogicDeviceInfo->register_way != pNewGBLogicDeviceInfo->register_way)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE register_way: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ֤�����к�*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->cert_num, pOldGBLogicDeviceInfo->cert_num))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE cert_num 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ֤����Ч��ʶ */
        if (pOldGBLogicDeviceInfo->certifiable != pNewGBLogicDeviceInfo->certifiable)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE certifiable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ��Чԭ���� */
        if (pOldGBLogicDeviceInfo->error_code != pNewGBLogicDeviceInfo->error_code)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE error_code: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ֤����ֹ��Ч��*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->end_time, pOldGBLogicDeviceInfo->end_time))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE end_time 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �������� */
        if (pOldGBLogicDeviceInfo->secrecy != pNewGBLogicDeviceInfo->secrecy)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE secrecy: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* IP��ַ*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->ip_address, pOldGBLogicDeviceInfo->ip_address))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE ip_address 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �˿ں�*/
        if (pOldGBLogicDeviceInfo->port != pNewGBLogicDeviceInfo->port)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE port: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ����*/
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
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, RCU�ϱ���AlarmPriority�����仯: �ϵ�AlarmPriority=%d, �µ�AlarmPriority=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->AlarmPriority, pNewGBLogicDeviceInfo->AlarmPriority);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The AlarmPriority reported by RCU change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->AlarmPriority, pNewGBLogicDeviceInfo->AlarmPriority);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE AlarmPriority: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* RCU Guard */
        if (pOldGBLogicDeviceInfo->guard_type != pNewGBLogicDeviceInfo->guard_type)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, RCU�ϱ���Guard�����仯: �ϵ�Guard=%d, �µ�Guard=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->guard_type, pNewGBLogicDeviceInfo->guard_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The Guard reported by RCU change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->guard_type, pNewGBLogicDeviceInfo->guard_type);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE guard_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ��λ���� */
        if (pOldGBLogicDeviceInfo->device_type != pNewGBLogicDeviceInfo->device_type)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE device_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ������CMSID */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->cms_id, pOldGBLogicDeviceInfo->cms_id))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE cms_id 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �Ƿ�ɿ�,���ǰ���ϱ��Ĵ���0����ǰ���ϱ���Ϊ׼ */


        if (pNewGBLogicDeviceInfo->ctrl_enable > 0)
        {

            if (pOldGBLogicDeviceInfo->ctrl_enable != pNewGBLogicDeviceInfo->ctrl_enable)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ǰ���ϱ��ĵ�λ�����ʶ�����仯: �ϵĿ����ʶ=%d, �µĿ����ʶ=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->ctrl_enable, pNewGBLogicDeviceInfo->ctrl_enable);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s,CMS reporting point ball logo at a lower level change: Old =%s,New =%s",  pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->ctrl_enable, pNewGBLogicDeviceInfo->ctrl_enable);
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE ctrl_enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }
        }

        /* ������¼�ƽ̨�ĵ�λ����ǰ���ϱ�Ϊ׼���ֶ� */
        if (NULL != pGBDeviceInfo
            && EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
            && 0 == pGBDeviceInfo->three_party_flag)
        {
            /* ��Ƶ�Խ� */
            if (pOldGBLogicDeviceInfo->mic_enable != pNewGBLogicDeviceInfo->mic_enable)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, �¼�CMS�ϱ��ĵ�λ�Ƿ�֧�ֶԽ���ʶ�����仯: �ϵ��Ƿ�֧�ֶԽ���ʶ=%d, �µ��Ƿ�֧�ֶԽ���ʶ=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING,  "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower  CMS whether to support the intercom CMS mark change: old =%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE mic_enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* ֡�� */
            if (pOldGBLogicDeviceInfo->frame_count != pNewGBLogicDeviceInfo->frame_count)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, �¼�CMS�ϱ��ĵ�λ֡�ʷ����仯: �ϵ�֡��=%d, �µ�֡��=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->frame_count, pNewGBLogicDeviceInfo->frame_count);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The lower CMS reporting point frame rate change: old fram rate=%d, new fram rate=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->frame_count, pNewGBLogicDeviceInfo->frame_count);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE frame_count: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* ����ʱ�� */
            if (pOldGBLogicDeviceInfo->alarm_duration != pNewGBLogicDeviceInfo->alarm_duration)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, �¼�CMS�ϱ��ĵ�λ�澯ʱ�������仯: �ϵĸ澯ʱ��=%d, �µĸ澯ʱ��=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_duration, pNewGBLogicDeviceInfo->alarm_duration);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The lower CMS reporting point alarm duration change: old alarm duration=%d, new alarm duration=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_duration, pNewGBLogicDeviceInfo->alarm_duration);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE alarm_duration: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* �Ƿ�֧�ֶ����� */
            if (pOldGBLogicDeviceInfo->stream_count != pNewGBLogicDeviceInfo->stream_count)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, �¼�CMS�ϱ��ĵ�λ�Ƿ�֧�ֶ����������仯: �ϵ��Ƿ�֧�ֶ�����=%d, �µ��Ƿ�֧�ֶ�����=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS whether to support support more stream change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE stream_count: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* ���� */
            if (pNewGBLogicDeviceInfo->longitude > 0 && pOldGBLogicDeviceInfo->longitude != pNewGBLogicDeviceInfo->longitude)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, �¼�CMS�ϱ��ĵ�λ���ȷ����仯: �ϵľ���=%f, �µľ���=%f", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->longitude, pNewGBLogicDeviceInfo->longitude);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS longitude change: old=%f, new=%f", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->longitude, pNewGBLogicDeviceInfo->longitude);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE longitude: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* γ�� */
            if (pNewGBLogicDeviceInfo->latitude > 0 && pOldGBLogicDeviceInfo->latitude != pNewGBLogicDeviceInfo->latitude)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, �¼�CMS�ϱ��ĵ�λγ�ȷ����仯: �ϵ�γ��=%f, �µ�γ��=%f", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->latitude, pNewGBLogicDeviceInfo->latitude);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS latitude change: old=%f, new=%f", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->latitude, pNewGBLogicDeviceInfo->latitude);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE latitude: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* ����ͼ�� */
            if (pNewGBLogicDeviceInfo->map_layer[0] != '\0' && 0 != sstrcmp(pNewGBLogicDeviceInfo->map_layer, pOldGBLogicDeviceInfo->map_layer))
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE map_layer 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }
        }

        /* �����ý�����صĵ�λ����ǰ���ϱ�Ϊ׼���ֶ� */
        if (EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
        {
            if (pOldGBLogicDeviceInfo->stream_count != pNewGBLogicDeviceInfo->stream_count)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ý�������ϱ��ĵ�λ�Ƿ�֧�ֶ����������仯: �ϵ��Ƿ�֧�ֶ�����=%d, �µ��Ƿ�֧�ֶ�����=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS whether to support support more stream change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE stream_count: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }
        }

        /* ��λ״̬ */
        if (pOldGBLogicDeviceInfo->status != pNewGBLogicDeviceInfo->status)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ǰ���ϱ���λ״̬�����仯: �ϵ�״̬=%d, �µ�״̬=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, Front-end reporting point status change: Old =%s,New =%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE status: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 2;
        }
    }

    if (1 == change_type) /* �����ݿ�Ϊ׼�����ݿ⵽�ڴ�ıȽ� */
    {
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ���ݿ��λ���Ʒ����仯: �ϵĵ�λ����=%s, �µĵ�λ����=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, Front-end reporting point name change: Old =%s,New =%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE device_name 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ��Ӧ��ý�������豸ͨ�� */
        if (pOldGBLogicDeviceInfo->phy_mediaDeviceChannel != pNewGBLogicDeviceInfo->phy_mediaDeviceChannel)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE phy_mediaDeviceChannel: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ��Ӧ�Ŀ��������豸���� */
        if (pOldGBLogicDeviceInfo->phy_ctrlDeviceIndex != pNewGBLogicDeviceInfo->phy_ctrlDeviceIndex)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE phy_ctrlDeviceIndex: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ��Ӧ�Ŀ��������豸ͨ�� */
        if (pOldGBLogicDeviceInfo->phy_ctrlDeviceChannel != pNewGBLogicDeviceInfo->phy_ctrlDeviceChannel)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE phy_ctrlDeviceChannel: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ¼������*/
        if (pOldGBLogicDeviceInfo->record_type != pNewGBLogicDeviceInfo->record_type)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ���ݿ��λ¼�����ͷ����仯: �ϵ�¼������=%d, �µ�¼������=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->record_type, pNewGBLogicDeviceInfo->record_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, Front-end reporting point record type change: Old =%s,New =%s",  pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->record_type, pNewGBLogicDeviceInfo->record_type);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE record_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �Ƿ�ɿ� */
        if (pOldGBLogicDeviceInfo->ctrl_enable != pNewGBLogicDeviceInfo->ctrl_enable)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ���ݿ��λ�����ʶ�����仯: �ϵĿ����ʶ=%d, �µĿ����ʶ=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->ctrl_enable, pNewGBLogicDeviceInfo->ctrl_enable);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s,CMS reporting point ball logo at a lower level change: Old =%s,New =%s",  pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->ctrl_enable, pNewGBLogicDeviceInfo->ctrl_enable);
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE ctrl_enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ������¼�ƽ̨�ĵ�λ�������ݿ�Ϊ׼���ֶ� */
        if (NULL != pGBDeviceInfo
            && EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type)
        {
            /* ��Ƶ�Խ� */
            if (pOldGBLogicDeviceInfo->mic_enable != pNewGBLogicDeviceInfo->mic_enable)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ���ݿ��λ�Ƿ�֧�ֶԽ���ʶ�����仯: �ϵ��Ƿ�֧�ֶԽ���ʶ=%d, �µ��Ƿ�֧�ֶԽ���ʶ=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING,  "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower  CMS whether to support the intercom CMS mark change: old =%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE mic_enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* ֡�� */
            if (pOldGBLogicDeviceInfo->frame_count != pNewGBLogicDeviceInfo->frame_count)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ���ݿ��λ֡�ʷ����仯: �ϵ�֡��=%d, �µ�֡��=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->frame_count, pNewGBLogicDeviceInfo->frame_count);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING,  "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower  CMS whether to support the intercom CMS mark change: old =%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE frame_count: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* ����ʱ�� */
            if (pOldGBLogicDeviceInfo->alarm_duration != pNewGBLogicDeviceInfo->alarm_duration)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ���ݿ��λ�澯ʱ�������仯: �ϵĸ澯ʱ��=%d, �µĸ澯ʱ��=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_duration, pNewGBLogicDeviceInfo->alarm_duration);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The lower CMS reporting point alarm duration change: old alarm duration=%d, new alarm duration=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_duration, pNewGBLogicDeviceInfo->alarm_duration);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE alarm_duration: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* ���� */
            if (pOldGBLogicDeviceInfo->longitude != pNewGBLogicDeviceInfo->longitude)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ���ݿ��λ���ȷ����仯: �ϵľ���=%.16lf, �µľ���=%.16lf", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->longitude, pNewGBLogicDeviceInfo->longitude);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS longitude change: old=%f, new=%f", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->longitude, pNewGBLogicDeviceInfo->longitude);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE longitude: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* γ�� */
            if (pOldGBLogicDeviceInfo->latitude != pNewGBLogicDeviceInfo->latitude)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ���ݿ��λ���ȷ����仯: �ϵ�γ��=%.16lf, �µ�γ��=%.16lf", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->latitude, pNewGBLogicDeviceInfo->latitude);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS latitude change: old=%f, new=%f", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->latitude, pNewGBLogicDeviceInfo->latitude);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE latitude: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }

            /* ����ͼ�� */
            if (0 != sstrcmp(pNewGBLogicDeviceInfo->map_layer, pOldGBLogicDeviceInfo->map_layer))
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE map_layer 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }
        }

        /* ӥ�������Ӧ��Ԥ��ID */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->strResved2, pOldGBLogicDeviceInfo->strResved2))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE strResved2: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 3;
        }

        /* ������¼�ƽ̨�ĵ�λ,���Ҳ���ý�����صĵ�λ�������ݿ�Ϊ׼���ֶ� */
        if (NULL != pGBDeviceInfo
            && EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type
            && EV9000_DEVICETYPE_MGWSERVER != pGBDeviceInfo->device_type)
        {
            /* �Ƿ�֧�ֶ����� */
            if (pOldGBLogicDeviceInfo->stream_count != pNewGBLogicDeviceInfo->stream_count)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ���ݿ��λ�Ƿ�֧�ֶ����������仯: �ϵ��Ƿ�֧�ֶ�����=%d, �µ��Ƿ�֧�ֶ�����=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS whether to support support more stream change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE stream_count: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : IsGBLogicDeviceInfoHasChangeForRoute
 ��������  : ��׼�߼��豸��Ϣ�Ƿ��б仯
 �������  : GBLogicDevice_info_t * pOldGBLogicDeviceInfo
             GBLogicDevice_info_t * pNewGBLogicDeviceInfo
             int change_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��10��10��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int IsGBLogicDeviceInfoHasChangeForRoute(GBLogicDevice_info_t * pOldGBLogicDeviceInfo, GBLogicDevice_info_t * pNewGBLogicDeviceInfo, int change_type)
{
    if (NULL == pOldGBLogicDeviceInfo || NULL == pNewGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: Param Error \r\n");
        return 0;
    }

    if (0 == change_type) /* ��ǰ���ϱ�Ϊ׼���ڴ浽���ݿ�ıȽ� */
    {
        /* ��λ���� */
        if (('\0' != pNewGBLogicDeviceInfo->device_name[0]
             && (0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IP Camera", 9)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPCamera", 8)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"Camera", 6)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"IPC", 3)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"ͨ��", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"��ǹ", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"����", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, (char*)"����", 4)
                 || 0 == strncmp(pNewGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_id, MAX_ID_LEN)))
            || '\0' == pNewGBLogicDeviceInfo->device_name[0])
        {
            /* ���ǰ���ϱ����� IP Camera��Camera�������ƣ������ϱ���Ϊ�գ���ȥ�Ƚϣ������ݿ�����Ϊ׼ */
        }
        else
        {
            if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ǰ���ϱ��ĵ�λ���Ʒ����仯: �ϵĵ�λ����=%s, �µĵ�λ����=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point's name change: Old Poit Name=%s,New Poit Name=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);

                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE device_name 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
                return 1;
            }
        }

        /* �Ƿ����� */
        if (pOldGBLogicDeviceInfo->enable != pNewGBLogicDeviceInfo->enable)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ǰ���ϱ��ĵ�λ���ñ�ʶ�����仯: �ϵĽ��ñ�ʶ=%d, �µĽ��ñ�ʶ=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->enable, pNewGBLogicDeviceInfo->enable);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point's disable mark change: Old Poit Mark=%s,New Poit Mark=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->enable, pNewGBLogicDeviceInfo->enable);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �����������ͨ������Ҫ���±����豸������ */
        if (pOldGBLogicDeviceInfo->alarm_device_sub_type != pNewGBLogicDeviceInfo->alarm_device_sub_type)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ǰ���ϱ��ĵ�λ�����豸���ͷ����仯: �ϵı����豸����=%u, �µı����豸����=%u", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_device_sub_type, pNewGBLogicDeviceInfo->alarm_device_sub_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point's alarm type change: Old alarm type=%s,New alarm type=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name,  pOldGBLogicDeviceInfo->alarm_device_sub_type, pNewGBLogicDeviceInfo->alarm_device_sub_type);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE alarm_device_sub_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �Ƿ������������� */
        if (pOldGBLogicDeviceInfo->other_realm != pNewGBLogicDeviceInfo->other_realm)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ǰ���ϱ��ĵ�λ�Ƿ���������������ʶ�����仯: �ϵ��Ƿ���������������ʶ=%d, �µ��Ƿ���������������ʶ=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->other_realm, pNewGBLogicDeviceInfo->other_realm);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, Front-end reporting point whether belong to other domain name change: Old =%s,New =%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->other_realm, pNewGBLogicDeviceInfo->other_realm);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE other_realm: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        if (pOldGBLogicDeviceInfo->ctrl_enable != pNewGBLogicDeviceInfo->ctrl_enable)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, �¼�CMS�ϱ��ĵ�λ�����ʶ�����仯: �ϵĿ����ʶ=%d, �µĿ����ʶ=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->ctrl_enable, pNewGBLogicDeviceInfo->ctrl_enable);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s,CMS reporting point ball logo at a lower level change: Old =%s,New =%s",  pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->ctrl_enable, pNewGBLogicDeviceInfo->ctrl_enable);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE ctrl_enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        if (pOldGBLogicDeviceInfo->mic_enable != pNewGBLogicDeviceInfo->mic_enable)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, �¼�CMS�ϱ��ĵ�λ�Ƿ�֧�ֶԽ���ʶ�����仯: �ϵ��Ƿ�֧�ֶԽ���ʶ=%d, �µ��Ƿ�֧�ֶԽ���ʶ=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING,  "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower	CMS whether to support the intercom CMS mark change: old =%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE mic_enable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        if (pOldGBLogicDeviceInfo->frame_count != pNewGBLogicDeviceInfo->frame_count)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, �¼�CMS�ϱ��ĵ�λ֡�ʷ����仯: �ϵ�֡��=%d, �µ�֡��=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->frame_count, pNewGBLogicDeviceInfo->frame_count);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING,  "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower	CMS whether to support the intercom CMS mark change: old =%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->mic_enable, pNewGBLogicDeviceInfo->mic_enable);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE frame_count: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        if (pOldGBLogicDeviceInfo->alarm_duration != pNewGBLogicDeviceInfo->alarm_duration)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, �¼�CMS�ϱ��ĵ�λ�澯ʱ�������仯: �ϵĸ澯ʱ��=%d, �µĸ澯ʱ��=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_duration, pNewGBLogicDeviceInfo->alarm_duration);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The lower CMS reporting point alarm duration change: old alarm duration=%d, new alarm duration=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->alarm_duration, pNewGBLogicDeviceInfo->alarm_duration);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE alarm_duration: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �Ƿ�֧�ֶ����� */
        if (pOldGBLogicDeviceInfo->stream_count != pNewGBLogicDeviceInfo->stream_count)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, �¼�CMS�ϱ��ĵ�λ�Ƿ�֧�ֶ����������仯: �ϵ��Ƿ�֧�ֶ�����=%d, �µ��Ƿ�֧�ֶ�����=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The point reported by lower CMS whether to support support more stream change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->stream_count, pNewGBLogicDeviceInfo->stream_count);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE stream_count: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ��Ӧ��ý�������豸���� */
        if (pOldGBLogicDeviceInfo->phy_mediaDeviceIndex != pNewGBLogicDeviceInfo->phy_mediaDeviceIndex)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE phy_mediaDeviceIndex: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �豸������ */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->manufacturer, pOldGBLogicDeviceInfo->manufacturer))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE manufacturer 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �豸�ͺ� */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->model, pOldGBLogicDeviceInfo->model))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE model 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �豸���� */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->owner, pOldGBLogicDeviceInfo->owner))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE owner 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

#if 0

        /* �������� */
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

        /* ���� */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->block, pOldGBLogicDeviceInfo->block))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE block 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ��װ��ַ */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->address, pOldGBLogicDeviceInfo->address))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE address 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �Ƿ������豸 */
        if (pOldGBLogicDeviceInfo->parental != pNewGBLogicDeviceInfo->parental)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE parental: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ���豸/����/ϵͳID */
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->parentID, pOldGBLogicDeviceInfo->parentID))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE parentID 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ���ȫģʽ*/
        if (pOldGBLogicDeviceInfo->safety_way != pNewGBLogicDeviceInfo->safety_way)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE safety_way: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ע�᷽ʽ */
        if (pOldGBLogicDeviceInfo->register_way != pNewGBLogicDeviceInfo->register_way)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE register_way: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ֤�����к�*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->cert_num, pOldGBLogicDeviceInfo->cert_num))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE cert_num 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ֤����Ч��ʶ */
        if (pOldGBLogicDeviceInfo->certifiable != pNewGBLogicDeviceInfo->certifiable)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE certifiable: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ��Чԭ���� */
        if (pOldGBLogicDeviceInfo->error_code != pNewGBLogicDeviceInfo->error_code)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE error_code: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ֤����ֹ��Ч��*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->end_time, pOldGBLogicDeviceInfo->end_time))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE end_time 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �������� */
        if (pOldGBLogicDeviceInfo->secrecy != pNewGBLogicDeviceInfo->secrecy)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE secrecy: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* IP��ַ*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->ip_address, pOldGBLogicDeviceInfo->ip_address))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE ip_address 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* �˿ں�*/
        if (pOldGBLogicDeviceInfo->port != pNewGBLogicDeviceInfo->port)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE port: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ����*/
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->password, pOldGBLogicDeviceInfo->password))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE password 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ���� */
        if (pNewGBLogicDeviceInfo->longitude > 0 && pOldGBLogicDeviceInfo->longitude != pNewGBLogicDeviceInfo->longitude)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE longitude: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* γ�� */
        if (pNewGBLogicDeviceInfo->latitude > 0 && pOldGBLogicDeviceInfo->latitude != pNewGBLogicDeviceInfo->latitude)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE latitude: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ����ͼ�� */
        if (pNewGBLogicDeviceInfo->map_layer[0] != '\0' && 0 != sstrcmp(pNewGBLogicDeviceInfo->map_layer, pOldGBLogicDeviceInfo->map_layer))
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChange() exit---: CHANGE map_layer 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ��λ���� */
        if (pOldGBLogicDeviceInfo->device_type != pNewGBLogicDeviceInfo->device_type)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE device_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ������CMSID */
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
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, RCU�ϱ���AlarmPriority�����仯: �ϵ�AlarmPriority=%d, �µ�AlarmPriority=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->AlarmPriority, pNewGBLogicDeviceInfo->AlarmPriority);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The AlarmPriority reported by RCU change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->AlarmPriority, pNewGBLogicDeviceInfo->AlarmPriority);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE AlarmPriority: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* RCU Guard */
        if (pOldGBLogicDeviceInfo->guard_type != pNewGBLogicDeviceInfo->guard_type)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, RCU�ϱ���Guard�����仯: �ϵ�Guard=%d, �µ�Guard=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->guard_type, pNewGBLogicDeviceInfo->guard_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The Guard reported by RCU change: old=%d, new=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->guard_type, pNewGBLogicDeviceInfo->guard_type);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE guard_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ��λ״̬ */
        if (pOldGBLogicDeviceInfo->status != pNewGBLogicDeviceInfo->status)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ǰ���ϱ���λ״̬�����仯: �ϵ�״̬=%d, �µ�״̬=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, Front-end reporting point status change: Old =%s,New =%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->status, pNewGBLogicDeviceInfo->status);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE status: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 2;
        }
    }

    if (1 == change_type) /* �����ݿ�Ϊ׼�����ݿ⵽�ڴ�ıȽ� */
    {
        if (0 != sstrcmp(pNewGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name))
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ���ݿ��λ���Ʒ����仯: �ϵĵ�λ����=%s, �µĵ�λ����=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point's name change: Old Poit Name=%s,New Poit Name=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->device_name, pNewGBLogicDeviceInfo->device_name);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE device_name 1: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ��Ӧ��ý�������豸ͨ�� */
        if (pOldGBLogicDeviceInfo->phy_mediaDeviceChannel != pNewGBLogicDeviceInfo->phy_mediaDeviceChannel)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE phy_mediaDeviceChannel: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ��Ӧ�Ŀ��������豸���� */
        if (pOldGBLogicDeviceInfo->phy_ctrlDeviceIndex != pNewGBLogicDeviceInfo->phy_ctrlDeviceIndex)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE phy_ctrlDeviceIndex: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ��Ӧ�Ŀ��������豸ͨ�� */
        if (pOldGBLogicDeviceInfo->phy_ctrlDeviceChannel != pNewGBLogicDeviceInfo->phy_ctrlDeviceChannel)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE phy_ctrlDeviceChannel: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }

        /* ¼������*/
        if (pOldGBLogicDeviceInfo->record_type != pNewGBLogicDeviceInfo->record_type)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ǰ���߼��豸��Ϣ�����仯, �߼��豸ID=%s, �߼���λ����=%s, ���ݿ��λ¼�����ͷ����仯: �ϵ�¼������=%d, �µ�¼������=%d", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->record_type, pNewGBLogicDeviceInfo->record_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Front end logic device information change, Logic Device ID=%s, Logic Poit Name=%s, The front-end reported point record type change: Old Poit Name=%s,New Poit Name=%s", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->device_name, pOldGBLogicDeviceInfo->record_type, pNewGBLogicDeviceInfo->record_type);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "IsGBLogicDeviceInfoHasChangeForRoute() exit---: CHANGE record_type: device_id=%s \r\n", pOldGBLogicDeviceInfo->device_id);
            return 1;
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : IsGBDeviceRegInfoHasChange
 ��������  : ��׼�����豸ע����Ϣ�Ƿ��б仯
 �������  : int pos
                            int iChannel
                            char* pcManufacturer
                            char* pcModel
                            char* pcVersion
 �������  : ��
 �� �� ֵ  :int
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��20��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int IsGBDeviceRegInfoHasChange(GBDevice_info_t * pGBDeviceInfo, int iDeviceType, char * pcLoginIP, int iLoginPort, int iRegInfoIndex)
{
    if (pGBDeviceInfo == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "IsGBDeviceRegInfoHasChange() exit---: Param Error \r\n");
        return 0;
    }

    /* �豸���� */
    if (pGBDeviceInfo->device_type != iDeviceType)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "IsGBDeviceRegInfoHasChange() exit---: CHANGE 12 \r\n");
        return 1;
    }

    /* �豸��¼�˿� */
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

    /* �豸��¼IP ��ַ */
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

    /* �豸ע������ */
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
 �� �� ��  : SendExecuteDevicePresetMessage
 ��������  : ����ִ��Ԥ��λ�����ǰ���豸
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             GBDevice_info_t* pGBDeviceInfo
             unsigned int uPresetID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��10�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����ִ��Ԥ��λMessage��Ϣ��ǰ���豸ʧ��, ��λ�ѱ��û�����:�߼���λID=%s, ��λ����=%s, ǰ��IP��ַ=%s, �˿ں�=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to send message that notifies preset, point has been locked by the user :Logic ID=%s, name=%s, IP=%s, Port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        return -2;
    }
    else if (LOCK_STATUS_ROUTE_LOCK == pGBLogicDeviceInfo->lock_status)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����ִ��Ԥ��λMessage��Ϣ��ǰ���豸ʧ��, ��λ�ѱ��ϼ�����:�߼���λID=%s, ��λ����=%s, ǰ��IP��ַ=%s, �˿ں�=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to send message that notifies preset, point has been locked by the superior :Logic ID=%s, name=%s, IP=%s, Port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        return -2;
    }

    /* �齨XML��Ϣ */
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

    /* ������Ϣ */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBLogicDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ִ��Ԥ��λ����Message��Ϣ��ǰ���豸ʧ��:�߼���λID=%s, ��λ����=%s, ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to send message that notifies preset:Logic ID=%s, Name=%s, ID=%s, IP=%s, port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendExecuteDevicePresetMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ִ��Ԥ��λ����Message��Ϣ��ǰ���豸�ɹ�:�߼���λID=%s, ��λ����=%s, ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Succeed to send message that notifies preset:Logic ID=%s, Name=%s, ID=%s, IP=%s, port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendExecuteDevicePresetMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendExecuteDevicePresetMessageToRoute
 ��������  : ����ִ��Ԥ��λ������ϼ�CMS
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             route_info_t * pRouteInfo
             unsigned int uPresetID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��11��26��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����ִ��Ԥ��λMessage��Ϣ���ϼ�CMSʧ��, ��λ�ѱ��û�����:�߼���λID=%s, ��λ����=%s, �ϼ�CMS IP��ַ=%s, �˿ں�=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to send message that notifies preset, point has been locked by the user :Logic ID=%s, name=%s, IP=%s, Port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_ip, pRouteInfo->server_port);
        return -2;
    }
    else if (LOCK_STATUS_ROUTE_LOCK == pGBLogicDeviceInfo->lock_status)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����ִ��Ԥ��λMessage��Ϣ���ϼ�CMSʧ��, ��λ�ѱ��ϼ�����:�߼���λID=%s, ��λ����=%s, �ϼ�CMS IP��ַ=%s, �˿ں�=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to send message that notifies preset, point has been locked by the superior :Logic ID=%s, name=%s, IP=%s, Port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_ip, pRouteInfo->server_port);
        return -2;
    }

    /* �齨XML��Ϣ */
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

    /* ������Ϣ */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBLogicDeviceInfo->device_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ִ��Ԥ��λMessage��Ϣ���ϼ�CMSʧ��:�߼���λID=%s, ��λ����=%s, �ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send message that notifies preset:Logic ID=%s, Name=%s, ID=%s, IP=%s, port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendExecuteDevicePresetMessageToRoute() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ִ��Ԥ��λMessage��Ϣ���ϼ�CMS�ɹ�:�߼���λID=%s, ��λ����=%s, �ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send message that notifies preset:Logic ID=%s, Name=%s, ID=%s, IP=%s, port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendExecuteDevicePresetMessageToRoute() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendDeviceControlAlarmMessage
 ��������  : ���ͱ��������������
 �������  : char * device_id
             GBDevice_info_t * pGBDeviceInfo
             int iuCmdType
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��5�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �齨XML��Ϣ */
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

    /* ������Ϣ */
    i = SIP_SendMessage(NULL, local_cms_id_get(), device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���ͱ��������������Message��Ϣ��ǰ���豸ʧ��:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send alarm output control command Message to the front-end equipment:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceControlAlarmMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ͱ��������������Message��Ϣ��ǰ���豸�ɹ�:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send alarm output control command Message to the front-end equipment:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendDeviceControlAlarmMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendQueryDeviceInfoMessage
 ��������  : �����豸��Ϣ��ѯ��Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��19�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"DeviceInfo");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"1");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    /* ������Ϣ */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���Ͳ�ѯ�豸��ϢMessage��Ϣ���豸ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send alarm output control command Message to the front-end equipment:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQueryDeviceInfoMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���Ͳ�ѯ�豸��ϢMessage��Ϣ���豸�ɹ�:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Fail to send alarm output control command Message to the front-end equipment:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQueryDeviceInfoMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendQueryDeviceCatalogMessage
 ��������  : �����豸Ŀ¼��ѯ��Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��19�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"Catalog");

    AccNode = outPacket.CreateElement((char*)"SN");
    snprintf(strSN, 32, "%u", pGBDeviceInfo->CataLogSN);
    outPacket.SetElementValue(AccNode, strSN);

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    /* ������Ϣ */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���Ͳ�ѯ�豸Ŀ¼Message��Ϣ���豸ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send query inventory message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQueryDeviceInfoMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���Ͳ�ѯ�豸Ŀ¼Message��Ϣ���豸�ɹ�:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send query inventory message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQueryDeviceInfoMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendQueryDeviceStatusMessage
 ��������  : �����豸״̬��ѯ��Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��19�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"DeviceStatus");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"1");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    /* ������Ϣ */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���Ͳ�ѯ�豸״̬Message��Ϣ���豸ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send query status message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQueryDeviceStatusMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���Ͳ�ѯ�豸״̬Message��Ϣ���豸�ɹ�:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send query status message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQueryDeviceStatusMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendQueryAllOfflineLogicDeviceStatusMessage
 ��������  : ��ѯ���е��ߵ��߼��豸��λ״̬��Ϣ
 �������  : GBDevice_info_t * pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��7��19��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ͻ�ȡǰ���豸���ߵ�λ״̬��Ϣ: �豸ID=%s, IP��ַ=%s, �˿ں�=%d, ���ߵ��߼���λ��=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (int)DeviceIDVector.size());
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
 �� �� ��  : SendQueryGBLogicDeviceStatusMessage
 ��������  : ���Ͳ�ѯ�߼��豸״̬��Ϣ
 �������  : char* device_id
             GBDevice_info_t * pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��11��19��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �齨XML��Ϣ */
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

    /* ������Ϣ */
    i = SIP_SendMessage(NULL, local_cms_id_get(), device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���Ͳ�ѯ�߼��豸״̬Message��Ϣ���豸ʧ��:�߼��豸ID=%s, �豸ID=%s, IP��ַ=%s, �˿ں�=%d", device_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send query status message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQueryGBLogicDeviceStatusMessage() SIP_SendMessage Error:device_id=%s, dest_id=%s, dest_ip=%s, dest_port=%d \r\n", device_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���Ͳ�ѯ�߼��豸״̬Message��Ϣ���豸�ɹ�:�߼��豸ID=%s, �豸ID=%s, IP��ַ=%s, �˿ں�=%d", device_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send query status message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQueryGBLogicDeviceStatusMessage() SIP_SendMessage OK:device_id=%s, dest_id=%s, dest_ip=%s, dest_port=%d \r\n", device_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendQueryDeviceGroupInfoMessage
 ��������  : ���ͻ�ȡ�߼��豸����������Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��12�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"LogicDeviceGroupConfig");

    AccNode = outPacket.CreateElement((char*)"SN");
    snprintf(strSN, 32, "%u", pGBDeviceInfo->LogicDeviceGroupSN);
    outPacket.SetElementValue(AccNode, strSN);

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    /* ������Ϣ */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���Ͳ�ѯ�豸����Message��Ϣ���豸ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send query group message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQueryDeviceGroupInfoMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���Ͳ�ѯ�豸����Message��Ϣ���豸�ɹ�:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send query group message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQueryDeviceGroupInfoMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendQueryDeviceGroupMapInfoMessage
 ��������  : ���ͻ�ȡ�߼��豸�����ϵ������Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��12�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"LogicDeviceMapGroupConfig");

    AccNode = outPacket.CreateElement((char*)"SN");
    snprintf(strSN, 32, "%u", pGBDeviceInfo->LogicDeviceMapGroupSN);
    outPacket.SetElementValue(AccNode, strSN);

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    /* ������Ϣ */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���Ͳ�ѯ�豸�����ϵMessage��Ϣ���豸ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send query group message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQueryDeviceGroupMapInfoMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���Ͳ�ѯ�豸�����ϵMessage��Ϣ���豸�ɹ�:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send query group message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQueryDeviceGroupMapInfoMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendDeviceTeleBootMessage
 ��������  : �����豸Զ����������
 �������  : GBDevice_info_t * pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��3��4�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"17298");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    AccNode = outPacket.CreateElement((char*)"TeleBoot");
    outPacket.SetElementValue(AccNode, (char*)"Boot");

    /* ������Ϣ */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���������豸Message��Ϣ���豸ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send restart message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceTeleBootMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���������豸Message��Ϣ���豸�ɹ�:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send restart message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendDeviceTeleBootMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendQueryDECDeviceMediaPortMessage
 ��������  : ���ͻ�ȡ������ý��˿���Ϣ
 �������  : GBDevice_info_t * pGBDeviceInfo
             char* dc_id
             char* camera_id
             int iStreamType
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��10��21��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �齨XML��Ϣ */
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

    /* ������Ϣ */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���Ͳ�ѯ������ý��˿�Message��Ϣ���豸ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ����ͨ��ID=%s, �߼���λId=%s, ��������=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, dc_id, camera_id, iStreamType);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send query media port message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQueryDECDeviceMediaPortMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���Ͳ�ѯ������ý��˿�Message��Ϣ���豸�ɹ�:�豸ID=%s, IP��ַ=%s, �˿ں�=%d, ����ͨ��ID=%s, �߼���λId=%s, ��������=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, dc_id, camera_id, iStreamType);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send query media port message to equipment :ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQueryDECDeviceMediaPortMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : UpdateGBDeviceRegStatus2DB
 ��������  : ���±�׼�����豸���ݿ�ע��״̬
 �������  : int pos
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

    /* �������ݿ� */
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
 �� �� ��  : UpdateGBDeviceLinkType2DB
 ��������  : �����豸���������͵����ݿ�
 �������  : GBDevice_info_t* pGBDeviceInfo
                            DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��23�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �������ݿ� */
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
 �� �� ��  : UpdateGBDeviceCMSID2DB
 ��������  : ���������豸��CMSID�����ݿ�
 �������  : char* device_id
             DBOper * pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��7��4�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �������ݿ� */
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
 �� �� ��  : UpdateGBDeviceInfo2DB
 ��������  : ���������豸��Ϣ�����ݿ�
 �������  : GBDevice_cfg_t& new_GBDevice_cfg
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��30��
    ��    ��   : �û�·����Ϣ����
    �޸�����   : �����ɺ���

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

    /* ��ȡ���ݿ�ԭ�������� */
    int tmp_ivalue = 0;
    string tmp_svalue = "";

    /* �豸��Ƶ����ͨ���� */
    tmp_ivalue = 0;
    pDevice_Srv_dboper->GetFieldValue("MaxCamera", tmp_ivalue);

    old_GBDevice_cfg.device_max_camera = tmp_ivalue;


    /* �豸��������ͨ���� */
    tmp_ivalue = 0;
    pDevice_Srv_dboper->GetFieldValue("MaxAlarm", tmp_ivalue);

    old_GBDevice_cfg.device_max_alarm = tmp_ivalue;

    /* �豸������ */
    tmp_svalue.clear();
    pDevice_Srv_dboper->GetFieldValue("Manufacturer", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        old_GBDevice_cfg.device_manufacturer = tmp_svalue;
    }

    /* �豸�ͺ� */
    tmp_svalue.clear();
    pDevice_Srv_dboper->GetFieldValue("Model", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        old_GBDevice_cfg.device_model = tmp_svalue;
    }

    /* �豸�汾 */
    tmp_svalue.clear();
    pDevice_Srv_dboper->GetFieldValue("Firmware", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        old_GBDevice_cfg.device_firmware = tmp_svalue;
    }

    /* �ж��Ƿ��б仯 */
    if (old_GBDevice_cfg.device_max_camera == new_GBDevice_cfg.device_max_camera
        && old_GBDevice_cfg.device_max_alarm == new_GBDevice_cfg.device_max_alarm
        && old_GBDevice_cfg.device_manufacturer == new_GBDevice_cfg.device_manufacturer
        && old_GBDevice_cfg.device_model == new_GBDevice_cfg.device_model
        && old_GBDevice_cfg.device_firmware == new_GBDevice_cfg.device_firmware)
    {
        return 0;
    }

    /* �������ݿ� */
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
 �� �� ��  : UpdateGBLogicDeviceRegStatus2DB
 ��������  : ���±�׼�߼��豸���ݿ�ע��״̬
 �������  : char* strDeviceID
                            int status
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

    /* �������ݿ� */
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
 �� �� ��  : UpdateGBLogicDeviceXYParam2DB
 ��������  : �����߼��豸�ľ�γ����Ϣ�����ݿ���
 �������  : char* strDeviceID
             double longitude
             double latitude
             char * strMapLayer
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��3��28�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �������ݿ� */
    strSQL.clear();
    strSQL = "UPDATE GBLogicDeviceConfig SET ";

    strSQL += "Longitude = ";
    memset(strTmp, 0 , 64);
    snprintf(strTmp, 64, "%.16lf", longitude);
    strSQL += strTmp;

    strSQL += ",";

    /* γ�� */
    strSQL += "Latitude = ";
    memset(strTmp, 0 , 64);
    snprintf(strTmp, 64, "%.16lf", latitude);
    strSQL += strTmp;

    /* ����ͼ�� */
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
 �� �� ��  : AddGBLogicDeviceInfoByGBDeviceInfo
 ��������  : ���ݱ�׼�����豸��Ϣ����߼��豸��Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��20�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

    if (NULL != pGBLogicDeviceInfo) /* �Ѿ����ڣ����� */
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
    else /* �����ڣ���� */
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
 �� �� ��  : AddGBLogicDeviceInfo2DB
 ��������  : ���߼��豸��Ϣ��ӵ����ݿ�
 �������  : int iLogicDeviceIndex
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��20�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

    /* 1����ѯSQL ���*/
    strQuerySQL.clear();
    strQuerySQL = "select * from GBLogicDeviceConfig WHERE DeviceID like '";
    strQuerySQL += pGBLogicDeviceInfo->device_id;
    strQuerySQL += "'";


    /* 2������SQL ���*/
    strInsertSQL.clear();
    strInsertSQL = "insert into GBLogicDeviceConfig (ID,DeviceID,CMSID,DeviceName,Enable,CtrlEnable,MicEnable,FrameCount,AlarmLengthOfTime,DeviceType,PhyDeviceIndex,PhyDeviceChannel,CtrlDeviceIndex,CtrlDeviceChannel,StreamCount,RecordType,OtherRealm,Manufacturer,Model,Owner,Block,Address,Parental,ParentID,SafetyWay,RegisterWay,CertNum,Certifiable,ErrCode,EndTime,Secrecy,IPAddress,Port,Password,Status,Longitude,Latitude,Resved2,Resved1) values (";

    /* �߼��豸����*/
    memset(strDeviceIndex, 0 , 64);
    snprintf(strDeviceIndex, 64, "%u", pGBLogicDeviceInfo->id);
    strInsertSQL += strDeviceIndex;

    strInsertSQL += ",";

    /* ��λͳһ��� */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->device_id;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ������CMS ID */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->cms_id;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ��λ���� */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->device_name;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* �Ƿ����� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->enable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* �Ƿ�ɿ� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->ctrl_enable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* �Ƿ�֧�ֶԽ� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->mic_enable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ֡�� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->frame_count);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ����ʱ�� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ��λ���� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->device_type);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ��Ӧ��ý�������豸���� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceIndex);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ��Ӧ��ý�������豸ͨ�� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceChannel);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ��Ӧ�Ŀ��������豸���� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceIndex);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ��Ӧ�Ŀ��������豸ͨ�� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceChannel);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->stream_count);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ¼������ */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->record_type);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* �Ƿ����������� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->other_realm);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* �豸������ */
    strInsertSQL += "'";

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        strInsertSQL += pGBLogicDeviceInfo->manufacturer;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";

    /* �豸�ͺ� */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->model;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* �豸���� */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->owner;
    strInsertSQL += "'";

    strInsertSQL += ",";

#if 0
    /* �������� */
    strInsertSQL += "'";

    if (NULL != pGBLogicDeviceInfo->civil_code)
    {
        strInsertSQL += pGBLogicDeviceInfo->civil_code;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";
#endif

    /* ���� */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->block;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ��װ��ַ */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->address;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* �Ƿ������豸 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->parental);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ���豸/����/ϵͳID */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->parentID;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ���ȫģʽ*/
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->safety_way);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ע�᷽ʽ */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->register_way);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ֤�����к�*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->cert_num;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ֤����Ч��ʶ */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->certifiable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ��Чԭ���� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->error_code);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ֤����ֹ��Ч��*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->end_time;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* �������� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->secrecy);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* IP��ַ*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->ip_address;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* �˿ں� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->port);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ����*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->password;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ��λ״̬ */
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

    /* ���� */
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
    strInsertSQL += strTmp1;

    strInsertSQL += ",";

    /* γ�� */
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
    strInsertSQL += strTmp1;

    strInsertSQL += ",";

    /* ����ͼ�� */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->map_layer;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* �����豸������ */
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
    strInsertSQL += strTmp1;

    strInsertSQL += ")";


    /* 3������SQL ���*/
    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE GBLogicDeviceConfig SET ";

#if 0
    /* ���� */
    strUpdateSQL += "ID = ";
    memset(strDeviceIndex, 0 , 64);
    snprintf(strDeviceIndex, 64, "%u", pGBLogicDeviceInfo->id);
    strUpdateSQL += strDeviceIndex;

    strUpdateSQL += ",";
#endif

    /* ������CMS ID */
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
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"ͨ��", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"��ǹ", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"����", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"����", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, pGBLogicDeviceInfo->device_id, MAX_ID_LEN)))
        || '\0' == pGBLogicDeviceInfo->device_name[0])
    {
        /* ���ǰ���ϱ����� IP Camera��Camera�������ƣ������ϱ���Ϊ�գ��򲻸������ݿ⣬�����ݿ�����Ϊ׼ */
    }
    else
    {
        /* ��λ���� */
        strUpdateSQL += "DeviceName = ";
        strUpdateSQL += "'";
        strUpdateSQL += pGBLogicDeviceInfo->device_name;
        strUpdateSQL += "'";

        strUpdateSQL += ",";
    }

    /* �Ƿ����� */
    strUpdateSQL += "Enable = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->enable);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* �Ƿ����������� */
    strUpdateSQL += "OtherRealm = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->other_realm);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    if (pGBLogicDeviceInfo->ctrl_enable > 0)
    {
        /* �Ƿ�ɿ� */
        strUpdateSQL += "CtrlEnable = ";
        memset(strTmp, 0 , 16);
        snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->ctrl_enable);
        strUpdateSQL += strTmp;

        strUpdateSQL += ",";
    }

    if (NULL != pGBDeviceInfo
        && EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
        && 0 == pGBDeviceInfo->three_party_flag) /* ������Լ����¼�ƽ̨�ĵ�λ������Ҫ���µ�λ�ɿر�ʶ*/
    {
        /* �Ƿ�֧�ֶԽ� */
        strUpdateSQL += "MicEnable = ";
        memset(strTmp, 0 , 16);
        snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->mic_enable);
        strUpdateSQL += strTmp;

        strUpdateSQL += ",";

        /* ֡�� */
        strUpdateSQL += "FrameCount = ";
        memset(strTmp, 0 , 16);
        snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->frame_count);
        strUpdateSQL += strTmp;

        strUpdateSQL += ",";

        /* �Ƿ�֧�ֶ����� */
        strUpdateSQL += "StreamCount = ";
        memset(strTmp, 0 , 16);
        snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->stream_count);
        strUpdateSQL += strTmp;

        strUpdateSQL += ",";

        /* �澯ʱ�� */
        strUpdateSQL += "AlarmLengthOfTime = ";
        memset(strTmp, 0 , 16);
        snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
        strUpdateSQL += strTmp;

        strUpdateSQL += ",";
    }
    else if (NULL != pGBDeviceInfo && EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)  /* �����ý�����صĵ�λ������Ҫ���µ�λ˫������ʶ */
    {
        /* �Ƿ�֧�ֶ����� */
        strUpdateSQL += "StreamCount = ";
        memset(strTmp, 0 , 16);
        snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->stream_count);
        strUpdateSQL += strTmp;

        strUpdateSQL += ",";
    }

    /* ��Ӧ��ý�������豸���� */
    strUpdateSQL += "PhyDeviceIndex = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceIndex);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

#if 0
    /* ��Ӧ��ý�������豸ͨ�� */
    strUpdateSQL += "PhyDeviceChannel = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceChannel);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ��Ӧ�Ŀ��������豸���� */
    strUpdateSQL += "CtrlDeviceIndex = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceIndex);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ��Ӧ�Ŀ��������豸ͨ�� */
    strUpdateSQL += "CtrlDeviceChannel = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceChannel);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* �Ƿ�֧�ֶ����� */
    strUpdateSQL += "StreamCount = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->stream_count);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ¼������ */
    strUpdateSQL += "RecordType = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->record_type);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";
#endif

    /* �豸������ */
    strUpdateSQL += "Manufacturer = ";
    strUpdateSQL += "'";

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        strUpdateSQL += pGBLogicDeviceInfo->manufacturer;
    }

    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* �豸�ͺ� */
    strUpdateSQL += "Model = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->model;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* �豸���� */
    strUpdateSQL += "Owner = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->owner;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

#if 0
    /* �������� */
    strUpdateSQL += "CivilCode = ";
    strUpdateSQL += "'";

    if (NULL != pGBLogicDeviceInfo->civil_code)
    {
        strUpdateSQL += pGBLogicDeviceInfo->civil_code;
    }

    strUpdateSQL += "'";

    strUpdateSQL += ",";
#endif

    /* ���� */
    strUpdateSQL += "Block = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->block;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* ��װ��ַ */
    strUpdateSQL += "Address = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->address;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* �Ƿ������豸 */
    strUpdateSQL += "Parental = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->parental);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ���豸/����/ϵͳID */
    strUpdateSQL += "ParentID = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->parentID;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* ���ȫģʽ*/
    strUpdateSQL += "SafetyWay = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->safety_way);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ע�᷽ʽ */
    strUpdateSQL += "RegisterWay = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->register_way);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ֤�����к�*/
    strUpdateSQL += "CertNum = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->cert_num;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* ֤����Ч��ʶ */
    strUpdateSQL += "Certifiable = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->certifiable);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ��Чԭ���� */
    strUpdateSQL += "ErrCode = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->error_code);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ֤����ֹ��Ч��*/
    strUpdateSQL += "EndTime = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->end_time;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* �������� */
    strUpdateSQL += "Secrecy = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->secrecy);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* IP��ַ*/
    strUpdateSQL += "IPAddress = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->ip_address;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* �˿ں� */
    strUpdateSQL += "Port = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->port);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ����*/
    strUpdateSQL += "Password = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->password;
    strUpdateSQL += "'";

    /* ��λ״̬ */
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
        && 0 == pGBDeviceInfo->three_party_flag) /* ������¼�ƽ̨�ĵ�λ������Ҫ�ȽϾ�γ�� */
    {
        /* ���� */
        if (pGBLogicDeviceInfo->longitude > 0)
        {
            strUpdateSQL += ",";

            strUpdateSQL += "Longitude = ";
            memset(strTmp1, 0 , 64);
            snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
            strUpdateSQL += strTmp1;
        }

        /* γ�� */
        if (pGBLogicDeviceInfo->latitude > 0)
        {
            strUpdateSQL += ",";

            strUpdateSQL += "Latitude = ";
            memset(strTmp1, 0 , 64);
            snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
            strUpdateSQL += strTmp1;
        }

        /* ����ͼ�� */
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

    /* �����豸������ */
    strUpdateSQL += "Resved1 = ";
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
    strUpdateSQL += strTmp1;

    strUpdateSQL += " WHERE DeviceID like '";
    strUpdateSQL += pGBLogicDeviceInfo->device_id;
    strUpdateSQL += "'";

    /* ��ѯ���ݿ� */
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

            /* �ɹ�֮����Ҫ��ȡ��Index ��߼��豸�ṹ�� */
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
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�����߼��豸���ñ�ʶ�����ݿ�:�߼��豸ID=%s, �߼���λ����=%s, ���ñ�ʶ=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBLogicDeviceInfo->enable);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Update logic device disable identification in database:logic device ID=%s, logic point name =%s, Disable identification =%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBLogicDeviceInfo->enable);

            }

            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddGBLogicDeviceInfo2DB() DB Update OK:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
        }
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : AddGBLogicDeviceInfo2DBForRoute
 ��������  : ���߼��豸��Ϣ��ӵ����ݿ�
 �������  : char * device_id
             DBOper * pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��10��10��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* 1����ѯSQL ���*/
    strQuerySQL.clear();
    strQuerySQL = "select * from GBLogicDeviceConfig WHERE DeviceID like '";
    strQuerySQL += pGBLogicDeviceInfo->device_id;
    strQuerySQL += "'";


    /* 2������SQL ���*/
    strInsertSQL.clear();
    strInsertSQL = "insert into GBLogicDeviceConfig (ID,DeviceID,CMSID,DeviceName,Enable,CtrlEnable,MicEnable,FrameCount,AlarmLengthOfTime,DeviceType,PhyDeviceIndex,PhyDeviceChannel,CtrlDeviceIndex,CtrlDeviceChannel,StreamCount,RecordType,OtherRealm,Manufacturer,Model,Owner,Block,Address,Parental,ParentID,SafetyWay,RegisterWay,CertNum,Certifiable,ErrCode,EndTime,Secrecy,IPAddress,Port,Password,Status,Longitude,Latitude,Resved2,Resved1) values (";

    /* �߼��豸����*/
    memset(strDeviceIndex, 0 , 64);
    snprintf(strDeviceIndex, 64, "%u", pGBLogicDeviceInfo->id);
    strInsertSQL += strDeviceIndex;

    strInsertSQL += ",";

    /* ��λͳһ��� */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->device_id;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ������CMS ID */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->cms_id;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ��λ���� */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->device_name;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* �Ƿ����� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->enable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* �Ƿ�ɿ� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->ctrl_enable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* �Ƿ�֧�ֶԽ� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->mic_enable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ֡�� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->frame_count);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ����ʱ�� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ��λ���� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->device_type);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ��Ӧ��ý�������豸���� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceIndex);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ��Ӧ��ý�������豸ͨ�� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceChannel);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ��Ӧ�Ŀ��������豸���� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceIndex);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ��Ӧ�Ŀ��������豸ͨ�� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceChannel);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->stream_count);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ¼������ */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->record_type);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* �Ƿ����������� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->other_realm);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* �豸������ */
    strInsertSQL += "'";

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        strInsertSQL += pGBLogicDeviceInfo->manufacturer;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";

    /* �豸�ͺ� */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->model;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* �豸���� */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->owner;
    strInsertSQL += "'";

    strInsertSQL += ",";

#if 0
    /* �������� */
    strInsertSQL += "'";

    if (NULL != pGBLogicDeviceInfo->civil_code)
    {
        strInsertSQL += pGBLogicDeviceInfo->civil_code;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";
#endif

    /* ���� */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->block;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ��װ��ַ */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->address;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* �Ƿ������豸 */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->parental);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ���豸/����/ϵͳID */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->parentID;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ���ȫģʽ*/
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->safety_way);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ע�᷽ʽ */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->register_way);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ֤�����к�*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->cert_num;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ֤����Ч��ʶ */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->certifiable);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ��Чԭ���� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->error_code);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ֤����ֹ��Ч��*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->end_time;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* �������� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->secrecy);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* IP��ַ*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->ip_address;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* �˿ں� */
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->port);
    strInsertSQL += strTmp;

    strInsertSQL += ",";

    /* ����*/
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->password;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ��λ״̬ */
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

    /* ���� */
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
    strInsertSQL += strTmp1;

    strInsertSQL += ",";

    /* γ�� */
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
    strInsertSQL += strTmp1;

    strInsertSQL += ",";

    /* ����ͼ�� */
    strInsertSQL += "'";
    strInsertSQL += pGBLogicDeviceInfo->map_layer;
    strInsertSQL += "'";

    strInsertSQL += ",";

    /* �����豸������ */
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
    strInsertSQL += strTmp1;

    strInsertSQL += ")";


    /* 3������SQL ���*/
    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE GBLogicDeviceConfig SET ";

#if 0
    /* ���� */
    strUpdateSQL += "ID = ";
    memset(strDeviceIndex, 0 , 64);
    snprintf(strDeviceIndex, 64, "%u", pGBLogicDeviceInfo->id);
    strUpdateSQL += strDeviceIndex;

    strUpdateSQL += ",";
#endif

    /* ������CMS ID */
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
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"ͨ��", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"��ǹ", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"����", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, (char*)"����", 4)
             || 0 == strncmp(pGBLogicDeviceInfo->device_name, pGBLogicDeviceInfo->device_id, MAX_ID_LEN)))
        || '\0' == pGBLogicDeviceInfo->device_name[0])
    {
        /* ���ǰ���ϱ����� IP Camera��Camera�������ƣ������ϱ���Ϊ�գ��򲻸������ݿ⣬�����ݿ�����Ϊ׼ */
    }
    else
    {
        /* ��λ���� */
        strUpdateSQL += "DeviceName = ";
        strUpdateSQL += "'";
        strUpdateSQL += pGBLogicDeviceInfo->device_name;
        strUpdateSQL += "'";

        strUpdateSQL += ",";
    }

    /* �Ƿ����� */
    strUpdateSQL += "Enable = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->enable);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* �Ƿ����������� */
    strUpdateSQL += "OtherRealm = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->other_realm);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* �Ƿ�ɿ� */
    strUpdateSQL += "CtrlEnable = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->ctrl_enable);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* �Ƿ�֧�ֶԽ� */
    strUpdateSQL += "MicEnable = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->mic_enable);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ֡�� */
    strUpdateSQL += "FrameCount = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->frame_count);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* �Ƿ�֧�ֶ����� */
    strUpdateSQL += "StreamCount = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->stream_count);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* �澯ʱ�� */
    strUpdateSQL += "AlarmLengthOfTime = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ��Ӧ��ý�������豸���� */
    strUpdateSQL += "PhyDeviceIndex = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceIndex);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

#if 0
    /* ��Ӧ��ý�������豸ͨ�� */
    strUpdateSQL += "PhyDeviceChannel = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_mediaDeviceChannel);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ��Ӧ�Ŀ��������豸���� */
    strUpdateSQL += "CtrlDeviceIndex = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceIndex);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ��Ӧ�Ŀ��������豸ͨ�� */
    strUpdateSQL += "CtrlDeviceChannel = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->phy_ctrlDeviceChannel);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* �Ƿ�֧�ֶ����� */
    strUpdateSQL += "StreamCount = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->stream_count);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ¼������ */
    strUpdateSQL += "RecordType = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->record_type);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";
#endif

    /* �豸������ */
    strUpdateSQL += "Manufacturer = ";
    strUpdateSQL += "'";

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        strUpdateSQL += pGBLogicDeviceInfo->manufacturer;
    }

    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* �豸�ͺ� */
    strUpdateSQL += "Model = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->model;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* �豸���� */
    strUpdateSQL += "Owner = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->owner;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

#if 0
    /* �������� */
    strUpdateSQL += "CivilCode = ";
    strUpdateSQL += "'";

    if (NULL != pGBLogicDeviceInfo->civil_code)
    {
        strUpdateSQL += pGBLogicDeviceInfo->civil_code;
    }

    strUpdateSQL += "'";

    strUpdateSQL += ",";
#endif

    /* ���� */
    strUpdateSQL += "Block = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->block;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* ��װ��ַ */
    strUpdateSQL += "Address = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->address;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* �Ƿ������豸 */
    strUpdateSQL += "Parental = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->parental);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ���豸/����/ϵͳID */
    strUpdateSQL += "ParentID = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->parentID;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* ���ȫģʽ*/
    strUpdateSQL += "SafetyWay = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->safety_way);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ע�᷽ʽ */
    strUpdateSQL += "RegisterWay = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->register_way);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ֤�����к�*/
    strUpdateSQL += "CertNum = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->cert_num;

    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* ֤����Ч��ʶ */
    strUpdateSQL += "Certifiable = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->certifiable);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ��Чԭ���� */
    strUpdateSQL += "ErrCode = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->error_code);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ֤����ֹ��Ч��*/
    strUpdateSQL += "EndTime = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->end_time;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* �������� */
    strUpdateSQL += "Secrecy = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->secrecy);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* IP��ַ*/
    strUpdateSQL += "IPAddress = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->ip_address;
    strUpdateSQL += "'";

    strUpdateSQL += ",";

    /* �˿ں� */
    strUpdateSQL += "Port = ";
    memset(strTmp, 0 , 16);
    snprintf(strTmp, 16, "%d", pGBLogicDeviceInfo->port);
    strUpdateSQL += strTmp;

    strUpdateSQL += ",";

    /* ����*/
    strUpdateSQL += "Password = ";
    strUpdateSQL += "'";
    strUpdateSQL += pGBLogicDeviceInfo->password;
    strUpdateSQL += "'";

    /* ��λ״̬ */
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

    /* ���� */
    if (pGBLogicDeviceInfo->longitude > 0)
    {
        strUpdateSQL += ",";

        strUpdateSQL += "Longitude = ";
        memset(strTmp1, 0 , 64);
        snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
        strUpdateSQL += strTmp1;
    }

    /* γ�� */
    if (pGBLogicDeviceInfo->longitude > 0)
    {
        strUpdateSQL += ",";

        strUpdateSQL += "Latitude = ";
        memset(strTmp1, 0 , 64);
        snprintf(strTmp1, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
        strUpdateSQL += strTmp1;

    }

    /* ����ͼ�� */
    if (pGBLogicDeviceInfo->map_layer[0] != '\0')
    {
        strUpdateSQL += ",";

        strUpdateSQL += "Resved2 = ";
        strUpdateSQL += "'";
        strUpdateSQL += pGBLogicDeviceInfo->map_layer;
        strUpdateSQL += "'";

        strUpdateSQL += ",";
    }

    /* �����豸������ */
    strUpdateSQL += "Resved1 = ";
    memset(strTmp1, 0 , 64);
    snprintf(strTmp1, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
    strUpdateSQL += strTmp1;

    strUpdateSQL += " WHERE DeviceID like '";
    strUpdateSQL += pGBLogicDeviceInfo->device_id;
    strUpdateSQL += "'";

    /* ��ѯ���ݿ� */
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

            /* �ɹ�֮����Ҫ��ȡ��Index ��߼��豸�ṹ�� */
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
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�����߼��豸���ñ�ʶ�����ݿ�:�߼��豸ID=%s, �߼���λ����=%s, ���ñ�ʶ=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBLogicDeviceInfo->enable);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Update logic device disable identification in database:logic device ID=%s, logic point name =%s, Disable identification =%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pGBLogicDeviceInfo->enable);

            }

            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "AddGBLogicDeviceInfo2DBForRoute() DB Update OK:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
        }
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : CheckIsGBLogicDeviceInfoInDB
 ��������  : ����߼��豸��Ϣ�Ƿ���������ݿ�
 �������  : char* device_id
                            DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��10��8��
    ��    ��   : �û�·����Ϣ����
    �޸�����   : �����ɺ���

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

    /* 1����ѯSQL ���*/
    strQuerySQL.clear();
    strQuerySQL = "select * from GBLogicDeviceConfig WHERE DeviceID like '";
    strQuerySQL += device_id;
    strQuerySQL += "'";

    /* ��ѯ���ݿ� */
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
 �� �� ��  : GetGBLogicDeviceRecordTypeFromDB
 ��������  : ��ȡ�߼��豸��¼������
 �������  : char* device_id
                            DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��16�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* 1����ѯSQL ���*/
    strQuerySQL.clear();
    strQuerySQL = "select RecordType from GBLogicDeviceConfig WHERE DeviceID like '";
    strQuerySQL += device_id;
    strQuerySQL += "'";

    /* ��ѯ���ݿ� */
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

    /* ¼������ */
    tmp_ivalue = 0;
    pDevice_Srv_dboper->GetFieldValue(0, tmp_ivalue);

    return tmp_ivalue;
}

/*****************************************************************************
 �� �� ��  : SendDeviceOffLineAlarmToAllClientUser
 ��������  : �����豸���߸澯�����߿ͻ����û�
 �������  : char* device_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��7�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

    iRet = SystemFaultAlarm(pGBLogicDeviceInfo->id, device_id, uType, (char*)"2", (char*)"0x01400002", "ǰ���߼��豸����:��λID=%s,��λ����=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);
    iRet = EnSystemFaultAlarm(pGBLogicDeviceInfo->id, device_id, uType, (char*)"2", (char*)"0x01400002", "The front end logic device off line:device ID=%s,device name=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : SendDeviceNoStreamAlarmToAllClientUser
 ��������  : �����豸�������澯�����߿ͻ����û�
 �������  : char * device_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��9��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    iRet = SystemFaultAlarm(pGBLogicDeviceInfo->id, device_id, uType, (char*)"2", (char*)"0x01400002", "ǰ���߼�û������:��λID=%s,��λ����=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);
    iRet = EnSystemFaultAlarm(pGBLogicDeviceInfo->id, device_id, uType, (char*)"2", (char*)"0x01400002", "The front end logic device no stream:device ID=%s,device name=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : SendIntelligentDeviceOffLineAlarmToAllClientUser
 ��������  : �������ܷ�����λ������Ϣ���û�
 �������  : char* device_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��10��23�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    iRet = SystemFaultAlarm(pGBLogicDeviceInfo->id, device_id, uType, (char*)"2", (char*)"0x01400002", "ǰ���߼��豸���ܷ���״̬����:��λID=%s,��λ����=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);
    iRet = EnSystemFaultAlarm(pGBLogicDeviceInfo->id, device_id, uType, (char*)"2", (char*)"0x01400002", "The front end logic device intelligent status off:device ID=%s,device name=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : SendGBPhyDeviceOffLineAlarmToAllClientUser
 ��������  : ���������豸������Ϣ���û�
 �������  : unsigned int uType
             unsigned int device_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��9��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    iRet = SystemFaultAlarm(device_index, pGBDeviceInfo->device_id, uType, (char*)"2", (char*)"0x01400002", "ǰ�������豸����:�����豸 ID=%s, �����豸 IP��ַ=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
    iRet = EnSystemFaultAlarm(device_index, pGBDeviceInfo->device_id, uType, (char*)"2", (char*)"0x01400002", "The front end device off line:device ID=%s, device IP addr=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : SendAllGBLogicDeviceStatusOffAlarmToAllClientUser
 ��������  : ���������豸��������������������߼��豸���߸澯�����߿ͻ���
 �������  : int device_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��7�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser
 ��������  : ���������豸��������������������߼��豸���ܷ���״̬���߸澯��
             ���߿ͻ���
 �������  : int device_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��10��23�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : SendQuerySubCMSDBIPMessage
 ��������  : ���ͻ�ȡ�¼�CMS IP��ַ��Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��3��10�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"GetDBIP");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"123");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    /* ������Ϣ */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���Ͳ�ѯ�¼�CMS ���ݿ�IP��ַMessage��Ϣ���豸ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendQuerySubCMSDBIPMessage error:dest_id=%s, dest_ip=%s, dest_port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQuerySubCMSDBIPMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���Ͳ�ѯ�¼�CMS ���ݿ�IP��ַMessage��Ϣ���豸�ɹ�:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendQuerySubCMSDBIPMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQuerySubCMSDBIPMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendExecuteDevicePresetMessageToSubCMS
 ��������  : ����ִ��Ԥ��λ��Ϣ���¼�CMS
 �������  : char* strPresetID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��28�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ִ��Ԥ��λ��Ϣ�������¼�CMS: �¼�CMS��=%d", (int)DeviceIDVector.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendExcuteResetMessageToAllSubCMS: SubCMSSum=%d", (int)DeviceIDVector.size());

    if (DeviceIDVector.size() > 0)
    {
        /* �齨XML��Ϣ */
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
                /* ������Ϣ */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ִ��Ԥ��λMessage��Ϣ���¼�CMSʧ��:�¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendExecuteDevicePresetMessageToSubCMS Error:dest_id=%s, dest_ip=%s, dest_port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendExecuteDevicePresetMessageToSubCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ִ��Ԥ��λMessage��Ϣ���¼�CMS�ɹ�:�¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
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
 �� �� ��  : SendQuerySubCMSTopologyPhyDeviceConfigMessage
 ��������  : ���ͻ�ȡ�¼�CMS�����������豸���ñ����Ϣ
 �������  : GBDevice_info_t* pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"TopologyPhyDeviceConfig");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"223");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    /* ������Ϣ */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���Ͳ�ѯ�¼�CMS���������豸����Message��Ϣ���豸ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendQuerySubCMSToPologyPhysicalDeviceMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendQuerySubCMSDBIPMessage() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���Ͳ�ѯ�¼�CMS���������豸����Message��Ϣ���豸�ɹ�:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendQuerySubCMSToPologyPhysicalDeviceMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendQuerySubCMSDBIPMessage() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendNotifyRestartMessageToSubCMS
 ��������  : CMS֪ͨ�¼�CMS����CMS��������
 �������  : GBDevice_info_t* pGBDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��7��5�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"CMSRestart");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"23456");

    AccNode = outPacket.CreateElement((char*)"CMSID");
    outPacket.SetElementValue(AccNode, local_cms_id_get());

    /* ������Ϣ */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���ͱ���CMS����֪ͨ����Message��Ϣ���豸ʧ��:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendNotifyRestartMessageToSubCMS Error:dest_id=%s, dest_ip=%s, dest_port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyRestartMessageToSubCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ͱ���CMS����֪ͨ����Message��Ϣ���豸�ɹ�:�豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyRestartMessageToSubCMS OK:dest_id=%s, dest_ip=%s, dest_port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendNotifyRestartMessageToSubCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendNotifyRestartMessageToAllSubCMS
 ��������  : ����������Ϣ�������¼�����CMS
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��11��9��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendNotifyRestartMessageToAllSubCMS()
{
    int i = 0;

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    GBDevice_Info_Iterator Itr;

    GBDevice_info_t* pProcGBDeviceInfo = NULL;
    needtoproc_GBDeviceinfo_queue needProc;

    /* �齨XML��Ϣ */
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

        if (pProcGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER && pProcGBDeviceInfo->three_party_flag == 0) /* �ǵ�����ƽ̨ */
        {
            needProc.push_back(pProcGBDeviceInfo);
        }
    }

    GBDEVICE_SMUTEX_UNLOCK();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������Ϣ�������¼�CMS: �¼�CMS��=%d", (int)needProc.size());
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
            /* ������Ϣ */
            i |= SIP_SendMessage(NULL, local_cms_id_get(), pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->strRegServerIP, pProcGBDeviceInfo->iRegServerPort, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���ͱ���CMS����֪ͨ����Message��Ϣ���¼�CMSʧ��:�¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendNotifyRestartMessageToAllSubCMS Error:dest_id=%s, dest_ip=%s, dest_port=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyRestartMessageToAllSubCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ͱ���CMS����֪ͨ����Message��Ϣ���¼�CMS�ɹ�:�¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyRestartMessageToAllSubCMS OK:dest_id=%s, dest_ip=%s, dest_port=%d", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendNotifyRestartMessageToAllSubCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pProcGBDeviceInfo->device_id, pProcGBDeviceInfo->login_ip, pProcGBDeviceInfo->login_port);
            }
        }
    }

    needProc.clear();

    return 1;
}

/*****************************************************************************
 �� �� ��  : SendTSUInfoMessageToDEC
 ��������  : ����TSU��Ϣ��DEC
 �������  : char* callee_id
             char* local_ip
             int local_port
             char* remote_ip
             int remote_port
             char* strSN
             char* device_id
             char* tsu_ip
             int tsu_port
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��6��15�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �齨XML��Ϣ */
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

    /* ������Ϣ���ϼ�CMS */
    i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, local_ip, local_port, remote_ip, remote_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����TSU IP��ַ��ϢMessage��Ϣ��������ʧ��:������ID=%s, IP��ַ=%s, �˿ں�=%d", callee_id, remote_ip, remote_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendTSUInfoMessageToDEC Error:dest_id=%s, dest_ip=%s, dest_port=%d", callee_id, remote_ip, remote_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendTSUInfoMessageToDEC() SIP_SendMessage Error :dest_id=%s, dest_ip=%s, dest_port=%d \r\n", callee_id, remote_ip, remote_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����TSU IP��ַ��ϢMessage��Ϣ���������ɹ�:������ID=%s, IP��ַ=%s, �˿ں�=%d", callee_id, remote_ip, remote_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendTSUInfoMessageToDEC OK:dest_id=%s, dest_ip=%s, dest_port=%d", callee_id, remote_ip, remote_port);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendTSUInfoMessageToDEC() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", callee_id, remote_ip, remote_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendSubscribeMessageToSubGBDevice
 ��������  : ���Ͷ�����Ϣ���¼������豸
 �������  : GBDevice_info_t* pGBDeviceInfo
             int iFlag
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��6��16�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"Catalog");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"17430");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

    if (0 == iFlag) /* ��ʼ���� */
    {
        subscribe_event_id++;
        pGBDeviceInfo->catalog_subscribe_event_id = subscribe_event_id;
        snprintf(strSN, 128, "%u", pGBDeviceInfo->catalog_subscribe_event_id);
        index = SIP_SendSubscribe(strSN, local_cms_id_get(), pGBDeviceInfo->device_id, (char*)"Catalog", pGBDeviceInfo->catalog_subscribe_event_id, local_subscribe_expires_get(), pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());
        pGBDeviceInfo->catalog_subscribe_dialog_index = index;
    }
    else if (1 == iFlag) /* ˢ�¶��� */
    {
        i = SIP_SubscribeRefresh(pGBDeviceInfo->catalog_subscribe_dialog_index);
    }
    else if (2 == iFlag) /* ȡ������ */
    {
        i = SIP_UnSubscribe(pGBDeviceInfo->catalog_subscribe_dialog_index);
    }

    pGBDeviceInfo->catalog_subscribe_flag = 0;
    pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get();

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���Ͷ���Subscribe��Ϣ��ǰ���豸ʧ��:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d, subscribe_event_id=%d, Flag=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->catalog_subscribe_event_id, iFlag);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendSubscribeMessageToSubGBDevice Error:ID=%s, IP=%s, port=%d, subscribe_event_id=%d, Flag=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->catalog_subscribe_event_id, iFlag);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendSubscribeMessageToSubGBDevice() SIP_SendSubscribe Error \r\n");
    }
    else if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���Ͷ���Subscribe��Ϣ��ǰ���豸�ɹ�:ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d, subscribe_event_id=%d, Flag=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->catalog_subscribe_event_id, iFlag);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendSubscribeMessageToSubGBDevice OK:ID=%s, IP=%s, port=%d, subscribe_event_id=%d, Flag=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->catalog_subscribe_event_id, iFlag);
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendSubscribeMessageToSubGBDevice() SIP_SendSubscribe OK \r\n");
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendRCUDeviceStatusToSubCMS
 ��������  : ����RCU�豸״̬���¼�CMS
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��10��21��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ��ȡ�õ����͵��¼�CMS�豸 */
    strSQL.clear();
    strSQL = "select GBPhyDeviceIndex from GBPhyDevicePermConfig WHERE DeviceIndex = "; /* ��ѯȨ�ޱ���ȡ����Ӧ�������豸 */
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

    /* �齨��Ϣ */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"RCUDeviceStatus");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"132");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

    /*״̬*/
    AccNode = outPacket.CreateElement((char*)"Status");
    snprintf(strStatus, 32, "%u", pGBLogicDeviceInfo->status);
    outPacket.SetElementValue(AccNode, strStatus);

    /*RCU�ϱ��ı�������*/
    AccNode = outPacket.CreateElement((char*)"AlarmPriority");
    snprintf(strAlarmPriority, 32, "%u", pGBLogicDeviceInfo->AlarmPriority);
    outPacket.SetElementValue(AccNode, strAlarmPriority);

    /*RCU�ϱ���Guard */
    AccNode = outPacket.CreateElement((char*)"Guard");
    snprintf(strGuard, 32, "%u", pGBLogicDeviceInfo->guard_type);
    outPacket.SetElementValue(AccNode, strGuard);

    /* RCU�ϱ���Value */
    AccNode = outPacket.CreateElement((char*)"Value");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->Value);

    /* RCU�ϱ���Unit */
    AccNode = outPacket.CreateElement((char*)"Unit");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->Unit);

    /* ѭ����ȡ���ݿ� */
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

        /* ��ȡ�����豸��Ϣ */
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
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����֪ͨRCU��λ״̬�仯Message��Ϣ���¼�CMSʧ��:�¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼���λID=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendRCUDeviceStatusToSubCMS Error:CMS ID=%s, IP=%s, port=%d, ID=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendRCUDeviceStatusToSubCMS() SIP_SendMessage Error \r\n");
            continue;
        }
        else if (i == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����֪ͨRCU��λ״̬�仯Message��Ϣ���¼�CMS�ɹ�:�¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼���λID=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendRCUDeviceStatusToSubCMS OK:CMS ID=%s, IP=%s, port=%d, ID=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id);
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendRCUDeviceStatusToSubCMS() SIP_SendMessage OK \r\n");
        }
    }

    return 1;
}

/*****************************************************************************
 �� �� ��  : SendRCUDeviceStatusToRouteCMS
 ��������  : �����豸״̬�������ϼ�CMS
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��10��21�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �齨��Ϣ */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"RCUDeviceStatus");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"132");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

    /*״̬*/
    AccNode = outPacket.CreateElement((char*)"Status");
    snprintf(strStatus, 32, "%u", pGBLogicDeviceInfo->status);
    outPacket.SetElementValue(AccNode, strStatus);

    /*RCU�ϱ��ı�������*/
    AccNode = outPacket.CreateElement((char*)"AlarmPriority");
    snprintf(strAlarmPriority, 32, "%u", pGBLogicDeviceInfo->AlarmPriority);
    outPacket.SetElementValue(AccNode, strAlarmPriority);

    /*RCU�ϱ���Guard */
    AccNode = outPacket.CreateElement((char*)"Guard");
    snprintf(strGuard, 32, "%u", pGBLogicDeviceInfo->guard_type);
    outPacket.SetElementValue(AccNode, strGuard);

    /* RCU�ϱ���Value */
    AccNode = outPacket.CreateElement((char*)"Value");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->Value);

    /* RCU�ϱ���Unit */
    AccNode = outPacket.CreateElement((char*)"Unit");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->Unit);

    i = SendMessageToOwnerRouteCMSExceptMMS2(pGBLogicDeviceInfo->id, NULL, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length(), pDboper);

    if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����RCU�豸״̬�仯Message��Ϣ�������ϼ�CMSʧ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Sent RCU device status change message  to the local superior CMS failure");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendRCUDeviceStatusToRouteCMS() SendMessageToOwnerRouteCMSExceptMMS2 Error \r\n");
        return -1;
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����RCU�豸״̬�仯Message��Ϣ�������ϼ�CMS�ɹ�");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Sent RCU device status change message  to the local superior CMS success");
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendRCUDeviceStatusToRouteCMS() SendMessageToOwnerRouteCMSExceptMMS2 OK \r\n");
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendAllGBLogicDeviceStatusProc
 ��������  : ���������߼��豸״̬��Ϣ�����߿ͻ����Լ��ϼ�CMS
 �������  : int device_index
             int status
             int enable
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��30��
    ��    ��   : �û�·����Ϣ����
    �޸�����   : �����ɺ���

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
        /* ���������豸״̬��Ϣ�������û�  */
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
            /* ��������ɾ����Ϣ���ϼ�CMS  */
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

            /* ��������ɾ����Ϣ���¼�CMS  */
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
            /* ���������豸״̬��Ϣ���ϼ�CMS  */
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

            /* ���������豸״̬��Ϣ���¼�CMS  */
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
 �� �� ��  : SendAllIntelligentGBLogicDeviceStatusProc
 ��������  : �����������ܷ����߼��豸��λ״̬���ͻ���
 �������  : int device_index
             int status
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��10��23�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
                /* �����豸״̬�仯��Ϣ  */
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
                    /* �����豸״̬�仯��Ϣ  */
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
 �� �� ��  : SendDeviceStatusToSubCMS
 ��������  : �����豸״̬���¼�CMS
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int status
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��10��12��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ��ȡ�õ����͵��¼�CMS�豸 */
    strSQL.clear();
    strSQL = "select GBPhyDeviceIndex from GBPhyDevicePermConfig WHERE DeviceIndex = "; /* ��ѯȨ�ޱ���ȡ����Ӧ�������豸 */
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

    /* �ظ���Ӧ,�齨��Ϣ */
    outPacket1.SetRootTag("Notify");
    AccNode1 = outPacket1.CreateElement((char*)"CmdType");
    outPacket1.SetElementValue(AccNode1, (char*)"DeviceStatus");

    AccNode1 = outPacket1.CreateElement((char*)"SN");
    outPacket1.SetElementValue(AccNode1, (char*)"132");

    AccNode1 = outPacket1.CreateElement((char*)"DeviceID");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->device_id);

    AccNode1 = outPacket1.CreateElement((char*)"Status");
    outPacket1.SetElementValue(AccNode1, (char*)strStatus1.c_str());

    /* ѭ����ȡ���ݿ� */
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

        /* ��ȡ�����豸��Ϣ */
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
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����֪ͨ��λ״̬�仯Message��Ϣ���¼�CMSʧ��:�¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼���λID=%s, ״̬=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strStatus1.c_str());
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendDeviceStatusToSubCMS Error:CMS ID=%s, IP=%s, port=%d, ID=%s, status=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strStatus1.c_str());
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceStatusToSubCMS() SIP_SendMessage Error \r\n");
            continue;
        }
        else if (i == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����֪ͨ��λ״̬�仯Message��Ϣ���¼�CMS�ɹ�:�¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼���λID=%s, ״̬=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strStatus1.c_str());
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendDeviceStatusToSubCMS OK:CMS ID=%s, IP=%s, port=%d, ID=%s, status=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strStatus1.c_str());
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendDeviceStatusToSubCMS() SIP_SendMessage OK \r\n");
        }
    }

    return 1;
}

/*****************************************************************************
 �� �� ��  : SendAllDeviceStatusToSubCMS
 ��������  : ���������豸״̬���¼�CMS
 �������  : vector<string>& DeviceIDVector
             int status
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : SendDeviceStatusMessageProc
 ��������  : �����豸״̬�仯��Ϣ����
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int status
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��8��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendDeviceStatusMessageProc(GBLogicDevice_info_t* pGBLogicDeviceInfo, int status, DBOper* pDboper)
{
    int i = 0;

    if (NULL == pGBLogicDeviceInfo || NULL == pDboper)
    {
        return -1;
    }

    /* �����豸״̬��Ϣ���ͻ��� */
    i = SendDeviceStatusToAllClientUser(pGBLogicDeviceInfo->device_id, status, pDboper);

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "SendDeviceStatusMessageProc() SendDeviceStatusToAllClientUser Error:i=%d \r\n", i);
    }
    else if (i > 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "SendDeviceStatusMessageProc() SendDeviceStatusToAllClientUser OK:i=%d \r\n", i);
    }

    /* �����豸״̬��Ϣ���ϼ�CMS  */
    i = SendDeviceStatusToRouteCMS(pGBLogicDeviceInfo->id, pGBLogicDeviceInfo->device_id, status, pDboper);

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendDeviceStatusMessageProc() SendDeviceStatusToRouteCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
    }
    else if (i > 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendDeviceStatusMessageProc() SendDeviceStatusToRouteCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
    }

    /* �����豸״̬��Ϣ���¼�CMS  */
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
 �� �� ��  : SendNotifyGroupMapCatalogTo3PartyRouteCMS
 ��������  : ���͵�λ�����ϵ�仯��Ϣ��������ƽ̨
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int iEvent
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ���͸�������ƽ̨ */
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

    /* �豸ͳһ��� */
    AccNode2 = outPacket2.CreateElement((char*)"DeviceID");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->device_id);

    /* ��λ���� */
    AccNode2 = outPacket2.CreateElement((char*)"Name");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->device_name);

    /* �豸������ */
    AccNode2 = outPacket2.CreateElement((char*)"Manufacturer");

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->manufacturer);
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, (char*)"");
    }

    /* �豸�ͺ� */
    AccNode2 = outPacket2.CreateElement((char*)"Model");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->model);

    /* �豸���� */
    AccNode2 = outPacket2.CreateElement((char*)"Owner");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->owner);

    /* �������� */
    AccNode2 = outPacket2.CreateElement((char*)"CivilCode");

    if ('\0' == pGBLogicDeviceInfo->civil_code[0])
    {
        outPacket2.SetElementValue(AccNode2, local_civil_code_get());
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->civil_code);
    }

    /* ���� */
    AccNode2 = outPacket2.CreateElement((char*)"Block");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->block);

    /* ��װ��ַ */
    AccNode2 = outPacket2.CreateElement((char*)"Address");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->address);

    /* �Ƿ������豸 */
    AccNode2 = outPacket2.CreateElement((char*)"Parental");
    snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
    outPacket2.SetElementValue(AccNode2, strParental);

    /* ���豸/����/ϵͳID, ������ƽ̨�Խӵ�ʱ��ͳһʹ�ñ���CMS ID */
    AccNode2 = outPacket2.CreateElement((char*)"ParentID");

    if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
    {
        outPacket2.SetElementValue(AccNode2, local_cms_id_get());
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->virtualParentID);
    }

    /* ���ȫģʽ*/
    AccNode2 = outPacket2.CreateElement((char*)"SafetyWay");
    snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
    outPacket2.SetElementValue(AccNode2, strSafetyWay);

    /* ע�᷽ʽ */
    AccNode2 = outPacket2.CreateElement((char*)"RegisterWay");
    snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
    outPacket2.SetElementValue(AccNode2, strRegisterWay);

    /* ֤�����к�*/
    AccNode2 = outPacket2.CreateElement((char*)"CertNum");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->cert_num);

    /* ֤����Ч��ʶ */
    AccNode2 = outPacket2.CreateElement((char*)"Certifiable");
    snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
    outPacket2.SetElementValue(AccNode2, strCertifiable);

    /* ��Чԭ���� */
    AccNode2 = outPacket2.CreateElement((char*)"ErrCode");
    snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
    outPacket2.SetElementValue(AccNode2, strErrCode);

    /* ֤����ֹ��Ч��*/
    AccNode2 = outPacket2.CreateElement((char*)"EndTime");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->end_time);

    /* �������� */
    AccNode2 = outPacket2.CreateElement((char*)"Secrecy");
    snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
    outPacket2.SetElementValue(AccNode2, strSecrecy);

    /* IP��ַ*/
    AccNode2 = outPacket2.CreateElement((char*)"IPAddress");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->ip_address);

    /* �˿ں� */
    AccNode2 = outPacket2.CreateElement((char*)"Port");
    snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
    outPacket2.SetElementValue(AccNode2, strPort);

    /* ����*/
    AccNode2 = outPacket2.CreateElement((char*)"Password");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->password);

    /* ��λ״̬ */
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

    /* ���� */
    AccNode2 = outPacket2.CreateElement((char*)"Longitude");
    snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
    outPacket2.SetElementValue(AccNode2, strLongitude);

    /* γ�� */
    AccNode2 = outPacket2.CreateElement((char*)"Latitude");
    snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
    outPacket2.SetElementValue(AccNode2, strLatitude);

    /* ��չ��Info�ֶ� */
    outPacket2.SetCurrentElement(ItemAccNode2);
    ItemInfoNode2 = outPacket2.CreateElement((char*)"Info");
    outPacket2.SetCurrentElement(ItemInfoNode2);

    /* �Ƿ�ɿ� */
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
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Ŀ¼�����ϵ�仯Notify��Ϣ���������ϼ�CMSʧ��");
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyGroupMapCatalogTo3PartyRouteCMS() SendNotifyTo3PartyRouteCMS2 Error \r\n");
        return -1;
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Ŀ¼�����ϵ�仯Notify��Ϣ���������ϼ�CMS�ɹ�");
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendNotifyGroupMapCatalogTo3PartyRouteCMS() SendNotifyTo3PartyRouteCMS2 OK \r\n");
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendNotifyCatalogToSubCMS
 ��������  : ���͵�λ�仯��Ϣ���¼�CMSƽ̨
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int iEvent
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��10��12��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ��ȡ�õ����͵��¼�CMS�豸 */
    strSQL.clear();
    strSQL = "select GBPhyDeviceIndex from GBPhyDevicePermConfig WHERE DeviceIndex = "; /* ��ѯȨ�ޱ���ȡ����Ӧ�������豸 */
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

    /* �ظ���Ӧ,�齨��Ϣ */
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

    /*RCU�ϱ��ı�������*/
    AccNode = outPacket.CreateElement((char*)"AlarmPriority");
    snprintf(strAlarmPriority, 32, "%d", pGBLogicDeviceInfo->AlarmPriority);
    outPacket.SetElementValue(AccNode, strAlarmPriority);

    /*RCU�ϱ���Guard*/
    AccNode = outPacket.CreateElement((char*)"Guard");
    snprintf(strGuard, 32, "%d", pGBLogicDeviceInfo->guard_type);
    outPacket.SetElementValue(AccNode, strGuard);

    /* RCU�ϱ���Value */
    AccNode = outPacket.CreateElement((char*)"Value");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->Value);

    /* RCU�ϱ���Unit */
    AccNode = outPacket.CreateElement((char*)"Unit");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->Unit);

    /* �豸���� */
    AccNode = outPacket.CreateElement((char*)"ID");
    snprintf(strID, 64, "%u", pGBLogicDeviceInfo->id);
    outPacket.SetElementValue(AccNode, strID);

    /* �豸ͳһ��� */
    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

    /* ��λ���� */
    AccNode = outPacket.CreateElement((char*)"Name");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

    /* �Ƿ�����*/
    AccNode = outPacket.CreateElement((char*)"Enable");

    if (0 == pGBLogicDeviceInfo->enable)
    {
        outPacket.SetElementValue(AccNode, (char*)"0");
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"1");
    }

    /* �Ƿ�ɿ� */
    AccNode = outPacket.CreateElement((char*)"CtrlEnable");

    if (1 == pGBLogicDeviceInfo->ctrl_enable)
    {
        outPacket.SetElementValue(AccNode, (char*)"Enable");
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"Disable");
    }

    /* �Ƿ�֧�ֶԽ� */
    AccNode = outPacket.CreateElement((char*)"MicEnable");

    if (0 == pGBLogicDeviceInfo->mic_enable)
    {
        outPacket.SetElementValue(AccNode, (char*)"Disable");
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"Enable");
    }

    /* ֡�� */
    AccNode = outPacket.CreateElement((char*)"FrameCount");
    snprintf(strFrameCount, 16, "%d", pGBLogicDeviceInfo->frame_count);
    outPacket.SetElementValue(AccNode, strFrameCount);

    /* �Ƿ�֧�ֶ����� */
    AccNode = outPacket.CreateElement((char*)"StreamCount");
    snprintf(strStreamCount, 16, "%d", pGBLogicDeviceInfo->stream_count);
    outPacket.SetElementValue(AccNode, strStreamCount);

    /* �澯ʱ�� */
    AccNode = outPacket.CreateElement((char*)"AlarmLengthOfTime");
    snprintf(strAlarmLengthOfTime, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
    outPacket.SetElementValue(AccNode, strAlarmLengthOfTime);

    /* �豸������ */
    AccNode = outPacket.CreateElement((char*)"Manufacturer");

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->manufacturer);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    /* �豸�ͺ� */
    AccNode = outPacket.CreateElement((char*)"Model");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->model);

    /* �豸���� */
    AccNode = outPacket.CreateElement((char*)"Owner");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->owner);

    /* �������� */
    AccNode = outPacket.CreateElement((char*)"CivilCode");

    if ('\0' == pGBLogicDeviceInfo->civil_code[0])
    {
        outPacket.SetElementValue(AccNode, local_civil_code_get());
    }
    else
    {
        outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->civil_code);
    }

    /* ���� */
    AccNode = outPacket.CreateElement((char*)"Block");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->block);

    /* ��װ��ַ */
    AccNode = outPacket.CreateElement((char*)"Address");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->address);

    /* �Ƿ������豸 */
    AccNode = outPacket.CreateElement((char*)"Parental");
    snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
    outPacket.SetElementValue(AccNode, strParental);

    /* ���豸/����/ϵͳID, ������ƽ̨�Խӵ�ʱ��ͳһʹ�ñ���CMS ID */
    AccNode = outPacket.CreateElement((char*)"ParentID");

    if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
    {
        outPacket.SetElementValue(AccNode, local_cms_id_get());
    }
    else
    {
        outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->virtualParentID);
    }

    /* ���ȫģʽ*/
    AccNode = outPacket.CreateElement((char*)"SafetyWay");
    snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
    outPacket.SetElementValue(AccNode, strSafetyWay);

    /* ע�᷽ʽ */
    AccNode = outPacket.CreateElement((char*)"RegisterWay");
    snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
    outPacket.SetElementValue(AccNode, strRegisterWay);

    /* ֤�����к�*/
    AccNode = outPacket.CreateElement((char*)"CertNum");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->cert_num);

    /* ֤����Ч��ʶ */
    AccNode = outPacket.CreateElement((char*)"Certifiable");
    snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
    outPacket.SetElementValue(AccNode, strCertifiable);

    /* ��Чԭ���� */
    AccNode = outPacket.CreateElement((char*)"ErrCode");
    snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
    outPacket.SetElementValue(AccNode, strErrCode);

    /* ֤����ֹ��Ч��*/
    AccNode = outPacket.CreateElement((char*)"EndTime");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->end_time);

    /* �������� */
    AccNode = outPacket.CreateElement((char*)"Secrecy");
    snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
    outPacket.SetElementValue(AccNode, strSecrecy);

    /* IP��ַ*/
    AccNode = outPacket.CreateElement((char*)"IPAddress");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->ip_address);

    /* �˿ں� */
    AccNode = outPacket.CreateElement((char*)"Port");
    snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
    outPacket.SetElementValue(AccNode, strPort);

    /* ����*/
    AccNode = outPacket.CreateElement((char*)"Password");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->password);

    /* ��λ״̬ */
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

    /* ���� */
    AccNode = outPacket.CreateElement((char*)"Longitude");
    snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
    outPacket.SetElementValue(AccNode, strLongitude);

    /* γ�� */
    AccNode = outPacket.CreateElement((char*)"Latitude");
    snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
    outPacket.SetElementValue(AccNode, strLatitude);

    /* ����ͼ�� */
    AccNode = outPacket.CreateElement((char*)"MapLayer");
    outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->map_layer);

    /* �����豸������ */
    AccNode = outPacket.CreateElement((char*)"ChlType");
    snprintf(strAlarmDeviceSubType, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
    outPacket.SetElementValue(AccNode, strAlarmDeviceSubType);

    /* ������CMS ID */
    AccNode = outPacket.CreateElement((char*)"CMSID");
    outPacket.SetElementValue(AccNode, local_cms_id_get());

    /* ��չ��Info�ֶ� */
    outPacket.SetCurrentElement(ItemAccNode);
    ItemInfoNode = outPacket.CreateElement((char*)"Info");
    outPacket.SetCurrentElement(ItemInfoNode);

    /* �Ƿ�ɿ� */
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

    /* ѭ����ȡ���ݿ� */
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

        /* ��ȡ�����豸��Ϣ */
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
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����֪ͨĿ¼�仯Message��Ϣ���¼�CMSʧ��:�¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼���λID=%s, �¼�=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strEvent.c_str());
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendNotifyCatalogToSubCMS Error:CMS ID=%s, IP=%s, port=%d, ID=%s, event=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strEvent.c_str());
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogToSubCMS() SIP_SendMessage Error \r\n");
            continue;
        }
        else if (i == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����֪ͨĿ¼�仯Message��Ϣ���¼�CMS�ɹ�:�¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �߼���λID=%s, �¼�=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strEvent.c_str());
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyCatalogToSubCMS OK:CMS ID=%s, IP=%s, port=%d, ID=%s, event=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBLogicDeviceInfo->device_id, (char*)strEvent.c_str());
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendNotifyCatalogToSubCMS() SIP_SendMessage OK \r\n");
        }
    }

    return 1;
}

/*****************************************************************************
 �� �� ��  : SendAllNotifyCatalogToSubCMS
 ��������  : �������е�λCatalog���¼�ƽ̨
 �������  : vector<string>& DeviceIDVector
             int iEvent
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : SendNotifyCatalogToRouteCMS
 ��������  : ����Ŀ¼�仯��Ϣ���ϼ�ƽ̨
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int iEvent
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��10��12��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �ظ���Ӧ,�齨��Ϣ */
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

    /* �豸���� */
    AccNode1 = outPacket1.CreateElement((char*)"ID");
    snprintf(strID, 64, "%u", pGBLogicDeviceInfo->id);
    outPacket1.SetElementValue(AccNode1, strID);

    /* �豸ͳһ��� */
    AccNode1 = outPacket1.CreateElement((char*)"DeviceID");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->device_id);

    /* ��λ���� */
    AccNode1 = outPacket1.CreateElement((char*)"Name");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->device_name);

    /* �Ƿ�����*/
    AccNode1 = outPacket1.CreateElement((char*)"Enable");

    if (0 == pGBLogicDeviceInfo->enable)
    {
        outPacket1.SetElementValue(AccNode1, (char*)"0");
    }
    else
    {
        outPacket1.SetElementValue(AccNode1, (char*)"1");
    }

    /* �Ƿ�ɿ� */
    AccNode1 = outPacket1.CreateElement((char*)"CtrlEnable");

    if (1 == pGBLogicDeviceInfo->ctrl_enable)
    {
        outPacket1.SetElementValue(AccNode1, (char*)"Enable");
    }
    else
    {
        outPacket1.SetElementValue(AccNode1, (char*)"Disable");
    }

    /* �Ƿ�֧�ֶԽ� */
    AccNode1 = outPacket1.CreateElement((char*)"MicEnable");

    if (0 == pGBLogicDeviceInfo->mic_enable)
    {
        outPacket1.SetElementValue(AccNode1, (char*)"Disable");
    }
    else
    {
        outPacket1.SetElementValue(AccNode1, (char*)"Enable");
    }

    /* ֡�� */
    AccNode1 = outPacket1.CreateElement((char*)"FrameCount");
    snprintf(strFrameCount, 16, "%d", pGBLogicDeviceInfo->frame_count);
    outPacket1.SetElementValue(AccNode1, strFrameCount);

    /* �Ƿ�֧�ֶ����� */
    AccNode1 = outPacket1.CreateElement((char*)"StreamCount");
    snprintf(strStreamCount, 16, "%d", pGBLogicDeviceInfo->stream_count);
    outPacket1.SetElementValue(AccNode1, strStreamCount);

    /* �澯ʱ�� */
    AccNode1 = outPacket1.CreateElement((char*)"AlarmLengthOfTime");
    snprintf(strAlarmLengthOfTime, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
    outPacket1.SetElementValue(AccNode1, strAlarmLengthOfTime);

    /* �豸������ */
    AccNode1 = outPacket1.CreateElement((char*)"Manufacturer");

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->manufacturer);
    }
    else
    {
        outPacket1.SetElementValue(AccNode1, (char*)"");
    }

    /* �豸�ͺ� */
    AccNode1 = outPacket1.CreateElement((char*)"Model");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->model);

    /* �豸���� */
    AccNode1 = outPacket1.CreateElement((char*)"Owner");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->owner);

    /* �������� */
    AccNode1 = outPacket1.CreateElement((char*)"CivilCode");

    if ('\0' == pGBLogicDeviceInfo->civil_code[0])
    {
        outPacket1.SetElementValue(AccNode1, local_civil_code_get());
    }
    else
    {
        outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->civil_code);
    }

    /* ���� */
    AccNode1 = outPacket1.CreateElement((char*)"Block");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->block);

    /* ��װ��ַ */
    AccNode1 = outPacket1.CreateElement((char*)"Address");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->address);

    /* �Ƿ������豸 */
    AccNode1 = outPacket1.CreateElement((char*)"Parental");
    snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
    outPacket1.SetElementValue(AccNode1, strParental);

    /* ���豸/����/ϵͳID, ������ƽ̨�Խӵ�ʱ��ͳһʹ�ñ���CMS ID */
    AccNode1 = outPacket1.CreateElement((char*)"ParentID");

    if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
    {
        outPacket1.SetElementValue(AccNode1, local_cms_id_get());
    }
    else
    {
        outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->virtualParentID);
    }

    /* ���ȫģʽ*/
    AccNode1 = outPacket1.CreateElement((char*)"SafetyWay");
    snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
    outPacket1.SetElementValue(AccNode1, strSafetyWay);

    /* ע�᷽ʽ */
    AccNode1 = outPacket1.CreateElement((char*)"RegisterWay");
    snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
    outPacket1.SetElementValue(AccNode1, strRegisterWay);

    /* ֤�����к�*/
    AccNode1 = outPacket1.CreateElement((char*)"CertNum");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->cert_num);

    /* ֤����Ч��ʶ */
    AccNode1 = outPacket1.CreateElement((char*)"Certifiable");
    snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
    outPacket1.SetElementValue(AccNode1, strCertifiable);

    /* ��Чԭ���� */
    AccNode1 = outPacket1.CreateElement((char*)"ErrCode");
    snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
    outPacket1.SetElementValue(AccNode1, strErrCode);

    /* ֤����ֹ��Ч��*/
    AccNode1 = outPacket1.CreateElement((char*)"EndTime");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->end_time);

    /* �������� */
    AccNode1 = outPacket1.CreateElement((char*)"Secrecy");
    snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
    outPacket1.SetElementValue(AccNode1, strSecrecy);

    /* IP��ַ*/
    AccNode1 = outPacket1.CreateElement((char*)"IPAddress");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->ip_address);

    /* �˿ں� */
    AccNode1 = outPacket1.CreateElement((char*)"Port");
    snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
    outPacket1.SetElementValue(AccNode1, strPort);

    /* ����*/
    AccNode1 = outPacket1.CreateElement((char*)"Password");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->password);

    /* ��λ״̬ */
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

    /* ���� */
    AccNode1 = outPacket1.CreateElement((char*)"Longitude");
    snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
    outPacket1.SetElementValue(AccNode1, strLongitude);

    /* γ�� */
    AccNode1 = outPacket1.CreateElement((char*)"Latitude");
    snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
    outPacket1.SetElementValue(AccNode1, strLatitude);

    /* ����ͼ�� */
    AccNode1 = outPacket1.CreateElement((char*)"MapLayer");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->map_layer);

    /* �����豸������ */
    AccNode1 = outPacket1.CreateElement((char*)"ChlType");
    snprintf(strAlarmDeviceSubType, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
    outPacket1.SetElementValue(AccNode1, strAlarmDeviceSubType);

    /* ������CMS ID */
    AccNode1 = outPacket1.CreateElement((char*)"CMSID");
    outPacket1.SetElementValue(AccNode1, local_cms_id_get());

    /*RCU�ϱ��ı�������*/
    AccNode1 = outPacket1.CreateElement((char*)"AlarmPriority");
    snprintf(strAlarmPriority, 32, "%u", pGBLogicDeviceInfo->AlarmPriority);
    outPacket1.SetElementValue(AccNode1, strAlarmPriority);

    /*RCU�ϱ���Guard */
    AccNode1 = outPacket1.CreateElement((char*)"Guard");
    snprintf(strGuard, 32, "%u", pGBLogicDeviceInfo->guard_type);
    outPacket1.SetElementValue(AccNode1, strGuard);

    /* RCU�ϱ���Value */
    AccNode1 = outPacket1.CreateElement((char*)"Value");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->Value);

    /* RCU�ϱ���Unit */
    AccNode1 = outPacket1.CreateElement((char*)"Unit");
    outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->Unit);

    /* ��չ��Info�ֶ� */
    outPacket1.SetCurrentElement(ItemAccNode1);
    ItemInfoNode1 = outPacket1.CreateElement((char*)"Info");
    outPacket1.SetCurrentElement(ItemInfoNode1);

    /* �Ƿ�ɿ� */
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
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Ŀ¼�仯Message��Ϣ�������ϼ�CMSʧ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendNotifyCatalogToRouteCMS Error");
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogEventToRouteCMS() SendNotifyCatalogToRouteCMS Error \r\n");
        return -1;
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Ŀ¼�仯Message��Ϣ�������ϼ�CMS�ɹ�");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyCatalogToRouteCMS OK");
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendNotifyCatalogEventToRouteCMS() SendNotifyCatalogToRouteCMS OK \r\n");
    }

    /* ���͸�������ƽ̨ */
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

    /* �豸ͳһ��� */
    AccNode2 = outPacket2.CreateElement((char*)"DeviceID");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->device_id);

    /* ��λ���� */
    AccNode2 = outPacket2.CreateElement((char*)"Name");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->device_name);

    /* �豸������ */
    AccNode2 = outPacket2.CreateElement((char*)"Manufacturer");

    if (NULL != pGBLogicDeviceInfo->manufacturer)
    {
        outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->manufacturer);
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, (char*)"");
    }

    /* �豸�ͺ� */
    AccNode2 = outPacket2.CreateElement((char*)"Model");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->model);

    /* �豸���� */
    AccNode2 = outPacket2.CreateElement((char*)"Owner");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->owner);

    /* �������� */
    AccNode2 = outPacket2.CreateElement((char*)"CivilCode");

    if ('\0' == pGBLogicDeviceInfo->civil_code[0])
    {
        outPacket2.SetElementValue(AccNode2, local_civil_code_get());
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->civil_code);
    }

    /* ���� */
    AccNode2 = outPacket2.CreateElement((char*)"Block");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->block);

    /* ��װ��ַ */
    AccNode2 = outPacket2.CreateElement((char*)"Address");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->address);

    /* �Ƿ������豸 */
    AccNode2 = outPacket2.CreateElement((char*)"Parental");
    snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
    outPacket2.SetElementValue(AccNode2, strParental);

    /* ���豸/����/ϵͳID, ������ƽ̨�Խӵ�ʱ��ͳһʹ�ñ���CMS ID */
    AccNode2 = outPacket2.CreateElement((char*)"ParentID");

    if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
    {
        outPacket2.SetElementValue(AccNode2, local_cms_id_get());
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->virtualParentID);
    }

    /* ���ȫģʽ*/
    AccNode2 = outPacket2.CreateElement((char*)"SafetyWay");
    snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
    outPacket2.SetElementValue(AccNode2, strSafetyWay);

    /* ע�᷽ʽ */
    AccNode2 = outPacket2.CreateElement((char*)"RegisterWay");
    snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
    outPacket2.SetElementValue(AccNode2, strRegisterWay);

    /* ֤�����к�*/
    AccNode2 = outPacket2.CreateElement((char*)"CertNum");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->cert_num);

    /* ֤����Ч��ʶ */
    AccNode2 = outPacket2.CreateElement((char*)"Certifiable");
    snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
    outPacket2.SetElementValue(AccNode2, strCertifiable);

    /* ��Чԭ���� */
    AccNode2 = outPacket2.CreateElement((char*)"ErrCode");
    snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
    outPacket2.SetElementValue(AccNode2, strErrCode);

    /* ֤����ֹ��Ч��*/
    AccNode2 = outPacket2.CreateElement((char*)"EndTime");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->end_time);

    /* �������� */
    AccNode2 = outPacket2.CreateElement((char*)"Secrecy");
    snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
    outPacket2.SetElementValue(AccNode2, strSecrecy);

    /* IP��ַ*/
    AccNode2 = outPacket2.CreateElement((char*)"IPAddress");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->ip_address);

    /* �˿ں� */
    AccNode2 = outPacket2.CreateElement((char*)"Port");
    snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
    outPacket2.SetElementValue(AccNode2, strPort);

    /* ����*/
    AccNode2 = outPacket2.CreateElement((char*)"Password");
    outPacket2.SetElementValue(AccNode2, pGBLogicDeviceInfo->password);

    /* ��λ״̬ */
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

    /* ���� */
    AccNode2 = outPacket2.CreateElement((char*)"Longitude");
    snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
    outPacket2.SetElementValue(AccNode2, strLongitude);

    /* γ�� */
    AccNode2 = outPacket2.CreateElement((char*)"Latitude");
    snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
    outPacket2.SetElementValue(AccNode2, strLatitude);

    /* ��չ��Info�ֶ� */
    outPacket2.SetCurrentElement(ItemAccNode2);
    ItemInfoNode2 = outPacket2.CreateElement((char*)"Info");
    outPacket2.SetCurrentElement(ItemInfoNode2);

    /* �Ƿ�ɿ� */
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
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Ŀ¼�仯Message��Ϣ���������ϼ�CMSʧ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendNotifyCatalogToRouteCMS Error");
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SendNotifyCatalogToRouteCMS() SendNotifyTo3PartyRouteCMS2 Error \r\n");
        return -1;
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Ŀ¼�仯Message��Ϣ���������ϼ�CMS�ɹ�");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyCatalogToRouteCMS OK");
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "SendNotifyCatalogToRouteCMS() SendNotifyTo3PartyRouteCMS2 OK \r\n");
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendNotifyCatalogMessageToAllRoute
 ��������  : ����Catalog�仯֪ͨ�¼���Ϣ����
 �������  : GBLogicDevice_info_t* pGBLogicDeviceInfo
             int iEvent
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��8��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendNotifyCatalogMessageToAllRoute(GBLogicDevice_info_t* pGBLogicDeviceInfo, int iEvent, DBOper* ptDBoper)
{
    int i = 0;

    if (NULL == pGBLogicDeviceInfo || NULL == ptDBoper)
    {
        return -1;
    }

    /* ���������Ϣ���ϼ�CMS  */
    i |= SendNotifyCatalogToRouteCMS(pGBLogicDeviceInfo, iEvent, ptDBoper);

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "SendNotifyCatalogMessageToAllRoute() SendNotifyCatalogToRouteCMS Error:iRet=%d \r\n", i);
    }
    else if (i > 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE,  "SendNotifyCatalogMessageToAllRoute() SendNotifyCatalogToRouteCMS OK:iRet=%d \r\n", i);
    }

    /* ���������Ϣ���¼�CMS  */
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
 �� �� ��  : ExecuteDevicePresetByPresetID
 ��������  : ����Ԥ��λIDִ��Ԥ��λ
 �������  : unsigned int iPresetID
                            DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��4��10�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ����Preset ID����ѯԤ��λ����ȡԤ��λ�ľ������� */
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

        /* ������ͬ���д�CMS�е�Ԥ��λ����Ҫ���͸���CMS */
        iRet = SendExecuteDevicePresetMessageToSubCMS(strPresetID);

        return iRet;
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "ExecuteDevicePresetByPresetID() record_count=%d\r\n", record_count);

    /* ѭ����ȡ��Ѳ�������� */
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

        /* ��ȡĿ�Ķ˵��豸��Ϣ */
        pDestGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(uDeviceIndex);

        if (NULL != pDestGBLogicDeviceInfo)
        {
            /* �����߼��豸����������жϣ�������Ϣ���� */
            if (1 == pDestGBLogicDeviceInfo->other_realm)
            {
                /* �����ϼ�·����Ϣ */
                iCalleeRoutePos = route_info_find(pDestGBLogicDeviceInfo->cms_id);

                if (iCalleeRoutePos >= 0)
                {
                    pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

                    if (NULL != pCalleeRouteInfo)
                    {
                        i = SendExecuteDevicePresetMessageToRoute(pDestGBLogicDeviceInfo, pCalleeRouteInfo, uPresetID);

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ִ�е�λԤ��λ, ���͵��ϼ�CMSʧ��:��λID=%s, ��λ����=%s, Ԥ��λID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ExcutePointPreset, SendToSuperiorCMS Error:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ�е�λԤ��λ, ���͵��ϼ�CMS�ɹ�:��λID=%s, ��λ����=%s, Ԥ��λID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ExcutePointPreset, SendToSuperiorCMS OK:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);

                            /* �����Զ���λԤ��λ��Ϣ��ִ��Ԥ��λ֮����Ҫ�Զ���Ϊ���� */
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
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ִ�е�λԤ��λ, ���͵�ǰ��ʧ��:��λID=%s, ��λ����=%s, Ԥ��λID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ExcutePointPreset, SendToSuperiorCMS Error:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ�е�λԤ��λ, ���͵�ǰ�˳ɹ�:��λID=%s, ��λ����=%s, Ԥ��λID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ExcutePointPreset, SendToSuperiorCMS OK:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);

                        /* �����Զ���λԤ��λ��Ϣ��ִ��Ԥ��λ֮����Ҫ�Զ���Ϊ���� */
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
 �� �� ��  : ExecuteDeviceAlarmRecord
 ��������  : ֪ͨTSUִ�б���¼��
 �������  : unsigned int uDeviceIndex
             int flag:0��ʶֹͣ����¼��1����ʶ��ʼ����¼��
             DBOper * pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��11��3��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
        && EV9000_RECORD_TYPE_NORMAL == pRecordInfo->record_type) /* ����¼�����ӵ�¼�񣬷���ͨ���õ�¼��, ֻ��Ҫ�������ñ���¼��ӿ� */
    {
        if (0 == flag) /* ֹͣ����¼�� */
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

            /* �Ѿ���ʼ�ı���¼�����Ҫֹͣ����¼�� */
            if (1 == pRecordInfo->iTSUAlarmRecordStatus)
            {
                pRecordInfo->iTSUAlarmRecordStatus = 0;
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record: TSU Already Stopped Alarm Record:tsu_ip=%s, task_id=%s, flag=%d", pCrData->tsu_ip, pCrData->task_id, flag);
            }

            /* ֪ͨTSU��ͣ¼�� */
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
        else if (1 == flag) /* ��ʼ����¼�� */
        {
            /* ֪ͨTSU�ָ�¼�� */
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

            /* �Ѿ�ֹͣ����¼�����Ҫ��ʼ����¼�� */
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
    else /* ���õ���ͨ¼�� */
    {
        if (1 == pRecordInfo->TimeOfAllWeek) /* ȫ��¼�������£�ֻ��Ҫ�������ñ���¼��ӿ� */
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

            if (0 == flag) /* ֹͣ����¼�� */
            {
                /* �Ѿ���ʼ�ı���¼�����Ҫֹͣ����¼�� */
                if (1 == pRecordInfo->iTSUAlarmRecordStatus)
                {
                    pRecordInfo->iTSUAlarmRecordStatus = 0;
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record: TSU Already Stopped Alarm Record:tsu_ip=%s, task_id=%s, flag=%d", pCrData->tsu_ip, pCrData->task_id, flag);
                }
            }
            else if (1 == flag) /* ��ʼ����¼�� */
            {
                /* �Ѿ�ֹͣ����¼�����Ҫ��ʼ����¼�� */
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
        else /* ��ȫ��¼�������£���Ҫ�жϸ�ʱ���Ƿ�����¼�� */
        {
            if (0 == flag) /* ֹͣ¼�� */
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

                /* �Ѿ���ʼ�ı���¼�����Ҫֹͣ����¼�� */
                if (1 == pRecordInfo->iTSUAlarmRecordStatus)
                {
                    pRecordInfo->iTSUAlarmRecordStatus = 0;
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "ExecuteDeviceAlarmRecord() notify_tsu_set_alarm_record: TSU Already Stopped Alarm Record:tsu_ip=%s, task_id=%s, flag=%d", pCrData->tsu_ip, pCrData->task_id, flag);
                }

                /* �ж��Ƿ���Ҫ֪ͨTSU��ͣ¼�� */
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
            else if (1 == flag) /* ��ʼ¼�� */
            {
                /* �ж��Ƿ���Ҫ֪ͨTSU�ָ�¼�� */
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

                /* �Ѿ�ֹͣ����Ҫ��ʼ¼�� */
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
 �� �� ��  : ExecuteDevicePresetByPresetIDAndDeviceIndex
 ��������  : ����Ԥ��λID���߼��豸����ִ��Ԥ��λ
 �������  : unsigned int uPresetID
             unsigned int uDeviceIndex
             DBOper * pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��7��11�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ����Preset ID����ѯԤ��λ����ȡԤ��λ�ľ������� */
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

    /* ѭ����ȡ��Ѳ�������� */
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

        /* ��ȡĿ�Ķ˵��豸��Ϣ */
        pDestGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(uDeviceIndex);

        if (NULL != pDestGBLogicDeviceInfo)
        {
            /* �����߼��豸����������жϣ�������Ϣ���� */
            if (1 == pDestGBLogicDeviceInfo->other_realm)
            {
                /* �����ϼ�·����Ϣ */
                iCalleeRoutePos = route_info_find(pDestGBLogicDeviceInfo->cms_id);

                if (iCalleeRoutePos >= 0)
                {
                    pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

                    if (NULL != pCalleeRouteInfo)
                    {
                        i = SendExecuteDevicePresetMessageToRoute(pDestGBLogicDeviceInfo, pCalleeRouteInfo, uPresetID);

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ִ�е�λԤ��λ, ���͵��ϼ�CMSʧ��:��λID=%s, ��λ����=%s, Ԥ��λID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ExcutePointPreset, SendToSuperiorCMS Error:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ�е�λԤ��λ, ���͵��ϼ�CMS�ɹ�:��λID=%s, ��λ����=%s, Ԥ��λID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ExcutePointPreset, SendToSuperiorCMS OK:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);

                            /* �����Զ���λԤ��λ��Ϣ��ִ��Ԥ��λ֮����Ҫ�Զ���Ϊ���� */
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
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ִ�е�λԤ��λ, ���͵�ǰ��ʧ��:��λID=%s, ��λ����=%s, Ԥ��λID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ExcutePointPreset, SendToSuperiorCMS Error:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ִ�е�λԤ��λ, ���͵�ǰ�˳ɹ�:��λID=%s, ��λ����=%s, Ԥ��λID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ExcutePointPreset, SendToSuperiorCMS OK:Device ID=%s, name=%s, Preset ID=%u", pDestGBLogicDeviceInfo->device_id, pDestGBLogicDeviceInfo->device_name, uPresetID);

                        /* �����Զ���λԤ��λ��Ϣ��ִ��Ԥ��λ֮����Ҫ�Զ���Ϊ���� */
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
 �� �� ��  : ExecuteControlRCUDevice
 ��������  : ִ��RCU���������λ����
 �������  : unsigned int uDeviceIndex
             char* ctrl_value
             DBOper * pDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��1��3��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���Ϳ���RCU�豸�������Message��Ϣ��ǰ���豸ʧ��:�߼���λ����=%u, ԭ��=��ȡ������λ��Ϣʧ��", uDeviceIndex);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "ExecuteControlRCUDevice() exit---: GBLogicDevice_info_find_by_device_index Error:uDeviceIndex=%u \r\n", uDeviceIndex);
        return -1;
    }

    pDestGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

    if (NULL != pDestGBDeviceInfo)
    {
        /* �齨XML��Ϣ */
        outPacket.SetRootTag("Control");
        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"LogicDeviceControl");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, (char*)"3476");

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

        AccNode = outPacket.CreateElement((char*)"Value");
        outPacket.SetElementValue(AccNode, ctrl_value);

        /* ������Ϣ */
        iRet = SIP_SendMessage(NULL, local_cms_id_get(), pGBLogicDeviceInfo->device_id, pDestGBDeviceInfo->strRegServerIP, pDestGBDeviceInfo->iRegServerPort, pDestGBDeviceInfo->login_ip, pDestGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (iRet != 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���Ϳ���RCU�豸�������Message��Ϣ��ǰ���豸ʧ��:�߼���λID=%s, ��λ����=%s, ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->login_ip, pDestGBDeviceInfo->login_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to send message that control RCU:Logic ID=%s, Name=%s, ID=%s, IP=%s, port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->login_ip, pDestGBDeviceInfo->login_port);

            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteControlRCUDevice() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->login_ip, pDestGBDeviceInfo->login_port);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���Ϳ���RCU�豸�������Message��Ϣ��ǰ���豸�ɹ�:�߼���λID=%s, ��λ����=%s, ǰ���豸ID=%s, IP��ַ=%s, �˿ں�=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->login_ip, pDestGBDeviceInfo->login_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Succeed to send message that control RCU:Logic ID=%s, Name=%s, ID=%s, IP=%s, port=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->login_ip, pDestGBDeviceInfo->login_port);
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "ExecuteControlRCUDevice() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->login_ip, pDestGBDeviceInfo->login_port);
        }
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���Ϳ���RCU�豸�������Message��Ϣ��ǰ���豸ʧ��:�߼���λID=%s, ��λ����=%s, ԭ��=��ȡ������λ��Ӧ�������豸��Ϣʧ��", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "ExecuteControlRCUDevice() Get DestGBDeviceInfo Error:device_id=%s \r\n", pGBLogicDeviceInfo->device_id);
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : ShowGBDeviceInfo
 ��������  : �鿴�����豸��Ϣ
 �������  : int sock
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��27�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : ShowLogicDeviceInfo
 ��������  : �鿴�߼��豸��Ϣ
 �������  : int sock
             int type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��12��5��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

        /* ��ȡ���������豸��Ϣ */
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

        /* ��ȡ���������豸��Ϣ */
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_SLAVE);

        if (NULL == pGBDeviceInfo)
        {
            iSlaveGBDeviceIndex = -1;
        }
        else
        {
            iSlaveGBDeviceIndex = pGBDeviceInfo->id;
        }

        /* ��ȡ���ܷ����������豸��Ϣ */
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
 �� �� ��  : checkNumberOfGBDeviceInfo
 ��������  : ���ע����豸����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��8��17�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : SendNotifyCatalogMessageToSubCMS
 ��������  : ���͵�λ��Ϣ���¼�CMS
 �������  : char* strCMSID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��10��9��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : GetGBDeviceListAndSendNotifyCatalogToSubCMS
 ��������  : ��ȡҪ���͸��¼�CMS�ĵ�λ��Ϣ�����͸��¼�CMS
 �������  : GBDevice_info_t* pGBDeviceInfo
             DBOper* pDevice_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��10��9��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GetGBDeviceListAndSendNotifyCatalogToSubCMS(GBDevice_info_t* pGBDeviceInfo, DBOper* pDevice_Srv_dboper)
{
    int i = 0;
    int index = 0;
    int record_count = 0; /* ��¼�� */
    int send_count = 0;   /* ���͵Ĵ��� */
    int query_count = 0;  /* ��ѯ����ͳ�� */
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

    /* ������е��߼��豸 */
    i = AddAllGBLogicDeviceIDToVectorForSubCMS(DeviceIDVector, pGBDeviceInfo->id, pDevice_Srv_dboper);

    /* 4����ȡ�����е��豸���� */
    record_count = DeviceIDVector.size();

    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetGBDeviceListAndSendNotifyCatalogToSubCMS() record_count=%d, pGBDeviceInfo->id=%d \r\n", record_count, pGBDeviceInfo->id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���͵�λ���¼�CMS:�¼�CMS ID=%s, �¼�CMS IP=%s, �¼�CMS Port=%d, ���͵ĵ�λ����=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, record_count);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send notify catalog message to sub cms:Sub CMS ID=%s, Sub CMS IP=%s, Sub CMS Port=%d, record count=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, record_count);

    /* 5�������¼��Ϊ0 */
    if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "GetGBDeviceListAndSendNotifyCatalogToSubCMS() exit---: No Record Count \r\n");
        return i;
    }

    notify_catalog_sn++;
    snprintf(strSN, 128, "%u", notify_catalog_sn);

    /* 6��ѭ��������������ȡ�û����豸��Ϣ������xml�� */
    CPacket* pOutPacket = NULL;

    for (index = 0; index < record_count; index++)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "RouteGetGBDeviceListAndSendCataLogToCMS() DeviceIndex=%u \r\n", device_index);

        /* �����¼������4����Ҫ�ִη��� */
        query_count++;

        /* ����XMLͷ�� */
        i = CreateGBLogicDeviceCatalogResponseXMLHeadForRoute(&pOutPacket, query_count, record_count, strSN, local_cms_id_get(), &ListAccNode);

        /* ����Item ֵ */
        i = AddLogicDeviceInfoToXMLItemForRoute(pOutPacket, ListAccNode, (char*)DeviceIDVector[index].c_str(), 0, pDevice_Srv_dboper);

        if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 0) || (query_count == record_count))
        {
            if (NULL != pOutPacket)
            {
                send_count++;

                /* ������Ϣ���¼�CMS */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�������͵�λMessage��Ϣ���¼�CMSʧ��:�¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendNotifyCatalogToSubCMS Error:SubCMS ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GetGBDeviceListAndSendNotifyCatalogToSubCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�������͵�λMessage��Ϣ���¼�CMS�ɹ�:�¼�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
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
 �� �� ��  : AddAllSubCMSIPToVector
 ��������  : ���¼�CMS������IP��ַ���뵽������
 �������  : vector<string>& SubCmsIPVector
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��11��18��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : IsIPInSubCMS
 ��������  : �鿴IP��ַ�Ƿ����¼�CMS��IP��ַ
 �������  : vector<string>& SubCmsIPVector
             char* ip_addr
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��11��18��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : checkGBDeviceIsOnline
 ��������  : ͨ��ping���ǰ���豸�Ƿ�����
 �������  : char* device_ip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��12��3��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : RemoveGBLogicDeviceLockInfoByUserInfo
 ��������  : �����û�������Ϣ�Ƴ��߼���λ��������Ϣ
 �������  : user_info_t* pUserInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��4��7��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�û�ע����¼, ���������λʧ��:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸device_index=%u, ԭ��=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, device_index, (char*)"���Զ����������Ƴ�ʧ��");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "User device control command processing, unlock point failed:User ID=%s, User IP=%s, Port=%d, device_index=%u, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, device_index, (char*)"Remove from the list of auto unlock failed.");
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "RemoveGBLogicDeviceLockInfoByUserInfo() device_auto_unlock_remove Error \r\n");
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�û�ע����¼, ���������λ�ɹ�:�û�ID=%s, IP��ַ=%s, �˿ں�=%d, �߼��豸device_index=%u", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, device_index);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User device control command processing, unlock point sucessfully:User ID=%s, User IP=%s, Port=%d, device_index=%u, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, device_index, (char*)"Remove from the list of auto unlock failed.");
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "RemoveGBLogicDeviceLockInfoByUserInfo() device_auto_unlock_remove OK \r\n");
        }
    }

    DeviceIndexVector.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : RemoveGBLogicDeviceLockInfoByRouteInfo
 ��������  : �����ϼ�ƽ̨������Ϣ�Ƴ��߼���λ��������Ϣ
 �������  : route_info_t* pRouteInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��4��7��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ϼ�ƽ̨����ʧ��, ���������λʧ��:�ϼ�CMS ID=%s, �ϼ�CMS IP��ַ=%s, �ϼ�CMS �˿ں�=%d, �߼��豸device_index=%u, ԭ��=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_index, (char*)"���Զ����������Ƴ�ʧ��");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "User device control command processing, unlock point failed:User ID=%s, User IP=%s, Port=%d, device_index=%u, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_index, (char*)"Remove from the list of auto unlock failed.");
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "RemoveGBLogicDeviceLockInfoByRouteInfo() device_auto_unlock_remove Error \r\n");
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�ƽ̨����ʧ��, ���������λ�ɹ�:�ϼ�CMS ID=%s, �ϼ�CMS IP��ַ=%s, �ϼ�CMS �˿ں�=%d, �߼��豸device_index=%u", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_index);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User device control command processing, unlock point successfully:User ID=%s, User IP=%s, Port=%d, device_index=%u, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_index, (char*)"Remove from the list of auto unlock failed.");
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "RemoveGBLogicDeviceLockInfoByRouteInfo() device_auto_unlock_remove OK \r\n");
        }
    }

    DeviceIndexVector.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : GBLogicDeviceConfig_db_refresh_proc
 ��������  : �����߼���λ���ݿ���²�����ʶ
 �������  : DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��6��23��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GBLogicDeviceConfig_db_refresh_proc()
{
    if (1 == db_GBLogicDeviceInfo_reload_mark) /* ����ִ�� */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�߼���λ���ݿ���Ϣ����ͬ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Logic point database information are synchronized");
        return 0;
    }

    db_GBLogicDeviceInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 �� �� ��  : check_GBLogicDeviceConfig_need_to_reload
 ��������  : ����Ƿ���Ҫͬ���߼���λ���ñ�
 �������  : DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��6��23��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void check_GBLogicDeviceConfig_need_to_reload(DBOper* pDboper)
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_GBLogicDeviceInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ���߼���λ���ݿ���Ϣ: ��ʼ---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization logic point database information: begain---");
    printf("\r\n\r\ncheck_GBLogicDeviceConfig_need_to_reload() :begain---, time=%d \r\n", time(NULL));

    /* �����߼��豸���е�ɾ����ʶ */
    set_GBLogicDevice_info_list_del_mark(3);

    /* ���߼��豸���ݿ��еı仯����ͬ�����ڴ� */
    check_GBLogicDevice_info_from_db_to_list(pDboper);

    /* ɾ��������߼��豸��Ϣ */
    delete_GBLogicDevice_info_from_list_by_mark();
    printf("check_GBLogicDeviceConfig_need_to_reload() :end---, time=%d \r\n", time(NULL));

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ���߼���λ���ݿ���Ϣ: ����---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization logic point database information: end---");
    db_GBLogicDeviceInfo_reload_mark = 0;

    return;
}

/*****************************************************************************
 �� �� ��  : GBDeviceConfig_db_refresh_proc
 ��������  : ���������豸���ݿ���²�����ʶ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��6��23��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GBDeviceConfig_db_refresh_proc()
{
    if (1 == db_GBDeviceInfo_reload_mark) /* ����ִ�� */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "��׼�����豸���ݿ���Ϣ����ͬ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Standard physical device database information are synchronized");
        return 0;
    }

    db_GBDeviceInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 �� �� ��  : check_GBLogicDeviceConfig_need_to_reload
 ��������  : ����Ƿ���Ҫͬ�������豸���ñ�
 �������  : DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��6��23��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void check_GBDeviceConfig_need_to_reload(DBOper* pDboper)
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_GBDeviceInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ����׼�����豸���ݿ���Ϣ: ��ʼ---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronous standard physical device database information: begian---");
    printf("\r\n\r\ncheck_GBDeviceConfig_need_to_reload() :begain---, time=%d \r\n", time(NULL));

    /* ���������豸���е�ɾ����ʶ */
    set_GBDevice_info_list_del_mark(1);

    /* �������豸���ݿ��еı仯����ͬ�����ڴ� */
    check_GBDevice_info_from_db_to_list(pDboper);

    /* ɾ������������豸��Ϣ */
    delete_GBDevice_info_from_list_by_mark();
    printf("check_GBDeviceConfig_need_to_reload() :end---, time=%d \r\n", time(NULL));

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ����׼�����豸���ݿ���Ϣ: ����---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronous standard physical device database information: end---");
    db_GBDeviceInfo_reload_mark = 0;

    return;
}

#if DECS("ZRV�豸��Ϣ����")
/*****************************************************************************
 �� �� ��  : ZRVDevice_info_init
 ��������  : ZRV�豸�ṹ��ʼ��
 �������  : ZRVDevice_info_t ** ZRVDevice_info
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : ZRVDevice_info_free
 ��������  : ZRV�豸�ṹ�ͷ�
 �������  : ZRVDevice_info_t * ZRVDevice_info
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : ZRVDevice_info_list_init
 ��������  : ZRV�豸��Ϣ���г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : ZRVDevice_info_list_free
 ��������  : ZRV�豸�����ͷ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : ZRVDevice_info_list_lock
 ��������  : ZRV�豸��������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : ZRVDevice_info_list_unlock
 ��������  : ZRV�豸���н���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : debug_ZRVDevice_info_list_lock
 ��������  : ZRV�豸��������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : debug_ZRVDevice_info_list_unlock
 ��������  : ZRV�豸���н���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : ZRVDevice_info_add
 ��������  : ���ZRV�豸��Ϣ��������
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : ZRVDevice_info_remove
 ��������  : �Ӷ������Ƴ�ZRV�豸��Ϣ
 �������  : char* device_ip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : ZRVDevice_info_find
 ��������  : �Ӷ����в���ZRV�豸
 �������  : char* device_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : ZRVDevice_info_find_by_device_index
 ��������  : ͨ�������豸TCP Sock��ȡ�����豸��Ϣ
 �������  : int tcp_sock
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : �û�·����Ϣ����
    �޸�����   : �����ɺ���

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
 �� �� ��  : scan_ZRVDevice_info_list_for_expires
 ��������  : ɨ�������豸����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ������Ҫ����ע����Ϣ�� */
    while (!needProc.empty())
    {
        pProcZRVDeviceInfo = (ZRVDevice_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pProcZRVDeviceInfo)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ZRV�豸������Ϣ��ʱ, ����ע����¼:ZRV�豸IP��ַ=%s, TCP Socket=%d", pProcZRVDeviceInfo->device_ip, pProcZRVDeviceInfo->tcp_sock);
            pProcZRVDeviceInfo->reg_status = 0;
            pProcZRVDeviceInfo->tcp_sock = -1;
        }
    }

    needProc.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : free_zrv_device_info_by_tcp_socket
 ��������  : �ͷ�ZRV�豸�����TCP Socket����
 �������  : int tcp_socket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ZRV�豸TCP���ӶϿ�, ����ע����¼:ZRV�豸IP��ַ=%s, TCP Socket=%d", pZRVDeviceInfo->device_ip, tcp_socket);
        SetZRVDeviceToConfigTable(pZRVDeviceInfo, &g_DBOper);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : is_zrv_device_tcp_socket_in_use
 ��������  : ZRV�豸��TCP Soceket �Ƿ�ʹ����
 �������  : int tcp_socket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
    //SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ѹ��������Ϣ��ZRV�豸, ��������=%d, ÿ̨ZRV�豸�����������=%d, ZRV�豸����device_index=%d", task_count, per_task_count_zrvdevice, device_index);
    PerCompressTaskID.clear();

    for (task_index = task_begin_index; task_index < task_end_index; task_index++)
    {
        if (task_index < task_count)
        {
            //SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ѹ��������Ϣ��ZRV�豸, ZRV�豸����device_index=%d, ��������task_index=%d, CompressTaskID=%s", device_index, task_index, (char*)CompressTaskID[task_index].c_str());
            PerCompressTaskID.push_back(CompressTaskID[task_index]);
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : ShowZRVDeviceInfo
 ��������  : �鿴ZRV�豸��Ϣ
 �������  : int sock
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �������ݿ������ */
    strSQL.clear();

    strSQL = "select * from ZRVDeviceInfo";
    strSQL += " WHERE DeviceIP like '";
    strSQL += pZRVDeviceInfo->device_ip;
    strSQL += "'";

    record_count = ptDBoper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "SetZRVDeviceToConfigTable() record_count=%d, strSQL=%s \r\n", record_count, strSQL.c_str());

    if (record_count < 0) /* ���ݿ���� */
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetZRVDeviceToConfigTable() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetZRVDeviceToConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        return 0;
    }
    else if (record_count == 0) /* û�м�¼ */
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

        if (record_count < 0) /* ���ݿ���� */
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetZRVDeviceToConfigTable() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "SetZRVDeviceToConfigTable() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
            return 0;
        }
        else if (record_count == 0) /* û�в鵽��¼ */
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "SetZRVDeviceToConfigTable() exit---: No Record Count \r\n");
            return 0;
        }

        /* ��ȡ���ݿ��¼��Ϣ */
        ptDBoper->GetFieldValue("ID", iDeviceIndex);
    }
    else
    {
        /* ��ȡ���ݿ��¼��Ϣ */
        ptDBoper->GetFieldValue("ID", iDeviceIndex);
        ptDBoper->GetFieldValue("Status", iTmpStatus);
        ptDBoper->GetFieldValue("Expires", iTmpExpires);

        if (iTmpStatus != pZRVDeviceInfo->reg_status
            || iTmpExpires != pZRVDeviceInfo->register_expires) /* �б仯�����µ����ݿ� */
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
 �� �� ��  : get_min_task_zrv_device_count
 ��������  : ��ȡ��������С��ZRV�豸
 �������  : char* platform_ip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��9��11��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
