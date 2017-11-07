
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
#include "common/log_proc.inc"

#include "route/route_reg_proc.inc"
#include "route/route_info_mgn.inc"

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
route_reg_msg_list_t* g_RouteRegMsgList = NULL;    /* ·��ע����Ϣ���� */

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

#if DECS("·��ע����Ϣ����")
/*****************************************************************************
 �� �� ��  : route_reg_msg_init
 ��������  : ·��ע����Ϣ�ṹ��ʼ��
 �������  : route_reg_msg_t ** route_reg_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_reg_msg_init(route_reg_msg_t** route_reg_msg)
{
    *route_reg_msg = (route_reg_msg_t*)osip_malloc(sizeof(route_reg_msg_t));

    if (*route_reg_msg == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_reg_msg_init() exit---: *route_reg_msg Smalloc Error \r\n");
        return -1;
    }

    (*route_reg_msg)->route_id = NULL;
    (*route_reg_msg)->reg_info_index = -1;

    return 0;
}

/*****************************************************************************
 �� �� ��  : route_reg_msg_free
 ��������  : ·��ע����Ϣ�ṹ�ͷ�
 �������  : route_reg_msg_t * route_reg_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void route_reg_msg_free(route_reg_msg_t* route_reg_msg)
{
    if (route_reg_msg == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_reg_msg_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != route_reg_msg->route_id)
    {
        osip_free(route_reg_msg->route_id);
        route_reg_msg->route_id = NULL;
    }

    route_reg_msg->reg_info_index = -1;

    osip_free(route_reg_msg);
    route_reg_msg = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : route_reg_msg_list_init
 ��������  : ·��ע����Ϣ���г�ʼ��
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
int route_reg_msg_list_init()
{
    g_RouteRegMsgList = (route_reg_msg_list_t*)osip_malloc(sizeof(route_reg_msg_list_t));

    if (g_RouteRegMsgList == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_reg_msg_list_init() exit---: g_RouteRegMsgList Smalloc Error \r\n");
        return -1;
    }

    g_RouteRegMsgList->pRouteRegMsgList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_RouteRegMsgList->pRouteRegMsgList)
    {
        osip_free(g_RouteRegMsgList);
        g_RouteRegMsgList = NULL;
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_reg_msg_list_init() exit---: Route Register Message List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_RouteRegMsgList->pRouteRegMsgList);

#ifdef MULTI_THR
    /* init smutex */
    g_RouteRegMsgList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_RouteRegMsgList->lock)
    {
        osip_free(g_RouteRegMsgList->pRouteRegMsgList);
        g_RouteRegMsgList->pRouteRegMsgList = NULL;
        osip_free(g_RouteRegMsgList);
        g_RouteRegMsgList = NULL;
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_reg_msg_list_init() exit---: Route Register Message List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : route_reg_msg_list_free
 ��������  : ·��ע����Ϣ�����ͷ�
 �������  : route_reg_msg_list_t * route_reg_msg_list
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void route_reg_msg_list_free()
{
    if (NULL == g_RouteRegMsgList)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_reg_msg_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_RouteRegMsgList->pRouteRegMsgList)
    {
        osip_list_special_free(g_RouteRegMsgList->pRouteRegMsgList, (void (*)(void*))&route_reg_msg_free);
        osip_free(g_RouteRegMsgList->pRouteRegMsgList);
        g_RouteRegMsgList->pRouteRegMsgList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_RouteRegMsgList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_RouteRegMsgList->lock);
        g_RouteRegMsgList->lock = NULL;
    }

#endif
    osip_free(g_RouteRegMsgList);
    g_RouteRegMsgList = NULL;
    return;
}

/*****************************************************************************
 �� �� ��  : route_reg_msg_list_lock
 ��������  : ·��ע����Ϣ��������
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
int route_reg_msg_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RouteRegMsgList == NULL || g_RouteRegMsgList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_reg_msg_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_RouteRegMsgList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : route_reg_msg_list_unlock
 ��������  : ·��ע����Ϣ����
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
int route_reg_msg_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RouteRegMsgList == NULL || g_RouteRegMsgList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_reg_msg_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_RouteRegMsgList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_route_reg_msg_list_lock
 ��������  : ·��ע����Ϣ��������
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
int debug_route_reg_msg_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RouteRegMsgList == NULL || g_RouteRegMsgList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "debug_route_reg_msg_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_RouteRegMsgList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_route_reg_msg_list_unlock
 ��������  : ·��ע����Ϣ����
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
int debug_route_reg_msg_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RouteRegMsgList == NULL || g_RouteRegMsgList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "debug_route_reg_msg_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_RouteRegMsgList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : route_reg_msg_add
 ��������  : ���·��ע����Ϣ��������
 �������  : char* route_id
                            int reg_info_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int route_reg_msg_add(char* route_id, int reg_info_index)
{
    route_reg_msg_t* pRouteRegMsg = NULL;
    int i = 0;

    if (g_RouteRegMsgList == NULL || route_id == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_reg_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    i = route_reg_msg_init(&pRouteRegMsg);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_reg_msg_add() exit---: Route Register Message Init Error \r\n");
        return -1;
    }

    pRouteRegMsg->route_id = osip_getcopy(route_id);

    pRouteRegMsg->reg_info_index = reg_info_index;

    i = osip_list_add(g_RouteRegMsgList->pRouteRegMsgList, pRouteRegMsg, -1); /* add to list tail */

    if (i < 0)
    {
        route_reg_msg_free(pRouteRegMsg);
        osip_free(pRouteRegMsg);
        pRouteRegMsg = NULL;
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_reg_msg_add() exit---: List Add Error \r\n");
        return -1;
    }

    return i - 1;
}

/*****************************************************************************
 �� �� ��  : route_reg_msg_remove
 ��������  : �Ӷ������Ƴ�·��ע����Ϣ
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
int route_reg_msg_remove(int pos)
{
    route_reg_msg_t* pRouteRegMsg = NULL;

    if (g_RouteRegMsgList == NULL || pos < 0 || (pos >= osip_list_size(g_RouteRegMsgList->pRouteRegMsgList)))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_reg_msg_remove() exit---: Param Error \r\n");
        return -1;
    }

    pRouteRegMsg = (route_reg_msg_t*)osip_list_get(g_RouteRegMsgList->pRouteRegMsgList, pos);

    if (NULL == pRouteRegMsg)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_reg_msg_remove() exit---: List Get Error \r\n");
        return -1;
    }

    osip_list_remove(g_RouteRegMsgList->pRouteRegMsgList, pos);
    route_reg_msg_free(pRouteRegMsg);
    osip_free(pRouteRegMsg);
    pRouteRegMsg = NULL;
    return 0;
}

void scan_route_reg_msg_list()
{
    int iRet = 0;
    route_reg_msg_t* pRouteRegMsg = NULL;

    if (NULL == g_RouteRegMsgList)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "scan_route_reg_msg_list() exit---: Param Error \r\n");
        return;
    }

    while (!osip_list_eol(g_RouteRegMsgList->pRouteRegMsgList, 0))
    {
        pRouteRegMsg = (route_reg_msg_t*)osip_list_get(g_RouteRegMsgList->pRouteRegMsgList, 0);

        if (NULL != pRouteRegMsg)
        {
            iRet = route_reg_msg_proc(pRouteRegMsg->route_id, pRouteRegMsg->reg_info_index);

            iRet = route_reg_msg_remove(0);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "scan_route_reg_msg_list() route_reg_msg_remove:iRet=%d \r\n", iRet);
        }
    }

    return;
}
#endif

int route_reg_msg_proc(char* route_id, int reg_info_index)
{
    return 0;
}
