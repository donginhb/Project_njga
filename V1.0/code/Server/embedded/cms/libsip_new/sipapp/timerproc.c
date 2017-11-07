/******************************************************************************

                  版权所有 (C), 2001-2013, 金智视讯技术有限公司

 ******************************************************************************
  文 件 名   : timerproc.c
  版 本 号   : 初稿
  作    者   : yanghaifeng
  生成日期   : 2013年4月1日
  最近修改   :
  功能描述   : 定时器管理
  函数列表   :
              cit_do
              cit_free
              cit_init
              cs_scan_timer_list
              cs_scan_timer_event
              cs_scan_10m
              cs_scan_1s
              cs_scan_30s
              cs_timer_find
              cs_timer_id_free
              cs_timer_id_init
              cs_timer_list_add
              cs_timer_list_clean
              cs_timer_list_free
              cs_timer_list_init
              cs_timer_list_lock
              cs_timer_list_unlock
              cs_timer_remove
              cs_timer_use
              cs_time_count
              itt_do
              itt_free
              itt_init
              ixt_do
              ixt_free
              ixt_init
              ixt_init_as_2xx
              ixt_init_as_ack
              onsignal
              sip_timer_creat
              sip_timer_destroy
              ua_scan_timer_list
              ua_timer_find
              ua_timer_find_pos
              ua_timer_free
              ua_timer_id_free
              ua_timer_id_init
              ua_timer_id_match
              ua_timer_init
              ua_timer_list_free
              ua_timer_list_init
              ua_timer_list_lock
              ua_timer_list_unlock
              ua_timer_remove
              ua_timer_use
  修改历史   :
  1.日    期   : 2013年4月1日
    作    者   : yanghaifeng
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#endif
#include <signal.h>

#include <osipparser2/osip_const.h>
#include <osip2/internal.h>
#include <osip2/osip.h>
#include <osip2/osip_fifo.h>
#include <osipparser2/osip_list.h>
#include <osipparser2/osip_uri.h>
#include <osipparser2/osip_port.h>

#include "gbltype.h"
#include "timerproc.inc"

#include "csdbg.inc"
#include "registrar.inc"
#include "sipmsg.inc"
#include "sipua.inc"
#include "sip_event.inc"
#include "callback.inc"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern app_callback_t* g_AppCallback;

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
ua_timer_list_t* g_UaTimerList = NULL;        /* UA sip定时器队列 */
cs_timer_list_t* g_CsTimerList_1s = NULL;
cs_timer_list_t* g_CsTimerList_30s = NULL;
cs_timer_list_t* g_CsTimerList_10m = NULL;
cs_timer_list_t* g_CsTimerList = NULL;

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#if DECS("服务端")

#ifdef WIN32   //modified by chenyu 130522
#pragma comment(lib,"Winmm.lib")
void CALLBACK cs_time_count_win(
    UINT uID,
    UINT uMsg,
    DWORD dwUser,
    DWORD dw1,
    DWORD dw2
)
//VOID CALLBACK cs_time_count_win(HWND hwnd, UINT message, UINT iTimerID, DWORD dwTime)
{
    if (NULL == g_CsTimerList)
    {
        return;
    }

    g_CsTimerList->scan_1s_flag = 1;
    return;
}

#else
void cs_time_count()
{
    if (NULL == g_CsTimerList)
    {
        return;
    }

    g_CsTimerList->scan_1s_flag = 1;
    return;
}
#endif

int  cs_timer_id_init(cs_timer_id_t** node)
{
    *node = (cs_timer_id_t*)osip_malloc(sizeof(cs_timer_id_t));

    if (*node == NULL)
    {
        return -1;
    }

    (*node)->count = 0;
    (*node)->pos = -1;
    (*node)->timer = NULL;

    return 0;
}

void cs_timer_id_free(cs_timer_id_t* node)
{
    if (node == NULL)
    {
        return;
    }

    switch (node->type)
    {
        case IN_REG_EXPIRE:
            osip_free(node->timer);
            node->timer = NULL;
            break;

        case OUT_REG_TIMER:
            osip_free(node->timer);
            node->timer = NULL;
            break;

        case UA_SESSION_EXPIRE:
            osip_free(node->timer);
            node->timer = NULL;
            break;

        case UAC_SEND_UPDATE:
            osip_free(node->timer);
            node->timer = NULL;
            break;

        default:
            osip_free(node->timer);
            node->timer = NULL;
            break;
    }

    node->timer = NULL;
    node->count = 0;
    node->pos = -1;
    return;
}

int cs_timer_list_init(cs_timer_list_t** timer_link)
{
    (*timer_link) = (cs_timer_list_t*)osip_malloc(sizeof(cs_timer_list_t));

    if (*timer_link == NULL)
    {
        return -1;
    }

    /* init timer list*/
    (*timer_link)->timer_list = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == (*timer_link)->timer_list)
    {
        osip_free(*timer_link);
        *timer_link = NULL;
        return -1;
    }

    osip_list_init((*timer_link)->timer_list);

#ifdef MULTI_THR
    /* init smutex */
    (*timer_link)->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == (*timer_link)->lock)
    {
        osip_free((*timer_link)->timer_list);
        (*timer_link)->timer_list = NULL;
        osip_free(*timer_link);
        *timer_link = NULL;
        return -1;
    }

#endif
    return 0;
}

int cs_timer_list_clean(cs_timer_list_t* timer_link)
{
    cs_timer_id_t* timer_node = NULL;

    CS_TIMER_SMUTEX_LOCK((struct osip_mutex*)timer_link->lock);

    while (!osip_list_eol(timer_link->timer_list, 0))
    {
        timer_node = (cs_timer_id_t*)osip_list_get(timer_link->timer_list, 0);

        if (NULL != timer_node)
        {
            osip_list_remove(timer_link->timer_list, 0);
            cs_timer_id_free(timer_node);
            osip_free(timer_node);
            timer_node = NULL;
        }
    }

    CS_TIMER_SMUTEX_UNLOCK((struct osip_mutex*)timer_link->lock);

    return 0;

}

void cs_timer_list_free(cs_timer_list_t* timer_link)
{
    if (NULL != timer_link)
    {
        cs_timer_list_clean(timer_link);
        osip_free(timer_link->timer_list);
        timer_link->timer_list = NULL;
#ifdef MULTI_THR
        osip_mutex_destroy((struct osip_mutex*)timer_link->lock);
        timer_link->lock = NULL;
#endif
    }

    return;
}

int cs_timer_list_lock(cs_timer_list_t* timer_link)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (timer_link == NULL || timer_link->lock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_timer_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)timer_link->lock);

#endif
    return iRet;
}

int cs_timer_list_unlock(cs_timer_list_t* timer_link)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (timer_link == NULL || timer_link->lock == NULL)
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "cs_timer_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)timer_link->lock);

#endif
    return iRet;
}

int cs_timer_list_add(cs_timer_list_t* link, cs_timer_id_t* node)
{
    if (NULL == link || NULL == node)
    {
        return -1;
    }

    CS_TIMER_SMUTEX_LOCK((struct osip_mutex*)link->lock);

    osip_list_add(link->timer_list, node, -1);

    CS_TIMER_SMUTEX_UNLOCK((struct osip_mutex*)link->lock);

    return 0;
}

cs_timer_id_t* cs_timer_find(cs_timer_type type, int pos, void* timer)
{
    int i;
    cs_timer_id_t* node = NULL;

    switch (type)
    {
        case IN_REG_EXPIRE:
            if (pos < 0)
            {
                return NULL;
            }

            CS_TIMER_SMUTEX_LOCK((struct osip_mutex*)g_CsTimerList_1s->lock);

            i = 0;

            while (!osip_list_eol(g_CsTimerList_1s->timer_list, i))
            {
                node = (cs_timer_id_t*)osip_list_get(g_CsTimerList_1s->timer_list, i);

                if (NULL == node)
                {
                    i++;
                    continue;
                }

                if (node->type == IN_REG_EXPIRE && node->pos == pos)
                {
                    CS_TIMER_SMUTEX_UNLOCK((struct osip_mutex*)g_CsTimerList_1s->lock);
                    return node;
                }

                i++;
            }

            CS_TIMER_SMUTEX_UNLOCK((struct osip_mutex*)g_CsTimerList_1s->lock);
            break;

        case OUT_REG_TIMER:
            if (pos < 0)
            {
                return NULL;
            }

            CS_TIMER_SMUTEX_LOCK((struct osip_mutex*)g_CsTimerList_1s->lock);

            i = 0;

            while (!osip_list_eol(g_CsTimerList_1s->timer_list, i))
            {
                node = (cs_timer_id_t*)osip_list_get(g_CsTimerList_1s->timer_list, i);

                if (NULL == node)
                {
                    i++;
                    continue;
                }

                if (node->type == OUT_REG_TIMER && node->pos == pos)
                {
                    CS_TIMER_SMUTEX_UNLOCK((struct osip_mutex*)g_CsTimerList_1s->lock);
                    return node;
                }

                i++;
            }

            CS_TIMER_SMUTEX_UNLOCK((struct osip_mutex*)g_CsTimerList_1s->lock);
            break;

        case UA_SESSION_EXPIRE:
            if (pos < 0)
            {
                return NULL;
            }

            CS_TIMER_SMUTEX_LOCK((struct osip_mutex*)g_CsTimerList_1s->lock);

            i = 0;

            while (!osip_list_eol(g_CsTimerList_1s->timer_list, i))
            {
                node = (cs_timer_id_t*)osip_list_get(g_CsTimerList_1s->timer_list, i);

                if (NULL == node)
                {
                    i++;
                    continue;
                }

                if (node->type == UA_SESSION_EXPIRE && node->pos == pos)
                {
                    CS_TIMER_SMUTEX_UNLOCK((struct osip_mutex*)g_CsTimerList_1s->lock);
                    return node;
                }

                i++;
            }

            CS_TIMER_SMUTEX_UNLOCK((struct osip_mutex*)g_CsTimerList_1s->lock);
            break;

        case UAC_SEND_UPDATE:
            if (pos < 0)
            {
                return NULL;
            }

            CS_TIMER_SMUTEX_LOCK((struct osip_mutex*)g_CsTimerList_1s->lock);

            i = 0;

            while (!osip_list_eol(g_CsTimerList_1s->timer_list, i))
            {
                node = (cs_timer_id_t*)osip_list_get(g_CsTimerList_1s->timer_list, i);

                if (NULL == node)
                {
                    i++;
                    continue;
                }

                if (node->type == UAC_SEND_UPDATE && node->pos == pos)
                {
                    CS_TIMER_SMUTEX_UNLOCK((struct osip_mutex*)g_CsTimerList_1s->lock);
                    return node;
                }

                i++;
            }

            CS_TIMER_SMUTEX_UNLOCK((struct osip_mutex*)g_CsTimerList_1s->lock);
            break;

        default:
            break;
    }

    return NULL;
}

int  cs_timer_use(cs_timer_type type, int pos, void* timer)
{
    cs_timer_id_t* node;

    switch (type)
    {
        case IN_REG_EXPIRE:
            if (pos < 0)
            {
                return -1;
            }

            node = cs_timer_find(IN_REG_EXPIRE, pos, NULL);

            if (node == NULL)
            {
                cs_timer_id_init(&node);
                node->type = IN_REG_EXPIRE;
                node->pos = pos;
                cs_timer_list_add(g_CsTimerList_1s,  node);
            }

            break;

        case OUT_REG_TIMER:
            if (pos < 0)
            {
                return -1;
            }

            node = cs_timer_find(OUT_REG_TIMER, pos, NULL);

            if (node == NULL)
            {
                cs_timer_id_init(&node);
                node->type = OUT_REG_TIMER;
                node->pos = pos;
                cs_timer_list_add(g_CsTimerList_1s,  node);
                SIP_DEBUG_TRACE(LOG_INFO, "cs_timer_use() OUT_REG_TIMER cs_timer_list_add: pos=%d \r\n", pos);
            }

            SIP_DEBUG_TRACE(LOG_INFO, "cs_timer_use() OUT_REG_TIMER break: pos=%d \r\n", pos);
            break;

        case UA_SESSION_EXPIRE:
            if (pos < 0)
            {
                return -1;
            }

            node = cs_timer_find(UA_SESSION_EXPIRE, pos, NULL);

            if (node == NULL)
            {
                cs_timer_id_init(&node);
                node->type = UA_SESSION_EXPIRE;
                node->pos = pos;
                cs_timer_list_add(g_CsTimerList_1s,  node);
            }

            break;

        case UAC_SEND_UPDATE:
            if (pos < 0)
            {
                return -1;
            }

            node = cs_timer_find(UAC_SEND_UPDATE, pos, NULL);

            if (node == NULL)
            {
                cs_timer_id_init(&node);
                node->type = UAC_SEND_UPDATE;
                node->pos = pos;
                cs_timer_list_add(g_CsTimerList_1s,  node);
            }

            break;


        default:
            break;
    }

    return 0;
}

int  cs_timer_remove(cs_timer_type type, int pos, void* timer)
{
    cs_timer_id_t* node;

    switch (type)
    {
        case IN_REG_EXPIRE:
            if (pos < 0)
            {
                return -1;
            }

            node = cs_timer_find(IN_REG_EXPIRE, pos, NULL);

            if (node != NULL)
            {
                node->pos = -1;
            }

            break;

        case OUT_REG_TIMER:
            if (pos < 0)
            {
                return -1;
            }

            node = cs_timer_find(OUT_REG_TIMER, pos, NULL);

            if (node != NULL)
            {
                node->pos = -1;
            }

            break;

        case UA_SESSION_EXPIRE:
            if (pos < 0)
            {
                return -1;
            }

            node = cs_timer_find(UA_SESSION_EXPIRE, pos, NULL);

            if (node != NULL)
            {
                node->pos = -1;
            }

            break;

        case UAC_SEND_UPDATE:
            if (pos < 0)
            {
                return -1;
            }

            node = cs_timer_find(UAC_SEND_UPDATE, pos, NULL);

            if (node != NULL)
            {
                node->pos = -1;
            }

            break;

        default:
            break;
    }

    return 0;
}

void cs_scan_1s()
{
    static int i = 0;

    cs_scan_timer_list(g_CsTimerList_1s);
    i++;

    /*
    *After 30 second ,we should send a "SCAN30S" message to the
    *  timer_event , which can inform the main progranm scan
    *  g_CsTimerList_30s link when processing  timer events .
    */
    if (30 == i)
    {
        g_CsTimerList->scan_30s_flag = 1;
        i = 0;
    }

    return;
}

void cs_scan_30s()
{
    static int i = 0;
    //SIP_DEBUG_TRACE(LOG_INFO, "***scaning 30s link***\r\n");
    cs_scan_timer_list(g_CsTimerList_30s);
    i++;

    /*
    *After 10 minutes,we should send a "SCAN310m" message to the
    *  timer_event , which can inform the main progranm scan
    *  timerlink_10m link when processing  timer events .
    */
    if (20 == i)
    {
        g_CsTimerList->scan_10m_flag = 1;
        i = 0;
    }

    return;
}

void cs_scan_10m()
{
    cs_scan_timer_list(g_CsTimerList_10m);
    return;
}

#ifdef WIN32    //added by chenyu 131223
MMRESULT nIDTimerEvent = 0; //定时器标志
#endif
int cs_sip_timer_creat()
{
    int iRet = 0;
    iRet = cs_timer_list_init(&g_CsTimerList_1s);

    if (0 != iRet)
    {
        return -1;
    }

    iRet = cs_timer_list_init(&g_CsTimerList_30s);

    if (0 != iRet)
    {
        cs_timer_list_free(g_CsTimerList_1s);
        osip_free(g_CsTimerList_1s);
        g_CsTimerList_1s = NULL;
        return -1;
    }

    iRet = cs_timer_list_init(&g_CsTimerList_10m);

    if (0 != iRet)
    {
        cs_timer_list_free(g_CsTimerList_1s);
        osip_free(g_CsTimerList_1s);
        g_CsTimerList_1s = NULL;
        cs_timer_list_free(g_CsTimerList_30s);
        osip_free(g_CsTimerList_30s);
        g_CsTimerList_30s = NULL;
        return -1;
    }

    iRet = cs_timer_list_init(&g_CsTimerList);

    if (0 != iRet)
    {
        cs_timer_list_free(g_CsTimerList_1s);
        osip_free(g_CsTimerList_1s);
        g_CsTimerList_1s = NULL;
        cs_timer_list_free(g_CsTimerList_30s);
        osip_free(g_CsTimerList_30s);
        g_CsTimerList_30s = NULL;
        cs_timer_list_free(g_CsTimerList_10m);
        osip_free(g_CsTimerList_10m);
        g_CsTimerList_10m = NULL;
        return -1;
    }

    /*Init the scan_timer_flag*/
    g_CsTimerList->scan_1s_flag = 0;
    g_CsTimerList->scan_30s_flag = 0;
    g_CsTimerList->scan_10m_flag = 0;

#ifdef WIN32   //modified by chenyu 130522 待测试
    //SetTimer(NULL, 1, 1000, cs_time_count_win);
    //启动计时器
    nIDTimerEvent = timeSetEvent(
                        1000,//延时1秒
                        0,
                        cs_time_count_win,
                        0,
                        (UINT)TIME_PERIODIC);

    if (nIDTimerEvent == 0)
    {
        OutputDebugString("cytest-启动计时器失败");
        return -1;
    }

#endif

    return 0;
}

void cs_sip_timer_destroy()
{
#ifdef WIN32   //modified by chenyu 130522  待测试
    //KillTimer(NULL, 1) ;
    timeKillEvent(nIDTimerEvent);
#else

#endif

    if (NULL != g_CsTimerList_1s)
    {
        cs_timer_list_free(g_CsTimerList_1s);
        osip_free(g_CsTimerList_1s);
        g_CsTimerList_1s = NULL;
    }

    if (NULL != g_CsTimerList_30s)
    {
        cs_timer_list_free(g_CsTimerList_30s);
        osip_free(g_CsTimerList_30s);
        g_CsTimerList_30s = NULL;
    }

    if (NULL != g_CsTimerList_10m)
    {
        cs_timer_list_free(g_CsTimerList_10m);
        osip_free(g_CsTimerList_10m);
        g_CsTimerList_10m = NULL;
    }

    if (NULL != g_CsTimerList)
    {
        cs_timer_list_free(g_CsTimerList);
        osip_free(g_CsTimerList);
        g_CsTimerList = NULL;
    }

    return;
}

void cs_scan_timer_list(cs_timer_list_t* timer_link)
{
    int pos = 0;
    int index = -1;
    vector<int> UASRegPosVector;
    vector<int> UACRegPosVector;
    vector<int> UASessionPosVector;
    vector<int> UASendUpdatePosVector;

    cs_timer_id_t* timer_node = NULL;

    if (NULL == timer_link)
    {
        return;
    }

    CS_TIMER_SMUTEX_LOCK((struct osip_mutex*)timer_link->lock);

    pos = 0;

    UASRegPosVector.clear();
    UACRegPosVector.clear();
    UASessionPosVector.clear();
    UASendUpdatePosVector.clear();

    while (!osip_list_eol(timer_link->timer_list, pos))
    {
        timer_node = (cs_timer_id_t*)osip_list_get(timer_link->timer_list, pos);

        if (NULL == timer_node)
        {
            pos++;
            continue;
        }

        switch (timer_node->type)
        {
            case IN_REG_EXPIRE:
                if (timer_node->pos < 0) /*invalid pos then remove the timer */
                {
                    osip_list_remove(timer_link->timer_list, pos);
                    cs_timer_id_free(timer_node);
                    osip_free(timer_node);
                    timer_node = NULL;
                    pos++;
                    continue;
                }

                UASRegPosVector.push_back(timer_node->pos);
                break;

            case OUT_REG_TIMER:
                if (timer_node->pos < 0)
                {
                    osip_list_remove(timer_link->timer_list, pos);
                    cs_timer_id_free(timer_node);
                    osip_free(timer_node);
                    timer_node = NULL;
                    pos++;
                    continue;
                }

                UACRegPosVector.push_back(timer_node->pos);
                break;

            case UA_SESSION_EXPIRE:
                if (timer_node->pos < 0) /*invalid pos then remove the timer */
                {
                    osip_list_remove(timer_link->timer_list, pos);
                    cs_timer_id_free(timer_node);
                    osip_free(timer_node);
                    timer_node = NULL;
                    pos++;
                    continue;
                }

                UASessionPosVector.push_back(timer_node->pos);
                break;

            case UAC_SEND_UPDATE:
                if (timer_node->pos < 0) /*invalid pos then remove the timer */
                {
                    osip_list_remove(timer_link->timer_list, pos);
                    cs_timer_id_free(timer_node);
                    osip_free(timer_node);
                    timer_node = NULL;
                    pos++;
                    continue;
                }

                UASendUpdatePosVector.push_back(timer_node->pos);
                break;

            default:
                break;
        }

        pos++;
    }

    CS_TIMER_SMUTEX_UNLOCK((struct osip_mutex*)timer_link->lock);

    if (UASRegPosVector.size() > 0)
    {
        for (index = 0; index < (int)UASRegPosVector.size(); index++)
        {
            uas_register_expire_proc(UASRegPosVector[index]);
        }
    }

    UASRegPosVector.clear();

    if (UACRegPosVector.size() > 0)
    {
        for (index = 0; index < (int)UACRegPosVector.size(); index++)
        {
            uac_register_expire_proc(UACRegPosVector[index]);
        }
    }

    UACRegPosVector.clear();

    if (UASessionPosVector.size() > 0)
    {
        for (index = 0; index < (int)UASessionPosVector.size(); index++)
        {
            ua_ssesion_expire_proc(UASessionPosVector[index]);
        }
    }

    UASessionPosVector.clear();

    if (UASendUpdatePosVector.size() > 0)
    {
        for (index = 0; index < (int)UASendUpdatePosVector.size(); index++)
        {
            ua_send_update_proc(UASendUpdatePosVector[index]);
        }
    }

    UASendUpdatePosVector.clear();

    return;
}

/*****************************************************************************
 函 数 名  : cs_scan_timer_event
 功能描述  : 服务端定时器扫描
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
void cs_scan_timer_event()
{
    if ((NULL == g_CsTimerList) || (NULL == g_CsTimerList->timer_list))
    {
        return;
    }

    /* Whether scan the 1s , 30s  and 10m timerlink */
    if (1 == g_CsTimerList->scan_1s_flag)
    {
        //printf("\n scan 1s timer link!\n");
        cs_scan_1s();
        g_CsTimerList->scan_1s_flag = 0;
        return;
    }

    if (1 == g_CsTimerList->scan_30s_flag)
    {
        //printf("\n scan 30s timer link!\n");
        cs_scan_30s();
        g_CsTimerList->scan_30s_flag = 0;
        return;
    }

    if (1 == g_CsTimerList->scan_10m_flag)
    {
        //printf("\n scan 10m timer link!\n");
        cs_scan_10m();
        g_CsTimerList->scan_10m_flag = 0;
        return;
    }

    return;
}

int uas_register_expire_proc(int pos)
{
    char proxy_id[128 + 4] = {0};
    char register_id[128 + 4] = {0};
    char login_ip[16] = {0};
    int login_port = 0;
    uas_reg_info_t* pUasRegInfo = NULL;
    int isNeedToRemoveTimer = 0;

    UAS_SMUTEX_LOCK();

    pUasRegInfo = uas_reginfo_get2(pos);

    if (pUasRegInfo != NULL)
    {
        osip_strncpy(register_id, pUasRegInfo->register_id, 128);

        if ('\0' != pUasRegInfo->serverid[0])
        {
            osip_strncpy(proxy_id, pUasRegInfo->serverid, 128);
        }

        pUasRegInfo->expires_count--;

        if (pUasRegInfo->expires_count <= 0)
        {
            osip_strncpy(login_ip, pUasRegInfo->from_host, 16);
            login_port = pUasRegInfo->from_port;
            isNeedToRemoveTimer = 1;
        }
    }

    UAS_SMUTEX_UNLOCK();

    if (1 == isNeedToRemoveTimer)
    {
        cs_timer_remove(IN_REG_EXPIRE, pos, NULL);

        //通知上层应用注册超时
        /* 调用钩子函数 */
        if (NULL != g_AppCallback && NULL != g_AppCallback->uas_register_received_timeout_cb)
        {
            g_AppCallback->uas_register_received_timeout_cb(proxy_id, register_id, login_ip, login_port, pos);
        }

        uas_reginfo_remove(pos);
    }

    return 0;
}

int uac_register_expire_proc(int pos)
{
    uac_reg_info_t* pUacRegInfo = NULL;
    int isNeedToRemoveTimer = 0;

    UAC_SMUTEX_LOCK();

    pUacRegInfo = uac_reginfo_get2(pos);

    if (pUacRegInfo != NULL)
    {
        pUacRegInfo->expires--;

        if (pUacRegInfo->expires <= 0)
        {
            //SIP_DEBUG_TRACE(LOG_INFO, "uac_register_expire_proc() :service_id=%s, server_ip=%s, server_port=%d, local_id=%s, local_ip=%s, local_port=%d, register_callid_number=%s, expires=%d, pos=%d \r\n", pUacRegInfo->proxy_id, pUacRegInfo->proxyip, pUacRegInfo->proxyport, pUacRegInfo->register_id, pUacRegInfo->localip, pUacRegInfo->localport, pUacRegInfo->register_callid_number, pUacRegInfo->expires, pos);
            uac_reginfo_free(pUacRegInfo);
            isNeedToRemoveTimer = 1;
        }
    }

    UAC_SMUTEX_UNLOCK();

    if (1 == isNeedToRemoveTimer)
    {
        cs_timer_remove(OUT_REG_TIMER, pos, NULL);
        SIP_DEBUG_TRACE(LOG_INFO, "uac_register_expire_proc() OUT_REG_TIMER REMOVE:pos=%d \r\n", pos);

        /* 通知上层应用，收到注册回应超时 */
        /* 调用钩子函数 */
        if (NULL != g_AppCallback && NULL != g_AppCallback->uac_register_response_received_cb)
        {
            g_AppCallback->uac_register_response_received_cb(pos, 0, 408, NULL, 0, g_AppCallback->uac_register_response_received_cb_user_data);
        }
    }

    return 0;
}

int ua_ssesion_expire_proc(int pos)
{
    int i = 0;
    ua_dialog_t* pUaDialog = NULL;
    int isNeedToRemoveTimer = 0;

    USED_UA_SMUTEX_LOCK();

    pUaDialog = ua_dialog_get2(pos);

    if (pUaDialog != NULL && pUaDialog->iSessionExpiresCount > 0 && UI_STATE_CONNECTED == pUaDialog->eUiState)
    {
        pUaDialog->iSessionExpiresCount--;

        if (pUaDialog->iSessionExpiresCount <= 0)
        {
            isNeedToRemoveTimer = 1;
        }
    }

    USED_UA_SMUTEX_UNLOCK();

    if (1 == isNeedToRemoveTimer)
    {
        cs_timer_remove(UA_SESSION_EXPIRE, pos, NULL);

        //通知上层应用注册超时
        /* 调用钩子函数 */
        if (NULL != g_AppCallback && NULL != g_AppCallback->ua_session_expires_cb)
        {
            g_AppCallback->ua_session_expires_cb(pos);
        }

        i = SIP_SendBye(pos);
        SIP_DEBUG_TRACE(LOG_ERROR, "ua_ssesion_expire_proc(): UA Session Expires: SIP_SendBye:index=%d, i=%d \r\n", pos, i);
    }

    return 0;
}

int ua_send_update_proc(int pos)
{
    int iRet = 0;
    ua_dialog_t* pUaDialog = NULL;
    int isNeedToSendUpdate = 0;
    int isNeedToRemoveTimer = 0;

    USED_UA_SMUTEX_LOCK();

    pUaDialog = ua_dialog_get2(pos);

    if (pUaDialog != NULL)
    {
        pUaDialog->iUpdateSendCount--;

        if (pUaDialog->iUpdateSendCount <= 0)
        {
            isNeedToSendUpdate = 1;
            pUaDialog->iUpdateSendCount = pUaDialog->iSessionExpires / 2;
        }

        if (pUaDialog->iSessionExpiresCount <= 0)
        {
            isNeedToRemoveTimer = 1;
        }
    }

    USED_UA_SMUTEX_UNLOCK();

    if (1 == isNeedToSendUpdate)
    {
        iRet = sip_update(pos);

        if (iRet != 0)
        {
            isNeedToRemoveTimer = 1;
            //SIP_DEBUG_TRACE(LOG_DEBUG, "ua_send_update_proc() SIP Update Send Error:pos=%d \r\n", pos);
        }
        else
        {
            //SIP_DEBUG_TRACE(LOG_DEBUG, "ua_send_update_proc() SIP Update Send OK:pos=%d \r\n", pos);
        }
    }

    if (1 == isNeedToRemoveTimer)
    {
        cs_timer_remove(UAC_SEND_UPDATE, pos, NULL);
        //SIP_DEBUG_TRACE(LOG_DEBUG, "ua_send_update_proc() cs_timer_remove:UAC_SEND_UPDATE:pos=%d \r\n", pos);
    }

    return 0;
}

#endif

#if DECS("客户端")
/* add by yanghf 2008/03/14 */
int ixt_init(ixt_t1** ixt, osip_message_t* sip, int out_socket)
{
    if (NULL == sip)
    {
        return -1;
    }

    if (MSG_IS_ACK(sip))
    {
        return ixt_init_as_ack(ixt, sip, out_socket);
    }

    if (MSG_IS_STATUS_2XX(sip))
    {
        return ixt_init_as_2xx(ixt, sip, out_socket);
    }

    return -1;
}

int ixt_init_as_2xx(ixt_t1** ixt, osip_message_t* sip, int out_socket)
{
    int i;
    int port;
    char* dest;

    if (NULL == sip)
    {
        return -1;
    }

    *ixt = (ixt_t1*)osip_malloc(sizeof(ixt_t1));

    if (NULL == (*ixt))
    {
        return -1;
    }

    i = response_get_destination(sip, &dest, &port);

    if (i != 0)
    {
        osip_free(*ixt);
        *ixt = NULL;
        return -1;
    }

    (*ixt)->msg2xx = sip;     /* copy of string to retransmit */
    (*ixt)->ack = NULL;       /* useless for ist */
    (*ixt)->dest = dest;
    (*ixt)->port = port;
    (*ixt)->sock = out_socket;

    (*ixt)->start = time(NULL);    /* send the first 200 for INVITE time */

    (*ixt)->interval = 64 * DEFAULT_T1 * 5;  /*when reach 64*T1 and not receive ACK then BYE it*/
    (*ixt)->retransmit_start = time(NULL);
    (*ixt)->retransmit_interval = DEFAULT_T1;  /* init to DEFAULT_T1 ,when reach DEFAULT_T2
    stop retramsmit*/
    return 0;
}

int ixt_init_as_ack(ixt_t1** ixt, osip_message_t* sip, int out_socket)
{
    int i;
    int port;
    char* dest;

    if (NULL == sip)
    {
        return -1;
    }

    *ixt = (ixt_t1*)osip_malloc(sizeof(ixt_t1));

    if (NULL == (*ixt))
    {
        return -1;
    }

    i = request_get_destination(sip, &dest, &port);

    if (i != 0)
    {
        osip_free(*ixt);
        *ixt = NULL;
        return -1;
    }

    (*ixt)->msg2xx = NULL;     /* copy of string to retransmit */
    (*ixt)->ack = sip;            /* useless for ist */
    (*ixt)->dest = dest;
    (*ixt)->port = port;
    (*ixt)->sock = out_socket;

    (*ixt)->start = time(NULL);          /* received the first 200 for INVITE time */
    (*ixt)->interval = 64 * DEFAULT_T1 * 5;  /* default: 64*T1 */
    (*ixt)->retransmit_start = time(NULL);
    (*ixt)->retransmit_interval = 64 * DEFAULT_T1; /* init to  not need retransmit */
    return 0;
}

void ixt_free(ixt_t1* ixt)
{
    if (NULL == ixt)
    {
        return;
    }

    if (NULL != ixt->msg2xx)
    {
        osip_message_free(ixt->msg2xx);
        ixt->msg2xx = NULL;
    }

    if (NULL != ixt->ack)
    {
        osip_message_free(ixt->ack);
        ixt->ack = NULL;
    }

    if (NULL != ixt->dest)
    {
        osip_free(ixt->dest);
        ixt->dest = NULL;
    }

    return;
}

/*
return : -1 :error (free the ixt_t struct)
0 :retransmit ok
1 :2xx retransmit timeout (do something and free the ixt_t struct)
2 :ACK for 2xx retransmit timeout (do something and free the ixt_t struct)
*/
int ixt_do(ixt_t1* ixt)
{
    time_t now = time(NULL);

    if (NULL == ixt)
    {
        return -1;
    }

    if (ixt->msg2xx != NULL)   /* 2xx */
    {
        if ((now - ixt->start) * 1000 >= ixt->interval)  /* timer out */
        {
            return 1;
        }

        if (ixt->retransmit_interval >= DEFAULT_T2)       /* stop retransmit ?*/
        {
            return 0;
        }

        if ((now - ixt->retransmit_start) * 1000 >= ixt->retransmit_interval) /* retransmit ?*/
        {
            tl_sendmessage(ixt->msg2xx, ixt->dest,  ixt->port, ixt->sock);
            ixt->retransmit_start = now;
            ixt->retransmit_interval *= 2;
        }
    }
    else if (ixt->ack != NULL)  /* ACK */
    {
        if ((now - ixt->start) * 1000 >= ixt->interval)
        {
            return 2;
        }

        if ((now - ixt->retransmit_start) * 1000 >= ixt->retransmit_interval)
        {
            tl_sendmessage(ixt->ack, ixt->dest, ixt->port, ixt->sock);
            ixt->retransmit_start = now;
            ixt->retransmit_interval = ixt->interval ;    /* will not retransmit again, except
            set the value to 0 when receive 2xx for INVTE */
            SIP_DEBUG_TRACE(LOG_INFO, "ixt_do() tl_sendmessage Ack \r\n");
        }
    }
    else
    {
        return -1;
    }

    return 0;

}

int cit_init(cit_t** cit, osip_transaction_t* tr)
{
    if (NULL == tr)
    {
        return -1;
    }

    *cit = (cit_t*)osip_malloc(sizeof(cit_t));

    if (NULL == (*cit))
    {
        return -1;
    }

    (*cit)->start = time(NULL);     /*send the CANCEL time*/
    (*cit)->interval = 64 * DEFAULT_T1 * 5;  /*default: 64*T1 */

    return 0;
}

void cit_free(cit_t* cit)
{
    if (NULL == cit)
    {
        return;
    }

    return;
}

/*    return : -1 :error (free the cit_t struct)
0 : ok
1 : timeout (do something and free the cit_t struct)
*/
int  cit_do(cit_t* cit)
{
    time_t now = time(NULL);

    if (NULL == cit)
    {
        return -1;
    }

    if ((now - cit->start) * 1000 >=  cit->interval)
    {
        return 1;
    }

    return 0;
}

int itt_init(itt_t** itt, int expires)
{
    *itt = (itt_t*)osip_malloc(sizeof(itt_t));

    if (NULL == (*itt))
    {
        return -1;
    }

    (*itt)->start = time(NULL);     /*send the INVITE time*/
    (*itt)->interval = expires;    /*expires*/

    return 0;
}
void itt_free(itt_t* itt)
{
    if (NULL == itt)
    {
        return;
    }

    return;
}
/*    return : -1 :error (free the act_t struct)
0 : ok
1 : timeout (do something and free the act_t struct)
*/
int itt_do(itt_t* itt)
{
    time_t now = time(NULL);

    if (NULL == itt)
    {
        return -1;
    }

    if ((now - itt->start) >= itt->interval)
    {
        return 1;
    }

    return 0;
}

int ua_timer_id_init(ua_timer_id_t** id, ua_timer_type type)
{
    *id = (ua_timer_id_t*)osip_malloc(sizeof(ua_timer_id_t));

    if (NULL == (*id))
    {
        return -1;
    }

    (*id)->type = type;
    (*id)->index = -1;
    (*id)->tr = NULL;
    (*id)->sip = NULL;
    return 0;
}

void ua_timer_id_free(ua_timer_id_t* id)
{
    if (NULL == id)
    {
        return;
    }

    id->index = -1;
    id->tr = NULL;
    id->sip = NULL;
    osip_free(id);
    id = NULL;
    return;
}

int ua_timer_id_match(ua_timer_id_t* id1, ua_timer_id_t* id2)
{
    osip_call_id_t* callid1, *callid2;
    osip_cseq_t* cseq1, *cseq2;
    ua_timer_type type;

    if (NULL == id1 || NULL == id2)
    {
        return -1;
    }

    type = id1->type;

    if (type != id2->type)
    {
        return -1;
    }

    switch (type)
    {
        case UA_ACK2XX_RETRANSMIT:
            if (id1->index != id2->index)
            {
                break;
            }

            if (NULL == id1->sip || NULL == id2->sip)
            {
                break;
            }

            callid1 = osip_message_get_call_id(id1->sip);
            callid2 = osip_message_get_call_id(id2->sip);
            cseq1 = osip_message_get_cseq(id1->sip);
            cseq2 = osip_message_get_cseq(id2->sip);

            if (NULL == callid1 || NULL == callid2 || NULL == cseq1 || NULL == cseq2)
            {
                break;
            }

            if (0 != osip_call_id_match(callid1, callid2))
            {
                break;
            }

            if (0 != osip_cseq_match(cseq1, cseq2))
            {
                break;
            }

            return 0;

        case UA_INVITE_TIMEOUT:
            if (id1->index == id2->index)
            {
                return 0;
            }

            break;

        case UA_CANCEL_TIMEOUT:
            if (id1->tr == id2->tr)
            {
                return 0;
            }

            break;
    }

    return -1;
}

int  ua_timer_init(ua_timer_t** timer)
{
    *timer = (ua_timer_t*)osip_malloc(sizeof(ua_timer_t));

    if (NULL == (*timer))
    {
        return -1;
    }

    (*timer)->timer = NULL;
    (*timer)->id = NULL;
    return 0;
}

void ua_timer_free(ua_timer_t* timer)
{
    if (NULL == timer)
    {
        return;
    }

    switch (timer->id->type)
    {
        case UA_ACK2XX_RETRANSMIT:
            ixt_free((ixt_t1*)timer->timer);
            osip_free(timer->timer);
            timer->timer = NULL;
            break;

        case UA_INVITE_TIMEOUT:
            itt_free((itt_t*)timer->timer);
            osip_free(timer->timer);
            timer->timer = NULL;
            break;

        case UA_CANCEL_TIMEOUT:
            cit_free((cit_t*)timer->timer);
            osip_free(timer->timer);
            timer->timer = NULL;
            break;

        default:  /* ??? */
            osip_free(timer->timer);
            timer->timer = NULL;
    }

    ua_timer_id_free(timer->id);

    osip_free(timer);
    timer = NULL;

    return;
}

int  ua_timer_list_init()
{
    g_UaTimerList = (ua_timer_list_t*)osip_malloc(sizeof(ua_timer_list_t));

    if (g_UaTimerList == NULL)
    {
        return -1;
    }

    g_UaTimerList->tl = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (g_UaTimerList->tl == NULL)
    {
        osip_free(g_UaTimerList);
        g_UaTimerList = NULL;
        return -1;
    }

    osip_list_init(g_UaTimerList->tl);

#ifdef MULTI_THR
    g_UaTimerList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_UaTimerList->lock)
    {
        osip_free(g_UaTimerList->tl);
        g_UaTimerList->tl = NULL;
        osip_free(g_UaTimerList);
        g_UaTimerList = NULL;
        return -1;
    }

#endif
    return 0;
}

void ua_timer_list_free()
{
    if (g_UaTimerList == NULL)
    {
        return;
    }

    if (NULL != g_UaTimerList->tl)
    {
        osip_list_special_free(g_UaTimerList->tl, (void (*)(void*))&ua_timer_free);
        osip_free(g_UaTimerList->tl);
        g_UaTimerList->tl = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_UaTimerList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_UaTimerList->lock);
        g_UaTimerList->lock = NULL;
    }

#endif
    osip_free(g_UaTimerList);
    g_UaTimerList = NULL;
    return;
}

int ua_timer_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if ((g_UaTimerList == NULL) || (g_UaTimerList->lock == NULL))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "ua_timer_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_UaTimerList->lock);

#endif
    return iRet;
}

int ua_timer_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if ((g_UaTimerList == NULL) || (g_UaTimerList->lock == NULL))
    {
        //SIP_DEBUG_TRACE(LOG_DEBUG, "ua_timer_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_UaTimerList->lock);

#endif
    return iRet;
}

ua_timer_t* ua_timer_use(ua_timer_type type, int index, osip_transaction_t* tr, osip_message_t* sip)
{
    int i = 0;
    ua_timer_t* timer = NULL;
    ua_timer_id_t* id = NULL;
    ixt_t1* ixt = NULL;
    itt_t* itt = NULL;
    cit_t* cit = NULL;
    i = ua_timer_init(&timer);

    if (i != 0)
    {
        return NULL;
    }

    i = ua_timer_id_init(&id, type);

    if (i != 0)
    {
        ua_timer_free(timer);
        timer = NULL;
        return NULL;
    }

    switch (type)
    {
        case UA_ACK2XX_RETRANSMIT:
            if (NULL == sip || !is_valid_dialog_index(index))
            {
                goto error;
            }

            if (NULL == tr)
            {
                i = ixt_init(&ixt, sip, 0);
            }
            else
            {
                i = ixt_init(&ixt, sip, tr->out_socket);
            }

            if (i != 0)
            {
                goto error;
            }

            id->index = index;
            id->sip = sip;
            timer->timer = ixt;
            timer->id = id;
            break;

        case UA_INVITE_TIMEOUT:
            if (!is_valid_dialog_index(index))
            {
                goto error;
            }

            //i = itt_init(&itt, g_siprofile.); /* 30秒 */
            i = itt_init(&itt, 30 * 200);

            if (i != 0)
            {
                goto error;
            }

            id->index = index;
            timer->timer = itt;
            timer->id = id;
            break;

        case UA_CANCEL_TIMEOUT:
            if (NULL == tr)
            {
                goto error;
            }

            i = cit_init(&cit, tr);

            if (i != 0)
            {
                goto error;
            }

            id->tr = tr;
            timer->timer = cit;
            timer->id = id;
            break;

        default:
            goto error;

    }

    UA_TIMER_SMUTEX_LOCK();

    i = osip_list_add(g_UaTimerList->tl, timer, -1); /* add to list tail */

    if (i < 0)
    {
        goto error;
    }

    UA_TIMER_SMUTEX_UNLOCK();
    return timer;
error:
    ua_timer_id_free(id);
    id = NULL;
    ua_timer_free(timer);
    timer = NULL;
    UA_TIMER_SMUTEX_UNLOCK();
    return NULL;
}

int ua_timer_remove(ua_timer_type type, int index, osip_transaction_t* tr, osip_message_t* sip)
{
    int pos = 0;
    ua_timer_id_t* id = NULL;
    ua_timer_t* timer = NULL;
    ua_timer_id_init(&id, type);

    if (NULL == id)
    {
        return -1;
    }

    id->index = index;
    id->tr = tr;
    id->sip = sip;

    UA_TIMER_SMUTEX_LOCK();

    pos = ua_timer_find_pos(id);

    if (pos < 0) /* error : not found */
    {
        goto error;
    }

    timer = (ua_timer_t*)osip_list_get(g_UaTimerList->tl, pos);
    osip_list_remove(g_UaTimerList->tl, pos);
    ua_timer_free(timer);
    timer = NULL;
    ua_timer_id_free(id);
    UA_TIMER_SMUTEX_UNLOCK();
    return 0;

error:
    UA_TIMER_SMUTEX_UNLOCK();
    ua_timer_id_free(id);
    return -1;
}

int ua_timer_find_pos(ua_timer_id_t* id)
{
    int i = 0, pos = 0;
    ua_timer_type type;
    ua_timer_t* timer = NULL;

    if (NULL == id)
    {
        return -1;
    }

    type = id->type;

    while (!osip_list_eol(g_UaTimerList->tl, pos))
    {
        timer = (ua_timer_t*)osip_list_get(g_UaTimerList->tl, pos);

        if (NULL == timer)
        {
            pos++;
            continue;
        }

        if (timer->id->type == type)
        {
            i = ua_timer_id_match(id, timer->id);

            if (0 == i)
            {
                return pos;
            }
        }

        pos++;
    }

    return -1;
}

ua_timer_t* ua_timer_find(ua_timer_type type, int index, osip_transaction_t* tr, osip_message_t* sip)
{
    int pos = -1;
    ua_timer_id_t* id = NULL;
    ua_timer_t* timer = NULL;
    ua_timer_id_init(&id, type);

    if (NULL == id)
    {
        return NULL;
    }

    id->index = index;
    id->tr = tr;
    id->sip = sip;

    UA_TIMER_SMUTEX_LOCK();

    pos = ua_timer_find_pos(id);

    if (pos < 0) /* error : not found */
    {
        goto error;
    }

    timer = (ua_timer_t*)osip_list_get(g_UaTimerList->tl, pos);
    ua_timer_id_free(id);
    UA_TIMER_SMUTEX_UNLOCK();
    return timer;

error:
    UA_TIMER_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : ua_scan_timer_list
 功能描述  : UA 定时器扫描
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
void ua_scan_timer_list()
{
    int index = 0;
    int i = 0, pos = -1;
    ua_timer_t* timer = NULL;
    ua_timer_id_t* timer_id = NULL;
    ixt_t1* ixt = NULL;
    itt_t* itt = NULL;
    cit_t* cit = NULL;
    ua_dialog_t* pUaDialog = NULL;
    sip_dialog_t* pSipDlg = NULL;
    char* caller_id = NULL;
    char* callee_id = NULL;
    char* call_id = NULL;
    vector<int> UAIndexVector1;
    vector<int> UAIndexVector2;

    UAIndexVector1.clear();
    UAIndexVector2.clear();

    if ((g_UaTimerList == NULL) || (NULL == g_UaTimerList->tl))
    {
        return;
    }

    UA_TIMER_SMUTEX_LOCK();

    if (osip_list_size(g_UaTimerList->tl) <= 0)
    {
        UA_TIMER_SMUTEX_UNLOCK();
        return;
    }

    pos = 0;

    while (!osip_list_eol(g_UaTimerList->tl, pos))
    {
        timer = (ua_timer_t*)osip_list_get(g_UaTimerList->tl, pos);

        if (timer == NULL || timer->id == NULL)
        {
            pos++;
            continue;
        }

        timer_id = timer->id;

        switch (timer_id->type)
        {
            case UA_ACK2XX_RETRANSMIT:
                pUaDialog = ua_dialog_get2(timer_id->index);

                if (NULL != pUaDialog
                    && (pUaDialog->eUiState == UI_STATE_CONNECTED || pUaDialog->eUiState == UI_STATE_CALL_ACCEPT))
                {
                    ixt = (ixt_t1*)timer->timer;
                    i = ixt_do(ixt);

                    if (0 == i)
                    {
                        pos++;
                        break;
                    }
                    else if (1 == i)
                    {
                        UAIndexVector1.push_back(timer_id->index);
                    }
                    else if (2 == i)
                    {
                    }
                }

                osip_list_remove(g_UaTimerList->tl, pos);
                ua_timer_free(timer);
                timer = NULL;
                break;

            case UA_INVITE_TIMEOUT:
                pUaDialog = ua_dialog_get2(timer_id->index);

                if (NULL != pUaDialog)
                {
                    itt = (itt_t*)timer->timer;
                    i = itt_do(itt);

                    if (0 == i)
                    {
                        pos++;
                        break;
                    }
                    else if (1 == i)
                    {
                        UAIndexVector2.push_back(timer_id->index);
                    }
                }

                osip_list_remove(g_UaTimerList->tl, pos);
                ua_timer_free(timer);
                timer = NULL;
                break;

            case UA_CANCEL_TIMEOUT:
                cit = (cit_t*)timer->timer;
                i = cit_do(cit);

                if (0 == i)
                {
                    pos++;
                    break;
                }
                else if (1 == i)
                {
                    //osip_remove_transaction(g_cell, timer_id->tr);
                    //throw_2garbage(g_garbage, timer_id->tr);
                }

                osip_list_remove(g_UaTimerList->tl, pos);
                ua_timer_free(timer);
                timer = NULL;
                break;

            default:
                osip_list_remove(g_UaTimerList->tl, pos);
                ua_timer_free(timer);
                timer = NULL;
        }

        pos++;
    }

    UA_TIMER_SMUTEX_UNLOCK();

    if (UAIndexVector1.size() > 0)
    {
        for (index = 0; index < (int)UAIndexVector1.size(); index++)
        {
            /* 通知上层 */
            pSipDlg = get_dialog_sip_dialog(UAIndexVector1[index]);

            if (NULL != pSipDlg)
            {
                if (NULL != g_AppCallback && NULL != g_AppCallback->bye_received_cb)
                {
                    if (NULL != pSipDlg->local_uri && NULL != pSipDlg->local_uri->url && pSipDlg->local_uri->url->username)
                    {
                        caller_id = osip_getcopy(pSipDlg->local_uri->url->username);
                    }

                    if (NULL != pSipDlg->remote_uri && NULL != pSipDlg->remote_uri->url && pSipDlg->remote_uri->url->username)
                    {
                        callee_id = osip_getcopy(pSipDlg->remote_uri->url->username);
                    }

                    if (NULL != pSipDlg->call_id)
                    {
                        call_id = osip_getcopy(pSipDlg->call_id);
                    }

                    g_AppCallback->bye_received_cb(caller_id, callee_id, call_id, timer_id->index, 0);

                    if (NULL != caller_id)
                    {
                        osip_free(caller_id);
                        caller_id = NULL;
                    }

                    if (NULL != callee_id)
                    {
                        osip_free(callee_id);
                        callee_id = NULL;
                    }

                    if (NULL != call_id)
                    {
                        osip_free(call_id);
                        call_id = NULL;
                    }
                }
            }

            SIP_DEBUG_TRACE(LOG_ERROR, "ua_scan_timer_list() UA_ACK2XX_RETRANSMIT Time Out: SIP_SendBye:index=%d \r\n", UAIndexVector1[index]);

            SIP_SendBye(UAIndexVector1[index]);
            OnTimeoutBye(UAIndexVector1[index]);
        }
    }

    UAIndexVector1.clear();

    if (UAIndexVector2.size() > 0)
    {
        for (index = 0; index < (int)UAIndexVector2.size(); index++)
        {
            sip_cancel(UAIndexVector2[index]);    /* time out */
            OnTimeoutCancel(UAIndexVector2[index]);

            SIP_DEBUG_TRACE(LOG_ERROR, "ua_scan_timer_list() UA_INVITE_TIMEOUT Time Out: sip_cancel:index=%d \r\n", UAIndexVector2[index]);

            /* 通知上层 */
            pSipDlg = get_dialog_sip_dialog(UAIndexVector2[index]);

            if (NULL != pSipDlg)
            {
                if (NULL != g_AppCallback && NULL != g_AppCallback->cancel_received_cb)
                {
                    if (NULL != pSipDlg->local_uri && NULL != pSipDlg->local_uri->url && pSipDlg->local_uri->url->username)
                    {
                        caller_id = osip_getcopy(pSipDlg->local_uri->url->username);
                    }

                    if (NULL != pSipDlg->remote_uri && NULL != pSipDlg->remote_uri->url && pSipDlg->remote_uri->url->username)
                    {
                        callee_id = osip_getcopy(pSipDlg->remote_uri->url->username);
                    }

                    if (NULL != pSipDlg->call_id)
                    {
                        call_id = osip_getcopy(pSipDlg->call_id);
                    }

                    g_AppCallback->cancel_received_cb(caller_id, callee_id, call_id, timer_id->index, 0);

                    if (NULL != caller_id)
                    {
                        osip_free(caller_id);
                        caller_id = NULL;
                    }

                    if (NULL != callee_id)
                    {
                        osip_free(callee_id);
                        callee_id = NULL;
                    }

                    if (NULL != call_id)
                    {
                        osip_free(call_id);
                        call_id = NULL;
                    }
                }
            }
        }
    }

    UAIndexVector2.clear();

    return;
}
#endif

/*****************************************************************************
 函 数 名  : SIP_1sTimerNotify
 功能描述  : 协议栈1秒定时器通知函数
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月6日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void SIP_1sTimerNotify()
{
#ifndef WIN32   //modified by chenyu 130522
    cs_time_count();
#endif
    return;
}
