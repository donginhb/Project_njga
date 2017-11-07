
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
#include <errno.h>
#endif

#include "common/gbldef.inc"
#include "common/gblfunc_proc.inc"
#include "common/log_proc.inc"

#include "user/user_thread_proc.inc"
#include "user/user_reg_proc.inc"
#include "user/user_srv_proc.inc"

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
#define MAX_USER_SRV_THREADS 50
#define MAX_USER_TCP_CLIENTS 100

typedef struct _user_tcp_client_t
{
    int sock;
    char login_ip[MAX_IP_LEN];
    int  login_port;
    int  expires_time;
    char strRcvBuff[1024 * 1024];
    int iRcvBuffLen;
} user_tcp_client_t;

user_tcp_client_t UserTCPClients[MAX_USER_TCP_CLIENTS];

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
user_srv_proc_tl_list_t* g_UserSrvProcThreadList = NULL;            /* 用户业务处理线程队列 */
int UserClientServSock = 0;

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#if DECS("用户业务处理线程")
/*****************************************************************************
 函 数 名  : user_srv_proc_thread_execute
 功能描述  :  用户业务处理线程
 输入参数  : user_srv_proc_tl_t * user_srv_proc
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void* user_srv_proc_thread_execute(void* p)
{
    int iRet = 0;
    user_srv_proc_tl_t* run = (user_srv_proc_tl_t*)p;
    static int clean_flag = 0;

    if (run == NULL)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_execute() exit---: Param Error \r\n");
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
                user_srv_msg_list_clean(run);
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

        if (NULL == run->pUserInfo)
        {
            run->run_time = time(NULL);
            osip_usleep(1000000);
            continue;
        }

        if (NULL == run->pUser_Srv_dboper)
        {
            DEBUG_TRACE(MODULE_USER, LOG_WARN, "user_srv_proc_thread_execute() User Srv DB Oper NULL: user_id=%s,login_ip=%s,login_port=%d \r\n", run->pUserInfo->user_id, run->pUserInfo->login_ip, run->pUserInfo->login_port);
            run->run_time = time(NULL);
            osip_usleep(1000000);
            continue;
#if 0
            DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_execute() User Srv DB Oper New:user_id=%s,login_ip=%s,login_port=%d \r\n", run->user_id, run->login_ip, run->login_port);

            run->pUser_Srv_dboper = new DBOper();

            if (run->pUser_Srv_dboper == NULL)
            {
                osip_usleep(1000000);
                continue;
            }

            DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_execute() User Srv DB Oper Connect Start:g_StrCon=%s\r\n", g_StrCon.c_str());

            if (run->pUser_Srv_dboper->Connect(g_StrCon.c_str(), (char*)"") < 0)
            {
                delete run->pUser_Srv_dboper;
                run->pUser_Srv_dboper = NULL;
                osip_usleep(1000000);
                continue;
            }

            DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_execute() User Srv DB Oper Connect End \r\n");
#endif
        }

        if (NULL == run->pUserSrvMsgQueue)
        {
            DEBUG_TRACE(MODULE_USER, LOG_WARN, "user_srv_proc_thread_execute() User Srv Msg Queue NULL: user_id=%s,login_ip=%s,login_port=%d \r\n", run->pUserInfo->user_id, run->pUserInfo->login_ip, run->pUserInfo->login_port);
            run->run_time = time(NULL);
            osip_usleep(1000000);
            continue;
#if 0
            run->pUserSrvMsgQueue = new user_srv_msg_queue;

            if (NULL == run->pUserSrvMsgQueue)
            {
                osip_usleep(1000000);
                continue;
            }

            run->pUserSrvMsgQueue->clear();
#endif
        }

        iRet = scan_user_srv_msg_list(run);

        run->run_time = time(NULL);
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_execute() update thread run time:run_time=%d \r\n", run->run_time);

        osip_usleep(5000);
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : user_srv_proc_thread_init
 功能描述  : 用户业务处理线程初始化
 输入参数  : user_srv_proc_tl_t** run
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int user_srv_proc_thread_init(user_srv_proc_tl_t** run)
{
    *run = (user_srv_proc_tl_t*) osip_malloc(sizeof(user_srv_proc_tl_t));

    if (*run == NULL)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_init() exit---: *run Smalloc Error \r\n");
        return -1;
    }

    (*run)->iUsed = 0;
    (*run)->pUserInfo = NULL;
    (*run)->thread = NULL;
    (*run)->th_exit = 0;
    (*run)->run_time = 0;
    (*run)->pUser_Srv_dboper = NULL;
    (*run)->pUserLogDbOper = NULL;
    (*run)->iUserLogDBOperConnectStatus = 0;
    (*run)->pUserSrvMsgQueue = NULL;

#ifdef MULTI_THR
    /* init smutex */
    (*run)->pUserSrvMsgQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == (*run)->pUserSrvMsgQueueLock)
    {
        osip_free(*run);
        *run = NULL;
        //DEBUG_TRACE(MODULE_USER, LOG_ERROR,  "user_srv_proc_thread_init() exit---: User Service Message List Lock Init Error \r\n");
        return -1;
    }

#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : user_srv_proc_thread_free
 功能描述  : 用户业务处理线程释放
 输入参数  : user_srv_proc_tl_t *run
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void user_srv_proc_thread_free(user_srv_proc_tl_t* run)
{
    user_srv_msg_t* user_srv_msg = NULL;

    if (run == NULL)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_free() exit---: Param Error \r\n");
        return;
    }

    run->iUsed = 0;
    run->pUserInfo = NULL;

    if (run->thread)
    {
        osip_free(run->thread);
        run->thread = NULL;
    }

    run->run_time = 0;

    if (run->pUser_Srv_dboper != NULL)
    {
        delete run->pUser_Srv_dboper;
        run->pUser_Srv_dboper = NULL;
    }

    if (run->pUserLogDbOper != NULL)
    {
        delete run->pUserLogDbOper;
        run->pUserLogDbOper = NULL;
    }

    run->iUserLogDBOperConnectStatus = 0;

    if (NULL != run->pUserSrvMsgQueue)
    {
        while (!run->pUserSrvMsgQueue->empty())
        {
            user_srv_msg = (user_srv_msg_t*) run->pUserSrvMsgQueue->front();
            run->pUserSrvMsgQueue->pop_front();

            if (NULL != user_srv_msg)
            {
                user_srv_msg_free(user_srv_msg);
                user_srv_msg = NULL;
            }
        }

        run->pUserSrvMsgQueue->clear();
        delete run->pUserSrvMsgQueue;
        run->pUserSrvMsgQueue = NULL;
    }

#ifdef MULTI_THR

    if (NULL != run->pUserSrvMsgQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)run->pUserSrvMsgQueueLock);
        run->pUserSrvMsgQueueLock = NULL;
    }

#endif

    osip_free(run);
    run = NULL;

    return;
}
#endif

#if DECS("用户业务处理线程队列")
/*****************************************************************************
 函 数 名  : user_srv_proc_thread_assign
 功能描述  : 用户业务处理线程分配
 输入参数  : user_info_t* pUserInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月27日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
int user_srv_proc_thread_assign(user_info_t* pUserInfo)
{
    user_srv_proc_tl_t* runthread = NULL;

    if (NULL == pUserInfo || pUserInfo->user_id[0] == '\0' || pUserInfo->login_ip[0] == '\0' || pUserInfo->login_port <= 0)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_assign() exit---: Param Error \r\n");
        return -1;
    }

    //printf("\r\n user_srv_proc_thread_assign() Enter--- \r\n");
    //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_assign() Enter--- \r\n");

    USER_SRV_PROC_THREAD_SMUTEX_LOCK();

    runthread = get_free_user_srv_proc_thread();

    if (NULL == runthread)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_srv_proc_thread_assign() exit---: Get Free Thread Error \r\n");
        USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return -1;
    }

    runthread->iUsed = 1;
    runthread->pUserInfo = pUserInfo;

    /* 初始化消息队列 */
    if (NULL == runthread->pUserSrvMsgQueue)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_assign() User Srv Msg Queue: Begin--- \r\n");
        runthread->pUserSrvMsgQueue = new user_srv_msg_queue;
        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_assign() User Srv Msg Queue: End--- \r\n");
    }

    //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_assign() User Srv Msg Queue Clear: Begin---\r\n");

    runthread->pUserSrvMsgQueue->clear();

    //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_assign() User Srv Msg Queue Clear: End--- \r\n");

    /* 初始化数据库连接 */
    if (NULL == runthread->pUser_Srv_dboper)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_assign() User Srv DB Oper New:user_id=%s, user_ip=%s, user_port=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

        runthread->pUser_Srv_dboper = new DBOper();

        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_assign() User Srv DB Oper New End \r\n");

        if (runthread->pUser_Srv_dboper == NULL)
        {
            //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_assign() exit---: User Srv DB Oper NULL \r\n");
            USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return -1;
        }

        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_assign() User Srv DB Start Connect:g_StrCon[0]=%s, g_StrCon[1]=%s \r\n", g_StrCon[0], g_StrCon[1]);

        if (runthread->pUser_Srv_dboper->Connect(g_StrCon, (char*)"") < 0)
        {
            delete runthread->pUser_Srv_dboper;
            runthread->pUser_Srv_dboper = NULL;
            USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return -1;
        }

        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_assign() User Srv DB Connect End \r\n");
    }

    /* 初始化日志数据库连接 */
    if (NULL == runthread->pUserLogDbOper)
    {
        runthread->pUserLogDbOper = new DBOper();

        if (runthread->pUserLogDbOper == NULL)
        {
            delete runthread->pUser_Srv_dboper;
            runthread->pUser_Srv_dboper = NULL;
            USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return -1;
        }

        if (runthread->pUserLogDbOper->Connect(g_StrConLog, (char*)"") < 0)
        {
            runthread->iUserLogDBOperConnectStatus = 0;
        }
        else
        {
            runthread->iUserLogDBOperConnectStatus = 1;
        }
    }

    USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_assign() Exit--- \r\n");

    //printf("\r\n user_srv_proc_thread_assign() Exit--- \r\n");
    return 0;
}

/*****************************************************************************
 函 数 名  : user_srv_proc_thread_recycle
 功能描述  : 用户业务处理线程回收
 输入参数  : char* user_id
             char* user_ip
             int user_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月27日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
int user_srv_proc_thread_recycle(char* user_id, char* user_ip, int user_port)
{
    user_srv_msg_t* user_srv_msg = NULL;
    user_srv_proc_tl_t* runthread = NULL;

    if (NULL == user_id || NULL == user_ip || user_port <= 0)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_recycle() exit---: Param Error \r\n");
        return -1;
    }

    //rintf("\r\n user_srv_proc_thread_recycle() Enter--- \r\n");
    //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_recycle() Enter--- \r\n");

    USER_SRV_PROC_THREAD_SMUTEX_LOCK();

    runthread = get_user_srv_proc_thread2(user_id, user_ip, user_port);

    if (NULL == runthread)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_recycle() exit---: Get Thread Error \r\n");
        USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return 0;
    }

    runthread->iUsed = 0;
    runthread->pUserInfo = NULL;

    if (NULL != runthread->pUser_Srv_dboper)
    {
        delete runthread->pUser_Srv_dboper;
        runthread->pUser_Srv_dboper = NULL;
        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_recycle() User Srv DB Oper Delete End--- \r\n");
    }

    if (NULL != runthread->pUserLogDbOper)
    {
        delete runthread->pUserLogDbOper;
        runthread->pUserLogDbOper = NULL;
    }

    runthread->iUserLogDBOperConnectStatus = 0;

    if (NULL != runthread->pUserSrvMsgQueue)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_recycle() UserSrvMsgQueue Free \r\n");

        while (!runthread->pUserSrvMsgQueue->empty())
        {
            user_srv_msg = (user_srv_msg_t*) runthread->pUserSrvMsgQueue->front();
            runthread->pUserSrvMsgQueue->pop_front();

            if (NULL != user_srv_msg)
            {
                user_srv_msg_free(user_srv_msg);
                user_srv_msg = NULL;
            }
        }

        runthread->pUserSrvMsgQueue->clear();
        //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_recycle() UserSrvMsgQueue Free End--- \r\n");
    }

    USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    //printf("\r\n user_srv_proc_thread_recycle() Exit--- \r\n");
    //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_srv_proc_thread_recycle() Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : user_srv_proc_thread_start_all
 功能描述  : 启动用户业务处理线程
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
int user_srv_proc_thread_start_all()
{
    int i = 0;
    int index = 0;
    user_srv_proc_tl_t* runthread = NULL;

    if (NULL == g_UserSrvProcThreadList || NULL == g_UserSrvProcThreadList->pUserSrvProcList)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_start() exit---: Param Error 2 \r\n");
        return -1;
    }

    USER_SRV_PROC_THREAD_SMUTEX_LOCK();

    for (index = 0; index < MAX_USER_SRV_THREADS; index++)
    {
        i = user_srv_proc_thread_init(&runthread);
        //DEBUG_TRACE(MODULE_USER, LOG_INFO, "user_srv_proc_thread_start() user_srv_proc_thread_init:i=%d \r\n", i);

        if (i != 0)
        {
            //DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_srv_proc_thread_start() exit---: User Srv Proc Thread Init Error \r\n");
            continue;
        }

        //添加到用户业务处理线程队列
        i = osip_list_add(g_UserSrvProcThreadList->pUserSrvProcList, runthread, -1); /* add to list tail */
        //DEBUG_TRACE(MODULE_USER, LOG_INFO, "user_srv_proc_thread_start() osip_list_add:i=%d \r\n", i);

        if (i < 0)
        {
            user_srv_proc_thread_free(runthread);
            runthread = NULL;
            //DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_srv_proc_thread_start() exit---: List Add Error \r\n");
            continue;
        }

        //启动处理线程
        runthread->thread = (osip_thread_t*)osip_thread_create(20000, user_srv_proc_thread_execute, (void*)runthread);

        if (runthread->thread == NULL)
        {
            osip_list_remove(g_UserSrvProcThreadList->pUserSrvProcList, i);
            user_srv_proc_thread_free(runthread);
            runthread = NULL;
            //DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_srv_proc_thread_start() exit---: User Srv Proc Thread Create Error \r\n");
            continue;
        }

        //printf("\r\n user_srv_proc_thread_start:runthread->thread=0x%lx \r\n", (unsigned long)runthread->thread);
    }

    USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : user_srv_proc_thread_stop_all
 功能描述  : 停止所有用户业务处理线程
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
int user_srv_proc_thread_stop_all()
{
    int pos = 0;
    int i = 0;
    user_srv_proc_tl_t* runthread = NULL;

    if (NULL == g_UserSrvProcThreadList || NULL == g_UserSrvProcThreadList->pUserSrvProcList)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_stop_all() exit---: Param Error \r\n");
        return -1;
    }

    USER_SRV_PROC_THREAD_SMUTEX_LOCK();

    //查找队列，停止线程
    for (pos = 0; pos < osip_list_size(g_UserSrvProcThreadList->pUserSrvProcList); pos++)
    {
        runthread = (user_srv_proc_tl_t*)osip_list_get(g_UserSrvProcThreadList->pUserSrvProcList, pos);

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

    USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : user_srv_proc_thread_restart
 功能描述  : 用户业务处理线程重新启动
 输入参数  : user_info_t* pUserInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月12日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int user_srv_proc_thread_restart(user_info_t* pUserInfo)
{
    int pos = 0;
    int iRet = 0;
    user_srv_proc_tl_t* runthread = NULL;

    if (NULL == pUserInfo || pUserInfo->user_id[0] == '\0' || pUserInfo->login_ip[0] == '\0' || pUserInfo->login_port <= 0)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_restart() exit---: Param Error \r\n");
        return -1;
    }

    USER_SRV_PROC_THREAD_SMUTEX_LOCK();

    //查找队列
    for (pos = 0; pos < osip_list_size(g_UserSrvProcThreadList->pUserSrvProcList); pos++)
    {
        runthread = (user_srv_proc_tl_t*)osip_list_get(g_UserSrvProcThreadList->pUserSrvProcList, pos);

        if (runthread == NULL || 0 == runthread->iUsed || NULL == runthread->pUserInfo)
        {
            continue;
        }

        if ('\0' == runthread->pUserInfo->user_id[0] || '\0' == runthread->pUserInfo->login_ip[0] || runthread->pUserInfo->login_port <= 0)
        {
            continue;
        }

        if ((0 == sstrcmp(runthread->pUserInfo->user_id, pUserInfo->user_id))
            && (0 == sstrcmp(runthread->pUserInfo->login_ip, pUserInfo->login_ip))
            && runthread->pUserInfo->login_port == pUserInfo->login_port)
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
            runthread->thread = (osip_thread_t*)osip_thread_create(20000, user_srv_proc_thread_execute, (void*)runthread);

            if (runthread->thread == NULL)
            {
                osip_list_remove(g_UserSrvProcThreadList->pUserSrvProcList, pos);
                user_srv_proc_thread_free(runthread);
                runthread = NULL;
                continue;
            }
        }
    }

    USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : user_srv_proc_thread_find
 功能描述  : 查找用户业务处理线程
 输入参数  : user_info_t* pUserInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int user_srv_proc_thread_find(user_info_t* pUserInfo)
{
    int pos = 0;
    user_srv_proc_tl_t* runthread = NULL;

    if (NULL == pUserInfo || pUserInfo->user_id[0] == '\0' || pUserInfo->login_ip[0] == '\0' || pUserInfo->login_port <= 0)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_find() exit---: Param Error \r\n");
        return -1;
    }

    USER_SRV_PROC_THREAD_SMUTEX_LOCK();

    //查找队列
    for (pos = 0; pos < osip_list_size(g_UserSrvProcThreadList->pUserSrvProcList); pos++)
    {
        runthread = (user_srv_proc_tl_t*)osip_list_get(g_UserSrvProcThreadList->pUserSrvProcList, pos);

        if (runthread == NULL || 0 == runthread->iUsed || NULL == runthread->pUserInfo)
        {
            continue;
        }

        if ('\0' == runthread->pUserInfo->user_id[0] || '\0' == runthread->pUserInfo->login_ip[0] || runthread->pUserInfo->login_port <= 0)
        {
            continue;
        }

        if ((0 == sstrcmp(runthread->pUserInfo->user_id, pUserInfo->user_id))
            && (0 == sstrcmp(runthread->pUserInfo->login_ip, pUserInfo->login_ip))
            && runthread->pUserInfo->login_port == pUserInfo->login_port)
        {
            USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return pos;
        }
    }

    USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return -1;
}

/*****************************************************************************
 函 数 名  : get_free_user_srv_proc_thread
 功能描述  : 获取空闲的用户业务处理线程
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月27日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
user_srv_proc_tl_t* get_free_user_srv_proc_thread()
{
    int pos = 0;
    user_srv_proc_tl_t* runthread = NULL;

    //查找队列，将sipudp从接收线程队列中移除
    for (pos = 0; pos < osip_list_size(g_UserSrvProcThreadList->pUserSrvProcList); pos++)
    {
        runthread = (user_srv_proc_tl_t*)osip_list_get(g_UserSrvProcThreadList->pUserSrvProcList, pos);

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
 函 数 名  : get_user_srv_proc_thread
 功能描述  : 获取用户业务处理线程
 输入参数  : char* user_id
             char* user_ip
             int user_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月27日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
user_srv_proc_tl_t* get_user_srv_proc_thread(char* user_id, char* user_ip, int user_port)
{
    int pos = 0;
    user_srv_proc_tl_t* runthread = NULL;

    if (NULL == user_id || NULL == user_ip || user_port <= 0)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "get_user_srv_proc_thread() exit---: Param Error \r\n");
        return NULL;
    }

    USER_SRV_PROC_THREAD_SMUTEX_LOCK();

    for (pos = 0; pos < osip_list_size(g_UserSrvProcThreadList->pUserSrvProcList); pos++)
    {
        runthread = (user_srv_proc_tl_t*)osip_list_get(g_UserSrvProcThreadList->pUserSrvProcList, pos);

        if (runthread == NULL || 0 == runthread->iUsed || NULL == runthread->pUserInfo)
        {
            continue;
        }

        if ('\0' == runthread->pUserInfo->user_id[0] || '\0' == runthread->pUserInfo->login_ip[0] || runthread->pUserInfo->login_port <= 0)
        {
            continue;
        }

        if ((0 == sstrcmp(runthread->pUserInfo->user_id, user_id))
            && (0 == sstrcmp(runthread->pUserInfo->login_ip, user_ip))
            && runthread->pUserInfo->login_port == user_port)
        {
            USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            //DEBUG_TRACE(MODULE_USER, LOG_TRACE, "get_user_srv_proc_thread() exit---: pos=%d, user_id=%s, user_ip=%s, user_port=%d \r\n", pos, user_id, user_ip, user_port);
            return runthread;
        }
    }

    USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : get_user_srv_proc_thread2
 功能描述  : 获取用户业务处理线程
 输入参数  : char* user_id
             char* user_ip
             int user_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月13日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
user_srv_proc_tl_t* get_user_srv_proc_thread2(char* user_id, char* user_ip, int user_port)
{
    int pos = 0;
    user_srv_proc_tl_t* runthread = NULL;

    if (NULL == user_id || NULL == user_ip || user_port <= 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "get_user_srv_proc_thread2() exit---: Param Error \r\n");
        return NULL;
    }

    for (pos = 0; pos < osip_list_size(g_UserSrvProcThreadList->pUserSrvProcList); pos++)
    {
        runthread = (user_srv_proc_tl_t*)osip_list_get(g_UserSrvProcThreadList->pUserSrvProcList, pos);

        if (runthread == NULL || 0 == runthread->iUsed || NULL == runthread->pUserInfo)
        {
            continue;
        }

        if ('\0' == runthread->pUserInfo->user_id[0] || '\0' == runthread->pUserInfo->login_ip[0] || runthread->pUserInfo->login_port <= 0)
        {
            continue;
        }

        if ((0 == sstrcmp(runthread->pUserInfo->user_id, user_id))
            && (0 == sstrcmp(runthread->pUserInfo->login_ip, user_ip))
            && runthread->pUserInfo->login_port == user_port)
        {
            return runthread;
        }
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : user_srv_proc_thread_list_init
 功能描述  : 初始化用户业务处理线程队列
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
int user_srv_proc_thread_list_init()
{
    g_UserSrvProcThreadList = (user_srv_proc_tl_list_t*)osip_malloc(sizeof(user_srv_proc_tl_list_t));

    if (g_UserSrvProcThreadList == NULL)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_list_init() exit---: g_UserSrvProcThreadList Smalloc Error \r\n");
        return -1;
    }

    g_UserSrvProcThreadList->pUserSrvProcList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (g_UserSrvProcThreadList->pUserSrvProcList == NULL)
    {
        osip_free(g_UserSrvProcThreadList);
        g_UserSrvProcThreadList = NULL;
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_list_init() exit---: User Srv Proc List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_UserSrvProcThreadList->pUserSrvProcList);

#ifdef MULTI_THR
    /* init smutex */
    g_UserSrvProcThreadList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_UserSrvProcThreadList->lock)
    {
        osip_free(g_UserSrvProcThreadList->pUserSrvProcList);
        g_UserSrvProcThreadList->pUserSrvProcList = NULL;
        osip_free(g_UserSrvProcThreadList);
        g_UserSrvProcThreadList = NULL;
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_list_init() exit---: User Srv Proc List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : user_srv_proc_thread_list_free
 功能描述  : 释放用户业务处理线程队列
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
void user_srv_proc_thread_list_free()
{
    if (NULL == g_UserSrvProcThreadList)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_UserSrvProcThreadList->pUserSrvProcList)
    {
        osip_list_special_free(g_UserSrvProcThreadList->pUserSrvProcList, (void (*)(void*))&user_srv_proc_thread_free);
        osip_free(g_UserSrvProcThreadList->pUserSrvProcList);
        g_UserSrvProcThreadList->pUserSrvProcList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_UserSrvProcThreadList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_UserSrvProcThreadList->lock);
        g_UserSrvProcThreadList->lock = NULL;
    }

#endif

    osip_free(g_UserSrvProcThreadList);
    g_UserSrvProcThreadList = NULL;

}

/*****************************************************************************
 函 数 名  : user_srv_proc_thread_list_lock
 功能描述  : 用户业务处理线程队列加锁
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
int user_srv_proc_thread_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UserSrvProcThreadList == NULL || g_UserSrvProcThreadList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_UserSrvProcThreadList->lock);
#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : user_srv_proc_thread_list_unlock
 功能描述  : 用户业务处理线程队列解锁
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
int user_srv_proc_thread_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UserSrvProcThreadList == NULL || g_UserSrvProcThreadList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_srv_proc_thread_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_UserSrvProcThreadList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_user_srv_proc_thread_list_lock
 功能描述  : 用户业务处理线程队列加锁
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
int debug_user_srv_proc_thread_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UserSrvProcThreadList == NULL || g_UserSrvProcThreadList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "debug_user_srv_proc_thread_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_UserSrvProcThreadList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_user_srv_proc_thread_list_unlock
 功能描述  : 用户业务处理线程队列解锁
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
int debug_user_srv_proc_thread_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_UserSrvProcThreadList == NULL || g_UserSrvProcThreadList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "debug_user_srv_proc_thread_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_UserSrvProcThreadList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : scan_user_srv_proc_thread_list
 功能描述  : 扫描用户业务处理线程队列
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
void scan_user_srv_proc_thread_list()
{
    int i = 0;
    //int iRet = 0;
    user_srv_proc_tl_t* pThreadProc = NULL;
    needtoproc_usersrvproc_queue needToProc;
    time_t now = time(NULL);

    if ((NULL == g_UserSrvProcThreadList) || (NULL == g_UserSrvProcThreadList->pUserSrvProcList))
    {
        //DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "scan_user_srv_proc_thread_list() exit---: Param Error \r\n");
        return;
    }

    needToProc.clear();

    USER_SRV_PROC_THREAD_SMUTEX_LOCK();

    if (osip_list_size(g_UserSrvProcThreadList->pUserSrvProcList) <= 0)
    {
        USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_UserSrvProcThreadList->pUserSrvProcList); i++)
    {
        pThreadProc = (user_srv_proc_tl_t*)osip_list_get(g_UserSrvProcThreadList->pUserSrvProcList, i);

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

        if (NULL == pThreadProc->pUserInfo)
        {
            continue;
        }

        if (pThreadProc->run_time < now && now - pThreadProc->run_time > 3600)
        {
            needToProc.push_back(pThreadProc);
        }
    }

    USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    /* 处理需要开始的 */
    while (!needToProc.empty())
    {
        pThreadProc = (user_srv_proc_tl_t*) needToProc.front();
        needToProc.pop_front();

        if (NULL != pThreadProc)
        {
            if (NULL != pThreadProc->pUserInfo)
            {
                //iRet = user_srv_proc_thread_restart(pThreadProc->pUserInfo);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "用户业务处理线程监控线程检测到用户业务处理线程挂死, 用户ID=%s, 用户IP=%s, 用户端口=%d", pThreadProc->pUserInfo->user_id, pThreadProc->pUserInfo->login_ip, pThreadProc->pUserInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "User service processing thread monitor thread hanging dead restart user services processing threads,User ID=%s, User IP=%s, User port=%d", pThreadProc->pUserInfo->user_id, pThreadProc->pUserInfo->login_ip, pThreadProc->pUserInfo->login_port);
                DEBUG_TRACE(MODULE_USER, LOG_FATAL, "scan_user_srv_proc_thread_list(): user_srv_proc_thread restart:Thread user_id=%s, user_ip=%s, user_port=%d, \r\n", pThreadProc->pUserInfo->user_id, pThreadProc->pUserInfo->login_ip, pThreadProc->pUserInfo->login_port);
                osip_usleep(5000000);
                system((char*)"killall cms; killall -9 cms");
            }
        }
    }

    needToProc.clear();

    return;
}

/*****************************************************************************
 函 数 名  : show_user_srv_proc_thread
 功能描述  : 显示用户线程的使用情况
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
void show_user_srv_proc_thread(int sock, int type)
{
    int i = 0;
    int pos = 0;
    char strLine[] = "\r-----------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rThread Index  Thread TID  Used Flag  User ID              User IP         User Port  SizeOfMsgQueue Run Time           \r\n";
    char rbuf[256] = {0};
    user_srv_proc_tl_t* runthread = NULL;
    char strRunTime[64] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    USER_SRV_PROC_THREAD_SMUTEX_LOCK();

    if (NULL == g_UserSrvProcThreadList || osip_list_size(g_UserSrvProcThreadList->pUserSrvProcList) <= 0)
    {
        USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return;
    }

    for (pos = 0; pos < osip_list_size(g_UserSrvProcThreadList->pUserSrvProcList); pos++)
    {
        runthread = (user_srv_proc_tl_t*)osip_list_get(g_UserSrvProcThreadList->pUserSrvProcList, pos);

        if (runthread == NULL)
        {
            continue;
        }

        if (0 == type) /* 显示未使用的 */
        {
            if (1 == runthread->iUsed || NULL != runthread->pUserInfo)
            {
                continue;
            }
        }
        else if (1 == type) /* 显示已经使用的 */
        {
            if (0 == runthread->iUsed || NULL == runthread->pUserInfo)
            {
                continue;
            }
        }
        else if (2 == type) /* 显示全部 */
        {

        }

        i = format_time(runthread->run_time, strRunTime);

        if (NULL != runthread->pUserInfo)
        {
            if (NULL != runthread->pUserSrvMsgQueue)
            {
                snprintf(rbuf, 256, "\r%-12u  %-10u  %-9u  %-20s %-15s %-9d  %-14d %-19s\r\n", pos, *(runthread->thread), runthread->iUsed, runthread->pUserInfo->user_id, runthread->pUserInfo->login_ip, runthread->pUserInfo->login_port, (int)runthread->pUserSrvMsgQueue->size(), strRunTime);
            }
            else
            {
                snprintf(rbuf, 256, "\r%-12u  %-10u  %-9u  %-20s %-15s %-9d  %-14d %-19s\r\n", pos, *(runthread->thread), runthread->iUsed, runthread->pUserInfo->user_id, runthread->pUserInfo->login_ip, runthread->pUserInfo->login_port, 0, strRunTime);
            }
        }
        else
        {
            if (NULL != runthread->pUserSrvMsgQueue)
            {
                snprintf(rbuf, 256, "\r%-12u  %-10u  %-9u  %-20s %-15s %-9d  %-14d %-19s\r\n", pos, *(runthread->thread), runthread->iUsed, (char*)"NULL", (char*)"NULL", 0, (int)runthread->pUserSrvMsgQueue->size(), strRunTime);
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

    USER_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}
#endif

#if DECS("用户登录时候获取数据的TCP连接")
/*****************************************************************************
 函 数 名  : user_tcp_client_init
 功能描述  : 用户客户端TCP结构初始化
 输入参数  : user_tcp_client_t* sc
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月29日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void user_tcp_client_init(user_tcp_client_t* sc)
{
    if (sc == NULL)
    {
        return;
    }

    sc->sock = -1;
    memset(sc->login_ip, 0, sizeof(sc->login_ip));
    sc->login_port = 0;
    sc->expires_time = 0;
    memset(sc->strRcvBuff, 0, 1024 * 1024);
    sc->iRcvBuffLen = 0;
    return;
}

/*****************************************************************************
 函 数 名  : UserLoginServerInit
 功能描述  : 用户登录TCP服务端初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月29日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int  UserLoginServerInit()
{
    int i = 0;
    int sock = -1;               /* socket to create */
    struct sockaddr_in ServAddr; /* Local address */
    int val = 1;

    for (i = 0; i < MAX_USER_TCP_CLIENTS; i++)
    {
        user_tcp_client_init(&UserTCPClients[i]);
    }

    /* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("UserLoginServerInit create socket error!!!     reason:");
        return -1;
    }

    /*closesocket（一般不会立即关闭而经历TIME_WAIT的过程）后想继续重用该socket：*/
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    /* Construct local address structure */
    memset(&ServAddr, 0, sizeof(ServAddr));   /* Zero out structure */
    ServAddr.sin_family = AF_INET;                /* Internet address family */
    ServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    ServAddr.sin_port = htons(EV9000_TCP_SERVERPORT);   /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr*) &ServAddr, sizeof(ServAddr)) < 0)
    {
        perror("UserLoginServerInit bind socket error!!!     reason:");
        close(sock);
        return -1;
    }

    /* Mark the socket so it will listen for incoming connections */
    if (listen(sock, MAX_USER_TCP_CLIENTS) < 0)
    {
        perror("UserLoginServerInit listen socket error!!!     reason:");
        close(sock);
        return -1;
    }

    UserClientServSock = sock;

    return 0;
}

/*****************************************************************************
 函 数 名  : UserLoginServerFree
 功能描述  : 用户登录TCP服务端释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月29日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void UserLoginServerFree()
{
    int i;

    if (UserClientServSock <= 0)
    {
        return;
    }

    close(UserClientServSock);
    UserClientServSock = -1;

    for (i = 0; i < MAX_USER_TCP_CLIENTS; i++)
    {
        if (UserTCPClients[i].sock != -1)
        {
            close(UserTCPClients[i].sock);
        }

        user_tcp_client_init(&UserTCPClients[i]);
    }

    return;
}

/*****************************************************************************
 函 数 名  : UserLoginServerMain
 功能描述  : 用户登录TCP服务端主程序
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月29日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void UserLoginServerMain(DBOper* pDbOper)
{
    int i = 0;
    int p = 0;
    int  maxDescriptor = 0;          /* Maximum socket descriptor value */
    fd_set sockSet;                  /* Set of socket descriptors for select() */
    struct timeval val;              /* Timeout for select() */
    char buff[1024 * 1024] = {0};
    char from_ip[16] = {0};
    int  clientSock = 0;      /* Socket descriptor for client */
    int  recvSize = 0;
    int  iRet = 0;
    int iSendBuf = 1024 * 1024;
    struct timeval iTimeout;

    if (UserClientServSock <= 0)
    {
        UserLoginServerInit();

        if (UserClientServSock <= 0)
        {
            return;
        }
    }

    FD_ZERO(&sockSet);
    FD_SET(UserClientServSock, &sockSet);
    maxDescriptor = UserClientServSock;

    for (i = 0; i < MAX_USER_TCP_CLIENTS; i++)
    {
        if (UserTCPClients[i].sock != -1)
        {
            FD_SET(UserTCPClients[i].sock, &sockSet);

            if (UserTCPClients[i].sock > maxDescriptor)
            {
                maxDescriptor = UserTCPClients[i].sock;
            }
        }
    }

    val.tv_sec = 0;      /* timeout (secs.) */
    val.tv_usec = 10;    /* 10 microseconds */

    i = select(maxDescriptor + 1, &sockSet, NULL, NULL, &val);

    if (i == 0)
    {
        return;
    }

    if (i == -1)
    {
        return;
    }

    if (FD_ISSET(UserClientServSock, &sockSet))
    {
        struct sockaddr_in ClntAddr;     /* Client address */
        unsigned int clntLen = 0;        /* Length of client address data structure */
        clntLen = sizeof(ClntAddr);

        /* Wait for a client to connect */
        if ((clientSock = accept(UserClientServSock, (struct sockaddr*) &ClntAddr,
                                 &clntLen)) < 0)
        {
            return;
        }

        for (p = 0; p < MAX_USER_TCP_CLIENTS; p++)
        {
            if (UserTCPClients[p].sock == -1)
            {
                break;
            }
        }

        if (p >= MAX_USER_TCP_CLIENTS)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "用户TCP连接数达到上限, 关闭客户端的TCP连接:TCP Socket=%d", clientSock);
            close(clientSock);

            /* 释放掉没用的TCP链接 */
            free_unused_user_tcp_connect();
            return;
        }
        else
        {
            if (NULL == inet_ntop(AF_INET, (void*) & (ClntAddr.sin_addr), from_ip, sizeof(from_ip)))
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "获取客户端TCP连接的IP地址失败, 关闭客户端的TCP连接:TCP Socket=%d", clientSock);
                close(clientSock);
                return;
            }

            /* 设置发送超时时间 */
            iTimeout.tv_sec = 1; /* 1秒超时 */
            iTimeout.tv_usec = 0;
            setsockopt(clientSock, SOL_SOCKET, SO_SNDTIMEO, (char *)&iTimeout, sizeof(struct timeval));

            /* 设置发送缓冲区 */
            setsockopt(clientSock, SOL_SOCKET, SO_SNDBUF, (char *)&iSendBuf, sizeof(int));

            UserTCPClients[p].sock = clientSock;
            UserTCPClients[p].login_port = ntohs(ClntAddr.sin_port);
            osip_strncpy(UserTCPClients[p].login_ip, from_ip, MAX_IP_LEN);
            DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserLoginServerMain() User: IP=%s, Port=%d, Socket=%d Connect \r\n", UserTCPClients[p].login_ip, UserTCPClients[p].login_port, clientSock);
        }
    }

    for (p = 0; p < MAX_USER_TCP_CLIENTS; p++)
    {
        if (UserTCPClients[p].sock == -1)
        {
            continue;
        }

        if (FD_ISSET(UserTCPClients[p].sock, &sockSet))
        {
            clientSock = UserTCPClients[p].sock;

            memset(buff, 0, 1024 * 1024);
            recvSize = recv(clientSock, buff, sizeof(buff), 0);

            if (recvSize == 0)
            {
                DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserLoginServerMain() User: IP=%s, Port=%d, Socket=%d, recvSize=%d Closed1 \r\n", UserTCPClients[p].login_ip, UserTCPClients[p].login_port, clientSock, recvSize);
                close(clientSock);

                if (UserTCPClients[p].iRcvBuffLen > 0 && UserTCPClients[p].strRcvBuff[0] != '\0')
                {
                    /* 解析处理 */
                    iRet = UserTCPLoginParseClientData(UserTCPClients[p].login_ip, UserTCPClients[p].login_port, UserTCPClients[p].sock, UserTCPClients[p].strRcvBuff, UserTCPClients[p].iRcvBuffLen, pDbOper);
                }

                /* 关闭 */
                user_tcp_client_init(&UserTCPClients[p]);
                free_user_reg_info_by_tcp_socket(clientSock);
                continue;
            }
            else if (recvSize < 0)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserLoginServerMain() User: IP=%s, Port=%d, Socket=%d, recvSize=%d Closed2 \r\n", UserTCPClients[p].login_ip, UserTCPClients[p].login_port, clientSock, recvSize);
                    close(clientSock);

                    if (UserTCPClients[p].iRcvBuffLen > 0 && UserTCPClients[p].strRcvBuff[0] != '\0')
                    {
                        /* 解析处理 */
                        iRet = UserTCPLoginParseClientData(UserTCPClients[p].login_ip, UserTCPClients[p].login_port, UserTCPClients[p].sock, UserTCPClients[p].strRcvBuff, UserTCPClients[p].iRcvBuffLen, pDbOper);
                    }

                    /* 关闭 */
                    user_tcp_client_init(&UserTCPClients[p]);
                    free_user_reg_info_by_tcp_socket(clientSock);
                    continue;
                }
                else if (errno == EAGAIN) /* 接收超时 */
                {
                    continue;
                }
            }

            DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserLoginServerMain() User: IP=%s, Port=%d, Socket=%d, recvSize=%d, buff=%s \r\n", UserTCPClients[p].login_ip, UserTCPClients[p].login_port, clientSock, recvSize, buff);

            if (strlen(UserTCPClients[p].strRcvBuff) + recvSize < 1024 * 1024)
            {
                /* 追加到字符串 */
                strncat(UserTCPClients[p].strRcvBuff, buff, recvSize);
                UserTCPClients[p].iRcvBuffLen += recvSize;
                DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserLoginServerMain() User: IP=%s, Port=%d, Socket=%d, recvSize=%d, RcvBuffLen=%d \r\n", UserTCPClients[p].login_ip, UserTCPClients[p].login_port, clientSock, recvSize, UserTCPClients[p].iRcvBuffLen);
            }

            /* 解析处理 */
            iRet = UserTCPLoginParseClientData(UserTCPClients[p].login_ip, UserTCPClients[p].login_port, UserTCPClients[p].sock, UserTCPClients[p].strRcvBuff, UserTCPClients[p].iRcvBuffLen, pDbOper);

            if (-99 != iRet) /* 不是XML解析失败的，清理掉RcvBuff */
            {
                memset(UserTCPClients[p].strRcvBuff, 0, 1024 * 1024);
                UserTCPClients[p].iRcvBuffLen = 0;
            }
        }
    }

    return;
}

/*****************************************************************************
 函 数 名  : ShowConnectTCPUser
 功能描述  : 显示客户端连接的TCP用户
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月29日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void ShowConnectTCPUser(int sock)
{
    int p = 0;
    char strLine[] = "\r-----------------------------------------------------\r\n";
    char strHead[] = "\rUser Client Socket  User Client IP   User Client Port\r\n";

    char rbuf[256] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    for (p = 0; p < MAX_USER_TCP_CLIENTS; p++)
    {
        if (UserTCPClients[p].sock == -1)
        {
            continue;
        }

        snprintf(rbuf, 256, "\r%-18d  %-15s  %-16d\r\n", UserTCPClients[p].sock, UserTCPClients[p].login_ip, UserTCPClients[p].login_port);

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
 函 数 名  : free_unused_user_tcp_connect
 功能描述  : 释放掉没有使用的用户TCP连接
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年1月5日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void free_unused_user_tcp_connect()
{
    int p = 0;

    for (p = 0; p < MAX_USER_TCP_CLIENTS; p++)
    {
        if (UserTCPClients[p].sock == -1)
        {
            continue;
        }

        if (!is_user_tcp_socket_in_use(UserTCPClients[p].sock))
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "用户TCP连接没有使用, 主动关闭:TCP Socket=%d", UserTCPClients[p].sock);
            close(UserTCPClients[p].sock);
            user_tcp_client_init(&UserTCPClients[p]);
        }
    }

    return;
}


#endif

