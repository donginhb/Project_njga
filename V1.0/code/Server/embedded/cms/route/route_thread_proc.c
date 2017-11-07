
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
#include "common/log_proc.inc"

#include "route/route_thread_proc.inc"
#include "route/route_reg_proc.inc"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern char g_StrCon[2][100];
extern char g_StrConLog[2][100];
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
#define MAX_ROUTE_SRV_THREADS 10

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
route_srv_proc_tl_list_t* g_RouteSrvProcThreadList = NULL;            /* 上级路由业务处理线程队列 */

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#if DECS("上级路由业务处理线程")
/*****************************************************************************
 函 数 名  : route_srv_proc_thread_for_appoint_execute
 功能描述  :  上级路由业务处理线程
 输入参数  : appoint_route_srv_proc_tl_t * route_srv_proc
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void* route_srv_proc_thread_for_appoint_execute(void* p)
{
    int iRet = 0;
    route_srv_proc_tl_t* run = (route_srv_proc_tl_t*)p;
    static int clean_flag = 0;

    if (run == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_for_appoint_execute() exit---: Param Error \r\n");
        return NULL;
    }

    while (!run->th_exit)
    {
        if (0 == cms_run_status)
        {
            run->run_time = time(NULL);
            osip_usleep(1000000);

            if (0 == clean_flag)
            {
                clean_flag = 1;
                appoint_route_srv_msg_list_clean(run);
            }

            continue;
        }

        if (1 == clean_flag)
        {
            clean_flag = 0;
        }

        if (0 == run->iUsed)
        {
            run->run_time = time(NULL);
            osip_usleep(1000000);
            continue;
        }

        if (NULL == run->pRouteInfo)
        {
            run->run_time = time(NULL);
            osip_usleep(1000000);
            continue;
        }

        if (NULL == run->pRoute_Srv_dboper)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_srv_proc_thread_for_appoint_execute() Route Srv DB Oper NULL: route_id=%s,login_ip=%s,login_port=%d \r\n", run->pRouteInfo->server_id, run->pRouteInfo->server_ip, run->pRouteInfo->server_port);
            run->run_time = time(NULL);
            osip_usleep(1000000);
            continue;
#if 0
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_for_appoint_execute() Route Srv DB Oper New:route_id=%s,login_ip=%s,login_port=%d \r\n", run->route_id, run->login_ip, run->login_port);

            run->pRoute_Srv_dboper = new DBOper();

            if (run->pRoute_Srv_dboper == NULL)
            {
                osip_usleep(1000000);
                continue;
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_for_appoint_execute() Route Srv DB Oper Connect Start:g_StrCon=%s\r\n", g_StrCon.c_str());

            if (run->pRoute_Srv_dboper->Connect(g_StrCon.c_str(), (char*)"") < 0)
            {
                delete run->pRoute_Srv_dboper;
                run->pRoute_Srv_dboper = NULL;
                osip_usleep(1000000);
                continue;
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_for_appoint_execute() Route Srv DB Oper Connect End \r\n");
#endif
        }

        if (NULL == run->pRouteSrvMsgQueue)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_srv_proc_thread_for_appoint_execute() Route Srv Msg Queue NULL: route_id=%s,login_ip=%s,login_port=%d \r\n", run->pRouteInfo->server_id, run->pRouteInfo->server_ip, run->pRouteInfo->server_port);
            run->run_time = time(NULL);
            osip_usleep(1000000);
            continue;
#if 0
            run->pRouteSrvMsgQueue = new route_srv_msg_queue;

            if (NULL == run->pRouteSrvMsgQueue)
            {
                osip_usleep(5000);
                continue;
            }

            run->pRouteSrvMsgQueue->clear();
#endif
        }

        iRet = scan_appoint_route_srv_msg_list(run);

        run->run_time = time(NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_for_appoint_execute() update thread run time:run_time=%d \r\n", run->run_time);

        osip_usleep(5000);
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : route_srv_proc_thread_init
 功能描述  : 上级路由业务处理线程初始化
 输入参数  : route_srv_proc_tl_t** run
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_srv_proc_thread_init(route_srv_proc_tl_t** run)
{
    *run = (route_srv_proc_tl_t*) osip_malloc(sizeof(route_srv_proc_tl_t));

    if (*run == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_init() exit---: *run Smalloc Error \r\n");
        return -1;
    }

    (*run)->iUsed = 0;
    (*run)->pRouteInfo = NULL;
    (*run)->thread = NULL;
    (*run)->th_exit = 0;
    (*run)->run_time = 0;
    (*run)->pRoute_Srv_dboper = NULL;
    (*run)->pRouteLogDbOper = NULL;
    (*run)->iRouteLogDBOperConnectStatus = 0;
    (*run)->pRouteSrvMsgQueue = NULL;

#ifdef MULTI_THR
    /* init smutex */
    (*run)->pRouteSrvMsgQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == (*run)->pRouteSrvMsgQueueLock)
    {
        osip_free(*run);
        run = NULL;
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_srv_proc_thread_init() exit---: Route Service Message List Lock Init Error \r\n");
        return -1;
    }

#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : route_srv_proc_thread_free
 功能描述  : 上级路由业务处理线程释放
 输入参数  : route_srv_proc_tl_t *run
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void route_srv_proc_thread_free(route_srv_proc_tl_t* run)
{
    route_srv_msg_t* route_srv_msg = NULL;

    if (run == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_free() exit---: Param Error \r\n");
        return;
    }

    run->iUsed = 0;
    run->pRouteInfo = NULL;

    if (run->thread)
    {
        osip_free(run->thread);
        run->thread = NULL;
    }

    run->run_time = 0;

    if (run->pRoute_Srv_dboper != NULL)
    {
        delete run->pRoute_Srv_dboper;
        run->pRoute_Srv_dboper = NULL;
    }

    if (run->pRouteLogDbOper != NULL)
    {
        delete run->pRouteLogDbOper;
        run->pRouteLogDbOper = NULL;
    }

    run->iRouteLogDBOperConnectStatus = 0;

    if (NULL != run->pRouteSrvMsgQueue)
    {
        while (!run->pRouteSrvMsgQueue->empty())
        {
            route_srv_msg = (route_srv_msg_t*) run->pRouteSrvMsgQueue->front();
            run->pRouteSrvMsgQueue->pop_front();

            if (NULL != route_srv_msg)
            {
                route_srv_msg_free(route_srv_msg);
                route_srv_msg = NULL;
            }
        }

        run->pRouteSrvMsgQueue->clear();
        delete run->pRouteSrvMsgQueue;
        run->pRouteSrvMsgQueue = NULL;
    }

#ifdef MULTI_THR

    if (NULL != run->pRouteSrvMsgQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)run->pRouteSrvMsgQueueLock);
        run->pRouteSrvMsgQueueLock = NULL;
    }

#endif

    osip_free(run);
    run = NULL;

    return;
}
#endif

#if DECS("上级路由业务处理线程队列")
/*****************************************************************************
 函 数 名  : route_srv_proc_thread_assign
 功能描述  : 上级路由业务处理线程分配
 输入参数  : route_info_t* pRouteInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月27日
    作    者   : 上级路由路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
int route_srv_proc_thread_assign(route_info_t* pRouteInfo)
{
    route_srv_proc_tl_t* runthread = NULL;

    if (NULL == pRouteInfo || pRouteInfo->server_id[0] == '\0' || pRouteInfo->server_ip[0] == '\0' || pRouteInfo->server_port <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_assign() exit---: Param Error \r\n");
        return -1;
    }

    //printf("\r\n route_srv_proc_thread_assign() Enter--- \r\n");
    //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_assign() Enter--- \r\n");

    ROUTE_SRV_PROC_THREAD_SMUTEX_LOCK();

    runthread = get_free_route_srv_proc_thread();

    if (NULL == runthread)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_srv_proc_thread_assign() exit---: Get Free Thread Error \r\n");
        ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return -1;
    }

    runthread->iUsed = 1;
    runthread->pRouteInfo = pRouteInfo;

    /* 初始化消息队列 */
    if (NULL == runthread->pRouteSrvMsgQueue)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_assign() Route Srv Msg Queue: Begin--- \r\n");
        runthread->pRouteSrvMsgQueue = new route_srv_msg_queue;
        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_assign() Route Srv Msg Queue: End--- \r\n");
    }

    //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_assign() Route Srv Msg Queue Clear: Begin---\r\n");

    runthread->pRouteSrvMsgQueue->clear();

    //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_assign() Route Srv Msg Queue Clear: End--- \r\n");

    /* 初始化数据库连接 */
    if (NULL == runthread->pRoute_Srv_dboper)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_assign() Route Srv DB Oper New:route_id=%s, route_ip=%s, route_port=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

        runthread->pRoute_Srv_dboper = new DBOper();

        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_assign() Route Srv DB Oper New End \r\n");

        if (runthread->pRoute_Srv_dboper == NULL)
        {
            //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_assign() exit---: Route Srv DB Oper NULL \r\n");
            ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return -1;
        }

        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_assign() Route Srv DB Start Connect:g_StrCon[0]=%s, g_StrCon[1]=%s \r\n", g_StrCon[0], g_StrCon[1]);

        if (runthread->pRoute_Srv_dboper->Connect(g_StrCon, (char*)"") < 0)
        {
            delete runthread->pRoute_Srv_dboper;
            runthread->pRoute_Srv_dboper = NULL;
            ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return -1;
        }

        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_assign() Route Srv DB Connect End \r\n");
    }

    /* 初始化日志数据库连接 */
    if (NULL == runthread->pRouteLogDbOper)
    {
        runthread->pRouteLogDbOper = new DBOper();

        if (runthread->pRouteLogDbOper == NULL)
        {
            delete runthread->pRoute_Srv_dboper;
            runthread->pRoute_Srv_dboper = NULL;
            ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return -1;
        }

        if (runthread->pRouteLogDbOper->Connect(g_StrConLog, (char*)"") < 0)
        {
            runthread->iRouteLogDBOperConnectStatus = 0;
        }
        else
        {
            runthread->iRouteLogDBOperConnectStatus = 1;
        }
    }

    ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_assign() Exit--- \r\n");

    //printf("\r\n route_srv_proc_thread_assign() Exit--- \r\n");
    return 0;
}

/*****************************************************************************
 函 数 名  : route_srv_proc_thread_recycle
 功能描述  : 上级路由业务处理线程回收
 输入参数  : char* route_id
             char* route_ip
             int route_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月27日
    作    者   : 上级路由路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
int route_srv_proc_thread_recycle(char* route_id, char* route_ip, int route_port)
{
    route_srv_msg_t* route_srv_msg = NULL;
    route_srv_proc_tl_t* runthread = NULL;

    if (NULL == route_id || NULL == route_ip || route_port <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_recycle() exit---: Param Error \r\n");
        return -1;
    }

    //rintf("\r\n route_srv_proc_thread_recycle() Enter--- \r\n");
    //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_recycle() Enter--- \r\n");

    ROUTE_SRV_PROC_THREAD_SMUTEX_LOCK();

    runthread = get_route_srv_proc_thread2(route_id, route_ip, route_port);

    if (NULL == runthread)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_recycle() exit---: Get Thread Error \r\n");
        ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return 0;
    }

    runthread->iUsed = 0;
    runthread->pRouteInfo = NULL;

    if (NULL != runthread->pRoute_Srv_dboper)
    {
        delete runthread->pRoute_Srv_dboper;
        runthread->pRoute_Srv_dboper = NULL;
        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_recycle() Route Srv DB Oper Delete End--- \r\n");
    }

    if (NULL != runthread->pRouteLogDbOper)
    {
        delete runthread->pRouteLogDbOper;
        runthread->pRouteLogDbOper = NULL;
    }

    runthread->iRouteLogDBOperConnectStatus = 0;

    if (NULL != runthread->pRouteSrvMsgQueue)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_recycle() RouteSrvMsgQueue Free \r\n");

        while (!runthread->pRouteSrvMsgQueue->empty())
        {
            route_srv_msg = (route_srv_msg_t*) runthread->pRouteSrvMsgQueue->front();
            runthread->pRouteSrvMsgQueue->pop_front();

            if (NULL != route_srv_msg)
            {
                route_srv_msg_free(route_srv_msg);
                route_srv_msg = NULL;
            }
        }

        runthread->pRouteSrvMsgQueue->clear();
        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_recycle() RouteSrvMsgQueue Free End--- \r\n");
    }

    ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    //printf("\r\n route_srv_proc_thread_recycle() Exit--- \r\n");
    //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_srv_proc_thread_recycle() Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : route_srv_proc_thread_start_all
 功能描述  : 启动上级路由业务处理线程
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_srv_proc_thread_start_all()
{
    int i = 0;
    int index = 0;
    route_srv_proc_tl_t* runthread = NULL;

    if (NULL == g_RouteSrvProcThreadList || NULL == g_RouteSrvProcThreadList->pRouteSrvProcList)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_start() exit---: Param Error 2 \r\n");
        return -1;
    }

    ROUTE_SRV_PROC_THREAD_SMUTEX_LOCK();

    for (index = 0; index < MAX_ROUTE_SRV_THREADS; index++)
    {
        i = route_srv_proc_thread_init(&runthread);
        //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_srv_proc_thread_start() route_srv_proc_thread_init:i=%d \r\n", i);

        if (i != 0)
        {
            //DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_srv_proc_thread_start() exit---: Route Srv Proc Thread Init Error \r\n");
            continue;
        }

        //添加到上级路由业务处理线程队列
        i = osip_list_add(g_RouteSrvProcThreadList->pRouteSrvProcList, runthread, -1); /* add to list tail */
        //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_srv_proc_thread_start() osip_list_add:i=%d \r\n", i);

        if (i < 0)
        {
            route_srv_proc_thread_free(runthread);
            runthread = NULL;
            //DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_srv_proc_thread_start() exit---: List Add Error \r\n");
            continue;
        }

        //启动处理线程
        runthread->thread = (osip_thread_t*)osip_thread_create(20000, route_srv_proc_thread_for_appoint_execute, (void*)runthread);

        if (runthread->thread == NULL)
        {
            osip_list_remove(g_RouteSrvProcThreadList->pRouteSrvProcList, i);
            route_srv_proc_thread_free(runthread);
            runthread = NULL;
            //DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_srv_proc_thread_start() exit---: Route Srv Proc Thread Create Error \r\n");
            continue;
        }

        //printf("\r\n route_srv_proc_thread_start:runthread->thread=0x%lx \r\n", (unsigned long)runthread->thread);
    }

    ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : route_srv_proc_thread_stop_all
 功能描述  : 停止所有上级路由业务处理线程
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_srv_proc_thread_stop_all()
{
    int pos = 0;
    int i = 0;
    route_srv_proc_tl_t* runthread = NULL;

    if (NULL == g_RouteSrvProcThreadList || NULL == g_RouteSrvProcThreadList->pRouteSrvProcList)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_stop_all() exit---: Param Error \r\n");
        return -1;
    }

    ROUTE_SRV_PROC_THREAD_SMUTEX_LOCK();

    //查找队列，停止线程
    for (pos = 0; pos < osip_list_size(g_RouteSrvProcThreadList->pRouteSrvProcList); pos++)
    {
        runthread = (route_srv_proc_tl_t*)osip_list_get(g_RouteSrvProcThreadList->pRouteSrvProcList, pos);

        if (runthread == NULL)
        {
            continue;
        }

        runthread->th_exit = 1;

        if (runthread->thread != NULL)
        {
            i = osip_thread_join((struct osip_thread*)runthread->thread);
        }
    }

    ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : route_srv_proc_thread_restart
 功能描述  : 上级路由业务处理线程重新启动
 输入参数  : route_info_t* pRouteInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月12日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_srv_proc_thread_restart(route_info_t* pRouteInfo)
{
    int pos = 0;
    int iRet = 0;
    route_srv_proc_tl_t* runthread = NULL;

    if (NULL == pRouteInfo || pRouteInfo->server_id[0] == '\0' || pRouteInfo->server_ip[0] == '\0' || pRouteInfo->server_port <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_restart() exit---: Param Error \r\n");
        return -1;
    }

    ROUTE_SRV_PROC_THREAD_SMUTEX_LOCK();

    //查找队列
    for (pos = 0; pos < osip_list_size(g_RouteSrvProcThreadList->pRouteSrvProcList); pos++)
    {
        runthread = (route_srv_proc_tl_t*)osip_list_get(g_RouteSrvProcThreadList->pRouteSrvProcList, pos);

        if (runthread == NULL || 0 == runthread->iUsed || NULL == runthread->pRouteInfo)
        {
            continue;
        }

        if ('\0' == runthread->pRouteInfo->server_id[0] || '\0' == runthread->pRouteInfo->server_ip[0] || runthread->pRouteInfo->server_port <= 0)
        {
            continue;
        }

        if ((0 == sstrcmp(runthread->pRouteInfo->server_id, pRouteInfo->server_id))
            && (0 == sstrcmp(runthread->pRouteInfo->server_ip, pRouteInfo->server_ip))
            && runthread->pRouteInfo->server_port == pRouteInfo->server_port)
        {
            /* 停止 */
            runthread->th_exit = 1;

            if (runthread->thread != NULL)
            {
                iRet = osip_thread_join((struct osip_thread*)runthread->thread);
                osip_free(runthread->thread);
                runthread->thread = NULL;
            }

            /* 启动 */
            runthread->thread = (osip_thread_t*)osip_thread_create(20000, route_srv_proc_thread_for_appoint_execute, (void*)runthread);

            if (runthread->thread == NULL)
            {
                osip_list_remove(g_RouteSrvProcThreadList->pRouteSrvProcList, pos);
                route_srv_proc_thread_free(runthread);
                runthread = NULL;
                continue;
            }
        }
    }

    ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : route_srv_proc_thread_find
 功能描述  : 查找上级路由业务处理线程
 输入参数  : route_info_t* pRouteInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_srv_proc_thread_find(route_info_t* pRouteInfo)
{
    int pos = 0;
    route_srv_proc_tl_t* runthread = NULL;

    if (NULL == pRouteInfo || pRouteInfo->server_id[0] == '\0' || pRouteInfo->server_ip[0] == '\0' || pRouteInfo->server_port <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_find() exit---: Param Error \r\n");
        return -1;
    }

    ROUTE_SRV_PROC_THREAD_SMUTEX_LOCK();

    //查找队列
    for (pos = 0; pos < osip_list_size(g_RouteSrvProcThreadList->pRouteSrvProcList); pos++)
    {
        runthread = (route_srv_proc_tl_t*)osip_list_get(g_RouteSrvProcThreadList->pRouteSrvProcList, pos);

        if (runthread == NULL || 0 == runthread->iUsed || NULL == runthread->pRouteInfo)
        {
            continue;
        }

        if ('\0' == runthread->pRouteInfo->server_id[0] || '\0' == runthread->pRouteInfo->server_ip[0] || runthread->pRouteInfo->server_port <= 0)
        {
            continue;
        }

        if ((0 == sstrcmp(runthread->pRouteInfo->server_id, pRouteInfo->server_id))
            && (0 == sstrcmp(runthread->pRouteInfo->server_ip, pRouteInfo->server_ip))
            && runthread->pRouteInfo->server_port == pRouteInfo->server_port)
        {
            ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return pos;
        }
    }

    ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return -1;
}

/*****************************************************************************
 函 数 名  : get_free_route_srv_proc_thread
 功能描述  : 获取空闲的上级路由业务处理线程
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月27日
    作    者   : 上级路由路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
route_srv_proc_tl_t* get_free_route_srv_proc_thread()
{
    int pos = 0;
    route_srv_proc_tl_t* runthread = NULL;

    //查找队列，将sipudp从接收线程队列中移除
    for (pos = 0; pos < osip_list_size(g_RouteSrvProcThreadList->pRouteSrvProcList); pos++)
    {
        runthread = (route_srv_proc_tl_t*)osip_list_get(g_RouteSrvProcThreadList->pRouteSrvProcList, pos);

        if (runthread == NULL)
        {
            continue;
        }

        if (0 == runthread->iUsed)
        {
            return runthread;
        }
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : get_route_srv_proc_thread
 功能描述  : 获取上级路由业务处理线程
 输入参数  : char* route_id
             char* route_ip
             int route_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月27日
    作    者   : 上级路由路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
route_srv_proc_tl_t* get_route_srv_proc_thread(char* route_id, char* route_ip, int route_port)
{
    int pos = 0;
    route_srv_proc_tl_t* runthread = NULL;

    if (NULL == route_id || NULL == route_ip || route_port <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "get_route_srv_proc_thread() exit---: Param Error \r\n");
        return NULL;
    }

    ROUTE_SRV_PROC_THREAD_SMUTEX_LOCK();

    for (pos = 0; pos < osip_list_size(g_RouteSrvProcThreadList->pRouteSrvProcList); pos++)
    {
        runthread = (route_srv_proc_tl_t*)osip_list_get(g_RouteSrvProcThreadList->pRouteSrvProcList, pos);

        if (runthread == NULL || 0 == runthread->iUsed || NULL == runthread->pRouteInfo)
        {
            continue;
        }

        if ('\0' == runthread->pRouteInfo->server_id[0] || '\0' == runthread->pRouteInfo->server_ip[0] || runthread->pRouteInfo->server_port <= 0)
        {
            continue;
        }

        if ((0 == sstrcmp(runthread->pRouteInfo->server_id, route_id))
            && (0 == sstrcmp(runthread->pRouteInfo->server_ip, route_ip))
            && runthread->pRouteInfo->server_port == route_port)
        {
            ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "get_route_srv_proc_thread() exit---: pos=%d, route_id=%s, route_ip=%s, route_port=%d \r\n", pos, route_id, route_ip, route_port);
            return runthread;
        }
    }

    ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : get_route_srv_proc_thread2
 功能描述  : 获取上级路由业务处理线程
 输入参数  : char* route_id
             char* route_ip
             int route_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月13日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
route_srv_proc_tl_t* get_route_srv_proc_thread2(char* route_id, char* route_ip, int route_port)
{
    int pos = 0;
    route_srv_proc_tl_t* runthread = NULL;

    if (NULL == route_id || NULL == route_ip || route_port <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "get_route_srv_proc_thread2() exit---: Param Error \r\n");
        return NULL;
    }

    for (pos = 0; pos < osip_list_size(g_RouteSrvProcThreadList->pRouteSrvProcList); pos++)
    {
        runthread = (route_srv_proc_tl_t*)osip_list_get(g_RouteSrvProcThreadList->pRouteSrvProcList, pos);

        if (runthread == NULL || 0 == runthread->iUsed || NULL == runthread->pRouteInfo)
        {
            continue;
        }

        if ('\0' == runthread->pRouteInfo->server_id[0] || '\0' == runthread->pRouteInfo->server_ip[0] || runthread->pRouteInfo->server_port <= 0)
        {
            continue;
        }

        if ((0 == sstrcmp(runthread->pRouteInfo->server_id, route_id))
            && (0 == sstrcmp(runthread->pRouteInfo->server_ip, route_ip))
            && runthread->pRouteInfo->server_port == route_port)
        {
            return runthread;
        }
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : route_srv_proc_thread_list_init
 功能描述  : 初始化上级路由业务处理线程队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_srv_proc_thread_list_init()
{
    g_RouteSrvProcThreadList = (route_srv_proc_tl_list_t*)osip_malloc(sizeof(route_srv_proc_tl_list_t));

    if (g_RouteSrvProcThreadList == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_list_init() exit---: g_RouteSrvProcThreadList Smalloc Error \r\n");
        return -1;
    }

    g_RouteSrvProcThreadList->pRouteSrvProcList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (g_RouteSrvProcThreadList->pRouteSrvProcList == NULL)
    {
        osip_free(g_RouteSrvProcThreadList);
        g_RouteSrvProcThreadList = NULL;
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_list_init() exit---: Route Srv Proc List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_RouteSrvProcThreadList->pRouteSrvProcList);

#ifdef MULTI_THR
    /* init smutex */
    g_RouteSrvProcThreadList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_RouteSrvProcThreadList->lock)
    {
        osip_free(g_RouteSrvProcThreadList->pRouteSrvProcList);
        g_RouteSrvProcThreadList->pRouteSrvProcList = NULL;
        osip_free(g_RouteSrvProcThreadList);
        g_RouteSrvProcThreadList = NULL;
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_list_init() exit---: Route Srv Proc List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : route_srv_proc_thread_list_free
 功能描述  : 释放上级路由业务处理线程队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void route_srv_proc_thread_list_free()
{
    if (NULL == g_RouteSrvProcThreadList)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_RouteSrvProcThreadList->pRouteSrvProcList)
    {
        osip_list_special_free(g_RouteSrvProcThreadList->pRouteSrvProcList, (void (*)(void*))&route_srv_proc_thread_free);
        osip_free(g_RouteSrvProcThreadList->pRouteSrvProcList);
        g_RouteSrvProcThreadList->pRouteSrvProcList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_RouteSrvProcThreadList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_RouteSrvProcThreadList->lock);
        g_RouteSrvProcThreadList->lock = NULL;
    }

#endif

    osip_free(g_RouteSrvProcThreadList);
    g_RouteSrvProcThreadList = NULL;

}

/*****************************************************************************
 函 数 名  : route_srv_proc_thread_list_lock
 功能描述  : 上级路由业务处理线程队列加锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月20日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_srv_proc_thread_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RouteSrvProcThreadList == NULL || g_RouteSrvProcThreadList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_RouteSrvProcThreadList->lock);
#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : route_srv_proc_thread_list_unlock
 功能描述  : 上级路由业务处理线程队列解锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月20日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_srv_proc_thread_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RouteSrvProcThreadList == NULL || g_RouteSrvProcThreadList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_proc_thread_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_RouteSrvProcThreadList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_route_srv_proc_thread_list_lock
 功能描述  : 上级路由业务处理线程队列加锁
 输入参数  : const char* file
             int line
             const char* func
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月12日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int debug_route_srv_proc_thread_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RouteSrvProcThreadList == NULL || g_RouteSrvProcThreadList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "debug_route_srv_proc_thread_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_RouteSrvProcThreadList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_route_srv_proc_thread_list_unlock
 功能描述  : 上级路由业务处理线程队列解锁
 输入参数  : const char* file
             int line
             const char* func
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月12日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int debug_route_srv_proc_thread_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RouteSrvProcThreadList == NULL || g_RouteSrvProcThreadList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "debug_route_srv_proc_thread_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_RouteSrvProcThreadList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : appoint_route_srv_msg_list_clean
 功能描述  : 上级路由业务消息队列清除
 输入参数  : route_srv_proc_tl_t* run
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年3月8日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void appoint_route_srv_msg_list_clean(route_srv_proc_tl_t* run)
{
    int iRet = 0;
    route_srv_msg_t* pRouteSrvMsg = NULL;

    if (NULL == run)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "appoint_route_srv_msg_list_clean() Param Error \r\n");
        return;
    }

    if (NULL == run->pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "appoint_route_srv_msg_list_clean() Route Info Error \r\n");
        return;
    }

    if (NULL == run->pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "appoint_route_srv_msg_list_clean() Route Srv dboper Error \r\n");
        return;
    }

    if (NULL == run->pRouteSrvMsgQueue)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "appoint_route_srv_msg_list_clean() Route Srv Message Queue Error \r\n");
        return;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)run->pRouteSrvMsgQueueLock);
#endif

    while (!run->pRouteSrvMsgQueue->empty())
    {
        pRouteSrvMsg = (route_srv_msg_t*) run->pRouteSrvMsgQueue->front();
        run->pRouteSrvMsgQueue->pop_front();

        if (NULL != pRouteSrvMsg)
        {
            route_srv_msg_free(pRouteSrvMsg);
            pRouteSrvMsg = NULL;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)run->pRouteSrvMsgQueueLock);
#endif

    return;
}

/*****************************************************************************
 函 数 名  : scan_route_srv_proc_thread_list
 功能描述  : 扫描上级路由业务处理线程队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月12日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void scan_route_srv_proc_thread_list()
{
    int i = 0;
    //int iRet = 0;
    route_srv_proc_tl_t* pThreadProc = NULL;
    needtoproc_routesrvproc_queue needToProc;
    time_t now = time(NULL);

    if ((NULL == g_RouteSrvProcThreadList) || (NULL == g_RouteSrvProcThreadList->pRouteSrvProcList))
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "scan_route_srv_proc_thread_list() exit---: Param Error \r\n");
        return;
    }

    needToProc.clear();

    ROUTE_SRV_PROC_THREAD_SMUTEX_LOCK();

    if (osip_list_size(g_RouteSrvProcThreadList->pRouteSrvProcList) <= 0)
    {
        ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_RouteSrvProcThreadList->pRouteSrvProcList); i++)
    {
        pThreadProc = (route_srv_proc_tl_t*)osip_list_get(g_RouteSrvProcThreadList->pRouteSrvProcList, i);

        if (NULL == pThreadProc)
        {
            continue;
        }

        if (0 == pThreadProc->iUsed)
        {
            continue;
        }

        if (1 == pThreadProc->th_exit)
        {
            continue;
        }

        if (NULL == pThreadProc->pRouteInfo)
        {
            continue;
        }

        if (pThreadProc->run_time < now && now - pThreadProc->run_time > 3600)
        {
            needToProc.push_back(pThreadProc);
        }
    }

    ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    /* 处理需要开始的 */
    while (!needToProc.empty())
    {
        pThreadProc = (route_srv_proc_tl_t*) needToProc.front();
        needToProc.pop_front();

        if (NULL != pThreadProc)
        {
            if (NULL != pThreadProc->pRouteInfo)
            {
                //iRet = route_srv_proc_thread_restart(pThreadProc->pRouteInfo);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级路由业务处理线程监控线程检测到上级路由业务处理线程挂死, 上级路由ID=%s, 上级路由IP=%s, 上级路由端口=%d", pThreadProc->pRouteInfo->server_id, pThreadProc->pRouteInfo->server_ip, pThreadProc->pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Route service processing thread monitor thread hanging dead restart route services processing threads,Route ID=%s, Route IP=%s, Route port=%d", pThreadProc->pRouteInfo->server_id, pThreadProc->pRouteInfo->server_ip, pThreadProc->pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_FATAL, "scan_route_srv_proc_thread_list(): route_srv_proc_thread restart:Thread route_id=%s, route_ip=%s, route_port=%d, \r\n", pThreadProc->pRouteInfo->server_id, pThreadProc->pRouteInfo->server_ip, pThreadProc->pRouteInfo->server_port);
                osip_usleep(5000000);
                system((char*)"killall cms; killall -9 cms");
            }
        }
    }

    needToProc.clear();

    return;
}

/*****************************************************************************
 函 数 名  : show_route_srv_proc_thread
 功能描述  : 显示上级路由线程的使用情况
 输入参数  : int sock
             int type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月6日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void show_route_srv_proc_thread(int sock, int type)
{
    int i = 0;
    int pos = 0;
    char strLine[] = "\r-----------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rThread Index  Thread TID  Used Flag  Route ID             Route IP        Route Port SizeOfMsgQueue Run Time           \r\n";
    char rbuf[256] = {0};
    route_srv_proc_tl_t* runthread = NULL;
    char strRunTime[64] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    ROUTE_SRV_PROC_THREAD_SMUTEX_LOCK();

    if (NULL == g_RouteSrvProcThreadList || osip_list_size(g_RouteSrvProcThreadList->pRouteSrvProcList) <= 0)
    {
        ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return;
    }

    for (pos = 0; pos < osip_list_size(g_RouteSrvProcThreadList->pRouteSrvProcList); pos++)
    {
        runthread = (route_srv_proc_tl_t*)osip_list_get(g_RouteSrvProcThreadList->pRouteSrvProcList, pos);

        if (runthread == NULL)
        {
            continue;
        }

        if (0 == type) /* 显示未使用的 */
        {
            if (1 == runthread->iUsed || NULL != runthread->pRouteInfo)
            {
                continue;
            }
        }
        else if (1 == type) /* 显示已经使用的 */
        {
            if (0 == runthread->iUsed || NULL == runthread->pRouteInfo)
            {
                continue;
            }
        }
        else if (2 == type) /* 显示全部 */
        {

        }

        i = format_time(runthread->run_time, strRunTime);

        if (NULL != runthread->pRouteInfo)
        {
            if (NULL != runthread->pRouteSrvMsgQueue)
            {
                snprintf(rbuf, 256, "\r%-12u  %-10u  %-9u  %-20s %-15s %-9d  %-14d %-19s\r\n", pos, *(runthread->thread), runthread->iUsed, runthread->pRouteInfo->server_id, runthread->pRouteInfo->server_ip, runthread->pRouteInfo->server_port, (int)runthread->pRouteSrvMsgQueue->size(), strRunTime);
            }
            else
            {
                snprintf(rbuf, 256, "\r%-12u  %-10u  %-9u  %-20s %-15s %-9d  %-14d %-19s\r\n", pos, *(runthread->thread), runthread->iUsed, runthread->pRouteInfo->server_id, runthread->pRouteInfo->server_ip, runthread->pRouteInfo->server_port, 0, strRunTime);
            }
        }
        else
        {
            if (NULL != runthread->pRouteSrvMsgQueue)
            {
                snprintf(rbuf, 256, "\r%-12u  %-10u  %-9u  %-20s %-15s %-9d  %-14d %-19s\r\n", pos, *(runthread->thread), runthread->iUsed, (char*)"NULL", (char*)"NULL", 0, (int)runthread->pRouteSrvMsgQueue->size(), strRunTime);
            }
            else
            {
                snprintf(rbuf, 256, "\r%-12u  %-10u  %-9u  %-20s %-15s %-9d  %-14d %-19s\r\n", pos, *(runthread->thread), runthread->iUsed, (char*)"NULL", (char*)"NULL", 0, 0, strRunTime);
            }
        }

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }

    }

    ROUTE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

/*****************************************************************************
 函 数 名  : scan_appoint_route_srv_msg_list
 功能描述  : 扫描指定的多级互联消息队列
 输入参数  : route_srv_proc_tl_t* run
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月30日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int scan_appoint_route_srv_msg_list(route_srv_proc_tl_t* run)
{
    int iRet = 0;
    route_srv_msg_t* pRouteSrvMsg = NULL;
    static int connect_interval = 0;

    if (NULL == run)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "scan_appoint_route_srv_msg_list() Param Error \r\n");
        return -1;
    }

    if (NULL == run->pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "scan_appoint_route_srv_msg_list() Route Info Error \r\n");
        return -1;
    }

    if (NULL == run->pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "scan_appoint_route_srv_msg_list() Route Srv dboper Error \r\n");
        return -1;
    }

    if (NULL == run->pRouteSrvMsgQueue)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "scan_appoint_route_srv_msg_list() Route Srv Message Queue Error \r\n");
        return -1;
    }

    if (!run->iRouteLogDBOperConnectStatus)
    {
        connect_interval++;

        if (connect_interval >= 60 * 200)
        {
            if (run->pRouteLogDbOper->Connect(g_StrConLog, (char*)"") < 0)
            {
                run->iRouteLogDBOperConnectStatus = 0;
            }
            else
            {
                run->iRouteLogDBOperConnectStatus = 1;
            }

            connect_interval = 0;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)run->pRouteSrvMsgQueueLock);
#endif

    while (!run->pRouteSrvMsgQueue->empty())
    {
        pRouteSrvMsg = (route_srv_msg_t*) run->pRouteSrvMsgQueue->front();
        run->pRouteSrvMsgQueue->pop_front();

        if (NULL != pRouteSrvMsg)
        {
            break;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)run->pRouteSrvMsgQueueLock);
#endif

    if (NULL != pRouteSrvMsg)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "scan_appoint_route_srv_msg_list() \
        \r\n In Param: \
        \r\n msg_type=%d \
        \r\n caller_id=%s \
        \r\n callee_id=%s \
        \r\n response_code=%d \
        \r\n ua_dialog_index=%d \
        \r\n msg_body_len=%d \
        \r\n ", pRouteSrvMsg->msg_type, pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->response_code, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->msg_body_len);

        iRet = route_srv_msg_proc(pRouteSrvMsg, run->pRoute_Srv_dboper);

        route_srv_msg_free(pRouteSrvMsg);
        pRouteSrvMsg = NULL;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : route_srv_msg_add_for_appoint
 功能描述  : 添加互联路由业务消息到指定的线程队列中
 输入参数  : route_srv_proc_tl_t* pRouteSrvProcThd
             msg_type_t msg_type
             char* caller_id
             char* callee_id
             int response_code
             char* reasonphrase
             int ua_dialog_index
             char* msg_body
             int msg_body_len
             int cr_pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月30日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_srv_msg_add_for_appoint(route_srv_proc_tl_t* pRouteSrvProcThd, msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, char* reasonphrase, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos)
{
    route_srv_msg_t* pRouteSrvMsg = NULL;
    int iRet = 0;

    if (caller_id == NULL || pRouteSrvProcThd == NULL || callee_id == NULL)
    {
        return -1;
    }

    if (NULL == pRouteSrvProcThd->pRouteInfo)
    {
        return -1;
    }

    if (NULL == pRouteSrvProcThd->pRouteSrvMsgQueue)
    {
        return -1;
    }

    iRet = route_srv_msg_init(&pRouteSrvMsg);

    if (iRet != 0)
    {
        return -1;
    }

    pRouteSrvMsg->msg_type = msg_type;
    pRouteSrvMsg->pRouteInfo = pRouteSrvProcThd->pRouteInfo;

    if (NULL != caller_id)
    {
        osip_strncpy(pRouteSrvMsg->caller_id, caller_id, MAX_ID_LEN);
    }

    if (NULL != callee_id)
    {
        osip_strncpy(pRouteSrvMsg->callee_id, callee_id, MAX_ID_LEN);
    }

    pRouteSrvMsg->response_code = response_code;

    if (NULL != reasonphrase)
    {
        osip_strncpy(pRouteSrvMsg->reasonphrase, reasonphrase, MAX_128CHAR_STRING_LEN);
    }

    pRouteSrvMsg->ua_dialog_index = ua_dialog_index;

    if (NULL != msg_body)
    {
        osip_strncpy(pRouteSrvMsg->msg_body, msg_body, MAX_MSG_BODY_STRING_LEN);
    }

    pRouteSrvMsg->msg_body_len = msg_body_len;
    pRouteSrvMsg->cr_pos = cr_pos;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)pRouteSrvProcThd->pRouteSrvMsgQueueLock);
#endif

    pRouteSrvProcThd->pRouteSrvMsgQueue->push_back(pRouteSrvMsg);

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)pRouteSrvProcThd->pRouteSrvMsgQueueLock);
#endif

    return 0;
}
#endif
