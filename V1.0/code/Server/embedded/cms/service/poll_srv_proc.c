
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

#include "common/gblfunc_proc.inc"
#include "common/gblconfig_proc.inc"
#include "common/log_proc.inc"

#include "device/device_info_mgn.inc"

#include "user/user_srv_proc.inc"
#include "device/device_srv_proc.inc"

#include "service/poll_srv_proc.inc"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/

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
poll_srv_list_t* g_PollSrvList = NULL;    /* 轮巡业务队列 */
int db_PollSrvInfo_reload_mark = 0;       /* 轮巡业务数据库更新标识:0:不需要更新，1:需要更新数据库 */

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#if DECS("轮巡业务队列")
/*****************************************************************************
 函 数 名  : poll_action_source_init
 功能描述  : 轮巡动作源结构初始化
 输入参数  : poll_action_source_t** poll_action_source
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月3日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int poll_action_source_init(poll_action_source_t** poll_action_source)
{
    *poll_action_source = (poll_action_source_t*)osip_malloc(sizeof(poll_action_source_t));

    if (*poll_action_source == NULL)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_source_init() exit---: *poll_srv Smalloc Error \r\n");
        return -1;
    }

    (*poll_action_source)->iStatus = 0;
    (*poll_action_source)->iType = 0;
    (*poll_action_source)->pcSourceID[0] = '\0';
    (*poll_action_source)->iSourceStreamType = 0;
    (*poll_action_source)->iLiveTime = 0;
    (*poll_action_source)->iLiveTimeCount = 0;
    (*poll_action_source)->iConnectFlag = 0;
    (*poll_action_source)->iConnectCount = 0;
    (*poll_action_source)->del_mark = 0;
    return 0;
}

/*****************************************************************************
 函 数 名  : poll_action_source_free
 功能描述  : 轮巡动作源结构释放
 输入参数  : poll_action_source_t* poll_action_source
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月3日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void poll_action_source_free(poll_action_source_t* poll_action_source)
{
    if (poll_action_source == NULL)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_source_free() exit---: Param Error \r\n");
        return;
    }

    poll_action_source->iStatus = 0;
    poll_action_source->iType = 0;

    memset(poll_action_source->pcSourceID, 0, MAX_ID_LEN + 4);

    poll_action_source->iSourceStreamType = 0;
    poll_action_source->iLiveTime = 0;
    poll_action_source->iLiveTimeCount = 0;
    poll_action_source->iConnectFlag = 0;
    poll_action_source->iConnectCount = 0;
    poll_action_source->del_mark = 0;

    osip_free(poll_action_source);
    poll_action_source = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : poll_action_source_find
 功能描述  : 根据源ID 轮巡动作源查找
 输入参数  : char* pcSourceID
             osip_list_t* pPollActionSourceList
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int poll_action_source_find(char* pcSourceID, osip_list_t* pPollActionSourceList)
{
    int i = 0;
    poll_action_source_t* pPollActionSource = NULL;

    if (NULL == pcSourceID || NULL == pPollActionSourceList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_source_find() exit---: Poll Action List Error \r\n");
        return -1;
    }

    for (i = 0; i < osip_list_size(pPollActionSourceList); i++)
    {
        pPollActionSource = (poll_action_source_t*)osip_list_get(pPollActionSourceList, i);

        if (NULL == pPollActionSource || '\0' == pPollActionSource->pcSourceID[0])
        {
            continue;
        }

        if (0 == sstrcmp(pPollActionSource->pcSourceID, pcSourceID))
        {
            return i;
        }
    }

    return -1;
}

/*****************************************************************************
 函 数 名  : next_poll_action_source_get
 功能描述  : 获取下一个电视墙轮巡动作的源信息
 输入参数  : int current_pos
             osip_list_t* pPollActionSourceList
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月25日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
poll_action_source_t* next_poll_action_source_get(int current_pos, osip_list_t* pPollActionSourceList)
{
    int i = 0;
    int next_pos = 0;
    poll_action_source_t* pPollActionSource = NULL;

    if (current_pos < 0 || NULL == pPollActionSourceList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "next_poll_action_source_get() exit---: Poll Action List Error \r\n");
        return NULL;
    }

    if (current_pos + 1 >= osip_list_size(pPollActionSourceList))
    {
        next_pos = 0;
    }
    else
    {
        next_pos = current_pos + 1;
    }

    /* 从当前位置开始往后查找 */
    for (i = next_pos; i < osip_list_size(pPollActionSourceList); i++)
    {
        pPollActionSource = (poll_action_source_t*)osip_list_get(pPollActionSourceList, i);

        if (NULL == pPollActionSource || '\0' == pPollActionSource->pcSourceID[0])
        {
            continue;
        }

        if (PLANACTION_TVWALL == pPollActionSource->iType)
        {
            return pPollActionSource;
        }
    }

    /* 如果从当前位置开始往后查找，没有找到，那么在从开始查找到当前位置 */
    for (i = 0; i < next_pos; i++)
    {
        pPollActionSource = (poll_action_source_t*)osip_list_get(pPollActionSourceList, i);

        if (NULL == pPollActionSource || '\0' == pPollActionSource->pcSourceID[0])
        {
            continue;
        }

        if (PLANACTION_TVWALL == pPollActionSource->iType)
        {
            return pPollActionSource;
        }
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : poll_action_init
 功能描述  : 轮巡动作结构初始化
 输入参数  : poll_action_t** poll_action
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月3日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int poll_action_init(poll_action_t** poll_action)
{
    *poll_action = (poll_action_t*)osip_malloc(sizeof(poll_action_t));

    if (*poll_action == NULL)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_init() exit---: *poll_srv Smalloc Error \r\n");
        return -1;
    }

    (*poll_action)->poll_id = 0;
    (*poll_action)->pcDestID[0] = '\0';
    (*poll_action)->del_mark = 0;
    (*poll_action)->current_pos = 0;

    (*poll_action)->pPollActionSourceList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == (*poll_action)->pPollActionSourceList)
    {
        osip_free(*poll_action);
        *poll_action = NULL;
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_init() exit---: Poll Action List Init Error \r\n");
        return -1;
    }

    osip_list_init((*poll_action)->pPollActionSourceList);

    return 0;
}

/*****************************************************************************
 函 数 名  : poll_action_free
 功能描述  : 轮巡动作结构释放
 输入参数  : poll_action_t* poll_action
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月3日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void poll_action_free(poll_action_t* poll_action)
{
    if (poll_action == NULL)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_free() exit---: Param Error \r\n");
        return;
    }

    poll_action->poll_id = 0;

    memset(poll_action->pcDestID, 0, MAX_ID_LEN + 4);

    poll_action->del_mark = 0;
    poll_action->current_pos = 0;

    if (NULL != poll_action->pPollActionSourceList)
    {
        osip_list_special_free(poll_action->pPollActionSourceList, (void (*)(void*))&poll_action_source_free);
        osip_free(poll_action->pPollActionSourceList);
        poll_action->pPollActionSourceList = NULL;
    }

    osip_free(poll_action);
    poll_action = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : poll_action_find
 功能描述  : 根据目的ID查找轮巡动作
 输入参数  : char* pcDestID
             osip_list_t* pPollActionList
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月3日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int poll_action_find(char* pcDestID, osip_list_t* pPollActionList)
{
    int i = 0;
    poll_action_t* pPollAction = NULL;

    if (NULL == pcDestID || NULL == pPollActionList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_find() exit---: Poll Action List Error \r\n");
        return -1;
    }

    for (i = 0; i < osip_list_size(pPollActionList); i++)
    {
        pPollAction = (poll_action_t*)osip_list_get(pPollActionList, i);

        if (NULL == pPollAction || '\0' == pPollAction->pcDestID[0])
        {
            continue;
        }

        if (0 == sstrcmp(pPollAction->pcDestID, pcDestID))
        {
            return i;
        }
    }

    return -1;
}

/*****************************************************************************
 函 数 名  : poll_action_get
 功能描述  : 轮巡动作获取
 输入参数  : int pos
                            osip_list_t* pPollActionList
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月3日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
poll_action_t* poll_action_get(int pos, osip_list_t* pPollActionList)
{
    poll_action_t* pPollAction = NULL;

    if (NULL == pPollActionList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_find() exit---: Poll Action List Error \r\n");
        return NULL;
    }

    if (pos < 0 || (pos >= osip_list_size(pPollActionList)))
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_find() exit---: Pos Error \r\n");
        return NULL;
    }

    pPollAction = (poll_action_t*)osip_list_get(pPollActionList, pos);

    if (NULL == pPollAction)
    {
        return NULL;
    }
    else
    {
        return pPollAction;
    }
}

/*****************************************************************************
 函 数 名  : poll_srv_init
 功能描述  : 轮巡业务结构初始化
 输入参数  : poll_srv_t ** poll_srv
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int poll_srv_init(poll_srv_t** poll_srv)
{
    *poll_srv = (poll_srv_t*)osip_malloc(sizeof(poll_srv_t));

    if (*poll_srv == NULL)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_init() exit---: *poll_srv Smalloc Error \r\n");
        return -1;
    }

    (*poll_srv)->poll_id = 0;
    (*poll_srv)->status = 0;
    (*poll_srv)->poll_name[0] = '\0';
    (*poll_srv)->start_time = 0;
    (*poll_srv)->duration_time = 0;
    (*poll_srv)->duration_time_count = 0;
    (*poll_srv)->del_mark = 0;
    (*poll_srv)->send_mark = 0;

    (*poll_srv)->pPollActionList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == (*poll_srv)->pPollActionList)
    {
        osip_free(*poll_srv);
        *poll_srv = NULL;
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_init() exit---: Poll Action List Init Error \r\n");
        return -1;
    }

    osip_list_init((*poll_srv)->pPollActionList);

    return 0;
}

/*****************************************************************************
 函 数 名  : poll_srv_free
 功能描述  : 轮巡业务结构释放
 输入参数  : poll_srv_t * poll_srv
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void poll_srv_free(poll_srv_t* poll_srv)
{
    if (poll_srv == NULL)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_free() exit---: Param Error \r\n");
        return;
    }

    poll_srv->poll_id = 0;
    poll_srv->status = 0;

    memset(poll_srv->poll_name, 0, MAX_128CHAR_STRING_LEN + 4);

    poll_srv->start_time = 0;
    poll_srv->duration_time = 0;
    poll_srv->duration_time_count = 0;
    poll_srv->del_mark = 0;
    poll_srv->send_mark = 0;

    if (NULL != poll_srv->pPollActionList)
    {
        osip_list_special_free(poll_srv->pPollActionList, (void (*)(void*))&poll_action_free);
        osip_free(poll_srv->pPollActionList);
        poll_srv->pPollActionList = NULL;
    }

    osip_free(poll_srv);
    poll_srv = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : poll_srv_find
 功能描述  : 根据ID查找轮巡业务
 输入参数  : unsigned int id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月17日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
int poll_srv_find(unsigned int id)
{
    int i = 0;
    poll_srv_t* pPollSrv = NULL;

    if (id <= 0 || NULL == g_PollSrvList || NULL == g_PollSrvList->pPollSrvList)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_find() exit---: Poll Action List Error \r\n");
        return -1;
    }

    POLL_SMUTEX_LOCK();

    for (i = 0; i < osip_list_size(g_PollSrvList->pPollSrvList); i++)
    {
        pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, i);

        if (NULL == pPollSrv || pPollSrv->poll_id < 0)
        {
            continue;
        }

        if (pPollSrv->poll_id == id)
        {
            POLL_SMUTEX_UNLOCK();
            return i;
        }
    }

    POLL_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : poll_srv_get
 功能描述  : 获取轮巡业务
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月17日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
poll_srv_t* poll_srv_get(int pos)
{
    poll_srv_t* pPollSrv = NULL;

    if (NULL == g_PollSrvList || NULL == g_PollSrvList->pPollSrvList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_get() exit---: Poll Action List Error \r\n");
        return NULL;
    }

    if (pos < 0 || (pos >= osip_list_size(g_PollSrvList->pPollSrvList)))
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_get() exit---: Pos Error \r\n");
        return NULL;
    }

    pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, pos);

    if (NULL == pPollSrv)
    {
        return NULL;
    }
    else
    {
        return pPollSrv;
    }
}

/*****************************************************************************
 函 数 名  : poll_srv_list_init
 功能描述  : 轮巡业务队列初始化
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
int poll_srv_list_init()
{
    g_PollSrvList = (poll_srv_list_t*)osip_malloc(sizeof(poll_srv_list_t));

    if (g_PollSrvList == NULL)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_list_init() exit---: g_PollSrvList Smalloc Error \r\n");
        return -1;
    }

    g_PollSrvList->pPollSrvList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_PollSrvList->pPollSrvList)
    {
        osip_free(g_PollSrvList);
        g_PollSrvList = NULL;
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_list_init() exit---: Poll Srv List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_PollSrvList->pPollSrvList);

#ifdef MULTI_THR
    /* init smutex */
    g_PollSrvList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_PollSrvList->lock)
    {
        osip_free(g_PollSrvList->pPollSrvList);
        g_PollSrvList->pPollSrvList = NULL;
        osip_free(g_PollSrvList);
        g_PollSrvList = NULL;
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_list_init() exit---: Poll Srv List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : poll_srv_list_free
 功能描述  : 轮巡业务队列释放
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
void poll_srv_list_free()
{
    if (NULL == g_PollSrvList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_PollSrvList->pPollSrvList)
    {
        osip_list_special_free(g_PollSrvList->pPollSrvList, (void (*)(void*))&poll_srv_free);
        osip_free(g_PollSrvList->pPollSrvList);
        g_PollSrvList->pPollSrvList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_PollSrvList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_PollSrvList->lock);
        g_PollSrvList->lock = NULL;
    }

#endif
    osip_free(g_PollSrvList);
    g_PollSrvList = NULL;
    return;
}

/*****************************************************************************
 函 数 名  : poll_srv_list_lock
 功能描述  : 轮巡业务队列锁定
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
int poll_srv_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PollSrvList == NULL || g_PollSrvList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_PollSrvList->lock);
#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : poll_srv_list_unlock
 功能描述  : 轮巡业务解锁
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
int poll_srv_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PollSrvList == NULL || g_PollSrvList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_PollSrvList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_poll_srv_list_lock
 功能描述  : 轮巡业务队列锁定
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
int debug_poll_srv_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PollSrvList == NULL || g_PollSrvList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "debug_poll_srv_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_PollSrvList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_poll_srv_list_unlock
 功能描述  : 轮巡业务解锁
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
int debug_poll_srv_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PollSrvList == NULL || g_PollSrvList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "debug_poll_srv_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_PollSrvList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : poll_srv_add
 功能描述  : 轮巡业务添加
 输入参数  : poll_srv_t* pPollSrv
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月3日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int poll_srv_add(poll_srv_t* pPollSrv)
{
    int i = 0;

    if (pPollSrv == NULL)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_add() exit---: Param Error \r\n");
        return -1;
    }

    POLL_SMUTEX_LOCK();

    i = osip_list_add(g_PollSrvList->pPollSrvList, pPollSrv, -1); /* add to list tail */

    //DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_srv_add() PollSrv:PollID=%u, StartTime=%d, DurationTime=%d, i=%d \r\n", pPollSrv->poll_id, pPollSrv->start_time, pPollSrv->duration_time, i);

    if (i < 0)
    {
        POLL_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_srv_add() exit---: List Add Error \r\n");
        return -1;
    }

    POLL_SMUTEX_UNLOCK();
    return i - 1;
}

/*****************************************************************************
 函 数 名  : poll_srv_remove
 功能描述  : 从队列中移除轮巡业务
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
int poll_srv_remove(int pos)
{
    poll_srv_t* pPollSrv = NULL;

    POLL_SMUTEX_LOCK();

    if (g_PollSrvList == NULL || pos < 0 || (pos >= osip_list_size(g_PollSrvList->pPollSrvList)))
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_srv_remove() exit---: Param Error \r\n");
        POLL_SMUTEX_UNLOCK();
        return -1;
    }

    pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, pos);

    if (NULL == pPollSrv)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_srv_remove() exit---: List Get Error \r\n");
        POLL_SMUTEX_UNLOCK();
        return -1;
    }

    osip_list_remove(g_PollSrvList->pPollSrvList, pos);
    poll_srv_free(pPollSrv);
    pPollSrv = NULL;
    POLL_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 函 数 名  : scan_poll_srv_list
 功能描述  : 扫描轮巡业务消息队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void scan_poll_srv_list(DBOper* pPoll_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    poll_srv_t* pPollSrv = NULL;
    needtoproc_pollsrv_queue needToProc;
    needtoproc_pollsrv_queue needToStop;
    needtoproc_pollsrv_queue needToSend;

    if ((NULL == g_PollSrvList) || (NULL == g_PollSrvList->pPollSrvList))
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "scan_poll_srv_list() exit---: Param Error \r\n");
        return;
    }

    needToProc.clear();
    needToStop.clear();
    needToSend.clear();

    POLL_SMUTEX_LOCK();

    if (osip_list_size(g_PollSrvList->pPollSrvList) <= 0)
    {
        POLL_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_PollSrvList->pPollSrvList); i++)
    {
        pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, i);

        if (NULL == pPollSrv)
        {
            continue;
        }

        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() poll_id=%u, PollSrv: StartTime=%d, DurationTime=%d, DurationTimeCount=%d, DelMark=%d, Status=%d \r\n", pPollSrv->poll_id, pPollSrv->start_time, pPollSrv->duration_time, pPollSrv->duration_time_count, pPollSrv->del_mark, pPollSrv->status);

        if (1 == pPollSrv->del_mark) /* 要删除的数据 */
        {
            if (1 == pPollSrv->status || 4 == pPollSrv->status)
            {
                needToStop.push_back(pPollSrv);
                pPollSrv->duration_time_count = 0;
            }
            else
            {
                continue;
            }
        }

        if (2 == pPollSrv->status) /* 需要启动轮巡 */
        {
            pPollSrv->status = 1;
            pPollSrv->duration_time_count = 0;
        }
        else if (3 == pPollSrv->status)  /* 需要停止轮巡 */
        {
            needToStop.push_back(pPollSrv);
            pPollSrv->duration_time_count = 0;
        }
        else if (4 == pPollSrv->status)  /* 需要发送通知给客户端 */
        {
            needToSend.push_back(pPollSrv);
            pPollSrv->status = 1;
        }

        if (1 == pPollSrv->status)  /* 看是否要停止轮巡 */
        {
            if (pPollSrv->duration_time > 0 && pPollSrv->duration_time_count > 0
                && pPollSrv->duration_time_count >= pPollSrv->duration_time)
            {
                needToStop.push_back(pPollSrv);
                pPollSrv->duration_time_count = 0;
            }
            else
            {
                needToProc.push_back(pPollSrv);

                if (pPollSrv->duration_time > 0)
                {
                    pPollSrv->duration_time_count++;
                }
            }
        }
    }

    POLL_SMUTEX_UNLOCK();

    /* 处理需要开始的 */
    while (!needToProc.empty())
    {
        pPollSrv = (poll_srv_t*) needToProc.front();
        needToProc.pop_front();

        if (NULL != pPollSrv)
        {
            iRet = poll_action_proc(pPollSrv->pPollActionList);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "scan_poll_srv_list() poll_action_proc Error:poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() poll_action_proc OK:poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
            }

            if (0 == pPollSrv->send_mark)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行轮巡发送PC屏幕开始轮巡:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Perform started round tour round tour to send PC screen", pPollSrv->poll_id, pPollSrv->poll_name);

                /* 通知客户端 */
                iRet = SendNotifyExecutePollActionToOnlineUser(pPollSrv->poll_id, pPollSrv->poll_name, 0, pPoll_Srv_dboper);

                if (iRet < 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "执行轮巡发送PC屏幕开始轮巡失败:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Perform started round tour round tour to send PC screen failure:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "scan_poll_srv_list() SendNotifyExecutePollActionToOnlineUser Error:  iRet=%d\r\n", iRet);
                }
                else if (iRet > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行轮巡发送PC屏幕开始轮巡成功:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Perform round tour started round tour successfully sent PC screen:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() SendNotifyExecutePollActionToOnlineUser OK: iRet=%d\r\n", iRet);
                }

                pPollSrv->send_mark = 1;

                /* 更新状态 */
                iRet = UpdatePollConfigStatus2DB(pPollSrv->poll_id, 1, pPoll_Srv_dboper);

                if (iRet < 0)
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "scan_poll_srv_list() UpdatePollConfigStatus2DB Start Error: poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() UpdatePollConfigStatus2DB Start OK: poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
                }
            }
        }
    }

    needToProc.clear();

    /* 处理需要发送的 */
    while (!needToSend.empty())
    {
        pPollSrv = (poll_srv_t*) needToSend.front();
        needToSend.pop_front();

        if (NULL != pPollSrv)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行轮巡发送PC屏幕开始轮巡:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Perform started round tour round tour to send PC screen:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);

            /* 通知客户端 */
            iRet = SendNotifyExecutePollActionToOnlineUser(pPollSrv->poll_id, pPollSrv->poll_name, 0, pPoll_Srv_dboper);

            if (iRet < 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "执行轮巡发送PC屏幕开始轮巡失败:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Perform started round tour round tour to send PC screen failure:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "scan_poll_srv_list() SendNotifyExecutePollActionToOnlineUser Error: iRet=%d\r\n", iRet);
            }
            else if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行轮巡发送PC屏幕开始轮巡成功:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Perform round tour started round tour successfully sent PC screen:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() SendNotifyExecutePollActionToOnlineUser OK: iRet=%d\r\n", iRet);
            }
        }
    }

    needToSend.clear();

    /* 处理需要停止的 */
    while (!needToStop.empty())
    {
        pPollSrv = (poll_srv_t*) needToStop.front();
        needToStop.pop_front();

        if (NULL != pPollSrv)
        {
            iRet = poll_action_stop(pPollSrv->pPollActionList);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "scan_poll_srv_list() poll_action_stop Error:poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() poll_action_stop OK:poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
            }

            if (iRet >= 0)
            {
                pPollSrv->status = 0;
                pPollSrv->duration_time_count = 0;

#if 1 /* 纯粹的电视墙轮巡，需要通知客户端 */

                if (iRet > 0) /* 纯粹的电视墙轮巡，需要通知客户端 */
                {
                    if (1 == pPollSrv->send_mark) /* 用户手动停止的不需要发送给客户端 */
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行轮巡发送PC屏幕停止轮巡:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyExecutePollActionStopPCPollAction:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);

                        /* 通知客户端 */
                        iRet = SendNotifyExecutePollActionToOnlineUser(pPollSrv->poll_id, pPollSrv->poll_name, 1, pPoll_Srv_dboper);

                        if (iRet < 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "执行轮巡发送PC屏幕停止轮巡失败:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "SendNotifyExecutePollActionStopPCPollAction Error:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "scan_poll_srv_list() SendNotifyExecutePollActionToOnlineUser Error: iRet=%d\r\n", iRet);
                        }
                        else if (iRet > 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行轮巡发送PC屏幕停止轮巡成功:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyExecutePollActionStopPCPollAction ok:PollID=%u, PollName=%s", pPollSrv->poll_id, pPollSrv->poll_name);
                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() SendNotifyExecutePollActionToOnlineUser OK: iRet=%d\r\n", iRet);
                        }
                    }
                }

#endif
                /* 更新状态 */
                iRet = UpdatePollConfigStatus2DB(pPollSrv->poll_id, 0, pPoll_Srv_dboper);

                if (iRet < 0)
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "scan_poll_srv_list() UpdatePollConfigStatus2DB Start Error: poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "scan_poll_srv_list() UpdatePollConfigStatus2DB Start OK: poll_id=%u, iRet=%d \r\n", pPollSrv->poll_id, iRet);
                }

                pPollSrv->send_mark = 0;
            }
        }
    }

    needToStop.clear();

    return;
}
#endif

/*****************************************************************************
 函 数 名  : poll_action_stop
 功能描述  : 停止预案动作
 输入参数  : osip_list_t* pPollActionList
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int poll_action_stop(osip_list_t* pPollActionList)
{
    //int iRet = 0;
    int i = 0, j = 0;
    poll_action_t* pPollAction = NULL;
    poll_action_source_t* pPollActionSource = NULL;
    int iNotifyUserFlag = 1;

    if (NULL == pPollActionList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_stop() exit---: Poll Action List Error \r\n");
        return -1;
    }

    for (i = 0; i < osip_list_size(pPollActionList); i++)
    {
        pPollAction = (poll_action_t*)osip_list_get(pPollActionList, i);

        if (NULL == pPollAction)
        {
            continue;
        }

        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_stop() PollAction: PollID=%u, DestID=%s, CurrentPos=%d \r\n", pPollAction->poll_id, pPollAction->pcDestID, pPollAction->current_pos);

        for (j = 0; j < osip_list_size(pPollAction->pPollActionSourceList); j++)
        {
            pPollActionSource = (poll_action_source_t*)osip_list_get(pPollAction->pPollActionSourceList, j);

            if (NULL == pPollActionSource)
            {
                continue;
            }

            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_stop() PollActionSource: Type=%d, SourceID=%s, LiveTime=%d, LiveTimeCount=%d, Status=%d \r\n", pPollActionSource->iType, pPollActionSource->pcSourceID, pPollActionSource->iLiveTime, pPollActionSource->iLiveTimeCount, pPollActionSource->iStatus);

            if (PLANACTION_PC == pPollActionSource->iType)
            {
                iNotifyUserFlag = 0; /* 有PC屏幕轮巡，不需要通知客户端 */
            }

            if (1 == pPollActionSource->iStatus)
            {
#if 0 /* 停止电视墙轮巡的时候，不能停止码流 */

                iRet = StopDecService(pPollActionSource->pcSourceID, pPollAction->pcDestID);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_stop() StopDecService Error: DestID=%s, SourceID=%s, iRet=%d\r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_stop() StopDecService OK: DestID=%s, SourceID=%s, iRet=%d\r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, iRet);
                }

#endif
                pPollActionSource->iStatus = 0;
                pPollActionSource->iLiveTimeCount = 0;
                //iNotifyUserFlag = 1;
            }
        }

        pPollAction->current_pos = 0;
    }

    return iNotifyUserFlag;
}

/*****************************************************************************
 函 数 名  : poll_action_proc
 功能描述  : 执行预案动作
 输入参数  : osip_list_t* pPollActionList
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int poll_action_proc(osip_list_t* pPollActionList)
{
    int iRet = 0;
    int i = 0, j = 0;
    poll_action_t* pPollAction = NULL;
    poll_action_source_t* pPollActionSource = NULL;
    poll_action_source_t* pNextPollActionSource = NULL;
    GBLogicDevice_info_t* pDestGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pDestGBDeviceInfo = NULL;
    int iNotifyUserFlag = 0;

    if (NULL == pPollActionList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "poll_action_proc() exit---: Poll Action List Error \r\n");
        return -1;
    }

    for (i = 0; i < osip_list_size(pPollActionList); i++)
    {
        pPollAction = (poll_action_t*)osip_list_get(pPollActionList, i);

        if (NULL == pPollAction)
        {
            continue;
        }

        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() PollAction: PollID=%u, DestID=%s, CurrentPos=%d, del_mark=%d \r\n", pPollAction->poll_id, pPollAction->pcDestID, pPollAction->current_pos, pPollAction->del_mark);

        if (1 == pPollAction->del_mark) /* 需要删除的，看是否需要停止 */
        {
            for (j = 0; j < osip_list_size(pPollAction->pPollActionSourceList); j++)
            {
                pPollActionSource = (poll_action_source_t*)osip_list_get(pPollAction->pPollActionSourceList, j);

                if (NULL != pPollActionSource && j == pPollAction->current_pos)
                {
                    if (1 == pPollActionSource->iStatus)
                    {
                        iRet = StopDecService(pPollActionSource->pcSourceID, pPollAction->pcDestID);

                        if (0 != iRet)
                        {
                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() StopDecService Error: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() StopDecService OK: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                        }

                        pPollActionSource->iStatus = 0;
                        iNotifyUserFlag = 2;
                    }
                }
            }
        }
        else
        {
            for (j = 0; j < osip_list_size(pPollAction->pPollActionSourceList); j++)
            {
                pPollActionSource = (poll_action_source_t*)osip_list_get(pPollAction->pPollActionSourceList, j);

                if (NULL == pPollActionSource)
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() Get PollActionSource Error\r\n");
                    pPollAction->current_pos++;
                    continue;
                }

                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() PollActionSource: Type=%d, SourceID=%s, LiveTime=%d, LiveTimeCount=%d, Status=%d, del_mark=%d \r\n", pPollActionSource->iType, pPollActionSource->pcSourceID, pPollActionSource->iLiveTime, pPollActionSource->iLiveTimeCount, pPollActionSource->iStatus, pPollActionSource->del_mark);

                if (PLANACTION_TVWALL == pPollActionSource->iType) /* 电视墙轮巡 */
                {
                    /* 获取目的端的设备信息 */
                    pDestGBLogicDeviceInfo = GBLogicDevice_info_find(pPollAction->pcDestID);

                    if (NULL == pDestGBLogicDeviceInfo)
                    {
                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() Get DestGBLogicDeviceInfo Error\r\n");
                        pPollAction->current_pos++;
                        continue;
                    }

                    pDestGBDeviceInfo = GBDevice_info_get_by_stream_type(pDestGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

                    if (NULL == pDestGBDeviceInfo || EV9000_DEVICETYPE_DECODER != pDestGBDeviceInfo->device_type) /* 只有电视墙才操作 */
                    {
                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() DestGBDeviceInfo OR DestGBDeviceInfo Ddevice Type Error, device_id=%s, device_type=%d\r\n", pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->device_type);
                        pPollAction->current_pos++;
                        continue;
                    }

                    if (j == pPollAction->current_pos) /* 正在当前轮巡 */
                    {
                        if (1 == pPollActionSource->del_mark) /* 需要删除的，当前正在轮巡的，删除了的，需要停止掉 */
                        {
                            if (1 == pPollActionSource->iStatus)
                            {
                                pPollActionSource->iLiveTimeCount = 0;
                                pPollAction->current_pos = j + 1;

                                /* 看看下一个轮巡的源ID是否和本次的一样，如果一样，就不需要关闭 */
                                pNextPollActionSource = next_poll_action_source_get(j, pPollAction->pPollActionSourceList);

                                if (NULL != pNextPollActionSource)
                                {
                                    if ((pNextPollActionSource->iSourceStreamType == pPollActionSource->iSourceStreamType)
                                        && (0 == sstrcmp(pNextPollActionSource->pcSourceID, pPollActionSource->pcSourceID)))
                                    {
                                        pNextPollActionSource->iConnectFlag = 1;
                                    }
                                    else
                                    {
                                        iRet = StopDecService(pPollActionSource->pcSourceID, pPollAction->pcDestID);

                                        if (0 != iRet)
                                        {
                                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() StopDecService Error: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                        }
                                        else
                                        {
                                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() StopDecService OK: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                        }
                                    }
                                }
                                else
                                {
                                    iRet = StopDecService(pPollActionSource->pcSourceID, pPollAction->pcDestID);

                                    if (0 != iRet)
                                    {
                                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() StopDecService Error: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() StopDecService OK: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                    }
                                }

                                pPollActionSource->iStatus = 0;
                                pPollActionSource->iConnectFlag = 0;
                                iNotifyUserFlag = 2;
                            }
                        }
                        else
                        {
                            if (0 == pPollActionSource->iStatus)
                            {
                                if (0 == pPollActionSource->iConnectFlag)
                                {
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行轮巡打开视墙:电视墙通道ID=%s, 逻辑设备ID=%s", pPollAction->pcDestID, pPollActionSource->pcSourceID);
                                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run polling open TV wall: TV wall channel ID=%s, logic device ID=%s", pPollAction->pcDestID, pPollActionSource->pcSourceID);

                                    if (pPollActionSource->iSourceStreamType <= 0)
                                    {
                                        iRet = start_connect_tv_proc(pPollActionSource->pcSourceID, EV9000_STREAM_TYPE_MASTER, pPollAction->pcDestID, 0);
                                    }
                                    else
                                    {
                                        iRet = start_connect_tv_proc(pPollActionSource->pcSourceID, pPollActionSource->iSourceStreamType, pPollAction->pcDestID, 0);
                                    }

                                    if (iRet < 0)
                                    {
                                        pPollActionSource->iConnectFlag = 0;
                                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() start_connect_tv_proc Error: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d\r\n", pDestGBLogicDeviceInfo->device_id, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                    }
                                    else
                                    {
                                        pPollActionSource->iConnectFlag = 1;
                                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "poll_action_proc() start_connect_tv_proc OK: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d\r\n", pDestGBLogicDeviceInfo->device_id, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                    }
                                }

                                pPollActionSource->iStatus = 1;
                                iNotifyUserFlag = 1;
                            }
                            else
                            {
                                if (pPollActionSource->iConnectFlag == 0) /* 如果没有连接成功，则一直连接 */
                                {
                                    if (pPollActionSource->iConnectCount >= 10)
                                    {
                                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行轮巡打开视墙:电视墙通道ID=%s, 逻辑设备ID=%s", pPollAction->pcDestID, pPollActionSource->pcSourceID);
                                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run polling open TV wall: TV wall channelID=%s, logic device ID=%s", pPollAction->pcDestID, pPollActionSource->pcSourceID);

                                        if (pPollActionSource->iSourceStreamType <= 0)
                                        {
                                            iRet = start_connect_tv_proc(pPollActionSource->pcSourceID, EV9000_STREAM_TYPE_MASTER, pPollAction->pcDestID, 0);
                                        }
                                        else
                                        {
                                            iRet = start_connect_tv_proc(pPollActionSource->pcSourceID, pPollActionSource->iSourceStreamType, pPollAction->pcDestID, 0);
                                        }

                                        if (iRet < 0)
                                        {
                                            pPollActionSource->iConnectFlag = 0;
                                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() start_connect_tv_proc Error: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d\r\n", pDestGBLogicDeviceInfo->device_id, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                        }
                                        else
                                        {
                                            pPollActionSource->iConnectFlag = 1;
                                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "poll_action_proc() start_connect_tv_proc OK: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d\r\n", pDestGBLogicDeviceInfo->device_id, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                        }

                                        pPollActionSource->iConnectCount = 0;
                                    }
                                    else
                                    {
                                        pPollActionSource->iConnectCount++;
                                    }
                                }

                                pPollActionSource->iLiveTimeCount++;
                                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() DestID=%s, SourceID=%s, LiveTimeCount=%d, CurrentPos=%d\r\n", pDestGBLogicDeviceInfo->device_id, pPollActionSource->pcSourceID, pPollActionSource->iLiveTimeCount, pPollAction->current_pos);
                            }

                            if (pPollActionSource->iLiveTimeCount >= pPollActionSource->iLiveTime)
                            {
                                pPollActionSource->iLiveTimeCount = 0;
                                pPollAction->current_pos = j + 1;

                                /* 看看下一个轮巡的源ID是否和本次的一样，如果一样，就不需要关闭 */
                                pNextPollActionSource = next_poll_action_source_get(j, pPollAction->pPollActionSourceList);

                                if (NULL != pNextPollActionSource)
                                {
                                    if ((pNextPollActionSource->iSourceStreamType == pPollActionSource->iSourceStreamType)
                                        && (0 == sstrcmp(pNextPollActionSource->pcSourceID, pPollActionSource->pcSourceID)))
                                    {
                                        pNextPollActionSource->iConnectFlag = 1;
                                    }
                                    else
                                    {
                                        iRet = StopDecService(pPollActionSource->pcSourceID, pPollAction->pcDestID);

                                        if (0 != iRet)
                                        {
                                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() StopDecService Error: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                        }
                                        else
                                        {
                                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() StopDecService OK: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                        }
                                    }
                                }
                                else
                                {
                                    iRet = StopDecService(pPollActionSource->pcSourceID, pPollAction->pcDestID);

                                    if (0 != iRet)
                                    {
                                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "poll_action_proc() StopDecService Error: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() StopDecService OK: DestID=%s, SourceID=%s, CurrentPos=%d, iRet=%d \r\n", pPollAction->pcDestID, pPollActionSource->pcSourceID, pPollAction->current_pos, iRet);
                                    }
                                }

                                pPollActionSource->iStatus = 0;
                                pPollActionSource->iConnectFlag = 0;
                                iNotifyUserFlag = 2;
                            }
                        }
                    }
                }
                else if (PLANACTION_PC == pPollActionSource->iType) /* PC屏幕轮巡,跳过 */
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() PollActionSource Type is PC Poll\r\n");
                    pPollAction->current_pos++;
                    iNotifyUserFlag = 1;
                    continue;
                }
                else
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() PollActionSource Type is Unknow:Type=%d\r\n", pPollActionSource->iType);
                    pPollAction->current_pos++;

                    continue;
                }

                if (pPollAction->current_pos >= osip_list_size(pPollAction->pPollActionSourceList))
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "poll_action_proc() CurrentPos=%d \r\n", pPollAction->current_pos, iRet);
                    pPollAction->current_pos = 0;
                }
            }
        }
    }

    return iNotifyUserFlag;
}

/*****************************************************************************
 函 数 名  : start_poll_srv_by_id
 功能描述  : 根据ID启动轮巡任务
 输入参数  : user_info_t* pUserInfo
             int id
             DBOper* pPoll_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int start_poll_srv_by_id(user_info_t* pUserInfo, unsigned int id, DBOper* pPoll_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    int poll_pos = -1;
    string strSQL = "";
    int record_count = 0;
    poll_srv_t* pPollSrv = NULL;
    char strPollID[32] = {0};

    if (NULL == pPoll_Srv_dboper || NULL == pUserInfo)
    {
        return -1;
    }

    poll_pos = poll_srv_find(id);

    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "start_poll_srv_by_id() poll_srv_find:id=%d, poll_pos=%d \r\n", id, poll_pos);

    if (poll_pos < 0)
    {
        snprintf(strPollID, 32, "%u", id);
        strSQL.clear();
        strSQL = "select * from PollConfig WHERE ID = ";
        strSQL += strPollID;

        record_count = pPoll_Srv_dboper->DB_Select(strSQL.c_str(), 1);

        if (record_count < 0)
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() ErrorMsg=%s\r\n", pPoll_Srv_dboper->GetLastDbErrorMsg());
            return -1;
        }
        else if (record_count == 0)
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "start_poll_srv_by_id() exit---: No Record Count \r\n");
            return 0;
        }


        unsigned int uPollID = 0;
        string strPollName = "";
        int iStartTime = 0;
        int iDurationTime = 0;

        pPoll_Srv_dboper->GetFieldValue("ID", uPollID);
        pPoll_Srv_dboper->GetFieldValue("PollName", strPollName);
        pPoll_Srv_dboper->GetFieldValue("StartTime", iStartTime);
        pPoll_Srv_dboper->GetFieldValue("DurationTime", iDurationTime);

        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "start_poll_srv_by_id() PollID=%u, PollName=%s, StartTime=%d, DurationTime=%d \r\n", uPollID, strPollName.c_str(), iStartTime, iDurationTime);

        i = poll_srv_init(&pPollSrv);

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() poll_srv_init:i=%d \r\n", i);
            return -1;
        }

        pPollSrv->poll_id = uPollID;

        if (!strPollName.empty())
        {
            osip_strncpy(pPollSrv->poll_name, (char*)strPollName.c_str(), MAX_128CHAR_STRING_LEN);
        }

        pPollSrv->start_time = iStartTime;
        pPollSrv->duration_time = iDurationTime;
        pPollSrv->status = 2;

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加手动触发执行的轮巡: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add a manually triggered polling: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);

        /* 添加动作数据到队列 */
        i = add_poll_action_dest_data_to_srv_list_proc(pPollSrv->poll_id, pPollSrv->pPollActionList, pPoll_Srv_dboper);

        if (i < 0)
        {
            poll_srv_free(pPollSrv);
            pPollSrv = NULL;
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() add_poll_action_data_to_srv_list_proc:i=%d \r\n", i);
            return -1;
        }

        /* 添加到队列 */
        if (poll_srv_add(pPollSrv) < 0)
        {
            poll_srv_free(pPollSrv);
            pPollSrv = NULL;
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() Poll Srv Add Error");
            return -1;
        }

        /* 通知客户端 */
        if (0 == pPollSrv->send_mark)
        {

#if 0 /* 用户手动执行的轮巡，不发给用户 */

            iRet = SendNotifyExecutePollActionToExceptOnlineUser(pUserInfo, pPollSrv->poll_id, 0);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() SendNotifyExecutePollActionToExceptOnlineUser Start Error: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "start_poll_srv_by_id() SendNotifyExecutePollActionToExceptOnlineUser Start OK: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
            }

#endif
            pPollSrv->send_mark = 1;

            /* 更新状态 */
            iRet = UpdatePollConfigStatus2DB(pPollSrv->poll_id, 1, pPoll_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() UpdatePollConfigStatus2DB Start Error: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "start_poll_srv_by_id() UpdatePollConfigStatus2DB Start OK: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
            }
        }
    }
    else
    {
        pPollSrv = poll_srv_get(poll_pos);

        if (NULL != pPollSrv)
        {
            if (0 == pPollSrv->status || 3 == pPollSrv->status)
            {
                pPollSrv->status = 2;
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加手动触发执行的轮巡: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add a manually triggered polling: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);

                /* 通知客户端 */
                if (0 == pPollSrv->send_mark)
                {

#if 0 /* 用户手动执行的轮巡，不发给用户 */

                    iRet = SendNotifyExecutePollActionToExceptOnlineUser(pUserInfo, pPollSrv->poll_id, 0);

                    if (0 != iRet)
                    {
                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() SendNotifyExecutePollActionToExceptOnlineUser Start Error: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "start_poll_srv_by_id() SendNotifyExecutePollActionToExceptOnlineUser Start OK: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
                    }

#endif
                    pPollSrv->send_mark = 1;

                    /* 更新状态 */
                    iRet = UpdatePollConfigStatus2DB(pPollSrv->poll_id, 1, pPoll_Srv_dboper);

                    if (iRet < 0)
                    {
                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() UpdatePollConfigStatus2DB Start Error: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "start_poll_srv_by_id() UpdatePollConfigStatus2DB Start OK: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
                    }
                }

                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "start_poll_srv_by_id() PollID=%u, PollName=%s, StartTime=%d, DurationTime=%d \r\n", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
            }
            else if (1 == pPollSrv->status) /* 再次发送一下 */
            {

#if 0 /* 用户手动执行的轮巡，不发给用户 */

                iRet = SendNotifyExecutePollActionToExceptOnlineUser(pUserInfo, pPollSrv->poll_id, 0);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() SendNotifyExecutePollActionToExceptOnlineUser Start Error: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "start_poll_srv_by_id() SendNotifyExecutePollActionToExceptOnlineUser Start OK: poll_id=%u, iRet=%d", pPollSrv->poll_id, iRet);
                }

#endif
            }
            else
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() PollID=%u, status=%d \r\n", pPollSrv->poll_id, pPollSrv->status);
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "start_poll_srv_by_id() poll_srv_get Error:poll_pos=%d \r\n", poll_pos);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : stop_poll_srv_by_id
 功能描述  : 根据ID停止轮巡任务
 输入参数  : int id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年10月17日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
int stop_poll_srv_by_id(unsigned int id)
{
    //int iRet = -1;
    int pos = -1;
    poll_srv_t* pPollSrv = NULL;

    pos = poll_srv_find(id);

    if (pos >= 0)
    {
        pPollSrv = poll_srv_get(pos);

        if (NULL != pPollSrv)
        {
            if (1 == pPollSrv->status || 4 == pPollSrv->status)
            {
                pPollSrv->status = 3;
                pPollSrv->send_mark = 0;
            }
            else if (2 == pPollSrv->status)
            {
                pPollSrv->status = 0;

#if 0 /* 停止轮巡不需要通知客户端 */

                /* 通知客户端 */
                iRet = SendNotifyExecutePollActionToOnlineUser(pPollSrv->poll_id, 1);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "stop_poll_srv_by_id() SendNotifyExecutePollActionToOnlineUser Error: poll_id=%u, iRet=%d\r\n", pPollSrv->poll_id, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "stop_poll_srv_by_id() SendNotifyExecutePollActionToOnlineUser OK: poll_id=%u, iRet=%d\r\n", pPollSrv->poll_id, iRet);
                }

#endif
                pPollSrv->send_mark = 0;
            }
            else
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "stop_poll_srv_by_id() poll_id=%u, status=%d\r\n", pPollSrv->poll_id, pPollSrv->status);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : set_poll_srv_list_del_mark
 功能描述  : 设置轮巡业务删除标识
 输入参数  : int del_mark
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年9月4日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int set_poll_srv_list_del_mark(int del_mark)
{
    int pos1 = 0;
    int pos2 = 0;
    int pos3 = 0;
    poll_srv_t* pPollSrv = NULL;
    poll_action_t* pPollAction = NULL;
    poll_action_source_t* pPollActionSource = NULL;

    if ((NULL == g_PollSrvList) || (NULL == g_PollSrvList->pPollSrvList))
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "set_poll_srv_list_del_mark() exit---: Param Error \r\n");
        return -1;
    }

    POLL_SMUTEX_LOCK();

    if (osip_list_size(g_PollSrvList->pPollSrvList) <= 0)
    {
        POLL_SMUTEX_UNLOCK();
        return -1;
    }

    for (pos1 = 0; pos1 < osip_list_size(g_PollSrvList->pPollSrvList); pos1++)
    {
        pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, pos1);

        if (NULL == pPollSrv)
        {
            continue;
        }

        pPollSrv->del_mark = del_mark;

        for (pos2 = 0; pos2 < osip_list_size(pPollSrv->pPollActionList); pos2++)
        {
            pPollAction = (poll_action_t*)osip_list_get(pPollSrv->pPollActionList, pos2);

            if (NULL == pPollAction)
            {
                continue;
            }

            pPollAction->del_mark = del_mark;

            for (pos3 = 0; pos3 < osip_list_size(pPollAction->pPollActionSourceList); pos3++)
            {
                pPollActionSource = (poll_action_source_t*)osip_list_get(pPollAction->pPollActionSourceList, pos3);

                if (NULL == pPollActionSource)
                {
                    continue;
                }

                pPollActionSource->del_mark = del_mark;
            }
        }
    }

    POLL_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : check_db_data_to_poll_srv_list
 功能描述  : 从数据中检测是否有需要执行的轮巡数据，如果有，则加载到内存中
 输入参数  : DBOper* pPoll_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int check_db_data_to_poll_srv_list(DBOper* pPoll_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    string strSQL = "";
    int record_count = 0;
    time_t now = time(NULL);
    int iTimeNow = 0;
    struct tm tp = {0};
    int while_count = 0;
    poll_srv_t* pPollSrv2 = NULL;

    if (NULL == pPoll_Srv_dboper)
    {
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from PollConfig order by StartTime asc";

    record_count = pPoll_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "check_db_data_to_poll_srv_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "check_db_data_to_poll_srv_list() ErrorMsg=%s\r\n", pPoll_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "check_db_data_to_poll_srv_list_for_start() exit---: No Record Count \r\n");
        return 0;
    }

    localtime_r(&now, &tp);
    iTimeNow = tp.tm_hour * 3600 + tp.tm_min * 60 + tp.tm_sec;

    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "check_db_data_to_poll_srv_list:record_count=%d \r\n", record_count);

    do
    {
        int i = 0;
        unsigned int uPollID = 0;
        string strPollName = "";
        int iOldStartTime = 0;
        int iStartTime = 0;
        int iDurationTime = 0;
        int iScheduledRun = 0;
        int poll_pos = -1;
        int iResved1 = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "check_db_data_to_poll_srv_list() While Count=%d \r\n", while_count);
        }

        pPoll_Srv_dboper->GetFieldValue("ID", uPollID);
        pPoll_Srv_dboper->GetFieldValue("PollName", strPollName);
        pPoll_Srv_dboper->GetFieldValue("StartTime", iStartTime);
        pPoll_Srv_dboper->GetFieldValue("DurationTime", iDurationTime);
        pPoll_Srv_dboper->GetFieldValue("ScheduledRun", iScheduledRun);
        pPoll_Srv_dboper->GetFieldValue("Resved1", iResved1);

        /* 查找队列，看队列里面是否已经存在 */
        poll_pos = poll_srv_find(uPollID);

        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "check_db_data_to_poll_srv_list() PollID=%u:poll_pos=%d \r\n", uPollID, poll_pos);

        if (poll_pos < 0) /* 添加到要执行队列 */
        {
            poll_srv_t* pPollSrv = NULL;

            i = poll_srv_init(&pPollSrv);

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "check_db_data_to_poll_srv_list() poll_srv_init:i=%d \r\n", i);
                continue;
            }

            pPollSrv->poll_id = uPollID;

            if (!strPollName.empty())
            {
                osip_strncpy(pPollSrv->poll_name, (char*)strPollName.c_str(), MAX_128CHAR_STRING_LEN);
            }

            pPollSrv->start_time = iStartTime;
            pPollSrv->duration_time = iDurationTime;
            pPollSrv->del_mark = 0;

            if ((iTimeNow == iStartTime) || (iTimeNow > iStartTime && iTimeNow - iStartTime < 30)) /* 30秒之内的才启动 */
            {
                if (iScheduledRun)
                {
                    pPollSrv->status = 2;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加定时促发执行的轮巡: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add time triggered polling: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);

                }
            }
            else if (0 == iStartTime && 0 == iDurationTime && 1 == iResved1) /* 手动执行的，状态是1的需要启动 */
            {
                if (!iScheduledRun)
                {
                    pPollSrv->status = 2;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加手动促发执行的但是没有停止的轮巡: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add a manually triggered but not stopped polling: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
                }
            }

            /* 添加到队列 */
            if (poll_srv_add(pPollSrv) < 0)
            {
                poll_srv_free(pPollSrv);
                pPollSrv = NULL;
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "check_db_data_to_poll_srv_list() Poll Srv Add Error\r\n");
                continue;
            }
            else
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "check_db_data_to_poll_srv_list() poll_srv_add:poll_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pPollSrv->poll_id, iStartTime, iDurationTime, pPollSrv->status);
            }
        }
        else
        {
            poll_srv_t* pPollSrv = NULL;

            pPollSrv = poll_srv_get(poll_pos);

            if (NULL != pPollSrv)
            {
                /* 看数据是否有变化 */
                if (!strPollName.empty())
                {
                    if (0 != sstrcmp(pPollSrv->poll_name, (char*)strPollName.c_str()))
                    {
                        memset(pPollSrv->poll_name, 0, MAX_128CHAR_STRING_LEN + 4);
                        osip_strncpy(pPollSrv->poll_name, (char*)strPollName.c_str(), MAX_128CHAR_STRING_LEN);
                    }
                }
                else
                {
                    memset(pPollSrv->poll_name, 0, MAX_128CHAR_STRING_LEN + 4);
                }

                iOldStartTime = pPollSrv->start_time;
                pPollSrv->start_time = iStartTime;
                pPollSrv->duration_time = iDurationTime;
                pPollSrv->del_mark = 0;

                if (0 == pPollSrv->status || 3 == pPollSrv->status)
                {
                    if ((iTimeNow == iStartTime) || (iTimeNow > iStartTime && iTimeNow - iStartTime < 30)) /* 30秒之内的才启动 */
                    {
                        if (iScheduledRun)
                        {
                            pPollSrv->status = 2;
                            //DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "check_db_data_to_poll_srv_list() poll_srv_add:poll_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pPollSrv->poll_id, iStartTime, iDurationTime, pPollSrv->status);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加定时促发执行的轮巡: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add time triggered polling: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);

                        }
                    }
                    else if (0 == iStartTime && 0 == iDurationTime && 1 == iResved1) /* 手动执行的，状态是1的需要启动 */
                    {
                        if (!iScheduledRun && 0 == pPollSrv->status) /* 3的状态是用户手动停止了，可能还没来得及入库，不能再次启动 */
                        {
                            pPollSrv->status = 2;
                            //DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "check_db_data_to_poll_srv_list() poll_srv_add:poll_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pPollSrv->poll_id, iStartTime, iDurationTime, pPollSrv->status);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加手动促发执行的但是没有停止的轮巡: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add a manually triggered but not stopped polling: poll_id=%d, poll_name=%s, start_time=%d, duration_time=%d", pPollSrv->poll_id, pPollSrv->poll_name, pPollSrv->start_time, pPollSrv->duration_time);

                        }
                    }
                }
                else if (1 == pPollSrv->status) /* 可能修改了启动时间，再发送一下 */
                {
                    if ((iStartTime != iOldStartTime) && ((iTimeNow == iStartTime) || (iTimeNow > iStartTime && iTimeNow - iStartTime < 30))) /* 30秒之内的才启动 */
                    {
                        if (iScheduledRun)
                        {
                            pPollSrv->status = 4;
                            DEBUG_TRACE(MODULE_POLL_SRV, LOG_INFO, "check_db_data_to_poll_srv_list() poll_srv_add:poll_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pPollSrv->poll_id, iStartTime, iDurationTime, pPollSrv->status);
                        }
                    }
                }
            }
        }
    }
    while (pPoll_Srv_dboper->MoveNext() >= 0);

    /* 添加目的数据和源数据 */
    for (i = 0; i < osip_list_size(g_PollSrvList->pPollSrvList); i++)
    {
        pPollSrv2 = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, i);

        if (NULL == pPollSrv2)
        {
            continue;
        }

        /* 添加动作数据到队列 */
        iRet = add_poll_action_dest_data_to_srv_list_proc(pPollSrv2->poll_id, pPollSrv2->pPollActionList, pPoll_Srv_dboper);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "check_db_data_to_poll_srv_list() add_poll_action_data_to_srv_list_proc Error:poll_id=%u \r\n", pPollSrv2->poll_id);
        }
        else
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "check_db_data_to_poll_srv_list() add_poll_action_data_to_srv_list_proc:poll_id=%u, iRet=%d \r\n", pPollSrv2->poll_id, iRet);
        }
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : add_poll_action_dest_data_to_srv_list_proc
 功能描述  : 添加轮巡动作的目的数据到轮巡业务队列
 输入参数  : int poll_id
                            osip_list_t* pPollActionList
                            DBOper* pPoll_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int add_poll_action_dest_data_to_srv_list_proc(unsigned int poll_id, osip_list_t* pPollActionList, DBOper* pPoll_Srv_dboper)
{
    int i = 0;
    int index = 0;
    int pos = -1;
    int iRet = 0;
    int record_count = 0;
    char strPollId[32] = {0};

    string strSQL = "";
    int while_count = 0;

    vector<string> strDestDeviceIDVector;

    if (NULL == pPoll_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "add_poll_action_dest_data_to_srv_list_proc() exit---: Poll Srv DB Oper Error \r\n");
        return -1;
    }

    if (NULL == pPollActionList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "add_poll_action_dest_data_to_srv_list_proc() exit---: Poll Action List Error \r\n");
        return -1;
    }

    /* 根据poll_id，查询轮巡动作表，获取轮巡的具体数据 */
    strSQL.clear();
    snprintf(strPollId, 32, "%u", poll_id);
    strSQL = "select DISTINCT DestID from PollActionConfig WHERE PollID = ";
    strSQL += strPollId;
    strSQL += " order by DestSortID asc";

    record_count = pPoll_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "add_poll_action_dest_data_to_srv_list_proc:PollID=%u, record_count=%d \r\n", poll_id, record_count);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_dest_data_to_srv_list_proc() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_dest_data_to_srv_list_proc() ErrorMsg=%s\r\n", pPoll_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "add_poll_action_dest_data_to_srv_list_proc() exit---: No Record Count \r\n");
        return 0;
    }

    strDestDeviceIDVector.clear();

    /* 循环添加轮巡动作数据 */
    do
    {
        string strDestID = "";

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "add_poll_action_dest_data_to_srv_list_proc() While Count=%d \r\n", while_count);
        }

        pPoll_Srv_dboper->GetFieldValue("DestID", strDestID);

        if (strDestID.empty())
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "add_poll_action_dest_data_to_srv_list_proc() DestID Empty \r\n");
            continue;
        }

        strDestDeviceIDVector.push_back(strDestID);
    }
    while (pPoll_Srv_dboper->MoveNext() >= 0);

    DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "add_poll_action_dest_data_to_srv_list_proc:strDestDeviceIDVector.size()=%d \r\n", strDestDeviceIDVector.size());

    /* 循环将信息写入内存中 */
    if (strDestDeviceIDVector.size() > 0)
    {
        for (index = 0; index < (int)strDestDeviceIDVector.size(); index++)
        {
            /* 根据DestID查找动作表 */
            pos = poll_action_find((char*)strDestDeviceIDVector[index].c_str(), pPollActionList);

            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "add_poll_action_dest_data_to_srv_list_proc() DestID=%s:pos=%d \r\n", (char*)strDestDeviceIDVector[index].c_str(), pos);

            if (pos < 0)
            {
                poll_action_t* pPollAction = NULL;

                /* 添加轮巡动作 */
                iRet = poll_action_init(&pPollAction);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_dest_data_to_srv_list_proc() Poll Action Init Error \r\n");
                    continue;
                }

                pPollAction->poll_id = poll_id;
                osip_strncpy(pPollAction->pcDestID, (char*)strDestDeviceIDVector[index].c_str(), MAX_ID_LEN);
                pPollAction->del_mark = 0;

                i = add_poll_action_source_data_to_srv_list_proc(pPollAction->poll_id, pPollAction->pcDestID, pPollAction->pPollActionSourceList, pPoll_Srv_dboper);

                if (i < 0)
                {
                    poll_action_free(pPollAction);
                    pPollAction = NULL;
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_dest_data_to_srv_list_proc() add_poll_action_source_data_to_srv_list_proc Error \r\n");
                    continue;
                }

                /* 添加到队列 */
                i = osip_list_add(pPollActionList, pPollAction, -1); /* add to list tail */

                DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "add_poll_action_dest_data_to_srv_list_proc() PollAction:PollID=%u, DestID=%s, i=%d \r\n", poll_id, pPollAction->pcDestID, i);

                if (i < 0)
                {
                    poll_action_free(pPollAction);
                    pPollAction = NULL;
                    DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_dest_data_to_srv_list_proc() Poll Action Add Error \r\n");
                    continue;
                }
            }
            else
            {
                poll_action_t* pPollAction = NULL;

                pPollAction = (poll_action_t*)osip_list_get(pPollActionList, pos);

                if (NULL != pPollAction)
                {
                    pPollAction->del_mark = 0;

                    i = add_poll_action_source_data_to_srv_list_proc(pPollAction->poll_id, pPollAction->pcDestID, pPollAction->pPollActionSourceList, pPoll_Srv_dboper);

                    if (i < 0)
                    {
                        poll_action_free(pPollAction);
                        pPollAction = NULL;
                        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_dest_data_to_srv_list_proc() add_poll_action_source_data_to_srv_list_proc Error \r\n");
                        continue;
                    }
                }
            }
        }
    }

    strDestDeviceIDVector.clear();

    return osip_list_size(pPollActionList);
}

/*****************************************************************************
 函 数 名  : add_poll_action_source_data_to_srv_list_proc
 功能描述  : 添加轮巡动作的源数据到轮巡业务队列
 输入参数  : int poll_id
                            char* pcDestID
                            osip_list_t* pPollActionSourceList
                            DBOper* pPoll_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int add_poll_action_source_data_to_srv_list_proc(unsigned int poll_id, char* pcDestID, osip_list_t* pPollActionSourceList, DBOper* pPoll_Srv_dboper)
{
    int i = 0;
    int record_count = 0;
    char strPollId[32] = {0};

    string strSQL = "";
    int while_count = 0;

    if (NULL == pcDestID || NULL == pPoll_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "add_poll_action_source_data_to_srv_list_proc() exit---: Poll Srv DB Oper Error \r\n");
        return -1;
    }

    if (NULL == pPollActionSourceList)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "add_poll_action_source_data_to_srv_list_proc() exit---: Poll Action List Error \r\n");
        return -1;
    }

    /* 根据poll_id，查询轮巡动作表，获取轮巡的具体数据 */
    strSQL.clear();
    snprintf(strPollId, 32, "%u", poll_id);
    strSQL = "select * from PollActionConfig WHERE PollID = ";
    strSQL += strPollId;
    strSQL += " and DestID='";
    strSQL += pcDestID;
    strSQL += "'";
    strSQL += " order by SourceSortID asc";

    record_count = pPoll_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_source_data_to_srv_list_proc() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_source_data_to_srv_list_proc() ErrorMsg=%s\r\n", pPoll_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "add_poll_action_source_data_to_srv_list_proc() exit---: No Record Count \r\n");
        return 0;
    }

    /* 循环添加轮巡动作数据 */
    do
    {
        int pos = -1;
        int iRet = 0;
        int iType = -1;
        string strSourceID = "";
        int iSourceStreamType = 0;
        string strDestID = "";
        int iScreenID = -1;
        int iLiveTime = -1;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "add_poll_action_source_data_to_srv_list_proc() While Count=%d \r\n", while_count);
        }

        pPoll_Srv_dboper->GetFieldValue("Type", iType);
        pPoll_Srv_dboper->GetFieldValue("SourceID", strSourceID);
        pPoll_Srv_dboper->GetFieldValue("StreamType", iSourceStreamType);
        pPoll_Srv_dboper->GetFieldValue("DestID", strDestID);
        pPoll_Srv_dboper->GetFieldValue("ScreenID", iScreenID);
        pPoll_Srv_dboper->GetFieldValue("LiveTime", iLiveTime);

        if (strSourceID.empty())
        {
            DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "add_poll_action_source_data_to_srv_list_proc() SourceID Empty \r\n");
            continue;
        }

        /* 根据DestID查找动作表 */
        pos = poll_action_source_find((char*)strSourceID.c_str(), pPollActionSourceList);

        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "add_poll_action_source_data_to_srv_list_proc() Type=%d, SourceID=%s, ScreenID=%d, LiveTime=%d, pos=%d \r\n", iType, strSourceID.c_str(), iScreenID, iLiveTime, pos);

        if (pos < 0)
        {
            poll_action_source_t* pPollActionSource = NULL;

            /* 添加源信息 */
            iRet = poll_action_source_init(&pPollActionSource);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_source_data_to_srv_list_proc() Poll Action Source Init Error \r\n");
                continue;
            }

            pPollActionSource->iStatus = 0;
            pPollActionSource->iType = iType;

            if (!strSourceID.empty())
            {
                osip_strncpy(pPollActionSource->pcSourceID, (char*)strSourceID.c_str(), MAX_ID_LEN);
            }

            pPollActionSource->iSourceStreamType = iSourceStreamType;
            pPollActionSource->iLiveTime = iLiveTime;
            pPollActionSource->iLiveTimeCount = 0;
            pPollActionSource->del_mark = 0;

            i = osip_list_add(pPollActionSourceList, pPollActionSource, -1); /* add to list tail */

            DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "add_poll_action_source_data_to_srv_list_proc() PollActionSource:Type=%d, SourceID=%s, LiveTime=%d, i=%d \r\n", pPollActionSource->iType, pPollActionSource->pcSourceID, pPollActionSource->iLiveTime, i);

            if (i < 0)
            {
                poll_action_source_free(pPollActionSource);
                pPollActionSource = NULL;
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "add_poll_action_source_data_to_srv_list_proc() Poll Action Source Add Error \r\n");
                continue;
            }
        }
        else
        {
            poll_action_source_t* pPollActionSource = NULL;

            pPollActionSource = (poll_action_source_t*)osip_list_get(pPollActionSourceList, pos);

            if (NULL != pPollActionSource)
            {
                if (0 == pPollActionSource->iStatus)
                {
                    pPollActionSource->iType = iType;

                    if (!strSourceID.empty())
                    {
                        if (0 != sstrcmp(pPollActionSource->pcSourceID, (char*)strSourceID.c_str()))
                        {
                            memset(pPollActionSource->pcSourceID, 0, MAX_ID_LEN + 4);
                            osip_strncpy(pPollActionSource->pcSourceID, (char*)strSourceID.c_str(), MAX_ID_LEN);
                        }
                    }
                    else
                    {
                        memset(pPollActionSource->pcSourceID, 0, MAX_ID_LEN + 4);
                    }

                    pPollActionSource->iSourceStreamType = iSourceStreamType;
                    pPollActionSource->iLiveTime = iLiveTime;
                    pPollActionSource->iLiveTimeCount = 0;
                }

                pPollActionSource->del_mark = 0;
            }
        }
    }
    while (pPoll_Srv_dboper->MoveNext() >= 0);

    return osip_list_size(pPollActionSourceList);
}

/*****************************************************************************
 函 数 名  : delete_poll_srv_data
 功能描述  : 删除运行结束的轮巡数据
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void delete_poll_srv_data()
{
    int pos1 = 0;
    int pos2 = 0;
    int pos3 = 0;
    poll_srv_t* pPollSrv = NULL;
    poll_action_t* pPollAction = NULL;
    poll_action_source_t* pPollActionSource = NULL;

    if ((NULL == g_PollSrvList) || (NULL == g_PollSrvList->pPollSrvList))
    {
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "delete_poll_srv_data() exit---: Param Error \r\n");
        return;
    }

    POLL_SMUTEX_LOCK();

    if (osip_list_size(g_PollSrvList->pPollSrvList) <= 0)
    {
        POLL_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "delete_poll_srv_data() exit---: Poll Srv List NULL \r\n");
        return;
    }

    pos1 = 0;

    while (!osip_list_eol(g_PollSrvList->pPollSrvList, pos1))
    {
        pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, pos1);

        if (NULL == pPollSrv)
        {
            osip_list_remove(g_PollSrvList->pPollSrvList, pos1);
            continue;
        }

        if (1 == pPollSrv->del_mark) /* 删除轮巡 */
        {
            osip_list_remove(g_PollSrvList->pPollSrvList, pos1);
            poll_srv_free(pPollSrv);
            pPollSrv = NULL;
        }
        else
        {
            if (NULL == pPollSrv->pPollActionList)
            {
                pos1++;
                continue;
            }

            if (osip_list_size(pPollSrv->pPollActionList) <= 0)
            {
                pos1++;
                continue;
            }

            pos2 = 0;

            while (!osip_list_eol(pPollSrv->pPollActionList, pos2))
            {
                pPollAction = (poll_action_t*)osip_list_get(pPollSrv->pPollActionList, pos2);

                if (NULL == pPollAction)
                {
                    osip_list_remove(pPollSrv->pPollActionList, pos2);
                    continue;
                }

                if (1 == pPollAction->del_mark) /* 删除轮巡 */
                {
                    osip_list_remove(pPollSrv->pPollActionList, pos2);
                    poll_action_free(pPollAction);
                    pPollAction = NULL;
                }
                else
                {
                    if (NULL == pPollAction->pPollActionSourceList)
                    {
                        pos2++;
                        continue;
                    }

                    if (osip_list_size(pPollAction->pPollActionSourceList) <= 0)
                    {
                        pos2++;
                        continue;
                    }

                    pos3 = 0;

                    while (!osip_list_eol(pPollAction->pPollActionSourceList, pos3))
                    {
                        pPollActionSource = (poll_action_source_t*)osip_list_get(pPollAction->pPollActionSourceList, pos3);

                        if (NULL == pPollActionSource)
                        {
                            osip_list_remove(pPollAction->pPollActionSourceList, pos3);
                            continue;
                        }

                        if (1 == pPollActionSource->del_mark) /* 删除轮巡 */
                        {
                            if (pPollAction->current_pos > pos3) /* 如果轮巡的点是需要删除点之后位置，则位置需要减掉一 */
                            {
                                pPollAction->current_pos--;
                            }

                            osip_list_remove(pPollAction->pPollActionSourceList, pos3);
                            poll_action_source_free(pPollActionSource);
                            pPollActionSource = NULL;
                        }
                        else
                        {
                            pos3++;
                        }
                    }

                    pos2++;
                }
            }

            pos1++;
        }
    }

    POLL_SMUTEX_UNLOCK();

    return;
}

/*****************************************************************************
 函 数 名  : SendNotifyExecutePollActionToOnlineUser
 功能描述  : 发送轮巡执行通知给在线客户端
 输入参数  : int iPollID
             char* poll_name
             int iType
             DBOper* pPoll_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendNotifyExecutePollActionToOnlineUser(unsigned int uPollID, char* poll_name, int iType, DBOper* pPoll_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    int index = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    char strPollID[32] = {0};
    vector<unsigned int> UserIndexVector;
    int iUserIndexCount = 0;
    unsigned int uUserIndex = 0;

    /*
     <?xml version="1.0"?>
         <Notify>
         <CmdType>ExecutePoll</CmdType>
         <SN>1234</SN>
         <PollID>轮询ID</PollID>
         </Notify>
     */

    /* 组建XML信息 */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"ExecutePoll");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"1234");

    AccNode = outPacket.CreateElement((char*)"PollID");
    snprintf(strPollID, 32, "%u", uPollID);
    outPacket.SetElementValue(AccNode, strPollID);

    AccNode = outPacket.CreateElement((char*)"PollName");

    if (NULL == poll_name)
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }
    else
    {
        outPacket.SetElementValue(AccNode, poll_name);
    }

    AccNode = outPacket.CreateElement((char*)"RunFlag");

    if (iType == 0)
    {
        outPacket.SetElementValue(AccNode, (char*)"Start");
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"Stop");
    }

    /* 获取poll用户权限表 */
    UserIndexVector.clear();
    iRet = get_user_index_from_user_poll_config(strPollID, UserIndexVector, pPoll_Srv_dboper);

    iUserIndexCount = UserIndexVector.size();

    if (iUserIndexCount <= 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "SendNotifyExecutePollActionToOnlineUser() exit---: Get User Index NULL \r\n");
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送轮巡执行通知给在线客户端: 查询到的用户索引总数=%d", iUserIndexCount);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "SendNotifyExecutePollActionToOnlineUser: Query to the total number of users index=%d", iUserIndexCount);

    /* 循环发送数据 */
    for (index = 0; index < iUserIndexCount; index++)
    {
        /* 获取用户索引 */
        uUserIndex = UserIndexVector[index];

        DEBUG_TRACE(MODULE_POLL_SRV, LOG_TRACE, "SendNotifyExecutePollActionToOnlineUser() index=%d, UserIndex=%u \r\n", index, uUserIndex);

        i |= SendMessageToOnlineUserByUserIndex(uUserIndex, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());
    }

    return i;
}

/*****************************************************************************
 函 数 名  : UpdatePollConfigStatus2DB
 功能描述  : 更新轮巡配置状态到数据库
 输入参数  : int poll_id
             int status
             DBOper* pPoll_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月31日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int UpdatePollConfigStatus2DB(int poll_id, int status, DBOper* pPoll_Srv_dboper)
{
    int iRet = 0;
    string strSQL = "";
    char strPollID[64] = {0};
    char strStatus[16] = {0};

    //printf("\r\n UpdateUserRegInfo2DB() Enter--- \r\n");

    if (poll_id <= 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "UpdatePollConfigStatus2DB() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pPoll_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "UpdatePollConfigStatus2DB() exit---: Poll Srv DB Oper Error \r\n");
        return -1;
    }

    snprintf(strPollID, 64, "%d", poll_id);
    snprintf(strStatus, 16, "%d", status);

    /* 更新数据库 */
    strSQL.clear();
    strSQL = "UPDATE PollConfig SET Resved1 = ";
    strSQL += strStatus;
    strSQL += " WHERE ID = ";
    strSQL += strPollID;

    iRet = pPoll_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "UpdatePollConfigStatus2DB() DB Oper Error: strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "UpdatePollConfigStatus2DB() ErrorMsg=%s\r\n", pPoll_Srv_dboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : get_user_index_from_user_poll_config
 功能描述  : 从用户轮巡权限表里面获取用户索引
 输入参数  : char* pcPollID
             vector<unsigned int>& UserIndexVector
             DBOper* pDBOper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月5日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int get_user_index_from_user_poll_config(char* pcPollID, vector<unsigned int>& UserIndexVector, DBOper* pDBOper)
{
    int iRet = 0;
    int record_count = 0;
    int while_count = 0;
    string strSQL = "";

    if (NULL == pcPollID || NULL == pDBOper)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_DEBUG, "get_user_index_from_user_poll_config() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "SELECT UserID FROM UserPollConfig WHERE PollID = ";
    strSQL += pcPollID;

    record_count = pDBOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "get_user_index_from_user_poll_config() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "get_user_index_from_user_poll_config() ErrorMsg=%s\r\n", pDBOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count > 0)
    {
        /* 循环查找数据库*/
        do
        {
            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "get_user_index_from_user_poll_config() While Count=%d \r\n", while_count);
            }

            unsigned int uUserIndex = 0;

            pDBOper->GetFieldValue("UserID", uUserIndex);

            iRet = AddUserIndexToUserIndexVector(UserIndexVector, uUserIndex);
        }
        while (pDBOper->MoveNext() >= 0);
    }

    /* 获取一下admin用户的权限 */
    strSQL.clear();
    strSQL = "SELECT ID FROM UserConfig WHERE UserName like 'admin'";

    record_count = pDBOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "get_user_index_from_user_poll_config() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "get_user_index_from_user_poll_config() ErrorMsg=%s\r\n", pDBOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count > 0)
    {
        /* 循环查找数据库*/
        do
        {
            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "get_user_index_from_user_poll_config() While Count=%d \r\n", while_count);
            }

            unsigned int uUserIndex = 0;

            pDBOper->GetFieldValue("ID", uUserIndex);

            iRet = AddUserIndexToUserIndexVector(UserIndexVector, uUserIndex);
        }
        while (pDBOper->MoveNext() >= 0);
    }

    /* 获取一下WiscomV用户的权限 */
    strSQL.clear();
    strSQL = "SELECT ID FROM UserConfig WHERE UserName like 'WiscomV'";

    record_count = pDBOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "get_user_index_from_user_poll_config() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_POLL_SRV, LOG_ERROR, "get_user_index_from_user_poll_config() ErrorMsg=%s\r\n", pDBOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count > 0)
    {
        /* 循环查找数据库*/
        do
        {
            while_count++;

            if (while_count % 10000 == 0)
            {
                DEBUG_TRACE(MODULE_POLL_SRV, LOG_WARN, "get_user_index_from_user_poll_config() While Count=%d \r\n", while_count);
            }

            unsigned int uUserIndex = 0;

            pDBOper->GetFieldValue("ID", uUserIndex);

            iRet = AddUserIndexToUserIndexVector(UserIndexVector, uUserIndex);
        }
        while (pDBOper->MoveNext() >= 0);
    }
    return 0;
}

/*****************************************************************************
 函 数 名  : ShowPollTaskInfo
 功能描述  : 显示当前轮巡任务信息
 输入参数  : int sock
             int status
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月7日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void ShowPollTaskInfo(int sock, int status)
{
    int i = 0, j = 0, k = 0;
    char strLine[] = "\r-----------------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rPollID  DurationTime DurationTimeCount DestID               CurrentPos Type SourceID             LiveTime LiveTimeCount\r\n";
    poll_srv_t* pPollSrv = NULL;
    poll_action_t* pPollAction = NULL;
    poll_action_source_t* pPollActionSource = NULL;
    char rbuf[128] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if ((NULL == g_PollSrvList) || (NULL == g_PollSrvList->pPollSrvList))
    {
        return;
    }

    POLL_SMUTEX_LOCK();

    if (osip_list_size(g_PollSrvList->pPollSrvList) <= 0)
    {
        POLL_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_PollSrvList->pPollSrvList); i++)
    {
        pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, i);

        if (NULL == pPollSrv || NULL == pPollSrv->pPollActionList)
        {
            continue;
        }

        if (status <= 1)
        {
            if (pPollSrv->status != status) /* 没有轮巡的忽略 */
            {
                continue;
            }
        }

        /* 查找具体轮巡动作 */
        for (j = 0; j < osip_list_size(pPollSrv->pPollActionList); j++)
        {
            pPollAction = (poll_action_t*)osip_list_get(pPollSrv->pPollActionList, j);

            if (NULL == pPollAction || NULL == pPollAction->pPollActionSourceList)
            {
                continue;
            }

            for (k = 0; k < osip_list_size(pPollAction->pPollActionSourceList); k++)
            {
                pPollActionSource = (poll_action_source_t*)osip_list_get(pPollAction->pPollActionSourceList, k);

                if (NULL == pPollActionSource)
                {
                    continue;
                }

                if (status <= 1)
                {
                    if (1 == status && k != pPollAction->current_pos)  /* 不是当前正在当前轮巡的忽略 */
                    {
                        continue;
                    }

                    if (status != pPollActionSource->iStatus) /* 没有启动的轮巡忽略 */
                    {
                        continue;
                    }
                }

                snprintf(rbuf, 128, "\r%-7u %-12d %-17d %-20s %-10d %-4d %-20s %-8d %-13d\r\n", pPollSrv->poll_id, pPollSrv->duration_time, pPollSrv->duration_time_count, pPollAction->pcDestID, pPollAction->current_pos, pPollActionSource->iType, pPollActionSource->pcSourceID, pPollActionSource->iLiveTime, pPollActionSource->iLiveTimeCount);

                if (sock > 0)
                {
                    send(sock, rbuf, strlen(rbuf), 0);
                }
            }
        }
    }

    POLL_SMUTEX_UNLOCK();

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

/*****************************************************************************
 函 数 名  : StopPollTask
 功能描述  : 停止轮巡任务
 输入参数  : int sock
             unsigned int poll_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月7日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StopPollTask(int sock, unsigned int poll_id)
{
    int iRet = 0;
    char rbuf[128] = {0};

    /* 停止业务 */
    iRet = stop_poll_srv_by_id(poll_id);

    if (sock > 0)
    {
        memset(rbuf, 0, 128);

        if (0 == iRet)
        {
            snprintf(rbuf, 128, "\r停止轮巡任务成功: 轮巡ID=%u\r\n$", poll_id);
            send(sock, rbuf, strlen(rbuf), 0);
        }
        else
        {
            snprintf(rbuf, 128, "\r停止轮巡任务失败: 轮巡ID=%u\r\n$", poll_id);
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllPollTask
 功能描述  : 停止所有轮巡任务
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月7日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StopAllPollTask(int sock)
{
    int i = 0;
    int iRet = 0;
    poll_srv_t* pPollSrv = NULL;
    needtoproc_pollsrv_queue needToStop;
    char rbuf[128] = {0};

    if ((NULL == g_PollSrvList) || (NULL == g_PollSrvList->pPollSrvList))
    {
        return -1;
    }

    needToStop.clear();

    POLL_SMUTEX_LOCK();

    if (osip_list_size(g_PollSrvList->pPollSrvList) <= 0)
    {
        POLL_SMUTEX_UNLOCK();
        return -1;
    }

    for (i = 0; i < osip_list_size(g_PollSrvList->pPollSrvList); i++)
    {
        pPollSrv = (poll_srv_t*)osip_list_get(g_PollSrvList->pPollSrvList, i);

        if (NULL == pPollSrv)
        {
            continue;
        }

        needToStop.push_back(pPollSrv);
        pPollSrv->duration_time_count = 0;
    }

    POLL_SMUTEX_UNLOCK();

    while (!needToStop.empty())
    {
        pPollSrv = (poll_srv_t*) needToStop.front();
        needToStop.pop_front();

        if (NULL != pPollSrv)
        {
            if (1 == pPollSrv->status || 4 == pPollSrv->status)
            {
                pPollSrv->status = 3;
            }
            else if (2 == pPollSrv->status)
            {
                pPollSrv->status = 0;

                /* 通知客户端 */ /* 停止轮巡都不需要发送给客户端 */
                //iRet = SendNotifyExecutePollActionToOnlineUser(pPollSrv->poll_id, 1);

                pPollSrv->send_mark = 1;
            }

            if (sock > 0)
            {
                memset(rbuf, 0, 128);

                if (iRet >= 0)
                {
                    snprintf(rbuf, 128, "\r停止轮巡任务成功: 轮巡ID=%d\r\n", pPollSrv->poll_id);
                    send(sock, rbuf, strlen(rbuf), 0);
                }
                else
                {
                    snprintf(rbuf, 128, "\r停止轮巡任务失败: 轮巡ID=%d\r\n", pPollSrv->poll_id);
                    send(sock, rbuf, strlen(rbuf), 0);
                }
            }
        }
    }

    needToStop.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : PollSrvConfig_db_refresh_proc
 功能描述  : 设置轮巡业务配置信息数据库更新操作标识
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月18日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int PollSrvConfig_db_refresh_proc()
{
    if (1 == db_PollSrvInfo_reload_mark) /* 正在执行 */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "轮巡业务配置数据库信息正在同步");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Poll Srv Info database information are synchronized");
        return 0;
    }

    db_PollSrvInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 函 数 名  : check_PollSrvConfig_need_to_reload_begin
 功能描述  : 检查是否需要同步轮巡业务配置开始
 输入参数  : DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月18日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void check_PollSrvConfig_need_to_reload_begin(DBOper* pDboper)
{
    /* 检查是否需要更新数据库标识 */
    if (!db_PollSrvInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步轮巡业务配置数据库信息: 开始---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization Poll Srv info database information: begain---");

    /* 设置轮巡队列的删除标识 */
    set_poll_srv_list_del_mark(1);

    /* 将数据库中的变化数据同步到内存 */
    check_db_data_to_poll_srv_list(pDboper);

    return;
}

/*****************************************************************************
 函 数 名  : check_PollSrvConfig_need_to_reload_end
 功能描述  : 检查是否需要同步轮巡业务配置表结束
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月18日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void check_PollSrvConfig_need_to_reload_end()
{
    /* 检查是否需要更新数据库标识 */
    if (!db_PollSrvInfo_reload_mark)
    {
        return;
    }

    /* 删除已经停止的轮巡数据 */
    delete_poll_srv_data();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步轮巡业务配置数据库信息: 结束---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization Poll Srv info database information: end---");
    db_PollSrvInfo_reload_mark = 0;

    return;
}
