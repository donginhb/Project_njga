
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
#include "common/gblconfig_proc.inc"
#include "common/gblfunc_proc.inc"
#include "common/log_proc.inc"
#include "common/db_proc.h"

#include "platformms/BoardInit.h"

#include "device/device_info_mgn.inc"
#include "route/platform_info_mgn.inc"
#include "route/platform_thread_proc.inc"

/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �ⲿ����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ȫ�ֱ���                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/
platform_info_list_t* g_PlatformInfoList = NULL;    /* ·����Ϣ���� */

int db_PlatformInfo_reload_mark = 0; /* ·����Ϣ���ݿ���±�ʶ:0:����Ҫ���£�1:��Ҫ�������ݿ� */
int db_MMSPlatformInfo_reload_mark = 0; /* �ֻ�MMS·����Ϣ���±�ʶ:0:����Ҫ���£�1:��Ҫ���� */

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

#if DECS("·����Ϣ����")
/*****************************************************************************
 �� �� ��  : platform_info_init
 ��������  : ·����Ϣ�ṹ��ʼ��
 �������  : platform_info_t ** platform_info
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int platform_info_init(platform_info_t** platform_info)
{
    *platform_info = (platform_info_t*)osip_malloc(sizeof(platform_info_t));

    if (*platform_info == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_info_init() exit---: *platform_info Smalloc Error \r\n");
        return -1;
    }

    (*platform_info)->id = 0;
    (*platform_info)->platform_ip[0] = '\0';

    return 0;
}

/*****************************************************************************
 �� �� ��  : platform_info_free
 ��������  : ·����Ϣ�ṹ�ͷ�
 �������  : platform_info_t * platform_info
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void platform_info_free(platform_info_t* platform_info)
{
    if (platform_info == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_info_free() exit---: Param Error \r\n");
        return;
    }

    platform_info->id = 0;
    memset(platform_info->platform_ip, 0, MAX_IP_LEN);

    osip_free(platform_info);
    platform_info = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : platform_info_list_init
 ��������  : ·����Ϣ���г�ʼ��
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
int platform_info_list_init()
{
    g_PlatformInfoList = (platform_info_list_t*)osip_malloc(sizeof(platform_info_list_t));

    if (g_PlatformInfoList == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_info_list_init() exit---: g_PlatformInfoList Smalloc Error \r\n");
        return -1;
    }

    g_PlatformInfoList->pPlatformInfoList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_PlatformInfoList->pPlatformInfoList)
    {
        osip_free(g_PlatformInfoList);
        g_PlatformInfoList = NULL;
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_info_list_init() exit---: Platform Info List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_PlatformInfoList->pPlatformInfoList);

#ifdef MULTI_THR
    /* init smutex */
    g_PlatformInfoList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_PlatformInfoList->lock)
    {
        osip_free(g_PlatformInfoList->pPlatformInfoList);
        g_PlatformInfoList->pPlatformInfoList = NULL;
        osip_free(g_PlatformInfoList);
        g_PlatformInfoList = NULL;
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_info_list_init() exit---: Platform Info List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : platform_info_list_free
 ��������  : ·����Ϣ�����ͷ�
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
void platform_info_list_free()
{
    if (NULL == g_PlatformInfoList)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_info_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_PlatformInfoList->pPlatformInfoList)
    {
        osip_list_special_free(g_PlatformInfoList->pPlatformInfoList, (void (*)(void*))&platform_info_free);
        osip_free(g_PlatformInfoList->pPlatformInfoList);
        g_PlatformInfoList->pPlatformInfoList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_PlatformInfoList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_PlatformInfoList->lock);
        g_PlatformInfoList->lock = NULL;
    }

#endif
    osip_free(g_PlatformInfoList);
    g_PlatformInfoList = NULL;
    return;
}

/*****************************************************************************
 �� �� ��  : platform_info_list_lock
 ��������  : ·����Ϣ��������
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
int platform_info_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PlatformInfoList == NULL || g_PlatformInfoList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_PlatformInfoList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : platform_info_list_unlock
 ��������  : ·����Ϣ���н���
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
int platform_info_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PlatformInfoList == NULL || g_PlatformInfoList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_PlatformInfoList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_platform_info_list_lock
 ��������  : ·����Ϣ��������
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
int debug_platform_info_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PlatformInfoList == NULL || g_PlatformInfoList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "debug_platform_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_PlatformInfoList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_platform_info_list_unlock
 ��������  : ·����Ϣ���н���
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
int debug_platform_info_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PlatformInfoList == NULL || g_PlatformInfoList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "debug_platform_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_PlatformInfoList->lock, file, line, func);

#endif
    return iRet;
}

int platform_info_add(platform_info_t* pPlatformInfo)
{
    int i = 0;
    int iRet = 0;
    int route_tl_pos = 0;

    if (pPlatformInfo == NULL)
    {
        return -1;
    }

    PLATFORM_INFO_SMUTEX_LOCK();

    i = osip_list_add(g_PlatformInfoList->pPlatformInfoList, pPlatformInfo, -1); /* add to list tail */

    if (i < 0)
    {
        PLATFORM_INFO_SMUTEX_UNLOCK();
        return -1;
    }

    PLATFORM_INFO_SMUTEX_UNLOCK();

    /* �����ϼ�ƽ̨ҵ�����߳� */
    route_tl_pos = platform_srv_proc_thread_find(pPlatformInfo->id);
    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "platform_info_add() platform_srv_proc_thread_find:id=%u, route_tl_pos=%d \r\n", pPlatformInfo->id, route_tl_pos);

    if (route_tl_pos < 0)
    {
        //���䴦���߳�
        iRet = platform_srv_proc_thread_assign(pPlatformInfo);
        DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "platform_info_add() platform_srv_proc_thread_assign:iRet=%d \r\n", iRet);

        if (iRet != 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�����ϼ�ƽ̨ҵ�����߳�ʧ��:�ϼ�ƽ̨ID=%u, IP��ַ=%s", pPlatformInfo->id, pPlatformInfo->platform_ip);
            platform_info_remove(i - 1);
            return -1;
        }
    }
    else
    {
        /* �ͷ�һ��֮ǰ��ҵ�����߳� */
        iRet = platform_srv_proc_thread_recycle(pPlatformInfo->id);

        //���䴦���߳�
        iRet = platform_srv_proc_thread_assign(pPlatformInfo);
        DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "platform_info_add() platform_srv_proc_thread_assign:iRet=%d \r\n", iRet);

        if (iRet != 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�����ϼ�ƽ̨ҵ�����߳�ʧ��:�ϼ�ƽ̨ID=%u, IP��ַ=%s", pPlatformInfo->id, pPlatformInfo->platform_ip);
            platform_info_remove(i - 1);
            return -1;
        }
    }

    return i - 1;
}

/*****************************************************************************
 �� �� ��  : platform_info_remove
 ��������  : �Ӷ������Ƴ�·����Ϣ
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
int platform_info_remove(int pos)
{
    int iRet = 0;
    unsigned int platform_index = 0;
    platform_info_t* pPlatformInfo = NULL;

    PLATFORM_INFO_SMUTEX_LOCK();

    if (g_PlatformInfoList == NULL || pos < 0 || (pos >= osip_list_size(g_PlatformInfoList->pPlatformInfoList)))
    {
        PLATFORM_INFO_SMUTEX_UNLOCK();
        return -1;
    }

    pPlatformInfo = (platform_info_t*)osip_list_get(g_PlatformInfoList->pPlatformInfoList, pos);

    if (NULL == pPlatformInfo)
    {
        PLATFORM_INFO_SMUTEX_UNLOCK();
        return -1;
    }

    platform_index = pPlatformInfo->id;

    osip_list_remove(g_PlatformInfoList->pPlatformInfoList, pos);
    platform_info_free(pPlatformInfo);
    pPlatformInfo = NULL;
    PLATFORM_INFO_SMUTEX_UNLOCK();

    /* ����ҵ�����߳� */
    iRet = platform_srv_proc_thread_recycle(platform_index);

    if (0 != iRet)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "platform_srv_proc_thread_recycle() platform_srv_proc_thread_recycle Error:platform_index=%u, iRet=%d \r\n", platform_index, iRet);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "platform_srv_proc_thread_recycle() platform_srv_proc_thread_recycle OK:platform_index=%u, iRet=%d \r\n", platform_index, iRet);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : platform_info_find
 ��������  : ����·����Ϣ
 �������  : char* server_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��16��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int platform_info_find(char* platform_ip)
{
    int pos = -1;
    platform_info_t* pPlatformInfo = NULL;

    if (NULL == g_PlatformInfoList || NULL == platform_ip)
    {
        return -1;
    }

    PLATFORM_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_PlatformInfoList->pPlatformInfoList) <= 0)
    {
        PLATFORM_INFO_SMUTEX_UNLOCK();
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_PlatformInfoList->pPlatformInfoList); pos++)
    {
        pPlatformInfo = (platform_info_t*)osip_list_get(g_PlatformInfoList->pPlatformInfoList, pos);

        if (NULL == pPlatformInfo)
        {
            continue;
        }

        if (sstrcmp(pPlatformInfo->platform_ip, platform_ip) == 0)
        {
            PLATFORM_INFO_SMUTEX_UNLOCK();
            return pos;
        }
    }

    PLATFORM_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : platform_info_find_by_platform_index
 ��������  : ����·����������·����Ϣ
 �������  : unsigned int platform_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��7��23�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int platform_info_find_by_platform_index(unsigned int platform_index)
{
    int pos = -1;
    platform_info_t* pPlatformInfo = NULL;

    if (NULL == g_PlatformInfoList || platform_index <= 0)
    {
        return -1;
    }

    PLATFORM_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_PlatformInfoList->pPlatformInfoList) <= 0)
    {
        PLATFORM_INFO_SMUTEX_UNLOCK();
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_PlatformInfoList->pPlatformInfoList); pos++)
    {
        pPlatformInfo = (platform_info_t*)osip_list_get(g_PlatformInfoList->pPlatformInfoList, pos);

        if (NULL == pPlatformInfo)
        {
            continue;
        }

        if (pPlatformInfo->id == platform_index)
        {
            PLATFORM_INFO_SMUTEX_UNLOCK();
            return pos;
        }
    }

    PLATFORM_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : platform_info_get
 ��������  : ��ȡ·����Ϣ
 �������  : int pos
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��16��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
platform_info_t* platform_info_get(int pos)
{
    platform_info_t* pPlatformInfo = NULL;

    if (g_PlatformInfoList == NULL || pos < 0 || pos >= osip_list_size(g_PlatformInfoList->pPlatformInfoList))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_info_get() exit---: Param Error \r\n");
        return NULL;
    }

    pPlatformInfo = (platform_info_t*)osip_list_get(g_PlatformInfoList->pPlatformInfoList, pos);

    return pPlatformInfo;
}
#endif

/*****************************************************************************
 �� �� ��  : set_platform_info_list_del_mark
 ��������  : ����·����Ϣɾ����ʶ
 �������  : int del_mark
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��12�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int set_platform_info_list_del_mark(int del_mark)
{
    int pos = -1;
    platform_info_t* pPlatformInfo = NULL;

    if (NULL == g_PlatformInfoList)
    {
        return -1;
    }

    PLATFORM_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_PlatformInfoList->pPlatformInfoList) <= 0)
    {
        PLATFORM_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "set_platform_info_list_del_mark() exit---: Record Info List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_PlatformInfoList->pPlatformInfoList); pos++)
    {
        pPlatformInfo = (platform_info_t*)osip_list_get(g_PlatformInfoList->pPlatformInfoList, pos);

        if (NULL == pPlatformInfo)
        {
            continue;
        }

        if (pPlatformInfo->id > 0)
        {
            if (0 == pPlatformInfo->del_mark)
            {
                pPlatformInfo->del_mark = del_mark;
            }
        }
    }

    PLATFORM_INFO_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : delete_record_info_from_list_by_mark
 ��������  : ����ɾ����ʶ��ɾ��·����Ϣ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��12�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int delete_platform_info_from_list_by_mark()
{
    int pos = -1;
    platform_info_t* pPlatformInfo = NULL;

    if ((NULL == g_PlatformInfoList) || (NULL == g_PlatformInfoList->pPlatformInfoList))
    {
        return -1;
    }

    PLATFORM_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_PlatformInfoList->pPlatformInfoList) <= 0)
    {
        PLATFORM_INFO_SMUTEX_UNLOCK();
        return 0;
    }

    pos = 0;

    while (!osip_list_eol(g_PlatformInfoList->pPlatformInfoList, pos))
    {
        pPlatformInfo = (platform_info_t*)osip_list_get(g_PlatformInfoList->pPlatformInfoList, pos);

        if (NULL == pPlatformInfo)
        {
            osip_list_remove(g_PlatformInfoList->pPlatformInfoList, pos);
            continue;
        }

        if (0 != pPlatformInfo->id && pPlatformInfo->del_mark == 2)
        {
            osip_list_remove(g_PlatformInfoList->pPlatformInfoList, pos);
            platform_info_free(pPlatformInfo);
            pPlatformInfo = NULL;
        }
        else
        {
            pos++;
        }
    }

    PLATFORM_INFO_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : check_platform_info_from_db_to_list
 ��������  : ���·�ɱ������Ƿ��б仯����ͬ�����ڴ�
 �������  : DBOper* pRecord_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��12�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_platform_info_from_db_to_list(DBOper* pPlatform_Srv_dboper)
{
    int ret = 0;
    string strSQL = "";
    int platform_pos = -1;
    int record_count = 0;
    platform_info_t* pOldPlatformInfo = NULL;
    int while_count = 0;

    if (NULL == pPlatform_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "check_platform_info_from_db_to_list() exit---: Platform Srv db Oper Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from VideoManagePlatformInfo";

    record_count = pPlatform_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "check_platform_info_from_db_to_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "check_platform_info_from_db_to_list() ErrorMsg=%s\r\n", pPlatform_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        return 0;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_platform_info_from_db_to_list():record_count=%d  \r\n", record_count);

    /* ѭ���������ݿ�*/
    do
    {
        platform_info_t* pPlatformInfo = NULL;
        unsigned int tmp_ivalue = 0;
        string tmp_svalue = "";
        int iRet = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "check_platform_info_from_db_to_list() While Count=%d \r\n", while_count);
        }

        int i_ret = platform_info_init(&pPlatformInfo);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "check_platform_info_from_db_to_list() platform_info_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        /* ���� */
        tmp_ivalue = 0;
        pPlatform_Srv_dboper->GetFieldValue("ID", tmp_ivalue);

        pPlatformInfo->id = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_platform_info_list() pPlatformInfo->id: %d", pPlatformInfo->id);

        /* �ϼ�������CMS ip��ַ*/
        tmp_svalue.clear();
        pPlatform_Srv_dboper->GetFieldValue("PlatformIP", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pPlatformInfo->platform_ip, (char*)tmp_svalue.c_str(), MAX_IP_LEN);
            //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_platform_info_from_db_to_list() pPlatformInfo->server_ip: %s", pPlatformInfo->server_ip);
        }
        else
        {
            platform_info_free(pPlatformInfo);
            pPlatformInfo = NULL;
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "check_platform_info_from_db_to_list() server_ip NULL \r\n");
            continue;
        }

        platform_pos = platform_info_find_by_platform_index(pPlatformInfo->id);

        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_platform_info_from_db_to_list() platform_info_find_by_platform_index:platform_pos=%d \r\n", platform_pos);

        if (platform_pos >= 0)
        {
            pOldPlatformInfo = platform_info_get(platform_pos);

            if (NULL != pOldPlatformInfo)
            {
                if (1 == pOldPlatformInfo->del_mark) /* IP��ַ�ȴ��ı� */
                {

                }
                else
                {
                    pOldPlatformInfo->del_mark = 0;

                    /* ���IP ��ַ�Ƿ��б仯 */
                    if (0 != sstrcmp(pPlatformInfo->platform_ip, pOldPlatformInfo->platform_ip)) /* IP��ַ���޸� */
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_platform_info_from_db_to_list() platform_ip changed:old=%s, new=%s \r\n", pOldPlatformInfo->platform_ip, pPlatformInfo->platform_ip);

                        memset(pOldPlatformInfo->platform_ip, 0, MAX_IP_LEN);
                        osip_strncpy(pOldPlatformInfo->platform_ip, pPlatformInfo->platform_ip, MAX_IP_LEN);

                        pOldPlatformInfo->del_mark = 1;
                    }
                }

                platform_info_free(pPlatformInfo);
                pPlatformInfo = NULL;
                continue;
            }
        }
        else
        {
            /* ��ӵ����� */
            if (platform_info_add(pPlatformInfo) < 0)
            {
                platform_info_free(pPlatformInfo);
                pPlatformInfo = NULL;
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "check_platform_info_from_db_to_list() platform_info_add Error \r\n");
                continue;
            }

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ƽ̨������Ϣ: Platform ID=%u, platform_ip=%s", pPlatformInfo->id, pPlatformInfo->platform_ip);
        }
    }
    while (pPlatform_Srv_dboper->MoveNext() >= 0);

    return ret;
}

/*****************************************************************************
 �� �� ��  : scan_platform_info_list
 ��������  : ɨ��·����Ϣ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��30��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_platform_info_list()
{
    int i = 0;
    int iRet = -1;
    platform_info_t* pPlatformInfo = NULL;
    needtoproc_platforminfo_queue needToChangeIP;
    needtoproc_platforminfo_queue needToDelete;
    platform_srv_proc_tl_t* runthread = NULL;

    if ((NULL == g_PlatformInfoList) || (NULL == g_PlatformInfoList->pPlatformInfoList))
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "scan_platform_info_list() exit---: Param Error \r\n");
        return;
    }

    needToChangeIP.clear();

    PLATFORM_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_PlatformInfoList->pPlatformInfoList) <= 0)
    {
        PLATFORM_INFO_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_PlatformInfoList->pPlatformInfoList); i++)
    {
        pPlatformInfo = (platform_info_t*)osip_list_get(g_PlatformInfoList->pPlatformInfoList, i);

        if (NULL == pPlatformInfo)
        {
            continue;
        }

        if (pPlatformInfo->del_mark == 1)  /* ��Ҫ�ı��߳��е�IP��ַ */
        {
            needToChangeIP.push_back(pPlatformInfo);
            pPlatformInfo->del_mark = 0;
            continue;
        }
        else if (pPlatformInfo->del_mark == 2 || pPlatformInfo->del_mark == 3) /* ��Ҫɾ���� */
        {
            needToDelete.push_back(pPlatformInfo);
            continue;
        }
    }

    PLATFORM_INFO_SMUTEX_UNLOCK();

    /* ������Ҫ���ͻ�ȡ������ID��Ϣ�� */
    while (!needToChangeIP.empty())
    {
        pPlatformInfo = (platform_info_t*) needToChangeIP.front();
        needToChangeIP.pop_front();

        if (NULL != pPlatformInfo)
        {
            /* ���Ҷ�Ӧ���̣߳��޸�IP��ַ */
            runthread = get_platform_srv_proc_thread(pPlatformInfo->id);

            if (NULL != runthread)
            {
                if (1 == runthread->iCompressTaskStatus || 2 == runthread->iCompressTaskStatus)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "scan_platform_info_list() Now Get CompressTask \r\n");
                    continue;
                }
                else
                {
                    memset(runthread->platform_ip, 0, MAX_IP_LEN);
                    osip_strncpy(runthread->platform_ip, pPlatformInfo->platform_ip, MAX_IP_LEN);
                    pPlatformInfo->del_mark = 0;
                }
            }
        }
    }

    needToChangeIP.clear();

    /* ������Ҫɾ���� */
    while (!needToDelete.empty())
    {
        pPlatformInfo = (platform_info_t*) needToDelete.front();
        needToDelete.pop_front();

        if (NULL != pPlatformInfo)
        {
            /* ���Ҷ�Ӧ���̣߳��޸�IP��ַ */
            runthread = get_platform_srv_proc_thread(pPlatformInfo->id);

            if (NULL != runthread)
            {
                /* �鿴�߳������Ƿ��л�ȡ��������ִ�У����Ƿ����ɾ�� */
                if (1 == runthread->iCompressTaskStatus || 2 == runthread->iCompressTaskStatus)
                {
                    pPlatformInfo->del_mark = 3;
                    DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "scan_platform_info_list() Now Get CompressTask \r\n");
                    continue;
                }
                else
                {
                    pPlatformInfo->del_mark = 2;
                }
            }
        }
    }

    needToDelete.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : PlatformInfoConfig_db_refresh_proc
 ��������  : �����ϼ�·��������Ϣ���ݿ���²�����ʶ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��10��18��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int PlatformInfoConfig_db_refresh_proc()
{
    if (1 == db_PlatformInfo_reload_mark) /* ����ִ�� */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�·���������ݿ���Ϣ����ͬ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Platform Info database information are synchronized");
        return 0;
    }

    db_PlatformInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 �� �� ��  : check_PlatformInfoConfig_need_to_reload_begin
 ��������  : ����Ƿ���Ҫͬ���ϼ�·�����ÿ�ʼ
 �������  : DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��10��18��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void check_PlatformInfoConfig_need_to_reload_begin(DBOper* pDboper)
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_PlatformInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ���ϼ�·���������ݿ���Ϣ: ��ʼ---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization platform info database information: begain---");

    /* ����·����Ϣ���е�ɾ����ʶ */
    set_platform_info_list_del_mark(2);

    /* ��·���������ݿ��еı仯����ͬ�����ڴ� */
    check_platform_info_from_db_to_list(pDboper);

    return;
}

/*****************************************************************************
 �� �� ��  : check_PlatformInfoConfig_need_to_reload_end
 ��������  : ����Ƿ���Ҫͬ���ϼ�·�����ñ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��10��18��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void check_PlatformInfoConfig_need_to_reload_end()
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_PlatformInfo_reload_mark)
    {
        return;
    }

    /* ɾ��������ϼ�·��������Ϣ */
    delete_platform_info_from_list_by_mark();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ���ϼ�·���������ݿ���Ϣ: ����---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization platform info database information: end---");
    db_PlatformInfo_reload_mark = 0;

    return;
}

/*****************************************************************************
 �� �� ��  : ShowPlatformInfo
 ��������  : ��ʾ�ϼ�����CMS��·����Ϣ
 �������  : int sock
                           int type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��3��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void ShowPlatformInfo(int sock, int type)
{
    int i = 0;
    char strLine[] = "\r-------------------------------\r\n";
    char strHead[] = "\rPlatform Index  Platform IP    \r\n";
    platform_info_t* pPlatformInfo = NULL;
    char rbuf[256] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if ((NULL == g_PlatformInfoList) || (NULL == g_PlatformInfoList->pPlatformInfoList))
    {
        return;
    }

    PLATFORM_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_PlatformInfoList->pPlatformInfoList) <= 0)
    {
        PLATFORM_INFO_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_PlatformInfoList->pPlatformInfoList); i++)
    {
        pPlatformInfo = (platform_info_t*)osip_list_get(g_PlatformInfoList->pPlatformInfoList, i);

        if (NULL == pPlatformInfo)
        {
            continue;
        }

        snprintf(rbuf, 256, "\r%-14u  %-15s\r\n", pPlatformInfo->id, pPlatformInfo->platform_ip);

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    PLATFORM_INFO_SMUTEX_UNLOCK();

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}
