
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
#include "device/device_srv_proc.inc"

#include "user/user_srv_proc.inc"

#include "service/plan_srv_proc.inc"
#include "service/cruise_srv_proc.inc"

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
plan_srv_list_t* g_PlanSrvList = NULL;    /* 预案业务队列 */
int db_PlanSrvInfo_reload_mark = 0;       /* 预案业务数据库更新标识:0:不需要更新，1:需要更新数据库 */

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#if DECS("预案业务队列")

/*****************************************************************************
 函 数 名  : plan_action_init
 功能描述  : 预案动作初始化
 输入参数  : EV9000_PlanActionConfig** plan_action
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月2日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int plan_action_init(EV9000_PlanActionConfig** plan_action)
{
    *plan_action = (EV9000_PlanActionConfig*)osip_malloc(sizeof(EV9000_PlanActionConfig));

    if (*plan_action == NULL)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "plan_action_init() exit---: *plan_action Smalloc Error \r\n");
        return -1;
    }

    (*plan_action)->nID = 0;
    (*plan_action)->nPlanID = 0;
    (*plan_action)->nType = 0;
    (*plan_action)->nDeviceIndex = 0;
    (*plan_action)->nDestID = 0;
    (*plan_action)->nScreenID = 0;
    (*plan_action)->nControlData = 0;
    (*plan_action)->nStreamType = 0;
    (*plan_action)->nResved1 = 0;
    (*plan_action)->strResved2[0] = '\0';

    return 0;
}

/*****************************************************************************
 函 数 名  : plan_action_free
 功能描述  : 预案动作释放
 输入参数  : EV9000_PlanActionConfig* plan_action
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月2日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void plan_action_free(EV9000_PlanActionConfig* plan_action)
{
    if (plan_action == NULL)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "plan_action_free() exit---: Param Error \r\n");
        return;
    }

    plan_action->nID = 0;
    plan_action->nPlanID = 0;
    plan_action->nType = 0;
    plan_action->nDeviceIndex = 0;
    plan_action->nDestID = 0;
    plan_action->nScreenID = 0;
    plan_action->nControlData = 0;
    plan_action->nStreamType = 0;
    plan_action->nResved1 = 0;
    plan_action->strResved2[0] = '\0';

    osip_free(plan_action);
    plan_action = NULL;

    return;
}
/*****************************************************************************
 函 数 名  : plan_srv_init
 功能描述  : 预案业务结构初始化
 输入参数  : plan_srv_t ** plan_srv
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int plan_srv_init(plan_srv_t** plan_srv)
{
    *plan_srv = (plan_srv_t*)osip_malloc(sizeof(plan_srv_t));

    if (*plan_srv == NULL)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "plan_srv_init() exit---: *plan_srv Smalloc Error \r\n");
        return -1;
    }

    (*plan_srv)->plan_id = 0;
    (*plan_srv)->plan_name[0] = '\0';
    (*plan_srv)->iScheduledRun = 0;
    (*plan_srv)->start_time = 0;
    (*plan_srv)->status = 0;
    (*plan_srv)->del_mark = 0;

    return 0;
}

/*****************************************************************************
 函 数 名  : plan_srv_free
 功能描述  : 预案业务结构释放
 输入参数  : plan_srv_t * plan_srv
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void plan_srv_free(plan_srv_t* plan_srv)
{
    if (plan_srv == NULL)
    {
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "plan_srv_free() exit---: Param Error \r\n");
        return;
    }

    plan_srv->plan_id = 0;

    memset(plan_srv->plan_name, 0, MAX_128CHAR_STRING_LEN + 4);

    plan_srv->iScheduledRun = 0;
    plan_srv->start_time = 0;
    plan_srv->status = 0;
    plan_srv->del_mark = 0;

    osip_free(plan_srv);
    plan_srv = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : plan_srv_list_init
 功能描述  : 预案业务队列初始化
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
int plan_srv_list_init()
{
    g_PlanSrvList = (plan_srv_list_t*)osip_malloc(sizeof(plan_srv_list_t));

    if (g_PlanSrvList == NULL)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "plan_srv_list_init() exit---: g_PlanSrvList Smalloc Error \r\n");
        return -1;
    }

    g_PlanSrvList->pPlanSrvList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_PlanSrvList->pPlanSrvList)
    {
        osip_free(g_PlanSrvList);
        g_PlanSrvList = NULL;
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "plan_srv_list_init() exit---: Plan Srv List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_PlanSrvList->pPlanSrvList);

#ifdef MULTI_THR
    /* init smutex */
    g_PlanSrvList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_PlanSrvList->lock)
    {
        osip_free(g_PlanSrvList->pPlanSrvList);
        g_PlanSrvList->pPlanSrvList = NULL;
        osip_free(g_PlanSrvList);
        g_PlanSrvList = NULL;
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "plan_srv_list_init() exit---: Plan Srv List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : plan_srv_list_free
 功能描述  : 预案业务队列释放
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
void plan_srv_list_free()
{
    if (NULL == g_PlanSrvList)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "plan_srv_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_PlanSrvList->pPlanSrvList)
    {
        osip_list_special_free(g_PlanSrvList->pPlanSrvList, (void (*)(void*))&plan_srv_free);
        osip_free(g_PlanSrvList->pPlanSrvList);
        g_PlanSrvList->pPlanSrvList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_PlanSrvList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_PlanSrvList->lock);
        g_PlanSrvList->lock = NULL;
    }

#endif
    osip_free(g_PlanSrvList);
    g_PlanSrvList = NULL;
    return;
}

/*****************************************************************************
 函 数 名  : plan_srv_list_lock
 功能描述  : 预案业务队列锁定
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
int plan_srv_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PlanSrvList == NULL || g_PlanSrvList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "plan_srv_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_PlanSrvList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : plan_srv_list_unlock
 功能描述  : 预案业务解锁
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
int plan_srv_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PlanSrvList == NULL || g_PlanSrvList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "plan_srv_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_PlanSrvList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_plan_srv_list_lock
 功能描述  : 预案业务队列锁定
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
int debug_plan_srv_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PlanSrvList == NULL || g_PlanSrvList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "debug_plan_srv_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_PlanSrvList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_plan_srv_list_unlock
 功能描述  : 预案业务解锁
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
int debug_plan_srv_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PlanSrvList == NULL || g_PlanSrvList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "debug_plan_srv_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_PlanSrvList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : plan_srv_add
 功能描述  : 添加预案业务到队列中
 输入参数  : char* plan_id
                            int srv_info_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
//int plan_srv_add(int plan_id)
//{
//    plan_srv_t* pPlanSrv = NULL;
//    int i = 0;

//    if (g_PlanSrvList == NULL)
//    {
//        return -1;
//    }

//    i = plan_srv_init(&pPlanSrv);

//    if (i != 0)
//    {
//        return -1;
//    }

//    pPlanSrv->plan_id = plan_id;

//    plan_srv_list_lock();
//    i = list_add(g_PlanSrvList->pPlanSrvList, pPlanSrv, -1); /* add to list tail */

//    if (i == -1)
//    {
//        plan_srv_list_unlock();
//        plan_srv_free(pPlanSrv);
//        sfree(pPlanSrv);
//        return -1;
//    }

//    plan_srv_list_unlock();
//    return i;
//}

int plan_srv_find(unsigned int id)
{
    int i = 0;
    plan_srv_t* pPlanSrv = NULL;

    if (id < 0 || NULL == g_PlanSrvList || NULL == g_PlanSrvList->pPlanSrvList)
    {
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "plan_srv_find() exit---: Poll Action List Error \r\n");
        return -1;
    }

    PLAN_SMUTEX_LOCK();

    for (i = 0; i < osip_list_size(g_PlanSrvList->pPlanSrvList); i++)
    {
        pPlanSrv = (plan_srv_t*)osip_list_get(g_PlanSrvList->pPlanSrvList, i);

        if (NULL == pPlanSrv || pPlanSrv->plan_id <= 0)
        {
            continue;
        }

        if (pPlanSrv->plan_id == id)
        {
            PLAN_SMUTEX_UNLOCK();
            return i;
        }
    }

    PLAN_SMUTEX_UNLOCK();
    return -1;
}

plan_srv_t* plan_srv_get(int pos)
{
    plan_srv_t* pPlanSrv = NULL;

    if (NULL == g_PlanSrvList || NULL == g_PlanSrvList->pPlanSrvList)
    {
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "plan_srv_get() exit---: Plan Action List Error \r\n");
        return NULL;
    }

    if (pos < 0 || (pos >= osip_list_size(g_PlanSrvList->pPlanSrvList)))
    {
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "plan_srv_get() exit---: Pos Error \r\n");
        return NULL;
    }

    pPlanSrv = (plan_srv_t*)osip_list_get(g_PlanSrvList->pPlanSrvList, pos);

    if (NULL == pPlanSrv)
    {
        return NULL;
    }
    else
    {
        return pPlanSrv;
    }
}

int plan_srv_add(plan_srv_t* pPlanSrv)
{
    int i = 0;

    if (pPlanSrv == NULL)
    {
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "plan_srv_add() exit---: Param Error \r\n");
        return -1;
    }

    PLAN_SMUTEX_LOCK();

    i = osip_list_add(g_PlanSrvList->pPlanSrvList, pPlanSrv, -1); /* add to list tail */

    if (i < 0)
    {
        PLAN_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "plan_srv_add() exit---: List Add Error \r\n");
        return -1;
    }

    PLAN_SMUTEX_UNLOCK();
    return i - 1;
}


/*****************************************************************************
 函 数 名  : plan_srv_remove
 功能描述  : 从队列中移除预案业务
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
int plan_srv_remove(int pos)
{
    plan_srv_t* pPlanSrv = NULL;

    if (g_PlanSrvList == NULL || pos < 0 || (pos >= osip_list_size(g_PlanSrvList->pPlanSrvList)))
    {
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "plan_srv_remove() exit---: Param Error \r\n");
        return -1;
    }

    PLAN_SMUTEX_LOCK();

    pPlanSrv = (plan_srv_t*)osip_list_get(g_PlanSrvList->pPlanSrvList, pos);

    if (NULL == pPlanSrv)
    {
        PLAN_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "plan_srv_remove() exit---: List Get Error \r\n");
        return -1;
    }

    osip_list_remove(g_PlanSrvList->pPlanSrvList, pos);
    plan_srv_free(pPlanSrv);
    pPlanSrv = NULL;
    PLAN_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 函 数 名  : scan_plan_srv_list
 功能描述  : 扫描预案业务消息队列
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
void scan_plan_srv_list(DBOper* pPlan_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    plan_srv_t* pPlanSrv = NULL;
    needtoproc_plansrv_queue needToProc;
    char strPlanID[32] = {0};
    time_t now = time(NULL);
    int iTimeNow = 0;
    struct tm tp = {0};

    if (NULL == pPlan_Srv_dboper)
    {
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "scan_plan_srv_list() exit---: Param Error \r\n");
        return;
    }

    needToProc.clear();

    PLAN_SMUTEX_LOCK();

    if (osip_list_size(g_PlanSrvList->pPlanSrvList) <= 0)
    {
        PLAN_SMUTEX_UNLOCK();
        return;
    }

    localtime_r(&now, &tp);
    iTimeNow = tp.tm_hour * 3600 + tp.tm_min * 60 + tp.tm_sec;

    for (i = 0; i < osip_list_size(g_PlanSrvList->pPlanSrvList); i++)
    {
        pPlanSrv = (plan_srv_t*)osip_list_get(g_PlanSrvList->pPlanSrvList, i);

        if (NULL == pPlanSrv)
        {
            continue;
        }

        if (1 == pPlanSrv->del_mark) /* 要删除的数据 */
        {
            continue;
        }

        if (0 == pPlanSrv->iScheduledRun) /* 非定时执行的过滤掉 */
        {
            continue;
        }

        if (0 == pPlanSrv->status) /* 执行预案 */
        {
            if ((iTimeNow == pPlanSrv->start_time) || (iTimeNow > pPlanSrv->start_time && iTimeNow - pPlanSrv->start_time < 30)) /* 30秒之内的才启动 */
            {
                pPlanSrv->status = 1;
                needToProc.push_back(pPlanSrv);
            }
        }
        else /* 0点的时候状态置为0 */
        {
            if (tp.tm_hour == 0 && tp.tm_min == 0 && tp.tm_sec == 0)
            {
                pPlanSrv->status = 0;
            }
        }
    }

    PLAN_SMUTEX_UNLOCK();

    while (!needToProc.empty())
    {
        pPlanSrv = (plan_srv_t*) needToProc.front();
        needToProc.pop_front();

        if (NULL != pPlanSrv)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "定时触发执行预案:plan_id=%u, plan_name=%s, start_time=%d, ", pPlanSrv->plan_id, pPlanSrv->plan_name, pPlanSrv->start_time);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Time triggered plan:plan_id=%d, plan_name=%s, start_time=%d, ", pPlanSrv->plan_id, pPlanSrv->plan_name, pPlanSrv->start_time);
            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "scan_plan_srv_list() PlanSrv:plan_id=%u, start_time=%d, status=%d, del_mark=%d\r\n", pPlanSrv->plan_id, pPlanSrv->start_time, pPlanSrv->status, pPlanSrv->del_mark);

            snprintf(strPlanID, 32, "%u", pPlanSrv->plan_id);

            iRet = StartPlanActionByPlanID(NULL, strPlanID, pPlanSrv->plan_name, pPlan_Srv_dboper);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "scan_plan_srv_list() StartPlanActionByPlanID Error:plan_id=%s, iRet=%d\r\n", strPlanID, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "scan_plan_srv_list() StartPlanActionByPlanID OK:plan_id=%s, iRet=%d\r\n", strPlanID, iRet);
            }
        }
    }

    needToProc.clear();

    return;
}
#endif

/*****************************************************************************
 函 数 名  : set_plan_srv_list_del_mark
 功能描述  : 设置预案的删除标识
 输入参数  : int del_mark
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月8日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int set_plan_srv_list_del_mark(int del_mark)
{
    int pos = -1;
    plan_srv_t* pPlanSrv = NULL;

    if (NULL == g_PlanSrvList)
    {
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "set_plan_srv_list_del_mark() exit---: Param Error \r\n");
        return -1;
    }

    PLAN_SMUTEX_LOCK();

    if (osip_list_size(g_PlanSrvList->pPlanSrvList) <= 0)
    {
        PLAN_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "set_plan_srv_list_del_mark() exit---: Record Info List NULL \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_PlanSrvList->pPlanSrvList); pos++)
    {
        pPlanSrv = (plan_srv_t*)osip_list_get(g_PlanSrvList->pPlanSrvList, pos);

        if ((NULL == pPlanSrv) || (pPlanSrv->plan_id <= 0))
        {
            continue;
        }

        pPlanSrv->del_mark = del_mark;
    }

    PLAN_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : check_db_data_to_plan_srv_list_for_start
 功能描述  : 从数据中检测是否有需要执行的预案数据，如果有，则加载到内存中
 输入参数  : DBOper* pPlan_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年4月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int check_db_data_to_plan_srv_list_for_start(DBOper* pPlan_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    string strSQL = "";
    int record_count = 0;
    int while_count = 0;

    if (NULL == pPlan_Srv_dboper)
    {
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from PlanConfig WHERE ScheduledRun = 1 order by StartTime asc";

    record_count = pPlan_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "check_db_data_to_plan_srv_list_for_start() record_count=%d", record_count);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "check_db_data_to_plan_srv_list_for_start() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "check_db_data_to_plan_srv_list_for_start() ErrorMsg=%s\r\n", pPlan_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "check_db_data_to_plan_srv_list_for_start() exit---: No Record Count \r\n");
        return 0;
    }

    do
    {
        unsigned int uPlanID = 0;
        string strPlanName = "";
        int iStartTime = 0;
        int iScheduledRun = 0;
        int plan_pos = -1;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_WARN, "check_db_data_to_plan_srv_list_for_start() While Count=%d \r\n", while_count);
        }

        pPlan_Srv_dboper->GetFieldValue("ID", uPlanID);
        pPlan_Srv_dboper->GetFieldValue("PlanName", strPlanName);
        pPlan_Srv_dboper->GetFieldValue("StartTime", iStartTime);
        pPlan_Srv_dboper->GetFieldValue("ScheduledRun", iScheduledRun);

        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "check_db_data_to_plan_srv_list_for_start() ID=%d, StartTime=%d, UserIndex=%d", iPlanID, iStartTime, iUserIndex);

        /* 查找队列，看队列里面是否已经存在 */
        plan_pos = plan_srv_find(uPlanID);

        if (plan_pos < 0) /* 添加到要执行队列 */
        {
            plan_srv_t* pPlanSrv = NULL;
            i = plan_srv_init(&pPlanSrv);

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "check_db_data_to_plan_srv_list_for_start() poll_srv_init:i=%d \r\n", i);
                continue;
            }

            pPlanSrv->plan_id = uPlanID;

            if (!strPlanName.empty())
            {
                osip_strncpy(pPlanSrv->plan_name, (char*)strPlanName.c_str(), MAX_128CHAR_STRING_LEN);
            }

            pPlanSrv->start_time = iStartTime;
            pPlanSrv->iScheduledRun = iScheduledRun;

            /* 添加到队列 */
            if (plan_srv_add(pPlanSrv) < 0)
            {
                plan_srv_free(pPlanSrv);
                pPlanSrv = NULL;
                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "check_db_data_to_plan_srv_list_for_start() Poll Srv Add Error");
                continue;
            }
            else
            {
                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "check_db_data_to_plan_srv_list_for_start() plan_srv_add:plan_id=%u", pPlanSrv->plan_id);
            }
        }
        else
        {
            plan_srv_t* pPlanSrv = NULL;
            pPlanSrv = plan_srv_get(plan_pos);

            if (NULL != pPlanSrv)
            {
                pPlanSrv->del_mark = 0;

                /* 看数据是否有变化 */
                if (!strPlanName.empty())
                {
                    if (0 != sstrcmp(pPlanSrv->plan_name, (char*)strPlanName.c_str()))
                    {
                        memset(pPlanSrv->plan_name, 0, MAX_128CHAR_STRING_LEN + 4);
                        osip_strncpy(pPlanSrv->plan_name, (char*)strPlanName.c_str(), MAX_128CHAR_STRING_LEN);
                    }
                }
                else
                {
                    memset(pPlanSrv->plan_name, 0, MAX_128CHAR_STRING_LEN + 4);
                }

                if (iStartTime != pPlanSrv->start_time)
                {
                    pPlanSrv->start_time = iStartTime;
                    pPlanSrv->status = 0;
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "check_db_data_to_plan_srv_list_for_start() plan_srv_restart:plan_id=%u", pPlanSrv->plan_id);
                }

                if (iScheduledRun != pPlanSrv->iScheduledRun)
                {
                    pPlanSrv->iScheduledRun = iScheduledRun;
                }
            }
        }
    }
    while (pPlan_Srv_dboper->MoveNext() >= 0);

    return iRet;
}

/*****************************************************************************
 函 数 名  : delete_plan_srv_data
 功能描述  : 删除过期的预案数据
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
void delete_plan_srv_data()
{
    int pos = 0;
    plan_srv_t* pPlanSrv = NULL;

    if ((NULL == g_PlanSrvList) || (NULL == g_PlanSrvList->pPlanSrvList))
    {
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "delete_plan_srv_data() exit---: Param Error \r\n");
        return;
    }

    PLAN_SMUTEX_LOCK();

    if (osip_list_size(g_PlanSrvList->pPlanSrvList) <= 0)
    {
        PLAN_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_WARN, "delete_plan_srv_data() exit---: Plan Srv List NULL \r\n");
        return;
    }

    pos = 0;

    while (!osip_list_eol(g_PlanSrvList->pPlanSrvList, pos))
    {
        pPlanSrv = (plan_srv_t*)osip_list_get(g_PlanSrvList->pPlanSrvList, pos);

        if (NULL == pPlanSrv)
        {
            osip_list_remove(g_PlanSrvList->pPlanSrvList, pos);
            continue;
        }

        if (1 == pPlanSrv->del_mark) /* 删除预案 */
        {
            osip_list_remove(g_PlanSrvList->pPlanSrvList, pos);
            plan_srv_free(pPlanSrv);
            pPlanSrv = NULL;
        }
        else
        {
            pos++;
        }
    }

    PLAN_SMUTEX_UNLOCK();

    return;
}

/*****************************************************************************
 函 数 名  : StartPlanActionByPlanID
 功能描述  : 根据预案编号启动预案动作
 输入参数  : user_info_t* pUserInfo
             char* pcPlanID
             char* pcPlanName
             DBOper* pPlan_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月14日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StartPlanActionByPlanID(user_info_t* pUserInfo, char* pcPlanID, char* pcPlanName, DBOper* pPlan_Srv_dboper)
{
    int i = 0;
    int record_count = 0;
    string strSQL = "";
    GBLogicDevice_info_t* pSourceGBLogicDeviceInfo = NULL;
    GBLogicDevice_info_t* pDestGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pDestGBDeviceInfo = NULL;
    int iHasPlanIDSend = 0;
    int while_count = 0;
    needtoproc_planaction_queue needToProc;
    EV9000_PlanActionConfig* plan_action = NULL;

    if (NULL == pcPlanID || NULL == pPlan_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG,  "StartPlanActionByPlanID() exit---: Plan Srv DB Oper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() PlanID=%s \r\n", pcPlanID);

    /* 根据plan_id，查询预案动作表，获取预案的具体数据 */
    strSQL.clear();
    strSQL = "select * from PlanActionConfig WHERE PlanID = ";
    strSQL += pcPlanID;
    record_count = pPlan_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() ErrorMsg=%s\r\n", pPlan_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_WARN, "StartPlanActionByPlanID() exit---: No Record Count \r\n");
        return 0;
    }

    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanID() record_count=%d\r\n", record_count);

    needToProc.clear();

    /* 循环获取预案动作数据 */
    do
    {
        int iRet = 0;
        unsigned int uID = 0;
        unsigned int uType = 0;
        unsigned int uDeviceIndex = 0;
        int iStreamType = 0;
        unsigned int uDestIndex = 0;
        unsigned int uScreenID = -1;
        unsigned int uControlData = 0;
        unsigned int uUserIndex = 0;
        string strResved2 = "";

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_WARN, "StartPlanActionByPlanID() While Count=%d \r\n", while_count);
        }

        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "StartPlanActionByPlanID() record_count=%d\r\n", ++record_count);

        uID = 0;
        pPlan_Srv_dboper->GetFieldValue("ID", uID);

        uType = 0;
        pPlan_Srv_dboper->GetFieldValue("Type", uType);

        uDeviceIndex = 0;
        pPlan_Srv_dboper->GetFieldValue("DeviceIndex", uDeviceIndex);

        iStreamType = 0;
        pPlan_Srv_dboper->GetFieldValue("StreamType", iStreamType);

        uDestIndex = 0;
        pPlan_Srv_dboper->GetFieldValue("DestID", uDestIndex);

        uScreenID = 0;
        pPlan_Srv_dboper->GetFieldValue("ScreenID", uScreenID);

        uControlData = 0;
        pPlan_Srv_dboper->GetFieldValue("ControlData", uControlData);

        uUserIndex = 0;
        pPlan_Srv_dboper->GetFieldValue("Resved1", uUserIndex);

        strResved2.clear();
        pPlan_Srv_dboper->GetFieldValue("Resved2", strResved2);

        iRet = plan_action_init(&plan_action);

        if (0 != iRet || NULL == plan_action)
        {
            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() plan_action_init Error: PlanAction ID=%u\r\n", uID);
            continue;
        }

        plan_action->nID = uID;
        plan_action->nType = uType;
        plan_action->nDeviceIndex = uDeviceIndex;
        plan_action->nDestID = uDestIndex;
        plan_action->nScreenID = uScreenID;
        plan_action->nControlData = uControlData;
        plan_action->nStreamType = iStreamType;
        plan_action->nResved1 = uUserIndex;

        memset(plan_action->strResved2, 0, EV9000_SHORT_STRING_LEN);

        if (!strResved2.empty())
        {
            osip_strncpy(plan_action->strResved2, (char*)strResved2.c_str(), EV9000_SHORT_STRING_LEN - 4);
        }

        needToProc.push_back(plan_action);

        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "StartPlanActionByPlanID() PlanAction ID=%u, Type=%u, DeviceIndex=%u, DestIndex=%u, StreamType=%d, ScreenID=%u, ControlData=%u\r\n", uID, uType, uDeviceIndex, uDestIndex, iStreamType, uScreenID, uControlData);
    }
    while (pPlan_Srv_dboper->MoveNext() >= 0);

    while (!needToProc.empty())
    {
        plan_action = (EV9000_PlanActionConfig*) needToProc.front();
        needToProc.pop_front();

        if (NULL != plan_action)
        {
            if (PLANACTION_TVWALL == plan_action->nType) /* 电视墙轮巡 */
            {
                if (plan_action->nDeviceIndex <= 0)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() uDeviceIndex=%u, uDestIndex=%u\r\n", plan_action->nDeviceIndex, plan_action->nDestID);
                    plan_action_free(plan_action);
                    plan_action = NULL;
                    continue;
                }

                /* 获取目的端的设备信息 */
                pDestGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(plan_action->nDestID);

                if (NULL != pDestGBLogicDeviceInfo)
                {
                    pDestGBDeviceInfo = GBDevice_info_get_by_stream_type(pDestGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

                    if (NULL != pDestGBDeviceInfo && EV9000_DEVICETYPE_DECODER == pDestGBDeviceInfo->device_type)
                    {
                        pSourceGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(plan_action->nDeviceIndex);

                        if (NULL != pSourceGBLogicDeviceInfo)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行预案打开视墙:电视墙通道ID=%s, 逻辑设备ID=%s", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan open TV wall:TV wall channel ID=%s, logic device ID=%s", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id);

                            if (plan_action->nStreamType <= 0)
                            {
                                i = start_connect_tv_proc(pSourceGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_MASTER, pDestGBLogicDeviceInfo->device_id, 0);
                            }
                            else
                            {
                                i = start_connect_tv_proc(pSourceGBLogicDeviceInfo->device_id, plan_action->nStreamType, pDestGBLogicDeviceInfo->device_id, 0);
                            }

                            if (i < 0)
                            {
                                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() start_connect_tv_proc Error: dest_id=%s, source_id=%s, i=%d\r\n", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "StartPlanActionByPlanID() start_connect_tv_proc OK: dest_id=%s, source_id=%s, i=%d\r\n", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id, i);
                            }
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() GBLogicDevice_info_find_by_device_index Error, uDeviceIndex=%u\r\n", plan_action->nDeviceIndex);
                        }
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() DestGBDeviceInfo OR DestGBDeviceInfo Device Type Error, pDestGBDeviceInfo->device_id=%s, pDestGBDeviceInfo->device_type=%d\r\n", pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->device_type);
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() Find Dest GBLogic Device Info Error, DestIndex=%u\r\n", plan_action->nDestID);
                }
            }
            else if (PLANACTION_PC == plan_action->nType) /* PC屏幕轮巡 */
            {
                if (0 == iHasPlanIDSend)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行预案发送PC屏幕轮巡:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan send PC screen polling:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);

                    /* 发送给客户端 */
                    i = SendNotifyExecutePlanActionToOnlineUser(pUserInfo, pcPlanID, pcPlanName, pPlan_Srv_dboper);

                    if (i < 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "执行预案发送PC屏幕轮巡失败:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Perform round tour plans to send PC screen failure: PlanID = % s, PlanName = % s", pcPlanID, pcPlanName);
                        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() SendNotifyExecutePlanActionToOnlineUser Error: i=%d\r\n", i);
                    }
                    else if (i > 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行预案发送PC屏幕轮巡成功:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Round tour successful execution plans to send PC screen: PlanID = % s, PlanName = % s", pcPlanID, pcPlanName);
                        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanID() SendNotifyExecutePlanActionToOnlineUser OK:i=%d\r\n", i);
                    }

                    iHasPlanIDSend = 1;
                }
            }
            else if (PLANACTION_PRESET == plan_action->nType) /* 预置位 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行预案发送预置位:PresetID=%u", plan_action->nControlData);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan send preset position:PresetID=%u", plan_action->nControlData);

                i = ExecuteDevicePresetByPresetID(plan_action->nControlData, pPlan_Srv_dboper);

                if (0 != i)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "执行预案发送预置位处理失败:PresetID=%d", plan_action->nControlData);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Perform a processing plan send preset failure: PresetID = % d", plan_action->nControlData);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() ExecuteDevicePresetByPresetID Error: uDeviceIndex=%u, uDestIndex=%u, uScreenID=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nDestID, plan_action->nScreenID, plan_action->nControlData, i);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行预案发送预置位处理成功:PresetID=%d", plan_action->nControlData);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Execution plans to send preset a treatment success: PresetID = % d", plan_action->nControlData);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanID() ExecuteDevicePresetByPresetID OK: uDeviceIndex=%u, uDestIndex=%u, uScreenID=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nDestID, plan_action->nScreenID, plan_action->nControlData, i);
                }
            }
            else if (PLANACTION_ALARM_RECORD == plan_action->nType) /* 报警录像 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行预案触发报警录像:DeviceIndex=%u, 类型=%d", plan_action->nDeviceIndex, plan_action->nControlData);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan Set Alarm Record:DeviceIndex=%u, Type=%d", plan_action->nDeviceIndex, plan_action->nControlData);

                i = ExecuteDeviceAlarmRecord(plan_action->nDeviceIndex, plan_action->nControlData, pPlan_Srv_dboper);

                if (0 != i)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "执行预案触发报警录像失败:DeviceIndex=%u, 类型=%d", plan_action->nDeviceIndex, plan_action->nControlData);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Execution plan triggered alarm video failure: DeviceIndex = % u, type = % d", plan_action->nDeviceIndex, plan_action->nControlData);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() ExecuteDeviceAlarmRecord Error: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行预案触发报警录像成功:DeviceIndex=%u, 类型=%d", plan_action->nDeviceIndex, plan_action->nControlData);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Execution plan triggered alarm video success: DeviceIndex = % u, type = % d", plan_action->nDeviceIndex, plan_action->nControlData);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanID() ExecuteDeviceAlarmRecord OK: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
            }
            else if (PLANACTION_SEND_MAIL == plan_action->nType) /* 发送邮件 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行预案触发发送邮件操作:UserIndex=%u", plan_action->nResved1);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan Send Email:UserIndex=%u", plan_action->nResved1);

                i = SendAlarmMsgMailToUserByUserIndex(plan_action->nResved1, NULL, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() SendAlarmMsgMailToUserByUserIndex Error: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanID() SendAlarmMsgMailToUserByUserIndex OK: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
            }
            else if (PLANACTION_CAPTURE_IMAGE == plan_action->nType) /* 抓图 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行预案触发抓取图片操作:DeviceIndex=%u", plan_action->nDeviceIndex);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan capture image:DeviceIndex=%u", plan_action->nDeviceIndex);

                //i = ExecuteDeviceCaptureImage(plan_action->nDeviceIndex, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() ExecuteDeviceCaptureImage Error: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanID() ExecuteDeviceCaptureImage OK: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
            }
            else if (PLANACTION_EXECUTE_CRUISE == plan_action->nType) /* 执行巡航 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行预案触发执行巡航:巡航ID=%u", plan_action->nDestID);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan execute cruise:CruiseID=%u", plan_action->nDestID);

                i = start_cruise_srv_by_id_for_plan(plan_action->nDestID, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() start_cruise_srv_by_id_for_plan Error: CruiseID=%u, i=%d\r\n", plan_action->nDestID, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanID() start_cruise_srv_by_id_for_plan OK: CruiseID=%u, i=%d\r\n", plan_action->nDestID, i);
                }
            }
            else if (PLANACTION_CONTROL_RCU == plan_action->nType) /* 执行RCU */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行预案触发执行控制RCU报警输出:DeviceIndex=%u, 控制数值=%s", plan_action->nDeviceIndex, plan_action->strResved2);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan control RCU:DeviceIndex=%u", plan_action->nDeviceIndex);

                i = ExecuteControlRCUDevice(plan_action->nDeviceIndex, plan_action->strResved2, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() ExecuteControlRCUDevice Error: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanID() ExecuteControlRCUDevice OK: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
            }
            else
            {
                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanID() PlanAction Type=%u Not Support\r\n", plan_action->nType);
                plan_action_free(plan_action);
                plan_action = NULL;
                continue;
            }

            plan_action_free(plan_action);
            plan_action = NULL;
        }
    }

    needToProc.clear();
    return 0;
}

/*****************************************************************************
 函 数 名  : StartPlanActionByPlanIDForAlarm
 功能描述  : 报警消息触法的根据预案编号启动预案动作
 输入参数  : char* pcPlanID
             char* pcPlanName
             alarm_msg_t* pAlarmMsg
             DBOper* pPlan_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月14日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StartPlanActionByPlanIDForAlarm(char* pcPlanID, char* pcPlanName, alarm_msg_t* pAlarmMsg, DBOper* pPlan_Srv_dboper)
{
    int i = 0;
    int record_count = 0;
    string strSQL = "";
    GBLogicDevice_info_t* pSourceGBLogicDeviceInfo = NULL;
    GBLogicDevice_info_t* pDestGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pDestGBDeviceInfo = NULL;
    int iHasPlanIDSend = 0;
    int while_count = 0;
    needtoproc_planaction_queue needToProc;
    EV9000_PlanActionConfig* plan_action = NULL;

    if (NULL == pcPlanID || NULL == pPlan_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG,  "StartPlanActionByPlanIDForAlarm() exit---: Plan Srv DB Oper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() PlanID=%s \r\n", pcPlanID);

    /* 根据plan_id，查询预案动作表，获取预案的具体数据 */
    strSQL.clear();
    strSQL = "select * from PlanActionConfig WHERE PlanID = ";
    strSQL += pcPlanID;
    record_count = pPlan_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() ErrorMsg=%s\r\n", pPlan_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_WARN, "StartPlanActionByPlanIDForAlarm() exit---: No Record Count \r\n");
        return 0;
    }

    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAlarm() record_count=%d\r\n", record_count);

    needToProc.clear();

    /* 循环获取预案动作数据 */
    do
    {
        int iRet = 0;
        unsigned int uID = 0;
        unsigned int uType = 0;
        unsigned int uDeviceIndex = 0;
        int iStreamType = 0;
        unsigned int uDestIndex = 0;
        unsigned int uScreenID = -1;
        unsigned int uControlData = 0;
        unsigned int uUserIndex = 0;
        string strResved2 = "";

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_WARN, "StartPlanActionByPlanIDForAlarm() While Count=%d \r\n", while_count);
        }

        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "StartPlanActionByPlanIDForAlarm() record_count=%d\r\n", ++record_count);

        uID = 0;
        pPlan_Srv_dboper->GetFieldValue("ID", uID);

        uType = 0;
        pPlan_Srv_dboper->GetFieldValue("Type", uType);

        uDeviceIndex = 0;
        pPlan_Srv_dboper->GetFieldValue("DeviceIndex", uDeviceIndex);

        iStreamType = 0;
        pPlan_Srv_dboper->GetFieldValue("StreamType", iStreamType);

        uDestIndex = 0;
        pPlan_Srv_dboper->GetFieldValue("DestID", uDestIndex);

        uScreenID = 0;
        pPlan_Srv_dboper->GetFieldValue("ScreenID", uScreenID);

        uControlData = 0;
        pPlan_Srv_dboper->GetFieldValue("ControlData", uControlData);

        uUserIndex = 0;
        pPlan_Srv_dboper->GetFieldValue("Resved1", uUserIndex);

        strResved2.clear();
        pPlan_Srv_dboper->GetFieldValue("Resved2", strResved2);

        iRet = plan_action_init(&plan_action);

        if (0 != iRet || NULL == plan_action)
        {
            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() plan_action_init Error: PlanAction ID=%u\r\n", uID);
            continue;
        }

        plan_action->nID = uID;
        plan_action->nType = uType;
        plan_action->nDeviceIndex = uDeviceIndex;
        plan_action->nDestID = uDestIndex;
        plan_action->nScreenID = uScreenID;
        plan_action->nControlData = uControlData;
        plan_action->nStreamType = iStreamType;
        plan_action->nResved1 = uUserIndex;

        memset(plan_action->strResved2, 0, EV9000_SHORT_STRING_LEN);

        if (!strResved2.empty())
        {
            osip_strncpy(plan_action->strResved2, (char*)strResved2.c_str(), EV9000_SHORT_STRING_LEN - 4);
        }

        needToProc.push_back(plan_action);

        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "StartPlanActionByPlanIDForAlarm() PlanAction ID=%u, Type=%u, DeviceIndex=%u, DestIndex=%u, StreamType=%d, ScreenID=%u, ControlData=%u\r\n", uID, uType, uDeviceIndex, uDestIndex, iStreamType, uScreenID, uControlData);
    }
    while (pPlan_Srv_dboper->MoveNext() >= 0);

    while (!needToProc.empty())
    {
        plan_action = (EV9000_PlanActionConfig*) needToProc.front();
        needToProc.pop_front();

        if (NULL != plan_action)
        {
            if (PLANACTION_TVWALL == plan_action->nType) /* 电视墙轮巡 */
            {
                if (plan_action->nDeviceIndex <= 0)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() uDeviceIndex=%u, uDestIndex=%u\r\n", plan_action->nDeviceIndex, plan_action->nDestID);
                    plan_action_free(plan_action);
                    plan_action = NULL;
                    continue;
                }

                /* 获取目的端的设备信息 */
                pDestGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(plan_action->nDestID);

                if (NULL != pDestGBLogicDeviceInfo)
                {
                    pDestGBDeviceInfo = GBDevice_info_get_by_stream_type(pDestGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

                    if (NULL != pDestGBDeviceInfo && EV9000_DEVICETYPE_DECODER == pDestGBDeviceInfo->device_type)
                    {
                        pSourceGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(plan_action->nDeviceIndex);

                        if (NULL != pSourceGBLogicDeviceInfo)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警触发执行预案打开视墙:电视墙通道ID=%s, 逻辑设备ID=%s", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan open TV wall:TV wall channel ID=%s, logic device ID=%s", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id);

                            if (plan_action->nStreamType <= 0)
                            {
                                i = start_connect_tv_proc(pSourceGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_MASTER, pDestGBLogicDeviceInfo->device_id, 0);
                            }
                            else
                            {
                                i = start_connect_tv_proc(pSourceGBLogicDeviceInfo->device_id, plan_action->nStreamType, pDestGBLogicDeviceInfo->device_id, 0);
                            }

                            if (i < 0)
                            {
                                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() start_connect_tv_proc Error: dest_id=%s, source_id=%s, i=%d\r\n", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "StartPlanActionByPlanIDForAlarm() start_connect_tv_proc OK: dest_id=%s, source_id=%s, i=%d\r\n", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id, i);
                            }
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() GBLogicDevice_info_find_by_device_index Error, uDeviceIndex=%u\r\n", plan_action->nDeviceIndex);
                        }
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() DestGBDeviceInfo OR DestGBDeviceInfo Device Type Error, pDestGBDeviceInfo->device_id=%s, pDestGBDeviceInfo->device_type=%d\r\n", pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->device_type);
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() Find Dest GBLogic Device Info Error, DestIndex=%u\r\n", plan_action->nDestID);
                }
            }
            else if (PLANACTION_PC == plan_action->nType) /* PC屏幕轮巡 */
            {
                if (0 == iHasPlanIDSend)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警触发执行预案发送PC屏幕轮巡:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan send PC screen polling:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);

                    /* 发送给客户端 */
                    i = SendNotifyExecutePlanActionToOnlineUser(NULL, pcPlanID, pcPlanName, pPlan_Srv_dboper);

                    if (i < 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "报警触发执行预案发送PC屏幕轮巡失败:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Round tour failure alarm trigger execution plans to send PC screen: PlanID = % s, PlanName = % s", pcPlanID, pcPlanName);
                        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() SendNotifyExecutePlanActionToOnlineUser Error: i=%d\r\n", i);
                    }
                    else if (i > 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警触发执行预案发送PC屏幕轮巡成功:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Alarm trigger execution plans to send PC screen round tour success: PlanID = % s, PlanName = % s", pcPlanID, pcPlanName);
                        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAlarm() SendNotifyExecutePlanActionToOnlineUser OK:i=%d\r\n", i);
                    }

                    iHasPlanIDSend = 1;
                }
            }
            else if (PLANACTION_PRESET == plan_action->nType) /* 预置位 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警触发执行预案发送预置位:PresetID=%u", plan_action->nControlData);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan send preset position:PresetID=%u", plan_action->nControlData);

                i = ExecuteDevicePresetByPresetID(plan_action->nControlData, pPlan_Srv_dboper);

                if (0 != i)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "报警触发执行预案发送预置位处理失败:PresetID=%d", plan_action->nControlData);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "run plan send preset position fialed:PresetID=%d", plan_action->nControlData);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() ExecuteDevicePresetByPresetID Error: uDeviceIndex=%u, uDestIndex=%u, uScreenID=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nDestID, plan_action->nScreenID, plan_action->nControlData, i);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警触发执行预案发送预置位处理成功:PresetID=%d", plan_action->nControlData);
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan send preset position success:PresetID=%d", plan_action->nControlData);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAlarm() ExecuteDevicePresetByPresetID OK: uDeviceIndex=%u, uDestIndex=%u, uScreenID=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nDestID, plan_action->nScreenID, plan_action->nControlData, i);
                }
            }
            else if (PLANACTION_ALARM_RECORD == plan_action->nType) /* 报警录像 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警触发执行预案报警录像:DeviceIndex=%u, 类型=%d", plan_action->nDeviceIndex, plan_action->nControlData);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan Set Alarm Record:DeviceIndex=%u, Type=%d", plan_action->nDeviceIndex, plan_action->nControlData);

                i = ExecuteDeviceAlarmRecord(plan_action->nDeviceIndex, plan_action->nControlData, pPlan_Srv_dboper);

                if (0 != i)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "报警触发执行预案报警录像失败:DeviceIndex=%u, 类型=%d", plan_action->nDeviceIndex, plan_action->nControlData);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "run plan send preset position fialed:DeviceIndex=%u, type=%d", plan_action->nDeviceIndex, plan_action->nControlData);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() ExecuteDeviceAlarmRecord Error: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警触发执行预案报警录像成功:DeviceIndex=%u, 类型=%d", plan_action->nDeviceIndex, plan_action->nControlData);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan send preset position success:DeviceIndex=%u, 类型=%d", plan_action->nDeviceIndex, plan_action->nControlData);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAlarm() ExecuteDeviceAlarmRecord OK: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
            }
            else if (PLANACTION_SEND_MAIL == plan_action->nType) /* 发送邮件 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警触发执行预案触发发送邮件操作:UserIndex=%u", plan_action->nResved1);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan Send Email:UserIndex=%u", plan_action->nResved1);

                i = SendAlarmMsgMailToUserByUserIndex(plan_action->nResved1, pAlarmMsg, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() SendAlarmMsgMailToUserByUserIndex Error: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAlarm() SendAlarmMsgMailToUserByUserIndex OK: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
            }
            else if (PLANACTION_CAPTURE_IMAGE == plan_action->nType) /* 抓图 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警触发执行预案触发抓取图片操作:DeviceIndex=%u", plan_action->nDeviceIndex);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan capture image:DeviceIndex=%u", plan_action->nDeviceIndex);

                //i = ExecuteDeviceCaptureImage(plan_action->nDeviceIndex, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() ExecuteDeviceCaptureImage Error: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAlarm() ExecuteDeviceCaptureImage OK: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
            }
            else if (PLANACTION_EXECUTE_CRUISE == plan_action->nType) /* 执行巡航 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警触发执行预案触发执行巡航:巡航ID=%u", plan_action->nDestID);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan execute cruise:CruiseID=%u", plan_action->nDestID);

                i = start_cruise_srv_by_id_for_plan(plan_action->nDestID, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() start_cruise_srv_by_id_for_plan Error: CruiseID=%u, i=%d\r\n", plan_action->nDestID, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAlarm() start_cruise_srv_by_id_for_plan OK: CruiseID=%u, i=%d\r\n", plan_action->nDestID, i);
                }
            }
            else if (PLANACTION_CONTROL_RCU == plan_action->nType) /* 执行RCU */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行预案触发执行控制RCU报警输出:DeviceIndex=%u, 控制数值=%s", plan_action->nDeviceIndex, plan_action->strResved2);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan control RCU:DeviceIndex=%u", plan_action->nDeviceIndex);

                i = ExecuteControlRCUDevice(plan_action->nDeviceIndex, plan_action->strResved2, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() ExecuteControlRCUDevice Error: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAlarm() ExecuteControlRCUDevice OK: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
            }
            else
            {
                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() PlanAction Type=%u Not Support\r\n", plan_action->nType);
                plan_action_free(plan_action);
                plan_action = NULL;
                continue;
            }

            plan_action_free(plan_action);
            plan_action = NULL;
        }
    }

    needToProc.clear();
    return 0;
}

/*****************************************************************************
 函 数 名  : StartPlanActionByPlanIDForAutoEndAlarm
 功能描述  : 报警自动结束触法的根据预案编号启动预案动作
 输入参数  : char* pcPlanID
             char* pcPlanName
             alarm_msg_t* pAlarmMsg
             DBOper* pPlan_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月14日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StartPlanActionByPlanIDForAutoEndAlarm(char* pcPlanID, char* pcPlanName, alarm_msg_t* pAlarmMsg, DBOper* pPlan_Srv_dboper)
{
    int i = 0;
    int record_count = 0;
    string strSQL = "";
    GBLogicDevice_info_t* pSourceGBLogicDeviceInfo = NULL;
    GBLogicDevice_info_t* pDestGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pDestGBDeviceInfo = NULL;
    int iHasPlanIDSend = 0;
    int while_count = 0;
    needtoproc_planaction_queue needToProc;
    EV9000_PlanActionConfig* plan_action = NULL;

    if (NULL == pcPlanID || NULL == pPlan_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG,  "StartPlanActionByPlanIDForAutoEndAlarm() exit---: Plan Srv DB Oper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() PlanID=%s \r\n", pcPlanID);

    /* 根据plan_id，查询预案动作表，获取预案的具体数据 */
    strSQL.clear();
    strSQL = "select * from PlanActionConfig WHERE PlanID = ";
    strSQL += pcPlanID;
    record_count = pPlan_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() ErrorMsg=%s\r\n", pPlan_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_WARN, "StartPlanActionByPlanIDForAutoEndAlarm() exit---: No Record Count \r\n");
        return 0;
    }

    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAutoEndAlarm() record_count=%d\r\n", record_count);

    needToProc.clear();

    /* 循环获取预案动作数据 */
    do
    {
        int iRet = 0;
        unsigned int uID = 0;
        unsigned int uType = 0;
        unsigned int uDeviceIndex = 0;
        int iStreamType = 0;
        unsigned int uDestIndex = 0;
        unsigned int uScreenID = -1;
        unsigned int uControlData = 0;
        unsigned int uUserIndex = 0;
        string strResved2 = "";

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_WARN, "StartPlanActionByPlanIDForAutoEndAlarm() While Count=%d \r\n", while_count);
        }

        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "StartPlanActionByPlanIDForAutoEndAlarm() record_count=%d\r\n", ++record_count);

        uID = 0;
        pPlan_Srv_dboper->GetFieldValue("ID", uID);

        uType = 0;
        pPlan_Srv_dboper->GetFieldValue("Type", uType);

        uDeviceIndex = 0;
        pPlan_Srv_dboper->GetFieldValue("DeviceIndex", uDeviceIndex);

        iStreamType = 0;
        pPlan_Srv_dboper->GetFieldValue("StreamType", iStreamType);

        uDestIndex = 0;
        pPlan_Srv_dboper->GetFieldValue("DestID", uDestIndex);

        uScreenID = 0;
        pPlan_Srv_dboper->GetFieldValue("ScreenID", uScreenID);

        uControlData = 0;
        pPlan_Srv_dboper->GetFieldValue("ControlData", uControlData);

        uUserIndex = 0;
        pPlan_Srv_dboper->GetFieldValue("Resved1", uUserIndex);

        strResved2.clear();
        pPlan_Srv_dboper->GetFieldValue("Resved2", strResved2);

        iRet = plan_action_init(&plan_action);

        if (0 != iRet || NULL == plan_action)
        {
            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() plan_action_init Error: PlanAction ID=%u\r\n", uID);
            continue;
        }

        plan_action->nID = uID;
        plan_action->nType = uType;
        plan_action->nDeviceIndex = uDeviceIndex;
        plan_action->nDestID = uDestIndex;
        plan_action->nScreenID = uScreenID;
        plan_action->nControlData = uControlData;
        plan_action->nStreamType = iStreamType;
        plan_action->nResved1 = uUserIndex;

        memset(plan_action->strResved2, 0, EV9000_SHORT_STRING_LEN);

        if (!strResved2.empty())
        {
            osip_strncpy(plan_action->strResved2, (char*)strResved2.c_str(), EV9000_SHORT_STRING_LEN - 4);
        }

        needToProc.push_back(plan_action);

        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "StartPlanActionByPlanIDForAutoEndAlarm() PlanAction ID=%u, Type=%u, DeviceIndex=%u, DestIndex=%u, StreamType=%d, ScreenID=%u, ControlData=%u\r\n", uID, uType, uDeviceIndex, uDestIndex, iStreamType, uScreenID, uControlData);
    }
    while (pPlan_Srv_dboper->MoveNext() >= 0);

    while (!needToProc.empty())
    {
        plan_action = (EV9000_PlanActionConfig*) needToProc.front();
        needToProc.pop_front();

        if (NULL != plan_action)
        {
            if (PLANACTION_TVWALL == plan_action->nType) /* 电视墙轮巡 */
            {
                if (plan_action->nDeviceIndex <= 0)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() uDeviceIndex=%u, uDestIndex=%u\r\n", plan_action->nDeviceIndex, plan_action->nDestID);
                    plan_action_free(plan_action);
                    plan_action = NULL;
                    continue;
                }

                /* 获取目的端的设备信息 */
                pDestGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(plan_action->nDestID);

                if (NULL != pDestGBLogicDeviceInfo)
                {
                    pDestGBDeviceInfo = GBDevice_info_get_by_stream_type(pDestGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

                    if (NULL != pDestGBDeviceInfo && EV9000_DEVICETYPE_DECODER == pDestGBDeviceInfo->device_type)
                    {
                        pSourceGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(plan_action->nDeviceIndex);

                        if (NULL != pSourceGBLogicDeviceInfo)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警自动结束触发执行预案打开视墙:电视墙通道ID=%s, 逻辑设备ID=%s", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan open TV wall:TV wall channel ID=%s, logic device ID=%s", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id);

                            if (plan_action->nStreamType <= 0)
                            {
                                i = start_connect_tv_proc(pSourceGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_MASTER, pDestGBLogicDeviceInfo->device_id, 0);
                            }
                            else
                            {
                                i = start_connect_tv_proc(pSourceGBLogicDeviceInfo->device_id, plan_action->nStreamType, pDestGBLogicDeviceInfo->device_id, 0);
                            }

                            if (i < 0)
                            {
                                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() start_connect_tv_proc Error: dest_id=%s, source_id=%s, i=%d\r\n", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "StartPlanActionByPlanIDForAutoEndAlarm() start_connect_tv_proc OK: dest_id=%s, source_id=%s, i=%d\r\n", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id, i);
                            }
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() GBLogicDevice_info_find_by_device_index Error, uDeviceIndex=%u\r\n", plan_action->nDeviceIndex);
                        }
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() DestGBDeviceInfo OR DestGBDeviceInfo Device Type Error, pDestGBDeviceInfo->device_id=%s, pDestGBDeviceInfo->device_type=%d\r\n", pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->device_type);
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() Find Dest GBLogic Device Info Error, DestIndex=%u\r\n", plan_action->nDestID);
                }
            }
            else if (PLANACTION_PC == plan_action->nType) /* PC屏幕轮巡 */
            {
                if (0 == iHasPlanIDSend)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警自动结束触发执行预案发送PC屏幕轮巡:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan send PC screen polling:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);

                    /* 发送给客户端 */
                    i = SendNotifyExecutePlanActionToOnlineUser(NULL, pcPlanID, pcPlanName, pPlan_Srv_dboper);

                    if (i < 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "报警自动结束触发执行预案发送PC屏幕轮巡失败:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "run plan send PC screen polling failed:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() SendNotifyExecutePlanActionToOnlineUser Error: i=%d\r\n", i);
                    }
                    else if (i > 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警自动结束触发执行预案发送PC屏幕轮巡成功:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan send PC screen polling success:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAutoEndAlarm() SendNotifyExecutePlanActionToOnlineUser OK:i=%d\r\n", i);
                    }

                    iHasPlanIDSend = 1;
                }
            }
            else if (PLANACTION_PRESET == plan_action->nType) /* 预置位 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警自动结束触发执行预案发送预置位:PresetID=%u", plan_action->nControlData);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan send preset position:PresetID=%u", plan_action->nControlData);

                i = ExecuteDevicePresetByPresetID(plan_action->nControlData, pPlan_Srv_dboper);

                if (0 != i)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "报警自动结束触发执行预案发送预置位处理失败:PresetID=%d", plan_action->nControlData);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "run plan send preset position failed:PresetID=%d", plan_action->nControlData);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() ExecuteDevicePresetByPresetID Error: uDeviceIndex=%u, uDestIndex=%u, uScreenID=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nDestID, plan_action->nScreenID, plan_action->nControlData, i);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警自动结束触发执行预案发送预置位处理成功:PresetID=%d", plan_action->nControlData);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Alarm automatically end trigger execution plans send preset treatment success:PresetID=%d", plan_action->nControlData);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAutoEndAlarm() ExecuteDevicePresetByPresetID OK: uDeviceIndex=%u, uDestIndex=%u, uScreenID=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nDestID, plan_action->nScreenID, plan_action->nControlData, i);
                }
            }

#if 0 /* 报警结束不要自动触发报警录像结束 ，TSU 会自动结束 */
            else if (PLANACTION_ALARM_RECORD == plan_action->nType) /* 报警录像 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行预案触发报警录像:DeviceIndex=%u, 类型=%d", plan_action->nDeviceIndex, plan_action->nControlData);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan Set Alarm Record:DeviceIndex=%u, Type=%d", plan_action->nDeviceIndex, plan_action->nControlData);

                i = ExecuteDeviceAlarmRecord(plan_action->nDeviceIndex, plan_action->nControlData, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() ExecuteDeviceAlarmRecord Error: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAutoEndAlarm() ExecuteDeviceAlarmRecord OK: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
            }

#endif
            else if (PLANACTION_SEND_MAIL == plan_action->nType) /* 发送邮件 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警自动结束触发执行预案触发发送邮件操作:UserIndex=%u", plan_action->nResved1);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan Send Email:UserIndex=%u", plan_action->nResved1);

                i = SendAlarmMsgMailToUserByUserIndex(plan_action->nResved1, pAlarmMsg, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() SendAlarmMsgMailToUserByUserIndex Error: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAutoEndAlarm() SendAlarmMsgMailToUserByUserIndex OK: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
            }
            else if (PLANACTION_CAPTURE_IMAGE == plan_action->nType) /* 抓图 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警自动结束触发执行预案触发抓取图片操作:DeviceIndex=%u", plan_action->nDeviceIndex);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan capture image:DeviceIndex=%u", plan_action->nDeviceIndex);

                //i = ExecuteDeviceCaptureImage(plan_action->nDeviceIndex, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() ExecuteDeviceCaptureImage Error: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAutoEndAlarm() ExecuteDeviceCaptureImage OK: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
            }
            else if (PLANACTION_EXECUTE_CRUISE == plan_action->nType) /* 执行巡航 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "报警自动结束触发执行预案触发执行巡航:巡航ID=%u", plan_action->nDestID);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan execute cruise:CruiseID=%u", plan_action->nDestID);

                i = start_cruise_srv_by_id_for_plan(plan_action->nDestID, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() start_cruise_srv_by_id_for_plan Error: CruiseID=%u, i=%d\r\n", plan_action->nDestID, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAutoEndAlarm() start_cruise_srv_by_id_for_plan OK: CruiseID=%u, i=%d\r\n", plan_action->nDestID, i);
                }
            }
            else if (PLANACTION_CONTROL_RCU == plan_action->nType) /* 执行RCU */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行预案触发执行控制RCU报警输出:DeviceIndex=%u, 控制数值=%s", plan_action->nDeviceIndex, plan_action->strResved2);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan control RCU:DeviceIndex=%u", plan_action->nDeviceIndex);

                i = ExecuteControlRCUDevice(plan_action->nDeviceIndex, plan_action->strResved2, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() ExecuteControlRCUDevice Error: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAutoEndAlarm() ExecuteControlRCUDevice OK: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
            }
            else
            {
                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAutoEndAlarm() PlanAction Type=%u Not Support\r\n", plan_action->nType);
                plan_action_free(plan_action);
                plan_action = NULL;
                continue;
            }

            plan_action_free(plan_action);
            plan_action = NULL;
        }
    }

    needToProc.clear();
    return 0;
}

/*****************************************************************************
 函 数 名  : StartPlanActionByPlanIDForFault
 功能描述  : 故障发生的时候根据预案编号启动预案动作
 输入参数  : char* pcPlanID
             char* pcPlanName
             fault_msg_t* pFaultMsg
             DBOper* pPlan_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StartPlanActionByPlanIDForFault(char* pcPlanID, char* pcPlanName, fault_msg_t* pFaultMsg, DBOper* pPlan_Srv_dboper)
{
    int i = 0;
    int record_count = 0;
    string strSQL = "";
    GBLogicDevice_info_t* pSourceGBLogicDeviceInfo = NULL;
    GBLogicDevice_info_t* pDestGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pDestGBDeviceInfo = NULL;
    int iHasPlanIDSend = 0;
    int while_count = 0;
    needtoproc_planaction_queue needToProc;
    EV9000_PlanActionConfig* plan_action = NULL;

    if (NULL == pcPlanID || NULL == pPlan_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG,  "StartPlanActionByPlanIDForFault() exit---: Plan Srv DB Oper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() PlanID=%s \r\n", pcPlanID);

    /* 根据plan_id，查询预案动作表，获取预案的具体数据 */
    strSQL.clear();
    strSQL = "select * from PlanActionConfig WHERE PlanID = ";
    strSQL += pcPlanID;
    record_count = pPlan_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() ErrorMsg=%s\r\n", pPlan_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_WARN, "StartPlanActionByPlanIDForFault() exit---: No Record Count \r\n");
        return 0;
    }

    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForFault() record_count=%d\r\n", record_count);

    needToProc.clear();

    /* 循环获取预案动作数据 */
    do
    {
        int iRet = 0;
        unsigned int uID = 0;
        unsigned int uType = 0;
        unsigned int uDeviceIndex = 0;
        int iStreamType = 0;
        unsigned int uDestIndex = 0;
        unsigned int uScreenID = -1;
        unsigned int uControlData = 0;
        unsigned int uUserIndex = 0;
        string strResved2 = "";

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_WARN, "StartPlanActionByPlanIDForFault() While Count=%d \r\n", while_count);
        }

        //DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "StartPlanActionByPlanID() record_count=%d\r\n", ++record_count);

        uID = 0;
        pPlan_Srv_dboper->GetFieldValue("ID", uID);

        uType = 0;
        pPlan_Srv_dboper->GetFieldValue("Type", uType);

        uDeviceIndex = 0;
        pPlan_Srv_dboper->GetFieldValue("DeviceIndex", uDeviceIndex);

        iStreamType = 0;
        pPlan_Srv_dboper->GetFieldValue("StreamType", iStreamType);

        uDestIndex = 0;
        pPlan_Srv_dboper->GetFieldValue("DestID", uDestIndex);

        uScreenID = 0;
        pPlan_Srv_dboper->GetFieldValue("ScreenID", uScreenID);

        uControlData = 0;
        pPlan_Srv_dboper->GetFieldValue("ControlData", uControlData);

        uUserIndex = 0;
        pPlan_Srv_dboper->GetFieldValue("Resved1", uUserIndex);

        strResved2.clear();
        pPlan_Srv_dboper->GetFieldValue("Resved2", strResved2);

        iRet = plan_action_init(&plan_action);

        if (0 != iRet || NULL == plan_action)
        {
            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() plan_action_init Error: PlanAction ID=%u\r\n", uID);
            continue;
        }

        plan_action->nID = uID;
        plan_action->nType = uType;
        plan_action->nDeviceIndex = uDeviceIndex;
        plan_action->nDestID = uDestIndex;
        plan_action->nScreenID = uScreenID;
        plan_action->nControlData = uControlData;
        plan_action->nStreamType = iStreamType;
        plan_action->nResved1 = uUserIndex;

        memset(plan_action->strResved2, 0, EV9000_SHORT_STRING_LEN);

        if (!strResved2.empty())
        {
            osip_strncpy(plan_action->strResved2, (char*)strResved2.c_str(), EV9000_SHORT_STRING_LEN - 4);
        }

        needToProc.push_back(plan_action);

        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "StartPlanActionByPlanIDForFault() PlanAction ID=%u, Type=%u, DeviceIndex=%u, DestIndex=%u, StreamType=%d, ScreenID=%u, ControlData=%u\r\n", uID, uType, uDeviceIndex, uDestIndex, iStreamType, uScreenID, uControlData);
    }
    while (pPlan_Srv_dboper->MoveNext() >= 0);

    while (!needToProc.empty())
    {
        plan_action = (EV9000_PlanActionConfig*) needToProc.front();
        needToProc.pop_front();

        if (NULL != plan_action)
        {
            if (PLANACTION_TVWALL == plan_action->nType) /* 电视墙轮巡 */
            {
                if (plan_action->nDeviceIndex <= 0)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() uDeviceIndex=%u, uDestIndex=%u\r\n", plan_action->nDeviceIndex, plan_action->nDestID);
                    plan_action_free(plan_action);
                    plan_action = NULL;
                    continue;
                }

                /* 获取目的端的设备信息 */
                pDestGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(plan_action->nDestID);

                if (NULL != pDestGBLogicDeviceInfo)
                {
                    pDestGBDeviceInfo = GBDevice_info_get_by_stream_type(pDestGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

                    if (NULL != pDestGBDeviceInfo && EV9000_DEVICETYPE_DECODER == pDestGBDeviceInfo->device_type)
                    {
                        pSourceGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(plan_action->nDeviceIndex);

                        if (NULL != pSourceGBLogicDeviceInfo)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "故障报警触发执行预案打开视墙:电视墙通道ID=%s, 逻辑设备ID=%s", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan open TV wall:TV wall channel ID=%s, logic device ID=%s", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id);

                            if (plan_action->nStreamType <= 0)
                            {
                                i = start_connect_tv_proc(pSourceGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_MASTER, pDestGBLogicDeviceInfo->device_id, 0);
                            }
                            else
                            {
                                i = start_connect_tv_proc(pSourceGBLogicDeviceInfo->device_id, plan_action->nStreamType, pDestGBLogicDeviceInfo->device_id, 0);
                            }

                            if (i < 0)
                            {
                                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() start_connect_tv_proc Error: dest_id=%s, source_id=%s, i=%d\r\n", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_INFO, "StartPlanActionByPlanIDForFault() start_connect_tv_proc OK: dest_id=%s, source_id=%s, i=%d\r\n", pDestGBLogicDeviceInfo->device_id, pSourceGBLogicDeviceInfo->device_id, i);
                            }
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() GBLogicDevice_info_find_by_device_index Error, uDeviceIndex=%u\r\n", plan_action->nDeviceIndex);
                        }
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() DestGBDeviceInfo OR DestGBDeviceInfo Device Type Error, pDestGBDeviceInfo->device_id=%s, pDestGBDeviceInfo->device_type=%d\r\n", pDestGBDeviceInfo->device_id, pDestGBDeviceInfo->device_type);
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() Find Dest GBLogic Device Info Error, DestIndex=%u\r\n", plan_action->nDestID);
                }
            }
            else if (PLANACTION_PC == plan_action->nType) /* PC屏幕轮巡 */
            {
                if (0 == iHasPlanIDSend)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "故障报警触发执行预案发送PC屏幕轮巡:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan send PC screen polling:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);

                    /* 发送给客户端 */
                    i = SendNotifyExecutePlanActionToOnlineUser(NULL, pcPlanID, pcPlanName, pPlan_Srv_dboper);

                    if (i < 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "故障报警触发执行预案发送PC屏幕轮巡失败:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "run plan send PC screen polling failed:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() SendNotifyExecutePlanActionToOnlineUser Error: i=%d\r\n", i);
                    }
                    else if (i > 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "故障报警触发执行预案发送PC屏幕轮巡成功:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan send PC screen polling success:PlanID=%s, PlanName=%s", pcPlanID, pcPlanName);
                        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForFault() SendNotifyExecutePlanActionToOnlineUser OK:i=%d\r\n", i);
                    }

                    iHasPlanIDSend = 1;
                }
            }
            else if (PLANACTION_PRESET == plan_action->nType) /* 预置位 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "故障报警触发执行预案发送预置位:PresetID=%u", plan_action->nControlData);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan send preset position:PresetID=%u", plan_action->nControlData);

                i = ExecuteDevicePresetByPresetID(plan_action->nControlData, pPlan_Srv_dboper);

                if (0 != i)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "故障报警触发执行预案发送预置位处理失败:PresetID=%d", plan_action->nControlData);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "run plan send preset position failed:PresetID=%d", plan_action->nControlData);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() ExecuteDevicePresetByPresetID Error: uDeviceIndex=%u, uDestIndex=%u, uScreenID=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nDestID, plan_action->nScreenID, plan_action->nControlData, i);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "故障报警触发执行预案发送预置位处理成功:PresetID=%d", plan_action->nControlData);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan send preset position success:PresetID=%d", plan_action->nControlData);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForFault() ExecuteDevicePresetByPresetID OK: uDeviceIndex=%u, uDestIndex=%u, uScreenID=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nDestID, plan_action->nScreenID, plan_action->nControlData, i);
                }
            }
            else if (PLANACTION_ALARM_RECORD == plan_action->nType) /* 报警录像 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "故障报警触发执行预案报警录像:DeviceIndex=%u, 类型=%d", plan_action->nDeviceIndex, plan_action->nControlData);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan Set Alarm Record:DeviceIndex=%u, Type=%d", plan_action->nDeviceIndex, plan_action->nControlData);

                i = ExecuteDeviceAlarmRecord(plan_action->nDeviceIndex, plan_action->nControlData, pPlan_Srv_dboper);

                if (0 != i)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "故障报警触发执行预案报警录像失败:DeviceIndex=%u, 类型=%d", plan_action->nDeviceIndex, plan_action->nControlData);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "run plan Set Alarm Record failed:DeviceIndex=%u, 类型=%d", plan_action->nDeviceIndex, plan_action->nControlData);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForAlarm() ExecuteDeviceAlarmRecord Error: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "故障报警触发执行预案报警录像成功:DeviceIndex=%u, 类型=%d", plan_action->nDeviceIndex, plan_action->nControlData);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan Set Alarm Record success:DeviceIndex=%u, 类型=%d", plan_action->nDeviceIndex, plan_action->nControlData);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForAlarm() ExecuteDeviceAlarmRecord OK: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
            }
            else if (PLANACTION_SEND_MAIL == plan_action->nType) /* 发送邮件 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "故障报警触发执行预案触发发送邮件操作:UserIndex=%u", plan_action->nResved1);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan Send Email:UserIndex=%u", plan_action->nResved1);

                i = SendFaultMsgMailToUserByUserIndex(plan_action->nResved1, pFaultMsg, pPlan_Srv_dboper);

                if (0 != i)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "故障报警触发执行预案触发发送邮件操作失败:UserIndex=%u", plan_action->nResved1);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Fault alarm trigger execution plan trigger email operation failed:UserIndex=%u", plan_action->nResved1);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() SendFaultMsgMailToUserByUserIndex Error: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "故障报警触发执行预案触发发送邮件操作成功:UserIndex=%u", plan_action->nResved1);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Fault alarm trigger execution plan trigger email operation successfully:UserIndex=%u", plan_action->nResved1);
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForFault() SendFaultMsgMailToUserByUserIndex OK: uDeviceIndex=%u, uControlData=%u, i=%d\r\n", plan_action->nDeviceIndex, plan_action->nControlData, i);
                }
            }
            else if (PLANACTION_CAPTURE_IMAGE == plan_action->nType) /* 抓图 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "故障报警触发执行预案触发抓取图片操作:DeviceIndex=%u", plan_action->nDeviceIndex);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan capture image:DeviceIndex=%u", plan_action->nDeviceIndex);

                //i = ExecuteDeviceCaptureImage(plan_action->nDeviceIndex, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() ExecuteDeviceCaptureImage Error: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForFault() ExecuteDeviceCaptureImage OK: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
            }
            else if (PLANACTION_EXECUTE_CRUISE == plan_action->nType) /* 执行巡航 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "故障报警触发执行预案触发执行巡航:巡航ID=%u", plan_action->nDestID);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan execute cruise:CruiseID=%u", plan_action->nDestID);

                i = start_cruise_srv_by_id_for_plan(plan_action->nDestID, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() start_cruise_srv_by_id_for_plan Error: CruiseID=%u, i=%d\r\n", plan_action->nDestID, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForFault() start_cruise_srv_by_id_for_plan OK: CruiseID=%u, i=%d\r\n", plan_action->nDestID, i);
                }
            }
            else if (PLANACTION_CONTROL_RCU == plan_action->nType) /* 执行RCU */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "执行预案触发执行控制RCU报警输出:DeviceIndex=%u, 控制数值=%s", plan_action->nDeviceIndex, plan_action->strResved2);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "run plan control RCU:DeviceIndex=%u", plan_action->nDeviceIndex);

                i = ExecuteControlRCUDevice(plan_action->nDeviceIndex, plan_action->strResved2, pPlan_Srv_dboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() ExecuteControlRCUDevice Error: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "StartPlanActionByPlanIDForFault() ExecuteControlRCUDevice OK: uDeviceIndex=%u, i=%d\r\n", plan_action->nDeviceIndex, i);
                }
            }
            else
            {
                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "StartPlanActionByPlanIDForFault() PlanAction Type=%u Not Support\r\n", plan_action->nType);
                plan_action_free(plan_action);
                plan_action = NULL;
                continue;
            }

            plan_action_free(plan_action);
            plan_action = NULL;
        }
    }

    needToProc.clear();
    return 0;
}

/*****************************************************************************
 函 数 名  : SendNotifyExecutePlanActionToOnlineUser
 功能描述  : 发送预案动作给在线用户
 输入参数  : user_info_t* pUserInfo
             char* pcPlanID
             char* pcPlanName
             DBOper* pPlan_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月14日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendNotifyExecutePlanActionToOnlineUser(user_info_t* pUserInfo, char* pcPlanID, char* pcPlanName, DBOper* pPlan_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    int index = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    vector<unsigned int> UserIndexVector;
    int iUserIndexCount = 0;
    unsigned int uUserIndex = 0;

    /*
     <?xml version="1.0"?>
         <Notify>
         <CmdType>ExecutePlan</CmdType>
         <SN>1234</SN>
         <PlanID>预案ID</PlanID>
         </Notify>
     */

    /* 组建XML信息 */
    outPacket.SetRootTag("Notify");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"ExecutePlan");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, (char*)"1234");

    AccNode = outPacket.CreateElement((char*)"PlanID");

    if (NULL != pcPlanID)
    {
        outPacket.SetElementValue(AccNode, pcPlanID);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    AccNode = outPacket.CreateElement((char*)"PlanName");

    if (NULL != pcPlanName)
    {
        outPacket.SetElementValue(AccNode, pcPlanName);
    }
    else
    {
        outPacket.SetElementValue(AccNode, (char*)"");
    }

    if (NULL != pUserInfo) /* 如果是用户手动启动的预案，不发给用户 */
    {
        //i = SendMessageToExceptOnlineUser(pUserInfo, local_cms_id_get(), (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());
    }
    else
    {
        /* 获取plan用户权限表 */
        UserIndexVector.clear();
        iRet = get_user_index_from_user_plan_config(pcPlanID, UserIndexVector, pPlan_Srv_dboper);

        iUserIndexCount = UserIndexVector.size();

        if (iUserIndexCount <= 0)
        {
            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_WARN, "SendNotifyExecutePlanActionToOnlineUser() exit---: Get User Index NULL \r\n");
            return 0;
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送预案执行通知给在线客户端: 查询到的用户索引总数=%d", iUserIndexCount);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send the plan to the execution notice online client: total number of queries to the index of the user=%d", iUserIndexCount);

        /* 循环发送数据 */
        for (index = 0; index < iUserIndexCount; index++)
        {
            /* 获取用户索引 */
            uUserIndex = UserIndexVector[index];

            DEBUG_TRACE(MODULE_PLAN_SRV, LOG_TRACE, "SendNotifyExecutePlanActionToOnlineUser() index=%d, UserIndex=%u \r\n", index, uUserIndex);

            i |= SendMessageToOnlineUserByUserIndex(uUserIndex, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : get_user_index_from_user_plan_config
 功能描述  : 从用户预案权限表里面获取用户索引
 输入参数  : char* pcPlanID
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
int get_user_index_from_user_plan_config(char* pcPlanID, vector<unsigned int>& UserIndexVector, DBOper* pDBOper)
{
    int iRet = 0;
    int record_count = 0;
    int while_count = 0;
    string strSQL = "";

    if (NULL == pcPlanID || NULL == pDBOper)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_DEBUG, "get_user_index_from_user_plan_config() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "SELECT UserID FROM UserPlanConfig WHERE PlanID = ";
    strSQL += pcPlanID;

    record_count = pDBOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "get_user_index_from_user_plan_config() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "get_user_index_from_user_plan_config() ErrorMsg=%s\r\n", pDBOper->GetLastDbErrorMsg());
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
                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_WARN, "get_user_index_from_user_plan_config() While Count=%d \r\n", while_count);
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
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "get_user_index_from_user_plan_config() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "get_user_index_from_user_plan_config() ErrorMsg=%s\r\n", pDBOper->GetLastDbErrorMsg());
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
                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_WARN, "get_user_index_from_user_plan_config() While Count=%d \r\n", while_count);
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
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "get_user_index_from_user_plan_config() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_PLAN_SRV, LOG_ERROR, "get_user_index_from_user_plan_config() ErrorMsg=%s\r\n", pDBOper->GetLastDbErrorMsg());
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
                DEBUG_TRACE(MODULE_PLAN_SRV, LOG_WARN, "get_user_index_from_user_plan_config() While Count=%d \r\n", while_count);
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
 函 数 名  : PlanSrvConfig_db_refresh_proc
 功能描述  : 设置预案业务配置信息数据库更新操作标识
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
int PlanSrvConfig_db_refresh_proc()
{
    if (1 == db_PlanSrvInfo_reload_mark) /* 正在执行 */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "预案业务配置数据库信息正在同步");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Plan Srv Info database information are synchronized");
        return 0;
    }

    db_PlanSrvInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 函 数 名  : check_PlanSrvConfig_need_to_reload_begin
 功能描述  : 检查是否需要同步预案业务配置开始
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
void check_PlanSrvConfig_need_to_reload_begin(DBOper* pDboper)
{
    /* 检查是否需要更新数据库标识 */
    if (!db_PlanSrvInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步预案业务配置数据库信息: 开始---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization Plan Srv info database information: begain---");

    /* 设置删除标识 */
    set_plan_srv_list_del_mark(1);

    /* 检查数据中是否有可以执行的预案数据 */
    check_db_data_to_plan_srv_list_for_start(pDboper);

    return;
}

/*****************************************************************************
 函 数 名  : check_PlanSrvConfig_need_to_reload_end
 功能描述  : 检查是否需要同步预案业务配置表结束
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
void check_PlanSrvConfig_need_to_reload_end()
{
    /* 检查是否需要更新数据库标识 */
    if (!db_PlanSrvInfo_reload_mark)
    {
        return;
    }

    /* 删除已经过期的预案数据 */
    delete_plan_srv_data();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步预案业务配置数据库信息: 结束---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization Plan Srv info database information: end---");
    db_PlanSrvInfo_reload_mark = 0;

    return;
}

