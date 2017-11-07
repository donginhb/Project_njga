
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

#include "user/user_info_mgn.inc"

#include "service/cruise_srv_proc.inc"


/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern DBOper g_DBOper;

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
cruise_srv_list_t* g_CruiseSrvList = NULL;    /* 巡航业务队列 */
int db_CruiseSrvInfo_reload_mark = 0;         /* 巡航业务数据库更新标识:0:不需要更新，1:需要更新数据库 */

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#if DECS("巡航业务队列")

/*****************************************************************************
 函 数 名  : cruise_action_init
 功能描述  : 巡航动作结构初始化
 输入参数  : cruise_action_t** cruise_action
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月3日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int cruise_action_init(cruise_action_t** cruise_action)
{
    *cruise_action = (cruise_action_t*)osip_malloc(sizeof(cruise_action_t));

    if (*cruise_action == NULL)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_action_init() exit---: *cruise_srv Smalloc Error \r\n");
        return -1;
    }

    (*cruise_action)->device_index = 0;
    (*cruise_action)->iPresetID = 0;
    (*cruise_action)->iStatus = 0;
    (*cruise_action)->iLiveTime = 0;
    (*cruise_action)->iLiveTimeCount = 0;
    (*cruise_action)->del_mark = 0;

    return 0;
}

/*****************************************************************************
 函 数 名  : cruise_action_free
 功能描述  : 巡航动作结构释放
 输入参数  : cruise_action_t* cruise_action
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月3日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void cruise_action_free(cruise_action_t* cruise_action)
{
    if (cruise_action == NULL)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_action_free() exit---: Param Error \r\n");
        return;
    }

    cruise_action->device_index = 0;
    cruise_action->iPresetID = 0;
    cruise_action->iStatus = 0;
    cruise_action->iLiveTime = 0;
    cruise_action->iLiveTimeCount = 0;
    cruise_action->del_mark = 0;

    osip_free(cruise_action);
    cruise_action = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : cruise_action_find
 功能描述  : 根据逻辑设备索引和预置位查找巡航动作
 输入参数  : unsigned int device_index
             unsigned int iPresetID
             osip_list_t* pCruiseActionList
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月3日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int cruise_action_find(unsigned int device_index, unsigned int iPresetID, osip_list_t* pCruiseActionList)
{
    int i = 0;
    cruise_action_t* pCruiseAction = NULL;

    if (device_index <= 0 || NULL == pCruiseActionList)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_action_find() exit---: Cruise Action List Error \r\n");
        return -1;
    }

    for (i = 0; i < osip_list_size(pCruiseActionList); i++)
    {
        pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseActionList, i);

        if (NULL == pCruiseAction || pCruiseAction->device_index <= 0)
        {
            continue;
        }

        if (pCruiseAction->device_index == device_index
            && pCruiseAction->iPresetID == iPresetID)
        {
            return i;
        }
    }

    return -1;
}

/*****************************************************************************
 函 数 名  : cruise_action_get
 功能描述  : 巡航动作获取
 输入参数  : int pos
                            osip_list_t* pCruiseActionList
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月3日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
cruise_action_t* cruise_action_get(int pos, osip_list_t* pCruiseActionList)
{
    cruise_action_t* pCruiseAction = NULL;

    if (NULL == pCruiseActionList)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_action_find() exit---: Cruise Action List Error \r\n");
        return NULL;
    }

    if (pos < 0 || (pos >= osip_list_size(pCruiseActionList)))
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_action_find() exit---: Pos Error \r\n");
        return NULL;
    }

    pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseActionList, pos);

    if (NULL == pCruiseAction)
    {
        return NULL;
    }
    else
    {
        return pCruiseAction;
    }
}

/*****************************************************************************
 函 数 名  : cruise_srv_init
 功能描述  : 巡航业务结构初始化
 输入参数  : cruise_srv_t ** cruise_srv
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int cruise_srv_init(cruise_srv_t** cruise_srv)
{
    *cruise_srv = (cruise_srv_t*)osip_malloc(sizeof(cruise_srv_t));

    if (*cruise_srv == NULL)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_init() exit---: *cruise_srv Smalloc Error \r\n");
        return -1;
    }

    (*cruise_srv)->cruise_id = 0;
    (*cruise_srv)->status = 0;
    (*cruise_srv)->cruise_type = 0;
    (*cruise_srv)->cruise_name[0] = '\0';
    (*cruise_srv)->start_time = 0;
    (*cruise_srv)->duration_time = 0;
    (*cruise_srv)->duration_time_count = 0;
    (*cruise_srv)->current_pos = 0;

    (*cruise_srv)->del_mark = 0;
    (*cruise_srv)->send_mark = 0;

    (*cruise_srv)->pCruiseActionList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == (*cruise_srv)->pCruiseActionList)
    {
        osip_free(*cruise_srv);
        *cruise_srv = NULL;
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_init() exit---: Cruise Action List Init Error \r\n");
        return -1;
    }

    osip_list_init((*cruise_srv)->pCruiseActionList);

    return 0;
}

/*****************************************************************************
 函 数 名  : cruise_srv_free
 功能描述  : 巡航业务结构释放
 输入参数  : cruise_srv_t * cruise_srv
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void cruise_srv_free(cruise_srv_t* cruise_srv)
{
    if (cruise_srv == NULL)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_free() exit---: Param Error \r\n");
        return;
    }

    cruise_srv->cruise_id = 0;
    cruise_srv->status = 0;
    cruise_srv->cruise_type = 0;

    memset(cruise_srv->cruise_name, 0, MAX_128CHAR_STRING_LEN + 4);

    cruise_srv->start_time = 0;
    cruise_srv->duration_time = 0;
    cruise_srv->duration_time_count = 0;
    cruise_srv->current_pos = 0;

    cruise_srv->del_mark = 0;
    cruise_srv->send_mark = 0;

    if (NULL != cruise_srv->pCruiseActionList)
    {
        osip_list_special_free(cruise_srv->pCruiseActionList, (void (*)(void*))&cruise_action_free);
        osip_free(cruise_srv->pCruiseActionList);
        cruise_srv->pCruiseActionList = NULL;
    }

    osip_free(cruise_srv);
    cruise_srv = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : cruise_srv_find
 功能描述  : 根据ID查找巡航业务
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
int cruise_srv_find(unsigned int id)
{
    int i = 0;
    cruise_srv_t* pCruiseSrv = NULL;

    if (id <= 0 || NULL == g_CruiseSrvList || NULL == g_CruiseSrvList->pCruiseSrvList)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_find() exit---: Cruise Action List Error \r\n");
        return -1;
    }

    CRUISE_SMUTEX_LOCK();

    for (i = 0; i < osip_list_size(g_CruiseSrvList->pCruiseSrvList); i++)
    {
        pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, i);

        if (NULL == pCruiseSrv || pCruiseSrv->cruise_id <= 0)
        {
            continue;
        }

        if (pCruiseSrv->cruise_id == id)
        {
            CRUISE_SMUTEX_UNLOCK();
            return i;
        }
    }

    CRUISE_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : cruise_srv_get
 功能描述  : 获取巡航业务
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
cruise_srv_t* cruise_srv_get(int pos)
{
    cruise_srv_t* pCruiseSrv = NULL;

    if (NULL == g_CruiseSrvList || NULL == g_CruiseSrvList->pCruiseSrvList)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_get() exit---: Cruise Action List Error \r\n");
        return NULL;
    }

    if (pos < 0 || (pos >= osip_list_size(g_CruiseSrvList->pCruiseSrvList)))
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_get() exit---: Pos Error \r\n");
        return NULL;
    }

    pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, pos);

    if (NULL == pCruiseSrv)
    {
        return NULL;
    }
    else
    {
        return pCruiseSrv;
    }
}

/*****************************************************************************
 函 数 名  : cruise_srv_list_init
 功能描述  : 巡航业务队列初始化
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
int cruise_srv_list_init()
{
    g_CruiseSrvList = (cruise_srv_list_t*)osip_malloc(sizeof(cruise_srv_list_t));

    if (g_CruiseSrvList == NULL)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_list_init() exit---: g_CruiseSrvList Smalloc Error \r\n");
        return -1;
    }

    g_CruiseSrvList->pCruiseSrvList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_CruiseSrvList->pCruiseSrvList)
    {
        osip_free(g_CruiseSrvList);
        g_CruiseSrvList = NULL;
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_list_init() exit---: Cruise Srv List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_CruiseSrvList->pCruiseSrvList);

#ifdef MULTI_THR
    /* init smutex */
    g_CruiseSrvList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_CruiseSrvList->lock)
    {
        osip_free(g_CruiseSrvList->pCruiseSrvList);
        g_CruiseSrvList->pCruiseSrvList = NULL;
        osip_free(g_CruiseSrvList);
        g_CruiseSrvList = NULL;
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_list_init() exit---: Cruise Srv List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : cruise_srv_list_free
 功能描述  : 巡航业务队列释放
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
void cruise_srv_list_free()
{
    if (NULL == g_CruiseSrvList)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_CruiseSrvList->pCruiseSrvList)
    {
        osip_list_special_free(g_CruiseSrvList->pCruiseSrvList, (void (*)(void*))&cruise_srv_free);
        osip_free(g_CruiseSrvList->pCruiseSrvList);
        g_CruiseSrvList->pCruiseSrvList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_CruiseSrvList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_CruiseSrvList->lock);
        g_CruiseSrvList->lock = NULL;
    }

#endif
    osip_free(g_CruiseSrvList);
    g_CruiseSrvList = NULL;
    return;
}

/*****************************************************************************
 函 数 名  : cruise_srv_list_lock
 功能描述  : 巡航业务队列锁定
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
int cruise_srv_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CruiseSrvList == NULL || g_CruiseSrvList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_CruiseSrvList->lock);
#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : cruise_srv_list_unlock
 功能描述  : 巡航业务解锁
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
int cruise_srv_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CruiseSrvList == NULL || g_CruiseSrvList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_CruiseSrvList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_cruise_srv_list_lock
 功能描述  : 巡航业务队列锁定
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
int debug_cruise_srv_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CruiseSrvList == NULL || g_CruiseSrvList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "debug_cruise_srv_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_CruiseSrvList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_cruise_srv_list_unlock
 功能描述  : 巡航业务解锁
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
int debug_cruise_srv_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CruiseSrvList == NULL || g_CruiseSrvList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "debug_cruise_srv_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_CruiseSrvList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : cruise_srv_add
 功能描述  : 巡航业务添加
 输入参数  : cruise_srv_t* pCruiseSrv
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月3日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int cruise_srv_add(cruise_srv_t* pCruiseSrv)
{
    int i = 0;

    if (pCruiseSrv == NULL)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_add() exit---: Param Error \r\n");
        return -1;
    }

    CRUISE_SMUTEX_LOCK();

    i = osip_list_add(g_CruiseSrvList->pCruiseSrvList, pCruiseSrv, -1); /* add to list tail */

    //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "cruise_srv_add() CruiseSrv:CruiseID=%u, StartTime=%d, DurationTime=%d, i=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->start_time, pCruiseSrv->duration_time, i);

    if (i < 0)
    {
        CRUISE_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "cruise_srv_add() exit---: List Add Error \r\n");
        return -1;
    }

    CRUISE_SMUTEX_UNLOCK();
    return i - 1;
}

/*****************************************************************************
 函 数 名  : cruise_srv_remove
 功能描述  : 从队列中移除巡航业务
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
int cruise_srv_remove(int pos)
{
    cruise_srv_t* pCruiseSrv = NULL;

    CRUISE_SMUTEX_LOCK();

    if (g_CruiseSrvList == NULL || pos < 0 || (pos >= osip_list_size(g_CruiseSrvList->pCruiseSrvList)))
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_srv_remove() exit---: Param Error \r\n");
        CRUISE_SMUTEX_UNLOCK();
        return -1;
    }

    pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, pos);

    if (NULL == pCruiseSrv)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "cruise_srv_remove() exit---: List Get Error \r\n");
        CRUISE_SMUTEX_UNLOCK();
        return -1;
    }

    osip_list_remove(g_CruiseSrvList->pCruiseSrvList, pos);
    cruise_srv_free(pCruiseSrv);
    pCruiseSrv = NULL;
    CRUISE_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 函 数 名  : scan_cruise_srv_list
 功能描述  : 扫描巡航业务消息队列
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
void scan_cruise_srv_list(DBOper* pCruise_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    cruise_srv_t* pCruiseSrv = NULL;
    needtoproc_cruisesrv_queue needToProc;
    needtoproc_cruisesrv_queue needToStop;
    needtoproc_cruisesrv_queue needToSend;

    if ((NULL == g_CruiseSrvList) || (NULL == g_CruiseSrvList->pCruiseSrvList))
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "scan_cruise_srv_list() exit---: Param Error \r\n");
        return;
    }

    needToProc.clear();
    needToStop.clear();
    needToSend.clear();

    CRUISE_SMUTEX_LOCK();

    if (osip_list_size(g_CruiseSrvList->pCruiseSrvList) <= 0)
    {
        CRUISE_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_CruiseSrvList->pCruiseSrvList); i++)
    {
        pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, i);

        if (NULL == pCruiseSrv)
        {
            continue;
        }

        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() cruise_id=%u, CruiseSrv: StartTime=%d, DurationTime=%d, DurationTimeCount=%d, DelMark=%d, Status=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->start_time, pCruiseSrv->duration_time, pCruiseSrv->duration_time_count, pCruiseSrv->del_mark, pCruiseSrv->status);

        if (1 == pCruiseSrv->del_mark) /* 要删除的数据 */
        {
            if (1 == pCruiseSrv->status || 4 == pCruiseSrv->status)
            {
                needToStop.push_back(pCruiseSrv);
                pCruiseSrv->duration_time_count = 0;
            }
            else
            {
                continue;
            }
        }

        if (2 == pCruiseSrv->status) /* 需要启动巡航 */
        {
            cruise_action_release(pCruiseSrv);
            pCruiseSrv->status = 1;
            pCruiseSrv->duration_time_count = 0;
        }
        else if (3 == pCruiseSrv->status)  /* 需要停止巡航 */
        {
            needToStop.push_back(pCruiseSrv);
            pCruiseSrv->duration_time_count = 0;
        }
        else if (4 == pCruiseSrv->status)  /* 需要发送通知给客户端 */
        {
            needToSend.push_back(pCruiseSrv);
            pCruiseSrv->status = 1;
        }

        if (1 == pCruiseSrv->status)  /* 看是否要停止巡航 */
        {
            if (0 != pCruiseSrv->cruise_type) /* 预案执行的轮巡 */
            {
                needToProc.push_back(pCruiseSrv);
            }
            else
            {
                if (pCruiseSrv->duration_time > 0 && pCruiseSrv->duration_time_count > 0
                    && pCruiseSrv->duration_time_count >= pCruiseSrv->duration_time)
                {
                    needToStop.push_back(pCruiseSrv);
                    pCruiseSrv->duration_time_count = 0;
                }
                else
                {
                    needToProc.push_back(pCruiseSrv);

                    if (pCruiseSrv->duration_time > 0)
                    {
                        pCruiseSrv->duration_time_count++;
                    }
                }
            }
        }
    }

    CRUISE_SMUTEX_UNLOCK();

    /* 处理需要开始的 */
    while (!needToProc.empty())
    {
        pCruiseSrv = (cruise_srv_t*) needToProc.front();
        needToProc.pop_front();

        if (NULL != pCruiseSrv)
        {
            iRet = cruise_action_proc(pCruiseSrv);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "scan_cruise_srv_list() cruise_action_proc Error:cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() cruise_action_proc OK:cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
            }

            if (0 == pCruiseSrv->send_mark)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行巡航发送开始巡航:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send notify execute cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);

                /* 通知客户端 */
                iRet = SendNotifyExecuteCruiseActionToOnlineUser(pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, 0, pCruise_Srv_dboper);

                if (iRet < 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "执行巡航发送开始巡航失败:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send notify execute cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "scan_cruise_srv_list() SendNotifyExecuteCruiseActionToOnlineUser Start Error: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                }
                else if (iRet > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行巡航发送开始巡航成功:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send notify execute cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() SendNotifyExecuteCruiseActionToOnlineUser Start OK: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                }

                pCruiseSrv->send_mark = 1;

                /* 更新状态 */
                iRet = UpdateCruiseConfigStatus2DB(pCruiseSrv->cruise_id, 1, pCruise_Srv_dboper);

                if (iRet < 0)
                {
                    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "scan_cruise_srv_list() UpdateCruiseConfigStatus2DB Start Error: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() UpdateCruiseConfigStatus2DB Start OK: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                }
            }
        }
    }

    needToProc.clear();

    /* 处理需要发送的 */
    while (!needToSend.empty())
    {
        pCruiseSrv = (cruise_srv_t*) needToSend.front();
        needToSend.pop_front();

        if (NULL != pCruiseSrv)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行巡航发送开始巡航:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send notify execute cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);


            /* 通知客户端 */
            iRet = SendNotifyExecuteCruiseActionToOnlineUser(pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, 0, pCruise_Srv_dboper);

            if (iRet < 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "执行巡航发送开始巡航失败:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send notify execute cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "scan_cruise_srv_list() SendNotifyExecuteCruiseActionToOnlineUser Start Error: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
            }
            else if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行巡航发送开始巡航成功:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send notify execute cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() SendNotifyExecuteCruiseActionToOnlineUser Start OK: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
            }
        }
    }

    needToSend.clear();

    /* 处理需要停止的 */
    while (!needToStop.empty())
    {
        pCruiseSrv = (cruise_srv_t*) needToStop.front();
        needToStop.pop_front();

        if (NULL != pCruiseSrv)
        {
            iRet = cruise_action_stop(pCruiseSrv);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "scan_cruise_srv_list() cruise_action_stop Error:cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() cruise_action_stop OK:cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
            }

            if (iRet >= 0)
            {
                pCruiseSrv->status = 0;
                pCruiseSrv->duration_time_count = 0;

                if (iRet > 0)
                {
                    if (1 == pCruiseSrv->send_mark) /* 用户手动停止的不需要发送给客户端 */
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行巡航发送停止巡航:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send notify stop cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);

                        /* 通知客户端 */
                        iRet = SendNotifyExecuteCruiseActionToOnlineUser(pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, 1, pCruise_Srv_dboper);

                        if (iRet < 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "执行巡航发送停止巡航失败:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to send notify stop cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "scan_cruise_srv_list() SendNotifyExecuteCruiseActionToOnlineUser Start Error: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                        }
                        else if (iRet > 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行巡航发送停止巡航成功:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Succeed to send notify stop cruise action:CruiseID=%u, CruiseName=%s", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name);
                            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() SendNotifyExecuteCruiseActionToOnlineUser Start OK: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                        }
                    }
                }

                /* 更新状态 */
                iRet = UpdateCruiseConfigStatus2DB(pCruiseSrv->cruise_id, 0, pCruise_Srv_dboper);

                if (iRet < 0)
                {
                    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "scan_cruise_srv_list() UpdateCruiseConfigStatus2DB Start Error: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "scan_cruise_srv_list() UpdateCruiseConfigStatus2DB Start OK: cruise_id=%u, iRet=%d \r\n", pCruiseSrv->cruise_id, iRet);
                }

                pCruiseSrv->send_mark = 0;
            }
        }
    }

    needToStop.clear();

    return;
}
#endif

/*****************************************************************************
 函 数 名  : cruise_action_release
 功能描述  : 巡航动作释放
 输入参数  : cruise_srv_t* pCruiseSrv
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月30日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int cruise_action_release(cruise_srv_t* pCruiseSrv)
{
    int i = 0;
    cruise_action_t* pCruiseAction = NULL;

    if (NULL == pCruiseSrv || NULL == pCruiseSrv->pCruiseActionList)
    {
        return -1;
    }

    for (i = 0; i < osip_list_size(pCruiseSrv->pCruiseActionList); i++)
    {
        pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseSrv->pCruiseActionList, i);

        if (NULL == pCruiseAction)
        {
            continue;
        }

        if (1 == pCruiseAction->iStatus)
        {
            pCruiseAction->iStatus = 0;
            pCruiseAction->iLiveTimeCount = 0;
        }
    }

    pCruiseSrv->current_pos = 0;

    return 0;
}

/*****************************************************************************
 函 数 名  : cruise_action_stop
 功能描述  : 停止预案动作
 输入参数  : cruise_srv_t* pCruiseSrv
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int cruise_action_stop(cruise_srv_t* pCruiseSrv)
{
    int i = 0;
    cruise_action_t* pCruiseAction = NULL;
    int iNotifyUserFlag = 1;

    if (NULL == pCruiseSrv || NULL == pCruiseSrv->pCruiseActionList)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_action_stop() exit---: Cruise Action List Error \r\n");
        return -1;
    }

    for (i = 0; i < osip_list_size(pCruiseSrv->pCruiseActionList); i++)
    {
        pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseSrv->pCruiseActionList, i);

        if (NULL == pCruiseAction)
        {
            continue;
        }

        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "cruise_action_stop() CruiseAction: CruiseID=%u, CurrentPos=%d, device_index=%u, PresetID=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->current_pos, pCruiseAction->device_index, pCruiseAction->iPresetID);

        if (1 == pCruiseAction->iStatus)
        {
            pCruiseAction->iStatus = 0;
            pCruiseAction->iLiveTimeCount = 0;
        }
    }

    pCruiseSrv->current_pos = 0;

    return iNotifyUserFlag;
}

/*****************************************************************************
 函 数 名  : cruise_action_proc
 功能描述  : 执行预案动作
 输入参数  : cruise_srv_t* pCruiseSrv
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int cruise_action_proc(cruise_srv_t* pCruiseSrv)
{
    int iRet = 0;
    int i = 0;
    cruise_action_t* pCruiseAction = NULL;
    int iNotifyUserFlag = 0;

    if (NULL == pCruiseSrv || NULL == pCruiseSrv->pCruiseActionList)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "cruise_action_proc() exit---: Cruise Action List Error \r\n");
        return -1;
    }

    for (i = 0; i < osip_list_size(pCruiseSrv->pCruiseActionList); i++)
    {
        pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseSrv->pCruiseActionList, i);

        if (NULL == pCruiseAction)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "cruise_action_proc() Get CruiseActionPreset Error\r\n");
            pCruiseSrv->current_pos++;
            continue;
        }

        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "cruise_action_proc() cruise_id=%d, current_pos=%d, DeviceIndex=%u, PresetID=%d, LiveTime=%d, LiveTimeCount=%d, Status=%d, del_mark=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->current_pos, pCruiseAction->device_index, pCruiseAction->iPresetID, pCruiseAction->iLiveTime, pCruiseAction->iLiveTimeCount, pCruiseAction->iStatus, pCruiseAction->del_mark);

        if (1 == pCruiseAction->del_mark) /* 需要删除的，看是否需要停止 */
        {
            if (i == pCruiseSrv->current_pos)
            {
                if (1 == pCruiseAction->iStatus)
                {
                    pCruiseAction->iLiveTimeCount = 0;
                    pCruiseAction->iStatus = 0;
                    pCruiseSrv->current_pos = i + 1;
                    iNotifyUserFlag = 2;
                }
            }

            if (pCruiseSrv->current_pos >= osip_list_size(pCruiseSrv->pCruiseActionList))
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "cruise_action_proc() CurrentPos=%d \r\n", pCruiseSrv->current_pos, iRet);
                pCruiseSrv->current_pos = 0;
            }
        }
        else
        {
            if (i == pCruiseSrv->current_pos) /* 正在当前巡航 */
            {
                if (0 == pCruiseAction->iStatus)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行巡航: DeviceIndex=%u, PresetID=%u", pCruiseAction->device_index, pCruiseAction->iPresetID);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Launch navigation: DeviceIndex=%u, PresetID=%u", pCruiseAction->device_index, pCruiseAction->iPresetID);

                    iRet = ExecuteDevicePresetByPresetIDAndDeviceIndex(pCruiseAction->iPresetID, pCruiseAction->device_index, &g_DBOper);

                    if (iRet < 0)
                    {
                        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "cruise_action_proc() ExecuteDevicePresetByPresetIDAndDeviceIndex Error: DeviceIndex=%u, PresetID=%u, CurrentPos=%d, iRet=%d\r\n", pCruiseAction->device_index, pCruiseAction->iPresetID, pCruiseSrv->current_pos, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "cruise_action_proc() ExecuteDevicePresetByPresetIDAndDeviceIndex OK: DeviceIndex=%u, PresetID=%u, CurrentPos=%d, iRet=%d\r\n", pCruiseAction->device_index, pCruiseAction->iPresetID, pCruiseSrv->current_pos, iRet);
                    }

                    pCruiseAction->iStatus = 1;
                    iNotifyUserFlag = 1;
                }
                else
                {
                    pCruiseAction->iLiveTimeCount++;
                    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "cruise_action_proc() DeviceIndex=%u, PresetID=%u, LiveTimeCount=%d, CurrentPos=%d\r\n", pCruiseAction->device_index, pCruiseAction->iPresetID, pCruiseAction->iLiveTimeCount, pCruiseSrv->current_pos);
                }

                if (pCruiseAction->iLiveTimeCount >= pCruiseAction->iLiveTime)
                {
                    pCruiseAction->iLiveTimeCount = 0;
                    pCruiseSrv->current_pos = i + 1;
                    pCruiseAction->iStatus = 0;
                    iNotifyUserFlag = 2;
                }
            }

            if (pCruiseSrv->current_pos >= osip_list_size(pCruiseSrv->pCruiseActionList))
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "cruise_action_proc() CurrentPos=%d \r\n", pCruiseSrv->current_pos, iRet);
                pCruiseSrv->current_pos = 0;

                /* 再次启动被预案中断的巡航 */
                if (1 == pCruiseSrv->cruise_type) /* 预案执行的巡航，执行一遍就不要执行了,需要停止 */
                {
                    pCruiseSrv->status = 3;
                }
                else if (2 == pCruiseSrv->cruise_type) /* 被预案中断的巡航，需要再次启动 */
                {
                    pCruiseSrv->status = 2;
                    pCruiseSrv->cruise_type = 0;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "启动被预案中断的巡航: 巡航ID=%u, 巡航名称=%s, 开始时间=%d, 持续时间=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Start cruise that plan interrupt : cruise ID=%u, cruise name=%s, start time=%d, during time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                }
                else if (3 == pCruiseSrv->cruise_type) /* 预案执行的巡航，被用户执行之后，需要再次启动 */
                {
                    pCruiseSrv->status = 2;
                    pCruiseSrv->cruise_type = 0;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "启动被用户执行的预案巡航: 巡航ID=%u, 巡航名称=%s, 开始时间=%d, 持续时间=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Start cruise that users interrupt: cruise ID=%u, cruise name=%s, start time=%d, during time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                }
            }
        }
    }

    return iNotifyUserFlag;
}

/*****************************************************************************
 函 数 名  : start_cruise_srv_by_id
 功能描述  : 根据ID启动巡航任务
 输入参数  : user_info_t* pUserInfo
             int id
             DBOper* pCruise_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int start_cruise_srv_by_id(user_info_t* pUserInfo, unsigned int id, DBOper* pCruise_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    int cruise_pos = -1;
    string strSQL = "";
    int record_count = 0;
    cruise_srv_t* pCruiseSrv = NULL;
    char strCruiseID[32] = {0};

    if (NULL == pCruise_Srv_dboper || NULL == pUserInfo)
    {
        return -1;
    }

    cruise_pos = cruise_srv_find(id);

    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id() cruise_srv_find:id=%u, cruise_pos=%d \r\n", id, cruise_pos);

    if (cruise_pos < 0)
    {
        snprintf(strCruiseID, 32, "%u", id);
        strSQL.clear();
        strSQL = "select * from CruiseConfig WHERE ID = ";
        strSQL += strCruiseID;

        record_count = pCruise_Srv_dboper->DB_Select(strSQL.c_str(), 1);

        if (record_count < 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() ErrorMsg=%s\r\n", pCruise_Srv_dboper->GetLastDbErrorMsg());
            return -1;
        }
        else if (record_count == 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "start_cruise_srv_by_id() exit---: No Record Count \r\n");
            return 0;
        }


        unsigned int uCruiseID = 0;
        string strCruiseName = "";
        int iStartTime = 0;
        int iDurationTime = 0;

        pCruise_Srv_dboper->GetFieldValue("ID", uCruiseID);
        pCruise_Srv_dboper->GetFieldValue("CruiseName", strCruiseName);
        pCruise_Srv_dboper->GetFieldValue("StartTime", iStartTime);
        pCruise_Srv_dboper->GetFieldValue("DurationTime", iDurationTime);

        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id() CruiseID=%u, CruiseName=%s, StartTime=%d, DurationTime=%d \r\n", uCruiseID, strCruiseName.c_str(), iStartTime, iDurationTime);

        i = cruise_srv_init(&pCruiseSrv);

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() cruise_srv_init:i=%d \r\n", i);
            return -1;
        }

        pCruiseSrv->status = 2;
        pCruiseSrv->cruise_type = 0;
        pCruiseSrv->cruise_id = uCruiseID;

        if (!strCruiseName.empty())
        {
            osip_strncpy(pCruiseSrv->cruise_name, (char*)strCruiseName.c_str(), MAX_128CHAR_STRING_LEN);
        }

        pCruiseSrv->start_time = iStartTime;
        pCruiseSrv->duration_time = iDurationTime;
        pCruiseSrv->duration_time_count = 0;
        pCruiseSrv->current_pos = 0;

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "添加手动触发执行的巡航: 巡航ID=%u, 巡航名称=%s, 开始时间=%d, 持续时间=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Add a manually triggered navigation: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);

        /* 添加动作数据到队列 */
        i = add_cruise_action_data_to_srv_list_proc(pCruiseSrv->cruise_id, pCruiseSrv->pCruiseActionList, pCruise_Srv_dboper);

        if (i < 0)
        {
            cruise_srv_free(pCruiseSrv);
            pCruiseSrv = NULL;
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() add_cruise_action_data_to_srv_list_proc:i=%d \r\n", i);
            return -1;
        }

        /* 添加到队列 */
        if (cruise_srv_add(pCruiseSrv) < 0)
        {
            cruise_srv_free(pCruiseSrv);
            pCruiseSrv = NULL;
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() Cruise Srv Add Error");
            return -1;
        }

        /* 通知客户端 */
        if (0 == pCruiseSrv->send_mark)
        {
            pCruiseSrv->send_mark = 1;

            /* 更新状态 */
            iRet = UpdateCruiseConfigStatus2DB(pCruiseSrv->cruise_id, 1, pCruise_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() UpdateCruiseConfigStatus2DB Start Error: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id() UpdateCruiseConfigStatus2DB Start OK: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
            }
        }
    }
    else
    {
        pCruiseSrv = cruise_srv_get(cruise_pos);

        if (NULL != pCruiseSrv)
        {
            if (0 == pCruiseSrv->status || 3 == pCruiseSrv->status)
            {
                pCruiseSrv->status = 2;
                pCruiseSrv->cruise_type = 0;
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "手动触发执行的巡航: 巡航ID=%u, 巡航名称=%s, 开始时间=%d, 持续时间=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Manually triggered navigation: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);

                /* 通知客户端 */
                if (0 == pCruiseSrv->send_mark)
                {
                    pCruiseSrv->send_mark = 1;

                    /* 更新状态 */
                    iRet = UpdateCruiseConfigStatus2DB(pCruiseSrv->cruise_id, 1, pCruise_Srv_dboper);

                    if (iRet < 0)
                    {
                        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() UpdateCruiseConfigStatus2DB Start Error: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id() UpdateCruiseConfigStatus2DB Start OK: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
                    }
                }

                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id() CruiseID=%u, CruiseName=%s, StartTime=%d, DurationTime=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
            }
            else if (1 == pCruiseSrv->status) /* 再次发送一下 */
            {
                if (0 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "手动触发执行的巡航已经正在执行: 巡航ID=%u, 巡航名称=%s, 开始时间=%d, 持续时间=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Manually triggered navigation being executed already: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d:", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                }
                else if (1 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "手动触发执行的巡航已经被预案触发正在执行, 预案巡航执行完成之后会继续执行: 巡航ID=%u, 巡航名称=%s, 开始时间=%d, 持续时间=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Manually triggered navigation being executed by plan trigger already, will continue execute after completing plan cruise: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d:", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    pCruiseSrv->cruise_type = 3;
                }
                else if (2 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "手动触发执行的巡航已经被预案触发中断执行, 预案巡航执行完成之后会继续执行: 巡航ID=%u, 巡航名称=%s, 开始时间=%d, 持续时间=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Manually triggered navigation being executed by plan trigger interrupt already, will continue execute after completing plan cruise: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d:", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                }
                else if (3 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "手动触发执行的巡航已经被预案触发正在执行, 预案巡航执行完成之后会继续执行: 巡航ID=%u, 巡航名称=%s, 开始时间=%d, 持续时间=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Manually triggered navigation being executed by plan trigger already, will continue execute after completing plan cruise: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d:", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                }
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() CruiseID=%u, status=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->status);
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id() cruise_srv_get Error:cruise_pos=%d \r\n", cruise_pos);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : start_cruise_srv_by_id_for_plan
 功能描述  : 预案触发执行巡航
 输入参数  : unsigned int id
             DBOper* pCruise_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月30日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int start_cruise_srv_by_id_for_plan(unsigned int id, DBOper* pCruise_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    int cruise_pos = -1;
    string strSQL = "";
    int record_count = 0;
    cruise_srv_t* pCruiseSrv = NULL;
    char strCruiseID[32] = {0};

    if (NULL == pCruise_Srv_dboper)
    {
        return -1;
    }

    cruise_pos = cruise_srv_find(id);

    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id_for_plan() cruise_srv_find:id=%u, cruise_pos=%d \r\n", id, cruise_pos);

    if (cruise_pos < 0)
    {
        snprintf(strCruiseID, 32, "%u", id);
        strSQL.clear();
        strSQL = "select * from CruiseConfig WHERE ID = ";
        strSQL += strCruiseID;

        record_count = pCruise_Srv_dboper->DB_Select(strSQL.c_str(), 1);

        if (record_count < 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "预案触发添加执行的巡航失败: 巡航ID=%u, 原因=%s:%s", id, (char*)"查询数据库失败", pCruise_Srv_dboper->GetLastDbErrorMsg());
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fail to excute cruise that plan trigger adds: cruise_ID=%u, cause=%s:", id, (char*)"Failed to query the database", pCruise_Srv_dboper->GetLastDbErrorMsg());

            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() ErrorMsg=%s\r\n", pCruise_Srv_dboper->GetLastDbErrorMsg());
            return -1;
        }
        else if (record_count == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "预案触发添加执行的巡航失败: 巡航ID=%u, 原因=%s", id, (char*)"没有查询到巡航数据库记录");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to excute cruise that plan trigger adds: cruise_ID=%u, cause=%s:", id, (char*)"Failed to query the database record of cruise");

            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "start_cruise_srv_by_id_for_plan() exit---: No Record Count \r\n");
            return 0;
        }


        unsigned int uCruiseID = 0;
        string strCruiseName = "";
        int iStartTime = 0;
        int iDurationTime = 0;

        pCruise_Srv_dboper->GetFieldValue("ID", uCruiseID);
        pCruise_Srv_dboper->GetFieldValue("CruiseName", strCruiseName);
        pCruise_Srv_dboper->GetFieldValue("StartTime", iStartTime);
        pCruise_Srv_dboper->GetFieldValue("DurationTime", iDurationTime);

        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id_for_plan() CruiseID=%u, CruiseName=%s, StartTime=%d, DurationTime=%d \r\n", uCruiseID, strCruiseName.c_str(), iStartTime, iDurationTime);

        i = cruise_srv_init(&pCruiseSrv);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "预案触发添加执行的巡航失败: 巡航ID=%u, 原因=%s", id, (char*)"巡航数据初始化失败");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to excute cruise that plan trigger adds: cruise_ID=%u, cause=%s:", id, (char*)"Cruise data failed to initialize");

            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() cruise_srv_init:i=%d \r\n", i);
            return -1;
        }

        pCruiseSrv->status = 2;
        pCruiseSrv->cruise_type = 1;
        pCruiseSrv->cruise_id = uCruiseID;

        if (!strCruiseName.empty())
        {
            osip_strncpy(pCruiseSrv->cruise_name, (char*)strCruiseName.c_str(), MAX_128CHAR_STRING_LEN);
        }

        pCruiseSrv->start_time = iStartTime;
        pCruiseSrv->duration_time = iDurationTime;
        pCruiseSrv->duration_time_count = 0;
        pCruiseSrv->current_pos = 0;

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "预案触发添加执行的巡航: 巡航ID=%u, 巡航名称=%s, 开始时间=%d, 持续时间=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Add a manually triggered navigation: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);

        /* 添加动作数据到队列 */
        i = add_cruise_action_data_to_srv_list_proc(pCruiseSrv->cruise_id, pCruiseSrv->pCruiseActionList, pCruise_Srv_dboper);

        if (i < 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "预案触发添加执行的巡航失败: 巡航ID=%u, 原因=%s", pCruiseSrv->cruise_id, (char*)"添加巡航动作数据失败");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to excute cruise that plan trigger adds: cruise_ID=%u, cause=%s:", pCruiseSrv->cruise_id, (char*)"Fail to add cruise operation data");

            cruise_srv_free(pCruiseSrv);
            pCruiseSrv = NULL;
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() add_cruise_action_data_to_srv_list_proc:i=%d \r\n", i);
            return -1;
        }

        /* 添加到队列 */
        if (cruise_srv_add(pCruiseSrv) < 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "预案触发添加执行的巡航失败: 巡航ID=%u, 原因=%s", pCruiseSrv->cruise_id, (char*)"添加巡航数据失败");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Fail to excute cruise that plan trigger adds: cruise_ID=%u, cause=%s:", pCruiseSrv->cruise_id, (char*)"Fail to add cruise data");

            cruise_srv_free(pCruiseSrv);
            pCruiseSrv = NULL;
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() Cruise Srv Add Error");
            return -1;
        }

        /* 通知客户端 */
        if (0 == pCruiseSrv->send_mark)
        {
            pCruiseSrv->send_mark = 1;

            /* 更新状态 */
            iRet = UpdateCruiseConfigStatus2DB(pCruiseSrv->cruise_id, 1, pCruise_Srv_dboper);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() UpdateCruiseConfigStatus2DB Start Error: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id_for_plan() UpdateCruiseConfigStatus2DB Start OK: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
            }
        }
    }
    else
    {
        pCruiseSrv = cruise_srv_get(cruise_pos);

        if (NULL != pCruiseSrv)
        {
            if (0 == pCruiseSrv->status || 3 == pCruiseSrv->status)
            {
                pCruiseSrv->status = 2;
                pCruiseSrv->cruise_type = 1;
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "预案触发添加执行的巡航: 巡航ID=%u, 巡航名称=%s, 开始时间=%d, 持续时间=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Manually triggered navigation: cruise_id=%u, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);

                /* 通知客户端 */
                if (0 == pCruiseSrv->send_mark)
                {
                    pCruiseSrv->send_mark = 1;

                    /* 更新状态 */
                    iRet = UpdateCruiseConfigStatus2DB(pCruiseSrv->cruise_id, 1, pCruise_Srv_dboper);

                    if (iRet < 0)
                    {
                        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() UpdateCruiseConfigStatus2DB Start Error: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id_for_plan() UpdateCruiseConfigStatus2DB Start OK: cruise_id=%u, iRet=%d", pCruiseSrv->cruise_id, iRet);
                    }
                }

                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "start_cruise_srv_by_id_for_plan() CruiseID=%u, CruiseName=%s, StartTime=%d, DurationTime=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
            }
            else if (1 == pCruiseSrv->status) /* 再次发送一下 */
            {
                if (0 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "预案触发添加执行的巡航正在执行, 中断执行原有巡航, 优先执行预案巡航之后再次执行原有巡航: 巡航ID=%u, 巡航名称=%s, 开始时间=%d, 持续时间=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Plans to add cruise trigger execution is being executed, the interrupt execution of the original cruise, priority implementation plan after the implementation of the existing cruise cruise again: cruise_ID=%u, cruise_name=%s, start_time=%d, during_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    pCruiseSrv->status = 2;
                    pCruiseSrv->cruise_type = 2;
                }
                else if (1 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "预案触发添加执行的巡航正在执行, 再次触发重新执行, 巡航ID=%u, 巡航名称=%s, 开始时间=%d, 持续时间=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Trigger execution plan to add cruise being implemented, re-execute the trigger again, cruise_ID=%u, cruise_name=%s, start_time=%d, during_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    pCruiseSrv->status = 2;
                }
                else if (2 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "预案触发添加执行的巡航正在执行, 再次触发重新执行, 巡航ID=%u, 巡航名称=%s, 开始时间=%d, 持续时间=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Trigger execution plan to add cruise being implemented, re-execute the trigger again, cruise_ID=%u, cruise_name=%s, start_time=%d, during_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    pCruiseSrv->status = 2;
                }
                else if (3 == pCruiseSrv->cruise_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "预案触发添加执行的巡航正在执行, 再次触发重新执行, 巡航ID=%u, 巡航名称=%s, 开始时间=%d, 持续时间=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Trigger execution plan to add cruise being implemented, re-execute the trigger again, cruise_ID=%u, cruise_name=%s, start_time=%d, during_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    pCruiseSrv->status = 2;
                }
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() CruiseID=%u, status=%d \r\n", pCruiseSrv->cruise_id, pCruiseSrv->status);
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "start_cruise_srv_by_id_for_plan() cruise_srv_get Error:cruise_pos=%d \r\n", cruise_pos);
        }
    }

    return 0;
}


/*****************************************************************************
 函 数 名  : stop_cruise_srv_by_id
 功能描述  : 根据ID停止巡航任务
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
int stop_cruise_srv_by_id(unsigned int id)
{
    int pos = -1;
    cruise_srv_t* pCruiseSrv = NULL;

    pos = cruise_srv_find(id);

    if (pos >= 0)
    {
        pCruiseSrv = cruise_srv_get(pos);

        if (NULL != pCruiseSrv)
        {
            if (1 == pCruiseSrv->status || 4 == pCruiseSrv->status)
            {
                pCruiseSrv->status = 3;
                pCruiseSrv->send_mark = 0;
            }
            else if (2 == pCruiseSrv->status)
            {
                pCruiseSrv->status = 0;
                pCruiseSrv->send_mark = 0;
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "stop_cruise_srv_by_id() cruise_id=%u, status=%d\r\n", pCruiseSrv->cruise_id, pCruiseSrv->status);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : set_cruise_srv_list_del_mark
 功能描述  : 设置巡航业务删除标识
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
int set_cruise_srv_list_del_mark(int del_mark)
{
    int pos1 = 0;
    int pos2 = 0;
    cruise_srv_t* pCruiseSrv = NULL;
    cruise_action_t* pCruiseAction = NULL;

    if ((NULL == g_CruiseSrvList) || (NULL == g_CruiseSrvList->pCruiseSrvList))
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "set_cruise_srv_list_del_mark() exit---: Param Error \r\n");
        return -1;
    }

    CRUISE_SMUTEX_LOCK();

    if (osip_list_size(g_CruiseSrvList->pCruiseSrvList) <= 0)
    {
        CRUISE_SMUTEX_UNLOCK();
        return 0;
    }

    for (pos1 = 0; pos1 < osip_list_size(g_CruiseSrvList->pCruiseSrvList); pos1++)
    {
        pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, pos1);

        if (NULL == pCruiseSrv)
        {
            continue;
        }

        pCruiseSrv->del_mark = del_mark;

        for (pos2 = 0; pos2 < osip_list_size(pCruiseSrv->pCruiseActionList); pos2++)
        {
            pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseSrv->pCruiseActionList, pos2);

            if (NULL == pCruiseAction)
            {
                continue;
            }

            pCruiseAction->del_mark = del_mark;
        }
    }

    CRUISE_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : check_db_data_to_cruise_srv_list
 功能描述  : 从数据中检测是否有需要执行的巡航数据，如果有，则加载到内存中
 输入参数  : DBOper* pCruise_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int check_db_data_to_cruise_srv_list(DBOper* pCruise_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    string strSQL = "";
    int record_count = 0;
    time_t now = time(NULL);
    int iTimeNow = 0;
    struct tm tp = {0};
    int while_count = 0;
    cruise_srv_t* pCruiseSrv2 = NULL;

    if (NULL == pCruise_Srv_dboper)
    {
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from CruiseConfig order by StartTime asc";

    record_count = pCruise_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "check_db_data_to_cruise_srv_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "check_db_data_to_cruise_srv_list() ErrorMsg=%s\r\n", pCruise_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "check_db_data_to_cruise_srv_list_for_start() exit---: No Record Count \r\n");
        return 0;
    }

    localtime_r(&now, &tp);
    iTimeNow = tp.tm_hour * 3600 + tp.tm_min * 60 + tp.tm_sec;

    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "check_db_data_to_cruise_srv_list:record_count=%d \r\n", record_count);

    do
    {
        int i = 0;
        unsigned int uCruiseID = 0;
        string strCruiseName = "";
        int iOldStartTime = 0;
        int iStartTime = 0;
        int iDurationTime = 0;
        int iScheduledRun = 0;
        int cruise_pos = -1;
        int iResved1 = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "check_db_data_to_cruise_srv_list() While Count=%d \r\n", while_count);
        }

        pCruise_Srv_dboper->GetFieldValue("ID", uCruiseID);
        pCruise_Srv_dboper->GetFieldValue("CruiseName", strCruiseName);
        pCruise_Srv_dboper->GetFieldValue("StartTime", iStartTime);
        pCruise_Srv_dboper->GetFieldValue("DurationTime", iDurationTime);
        pCruise_Srv_dboper->GetFieldValue("ScheduledRun", iScheduledRun);
        pCruise_Srv_dboper->GetFieldValue("Resved1", iResved1);

        /* 查找队列，看队列里面是否已经存在 */
        cruise_pos = cruise_srv_find(uCruiseID);

        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "check_db_data_to_cruise_srv_list() CruiseID=%u:cruise_pos=%d \r\n", uCruiseID, cruise_pos);

        if (cruise_pos < 0) /* 添加到要执行队列 */
        {
            cruise_srv_t* pCruiseSrv = NULL;

            i = cruise_srv_init(&pCruiseSrv);

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "check_db_data_to_cruise_srv_list() cruise_srv_init:i=%d \r\n", i);
                continue;
            }

            pCruiseSrv->cruise_id = uCruiseID;

            if (!strCruiseName.empty())
            {
                osip_strncpy(pCruiseSrv->cruise_name, (char*)strCruiseName.c_str(), MAX_128CHAR_STRING_LEN);
            }

            pCruiseSrv->start_time = iStartTime;
            pCruiseSrv->duration_time = iDurationTime;
            pCruiseSrv->del_mark = 0;

            if ((iTimeNow == iStartTime) || (iTimeNow > iStartTime && iTimeNow - iStartTime < 30)) /* 30秒之内的才启动 */
            {
                if (iScheduledRun)
                {
                    pCruiseSrv->status = 2;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加定时触发执行的巡航: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add timed navigation: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                }
            }
            else if (0 == iStartTime && 0 == iDurationTime && 1 == iResved1) /* 手动执行的，状态是1的需要启动 */
            {
                if (!iScheduledRun)
                {
                    pCruiseSrv->status = 2;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加手动促发执行的但是没有停止的巡航: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add a manually triggered but not stopped navigation: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                }
            }

            /* 添加到队列 */
            if (cruise_srv_add(pCruiseSrv) < 0)
            {
                cruise_srv_free(pCruiseSrv);
                pCruiseSrv = NULL;
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "check_db_data_to_cruise_srv_list() Cruise Srv Add Error\r\n");
                continue;
            }
            else
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_INFO, "check_db_data_to_cruise_srv_list() cruise_srv_add:cruise_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pCruiseSrv->cruise_id, iStartTime, iDurationTime, pCruiseSrv->status);
            }
        }
        else
        {
            cruise_srv_t* pCruiseSrv = NULL;

            pCruiseSrv = cruise_srv_get(cruise_pos);

            if (NULL != pCruiseSrv)
            {
                /* 看数据是否有变化 */
                if (!strCruiseName.empty())
                {
                    if (0 != sstrcmp(pCruiseSrv->cruise_name, (char*)strCruiseName.c_str()))
                    {
                        memset(pCruiseSrv->cruise_name, 0, MAX_128CHAR_STRING_LEN + 4);
                        osip_strncpy(pCruiseSrv->cruise_name, (char*)strCruiseName.c_str(), MAX_128CHAR_STRING_LEN);
                    }
                }
                else
                {
                    memset(pCruiseSrv->cruise_name, 0, MAX_128CHAR_STRING_LEN + 4);
                }

                iOldStartTime = pCruiseSrv->start_time;
                pCruiseSrv->start_time = iStartTime;
                pCruiseSrv->duration_time = iDurationTime;
                pCruiseSrv->del_mark = 0;

                if (0 == pCruiseSrv->status || 3 == pCruiseSrv->status)
                {
                    if ((iTimeNow == iStartTime) || (iTimeNow > iStartTime && iTimeNow - iStartTime < 30)) /* 30秒之内的才启动 */
                    {
                        if (iScheduledRun)
                        {
                            pCruiseSrv->status = 2;
                            //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_INFO, "check_db_data_to_cruise_srv_list() cruise_srv_add:cruise_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pCruiseSrv->cruise_id, iStartTime, iDurationTime, pCruiseSrv->status);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加定时促发执行的巡航: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add timed navigation: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                        }
                    }
                    else if (0 == iStartTime && 0 == iDurationTime && 1 == iResved1) /* 手动执行的，状态是1的需要启动 */
                    {
                        if (!iScheduledRun)
                        {
                            pCruiseSrv->status = 2;
                            //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_INFO, "check_db_data_to_cruise_srv_list() cruise_srv_add:cruise_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pCruiseSrv->cruise_id, iStartTime, iDurationTime, pCruiseSrv->status);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加手动促发执行的但是没有停止的巡航: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Add a manually triggered but not stopped navigation: cruise_id=%d, cruise_name=%s, start_time=%d, duration_time=%d", pCruiseSrv->cruise_id, pCruiseSrv->cruise_name, pCruiseSrv->start_time, pCruiseSrv->duration_time);

                        }
                    }
                }
                else if (1 == pCruiseSrv->status) /* 可能修改了启动时间，再发送一下 */
                {
                    if ((iStartTime != iOldStartTime) && ((iTimeNow == iStartTime) || (iTimeNow > iStartTime && iTimeNow - iStartTime < 30))) /* 30秒之内的才启动 */
                    {
                        if (iScheduledRun)
                        {
                            pCruiseSrv->status = 4;
                            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_INFO, "check_db_data_to_cruise_srv_list() cruise_srv_add:cruise_id=%u, StartTime=%d,DurationTime=%d,Status=%d, \r\n", pCruiseSrv->cruise_id, iStartTime, iDurationTime, pCruiseSrv->status);
                        }
                    }
                }
            }
        }
    }
    while (pCruise_Srv_dboper->MoveNext() >= 0);

    /* 添加目的数据和源数据 */
    for (i = 0; i < osip_list_size(g_CruiseSrvList->pCruiseSrvList); i++)
    {
        pCruiseSrv2 = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, i);

        if (NULL == pCruiseSrv2)
        {
            continue;
        }

        /* 添加动作数据到队列 */
        iRet = add_cruise_action_data_to_srv_list_proc(pCruiseSrv2->cruise_id, pCruiseSrv2->pCruiseActionList, pCruise_Srv_dboper);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "check_db_data_to_cruise_srv_list() add_cruise_action_data_to_srv_list_proc Error:cruise_id=%u \r\n", pCruiseSrv2->cruise_id);
        }
        else
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "check_db_data_to_cruise_srv_list() add_cruise_action_data_to_srv_list_proc:cruise_id=%u, iRet=%d \r\n", pCruiseSrv2->cruise_id, iRet);
        }
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : add_cruise_action_data_to_srv_list_proc
 功能描述  : 添加巡航动作的数据到巡航业务队列
 输入参数  : int cruise_id
             osip_list_t* pCruiseActionList
             DBOper* pCruise_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int add_cruise_action_data_to_srv_list_proc(unsigned int cruise_id, osip_list_t* pCruiseActionList, DBOper* pCruise_Srv_dboper)
{
    int i = 0;
    int pos = -1;
    int iRet = 0;
    int record_count = 0;
    char strCruiseID[32] = {0};

    string strSQL = "";
    int while_count = 0;

    if (NULL == pCruise_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "add_cruise_action_data_to_srv_list_proc() exit---: Cruise Srv DB Oper Error \r\n");
        return -1;
    }

    if (NULL == pCruiseActionList)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "add_cruise_action_data_to_srv_list_proc() exit---: Cruise Action List Error \r\n");
        return -1;
    }

    /* 根据cruise_id，查询巡航动作表，获取巡航的具体数据 */
    strSQL.clear();
    snprintf(strCruiseID, 32, "%u", cruise_id);
    strSQL = "select * from CruiseActionConfig WHERE CruiseID = ";
    strSQL += strCruiseID;
    strSQL += " order by SortID asc";

    record_count = pCruise_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "add_cruise_action_data_to_srv_list_proc:CruiseID=%u, record_count=%d \r\n", cruise_id, record_count);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "add_cruise_action_data_to_srv_list_proc() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "add_cruise_action_data_to_srv_list_proc() ErrorMsg=%s\r\n", pCruise_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "add_cruise_action_dest_data_to_srv_list_proc() exit---: No Record Count \r\n");
        return 0;
    }

    /* 循环添加巡航动作数据 */
    do
    {
        unsigned int iDeviceIndx = 0;
        unsigned int iPresetID = 0;
        int iLiveTime = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "add_cruise_action_data_to_srv_list_proc() While Count=%d \r\n", while_count);
        }

        pCruise_Srv_dboper->GetFieldValue("DeviceIndex", iDeviceIndx);
        pCruise_Srv_dboper->GetFieldValue("PresetID", iPresetID);
        pCruise_Srv_dboper->GetFieldValue("LiveTime", iLiveTime);

        if (iDeviceIndx <= 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "add_cruise_action_data_to_srv_list_proc() DeviceIndx Empty \r\n");
            continue;
        }

        if (iPresetID <= 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "add_cruise_action_data_to_srv_list_proc() PresetID Empty \r\n");
            continue;
        }

        if (iLiveTime <= 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "add_cruise_action_data_to_srv_list_proc() LiveTime Empty \r\n");
            continue;
        }

        /* 根据DeviceIndex查找动作表 */
        pos = cruise_action_find(iDeviceIndx, iPresetID, pCruiseActionList);

        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "add_cruise_action_data_to_srv_list_proc() DeviceIndex=%u, PresetID=%d, pos=%d \r\n", iDeviceIndx, iPresetID, pos);

        if (pos < 0)
        {
            cruise_action_t* pCruiseAction = NULL;

            /* 添加巡航动作 */
            iRet = cruise_action_init(&pCruiseAction);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "add_cruise_action_data_to_srv_list_proc() Cruise Action Init Error \r\n");
                continue;
            }

            pCruiseAction->device_index = iDeviceIndx;
            pCruiseAction->iPresetID = iPresetID;
            pCruiseAction->iStatus = 0;
            pCruiseAction->iLiveTime = iLiveTime;
            pCruiseAction->iLiveTimeCount = 0;
            pCruiseAction->del_mark = 0;

            /* 添加到队列 */
            i = osip_list_add(pCruiseActionList, pCruiseAction, -1); /* add to list tail */

            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "add_cruise_action_data_to_srv_list_proc() CruiseAction:CruiseID=%u, device_index=%u, i=%d \r\n", cruise_id, pCruiseAction->device_index, i);

            if (i < 0)
            {
                cruise_action_free(pCruiseAction);
                pCruiseAction = NULL;
                DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "add_cruise_action_data_to_srv_list_proc() Cruise Action Add Error \r\n");
                continue;
            }
        }
        else
        {
            cruise_action_t* pCruiseAction = NULL;

            pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseActionList, pos);

            if (NULL != pCruiseAction)
            {
                pCruiseAction->del_mark = 0;

                if (0 == pCruiseAction->iStatus)
                {
                    pCruiseAction->iPresetID = iPresetID;
                    pCruiseAction->iLiveTime = iLiveTime;
                    pCruiseAction->iLiveTimeCount = 0;
                }
            }
        }
    }
    while (pCruise_Srv_dboper->MoveNext() >= 0);

    DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "add_cruise_action_data_to_srv_list_proc:CruiseActionList.size()=%d \r\n", osip_list_size(pCruiseActionList));

    return osip_list_size(pCruiseActionList);
}

/*****************************************************************************
 函 数 名  : delete_cruise_srv_data
 功能描述  : 删除运行结束的巡航数据
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
void delete_cruise_srv_data()
{
    int pos1 = 0;
    int pos2 = 0;
    cruise_srv_t* pCruiseSrv = NULL;
    cruise_action_t* pCruiseAction = NULL;

    if ((NULL == g_CruiseSrvList) || (NULL == g_CruiseSrvList->pCruiseSrvList))
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "delete_cruise_srv_data() exit---: Param Error \r\n");
        return;
    }

    CRUISE_SMUTEX_LOCK();

    if (osip_list_size(g_CruiseSrvList->pCruiseSrvList) <= 0)
    {
        CRUISE_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "delete_cruise_srv_data() exit---: Cruise Srv List NULL \r\n");
        return;
    }

    pos1 = 0;

    while (!osip_list_eol(g_CruiseSrvList->pCruiseSrvList, pos1))
    {
        pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, pos1);

        if (NULL == pCruiseSrv)
        {
            osip_list_remove(g_CruiseSrvList->pCruiseSrvList, pos1);
            continue;
        }

        if (1 == pCruiseSrv->del_mark) /* 删除巡航 */
        {
            osip_list_remove(g_CruiseSrvList->pCruiseSrvList, pos1);
            cruise_srv_free(pCruiseSrv);
            pCruiseSrv = NULL;
        }
        else
        {
            /* 时间信息队列 */
            if (NULL == pCruiseSrv->pCruiseActionList)
            {
                pos1++;
                continue;
            }

            if (osip_list_size(pCruiseSrv->pCruiseActionList) <= 0)
            {
                pos1++;
                continue;
            }

            pos2 = 0;

            while (!osip_list_eol(pCruiseSrv->pCruiseActionList, pos2))
            {
                pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseSrv->pCruiseActionList, pos2);

                if (NULL == pCruiseAction)
                {
                    osip_list_remove(pCruiseSrv->pCruiseActionList, pos2);
                    continue;
                }

                if (1 == pCruiseAction->del_mark) /* 删除巡航 */
                {
                    if (pCruiseSrv->current_pos > pos2) /* 如果巡航的点是需要删除点之后位置，则位置需要减掉一 */
                    {
                        pCruiseSrv->current_pos--;
                    }

                    osip_list_remove(pCruiseSrv->pCruiseActionList, pos2);
                    cruise_action_free(pCruiseAction);
                    pCruiseAction = NULL;
                }
                else
                {
                    pos2++;
                }
            }

            pos1++;
        }
    }

    CRUISE_SMUTEX_UNLOCK();

    return;
}

/*****************************************************************************
 函 数 名  : SendNotifyExecuteCruiseActionToOnlineUser
 功能描述  : 发送巡航执行通知给在线客户端
 输入参数  : int iCruiseID
             char* cruise_name
             int iType
             DBOper* pCruise_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendNotifyExecuteCruiseActionToOnlineUser(unsigned int uCruiseID, char* cruise_name, int iType, DBOper* pCruise_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    int index = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    char strCruiseID[32] = {0};
    vector<unsigned int> UserIndexVector;
    int iUserIndexCount = 0;
    unsigned int uUserIndex = 0;

    /*
     <?xml version="1.0"?>
         <Notify>
         <CmdType>ExecuteCruise</CmdType>
         <SN>1234</SN>
         <CruiseID>轮询ID</CruiseID>
         </Notify>
     */

    /* 组建XML信息 */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"ExecuteCruise");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"1234");

    AccNode = outPacket.CreateElement((char*)"CruiseID");
    snprintf(strCruiseID, 32, "%u", uCruiseID);
    outPacket.SetElementValue(AccNode, strCruiseID);

    AccNode = outPacket.CreateElement((char*)"CruiseName");

    if (NULL == cruise_name)
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }
    else
    {
        outPacket.SetElementValue(AccNode, cruise_name);
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

    /* 获取cruise用户权限表 */
    UserIndexVector.clear();
    iRet = get_user_index_from_user_cruise_config(strCruiseID, UserIndexVector, pCruise_Srv_dboper);

    iUserIndexCount = UserIndexVector.size();

    if (iUserIndexCount <= 0)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "SendNotifyExecuteCruiseActionToOnlineUser() exit---: Get User Index NULL \r\n");
        return 0;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送巡航执行通知给在线客户端: 查询到的用户索引总数=%d", iUserIndexCount);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send cruising executing notice to the client online: The total number of queries to the user index=%d", iUserIndexCount);

    /* 循环发送数据 */
    for (index = 0; index < iUserIndexCount; index++)
    {
        /* 获取用户索引 */
        uUserIndex = UserIndexVector[index];

        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_TRACE, "SendNotifyExecuteCruiseActionToOnlineUser() index=%d, UserIndex=%u \r\n", index, uUserIndex);

        i |= SendMessageToOnlineUserByUserIndex(uUserIndex, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());
    }

    return i;
}

/*****************************************************************************
 函 数 名  : UpdateCruiseConfigStatus2DB
 功能描述  : 更新巡航配置状态到数据库
 输入参数  : int cruise_id
             int status
             DBOper* pCruise_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月31日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int UpdateCruiseConfigStatus2DB(int cruise_id, int status, DBOper* pCruise_Srv_dboper)
{
    int iRet = 0;
    string strSQL = "";
    char strCruiseID[64] = {0};
    char strStatus[16] = {0};

    //printf("\r\n UpdateUserRegInfo2DB() Enter--- \r\n");

    if (cruise_id <= 0)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "UpdateCruiseConfigStatus2DB() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pCruise_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "UpdateCruiseConfigStatus2DB() exit---: Cruise Srv DB Oper Error \r\n");
        return -1;
    }

    snprintf(strCruiseID, 64, "%d", cruise_id);
    snprintf(strStatus, 16, "%d", status);

    /* 更新数据库 */
    strSQL.clear();
    strSQL = "UPDATE CruiseConfig SET Resved1 = ";
    strSQL += strStatus;
    strSQL += " WHERE ID = ";
    strSQL += strCruiseID;

    iRet = pCruise_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "UpdateCruiseConfigStatus2DB() DB Oper Error: strSQL=%s, iRet=%d \r\n", strSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "UpdateCruiseConfigStatus2DB() ErrorMsg=%s\r\n", pCruise_Srv_dboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : get_user_index_from_user_cruise_config
 功能描述  : 从用户巡航权限表里面获取用户索引
 输入参数  : char* pcCruiseID
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
int get_user_index_from_user_cruise_config(char* pcCruiseID, vector<unsigned int>& UserIndexVector, DBOper* pDBOper)
{
    int iRet = 0;
    int record_count = 0;
    int while_count = 0;
    string strSQL = "";

    if (NULL == pcCruiseID || NULL == pDBOper)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_DEBUG, "get_user_index_from_user_cruise_config() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "SELECT UserID FROM UserCruiseConfig WHERE CruiseID = ";
    strSQL += pcCruiseID;

    record_count = pDBOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "get_user_index_from_user_cruise_config() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_ERROR, "get_user_index_from_user_cruise_config() ErrorMsg=%s\r\n", pDBOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "get_user_index_from_user_cruise_config() No Record \r\n");
        return 0;
    }

    /* 循环查找数据库*/
    do
    {
        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_CRUISE_SRV, LOG_WARN, "get_user_index_from_user_cruise_config() While Count=%d \r\n", while_count);
        }

        unsigned int uUserIndex = 0;

        pDBOper->GetFieldValue("UserID", uUserIndex);

        iRet = AddUserIndexToUserIndexVector(UserIndexVector, uUserIndex);
    }
    while (pDBOper->MoveNext() >= 0);

    return 0;
}

/*****************************************************************************
 函 数 名  : ShowCruiseTaskInfo
 功能描述  : 显示当前巡航任务信息
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
void ShowCruiseTaskInfo(int sock, int status)
{
    int i = 0, j = 0;
    char strLine[] = "\r--------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rCruiseID StartTime DurationTime DurationTimeCount CurrentPos DeviceIndex PresetID LiveTime LiveTimeCount\r\n";
    cruise_srv_t* pCruiseSrv = NULL;
    cruise_action_t* pCruiseAction = NULL;
    char rbuf[128] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if ((NULL == g_CruiseSrvList) || (NULL == g_CruiseSrvList->pCruiseSrvList))
    {
        return;
    }

    CRUISE_SMUTEX_LOCK();

    if (osip_list_size(g_CruiseSrvList->pCruiseSrvList) <= 0)
    {
        CRUISE_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_CruiseSrvList->pCruiseSrvList); i++)
    {
        pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, i);

        if (NULL == pCruiseSrv || NULL == pCruiseSrv->pCruiseActionList)
        {
            continue;
        }

        if (status <= 1)
        {
            if (pCruiseSrv->status != status) /* 没有巡航的忽略 */
            {
                continue;
            }
        }

        /* 查找具体巡航动作 */
        for (j = 0; j < osip_list_size(pCruiseSrv->pCruiseActionList); j++)
        {
            pCruiseAction = (cruise_action_t*)osip_list_get(pCruiseSrv->pCruiseActionList, j);

            if (NULL == pCruiseAction)
            {
                continue;
            }

            if (status <= 1)
            {
                if (1 == status && j != pCruiseSrv->current_pos)  /* 不是当前正在当前巡航的忽略 */
                {
                    continue;
                }

                if (status != pCruiseAction->iStatus) /* 没有启动的巡航忽略 */
                {
                    continue;
                }
            }

            snprintf(rbuf, 128, "\r%-8u %-9u %-12d %-17d %-10d %-11u %-8d %-8u %-13d\r\n", pCruiseSrv->cruise_id, pCruiseSrv->start_time, pCruiseSrv->duration_time, pCruiseSrv->duration_time_count, pCruiseSrv->current_pos, pCruiseAction->device_index, pCruiseAction->iPresetID, pCruiseAction->iLiveTime, pCruiseAction->iLiveTimeCount);

            if (sock > 0)
            {
                send(sock, rbuf, strlen(rbuf), 0);
            }
        }
    }

    CRUISE_SMUTEX_UNLOCK();

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

/*****************************************************************************
 函 数 名  : StopCruiseTask
 功能描述  : 停止巡航任务
 输入参数  : int sock
             unsigned int cruise_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月7日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StopCruiseTask(int sock, unsigned int cruise_id)
{
    int iRet = 0;
    char rbuf[128] = {0};

    /* 停止业务 */
    iRet = stop_cruise_srv_by_id(cruise_id);

    if (sock > 0)
    {
        memset(rbuf, 0, 128);

        if (0 == iRet)
        {
            snprintf(rbuf, 128, "\r停止巡航任务成功: 巡航ID=%u\r\n$", cruise_id);
            send(sock, rbuf, strlen(rbuf), 0);
        }
        else
        {
            snprintf(rbuf, 128, "\r停止巡航任务失败: 巡航ID=%u\r\n$", cruise_id);
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllCruiseTask
 功能描述  : 停止所有巡航任务
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
int StopAllCruiseTask(int sock)
{
    int i = 0;
    int iRet = 0;
    cruise_srv_t* pCruiseSrv = NULL;
    needtoproc_cruisesrv_queue needToStop;
    char rbuf[128] = {0};

    if ((NULL == g_CruiseSrvList) || (NULL == g_CruiseSrvList->pCruiseSrvList))
    {
        return -1;
    }

    needToStop.clear();

    CRUISE_SMUTEX_LOCK();

    if (osip_list_size(g_CruiseSrvList->pCruiseSrvList) <= 0)
    {
        CRUISE_SMUTEX_UNLOCK();
        return 0;
    }

    for (i = 0; i < osip_list_size(g_CruiseSrvList->pCruiseSrvList); i++)
    {
        pCruiseSrv = (cruise_srv_t*)osip_list_get(g_CruiseSrvList->pCruiseSrvList, i);

        if (NULL == pCruiseSrv)
        {
            continue;
        }

        needToStop.push_back(pCruiseSrv);
        pCruiseSrv->duration_time_count = 0;
    }

    CRUISE_SMUTEX_UNLOCK();

    while (!needToStop.empty())
    {
        pCruiseSrv = (cruise_srv_t*) needToStop.front();
        needToStop.pop_front();

        if (NULL != pCruiseSrv)
        {
            if (1 == pCruiseSrv->status || 4 == pCruiseSrv->status)
            {
                pCruiseSrv->status = 3;
            }
            else if (2 == pCruiseSrv->status)
            {
                pCruiseSrv->status = 0;

                /* 通知客户端 */ /* 停止巡航都不需要发送给客户端 */
                //iRet = SendNotifyExecuteCruiseActionToOnlineUser(pCruiseSrv->cruise_id, 1);

                pCruiseSrv->send_mark = 1;
            }

            if (sock > 0)
            {
                memset(rbuf, 0, 128);

                if (iRet >= 0)
                {
                    snprintf(rbuf, 128, "\r停止巡航任务成功: 巡航ID=%d\r\n", pCruiseSrv->cruise_id);
                    send(sock, rbuf, strlen(rbuf), 0);
                }
                else
                {
                    snprintf(rbuf, 128, "\r停止巡航任务失败: 巡航ID=%d\r\n", pCruiseSrv->cruise_id);
                    send(sock, rbuf, strlen(rbuf), 0);
                }
            }
        }
    }

    needToStop.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : CruiseSrvConfig_db_refresh_proc
 功能描述  : 设置巡航业务配置信息数据库更新操作标识
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
int CruiseSrvConfig_db_refresh_proc()
{
    if (1 == db_CruiseSrvInfo_reload_mark) /* 正在执行 */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "巡航业务配置数据库信息正在同步");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Cruise Srv Info database information are synchronized");
        return 0;
    }

    db_CruiseSrvInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 函 数 名  : check_CruiseSrvConfig_need_to_reload_begin
 功能描述  : 检查是否需要同步巡航业务配置开始
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
void check_CruiseSrvConfig_need_to_reload_begin(DBOper* pDboper)
{
    /* 检查是否需要更新数据库标识 */
    if (!db_CruiseSrvInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步巡航业务配置数据库信息: 开始---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization Cruise Srv info database information: begain---");

    /* 设置巡航队列的删除标识 */
    set_cruise_srv_list_del_mark(1);

    /* 将数据库中的变化数据同步到内存 */
    check_db_data_to_cruise_srv_list(pDboper);

    return;
}

/*****************************************************************************
 函 数 名  : check_CruiseSrvConfig_need_to_reload_end
 功能描述  : 检查是否需要同步巡航业务配置表结束
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
void check_CruiseSrvConfig_need_to_reload_end()
{
    /* 检查是否需要更新数据库标识 */
    if (!db_CruiseSrvInfo_reload_mark)
    {
        return;
    }

    /* 删除已经停止的轮巡数据 */
    delete_cruise_srv_data();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步巡航业务配置数据库信息: 结束---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization Cruise Srv info database information: end---");
    db_CruiseSrvInfo_reload_mark = 0;

    return;
}
