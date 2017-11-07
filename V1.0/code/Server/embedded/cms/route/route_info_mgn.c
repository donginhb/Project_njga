
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

#include "route/route_thread_proc.inc"

/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern DBOper g_DBOper;
extern GBLogicDevice_Info_MAP g_GBLogicDeviceInfoMap;              /* ��׼�߼��豸��Ϣ���� */

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
route_info_list_t* g_RouteInfoList = NULL;    /* ·����Ϣ���� */

int db_RouteInfo_reload_mark = 0; /* ·����Ϣ���ݿ���±�ʶ:0:����Ҫ���£�1:��Ҫ�������ݿ� */
int db_MMSRouteInfo_reload_mark = 0; /* �ֻ�MMS·����Ϣ���±�ʶ:0:����Ҫ���£�1:��Ҫ���� */

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

#if DECS("·����Ϣ����")
/*****************************************************************************
 �� �� ��  : route_info_init
 ��������  : ·����Ϣ�ṹ��ʼ��
 �������  : route_info_t ** route_info
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_info_init(route_info_t** route_info)
{
    *route_info = (route_info_t*)osip_malloc(sizeof(route_info_t));

    if (*route_info == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_init() exit---: *route_info Smalloc Error \r\n");
        return -1;
    }

    (*route_info)->id = 0;
    (*route_info)->server_id[0] = '\0';
    (*route_info)->server_ip[0] = '\0';
    (*route_info)->server_host[0] = '\0';
    (*route_info)->server_port = 0;
    (*route_info)->register_account[0] = '\0';
    (*route_info)->register_password[0] = '\0';
    (*route_info)->link_type = 0;
    (*route_info)->three_party_flag = 0;
    (*route_info)->trans_protocol = 0;
    (*route_info)->access_method = 0;
    (*route_info)->tcp_sock = -1;
    (*route_info)->ip_is_in_sub = 0;
    (*route_info)->strRegLocalEthName[0] = '\0';
    (*route_info)->strRegLocalIP[0] = '\0';
    (*route_info)->iRegLocalPort = 5060;
    (*route_info)->del_mark = 0;
    (*route_info)->keep_alive_count = MIN_KEEP_ALIVE_INTERVAL;
    (*route_info)->failed_keep_alive_count = 0;
    (*route_info)->catalog_subscribe_flag = 0;
    (*route_info)->catalog_subscribe_expires = 0;
    (*route_info)->catalog_subscribe_expires_count = 0;
    (*route_info)->catalog_subscribe_event_id = 0;
    (*route_info)->catalog_subscribe_dialog_index = -1;

    (*route_info)->reg_status = 0;
    (*route_info)->reg_interval = 0;
    (*route_info)->expires = 0;
    (*route_info)->min_expires = 0;
    (*route_info)->reg_info_index = -1;
    (*route_info)->keep_alive_sn = 0;

    (*route_info)->catlog_get_status = 0;

    (*route_info)->CataLogNumCount = 0;
    (*route_info)->CataLogSN = 0;
    return 0;
}

/*****************************************************************************
 �� �� ��  : route_info_free
 ��������  : ·����Ϣ�ṹ�ͷ�
 �������  : route_info_t * route_info
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void route_info_free(route_info_t* route_info)
{
    if (route_info == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_free() exit---: Param Error \r\n");
        return;
    }

    route_info->id = 0;

    memset(route_info->server_id, 0, MAX_ID_LEN + 4);
    memset(route_info->server_ip, 0, MAX_IP_LEN);
    memset(route_info->server_host, 0, MAX_128CHAR_STRING_LEN + 4);

    route_info->server_port = 0;

    memset(route_info->register_account, 0, MAX_128CHAR_STRING_LEN + 4);
    memset(route_info->register_password, 0, MAX_128CHAR_STRING_LEN + 4);

    route_info->link_type = 0;
    route_info->three_party_flag = 0;
    route_info->trans_protocol = 0;
    route_info->access_method = 0;
    route_info->tcp_sock = -1;
    route_info->ip_is_in_sub = 0;
    memset(route_info->strRegLocalEthName, 0, MAX_IP_LEN);
    memset(route_info->strRegLocalIP, 0, MAX_IP_LEN);
    route_info->iRegLocalPort = 5060;
    route_info->del_mark = 0;
    route_info->keep_alive_count = MIN_KEEP_ALIVE_INTERVAL;
    route_info->failed_keep_alive_count = 0;
    route_info->catalog_subscribe_flag = 0;
    route_info->catalog_subscribe_expires = 0;
    route_info->catalog_subscribe_expires_count = 0;
    route_info->catalog_subscribe_event_id = 0;
    route_info->catalog_subscribe_dialog_index = -1;

    route_info->reg_status = 0;
    route_info->reg_interval = 0;
    route_info->expires = 0;
    route_info->min_expires = 0;
    route_info->reg_info_index = -1;
    route_info->keep_alive_sn = 0;

    route_info->catlog_get_status = 0;

    route_info->CataLogNumCount = 0;
    route_info->CataLogSN = 0;

    osip_free(route_info);
    route_info = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : route_info_list_init
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
int route_info_list_init()
{
    g_RouteInfoList = (route_info_list_t*)osip_malloc(sizeof(route_info_list_t));

    if (g_RouteInfoList == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_list_init() exit---: g_RouteInfoList Smalloc Error \r\n");
        return -1;
    }

    g_RouteInfoList->pRouteInfoList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_RouteInfoList->pRouteInfoList)
    {
        osip_free(g_RouteInfoList);
        g_RouteInfoList = NULL;
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_list_init() exit---: Route Info List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_RouteInfoList->pRouteInfoList);

#ifdef MULTI_THR
    /* init smutex */
    g_RouteInfoList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_RouteInfoList->lock)
    {
        osip_free(g_RouteInfoList->pRouteInfoList);
        g_RouteInfoList->pRouteInfoList = NULL;
        osip_free(g_RouteInfoList);
        g_RouteInfoList = NULL;
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_list_init() exit---: Route Info List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : route_info_list_free
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
void route_info_list_free()
{
    if (NULL == g_RouteInfoList)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_RouteInfoList->pRouteInfoList)
    {
        osip_list_special_free(g_RouteInfoList->pRouteInfoList, (void (*)(void*))&route_info_free);
        osip_free(g_RouteInfoList->pRouteInfoList);
        g_RouteInfoList->pRouteInfoList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_RouteInfoList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_RouteInfoList->lock);
        g_RouteInfoList->lock = NULL;
    }

#endif
    osip_free(g_RouteInfoList);
    g_RouteInfoList = NULL;
    return;
}

/*****************************************************************************
 �� �� ��  : route_info_list_lock
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
int route_info_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RouteInfoList == NULL || g_RouteInfoList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_RouteInfoList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : route_info_list_unlock
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
int route_info_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RouteInfoList == NULL || g_RouteInfoList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_RouteInfoList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_route_info_list_lock
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
int debug_route_info_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RouteInfoList == NULL || g_RouteInfoList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "debug_route_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_RouteInfoList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_route_info_list_unlock
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
int debug_route_info_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RouteInfoList == NULL || g_RouteInfoList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "debug_route_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_RouteInfoList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : route_info_add
 ��������  : ���·����Ϣ��������
 �������  : char* server_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
//int route_info_add(char* server_id)
//{
//    route_info_t* pRouteInfo = NULL;
//    int i = 0;

//    if (g_RouteInfoList == NULL || server_id == NULL)
//    {
//        return -1;
//    }

//    i = route_info_init(&pRouteInfo);

//    if (i != 0)
//    {
//        return -1;
//    }

//    pRouteInfo->server_id = sgetcopy(server_id);

//    route_info_list_lock();
//    i = list_add(g_RouteInfoList->pRouteInfoList, pRouteInfo, -1); /* add to list tail */

//    if (i == -1)
//    {
//        route_info_list_unlock();
//        route_info_free(pRouteInfo);
//        sfree(pRouteInfo);
//        return -1;
//    }

//    route_info_list_unlock();
//    return i;
//}

int route_info_add(route_info_t* pRouteInfo)
{
    int i = 0;

    if (pRouteInfo == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_add() exit---: Param Error \r\n");
        return -1;
    }

    ROUTE_INFO_SMUTEX_LOCK();

    i = osip_list_add(g_RouteInfoList->pRouteInfoList, pRouteInfo, -1); /* add to list tail */

    if (i < 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_add() exit---: List Add Error \r\n");
        return -1;
    }

    ROUTE_INFO_SMUTEX_UNLOCK();
    return i - 1;
}


/*****************************************************************************
 �� �� ��  : route_info_remove
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
int route_info_remove(int pos)
{
    route_info_t* pRouteInfo = NULL;

    ROUTE_INFO_SMUTEX_LOCK();

    if (g_RouteInfoList == NULL || pos < 0 || (pos >= osip_list_size(g_RouteInfoList->pRouteInfoList)))
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_remove() exit---: Param Error \r\n");
        return -1;
    }

    pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

    if (NULL == pRouteInfo)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_remove() exit---: List Get Error \r\n");
        return -1;
    }

    osip_list_remove(g_RouteInfoList->pRouteInfoList, pos);
    route_info_free(pRouteInfo);
    pRouteInfo = NULL;
    ROUTE_INFO_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : route_info_find
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
int route_info_find(char* server_id)
{
    int pos = -1;
    route_info_t* pRouteInfo = NULL;

    if (NULL == g_RouteInfoList || NULL == server_id)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_find() exit---: Param Error \r\n");
        return -1;
    }

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_find() exit---: Route Info List NULL \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (sstrcmp(pRouteInfo->server_id, server_id) == 0)
        {
            ROUTE_INFO_SMUTEX_UNLOCK();
            return pos;
        }
    }

    ROUTE_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : route_info_find_by_route_index
 ��������  : ����·����������·����Ϣ
 �������  : unsigned int route_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��7��23�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_info_find_by_route_index(unsigned int route_index)
{
    int pos = -1;
    route_info_t* pRouteInfo = NULL;

    if (NULL == g_RouteInfoList || route_index <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_find_by_route_index() exit---: Param Error \r\n");
        return -1;
    }

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_find_by_route_index() exit---: Route Info List NULL \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (pRouteInfo->id == route_index)
        {
            ROUTE_INFO_SMUTEX_UNLOCK();
            return pos;
        }
    }

    ROUTE_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : route_info_find_by_host_and_port
 ��������  : ͨ������˵�host����·����Ϣ
 �������  : (char* server_host, int server_port)
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��16��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_info_find_by_host_and_port(char* server_host, int server_port)
{
    int pos = -1;
    route_info_t* pRouteInfo = NULL;

    if (NULL == g_RouteInfoList || NULL == server_host)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_find_by_host_and_port() exit---: Param Error \r\n");
        return -1;
    }

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_find_by_host_and_port() exit---: Route Info List NULL \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (((sstrcmp(pRouteInfo->server_ip, server_host) == 0)
             || (sstrcmp(pRouteInfo->server_host, server_host) == 0))
            && (server_port == pRouteInfo->server_port))
        {
            ROUTE_INFO_SMUTEX_UNLOCK();
            return pos;
        }
    }

    ROUTE_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : route_info_find_by_reg_index
 ��������  : ͨ��Э��ջ��ע��index����·����Ϣ
 �������  : int reg_info_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��16��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_info_find_by_reg_index(int reg_info_index)
{
    int pos = -1;
    route_info_t* pRouteInfo = NULL;

    if (NULL == g_RouteInfoList || reg_info_index < 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_find_by_reg_index() exit---: Param Error \r\n");
        return -1;
    }

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_find_by_reg_index() exit---: Route Info List NULL \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if ((NULL == pRouteInfo) || (pRouteInfo->reg_info_index == -1))
        {
            continue;
        }

        if (pRouteInfo->reg_info_index == reg_info_index)
        {
            ROUTE_INFO_SMUTEX_UNLOCK();
            return pos;
        }
    }

    ROUTE_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : route_info_get
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
route_info_t* route_info_get(int pos)
{
    route_info_t* pRouteInfo = NULL;

    if (g_RouteInfoList == NULL || pos < 0 || pos >= osip_list_size(g_RouteInfoList->pRouteInfoList))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_get() exit---: Param Error \r\n");
        return NULL;
    }

    pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

    return pRouteInfo;
}
#endif

/*****************************************************************************
 �� �� ��  : set_route_info_list_del_mark
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
int set_route_info_list_del_mark(int del_mark)
{
    int pos = -1;
    route_info_t* pRouteInfo = NULL;

    if (NULL == g_RouteInfoList)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "set_route_info_list_del_mark() exit---: Param Error \r\n");
        return -1;
    }

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "set_route_info_list_del_mark() exit---: Record Info List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (0 != pRouteInfo->id)
        {
            pRouteInfo->del_mark = del_mark;
        }
    }

    ROUTE_INFO_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : set_mms_route_info_list_del_mark
 ��������  : ����MMS��·����Ϣɾ����ʶ
 �������  : int del_mark
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��8��12��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int set_mms_route_info_list_del_mark(int del_mark)
{
    int pos = -1;
    route_info_t* pRouteInfo = NULL;

    if (NULL == g_RouteInfoList)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "set_mms_route_info_list_del_mark() exit---: Param Error \r\n");
        return -1;
    }

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "set_route_info_list_del_mark() exit---: Record Info List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (0 == pRouteInfo->id)
        {
            pRouteInfo->del_mark = del_mark;
        }
    }

    ROUTE_INFO_SMUTEX_UNLOCK();
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
int delete_route_info_from_list_by_mark()
{
    int pos = -1;
    route_info_t* pRouteInfo = NULL;

    if ((NULL == g_RouteInfoList) || (NULL == g_RouteInfoList->pRouteInfoList))
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "delete_route_info_from_list_by_mark() exit---: Param Error \r\n");
        return -1;
    }

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "delete_route_info_from_list_by_mark() exit---: Record Info List NULL \r\n");
        return 0;
    }

    pos = 0;

    while (!osip_list_eol(g_RouteInfoList->pRouteInfoList, pos))
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            osip_list_remove(g_RouteInfoList->pRouteInfoList, pos);
            continue;
        }

        if (0 != pRouteInfo->id && pRouteInfo->del_mark == 2)
        {
            osip_list_remove(g_RouteInfoList->pRouteInfoList, pos);
            route_info_free(pRouteInfo);
            pRouteInfo = NULL;
        }
        else
        {
            pos++;
        }
    }

    ROUTE_INFO_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : delete_mms_route_info_from_list_by_mark
 ��������  : ����ɾ����ʶ��ɾ��MMS·����Ϣ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��9��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int delete_mms_route_info_from_list_by_mark()
{
    int pos = -1;
    route_info_t* pRouteInfo = NULL;

    if ((NULL == g_RouteInfoList) || (NULL == g_RouteInfoList->pRouteInfoList))
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "delete_route_info_from_list_by_mark() exit---: Param Error \r\n");
        return -1;
    }

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "delete_route_info_from_list_by_mark() exit---: Record Info List NULL \r\n");
        return 0;
    }

    pos = 0;

    while (!osip_list_eol(g_RouteInfoList->pRouteInfoList, pos))
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            osip_list_remove(g_RouteInfoList->pRouteInfoList, pos);
            continue;
        }

        if (0 == pRouteInfo->id && pRouteInfo->del_mark == 2)
        {
            osip_list_remove(g_RouteInfoList->pRouteInfoList, pos);
            route_info_free(pRouteInfo);
            pRouteInfo = NULL;
        }
        else
        {
            pos++;
        }
    }

    ROUTE_INFO_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : check_route_info_from_db_to_list
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
int check_route_info_from_db_to_list(DBOper* pRoute_Srv_dboper)
{
    int ret = 0;
    string strSQL = "";
    int route_pos = -1;
    int record_count = 0;
    route_info_t* pOldRouteInfo = NULL;
    int while_count = 0;

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "check_route_info_from_db_to_list() exit---: Route Srv db Oper Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from RouteNetConfig";

    record_count = pRoute_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "check_route_info_from_db_to_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "check_route_info_from_db_to_list() ErrorMsg=%s\r\n", pRoute_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "check_route_info_from_db_to_list() exit---: No Record Count \r\n");
        return 0;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list():record_count=%d  \r\n", record_count);

    /* ѭ���������ݿ�*/
    do
    {
        route_info_t* pRouteInfo = NULL;
        int tmp_ivalue = 0;
        string tmp_svalue = "";
        int send_flag = 0;
        vector<string> SubCmsIPVector;
        int iRet = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "check_route_info_from_db_to_list() While Count=%d \r\n", while_count);
        }

        int i_ret = route_info_init(&pRouteInfo);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "check_route_info_from_db_to_list() route_info_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        /* ���� */
        tmp_ivalue = 0;
        pRoute_Srv_dboper->GetFieldValue("ID", tmp_ivalue);

        pRouteInfo->id = tmp_ivalue;
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "set_db_data_to_route_info_list() pRouteInfo->id: %d", pRouteInfo->id);

        /* �ϼ�������CMSͳһ���id */
        tmp_svalue.clear();
        pRoute_Srv_dboper->GetFieldValue("ServerID", tmp_svalue);

        if (!tmp_svalue.empty() && tmp_svalue.length() > 0)
        {
            osip_strncpy(pRouteInfo->server_id, (char*)tmp_svalue.c_str(), MAX_ID_LEN);
            //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list() pRouteInfo->server_id: %s", pRouteInfo->server_id);
        }
        else
        {
            route_info_free(pRouteInfo);
            pRouteInfo = NULL;
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "check_route_info_from_db_to_list() server_id NULL \r\n");
            continue;
        }

        /* �ϼ�������CMS ip��ַ*/
        tmp_svalue.clear();
        pRoute_Srv_dboper->GetFieldValue("ServerIP", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pRouteInfo->server_ip, (char*)tmp_svalue.c_str(), MAX_IP_LEN);
            //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list() pRouteInfo->server_ip: %s", pRouteInfo->server_ip);
        }
        else
        {
            route_info_free(pRouteInfo);
            pRouteInfo = NULL;
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "check_route_info_from_db_to_list() server_ip NULL \r\n");
            continue;
        }

        /* �ϼ�������CMS ������ַ */
        tmp_svalue.clear();
        pRoute_Srv_dboper->GetFieldValue("ServerHost", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pRouteInfo->server_host, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list() pRouteInfo->server_host: %s", pRouteInfo->server_host);
        }
        else
        {
            //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list() pRouteInfo->server_host NULL");
        }

        /* �ϼ�������CMS �˿ں�*/
        tmp_ivalue = 0;
        pRoute_Srv_dboper->GetFieldValue("ServerPort", tmp_ivalue);

        if (tmp_ivalue > 0)
        {
            pRouteInfo->server_port = tmp_ivalue;
        }

        //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list() pRouteInfo->server_port: %d", pRouteInfo->server_port);

        if (IsLocalHost(pRouteInfo->server_ip) && 0 == sstrcmp(pRouteInfo->server_id, local_cms_id_get()))
        {
            route_info_free(pRouteInfo);
            pRouteInfo = NULL;
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "check_route_info_from_db_to_list() Server Info Is Match Local CMS \r\n");
            continue;
        }

        /* ע���˺� */
        tmp_svalue.clear();
        pRoute_Srv_dboper->GetFieldValue("UserName", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pRouteInfo->register_account, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "check_route_info_from_db_to_list() pRouteInfo->register_account: %s \r\n", pRouteInfo->register_account);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "check_route_info_from_db_to_list() pRouteInfo->register_account NULL \r\n");
        }

        /* ע������ */
        tmp_svalue.clear();
        pRoute_Srv_dboper->GetFieldValue("Password", tmp_svalue);

        if (!tmp_svalue.empty())
        {
            osip_strncpy(pRouteInfo->register_password, (char*)tmp_svalue.c_str(), MAX_128CHAR_STRING_LEN);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "check_route_info_from_db_to_list() pRouteInfo->register_password:%s \r\n", pRouteInfo->register_password);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "check_route_info_from_db_to_list() pRouteInfo->register_password NULL \r\n");
        }

        /* �������� */
        tmp_ivalue = 0;
        pRoute_Srv_dboper->GetFieldValue("LinkType", tmp_ivalue);

        pRouteInfo->link_type = tmp_ivalue;
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "check_route_info_from_db_to_list() pRouteInfo->link_type: %d \r\n", pRouteInfo->link_type);

        /* �Ƿ��ǵ�����ƽ̨ */
        tmp_ivalue = 0;
        pRoute_Srv_dboper->GetFieldValue("Resved1", tmp_ivalue);

        pRouteInfo->three_party_flag = tmp_ivalue;
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "check_route_info_from_db_to_list() pRouteInfo->three_party_flag: %d \r\n", pRouteInfo->three_party_flag);

        /* ����Э������ */
        tmp_ivalue = 0;
        pRoute_Srv_dboper->GetFieldValue("TransferProtocol", tmp_ivalue);

        pRouteInfo->trans_protocol = tmp_ivalue;
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "check_route_info_from_db_to_list() pRouteInfo->trans_protocol: %d \r\n", pRouteInfo->trans_protocol);

        /* ����ʹ�õ��������� */
        tmp_svalue.clear();
        pRoute_Srv_dboper->GetFieldValue("LocalEthName", tmp_svalue);

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

        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "check_route_info_from_db_to_list() pRouteInfo->strRegLocalEthName: %s", pRouteInfo->strRegLocalEthName);

        /* ���¼�CMS��IP��ַȡ�� */
        SubCmsIPVector.clear();
        AddAllSubCMSIPToVector(SubCmsIPVector);

        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "check_route_info_from_db_to_list() SubCmsIPVector.size()=%d \r\n", (int)SubCmsIPVector.size());

        /* ���ض���ip��ַ���˵� */
        iRet = IsIPInSubCMS(SubCmsIPVector, pRouteInfo->server_ip);

        if (1 == iRet)
        {
            pRouteInfo->ip_is_in_sub = 1;
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "check_route_info_from_db_to_list() server_ip=%s Is Sub CMS \r\n", pRouteInfo->server_ip);
        }
        else
        {
            pRouteInfo->ip_is_in_sub = 0;
        }

        /* ����·����Ϣ */
        if (0 == sstrcmp(pRouteInfo->server_id, (char*)"12345678902007654321")) /* ��Ҫ��ȡ������ID */
        {
            route_pos = route_info_find_by_route_index(pRouteInfo->id);
        }
        else
        {
            route_pos = route_info_find(pRouteInfo->server_id);
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "check_route_info_from_db_to_list() route_info_find:route_pos=%d \r\n", route_pos);

        if (route_pos >= 0)
        {
            pOldRouteInfo = route_info_get(route_pos);

            if (NULL != pOldRouteInfo)
            {
                pOldRouteInfo->del_mark = 0;

                if (0 == sstrcmp(pRouteInfo->server_id, (char*)"12345678902007654321")
                    && 0 != sstrcmp(pRouteInfo->server_id, pOldRouteInfo->server_id)) /* ��Ҫ�ٴθ��µ����ݿ� */
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "check_route_info_from_db_to_list() server_id update failed to db:server ip=%s, server ip=%d \r\n", pOldRouteInfo->server_ip, pOldRouteInfo->server_port);
                }

                /* ��������IP ��ַ�Ƿ��б仯 */
                if (0 != sstrcmp(pRouteInfo->server_ip, pOldRouteInfo->server_ip)) /* IP��ַ���޸� */
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list() server_ip changed:old=%s, new=%s \r\n", pOldRouteInfo->server_ip, pRouteInfo->server_ip);

                    memset(pOldRouteInfo->server_ip, 0, MAX_IP_LEN);
                    osip_strncpy(pOldRouteInfo->server_ip, pRouteInfo->server_ip, MAX_IP_LEN);

                    pOldRouteInfo->del_mark = 1;
                }

                /* ��������Ƿ��б仯 */
                if (pRouteInfo->id != pOldRouteInfo->id)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list() id changed:old=%d, new=%d \r\n", pOldRouteInfo->id, pRouteInfo->id);
                    pOldRouteInfo->id = pRouteInfo->id;
                }

                /* ���������˿��Ƿ��б仯 */
                if (pRouteInfo->server_port != pOldRouteInfo->server_port)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list() server_port changed:old=%d, new=%d \r\n", pOldRouteInfo->server_port, pRouteInfo->server_port);
                    pOldRouteInfo->server_port = pRouteInfo->server_port;
                    pOldRouteInfo->del_mark = 1;
                }

                /* ����û����Ƿ��б仯 */
                if (0 != sstrcmp(pRouteInfo->register_account, pOldRouteInfo->register_account)) /* ���޸� */
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list() register_account changed:old=%s, new=%s \r\n", pOldRouteInfo->register_account, pRouteInfo->register_account);

                    memset(pOldRouteInfo->register_account, 0, MAX_128CHAR_STRING_LEN + 4);
                    osip_strncpy(pOldRouteInfo->register_account, pRouteInfo->register_account, MAX_128CHAR_STRING_LEN);

                    pOldRouteInfo->del_mark = 1;
                }

                /* ��������Ƿ��б仯 */
                if (0 != sstrcmp(pRouteInfo->register_password, pOldRouteInfo->register_password)) /* ���޸� */
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list() register_password changed:old=%s, new=%s \r\n", pOldRouteInfo->register_password, pRouteInfo->register_password);

                    memset(pOldRouteInfo->register_password, 0, MAX_128CHAR_STRING_LEN + 4);
                    osip_strncpy(pOldRouteInfo->register_password, pRouteInfo->register_password, MAX_128CHAR_STRING_LEN);

                    pOldRouteInfo->del_mark = 1;
                }

                /* ��Ȿ�������Ƿ��б仯 */
                if (0 != sstrcmp(pRouteInfo->strRegLocalEthName, pOldRouteInfo->strRegLocalEthName)) /* ���޸� */
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list() strRegLocalEthName changed:old=%s, new=%s \r\n", pOldRouteInfo->strRegLocalEthName, pRouteInfo->strRegLocalEthName);
                    memset(pOldRouteInfo->strRegLocalEthName, 0, MAX_IP_LEN);
                    osip_strncpy(pOldRouteInfo->strRegLocalEthName, pRouteInfo->strRegLocalEthName, MAX_IP_LEN);

                    pOldRouteInfo->del_mark = 1;
                }

                /* ������������Ƿ��б仯 */
                if (pRouteInfo->link_type != pOldRouteInfo->link_type)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list() link_type changed:old=%d, new=%d \r\n", pOldRouteInfo->link_type, pRouteInfo->link_type);
                    pOldRouteInfo->link_type = pRouteInfo->link_type;
                    pOldRouteInfo->del_mark = 1;
                }

                /* ��������ƽ̨��ʶ�Ƿ��б仯 */
                if (pRouteInfo->three_party_flag != pOldRouteInfo->three_party_flag)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list() three_party_flag changed:old=%d, new=%d \r\n", pOldRouteInfo->three_party_flag, pRouteInfo->three_party_flag);
                    pOldRouteInfo->three_party_flag = pRouteInfo->three_party_flag;

                    send_flag = 1;
                }

                /* ����Ƿ��ǵ�����ƽ̨�Ƿ��б仯 */
                if (pRouteInfo->trans_protocol != pOldRouteInfo->trans_protocol)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list() trans_protocol changed:old=%d, new=%d \r\n", pOldRouteInfo->trans_protocol, pRouteInfo->trans_protocol);
                    pOldRouteInfo->trans_protocol = pRouteInfo->trans_protocol;

                    send_flag = 1;
                }

                /* ����ϼ�CMS��IP��ַ���¼�CMS��ip��ַ�Ƿ��г�ͻ */
                if (pRouteInfo->ip_is_in_sub != pOldRouteInfo->ip_is_in_sub)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "check_route_info_from_db_to_list() ip_is_in_sub changed:old=%d, new=%d \r\n", pOldRouteInfo->ip_is_in_sub, pRouteInfo->ip_is_in_sub);
                    pOldRouteInfo->ip_is_in_sub = pRouteInfo->ip_is_in_sub;
                }

                if (1 == send_flag)
                {
                    ret = SendDeviceInfoMessageToRouteCMS(pOldRouteInfo->server_id, pOldRouteInfo->strRegLocalIP, pOldRouteInfo->iRegLocalPort, pOldRouteInfo->server_ip, pOldRouteInfo->server_port, (char*)"123", local_cms_id_get(), pOldRouteInfo->three_party_flag, pOldRouteInfo->trans_protocol);
                }

                route_info_free(pRouteInfo);
                pRouteInfo = NULL;
                continue;
            }
        }
        else
        {
            /* ��ӵ����� */
            if (route_info_add(pRouteInfo) < 0)
            {
                route_info_free(pRouteInfo);
                pRouteInfo = NULL;
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "check_route_info_from_db_to_list() Record Info Add Error \r\n");
                continue;
            }

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ϼ�·��������Ϣ: Route CMS ID=%s, Route CMS IP=%s, Route CMS Port=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Add Route Info: Route CMS ID=%s, Route CMS IP=%s, Route CMS Port=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
    }
    while (pRoute_Srv_dboper->MoveNext() >= 0);

    return ret;
}

/*****************************************************************************
 �� �� ��  : scan_route_info_list
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
void scan_route_info_list()
{
    int i = 0;
    int iRet = -1;
    route_info_t* pRouteInfo = NULL;
    route_info_t* pGetServerIDRouteInfo = NULL;
    route_info_t* pRegisterRouteInfo = NULL;
    route_info_t* pRefreshRouteInfo = NULL;
    route_info_t* pKeepAliveRouteInfo = NULL;
    route_info_t* pUnRegisterRouteInfo = NULL;
    char* local_ip = NULL;
    int local_port = 0;
    needtoproc_routeinfo_queue needToGetServerID;
    needtoproc_routeinfo_queue needRegister;
    needtoproc_routeinfo_queue needRefresh;
    needtoproc_routeinfo_queue needKeepAlive;
    needtoproc_routeinfo_queue needUnRegister;

    if ((NULL == g_RouteInfoList) || (NULL == g_RouteInfoList->pRouteInfoList))
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "scan_route_info_list() exit---: Param Error \r\n");
        return;
    }

    needToGetServerID.clear();
    needRegister.clear();
    needRefresh.clear();
    needUnRegister.clear();

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_RouteInfoList->pRouteInfoList); i++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, i);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (!sys_show_code_flag_get()) /* �ǹ���ģʽ */
        {
            if (0 == sstrcmp(pRouteInfo->server_id, (char*)"12345678902007654321")) /* ��Ҫ��ȡ������ID */
            {
                needToGetServerID.push_back(pRouteInfo);
                pRouteInfo->del_mark = 0;
                continue;
            }
        }

        if (pRouteInfo->del_mark == 1)  /* ��Ҫ���·���ע�� */
        {
            needUnRegister.push_back(pRouteInfo);
            pRouteInfo->del_mark = 0;
            continue;
        }
        else if (pRouteInfo->del_mark == 2) /* ��Ҫȥע�� */
        {
            needUnRegister.push_back(pRouteInfo);
            continue;
        }
        else if (pRouteInfo->del_mark == 0) /* ��Ҫ����ע�����ˢ��ע�� */
        {
            if (pRouteInfo->reg_status == 0)
            {
                pRouteInfo->reg_interval--;

                if (pRouteInfo->reg_interval <= 0)
                {
                    needRegister.push_back(pRouteInfo); /* ���� ע�� */
                }
            }
            else
            {
                pRouteInfo->expires--;
                pRouteInfo->keep_alive_count--;

                if (pRouteInfo->expires <= (pRouteInfo->min_expires) / 2)
                {
                    needRefresh.push_back(pRouteInfo); /* ����ˢ��ע�� */
                }
                else if (pRouteInfo->keep_alive_count <= 0)
                {
                    needKeepAlive.push_back(pRouteInfo);  /* ���ͱ�����Ϣ */
                }

                /* ������Ч��ɨ�� */
                if (1 == pRouteInfo->catalog_subscribe_flag)
                {
                    pRouteInfo->catalog_subscribe_expires_count--;

                    if (pRouteInfo->catalog_subscribe_expires_count <= 0)
                    {
                        pRouteInfo->catalog_subscribe_flag = 0;
                        pRouteInfo->catalog_subscribe_dialog_index = -1;
                    }
                }
            }
        }
    }

    ROUTE_INFO_SMUTEX_UNLOCK();

    /* ������Ҫ���ͻ�ȡ������ID��Ϣ�� */
    while (!needToGetServerID.empty())
    {
        pGetServerIDRouteInfo = (route_info_t*) needToGetServerID.front();
        needToGetServerID.pop_front();

        if (NULL != pGetServerIDRouteInfo)
        {
            /* ���ͻ�ȡ������ID����Ϣ */
            iRet = SendGetServerIDMessageToRouteCMS(pGetServerIDRouteInfo);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "scan_route_info_list() SendGetServerIDMessageToRouteCMS Error:server_id=%s, server_ip=%s, server_port=%d, local_id=%s, local_ip=%s, local_port=%d, link_type=%d, iRet=%d \r\n", pGetServerIDRouteInfo->server_id, pGetServerIDRouteInfo->server_ip, pGetServerIDRouteInfo->server_port, local_cms_id_get(), local_ip, local_port, pGetServerIDRouteInfo->link_type, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "scan_route_info_list() SendGetServerIDMessageToRouteCMS OK:server_id=%s, server_ip=%s, server_port=%d, local_id=%s, local_ip=%s, local_port=%d, link_type=%d, iRet=%d \r\n", pGetServerIDRouteInfo->server_id, pGetServerIDRouteInfo->server_ip, pGetServerIDRouteInfo->server_port, local_cms_id_get(), local_ip, local_port, pGetServerIDRouteInfo->link_type, iRet);
            }
        }
    }

    needToGetServerID.clear();

    /* ������Ҫ����ע����Ϣ�� */
    while (!needRegister.empty())
    {
        pRegisterRouteInfo = (route_info_t*) needRegister.front();
        needRegister.pop_front();

        if (NULL != pRegisterRouteInfo)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�·��û��ע��, ��Ҫ����ע����Ϣ: �ϼ�·��ID=%s, �ϼ�·��IP=%s, �ϼ�·��Port=%d", pRegisterRouteInfo->server_id, pRegisterRouteInfo->server_ip, pRegisterRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Route Info Not Registered, need to Register: Route CMS ID=%s, Route CMS IP=%s, Route CMS Port=%d", pRegisterRouteInfo->server_id, pRegisterRouteInfo->server_ip, pRegisterRouteInfo->server_port);

            /* ��ȡ����ip��ַ�Ͷ˿ں� */
            local_ip = local_ip_get(pRegisterRouteInfo->strRegLocalEthName);

            if (NULL == local_ip)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "scan_route_info_list(): Find Local Addr Error: strRegLocalEthName=%s, Get By Default EthName \r\n", pRegisterRouteInfo->strRegLocalEthName);

                /* ��Ĭ�ϵ������ٻ�ȡһ�� */
                local_ip = local_ip_get(default_eth_name_get());

                if (NULL == local_ip)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "scan_route_info_list() exit---: Find Local Addr Error: strRegLocalEthName=%s \r\n", pRegisterRouteInfo->strRegLocalEthName);
                    continue;
                }
            }

            local_port = local_port_get(pRegisterRouteInfo->strRegLocalEthName);

            if (local_port <= 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "scan_route_info_list(): Get Local Port Error : strRegLocalEthName=%s, Get By Default EthName \r\n", pRegisterRouteInfo->strRegLocalEthName);

                /* ��Ĭ�ϵ������ٻ�ȡһ�� */
                local_port = local_port_get(default_eth_name_get());

                if (local_port <= 0)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "scan_route_info_list() exit---: Get Local Port Error : strRegLocalEthName=%s \r\n", pRegisterRouteInfo->strRegLocalEthName);
                    continue;
                }
            }

            /* ���ͳ�ʼע�� */
            iRet = SIP_SendRegisterForRoute(pRegisterRouteInfo->server_id, local_cms_id_get(), local_ip, local_port, pRegisterRouteInfo->server_ip, pRegisterRouteInfo->server_port, pRegisterRouteInfo->register_account, pRegisterRouteInfo->register_password, local_registry_cleanuprate_get(), pRegisterRouteInfo->link_type);

            if (iRet < 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ע����Ϣ���ϼ�·��ʧ��: �ϼ�·��ID=%s, �ϼ�·��IP=%s, �ϼ�·��Port=%d", pRegisterRouteInfo->server_id, pRegisterRouteInfo->server_ip, pRegisterRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send Register To Route Error: Route CMS ID=%s, Route CMS IP=%s, Route CMS Port=%d", pRegisterRouteInfo->server_id, pRegisterRouteInfo->server_ip, pRegisterRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "scan_route_info_list() SIP_SendRegisterForRoute Error:server_id=%s, server_ip=%s, server_port=%d, local_id=%s, local_ip=%s, local_port=%d, link_type=%d, iRet=%d \r\n", pRegisterRouteInfo->server_id, pRegisterRouteInfo->server_ip, pRegisterRouteInfo->server_port, local_cms_id_get(), local_ip, local_port, pRegisterRouteInfo->link_type, iRet);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ע����Ϣ���ϼ�·�ɳɹ�: �ϼ�·��ID=%s, �ϼ�·��IP=%s, �ϼ�·��Port=%d", pRegisterRouteInfo->server_id, pRegisterRouteInfo->server_ip, pRegisterRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send Register To Route OK: Route CMS ID=%s, Route CMS IP=%s, Route CMS Port=%d", pRegisterRouteInfo->server_id, pRegisterRouteInfo->server_ip, pRegisterRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "scan_route_info_list() SIP_SendRegisterForRoute OK:server_id=%s, server_ip=%s, server_port=%d, local_id=%s, local_ip=%s, local_port=%d, link_type=%d, iRet=%d \r\n", pRegisterRouteInfo->server_id, pRegisterRouteInfo->server_ip, pRegisterRouteInfo->server_port, local_cms_id_get(), local_ip, local_port, pRegisterRouteInfo->link_type, iRet);
            }

            if (iRet >= 0)
            {
                pRegisterRouteInfo->reg_info_index = iRet;
                pRegisterRouteInfo->reg_status = 0;
                pRegisterRouteInfo->expires = 0;
                pRegisterRouteInfo->min_expires = 0;
                pRegisterRouteInfo->keep_alive_count = local_keep_alive_interval_get();
                pRegisterRouteInfo->failed_keep_alive_count = 0;
                pRegisterRouteInfo->reg_interval = local_register_retry_interval_get();
            }
            else
            {
                pRegisterRouteInfo->reg_info_index = -1;
                pRegisterRouteInfo->reg_status = 0;
                pRegisterRouteInfo->expires = 0;
                pRegisterRouteInfo->min_expires = 0;
                pRegisterRouteInfo->keep_alive_count = local_keep_alive_interval_get();
                pRegisterRouteInfo->failed_keep_alive_count = 0;
                pRegisterRouteInfo->reg_interval = 0;

                /* �Ƴ��ϼ������ĵ�λ��Ϣ */
                iRet = RemoveGBLogicDeviceLockInfoByRouteInfo(pRouteInfo);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_route_info_list() RemoveGBLogicDeviceLockInfoByRouteInfo Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_route_info_list() RemoveGBLogicDeviceLockInfoByRouteInfo OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }

                iRet = StopAllServiceTaskByCallerIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_route_info_list() StopAllServiceTaskByCallerIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_route_info_list() StopAllServiceTaskByCallerIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }

                /* �������ϼ�CMS������ */
                iRet = StopAllServiceTaskByCalleeIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_route_info_list() StopAllServiceTaskByCalleeIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_route_info_list() StopAllServiceTaskByCalleeIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }

                /* ����ҵ�����߳� */
                iRet = route_srv_proc_thread_recycle(pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "scan_route_info_list() route_srv_proc_thread_recycle Error:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "scan_route_info_list() route_srv_proc_thread_recycle OK:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
            }

            memset(pRegisterRouteInfo->strRegLocalIP, 0, MAX_IP_LEN);
            osip_strncpy(pRegisterRouteInfo->strRegLocalIP, local_ip, MAX_IP_LEN);
            pRegisterRouteInfo->iRegLocalPort = local_port;
        }
    }

    needRegister.clear();

    /* ������Ҫ����ˢ��ע����Ϣ�� */
    while (!needRefresh.empty())
    {
        pRefreshRouteInfo = (route_info_t*) needRefresh.front();
        needRefresh.pop_front();

        if (NULL != pRefreshRouteInfo)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�·����Ҫ����ˢ��ע����Ϣ: �ϼ�·��ID=%s, �ϼ�·��IP=%s, �ϼ�·��Port=%d", pRefreshRouteInfo->server_id, pRefreshRouteInfo->server_ip, pRefreshRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Route Info Need Refresh Register: Route CMS ID=%s, Route CMS IP=%s, Route CMS Port=%d", pRefreshRouteInfo->server_id, pRefreshRouteInfo->server_ip, pRefreshRouteInfo->server_port);

            /* ����ˢ��ע�� */
            iRet = SIP_SendRegisterForRefresh(pRefreshRouteInfo->reg_info_index);

            if (iRet < 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ˢ��ע����Ϣ���ϼ�·��ʧ��: �ϼ�·��ID=%s, �ϼ�·��IP=%s, �ϼ�·��Port=%d", pRefreshRouteInfo->server_id, pRefreshRouteInfo->server_ip, pRefreshRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send Refresh Register To Route Error: Route CMS ID=%s, Route CMS IP=%s, Route CMS Port=%d", pRefreshRouteInfo->server_id, pRefreshRouteInfo->server_ip, pRefreshRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "scan_route_info_list() SIP_SendRegisterForRefresh Error:reg_info_index=%d, iRet=%d \r\n", pRefreshRouteInfo->reg_info_index, iRet);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ˢ��ע����Ϣ���ϼ�·�ɳɹ�: �ϼ�·��ID=%s, �ϼ�·��IP=%s, �ϼ�·��Port=%d", pRefreshRouteInfo->server_id, pRefreshRouteInfo->server_ip, pRefreshRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send Refresh Register To Route OK: Route CMS ID=%s, Route CMS IP=%s, Route CMS Port=%d", pRefreshRouteInfo->server_id, pRefreshRouteInfo->server_ip, pRefreshRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "scan_route_info_list() SIP_SendRegisterForRefresh OK:reg_info_index=%d, iRet=%d \r\n", pRefreshRouteInfo->reg_info_index, iRet);
            }

            if (iRet >= 0)
            {
                pRefreshRouteInfo->expires = 0;
                pRefreshRouteInfo->min_expires = 0;
                pRefreshRouteInfo->keep_alive_count = local_keep_alive_interval_get();
                pRefreshRouteInfo->failed_keep_alive_count = 0;
                pRefreshRouteInfo->reg_interval = local_register_retry_interval_get();
            }
            else /* ����Э��ջ��ʱ��reg_info_index�Ѿ�û���ˣ�ע��ˢ��ʧ�ܣ����ʱ����Ҫ���·���ע�� */
            {
                pRefreshRouteInfo->reg_info_index = -1;
                pRefreshRouteInfo->reg_status = 0;
                pRefreshRouteInfo->expires = 0;
                pRefreshRouteInfo->min_expires = 0;
                pRefreshRouteInfo->keep_alive_count = local_keep_alive_interval_get();
                pRefreshRouteInfo->failed_keep_alive_count = 0;
                pRefreshRouteInfo->reg_interval = 0;

                /* �Ƴ��ϼ������ĵ�λ��Ϣ */
                iRet = RemoveGBLogicDeviceLockInfoByRouteInfo(pRouteInfo);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_route_info_list() RemoveGBLogicDeviceLockInfoByRouteInfo Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_route_info_list() RemoveGBLogicDeviceLockInfoByRouteInfo OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }

                iRet = StopAllServiceTaskByCallerIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_route_info_list() StopAllServiceTaskByCallerIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_route_info_list() StopAllServiceTaskByCallerIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }

                /* �������ϼ�CMS������ */
                iRet = StopAllServiceTaskByCalleeIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_route_info_list() StopAllServiceTaskByCalleeIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_route_info_list() StopAllServiceTaskByCalleeIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }

                /* ����ҵ�����߳� */
                iRet = route_srv_proc_thread_recycle(pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "scan_route_info_list() route_srv_proc_thread_recycle Error:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "scan_route_info_list() route_srv_proc_thread_recycle OK:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
            }
        }
    }

    needRefresh.clear();

    /* ������Ҫ���ͱ�����Ϣ�� */
    while (!needKeepAlive.empty())
    {
        pKeepAliveRouteInfo = (route_info_t*) needKeepAlive.front();
        needKeepAlive.pop_front();

        if (NULL != pKeepAliveRouteInfo)
        {
            /* ���ͱ�����Ϣ */
            iRet = SendKeepAliveMessageToRouteCMS(pKeepAliveRouteInfo);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "scan_route_info_list() SendKeepAliveMessageToRouteCMS Error:reg_info_index=%d, iRet=%d \r\n", pKeepAliveRouteInfo->reg_info_index, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "scan_route_info_list() SendKeepAliveMessageToRouteCMS OK:reg_info_index=%d, iRet=%d \r\n", pKeepAliveRouteInfo->reg_info_index, iRet);
            }

            if (iRet == 0)
            {
                pKeepAliveRouteInfo->keep_alive_count = local_keep_alive_interval_get();
            }
        }
    }

    needKeepAlive.clear();

    /* ������Ҫ����ȥע����Ϣ�� */
    while (!needUnRegister.empty())
    {
        pUnRegisterRouteInfo = (route_info_t*) needUnRegister.front();
        needUnRegister.pop_front();

        if (NULL != pUnRegisterRouteInfo)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�·��������Ϣ�����仯����ɾ��, ��Ҫ��ע��: �ϼ�·��ID=%s, �ϼ�·��IP=%s, �ϼ�·��Port=%d", pUnRegisterRouteInfo->server_id, pUnRegisterRouteInfo->server_ip, pUnRegisterRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Route Info changed or delete, need to UnRegister: Route CMS ID=%s, Route CMS IP=%s, Route CMS Port=%d", pUnRegisterRouteInfo->server_id, pUnRegisterRouteInfo->server_ip, pUnRegisterRouteInfo->server_port);

            if (pUnRegisterRouteInfo->reg_status == 1 && pUnRegisterRouteInfo->reg_info_index >= 0)
            {
                iRet = SIP_SendUnRegister(pUnRegisterRouteInfo->reg_info_index);

                if (iRet < 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ע����Ϣ���ϼ�·��ʧ��: �ϼ�·��ID=%s, �ϼ�·��IP=%s, �ϼ�·��Port=%d", pUnRegisterRouteInfo->server_id, pUnRegisterRouteInfo->server_ip, pUnRegisterRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send UnRegister To Route Info Error: Route CMS ID=%s, Route CMS IP=%s, Route CMS Port=%d", pUnRegisterRouteInfo->server_id, pUnRegisterRouteInfo->server_ip, pUnRegisterRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "scan_route_info_list() SIP_SendUnRegister Error:reg_info_index=%d, iRet=%d \r\n", pUnRegisterRouteInfo->reg_info_index, iRet);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ע����Ϣ���ϼ�·�ɳɹ�: �ϼ�·��ID=%s, �ϼ�·��IP=%s, �ϼ�·��Port=%d", pUnRegisterRouteInfo->server_id, pUnRegisterRouteInfo->server_ip, pUnRegisterRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send UnRegister To Route Info OK: Route CMS ID=%s, Route CMS IP=%s, Route CMS Port=%d", pUnRegisterRouteInfo->server_id, pUnRegisterRouteInfo->server_ip, pUnRegisterRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "scan_route_info_list() SIP_SendUnRegister OK:reg_info_index=%d, iRet=%d \r\n", pUnRegisterRouteInfo->reg_info_index, iRet);
                }

                if (iRet >= 0)
                {
                    pUnRegisterRouteInfo->expires = 0;
                    pUnRegisterRouteInfo->min_expires = 0;
                    pUnRegisterRouteInfo->keep_alive_count = local_keep_alive_interval_get();
                    pUnRegisterRouteInfo->failed_keep_alive_count = 0;
                    pUnRegisterRouteInfo->reg_interval = local_register_retry_interval_get();
                }
                else
                {
                    pUnRegisterRouteInfo->reg_info_index = -1;
                    pUnRegisterRouteInfo->reg_status = 0;
                    pUnRegisterRouteInfo->expires = 0;
                    pUnRegisterRouteInfo->min_expires = 0;
                    pUnRegisterRouteInfo->keep_alive_count = local_keep_alive_interval_get();
                    pUnRegisterRouteInfo->failed_keep_alive_count = 0;
                    pUnRegisterRouteInfo->reg_interval = 0;

                    /* �Ƴ��ϼ������ĵ�λ��Ϣ */
                    iRet = RemoveGBLogicDeviceLockInfoByRouteInfo(pRouteInfo);

                    if (0 != iRet)
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_route_info_list() RemoveGBLogicDeviceLockInfoByRouteInfo Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_route_info_list() RemoveGBLogicDeviceLockInfoByRouteInfo OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                    }

                    iRet = StopAllServiceTaskByCallerIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

                    if (0 != iRet)
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_route_info_list() StopAllServiceTaskByCallerIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_route_info_list() StopAllServiceTaskByCallerIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                    }

                    /* �������ϼ�CMS������ */
                    iRet = StopAllServiceTaskByCalleeIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

                    if (0 != iRet)
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_route_info_list() StopAllServiceTaskByCalleeIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_route_info_list() StopAllServiceTaskByCalleeIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                    }

                    /* ����ҵ�����߳� */
                    iRet = route_srv_proc_thread_recycle(pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

                    if (0 != iRet)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "scan_route_info_list() route_srv_proc_thread_recycle Error:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "scan_route_info_list() route_srv_proc_thread_recycle OK:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                    }
                }
            }
        }
    }

    needUnRegister.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : SendMessageToRouteCMS
 ��������  : ����SIP Message��Ϣ���ϼ�CMS
 �������  : unsigned int device_index
             char* callee_id
             char* msg
             int msg_len
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��11�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendMessageToRouteCMS(unsigned int device_index, char* callee_id, char* msg, int msg_len, DBOper* pDboper)
{
    int i = 0;
    int pos = -1;
    //int iRet = 0;
    route_info_t* pRouteInfo = NULL;
    needtoproc_routeinfo_queue needProc;

    if (device_index <= 0 || NULL == msg || msg_len <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendMessageToRouteCMS() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == g_RouteInfoList)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendMessageToRouteCMS() exit---: Route Info List Error \r\n");
        return -1;
    }

    needProc.clear();

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendMessageToRouteCMS() exit---: Route Info List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (pRouteInfo->reg_status == 0 || pRouteInfo->reg_info_index < 0)
        {
            continue;
        }

        /* �жϷ�����IP��ַ�Ƿ����¼�CMS�ĵ�ַ��������¼�CMS�ĵ�ַ���򲻷�����Ϣ�� */
        if ('\0' != pRouteInfo->server_ip[0])
        {
            /* ���ض���ip��ַ���˵� */
            //iRet = IsIPInSubCMS(SubCmsIPVector, pRouteInfo->server_ip);

            if (pRouteInfo->ip_is_in_sub)
            {
                //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendMessageToRouteCMS() server_ip=%s ignore Send \r\n", pRouteInfo->server_ip);
                continue;
            }
        }

        needProc.push_back(pRouteInfo);
    }

    ROUTE_INFO_SMUTEX_UNLOCK();

    if ((int)needProc.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "������Ϣ�������ϼ�CMS: �ϼ�CMS��=%d", (int)needProc.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send a message to all the superior CMS: superior CMS = % d", (int)needProc.size());

    while (!needProc.empty())
    {
        pRouteInfo = (route_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pRouteInfo)
        {
            /* �ж��ϼ�·���Ƿ��иõ�λ��Ȩ�� */
            if (!IsRouteHasPermissionForDevice(device_index, pRouteInfo, pDboper))
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����Message��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �ϼ�CMSû�иĵ�λ��Ȩ��:��λ����=%u", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_index);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Message sent to the superior CMS failure:ID = % s = % s IP address, port number = % d , users do not have permission to change point ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                continue;
            }

            /* ������Ϣ���ϼ�CMS */
            if (NULL == callee_id)
            {
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pRouteInfo->server_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, msg, msg_len);
            }
            else
            {
                i |= SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, msg, msg_len);
            }

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Message��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendMessageToRouteCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Message��Ϣ���ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send Message Message to superiors CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendMessageToRouteCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
    }

    needProc.clear();

    return 1;
}

/*****************************************************************************
 �� �� ��  : SendMessageToOwnerRouteCMS
 ��������  : ����SIP Message��Ϣ���Լ����ϼ�ƽ̨
 �������  : unsigned int device_index
             char* callee_id
             char* msg
             int msg_len
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��12��10��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendMessageToOwnerRouteCMS(unsigned int device_index, char* callee_id, char* msg, int msg_len, DBOper* pDboper)
{
    int i = 0;
    int pos = -1;
    //int iRet = 0;
    route_info_t* pRouteInfo = NULL;
    needtoproc_routeinfo_queue needProc;

    if (device_index <= 0 || NULL == msg || msg_len <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendMessageToOwnerRouteCMS() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == g_RouteInfoList)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendMessageToOwnerRouteCMS() exit---: Route Info List Error \r\n");
        return -1;
    }

    needProc.clear();

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendMessageToOwnerRouteCMS() exit---: Route Info List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (pRouteInfo->reg_status == 0 || pRouteInfo->reg_info_index < 0)
        {
            continue;
        }

        if (pRouteInfo->three_party_flag == 1) /* ������ƽ̨������״̬��Ϣ */
        {
            continue;
        }

        /* �жϷ�����IP��ַ�Ƿ����¼�CMS�ĵ�ַ��������¼�CMS�ĵ�ַ���򲻷�����Ϣ�� */
        if ('\0' != pRouteInfo->server_ip[0])
        {
            if (pRouteInfo->ip_is_in_sub)
            {
                //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendMessageToOwnerRouteCMS() server_ip=%s ignore Send \r\n", pRouteInfo->server_ip);
                continue;
            }
        }

        needProc.push_back(pRouteInfo);
    }

    ROUTE_INFO_SMUTEX_UNLOCK();

    if ((int)needProc.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Message��Ϣ�����б����ϼ�CMS: �ϼ�CMS��=%d", (int)needProc.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send a message to all the local superior CMS: superior CMS = % d", (int)needProc.size());

    while (!needProc.empty())
    {
        pRouteInfo = (route_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pRouteInfo)
        {
            /* �ж��ϼ�·���Ƿ��иõ�λ��Ȩ�� */
            if (!IsRouteHasPermissionForDevice(device_index, pRouteInfo, pDboper))
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����Message��Ϣ�������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �ϼ�CMSû�иĵ�λ��Ȩ��:��λ����=%u", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_index);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Message sent to the local superior CMS failure:ID = % s = % s IP address, port number = % d , users do not have permission to change point ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                continue;
            }

            /* ������Ϣ���ϼ�CMS */
            if (NULL == callee_id)
            {
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pRouteInfo->server_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, msg, msg_len);
            }
            else
            {
                i |= SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, msg, msg_len);
            }

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Message��Ϣ�������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Message sent to the local superior CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendMessageToOwnerRouteCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Message��Ϣ�������ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Message sent to the local superior CMS success: the superior CMS, ID = % s = % s IP address, port number = % d ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendMessageToOwnerRouteCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
    }

    needProc.clear();

    return 1;
}

/*****************************************************************************
 �� �� ��  : SendMessageToOwnerRouteCMS2
 ��������  : ����SIP Message��Ϣ���Լ����ϼ�ƽ̨�����жϵ�λȨ��
 �������  : char* callee_id
             char* msg
             int msg_len
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��5��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendMessageToOwnerRouteCMS2(char* callee_id, char* msg, int msg_len, DBOper* pDboper)
{
    int i = 0;
    int pos = -1;
    //int iRet = 0;
    route_info_t* pRouteInfo = NULL;
    needtoproc_routeinfo_queue needProc;

    if (NULL == msg || msg_len <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendMessageToOwnerRouteCMS2() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == g_RouteInfoList)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendMessageToOwnerRouteCMS2() exit---: Route Info List Error \r\n");
        return -1;
    }

    needProc.clear();

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendMessageToOwnerRouteCMS() exit---: Route Info List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (pRouteInfo->reg_status == 0 || pRouteInfo->reg_info_index < 0)
        {
            continue;
        }

        if (pRouteInfo->three_party_flag == 1) /* ������ƽ̨������״̬��Ϣ */
        {
            continue;
        }

        /* �жϷ�����IP��ַ�Ƿ����¼�CMS�ĵ�ַ��������¼�CMS�ĵ�ַ���򲻷�����Ϣ�� */
        if ('\0' != pRouteInfo->server_ip[0])
        {
            if (pRouteInfo->ip_is_in_sub)
            {
                //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendMessageToOwnerRouteCMS() server_ip=%s ignore Send \r\n", pRouteInfo->server_ip);
                continue;
            }
        }

        needProc.push_back(pRouteInfo);
    }

    ROUTE_INFO_SMUTEX_UNLOCK();

    if ((int)needProc.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Message��Ϣ�����б����ϼ�CMS: �ϼ�CMS��=%d", (int)needProc.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send a message to all the local superior CMS: superior CMS = % d", (int)needProc.size());

    while (!needProc.empty())
    {
        pRouteInfo = (route_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pRouteInfo)
        {
            /* ������Ϣ���ϼ�CMS */
            if (NULL == callee_id)
            {
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pRouteInfo->server_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, msg, msg_len);
            }
            else
            {
                i |= SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, msg, msg_len);
            }

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Message��Ϣ�������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Message sent to the local superior CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendMessageToOwnerRouteCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Message��Ϣ�������ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Message sent to the local superior CMS success: the superior CMS, ID = % s = % s IP address, port number = % d ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendMessageToOwnerRouteCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
    }

    needProc.clear();

    return 1;
}

/*****************************************************************************
 �� �� ��  : SendMessageToOwnerRouteCMSExceptMMS
 ��������  : ����SIP Message��Ϣ���Լ����ϼ�ƽ̨������MMS
 �������  : char* callee_id
             char* msg
             int msg_len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��29��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendMessageToOwnerRouteCMSExceptMMS(char* callee_id, char* msg, int msg_len)
{
    int i = 0;
    int pos = -1;
    //int iRet = 0;
    route_info_t* pRouteInfo = NULL;
    needtoproc_routeinfo_queue needProc;

    if (NULL == msg || msg_len <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendMessageToOwnerRouteCMSExceptMMS() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == g_RouteInfoList)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendMessageToOwnerRouteCMSExceptMMS() exit---: Route Info List Error \r\n");
        return -1;
    }

    needProc.clear();

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendMessageToOwnerRouteCMSExceptMMS() exit---: Route Info List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (0 == pRouteInfo->id) /* IDΪ0����MMS */
        {
            continue;
        }

        if (pRouteInfo->reg_status == 0 || pRouteInfo->reg_info_index < 0)
        {
            continue;
        }

        if (pRouteInfo->three_party_flag == 1) /* ������ƽ̨������״̬��Ϣ */
        {
            continue;
        }

        /* �жϷ�����IP��ַ�Ƿ����¼�CMS�ĵ�ַ��������¼�CMS�ĵ�ַ���򲻷�����Ϣ�� */
        if ('\0' != pRouteInfo->server_ip[0])
        {
            if (pRouteInfo->ip_is_in_sub)
            {
                //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendMessageToOwnerRouteCMS() server_ip=%s ignore Send \r\n", pRouteInfo->server_ip);
                continue;
            }
        }

        needProc.push_back(pRouteInfo);
    }

    ROUTE_INFO_SMUTEX_UNLOCK();

    if ((int)needProc.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Message��Ϣ�����б����ϼ�CMS: �ϼ�CMS��=%d", (int)needProc.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send a message to all the local superior CMS: superior CMS = % d", (int)needProc.size());

    while (!needProc.empty())
    {
        pRouteInfo = (route_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pRouteInfo)
        {
            /* ������Ϣ���ϼ�CMS */
            if (NULL == callee_id)
            {
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pRouteInfo->server_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, msg, msg_len);
            }
            else
            {
                i |= SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, msg, msg_len);
            }

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Message��Ϣ�������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Message sent to the local superior CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendMessageToOwnerRouteCMSExceptMMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Message��Ϣ�������ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Message sent to the local superior CMS success: the superior CMS, ID = % s = % s IP address, port number = % d ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendMessageToOwnerRouteCMSExceptMMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
    }

    needProc.clear();

    return 1;
}

/*****************************************************************************
 �� �� ��  : SendMessageToOwnerRouteCMSExceptMMS2
 ��������  : ����SIP Message��Ϣ���Լ����ϼ�ƽ̨������MMS
 �������  : unsigned int device_index
             char* callee_id
             char* msg
             int msg_len
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��12��10��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendMessageToOwnerRouteCMSExceptMMS2(unsigned int device_index, char* callee_id, char* msg, int msg_len, DBOper* pDboper)
{
    int i = 0;
    int pos = -1;
    //int iRet = 0;
    route_info_t* pRouteInfo = NULL;
    needtoproc_routeinfo_queue needProc;

    if (device_index <= 0 || NULL == msg || msg_len <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendMessageToOwnerRouteCMSExceptMMS() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == g_RouteInfoList)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendMessageToOwnerRouteCMSExceptMMS() exit---: Route Info List Error \r\n");
        return -1;
    }

    needProc.clear();

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendMessageToOwnerRouteCMSExceptMMS() exit---: Route Info List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (0 == pRouteInfo->id) /* IDΪ0����MMS */
        {
            continue;
        }

        if (pRouteInfo->reg_status == 0 || pRouteInfo->reg_info_index < 0)
        {
            continue;
        }

        if (pRouteInfo->three_party_flag == 1) /* ������ƽ̨������״̬��Ϣ */
        {
            continue;
        }

        /* �жϷ�����IP��ַ�Ƿ����¼�CMS�ĵ�ַ��������¼�CMS�ĵ�ַ���򲻷�����Ϣ�� */
        if ('\0' != pRouteInfo->server_ip[0])
        {
            if (pRouteInfo->ip_is_in_sub)
            {
                //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendMessageToOwnerRouteCMS() server_ip=%s ignore Send \r\n", pRouteInfo->server_ip);
                continue;
            }
        }

        needProc.push_back(pRouteInfo);
    }

    ROUTE_INFO_SMUTEX_UNLOCK();

    if ((int)needProc.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Message��Ϣ�����б����ϼ�CMS: �ϼ�CMS��=%d", (int)needProc.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send a message to all the local superior CMS: superior CMS = % d", (int)needProc.size());

    while (!needProc.empty())
    {
        pRouteInfo = (route_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pRouteInfo)
        {
            /* �ж��ϼ�·���Ƿ��иõ�λ��Ȩ�� */
            if (!IsRouteHasPermissionForDevice(device_index, pRouteInfo, pDboper))
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����Message��Ϣ�������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �ϼ�CMSû�иĵ�λ��Ȩ��:��λ����=%u", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_index);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Message sent to the local superior CMS failure:ID = % s = % s IP address, port number = % d , users do not have permission to change point ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                continue;
            }

            /* ������Ϣ���ϼ�CMS */
            if (NULL == callee_id)
            {
                i |= SIP_SendMessage(NULL, local_cms_id_get(), pRouteInfo->server_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, msg, msg_len);
            }
            else
            {
                i |= SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, msg, msg_len);
            }

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Message��Ϣ�������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Message sent to the local superior CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendMessageToOwnerRouteCMSExceptMMS2() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Message��Ϣ�������ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Message sent to the local superior CMS success: the superior CMS, ID = % s = % s IP address, port number = % d ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendMessageToOwnerRouteCMSExceptMMS2() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
    }

    needProc.clear();

    return 1;
}

/*****************************************************************************
 �� �� ��  : SendNotifyTo3PartyRouteCMS
 ��������  : ����SIP Notify��Ϣ�����������ϼ�ƽ̨�����жϵ�λȨ��
 �������  : char* callee_id
             char* msg
             int msg_len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��5��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendNotifyTo3PartyRouteCMS(char* callee_id, char* msg, int msg_len)
{
    int i = 0;
    int pos = -1;
    //int iRet = 0;
    route_info_t* pRouteInfo = NULL;
    needtoproc_routeinfo_queue needProc;

    if (NULL == msg || msg_len <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendNotifyTo3PartyRouteCMS() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == g_RouteInfoList)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendNotifyTo3PartyRouteCMS() exit---: Route Info List Error \r\n");
        return -1;
    }

    needProc.clear();

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendNotifyTo3PartyRouteCMS() exit---: Route Info List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (pRouteInfo->reg_status == 0 || pRouteInfo->reg_info_index < 0)
        {
            continue;
        }

        if (pRouteInfo->three_party_flag == 0)
        {
            continue;
        }

        if (pRouteInfo->catalog_subscribe_flag == 0) /* û�ж��Ĳ�����֪ͨ��Ϣ */
        {
            continue;
        }

        /* �жϷ�����IP��ַ�Ƿ����¼�CMS�ĵ�ַ��������¼�CMS�ĵ�ַ���򲻷�����Ϣ�� */
        if ('\0' != pRouteInfo->server_ip[0])
        {
            if (pRouteInfo->ip_is_in_sub)
            {
                //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendNotifyTo3PartyRouteCMS() server_ip=%s ignore Send \r\n", pRouteInfo->server_ip);
                continue;
            }
        }

        needProc.push_back(pRouteInfo);
    }

    ROUTE_INFO_SMUTEX_UNLOCK();

    if ((int)needProc.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Notify��Ϣ�����е������ϼ�CMS: �ϼ�CMS��=%d", (int)needProc.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send a message to all the local superior CMS: superior CMS = % d", (int)needProc.size());

    while (!needProc.empty())
    {
        pRouteInfo = (route_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pRouteInfo)
        {
            if (pRouteInfo->catalog_subscribe_dialog_index >= 0)
            {
                i = SIP_SendNotifyWithinDialog(pRouteInfo->catalog_subscribe_dialog_index, msg, msg_len);
            }
            else
            {
                /* ������Ϣ���ϼ�CMS */
                if (NULL == callee_id)
                {
                    i = SIP_SendNotify(NULL, local_cms_id_get(), pRouteInfo->server_id, (char*)"Catalog", pRouteInfo->catalog_subscribe_event_id, pRouteInfo->catalog_subscribe_expires, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, msg, msg_len);
                }
                else
                {
                    i = SIP_SendNotify(NULL, local_cms_id_get(), callee_id, (char*)"Catalog", pRouteInfo->catalog_subscribe_event_id, pRouteInfo->catalog_subscribe_expires, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, msg, msg_len);
                }
            }

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Notify��Ϣ���������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Notify sent to the local superior CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendNotifyTo3PartyRouteCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Notify��Ϣ���������ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Notify sent to the local superior CMS success: the superior CMS, ID = % s = % s IP address, port number = % d ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendNotifyTo3PartyRouteCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
    }

    needProc.clear();

    return 1;
}

/*****************************************************************************
 �� �� ��  : SendNotifyTo3PartyRouteCMS2
 ��������  : ����Ŀ¼�仯֪ͨ��Ϣ��������ƽ̨���жϵ�λȨ��
 �������  : unsigned int device_index
             char* callee_id
             char* msg
             int msg_len
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��12��10��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendNotifyTo3PartyRouteCMS2(unsigned int device_index, char* callee_id, char* msg, int msg_len, DBOper* pDboper)
{
    int i = 0;
    int pos = -1;
    //int iRet = 0;
    route_info_t* pRouteInfo = NULL;
    needtoproc_routeinfo_queue needProc;

    if (device_index <= 0 || NULL == msg || msg_len <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendNotifyTo3PartyRouteCMS2() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == g_RouteInfoList)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendNotifyTo3PartyRouteCMS2() exit---: Route Info List Error \r\n");
        return -1;
    }

    needProc.clear();

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendNotifyTo3PartyRouteCMS2() exit---: Route Info List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (pRouteInfo->reg_status == 0 || pRouteInfo->reg_info_index < 0)
        {
            continue;
        }

        if (pRouteInfo->three_party_flag == 0) /* ���ǵ�����ƽ̨������֪ͨ��Ϣ */
        {
            continue;
        }

        if (pRouteInfo->catalog_subscribe_flag == 0) /* û�ж��Ĳ�����֪ͨ��Ϣ */
        {
            continue;
        }

        /* �жϷ�����IP��ַ�Ƿ����¼�CMS�ĵ�ַ��������¼�CMS�ĵ�ַ���򲻷�����Ϣ�� */
        if ('\0' != pRouteInfo->server_ip[0])
        {
            if (pRouteInfo->ip_is_in_sub)
            {
                //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendNotifyTo3PartyRouteCMS2() server_ip=%s ignore Send \r\n", pRouteInfo->server_ip);
                continue;
            }
        }

        needProc.push_back(pRouteInfo);
    }

    ROUTE_INFO_SMUTEX_UNLOCK();

    if ((int)needProc.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Notify��Ϣ�����е������ϼ�CMS: �ϼ�CMS��=%d", (int)needProc.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send a message to all third-party superior CMS: superior CMS = % d", (int)needProc.size());

    while (!needProc.empty())
    {
        pRouteInfo = (route_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pRouteInfo)
        {
            /* �ж��ϼ�·���Ƿ��иõ�λ��Ȩ�� */
            if (!IsRouteHasPermissionForDevice(device_index, pRouteInfo, pDboper))
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����Notify��Ϣ���������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �ϼ�CMSû�иĵ�λ��Ȩ��:��λ����=%u", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_index);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Message sent to the local superior CMS failure:ID = % s = % s IP address, port number = % d , users do not have permission to change point ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                continue;
            }

            if (pRouteInfo->catalog_subscribe_dialog_index >= 0)
            {
                i = SIP_SendNotifyWithinDialog(pRouteInfo->catalog_subscribe_dialog_index, msg, msg_len);
            }
            else
            {
                /* ������Ϣ���ϼ�CMS */
                if (NULL == callee_id)
                {
                    i = SIP_SendNotify(NULL, local_cms_id_get(), pRouteInfo->server_id, (char*)"Catalog", pRouteInfo->catalog_subscribe_event_id, pRouteInfo->catalog_subscribe_expires, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, msg, msg_len);
                }
                else
                {
                    i = SIP_SendNotify(NULL, local_cms_id_get(), callee_id, (char*)"Catalog", pRouteInfo->catalog_subscribe_event_id, pRouteInfo->catalog_subscribe_expires, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, msg, msg_len);
                }
            }

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Notify��Ϣ���������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Directory changes Notify messages sent to a third party the superior CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendNotifyTo3PartyRouteCMS2() SIP_SendNotify Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Notify��Ϣ���������ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Directory changes Notify messages sent to a third party the superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendNotifyTo3PartyRouteCMS2() SIP_SendNotify OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
    }

    needProc.clear();

    return 1;
}

/*****************************************************************************
 �� �� ��  : SendNotifyCatalogStatusTo3PartyRouteCMS
 ��������  :  ����״̬�仯֪ͨ��Ϣ��������ƽ̨
 �������  : route_info_t* pRouteInfo
             char* device_id
             int status
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��9��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendNotifyCatalogStatusTo3PartyRouteCMS(route_info_t* pRouteInfo, char* device_id, int status)
{
    int i = 0;
    CPacket outPacket2;
    DOMElement* AccNode2 = NULL;
    DOMElement* ItemAccNode2 = NULL;
    DOMElement* ItemInfoNode2 = NULL;

    if (NULL == pRouteInfo || NULL == device_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendNotifyCatalogStatusTo3PartyRouteCMS() exit---: Param Error \r\n");
        return -1;
    }

    if (pRouteInfo->catalog_subscribe_flag == 0) /* û�ж��Ĳ�����֪ͨ��Ϣ */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Ŀ¼״̬�仯Catalog���ϼ�������CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=�ϼ�û�ж���", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        return -1;
    }

    outPacket2.SetRootTag("Notify");
    AccNode2 = outPacket2.CreateElement((char*)"CmdType");
    outPacket2.SetElementValue(AccNode2, (char*)"Catalog");

    AccNode2 = outPacket2.CreateElement((char*)"SN");
    outPacket2.SetElementValue(AccNode2, (char*)"137");

    AccNode2 = outPacket2.CreateElement((char*)"DeviceID");
    outPacket2.SetElementValue(AccNode2, local_cms_id_get());

    AccNode2 = outPacket2.CreateElement((char*)"SumNum");
    outPacket2.SetElementValue(AccNode2, (char*)"1");

    AccNode2 = outPacket2.CreateElement((char*)"DeviceList");
    outPacket2.SetElementAttr(AccNode2, (char*)"Num", (char*)"1");

    outPacket2.SetCurrentElement(AccNode2);
    ItemAccNode2 = outPacket2.CreateElement((char*)"Item");
    outPacket2.SetCurrentElement(ItemAccNode2);

    /* �豸ͳһ��� */
    AccNode2 = outPacket2.CreateElement((char*)"DeviceID");
    outPacket2.SetElementValue(AccNode2, device_id);

    AccNode2 = outPacket2.CreateElement((char*)"Event");

    if (1 == status)
    {
        outPacket2.SetElementValue(AccNode2, (char*)"ON");
    }
    else if (2 == status)
    {
        outPacket2.SetElementValue(AccNode2, (char*)"VLOST");
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, (char*)"OFF");
    }

    if (pRouteInfo->catalog_subscribe_dialog_index >= 0)
    {
        i = SIP_SendNotifyWithinDialog(pRouteInfo->catalog_subscribe_dialog_index, (char*)outPacket2.GetXml(NULL).c_str(), outPacket2.GetXml(NULL).length());
    }
    else
    {
        /* ������Ϣ���ϼ�CMS */
        i = SIP_SendNotify(NULL, local_cms_id_get(), pRouteInfo->server_id, (char*)"Catalog", pRouteInfo->catalog_subscribe_event_id, pRouteInfo->catalog_subscribe_expires, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket2.GetXml(NULL).c_str(), outPacket2.GetXml(NULL).length());
    }

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Ŀ¼״̬�仯Catalog��Ϣ���������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Directory changes Notify messages sent to a third party the superior CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendNotifyCatalogStatusTo3PartyRouteCMS() SIP_SendNotify Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Ŀ¼״̬�仯Catalog��Ϣ���������ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Directory changes Notify messages sent to a third party the superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendNotifyCatalogStatusTo3PartyRouteCMS() SIP_SendNotify OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : SendNotifyCatalogStatusTo3PartyRouteCMSByTCP
 ��������  : ͨ��TCP����״̬�仯֪ͨ��Ϣ��������ƽ̨
 �������  : route_info_t* pRouteInfo
             char* device_id
             int status
             int tcp_socket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��21��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendNotifyCatalogStatusTo3PartyRouteCMSByTCP(route_info_t* pRouteInfo, char* device_id, int status, int tcp_socket)
{
    int i = 0;
    CPacket outPacket2;
    DOMElement* AccNode2 = NULL;
    DOMElement* ItemAccNode2 = NULL;
    DOMElement* ItemInfoNode2 = NULL;

    if (NULL == pRouteInfo || NULL == device_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendNotifyCatalogStatusTo3PartyRouteCMSByTCP() exit---: Param Error \r\n");
        return -1;
    }

    outPacket2.SetRootTag("Notify");
    AccNode2 = outPacket2.CreateElement((char*)"CmdType");
    outPacket2.SetElementValue(AccNode2, (char*)"Catalog");

    AccNode2 = outPacket2.CreateElement((char*)"SN");
    outPacket2.SetElementValue(AccNode2, (char*)"137");

    AccNode2 = outPacket2.CreateElement((char*)"DeviceID");
    outPacket2.SetElementValue(AccNode2, local_cms_id_get());

    AccNode2 = outPacket2.CreateElement((char*)"SumNum");
    outPacket2.SetElementValue(AccNode2, (char*)"1");

    AccNode2 = outPacket2.CreateElement((char*)"DeviceList");
    outPacket2.SetElementAttr(AccNode2, (char*)"Num", (char*)"1");

    outPacket2.SetCurrentElement(AccNode2);
    ItemAccNode2 = outPacket2.CreateElement((char*)"Item");
    outPacket2.SetCurrentElement(ItemAccNode2);

    /* �豸ͳһ��� */
    AccNode2 = outPacket2.CreateElement((char*)"DeviceID");
    outPacket2.SetElementValue(AccNode2, device_id);

    AccNode2 = outPacket2.CreateElement((char*)"Event");

    if (1 == status)
    {
        outPacket2.SetElementValue(AccNode2, (char*)"ON");
    }
    else if (2 == status)
    {
        outPacket2.SetElementValue(AccNode2, (char*)"VLOST");
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, (char*)"OFF");
    }

    /* ������Ϣ���ϼ�CMS */
    i = SIP_SendNotify_By_TCP(NULL, local_cms_id_get(), pRouteInfo->server_id, (char*)"Catalog", pRouteInfo->catalog_subscribe_event_id, pRouteInfo->catalog_subscribe_expires, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket2.GetXml(NULL).c_str(), outPacket2.GetXml(NULL).length(), tcp_socket);

    if (i != 0)
    {
        if (i == -2)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ͨ��TCP����Ŀ¼״̬�仯Catalog��Ϣ���������ϼ�CMSʧ��,TCP�����쳣�ر�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Directory changes Notify messages sent to a third party the superior CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendNotifyCatalogStatusTo3PartyRouteCMSByTCP() SIP_SendNotify_By_TCP Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            return -2;
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ͨ��TCP����Ŀ¼״̬�仯Catalog��Ϣ���������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Directory changes Notify messages sent to a third party the superior CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendNotifyCatalogStatusTo3PartyRouteCMSByTCP() SIP_SendNotify_By_TCP Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͨ��TCP����Ŀ¼״̬�仯Catalog��Ϣ���������ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Directory changes Notify messages sent to a third party the superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendNotifyCatalogStatusTo3PartyRouteCMSByTCP() SIP_SendNotify_By_TCP OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : SendDeviceStatusToRouteCMS
 ��������  : �����豸״̬�������ϼ�CMS
 �������  : unsigned int device_index
             char* device_id
             int status
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendDeviceStatusToRouteCMS(unsigned int device_index, char* device_id, int status, DBOper* pDboper)
{
    int i = 0;
    CPacket outPacket1;
    DOMElement* AccNode1 = NULL;
    string strStatus1 = "";
    CPacket outPacket2;
    DOMElement* AccNode2 = NULL;
    DOMElement* ItemAccNode = NULL;
    string strStatus2 = "";

    if (device_index <= 0 || NULL == device_id)
    {
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

    if (NULL != device_id)
    {
        outPacket1.SetElementValue(AccNode1, device_id);
    }
    else
    {
        outPacket1.SetElementValue(AccNode1, (char*)"");
    }

    AccNode1 = outPacket1.CreateElement((char*)"Status");
    outPacket1.SetElementValue(AccNode1, (char*)strStatus1.c_str());

    i = SendMessageToOwnerRouteCMS2(NULL, (char*)outPacket1.GetXml(NULL).c_str(), outPacket1.GetXml(NULL).length(), pDboper);

    if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�����豸״̬�仯Message��Ϣ�������ϼ�CMSʧ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Sent device status change message  to the local superior CMS failure");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendDeviceStatusToRouteCMS() SendMessageToOwnerRouteCMS2 Error \r\n");
        return -1;
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�����豸״̬�仯Message��Ϣ�������ϼ�CMS�ɹ�");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Sent device status change message  to the local superior CMS success");
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendDeviceStatusToRouteCMS() SendMessageToOwnerRouteCMS2 OK \r\n");
    }

    /* ����Notify��Ϣ��������ƽ̨ */
    if (0 == status)
    {
        strStatus2 = "OFF";
    }
    else if (2 == status)
    {
        strStatus2 = "VLOST";
    }
    else
    {
        strStatus2 = "ON";
    }

    /* �ظ���Ӧ,�齨��Ϣ */
    outPacket2.SetRootTag("Notify");
    AccNode2 = outPacket2.CreateElement((char*)"CmdType");
    outPacket2.SetElementValue(AccNode2, (char*)"Catalog");

    AccNode2 = outPacket2.CreateElement((char*)"SN");
    outPacket2.SetElementValue(AccNode2, (char*)"112");

    AccNode2 = outPacket2.CreateElement((char*)"DeviceID");
    outPacket2.SetElementValue(AccNode2, local_cms_id_get());

    AccNode2 = outPacket2.CreateElement((char*)"SumNum");
    outPacket2.SetElementValue(AccNode2, (char*)"1");

    AccNode2 = outPacket2.CreateElement((char*)"DeviceList");
    outPacket2.SetElementAttr(AccNode2, (char*)"Num", (char*)"1");

    outPacket2.SetCurrentElement(AccNode2);
    ItemAccNode = outPacket2.CreateElement((char*)"Item");
    outPacket2.SetCurrentElement(ItemAccNode);

    AccNode2 = outPacket2.CreateElement((char*)"DeviceID");

    if (NULL != device_id)
    {
        outPacket2.SetElementValue(AccNode2, device_id);
    }
    else
    {
        outPacket2.SetElementValue(AccNode2, (char*)"");
    }

    AccNode2 = outPacket2.CreateElement((char*)"Event");
    outPacket2.SetElementValue(AccNode2, (char*)strStatus2.c_str());

    i = SendNotifyTo3PartyRouteCMS(NULL, (char*)outPacket2.GetXml(NULL).c_str(), outPacket2.GetXml(NULL).length());

    if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�����豸״̬�仯Notify��Ϣ���������ϼ�CMSʧ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Sent device status change message  to the third party  superior CMS failure");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendDeviceStatusToRouteCMS() SendNotifyTo3PartyRouteCMS Error \r\n");
        return -1;
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�����豸״̬�仯Notify��Ϣ���������ϼ�CMS�ɹ�");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Sent device status change message  to the third party  superior CMS success");
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendDeviceStatusToRouteCMS() SendNotifyTo3PartyRouteCMS OK \r\n");
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendAllDeviceStatusToOwnerRouteCMS
 ��������  : �������е�λ״̬�������ϼ�ƽ̨
 �������  : route_info_t* pRouteInfo
             vector<string>& DeviceIDVector
             int status
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendAllDeviceStatusToOwnerRouteCMS(route_info_t* pRouteInfo, vector<string>& DeviceIDVector, int status)
{
    int i = 0;
    int index = 0;
    CPacket outPacket1;
    DOMElement* AccNode1 = NULL;
    string strStatus1 = "";

    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo)
    {
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
    else
    {
        return -1;
    }

    for (index = 0; index < (int)DeviceIDVector.size(); index++)
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

        if (NULL == pGBLogicDeviceInfo)
        {
            continue;
        }

        /* �齨��Ϣ */
        outPacket1.SetRootTag("Notify");
        AccNode1 = outPacket1.CreateElement((char*)"CmdType");
        outPacket1.SetElementValue(AccNode1, (char*)"DeviceStatus");

        AccNode1 = outPacket1.CreateElement((char*)"SN");
        outPacket1.SetElementValue(AccNode1, (char*)"132");

        AccNode1 = outPacket1.CreateElement((char*)"DeviceID");
        outPacket1.SetElementValue(AccNode1, pGBLogicDeviceInfo->device_id);

        AccNode1 = outPacket1.CreateElement((char*)"Status");
        outPacket1.SetElementValue(AccNode1, (char*)strStatus1.c_str());

        /* ������Ϣ���ϼ�CMS */
        i |= SIP_SendMessage(NULL, local_cms_id_get(), pRouteInfo->server_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket1.GetXml(NULL).c_str(), outPacket1.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����״̬�仯Message��Ϣ�������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��λID=%s, ״̬=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����״̬�仯Message��Ϣ�������ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��λID=%s, ״̬=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status);
        }
    }

    return i;
}


/*****************************************************************************
 �� �� ��  : SendAllDeviceStatusTo3PartyRouteCMS
 ��������  : �������е�λ״̬���������ϼ�ƽ̨
 �������  : route_info_t* pRouteInfo
             vector<string>& DeviceIDVector
             int status
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendAllDeviceStatusTo3PartyRouteCMS(route_info_t* pRouteInfo, vector<string>& DeviceIDVector, int status)
{
    int i = -1;
    int iRet = -1;
    int index = -1;
    static int iWaitCount = 0;
    int tcp_socket = -1;

    CPacket outPacket2;
    DOMElement* AccNode2 = NULL;
    DOMElement* ItemAccNode = NULL;

    if (NULL == pRouteInfo)
    {
        return -1;
    }

    /* �Ȳ���TCP���� */
    //tcp_socket = CMS_CreateSIPTCPConnect(pRouteInfo->server_ip, pRouteInfo->server_port);

    if (tcp_socket >= 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�������е�λ״̬���ϼ�������ƽ̨, ����SIP TCP�ɹ�,��ͨ��SIP TCP����Ŀ¼״̬�仯Catalog��Ϣ���������ϼ�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket);

        if (DeviceIDVector.size() > 0)
        {
            for (index = 0; index < (int)DeviceIDVector.size(); index++)
            {
                iRet = SendNotifyCatalogStatusTo3PartyRouteCMSByTCP(pRouteInfo, (char*)DeviceIDVector[index].c_str(), status, tcp_socket);

                if (iRet == -2)
                {
                    break;
                }
            }
        }

        CMS_CloseSIPTCPConnect(tcp_socket);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�������е�λ״̬���ϼ�������ƽ̨, ����SIP TCPʧ��,��ͨ��SIP UDP����Ŀ¼״̬�仯Catalog��Ϣ���������ϼ�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket);

        if (DeviceIDVector.size() > 0)
        {
            for (index = 0; index < (int)DeviceIDVector.size(); index++)
            {
                iRet = SendNotifyCatalogStatusTo3PartyRouteCMS(pRouteInfo, (char*)DeviceIDVector[index].c_str(), status);
            }
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendAllOfflineDeviceStatusTo3PartyRouteCMS
 ��������  : �������е��ߵ�λ״̬Catalog���ϼ�������ƽ̨
 �������  : route_info_t* pRouteInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��9��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendAllOfflineDeviceStatusTo3PartyRouteCMS(route_info_t* pRouteInfo)
{
    int iRet = 0;
    int index = -1;
    static int iWaitCount = 0;
    int tcp_socket = -1;

    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    vector<string> DeviceIDVector;
    vector<int> StatusVector;

    if (NULL == pRouteInfo)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "SendAllOfflineDeviceStatusTo3PartyRouteCMS() exit---: Param Error \r\n");
        return -1;
    }

    /* ��ȡ���е��ߵ�λ��Ϣ */
    DeviceIDVector.clear();
    StatusVector.clear();

    GBLOGICDEVICE_SMUTEX_LOCK();

    if (g_GBLogicDeviceInfoMap.size() <= 0)
    {
        GBLOGICDEVICE_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_GBLogicDeviceInfoMap.begin(); Itr != g_GBLogicDeviceInfoMap.end(); Itr++)
    {
        pGBLogicDeviceInfo = Itr->second;

        if ((NULL == pGBLogicDeviceInfo) || (pGBLogicDeviceInfo->enable <= 0))
        {
            continue;
        }

        if (pGBLogicDeviceInfo->status == 0 || pGBLogicDeviceInfo->status == 3)
        {
            DeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
            StatusVector.push_back(pGBLogicDeviceInfo->status);
        }
        else if (pGBLogicDeviceInfo->status == 2)
        {
            DeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
            StatusVector.push_back(pGBLogicDeviceInfo->status);
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    /* �Ȳ���TCP���� */
    //tcp_socket = CMS_CreateSIPTCPConnect(pRouteInfo->server_ip, pRouteInfo->server_port);

    if (tcp_socket >= 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�������е��ߵ�λ���ϼ�CMS, ����SIP TCP�ɹ�,��ͨ��SIP TCP����Ŀ¼״̬�仯Catalog��Ϣ���������ϼ�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket);

        if (DeviceIDVector.size() > 0)
        {
            for (index = 0; index < (int)DeviceIDVector.size(); index++)
            {
                iRet = SendNotifyCatalogStatusTo3PartyRouteCMSByTCP(pRouteInfo, (char*)DeviceIDVector[index].c_str(), StatusVector[index], tcp_socket);

                if (iRet == -2)
                {
                    break;
                }
            }
        }

        CMS_CloseSIPTCPConnect(tcp_socket);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�������е��ߵ�λ���ϼ�CMS, ͨ��SIP UDP����Ŀ¼״̬�仯Catalog��Ϣ���������ϼ�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket);

        if (DeviceIDVector.size() > 0)
        {
            for (index = 0; index < (int)DeviceIDVector.size(); index++)
            {
                iRet = SendNotifyCatalogStatusTo3PartyRouteCMS(pRouteInfo, (char*)DeviceIDVector[index].c_str(), StatusVector[index]);
            }
        }
    }

    DeviceIDVector.clear();
    StatusVector.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : SendAllDeviceStatusToRouteCMS
 ��������  : �������е�λ״̬���ϼ�ƽ̨
 �������  : vector<string>& DeviceIDVector
             int status
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendAllDeviceStatusToRouteCMS(vector<string>& DeviceIDVector, int status)
{
    int i = 0;
    int pos = -1;
    route_info_t* pRouteInfo = NULL;
    needtoproc_routeinfo_queue needProc1;
    needtoproc_routeinfo_queue needProc2;

    needProc1.clear();
    needProc2.clear();

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendMessageToOwnerRouteCMS() exit---: Route Info List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (pRouteInfo->reg_status == 0 || pRouteInfo->reg_info_index < 0)
        {
            continue;
        }

        /* �жϷ�����IP��ַ�Ƿ����¼�CMS�ĵ�ַ��������¼�CMS�ĵ�ַ���򲻷�����Ϣ�� */
        if ('\0' != pRouteInfo->server_ip[0])
        {
            if (pRouteInfo->ip_is_in_sub)
            {
                continue;
            }
        }

        if (pRouteInfo->three_party_flag == 1) /* ������ƽ̨ */
        {
            if (pRouteInfo->catalog_subscribe_flag == 0) /* û�ж��Ĳ�����֪ͨ��Ϣ */
            {
                continue;
            }

            needProc2.push_back(pRouteInfo);
        }
        else if (pRouteInfo->three_party_flag == 0)
        {
            needProc1.push_back(pRouteInfo);
        }
    }

    ROUTE_INFO_SMUTEX_UNLOCK();

    if ((int)needProc1.size() == 0 && (int)needProc2.size() == 0)
    {
        return 0;
    }

    /* ���͸��Լ���ƽ̨ */
    if ((int)needProc1.size() > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����״̬�仯֪ͨ��Ϣ�����б����ϼ�CMS: �ϼ�CMS��=%d", (int)needProc1.size());

        while (!needProc1.empty())
        {
            pRouteInfo = (route_info_t*) needProc1.front();
            needProc1.pop_front();

            if (NULL != pRouteInfo)
            {
                /* ������Ϣ���ϼ�CMS */
                i = SendAllDeviceStatusToOwnerRouteCMS(pRouteInfo, DeviceIDVector, status);
            }
        }

        needProc1.clear();
    }

    /* ���͸�������ƽ̨ */
    if ((int)needProc2.size() > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����״̬�仯֪ͨ��Ϣ�����е������ϼ�CMS: �ϼ�CMS��=%d", (int)needProc2.size());

        while (!needProc2.empty())
        {
            pRouteInfo = (route_info_t*) needProc2.front();
            needProc2.pop_front();

            if (NULL != pRouteInfo)
            {
                /* ������Ϣ���ϼ�CMS */
                i = SendAllDeviceStatusTo3PartyRouteCMS(pRouteInfo, DeviceIDVector, status);
            }
        }

        needProc2.clear();
    }

    return 1;
}

/*****************************************************************************
 �� �� ��  : SendAllDeviceCatalogToOwnerRouteCMS
 ��������  : �������е�λCatalog�������ϼ�ƽ̨
 �������  : route_info_t* pRouteInfo
             vector<string>& DeviceIDVector
             int iEvent
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendAllDeviceCatalogToOwnerRouteCMS(route_info_t* pRouteInfo, vector<string>& DeviceIDVector, int iEvent, DBOper* pDboper)
{
    int i = 0;
    int index = -1;
    string strEvent = "";

    CPacket outPacket1;
    DOMElement* AccNode1 = NULL;
    DOMElement* ItemAccNode1 = NULL;
    DOMElement* ItemInfoNode1 = NULL;

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

    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo || NULL == pDboper)
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

    for (index = 0; index < (int)DeviceIDVector.size(); index++)
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

        if (NULL == pGBLogicDeviceInfo)
        {
            continue;
        }

        /* �ж��ϼ�·���Ƿ��иõ�λ��Ȩ�� */
        if (!IsRouteHasPermissionForDevice(pGBLogicDeviceInfo->id, pRouteInfo, pDboper))
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����Ŀ¼�仯Message��Ϣ�������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �ϼ�CMSû�иĵ�λ��Ȩ��:��λ����=%u", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBLogicDeviceInfo->id);
            continue;
        }

        /* �齨��Ϣ */
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

        /* ������Ϣ���ϼ�CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), pRouteInfo->server_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket1.GetXml(NULL).c_str(), outPacket1.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Ŀ¼�仯Message��Ϣ�������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��λID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBLogicDeviceInfo->device_id);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Ŀ¼�仯Message��Ϣ�������ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��λID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBLogicDeviceInfo->device_id);
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendAllDeviceCatalogTo3PartyRouteCMS
 ��������  : �������е�λCatalog��������ƽ̨
 �������  : route_info_t* pRouteInfo
             vector<string>& DeviceIDVector
             int iEvent
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendAllDeviceCatalogTo3PartyRouteCMS(route_info_t* pRouteInfo, vector<string>& DeviceIDVector, int iEvent, DBOper* pDboper)
{
    int i = 0;
    int iRet = -1;
    int index = -1;
    int tcp_socket = -1;
    string strEvent = "";

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

    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo || NULL == pDboper)
    {

        return -1;
    }

    if (0 == pRouteInfo->catalog_subscribe_flag)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�������е�λĿ¼�仯Catalog���ϼ�������CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=�ϼ�û�ж���", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
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

    /* �Ȳ���TCP���� */
    //tcp_socket = CMS_CreateSIPTCPConnect(pRouteInfo->server_ip, pRouteInfo->server_port);

    if (tcp_socket >= 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�������е�λCatalog���ϼ�CMS, ����SIP TCP�ɹ�,��ͨ��SIP TCP����Ŀ¼�仯Catalog��Ϣ���������ϼ�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket);

        if (DeviceIDVector.size() > 0)
        {
            for (index = 0; index < (int)DeviceIDVector.size(); index++)
            {
                pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

                if (NULL == pGBLogicDeviceInfo)
                {
                    continue;
                }

                /* �ж��ϼ�·���Ƿ��иõ�λ��Ȩ�� */
                if (!IsRouteHasPermissionForDevice(pGBLogicDeviceInfo->id, pRouteInfo, pDboper))
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ͨ��TCP����Ŀ¼�仯Notify��Ϣ�������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �ϼ�CMSû�иĵ�λ��Ȩ��:��λ����=%u", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBLogicDeviceInfo->id);
                    continue;
                }

                /* �齨��Ϣ */
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

                /* ������Ϣ���ϼ�CMS */
                iRet = SIP_SendNotify_By_TCP(NULL, local_cms_id_get(), pRouteInfo->server_id, (char*)"Catalog", pRouteInfo->catalog_subscribe_event_id, pRouteInfo->catalog_subscribe_expires, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket2.GetXml(NULL).c_str(), outPacket2.GetXml(NULL).length(), tcp_socket);

                if (iRet != 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ͨ��TCP����Ŀ¼�仯Notify��Ϣ���������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��λID=%s, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBLogicDeviceInfo->device_id, tcp_socket);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͨ��TCP����Ŀ¼�仯Notify��Ϣ���������ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��λID=%s, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBLogicDeviceInfo->device_id, tcp_socket);
                }

                if (iRet == -2)
                {
                    break;
                }
            }
        }

        CMS_CloseSIPTCPConnect(tcp_socket);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�������е�λCatalog���ϼ�CMS, ����SIP TCPʧ��,��ͨ��SIP UDP����Ŀ¼״̬�仯Catalog��Ϣ���������ϼ�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket);

        if (DeviceIDVector.size() > 0)
        {
            for (index = 0; index < (int)DeviceIDVector.size(); index++)
            {
                pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)DeviceIDVector[index].c_str());

                if (NULL == pGBLogicDeviceInfo)
                {
                    continue;
                }

                /* �ж��ϼ�·���Ƿ��иõ�λ��Ȩ�� */
                if (!IsRouteHasPermissionForDevice(pGBLogicDeviceInfo->id, pRouteInfo, pDboper))
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����Ŀ¼�仯Message��Ϣ�������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, �ϼ�CMSû�иĵ�λ��Ȩ��:��λ����=%u", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBLogicDeviceInfo->id);
                    continue;
                }

                /* �齨��Ϣ */
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

                if (pRouteInfo->catalog_subscribe_dialog_index >= 0)
                {
                    /* ת����Ϣ���ϼ�CMS */
                    iRet = SIP_SendNotifyWithinDialog(pRouteInfo->catalog_subscribe_dialog_index, (char*)outPacket2.GetXml(NULL).c_str(), outPacket2.GetXml(NULL).length());
                }
                else
                {
                    /* ������Ϣ���ϼ�CMS */
                    iRet = SIP_SendNotify(NULL, local_cms_id_get(), pRouteInfo->server_id, (char*)"Catalog", pRouteInfo->catalog_subscribe_event_id, pRouteInfo->catalog_subscribe_expires, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket2.GetXml(NULL).c_str(), outPacket2.GetXml(NULL).length());
                }


                if (iRet != 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Ŀ¼�仯Notify��Ϣ���������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��λID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBLogicDeviceInfo->device_id);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Ŀ¼�仯Notify��Ϣ���������ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ��λID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBLogicDeviceInfo->device_id);
                }
            }
        }
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendAllDeviceCatalogToRouteCMS
 ��������  : �������е�λCatalog���ϼ�ƽ̨
 �������  : vector<string>& DeviceIDVector
             int iEvent
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendAllDeviceCatalogToRouteCMS(vector<string>& DeviceIDVector, int iEvent, DBOper* pDboper)
{
    int i = 0;
    int pos = -1;
    route_info_t* pRouteInfo = NULL;
    needtoproc_routeinfo_queue needProc1;
    needtoproc_routeinfo_queue needProc2;

    needProc1.clear();
    needProc2.clear();

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendAllDeviceCatalogToRouteCMS() exit---: Route Info List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (pRouteInfo->reg_status == 0 || pRouteInfo->reg_info_index < 0)
        {
            continue;
        }

        /* �жϷ�����IP��ַ�Ƿ����¼�CMS�ĵ�ַ��������¼�CMS�ĵ�ַ���򲻷�����Ϣ�� */
        if ('\0' != pRouteInfo->server_ip[0])
        {
            if (pRouteInfo->ip_is_in_sub)
            {
                continue;
            }
        }

        if (pRouteInfo->three_party_flag == 1) /* ������ƽ̨ */
        {
            if (pRouteInfo->catalog_subscribe_flag == 0) /* û�ж��Ĳ�����֪ͨ��Ϣ */
            {
                continue;
            }

            needProc2.push_back(pRouteInfo);
        }
        else if (pRouteInfo->three_party_flag == 0)
        {
            needProc1.push_back(pRouteInfo);
        }
    }

    ROUTE_INFO_SMUTEX_UNLOCK();

    if ((int)needProc1.size() == 0 && (int)needProc2.size() == 0)
    {
        return 0;
    }

    /* ���͸��Լ���ƽ̨ */
    if ((int)needProc1.size() > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Ŀ¼�仯֪ͨ��Ϣ�����б����ϼ�CMS: �ϼ�CMS��=%d", (int)needProc1.size());

        while (!needProc1.empty())
        {
            pRouteInfo = (route_info_t*) needProc1.front();
            needProc1.pop_front();

            if (NULL != pRouteInfo)
            {
                /* ������Ϣ���ϼ�CMS */
                i = SendAllDeviceCatalogToOwnerRouteCMS(pRouteInfo, DeviceIDVector, iEvent, pDboper);
            }
        }

        needProc1.clear();
    }

    /* ���͸�������ƽ̨ */
    if ((int)needProc2.size() > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Ŀ¼�仯֪ͨ��Ϣ�����е������ϼ�CMS: �ϼ�CMS��=%d", (int)needProc2.size());

        while (!needProc2.empty())
        {
            pRouteInfo = (route_info_t*) needProc2.front();
            needProc2.pop_front();

            if (NULL != pRouteInfo)
            {
                /* ������Ϣ���ϼ�CMS */
                i = SendAllDeviceCatalogTo3PartyRouteCMS(pRouteInfo, DeviceIDVector, iEvent, pDboper);
            }
        }

        needProc2.clear();
    }

    return 1;
}

/*****************************************************************************
 �� �� ��  : SendNotifyTopologyPhyDeviceToRouteCMS
 ��������  : ���������豸�仯��Ϣ�������ϼ�CMS
 �������  : int event
             char* strDeviceID
             char* strDeviceName
             char* strDeviceType
             char* strDeviceIP
             char* strStatus
             char* strCMSID
             char* strLinkType
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��28�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendNotifyTopologyPhyDeviceToRouteCMS(int event, char* strDeviceID, char* strDeviceName, char* strDeviceType, char* strDeviceIP, char* strStatus, char* strCMSID, char* strLinkType)
{
    int i = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;

    /* �ظ���Ӧ,�齨��Ϣ */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"TopologyPhyDeviceStatus");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"6767");

    /* �¼� */
    AccNode = outPacket.CreateElement((char*)"Event");

    if (1 == event)
    {
        outPacket.SetElementValue(AccNode, (char*)"ADD");
    }
    else if (2 == event)
    {
        outPacket.SetElementValue(AccNode, (char*)"DEL");
    }
    else if (3 == event)
    {
        outPacket.SetElementValue(AccNode, (char*)"MOD");
    }
    else if (4 == event)
    {
        outPacket.SetElementValue(AccNode, (char*)"STATUS");
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    /* �豸ID */
    AccNode = outPacket.CreateElement((char*)"DeviceID");

    if (NULL != strDeviceID)
    {
        outPacket.SetElementValue(AccNode, strDeviceID);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    /* �豸���� */
    AccNode = outPacket.CreateElement((char*)"DeviceName");

    if (NULL != strDeviceName)
    {
        outPacket.SetElementValue(AccNode, strDeviceName);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    /* �豸���� */
    AccNode = outPacket.CreateElement((char*)"DeviceType");

    if (NULL != strDeviceType)
    {
        outPacket.SetElementValue(AccNode, strDeviceType);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    /* �豸IP */
    AccNode = outPacket.CreateElement((char*)"DeviceIP");

    if (NULL != strDeviceIP)
    {
        outPacket.SetElementValue(AccNode, strDeviceIP);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    /* �豸״̬ */
    AccNode = outPacket.CreateElement((char*)"Status");

    if (NULL != strStatus)
    {
        outPacket.SetElementValue(AccNode, strStatus);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    /* ���� CMS ��� */
    AccNode = outPacket.CreateElement((char*)"CMSID");

    if (NULL != strCMSID)
    {
        outPacket.SetElementValue(AccNode, strCMSID);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    /* �Ƿ�ͬ�� */
    AccNode = outPacket.CreateElement((char*)"LinkType");

    if (NULL != strLinkType)
    {
        outPacket.SetElementValue(AccNode, strLinkType);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    i = SendMessageToOwnerRouteCMSExceptMMS(NULL, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���������豸�仯Message��Ϣ�������ϼ�CMSʧ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send topological equipment changes Message to the local superior CMS failure");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendNotifyTopologyPhyDeviceToRouteCMS() SendMessageToOwnerRouteCMSExceptMMS Error \r\n");
        return -1;
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���������豸�仯Message��Ϣ�������ϼ�CMS�ɹ�");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send topological equipment changes Message to the local superior CMS success");
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendNotifyTopologyPhyDeviceToRouteCMS() SendMessageToOwnerRouteCMSExceptMMS OK \r\n");
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendNotifyGroupCatalogTo3PartyRouteCMS
 ��������  : ���ͷ���仯��Catalog��Ϣ���ϼ�������CMS
 �������  : char* group_id
             char* group_name
             char* parent_id
             int iEvent
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��9��2��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendNotifyGroupCatalogTo3PartyRouteCMS(char* group_id, char* group_name, char* parent_id, int iEvent)
{
    int i = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    string strEvent = "";
    DOMElement* ItemAccNode = NULL;

    if (NULL == group_id)
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

    /* �齨��Ϣ */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"Catalog");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"134");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, local_cms_id_get());

    AccNode = outPacket.CreateElement((char*)"SumNum");
    outPacket.SetElementValue(AccNode, (char*)"1");

    AccNode = outPacket.CreateElement((char*)"DeviceList");
    outPacket.SetElementAttr(AccNode, (char*)"Num", (char*)"1");

    outPacket.SetCurrentElement(AccNode);
    ItemAccNode = outPacket.CreateElement((char*)"Item");
    outPacket.SetCurrentElement(ItemAccNode);

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, group_id);

    if (NULL != group_name && group_name[0] != '\0')
    {
        AccNode = outPacket.CreateElement((char*)"Name");
        outPacket.SetElementValue(AccNode, group_name);
    }

    if (NULL != parent_id && parent_id[0] != '\0')
    {
        AccNode = outPacket.CreateElement((char*)"ParentID");
        outPacket.SetElementValue(AccNode, parent_id);
    }

    AccNode = outPacket.CreateElement((char*)"Event");
    outPacket.SetElementValue(AccNode, (char*)strEvent.c_str());

    i = SendNotifyTo3PartyRouteCMS(NULL, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�����߼���λ����仯Message��Ϣ���������ϼ�CMSʧ��");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendNotifyGroupCatalogTo3PartyRouteCMS() SendNotifyTo3PartyRouteCMS Error \r\n");
        return -1;
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�����߼���λ����仯Message��Ϣ���������ϼ�CMS�ɹ�");
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendNotifyGroupCatalogTo3PartyRouteCMS() SendNotifyTo3PartyRouteCMS OK \r\n");
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendNotifyGroupCatalogToOwnerRouteCMS
 ��������  : ���ͷ���仯��Catalog��Ϣ���Լ����ϼ�CMS
 �������  : char* group_id
             char* group_name
             char* parent_id
             int sort_id
             int iEvent
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��9��2��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendNotifyGroupCatalogToOwnerRouteCMS(char* group_id, char* group_name, char* parent_id, int sort_id, int iEvent)
{
    int i = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    string strEvent = "";
    DOMElement* ItemAccNode = NULL;
    char strSortID[16] = {0};

    if (NULL == group_id)
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

    /* �齨��Ϣ */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"LogicDeviceGroupConfig");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"234");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, local_cms_id_get());

    AccNode = outPacket.CreateElement((char*)"SumNum");
    outPacket.SetElementValue(AccNode, (char*)"1");

    AccNode = outPacket.CreateElement((char*)"DeviceList");
    outPacket.SetElementAttr(AccNode, (char*)"Num", (char*)"1");

    outPacket.SetCurrentElement(AccNode);
    ItemAccNode = outPacket.CreateElement((char*)"Item");
    outPacket.SetCurrentElement(ItemAccNode);

    AccNode = outPacket.CreateElement((char*)"GroupID");
    outPacket.SetElementValue(AccNode, group_id);

    if (NULL != group_name && group_name[0] != '\0')
    {
        AccNode = outPacket.CreateElement((char*)"Name");
        outPacket.SetElementValue(AccNode, group_name);
    }

    if (NULL != parent_id && parent_id[0] != '\0')
    {
        AccNode = outPacket.CreateElement((char*)"ParentID");
        outPacket.SetElementValue(AccNode, parent_id);
    }

    AccNode = outPacket.CreateElement((char*)"SortID");
    snprintf(strSortID, 16, "%d", sort_id);
    outPacket.SetElementValue(AccNode, strSortID);

    AccNode = outPacket.CreateElement((char*)"Event");
    outPacket.SetElementValue(AccNode, (char*)strEvent.c_str());

    i = SendMessageToOwnerRouteCMSExceptMMS(NULL, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�����߼���λ����仯Message��Ϣ�������ϼ�CMSʧ��");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendNotifyGroupCatalogToOwnerRouteCMS() SendMessageToOwnerRouteCMSExceptMMS Error \r\n");
        return -1;
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�����߼���λ����仯Message��Ϣ�������ϼ�CMS�ɹ�");
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendNotifyGroupCatalogToOwnerRouteCMS() SendMessageToOwnerRouteCMSExceptMMS OK \r\n");
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendNotifyGroupMapToOwnerRouteCMS
 ��������  : �����߼��豸�����ϵ�仯���ϼ�ƽ̨
 �������  : char* group_id
             unsigned int device_index
             int sort_id
             int iEvent
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendNotifyGroupMapToOwnerRouteCMS(char* group_id, unsigned int device_index, int sort_id, int iEvent)
{
    int i = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    string strEvent = "";
    DOMElement* ItemAccNode = NULL;
    char strSortID[16] = {0};
    char strDeviceIndex[32] = {0};

    if (NULL == group_id)
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

    /* �齨��Ϣ */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"LogicDeviceMapGroupConfig");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"334");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, local_cms_id_get());

    AccNode = outPacket.CreateElement((char*)"SumNum");
    outPacket.SetElementValue(AccNode, (char*)"1");

    AccNode = outPacket.CreateElement((char*)"DeviceList");
    outPacket.SetElementAttr(AccNode, (char*)"Num", (char*)"1");

    outPacket.SetCurrentElement(AccNode);
    ItemAccNode = outPacket.CreateElement((char*)"Item");
    outPacket.SetCurrentElement(ItemAccNode);

    AccNode = outPacket.CreateElement((char*)"GroupID");
    outPacket.SetElementValue(AccNode, group_id);

    AccNode = outPacket.CreateElement((char*)"DeviceIndex");
    snprintf(strDeviceIndex, 16, "%u", device_index);
    outPacket.SetElementValue(AccNode, strDeviceIndex);

    AccNode = outPacket.CreateElement((char*)"SortID");
    snprintf(strSortID, 16, "%d", sort_id);
    outPacket.SetElementValue(AccNode, strSortID);

    AccNode = outPacket.CreateElement((char*)"Event");
    outPacket.SetElementValue(AccNode, (char*)strEvent.c_str());

    i = SendMessageToOwnerRouteCMSExceptMMS(NULL, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�����߼���λ�����ϵӳ��仯Message��Ϣ�������ϼ�CMSʧ��");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendNotifyGroupMapToOwnerRouteCMS() SendMessageToOwnerRouteCMSExceptMMS Error \r\n");
        return -1;
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�����߼���λ�����ϵӳ��仯Message��Ϣ�������ϼ�CMS�ɹ�");
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendNotifyGroupMapToOwnerRouteCMS() SendMessageToOwnerRouteCMSExceptMMS OK \r\n");
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendKeepAliveMessageToRouteCMS
 ��������  : ���ͱ�����Ϣ���ϼ�����CMS
 �������  : route_info_t* pRouteInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��21�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendKeepAliveMessageToRouteCMS(route_info_t* pRouteInfo)
{
    int i = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    char strSN[128] = {0};

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendKeepAliveMessageToRouteCMS() exit---: Param Error \r\n");
        return -1;
    }

    if (pRouteInfo->reg_status == 0 || pRouteInfo->reg_info_index < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendKeepAliveMessageToRouteCMS() exit---: Route CMS Not Register \r\n");
        return -1;
    }

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"Keepalive");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"1");

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, local_cms_id_get());

    AccNode = outPacket.CreateElement((char*)"Status");
    outPacket.SetElementValue(AccNode, (char*)"OK");

    pRouteInfo->keep_alive_sn++;
    snprintf(strSN, 128, "%u", pRouteInfo->keep_alive_sn);

    /* ������Ϣ���ϼ�CMS */
    i = SIP_SendMessage(strSN, local_cms_id_get(), pRouteInfo->server_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���ͱ�����ϢMessage��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send a keep alive messages to superiors CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendKeepAliveMessageToRouteCMS() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ͱ�����ϢMessage��Ϣ���ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send a keep alive messages to superiors CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendKeepAliveMessageToRouteCMS() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendGetServerIDMessageToRouteCMS
 ��������  : ���ͻ�ȡ�ϼ�CMS ID������
 �������  : route_info_t* pRouteInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��6��13��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendGetServerIDMessageToRouteCMS(route_info_t* pRouteInfo)
{
    int i = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    char* local_ip = NULL;
    int local_port = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendGetServerIDMessageToRouteCMS() exit---: Param Error \r\n");
        return -1;
    }

    /* ��ȡ����ip��ַ�Ͷ˿ں� */
    local_ip = local_ip_get(pRouteInfo->strRegLocalEthName);

    if (NULL == local_ip)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendGetServerIDMessageToRouteCMS(): Find Local Addr Error: strRegLocalEthName=%s, Get By Default EthName \r\n", pRouteInfo->strRegLocalEthName);

        /* ��Ĭ�ϵ������ٻ�ȡһ�� */
        local_ip = local_ip_get(default_eth_name_get());

        if (NULL == local_ip)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendGetServerIDMessageToRouteCMS() exit---: Find Local Addr Error: strRegLocalEthName=%s \r\n", pRouteInfo->strRegLocalEthName);
            return -1;
        }
    }

    local_port = local_port_get(pRouteInfo->strRegLocalEthName);

    if (local_port <= 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendGetServerIDMessageToRouteCMS(): Get Local Port Error : strRegLocalEthName=%s, Get By Default EthName \r\n", pRouteInfo->strRegLocalEthName);

        /* ��Ĭ�ϵ������ٻ�ȡһ�� */
        local_port = local_port_get(default_eth_name_get());

        if (local_port <= 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendGetServerIDMessageToRouteCMS() exit---: Get Local Port Error : strRegLocalEthName=%s \r\n", pRouteInfo->strRegLocalEthName);
            return -1;
        }
    }

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Query");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"GetServerID");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"134");

    AccNode = outPacket.CreateElement((char*)"ServerIP");
    outPacket.SetElementValue(AccNode, pRouteInfo->server_ip);

    /* ������Ϣ���ϼ�CMS */
    i = SIP_SendMessage(NULL, (char*)"wiscomCallerID", (char*)"wiscomCalleeID", local_ip, local_port, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���ͻ�ȡ�ϼ�CMS ID Message��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Sent to obtain the superior CMS ID message to superiors CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendKeepAliveMessageToRouteCMS() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ͻ�ȡ�ϼ�CMS ID ��Ϣ���ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "To obtain the superior CMS ID messages sent to the superior CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendKeepAliveMessageToRouteCMS() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendDeviceInfoMessageToRouteCMS
 ��������  : �����豸��Ϣ��Ϣ���ϼ�����CMS
 �������  : char* caller_id
             char* callee_id
             char* local_ip
             int local_port
             char* remote_ip
             int remote_port
             char* strSN
             char* device_id
             int three_party_flag
             int trans_protocol
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��21�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendDeviceInfoMessageToRouteCMS(char* callee_id, char* local_ip, int local_port, char* remote_ip, int remote_port, char* strSN, char* device_id, int three_party_flag, int trans_protocol)
{
    int i = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    char strCMSVer[256] = {0};

    if (NULL == callee_id || NULL == local_ip || NULL == remote_ip || NULL == strSN || NULL == device_id || local_port <= 0 || remote_port <= 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendDeviceInfoMessageToRouteCMS() exit---: Param Error \r\n");
        return -1;
    }

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Response");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"DeviceInfo");

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

    AccNode = outPacket.CreateElement((char*)"DeviceType");
    outPacket.SetElementValue(AccNode, (char*)"Wiscom CMS");

    AccNode = outPacket.CreateElement((char*)"Manufacturer");
    outPacket.SetElementValue(AccNode, (char*)"WISCOM");

    AccNode = outPacket.CreateElement((char*)"Model");
    outPacket.SetElementValue(AccNode, (char*)"iEV9000RX");

    AccNode = outPacket.CreateElement((char*)"Firmware");
    //GetAppVerInfoForRoute(strCMSVer);
    outPacket.SetElementValue(AccNode, strCMSVer);

    AccNode = outPacket.CreateElement((char*)"MaxCamera");
    outPacket.SetElementValue(AccNode, (char*)"5000");

    AccNode = outPacket.CreateElement((char*)"MaxAlarm");
    outPacket.SetElementValue(AccNode, (char*)"5000");

    if (0 == three_party_flag) /* ������ǶԽӵ�����ƽ̨���ż������б�ʶ */
    {
        AccNode = outPacket.CreateElement((char*)"OwnerFlag");
        outPacket.SetElementValue(AccNode, (char*)"WiscomV");
    }

    if (1 == trans_protocol) /* �����TCPЭ�飬�ż��ϴ������� */
    {
        AccNode = outPacket.CreateElement((char*)"TransProtocol");
        outPacket.SetElementValue(AccNode, (char*)"TCP");
    }

    /* ������Ϣ���ϼ�CMS */
    i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, local_ip, local_port, remote_ip, remote_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���ͱ����豸��ϢMessage��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", callee_id, remote_ip, remote_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Sends the corresponding equipment information message to superiors CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", callee_id, remote_ip, remote_port);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendKeepAliveMessageToRouteCMS() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", callee_id, remote_ip, remote_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ͱ����豸��ϢMessage��Ϣ���ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", callee_id, remote_ip, remote_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Sends the corresponding equipment information Message Message to superiors CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d", callee_id, remote_ip, remote_port);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendKeepAliveMessageToRouteCMS() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", callee_id, remote_ip, remote_port);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : SendUnRegisterToAllRouteCMS
 ��������  : ����ȥע��������ϼ�CMS
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendUnRegisterToAllRouteCMS()
{
    int i = 0;
    int pos = -1;
    route_info_t* pRouteInfo = NULL;
    needtoproc_routeinfo_queue needProc;

    if (NULL == g_RouteInfoList)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendUnRegisterToAllRouteCMS() exit---: Route Info List Error \r\n");
        return -1;
    }

    needProc.clear();

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendUnRegisterToAllRouteCMS() exit---: Route Info List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (pRouteInfo->reg_status == 0 || pRouteInfo->reg_info_index < 0)
        {
            continue;
        }

        needProc.push_back(pRouteInfo); /* ����ȥע�� */
    }

    ROUTE_INFO_SMUTEX_UNLOCK();

    if ((int)needProc.size() == 0)
    {
        return 0;
    }

    /* ������Ҫ����ע����Ϣ�� */
    while (!needProc.empty())
    {
        pRouteInfo = (route_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pRouteInfo)
        {
            /* ����ȥע�� */
            i = SIP_SendUnRegister(pRouteInfo->reg_info_index);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendUnRegisterToAllRouteCMS() SIP_SendUnRegister Error:reg_info_index=%d, iRet=%d \r\n", pRouteInfo->reg_info_index, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendUnRegisterToAllRouteCMS() SIP_SendUnRegister OK:reg_info_index=%d, iRet=%d \r\n", pRouteInfo->reg_info_index, i);
            }
        }
    }

    needProc.clear();

    return 1;
}

/*****************************************************************************
 �� �� ��  : SendNotifyRestartMessageToAllRoutCMS
 ��������  : ��������������ϼ�CMS
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��7��6�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendNotifyRestartMessageToAllRoutCMS()
{
    int i = 0;
    int pos = -1;
    route_info_t* pRouteInfo = NULL;
    needtoproc_routeinfo_queue needProc;

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    char* local_ip = NULL;
    int local_port = 0;

    if (NULL == g_RouteInfoList)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendNotifyRestartMessageToAllRoutCMS() exit---: Route Info List Error \r\n");
        return -1;
    }

    needProc.clear();

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendNotifyRestartMessageToAllRoutCMS() exit---: Route Info List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        needProc.push_back(pRouteInfo); /* ����������Ϣ */
    }

    ROUTE_INFO_SMUTEX_UNLOCK();

    /* �齨XML��Ϣ */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"CMSRestart");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"23456");

    AccNode = outPacket.CreateElement((char*)"CMSID");
    outPacket.SetElementValue(AccNode, local_cms_id_get());

    if ((int)needProc.size() == 0)
    {
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����֪ͨ������Ϣ�������ϼ�CMS: �ϼ�CMS��=%d", (int)needProc.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send restarting  notifications message to all the superior CMS: superior CMS = % d", (int)needProc.size());

    while (!needProc.empty())
    {
        pRouteInfo = (route_info_t*) needProc.front();
        needProc.pop_front();

        if (NULL != pRouteInfo)
        {
            /* ��ȡ����ip��ַ�Ͷ˿ں� */
            local_ip = local_ip_get(pRouteInfo->strRegLocalEthName);

            if (NULL == local_ip)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendNotifyRestartMessageToAllRoutCMS(): Find Local Addr Error: strRegLocalEthName=%s, Get By Default EthName \r\n", pRouteInfo->strRegLocalEthName);

                /* ��Ĭ�ϵ������ٻ�ȡһ�� */
                local_ip = local_ip_get(default_eth_name_get());

                if (NULL == local_ip)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendNotifyRestartMessageToAllRoutCMS() exit---: Find Local Addr Error: strRegLocalEthName=%s \r\n", pRouteInfo->strRegLocalEthName);
                    continue;
                }
            }

            local_port = local_port_get(pRouteInfo->strRegLocalEthName);

            if (local_port <= 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "SendNotifyRestartMessageToAllRoutCMS(): Get Local Port Error : strRegLocalEthName=%s, Get By Default EthName \r\n", pRouteInfo->strRegLocalEthName);

                /* ��Ĭ�ϵ������ٻ�ȡһ�� */
                local_port = local_port_get(default_eth_name_get());

                if (local_port <= 0)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendNotifyRestartMessageToAllRoutCMS() exit---: Get Local Port Error : strRegLocalEthName=%s \r\n", pRouteInfo->strRegLocalEthName);
                    continue;
                }
            }

            /* ������Ϣ */
            i |= SIP_SendMessage(NULL, local_cms_id_get(), pRouteInfo->server_id, local_ip, local_port, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����֪ͨ����Message��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendNotifyRestartMessageToAllRoutCMS Error:Superior CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SendNotifyRestartMessageToAllRoutCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����֪ͨ����Message��Ϣ���ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyRestartMessageToAllRoutCMS Ok:Superior CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SendNotifyRestartMessageToAllRoutCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
    }

    needProc.clear();

    return 1;
}

/*****************************************************************************
 �� �� ��  : SendNotifyCatalogMessageToRouteCMS
 ��������  : ����Ŀ¼֪ͨ�����ϼ�ƽ̨
 �������  : char* callee_id
             char* msg
             int msg_len
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��6��10�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SendNotifyCatalogMessageToRouteCMS(unsigned int route_index)
{
    int i = 0;
    int pos = -1;
    route_info_t* pRouteInfo = NULL;

    if (route_index <= 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendNotifyCatalogMessageToRouteCMS() exit---: Param Error \r\n");
        return -1;
    }

    pos = route_info_find_by_route_index(route_index);

    if (pos < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendNotifyCatalogMessageToRouteCMS() exit---: route_info_find_by_route_index Error:route_index=%d \r\n", route_index);
        return -1;
    }

    pRouteInfo = route_info_get(pos);

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendNotifyCatalogMessageToRouteCMS() exit---: route_info_get Error:pos=%d \r\n", pos);
        return -1;
    }

    if (pRouteInfo->reg_status <= 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SendNotifyCatalogMessageToRouteCMS() exit---: reg_status Error:reg_status=%d \r\n", pRouteInfo->reg_status);
        return -1;
    }

    i = RouteGetGBDeviceListAndSendNotifyCatalog(pRouteInfo, &g_DBOper);

    return i;
}

/*****************************************************************************
 �� �� ��  : RouteGetGBDeviceListAndSendNotifyCatalog
 ��������  : ����֪ͨĿ¼��Ϣ
 �������  : route_info_t* pRouteInfo
             char* caller_id
             char* strDeviceID
             char* strSN
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��10�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int RouteGetGBDeviceListAndSendNotifyCatalog(route_info_t* pRouteInfo, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    static int iWaitCount = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendNotifyCatalog() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendNotifyCatalog() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendNotifyCatalog() Enter---: caller_ip=%s, caller_port=%d \r\n",  pRouteInfo->server_ip, pRouteInfo->server_port);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�����߼���λĿ¼֪ͨ��Ϣ���ϼ�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Logic level directory notice sending a message to the superior CMS: superior CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    if (1 == pRouteInfo->catlog_get_status)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�����߼���λĿ¼֪ͨ��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=�ϼ�CMS���ڻ�ȡCatlog", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        return 0;
    }
    else
    {
        /* ������ݿ��Ƿ����ڸ��� */
        pRouteInfo->catlog_get_status = 1;

        while (checkIfHasDBRefresh() && iWaitCount < 12)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�����߼���λĿ¼֪ͨ��Ϣ���ϼ�CMS,���ݿ����ڸ���,�ȴ����ݿ�������:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            osip_usleep(5000000);

            iWaitCount++;
        }

        if (iWaitCount >= 12)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�����߼���λĿ¼֪ͨ��Ϣ���ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ԭ��=�ȴ��������ݿ���ɳ�ʱ,���Ժ��ٻ�ȡ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            iWaitCount = 0;
            pRouteInfo->catlog_get_status = 0;
            return 0;
        }

        iWaitCount = 0;

        if (1 == pRouteInfo->three_party_flag) /* ��Ҫ�����������������Ϣ */
        {
            i = RouteGetGBDeviceListAndSendNotifyCatalogTo3PartyCMS(pRouteInfo, pRoute_Srv_dboper);
        }
        else
        {
            i = RouteGetGBDeviceListAndSendNotifyCatalogToOwnerCMS(pRouteInfo, pRoute_Srv_dboper);
        }

        pRouteInfo->catlog_get_status = 0;
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : RouteGetGBDeviceListAndSendNotifyCatalogToOwnerCMS
 ��������  : ����֪ͨĿ¼��Ϣ���ϼ�CMS
 �������  : route_info_t* pRouteInfo
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��9��6��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int RouteGetGBDeviceListAndSendNotifyCatalogToOwnerCMS(route_info_t* pRouteInfo, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int index = 0;
    int record_count = 0; /* ��¼�� */
    int send_count = 0;   /* ���͵Ĵ��� */
    int query_count = 0;  /* ��ѯ����ͳ�� */
    DOMElement* ListAccNode = NULL;

    string strSQL = "";
    vector<string> DeviceIDVector;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendNotifyCatalogToOwnerCMS() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendNotifyCatalogToOwnerCMS() exit---: Param Error \r\n");
        return -1;
    }

    /* �ϼ�cms��ѯ���豸�б��Ǳ���cms�������豸�б�
     */
    DeviceIDVector.clear();

    /* ������е��߼��豸 */
    i = AddAllGBLogicDeviceIDToVectorForRoute(DeviceIDVector, pRouteInfo->id, pRouteInfo->three_party_flag, pRouteInfo->link_type, pRoute_Srv_dboper);

    /* 4����ȡ�����е��豸���� */
    record_count = DeviceIDVector.size();

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "RouteGetGBDeviceListAndSendNotifyCatalogToOwnerCMS() record_count=%d \r\n", record_count);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�����߼��豸Ŀ¼֪ͨ��Ϣ�������ϼ�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ���͵ķ�����Ϣ����=%d\r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send catalog message to route cms:route CMS ID=%s, route CMS IP=%s, route CMS Port=%d, group record count=%d\r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);

    /* 5�������¼��Ϊ0 */
    if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "RouteGetGBDeviceListAndSendNotifyCatalogToOwnerCMS() exit---: No Record Count \r\n");
        return i;
    }

    /* 6��ѭ��������������ȡ�û����豸��Ϣ������xml�� */
    CPacket* pOutPacket = NULL;

    for (index = 0; index < record_count; index++)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "RouteGetGBDeviceListAndSendCataLogToCMS() DeviceIndex=%u \r\n", device_index);

        /* �����¼������4����Ҫ�ִη��� */
        query_count++;

        /* ����XMLͷ�� */
        i = CreateGBLogicDeviceCatalogNotifyXMLHeadForRoute(&pOutPacket, query_count, record_count, (char*)"3456", local_cms_id_get(), &ListAccNode);

        /* ����Item ֵ */
        i = AddLogicDeviceInfoToXMLItemForRouteNotify(pOutPacket, ListAccNode, (char*)DeviceIDVector[index].c_str(), pRouteInfo->three_party_flag, pRoute_Srv_dboper);

        if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 0) || (query_count == record_count))
        {
            if (NULL != pOutPacket)
            {
                send_count++;

                if (pRouteInfo->catalog_subscribe_dialog_index >= 0)
                {
                    /* ת����Ϣ���ϼ�CMS */
                    i = SIP_SendNotifyWithinDialog(pRouteInfo->catalog_subscribe_dialog_index, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());
                }
                else
                {
                    /* ת����Ϣ���ϼ�CMS */
                    i = SIP_SendNotify(NULL, local_cms_id_get(), pRouteInfo->server_id, (char*)"Catalog", 234, 3600, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());
                }

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Ŀ¼�仯Notify��Ϣ�������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Directory changes Notify messages sent to the local superior CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendNotifyCatalogToOwnerCMS() SIP_SendNotify Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Ŀ¼�仯Notify��Ϣ�������ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Directory changes Notify messages sent to the local superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendNotifyCatalogToOwnerCMS() SIP_SendNotify OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }

                delete pOutPacket;
                pOutPacket = NULL;
            }
        }
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendNotifyCatalogToOwnerCMS Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 �� �� ��  : RouteGetGBDeviceListAndSendNotifyCatalogTo3PartyCMS
 ��������  : ����֪ͨĿ¼��Ϣ��������ƽ̨
 �������  : route_info_t* pRouteInfo
             DBOper* pRoute_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��9��6��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int RouteGetGBDeviceListAndSendNotifyCatalogTo3PartyCMS(route_info_t* pRouteInfo, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int index = 0;
    int group_record_count = 0; /* ��¼�� */
    int device_record_count = 0; /* ��¼�� */
    int record_count = 0; /* ��¼�� */
    int send_count = 0;   /* ���͵Ĵ��� */
    int query_count = 0;  /* ��ѯ����ͳ�� */
    DOMElement* ListAccNode = NULL;
    primary_group_t* pPrimaryGroup = NULL;
    int tcp_socket = -1;

    string strSQL = "";
    vector<string> GroupIDVector;
    vector<string> DeviceIDVector;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendNotifyCatalogTo3PartyCMS() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendNotifyCatalogTo3PartyCMS() exit---: Param Error \r\n");
        return -1;
    }

    /* ��ȡ����������������б� */
    GroupIDVector.clear();

    i = AddGblLogicDeviceGroupToVectorForRoute(GroupIDVector);

    /* ��ȡ�����е��豸���� */
    group_record_count = GroupIDVector.size();

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "RouteGetGBDeviceListAndSendNotifyCatalogTo3PartyCMS() group_record_count=%d \r\n", group_record_count);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�����߼��豸Ŀ¼֪ͨ��Ϣ���������ϼ�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ���͵ķ�����Ϣ����=%d\r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send catalog message to 3 party route cms:route CMS ID=%s, route CMS IP=%s, route CMS Port=%d, group record count=%d\r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);

    /* ��ȡ�߼��豸�б� */
    DeviceIDVector.clear();

    i = AddAllGBLogicDeviceIDToVectorForRoute(DeviceIDVector, pRouteInfo->id, pRouteInfo->three_party_flag, pRouteInfo->link_type, pRoute_Srv_dboper);

    /* ��ȡ�����е��豸���� */
    device_record_count = DeviceIDVector.size();

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "RouteGetGBDeviceListAndSendNotifyCatalogTo3PartyCMS() device_record_count=%d \r\n", device_record_count);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�����߼��豸Ŀ¼֪ͨ��Ϣ���������ϼ�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ���͵ĵ�λ��Ϣ����=%d\r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send catalog message to 3 party route cms:route CMS ID=%s, route CMS IP=%s, route CMS Port=%d, device record count=%d\r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);

    /* 4����ȡ���� */
    record_count = group_record_count + device_record_count;

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "RouteGetGBDeviceListAndSendNotifyCatalogTo3PartyCMS() record_count=%d \r\n", record_count);

    /* 5�������¼��Ϊ0 */
    if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "RouteGetGBDeviceListAndSendNotifyCatalogTo3PartyCMS() exit---: No Record Count \r\n");
        return i;
    }

    /* 6��ѭ��������������ȡ�û����豸��Ϣ������xml�� */
    CPacket* pOutPacket = NULL;

    /* �Ȳ���TCP���� */
    //tcp_socket = CMS_CreateSIPTCPConnect(pRouteInfo->server_ip, pRouteInfo->server_port);

    if (tcp_socket >= 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�����߼��豸Ŀ¼֪ͨ��Ϣ���������ϼ�CMS, ����SIP TCP�ɹ�,��ͨ��SIP TCP����Ŀ¼Catalog��Ϣ���������ϼ�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket);

        for (index = 0; index < record_count; index++)
        {
            /* �����¼������4����Ҫ�ִη��� */
            query_count++;

            /* ����XMLͷ�� */
            i = CreateGBLogicDeviceCatalogNotifyXMLHeadForRoute(&pOutPacket, query_count, record_count, (char*)"3456", local_cms_id_get(), &ListAccNode);

            if (index < group_record_count)
            {
                pPrimaryGroup = get_primary_group((char*)GroupIDVector[index].c_str());

                if (NULL != pPrimaryGroup)
                {
                    /* ����Item ֵ */
                    i = AddLogicDeviceGroupInfoToXMLItemForRoute(pOutPacket, ListAccNode, pPrimaryGroup->group_code, pPrimaryGroup->group_name, pPrimaryGroup->parent_code);
                }
                else
                {
                    /* ����Item ֵ */
                    i = AddLogicDeviceGroupInfoToXMLItemForRoute(pOutPacket, ListAccNode, (char*)"", (char*)"", NULL);
                }
            }
            else if (index >= group_record_count)
            {
                /* ����Item ֵ */
                i = AddLogicDeviceInfoToXMLItemForRouteNotify(pOutPacket, ListAccNode, (char*)DeviceIDVector[index - group_record_count].c_str(), pRouteInfo->three_party_flag, pRoute_Srv_dboper);
            }

            if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 0) || (query_count == record_count))
            {
                if (NULL != pOutPacket)
                {
                    send_count++;

                    /* ת����Ϣ���ϼ�CMS */
                    i = SIP_SendNotify_By_TCP(NULL, local_cms_id_get(), pRouteInfo->server_id, (char*)"Catalog", 234, 3600, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length(), tcp_socket);

                    if (i != 0)
                    {
                        if (i == -2)
                        {
                            SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "ͨ��TCP����Ŀ¼�仯Notify��Ϣ���������ϼ�CMSʧ��,TCP�����쳣�ر�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d, ���ʹ���=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket, send_count);
                            EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logic device info from superior CMS,Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() SIP_SendNotify_By_TCP Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            break;
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ͨ��TCP����Ŀ¼�仯Notify��Ϣ���������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d, ���ʹ���=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket, send_count);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Directory changes Notify messages sent to a third party the superior CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendNotifyCatalogTo3PartyCMS() SIP_SendNotify_By_TCP Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͨ��TCP����Ŀ¼�仯Notify��Ϣ���������ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d, ���ʹ���=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket, send_count);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Directory changes Notify messages sent to a third party the superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendNotifyCatalogTo3PartyCMS() SIP_SendNotify_By_TCP OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    }

                    delete pOutPacket;
                    pOutPacket = NULL;
                }
            }
        }

        CMS_CloseSIPTCPConnect(tcp_socket);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�����߼��豸Ŀ¼֪ͨ��Ϣ���������ϼ�CMS, ����SIP TCPʧ��,��ͨ��SIP UDP����Ŀ¼Catalog��Ϣ���������ϼ�CMS:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket);

        for (index = 0; index < record_count; index++)
        {
            /* �����¼������4����Ҫ�ִη��� */
            query_count++;

            /* ����XMLͷ�� */
            i = CreateGBLogicDeviceCatalogNotifyXMLHeadForRoute(&pOutPacket, query_count, record_count, (char*)"3456", local_cms_id_get(), &ListAccNode);

            if (index < group_record_count)
            {
                pPrimaryGroup = get_primary_group((char*)GroupIDVector[index].c_str());

                if (NULL != pPrimaryGroup)
                {
                    /* ����Item ֵ */
                    i = AddLogicDeviceGroupInfoToXMLItemForRoute(pOutPacket, ListAccNode, pPrimaryGroup->group_code, pPrimaryGroup->group_name, pPrimaryGroup->parent_code);
                }
                else
                {
                    /* ����Item ֵ */
                    i = AddLogicDeviceGroupInfoToXMLItemForRoute(pOutPacket, ListAccNode, (char*)"", (char*)"", NULL);
                }
            }
            else if (index >= group_record_count)
            {
                /* ����Item ֵ */
                i = AddLogicDeviceInfoToXMLItemForRouteNotify(pOutPacket, ListAccNode, (char*)DeviceIDVector[index - group_record_count].c_str(), pRouteInfo->three_party_flag, pRoute_Srv_dboper);
            }

            if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 0) || (query_count == record_count))
            {
                if (NULL != pOutPacket)
                {
                    send_count++;

                    if (pRouteInfo->catalog_subscribe_dialog_index >= 0)
                    {
                        /* ת����Ϣ���ϼ�CMS */
                        i = SIP_SendNotifyWithinDialog(pRouteInfo->catalog_subscribe_dialog_index, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());
                    }
                    else
                    {
                        /* ת����Ϣ���ϼ�CMS */
                        i = SIP_SendNotify(NULL, local_cms_id_get(), pRouteInfo->server_id, (char*)"Catalog", 234, 3600, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());
                    }

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����Ŀ¼�仯Notify��Ϣ���������ϼ�CMSʧ��:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ���ʹ���=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, send_count);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Directory changes Notify messages sent to a third party the superior CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendNotifyCatalogTo3PartyCMS() SIP_SendNotify Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����Ŀ¼�仯Notify��Ϣ���������ϼ�CMS�ɹ�:�ϼ�CMS ID=%s, IP��ַ=%s, �˿ں�=%d, ���ʹ���=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, send_count);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Directory changes Notify messages sent to a third party the superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendNotifyCatalogTo3PartyCMS() SIP_SendNotify OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    }

                    delete pOutPacket;
                    pOutPacket = NULL;
                }
            }
        }
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendNotifyCatalogTo3PartyCMS Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 �� �� ��  : SetGBLogicDeviceInfoDelFlagForRoute
 ��������  : ����·����Ϣ���������߼�ͨ����ɾ����ʶ
 �������  : route_info_t* pRouteInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��10��10��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SetGBLogicDeviceInfoDelFlagForRoute(route_info_t* pRouteInfo)
{
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;

    if (pRouteInfo == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SetGBLogicDeviceInfoDelFlagForRoute() exit---: Param Error \r\n");
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

        if (0 == sstrcmp(pGBLogicDeviceInfo->cms_id, pRouteInfo->server_id))
        {
            pGBLogicDeviceInfo->del_mark = 1;
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 �� �� ��  : SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute
 ��������  : ����ɾ����ʶ�����߼��豸�Ľ��ñ�ʶ
 �������  : route_info_t* pRouteInfo
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
int SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute(route_info_t* pRouteInfo, DBOper* pDevice_Srv_dboper)
{
    int index = 0;
    int iRet = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBLogicDevice_Info_Iterator Itr;
    vector<string> MasterDeviceIDVector;

    if (pRouteInfo == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute() exit---: Param Error \r\n");
        return -1;
    }

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

        if (0 == sstrcmp(pGBLogicDeviceInfo->cms_id, pRouteInfo->server_id))
        {
            if (pGBLogicDeviceInfo->del_mark == 1)
            {
                pGBLogicDeviceInfo->enable = 0;
                pGBLogicDeviceInfo->status = 0;
                MasterDeviceIDVector.push_back(pGBLogicDeviceInfo->device_id);
            }
        }
    }

    GBLOGICDEVICE_SMUTEX_UNLOCK();

    if (MasterDeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)MasterDeviceIDVector.size(); index++)
        {
            pGBLogicDeviceInfo = GBLogicDevice_info_find((char*)MasterDeviceIDVector[index].c_str());

            if (NULL == pGBLogicDeviceInfo)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute() exit---: Get Device Info Error:device_id=%s \r\n", (char*)MasterDeviceIDVector[index].c_str());
                continue;
            }

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϼ�CMSĿ¼��ѯ��Ӧ��Ϣ:����ɾ����ʶ�����߼��豸�Ľ��ñ�ʶ, �߼��豸ID=%s", pGBLogicDeviceInfo->device_id);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Superior CMS directory search response message:set logic device disabled notification accorrding to delete notification, logic deviceID=%s", pGBLogicDeviceInfo->device_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute(): Disable Device: device_id=%s \r\n", pGBLogicDeviceInfo->device_id);

            iRet = SendDeviceStatusToAllClientUser((char*)MasterDeviceIDVector[index].c_str(), 0, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute() SendDeviceStatusToAllClientUser Error:iRet=%d \r\n", iRet);
            }
            else if (iRet > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute() SendDeviceStatusToAllClientUser OK:iRet=%d \r\n", iRet);
            }

            /* ����ɾ����Ϣ���¼�CMS  */
            iRet = SendNotifyCatalogToSubCMS(pGBLogicDeviceInfo, 1, pDevice_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute() SendNotifyCatalogToSubCMS Error:iRet=%d \r\n", iRet);
            }
            else if (iRet > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute() SendNotifyCatalogToSubCMS OK:iRet=%d \r\n", iRet);
            }

            /* �����豸״̬�澯��Ϣ�������û�  */
            iRet = SendDeviceOffLineAlarmToAllClientUser((char*)MasterDeviceIDVector[index].c_str());

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute() SendDeviceOffLineAlarmToAllClientUser Error:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute() SendDeviceOffLineAlarmToAllClientUser OK:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }

            /* ���������߼��豸��λҵ��ֹͣ*/
            iRet = StopAllServiceTaskByLogicDeviceID((char*)MasterDeviceIDVector[index].c_str());

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute() StopAllServiceTaskByLogicDeviceID Error:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute() StopAllServiceTaskByLogicDeviceID OK:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }

            /* ���������߼��豸��λ��Ƶ�Խ�ҵ��ֹͣ */
            iRet = StopAudioServiceTaskByLogicDeviceID((char*)MasterDeviceIDVector[index].c_str());

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute() StopAudioServiceTaskByLogicDeviceID Error:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute() StopAudioServiceTaskByLogicDeviceID OK:DeviceID=%s, iRet=%d \r\n", (char*)MasterDeviceIDVector[index].c_str(), iRet);
            }

            /* ͬ�������ݿ� */
            iRet = AddGBLogicDeviceInfo2DBForRoute((char*)MasterDeviceIDVector[index].c_str(), pDevice_Srv_dboper);
        }
    }

    MasterDeviceIDVector.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : RouteInfoConfig_db_refresh_proc
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
int RouteInfoConfig_db_refresh_proc()
{
    if (1 == db_RouteInfo_reload_mark) /* ����ִ�� */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ϼ�·���������ݿ���Ϣ����ͬ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Route Info database information are synchronized");
        return 0;
    }

    db_RouteInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 �� �� ��  : MMSRouteInfoConfig_refresh_proc
 ��������  : �����ֻ�MMS�ϼ�·��������Ϣ���²�����ʶ
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
int MMSRouteInfoConfig_refresh_proc()
{
    if (1 == db_MMSRouteInfo_reload_mark) /* ����ִ�� */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�ֻ�MMS�ϼ�·��������Ϣ����ͬ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "MMS Route Info information are synchronized");
        return 0;
    }

    db_MMSRouteInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 �� �� ��  : check_RouteInfoConfig_need_to_reload_begin
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
void check_RouteInfoConfig_need_to_reload_begin(DBOper* pDboper)
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_RouteInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ���ϼ�·���������ݿ���Ϣ: ��ʼ---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization route info database information: begain---");

    /* ����·����Ϣ���е�ɾ����ʶ */
    set_route_info_list_del_mark(2);

    /* ��·���������ݿ��еı仯����ͬ�����ڴ� */
    check_route_info_from_db_to_list(pDboper);

    return;
}

/*****************************************************************************
 �� �� ��  : check_MMSRouteInfoConfig_need_to_reload_begin
 ��������  : ����Ƿ���Ҫͬ���ֻ�MMS�ϼ�·�����ÿ�ʼ
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
void check_MMSRouteInfoConfig_need_to_reload_begin()
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_MMSRouteInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ���ֻ�MMS�ϼ�·���������ݿ���Ϣ: ��ʼ---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization mms route info database information: begain---");

    /* ����·����Ϣ���е�ɾ����ʶ */
    set_mms_route_info_list_del_mark(2);

    /* ���Ĭ�ϵ�mms·����Ϣ */
    add_default_mms_route_info_to_route_info_list();

    return;
}

/*****************************************************************************
 �� �� ��  : check_RouteInfoConfig_need_to_reload_end
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
void check_RouteInfoConfig_need_to_reload_end()
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_RouteInfo_reload_mark)
    {
        return;
    }

    /* ɾ��������ϼ�·��������Ϣ */
    delete_route_info_from_list_by_mark();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ���ϼ�·���������ݿ���Ϣ: ����---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization route info database information: end---");
    db_RouteInfo_reload_mark = 0;

    return;
}

/*****************************************************************************
 �� �� ��  : check_MMSRouteInfoConfig_need_to_reload_end
 ��������  : ����Ƿ���Ҫͬ���ֻ�MMS�ϼ�·�����ý���
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
void check_MMSRouteInfoConfig_need_to_reload_end()
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_MMSRouteInfo_reload_mark)
    {
        return;
    }

    /* ɾ��������߼��豸��Ϣ */
    delete_mms_route_info_from_list_by_mark();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ���ֻ�MMS�ϼ�·���������ݿ���Ϣ: ����---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization mms route info database information: end---");
    db_MMSRouteInfo_reload_mark = 0;

    return;
}

/*****************************************************************************
 �� �� ��  : IsRouteHasPermissionForDevice
 ��������  : ����ϼ�·���Ƿ��е�λȨ��
 �������  : unsigned int device_index
             route_info_t* pRouteInfo
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��12��10��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int IsRouteHasPermissionForDevice(unsigned int device_index, route_info_t* pRouteInfo, DBOper* pDboper)
{
    int record_count = 0;
    string strSQL = "";
    char strDeviceIndex[16] = {0};
    char strRouteIndex[16] = {0};
    char strDeviceType[16] = {0};

    if ((device_index <= 0) || (NULL == pRouteInfo) || (NULL == pDboper))
    {
        return 0;
    }

    strSQL.clear();
    snprintf(strDeviceIndex, 16, "%u", device_index);
    snprintf(strRouteIndex, 16, "%u", pRouteInfo->id);
    snprintf(strDeviceType, 16, "%d", EV9000_DEVICETYPE_SCREEN);

    if (pRouteInfo->three_party_flag) /* ������ƽ̨ */
    {
        strSQL.clear();
        strSQL = "select GDC.DeviceID from GBLogicDeviceConfig as GDC, RouteDevicePermConfig as RDPC WHERE GDC.ID = RDPC.DeviceIndex and GDC.Enable=1 and RDPC.RouteIndex = "; /* ��ѯȨ�ޱ� */
        strSQL += strRouteIndex;
        strSQL += " AND RDPC.DeviceIndex = ";
        strSQL += strDeviceIndex;

        record_count = pDboper->DB_Select(strSQL.c_str(), 1);

        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "IsRouteHasPermissionForDevice() record_count=%d, route_index=%d \r\n", record_count, pRouteInfo->id);

        if (record_count <= 0) /* ����ԭ����, ��ȡ���е��߼���λ */
        {
            strSQL = "select DeviceID from GBLogicDeviceConfig WHERE Enable = 1 AND OtherRealm = 0 AND DeviceType <> ";
            strSQL += strDeviceType;
            strSQL += " AND ID = ";
            strSQL += strDeviceIndex;
            strSQL += " order by DeviceName asc";

            record_count = pDboper->DB_Select(strSQL.c_str(), 1);

            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "IsRouteHasPermissionForDevice() record_count=%d, route_index=%d \r\n", record_count, pRouteInfo->id);

            if (record_count <= 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "IsRouteHasPermissionForDevice() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "IsRouteHasPermissionForDevice() ErrorMsg=%s\r\n", pDboper->GetLastDbErrorMsg());
                return 0;
            }
        }
    }
    else
    {
        if (1 == pRouteInfo->link_type) /* ͬ��������£���Ҫ�ϱ�����ǽͨ��,Ȩ�޲���Ч */
        {
            strSQL = "select DeviceID from GBLogicDeviceConfig WHERE OtherRealm = 0 AND ID = ";
            strSQL += strDeviceIndex;
            strSQL += " order by DeviceName asc";

            record_count = pDboper->DB_Select(strSQL.c_str(), 1);

            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "IsRouteHasPermissionForDevice() record_count=%d, route_index=%d \r\n", record_count, pRouteInfo->id);

            if (record_count <= 0)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "IsRouteHasPermissionForDevice() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "IsRouteHasPermissionForDevice() ErrorMsg=%s\r\n", pDboper->GetLastDbErrorMsg());
                return 0;
            }
        }
        else
        {
            strSQL.clear();
            strSQL = "select GDC.DeviceID from GBLogicDeviceConfig as GDC, RouteDevicePermConfig as RDPC WHERE GDC.ID = RDPC.DeviceIndex and RDPC.RouteIndex = "; /* ��ѯȨ�ޱ� */
            strSQL += strRouteIndex;
            strSQL += " AND RDPC.DeviceIndex = ";
            strSQL += strDeviceIndex;

            record_count = pDboper->DB_Select(strSQL.c_str(), 1);

            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "IsRouteHasPermissionForDevice() record_count=%d, route_index=%d \r\n", record_count, pRouteInfo->id);

            if (record_count <= 0) /* ����ԭ����, ��ȡ���е��߼���λ */
            {
                strSQL = "select DeviceID from GBLogicDeviceConfig WHERE OtherRealm = 0 AND DeviceType <> ";
                strSQL += strDeviceType;
                strSQL += " AND ID = ";
                strSQL += strDeviceIndex;
                strSQL += " order by DeviceName asc";

                record_count = pDboper->DB_Select(strSQL.c_str(), 1);

                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "IsRouteHasPermissionForDevice() record_count=%d, route_index=%d \r\n", record_count, pRouteInfo->id);

                if (record_count <= 0)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "IsRouteHasPermissionForDevice() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "IsRouteHasPermissionForDevice() ErrorMsg=%s\r\n", pDboper->GetLastDbErrorMsg());
                    return 0;
                }
            }
        }
    }

    if (record_count > 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*****************************************************************************
 �� �� ��  : checkRoutIfHasSendCataProc
 ��������  : ����Ƿ���Route���ڷ���Catalog
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��2��20��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int checkRoutIfHasSendCataProc()
{
    int iResult = 0;
    int pos = -1;
    route_info_t* pRouteInfo = NULL;

    if (NULL == g_RouteInfoList)
    {
        return 0;
    }

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (pRouteInfo->reg_status == 0 || pRouteInfo->reg_info_index < 0)
        {
            continue;
        }

        if (pRouteInfo->catlog_get_status == 1) /* �ϼ����ڻ�ȡCatalog */
        {
            printf("checkRoutIfHasSendCataProc() Route: server_id=%s, server_ip=%s, server_port=%d Sending Catalog Now \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            iResult = 1;
            break;
        }
    }

    ROUTE_INFO_SMUTEX_UNLOCK();

    return iResult;
}

/*****************************************************************************
 �� �� ��  : ShowRouteInfo
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
void ShowRouteInfo(int sock, int type)
{
    int i = 0;
    char strLine[] = "\r----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rServer ID            Server IP       Server Port  Link Type TransType RegLocalEthName RegLocalIP      ThreeParty Status     IPConflict SubscribeStatus SubscribeExpires CatalogStauts UACIndex\r\n";
    route_info_t* pRouteInfo = NULL;
    char rbuf[256] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if ((NULL == g_RouteInfoList) || (NULL == g_RouteInfoList->pRouteInfoList))
    {
        return;
    }

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_RouteInfoList->pRouteInfoList); i++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, i);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (type <= 1)
        {
            if (type != pRouteInfo->reg_status)
            {
                continue;
            }
        }

        if (0 == pRouteInfo->reg_status)
        {
            snprintf(rbuf, 256, "\r%-20s %-15s %-12d %-9d %-9d %-15s %-15s %-10d %-10s %-10d %-15d %-16d %-13d %-8d\r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->link_type, pRouteInfo->trans_protocol, pRouteInfo->strRegLocalEthName, pRouteInfo->strRegLocalIP, pRouteInfo->three_party_flag, (char*)"UnRegister", pRouteInfo->ip_is_in_sub, pRouteInfo->catalog_subscribe_flag, pRouteInfo->catalog_subscribe_expires, pRouteInfo->catlog_get_status, pRouteInfo->reg_info_index);
        }
        else if (1 == pRouteInfo->reg_status)
        {
            snprintf(rbuf, 256, "\r%-20s %-15s %-12d %-9d %-9d %-15s %-15s %-10d %-10s %-10d %-15d %-16d %-13d %-8d\r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->link_type, pRouteInfo->trans_protocol, pRouteInfo->strRegLocalEthName, pRouteInfo->strRegLocalIP, pRouteInfo->three_party_flag, (char*)"Registered", pRouteInfo->ip_is_in_sub, pRouteInfo->catalog_subscribe_flag, pRouteInfo->catalog_subscribe_expires, pRouteInfo->catlog_get_status, pRouteInfo->reg_info_index);
        }
        else
        {
            snprintf(rbuf, 256, "\r%-20s %-15s %-12d %-9d %-9d %-15s %-15s %-10d %-10s %-10d %-15d %-16d %-13d %-8d\r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->link_type, pRouteInfo->trans_protocol, pRouteInfo->strRegLocalEthName, pRouteInfo->strRegLocalIP, pRouteInfo->three_party_flag, (char*)"Unknow", pRouteInfo->ip_is_in_sub, pRouteInfo->catalog_subscribe_flag, pRouteInfo->catalog_subscribe_expires, pRouteInfo->catlog_get_status, pRouteInfo->reg_info_index);
        }

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    ROUTE_INFO_SMUTEX_UNLOCK();

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}
