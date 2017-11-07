
/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <stdio.h>
#ifdef WIN32
#include <winsock.h>
#include <sys/types.h>
#else
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#endif

#include "common/gbldef.inc"
#include "common/gblfunc_proc.inc"
#include "common/log_proc.inc"

#include "device/device_thread_proc.inc"
#include "device/device_reg_proc.inc"

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
#define MAX_DEVICE_SRV_THREADS 50
#define MAX_ZRV_DEVICE_TCP_CLIENTS 50

typedef struct _zrv_device_tcp_client_t
{
    int sock;
    char login_ip[MAX_IP_LEN];
    int  login_port;
    int  expires_time;
    char strRcvBuff[1024 * 1024];
    int iRcvBuffLen;
    int iRcvBuffLenCount;
} zrv_device_tcp_client_t;

zrv_device_tcp_client_t ZRVDeviceTCPClients[MAX_ZRV_DEVICE_TCP_CLIENTS];

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
device_srv_proc_tl_list_t* g_DeviceSrvProcThreadList = NULL;            /* 前端设备业务处理线程队列 */
int ZRVDeviceClientServSock = 0;

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#if DECS("前端设备业务处理线程")
/*****************************************************************************
 函 数 名  : device_srv_proc_thread_for_appoint_execute
 功能描述  : 前端设备业务处理线程
 输入参数  : appoint_device_srv_proc_tl_t * device_srv_proc
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void* device_srv_proc_thread_for_appoint_execute(void* p)
{
    int iRet = 0;
    device_srv_proc_tl_t* run = (device_srv_proc_tl_t*)p;
    static int clean_flag = 0;

    if (run == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_for_appoint_execute() exit---: Param Error \r\n");
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
                appoint_device_srv_msg_list_clean(run);
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

        if (NULL == run->pDeviceInfo)
        {
            run->run_time = time(NULL);
            osip_usleep(1000000);
            continue;
        }

        if (NULL == run->pDevice_Srv_dboper)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "device_srv_proc_thread_for_appoint_execute() Device Srv DB Oper NULL: device_id=%s,login_ip=%s,login_port=%d \r\n", run->pDeviceInfo->device_id, run->pDeviceInfo->login_ip, run->pDeviceInfo->login_port);
            run->run_time = time(NULL);
            osip_usleep(1000000);
            continue;
#if 0
            DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_for_appoint_execute() Device Srv DB Oper New:device_id=%s,login_ip=%s,login_port=%d \r\n", run->device_id, run->login_ip, run->login_port);

            run->pDevice_Srv_dboper = new DBOper();

            if (run->pDevice_Srv_dboper == NULL)
            {
                osip_usleep(1000000);
                continue;
            }

            DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_for_appoint_execute() Device Srv DB Oper Connect Start:g_StrCon=%s\r\n", g_StrCon.c_str());

            if (run->pDevice_Srv_dboper->Connect(g_StrCon.c_str(), (char*)"") < 0)
            {
                delete run->pDevice_Srv_dboper;
                run->pDevice_Srv_dboper = NULL;
                osip_usleep(1000000);
                continue;
            }

            DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_for_appoint_execute() Device Srv DB Oper Connect End \r\n");
#endif
        }

        if (NULL == run->pDeviceSrvMsgQueue)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "device_srv_proc_thread_for_appoint_execute() Device Srv Msg Queue NULL: device_id=%s,login_ip=%s,login_port=%d \r\n", run->pDeviceInfo->device_id, run->pDeviceInfo->login_ip, run->pDeviceInfo->login_port);
            run->run_time = time(NULL);
            osip_usleep(1000000);
            continue;
#if 0
            run->pDeviceSrvMsgQueue = new device_srv_msg_queue;

            if (NULL == run->pDeviceSrvMsgQueue)
            {
                osip_usleep(5000);
                continue;
            }

            run->pDeviceSrvMsgQueue->clear();
#endif
        }

        iRet = scan_appoint_device_srv_msg_list(run);

        run->run_time = time(NULL);
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_for_appoint_execute() update thread run time:run_time=%d \r\n", run->run_time);

        osip_usleep(5000);
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : device_srv_proc_thread_init
 功能描述  : 前端设备业务处理线程初始化
 输入参数  : device_srv_proc_tl_t** run
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int device_srv_proc_thread_init(device_srv_proc_tl_t** run)
{
    *run = (device_srv_proc_tl_t*) osip_malloc(sizeof(device_srv_proc_tl_t));

    if (*run == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_init() exit---: *run Smalloc Error \r\n");
        return -1;
    }

    (*run)->iUsed = 0;
    (*run)->pDeviceInfo = NULL;
    (*run)->thread = NULL;
    (*run)->th_exit = 0;
    (*run)->run_time = 0;
    (*run)->pDevice_Srv_dboper = NULL;
    (*run)->pDeviceLogDbOper = NULL;
    (*run)->iDeviceLogDBOperConnectStatus = 0;
    (*run)->pDeviceSrvMsgQueue = NULL;

#ifdef MULTI_THR
    /* init smutex */
    (*run)->pDeviceSrvMsgQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == (*run)->pDeviceSrvMsgQueueLock)
    {
        osip_free(*run);
        *run = NULL;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR,  "device_srv_proc_thread_init() exit---: Device Service Message List Lock Init Error \r\n");
        return -1;
    }

#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : device_srv_proc_thread_free
 功能描述  : 前端设备业务处理线程释放
 输入参数  : device_srv_proc_tl_t *run
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void device_srv_proc_thread_free(device_srv_proc_tl_t* run)
{
    device_srv_msg_t* device_srv_msg = NULL;

    if (run == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_free() exit---: Param Error \r\n");
        return;
    }

    run->iUsed = 0;
    run->pDeviceInfo = NULL;

    if (run->thread)
    {
        osip_free(run->thread);
        run->thread = NULL;
    }

    run->run_time = 0;

    if (run->pDevice_Srv_dboper != NULL)
    {
        delete run->pDevice_Srv_dboper;
        run->pDevice_Srv_dboper = NULL;
    }

    if (run->pDeviceLogDbOper != NULL)
    {
        delete run->pDeviceLogDbOper;
        run->pDeviceLogDbOper = NULL;
    }

    run->iDeviceLogDBOperConnectStatus = 0;

    if (NULL != run->pDeviceSrvMsgQueue)
    {
        while (!run->pDeviceSrvMsgQueue->empty())
        {
            device_srv_msg = (device_srv_msg_t*) run->pDeviceSrvMsgQueue->front();
            run->pDeviceSrvMsgQueue->pop_front();

            if (NULL != device_srv_msg)
            {
                device_srv_msg_free(device_srv_msg);
                device_srv_msg = NULL;
            }
        }

        run->pDeviceSrvMsgQueue->clear();
        delete run->pDeviceSrvMsgQueue;
        run->pDeviceSrvMsgQueue = NULL;
    }

#ifdef MULTI_THR

    if (NULL != run->pDeviceSrvMsgQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)run->pDeviceSrvMsgQueueLock);
        run->pDeviceSrvMsgQueueLock = NULL;
    }

#endif

    osip_free(run);
    run = NULL;

    return;
}
#endif

#if DECS("前端设备业务处理线程队列")
/*****************************************************************************
 函 数 名  : device_srv_proc_thread_assign
 功能描述  : 前端设备业务处理线程分配
 输入参数  : GBDevice_info_t* pDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月27日
    作    者   : 前端设备路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
int device_srv_proc_thread_assign(GBDevice_info_t* pDeviceInfo)
{
    device_srv_proc_tl_t* runthread = NULL;

    if (NULL == pDeviceInfo || pDeviceInfo->device_id[0] == '\0' || pDeviceInfo->login_ip[0] == '\0' || pDeviceInfo->login_port <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_assign() exit---: Param Error \r\n");
        return -1;
    }

    //printf("\r\n device_srv_proc_thread_assign() Enter--- \r\n");
    //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_assign() Enter--- \r\n");

    DEVICE_SRV_PROC_THREAD_SMUTEX_LOCK();

    runthread = get_free_device_srv_proc_thread();

    if (NULL == runthread)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "device_srv_proc_thread_assign() exit---: Get Free Thread Error \r\n");
        DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return -1;
    }

    runthread->iUsed = 1;
    runthread->pDeviceInfo = pDeviceInfo;

    /* 初始化消息队列 */
    if (NULL == runthread->pDeviceSrvMsgQueue)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_assign() Device Srv Msg Queue: Begin--- \r\n");
        runthread->pDeviceSrvMsgQueue = new device_srv_msg_queue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_assign() Device Srv Msg Queue: End--- \r\n");
    }

    //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_assign() Device Srv Msg Queue Clear: Begin---\r\n");

    runthread->pDeviceSrvMsgQueue->clear();

    //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_assign() Device Srv Msg Queue Clear: End--- \r\n");

    /* 初始化数据库连接 */
    if (NULL == runthread->pDevice_Srv_dboper)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_assign() Device Srv DB Oper New:device_id=%s, device_ip=%s, device_port=%d \r\n", pDeviceInfo->device_id, pDeviceInfo->login_ip, pDeviceInfo->login_port);

        runthread->pDevice_Srv_dboper = new DBOper();

        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_assign() Device Srv DB Oper New End \r\n");

        if (runthread->pDevice_Srv_dboper == NULL)
        {
            //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_assign() exit---: Device Srv DB Oper NULL \r\n");
            DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return -1;
        }

        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_assign() Device Srv DB Start Connect:g_StrCon[0]=%s, g_StrCon[1]=%s \r\n", g_StrCon[0], g_StrCon[1]);

        if (runthread->pDevice_Srv_dboper->Connect(g_StrCon, (char*)"") < 0)
        {
            delete runthread->pDevice_Srv_dboper;
            runthread->pDevice_Srv_dboper = NULL;
            DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return -1;
        }

        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_assign() Device Srv DB Connect End \r\n");
    }

    /* 初始化日志数据库连接 */
    if (NULL == runthread->pDeviceLogDbOper)
    {
        runthread->pDeviceLogDbOper = new DBOper();

        if (runthread->pDeviceLogDbOper == NULL)
        {
            delete runthread->pDevice_Srv_dboper;
            runthread->pDevice_Srv_dboper = NULL;
            DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return -1;
        }

        if (runthread->pDeviceLogDbOper->Connect(g_StrConLog, (char*)"") < 0)
        {
            runthread->iDeviceLogDBOperConnectStatus = 0;
        }
        else
        {
            runthread->iDeviceLogDBOperConnectStatus = 1;
        }
    }

    DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_assign() Exit--- \r\n");

    //printf("\r\n device_srv_proc_thread_assign() Exit--- \r\n");
    return 0;
}

/*****************************************************************************
 函 数 名  : device_srv_proc_thread_recycle
 功能描述  : 前端设备业务处理线程回收
 输入参数  : char* device_id
             char* device_ip
             int device_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月27日
    作    者   : 前端设备路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
int device_srv_proc_thread_recycle(char* device_id, char* device_ip, int device_port)
{
    device_srv_msg_t* device_srv_msg = NULL;
    device_srv_proc_tl_t* runthread = NULL;

    if (NULL == device_id || NULL == device_ip || device_port <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_recycle() exit---: Param Error \r\n");
        return -1;
    }

    //rintf("\r\n device_srv_proc_thread_recycle() Enter--- \r\n");
    //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_recycle() Enter--- \r\n");

    DEVICE_SRV_PROC_THREAD_SMUTEX_LOCK();

    runthread = get_device_srv_proc_thread2(device_id, device_ip, device_port);

    if (NULL == runthread)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_recycle() exit---: Get Thread Error \r\n");
        DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return 0;
    }

    runthread->iUsed = 0;
    runthread->pDeviceInfo = NULL;

    if (NULL != runthread->pDevice_Srv_dboper)
    {
        delete runthread->pDevice_Srv_dboper;
        runthread->pDevice_Srv_dboper = NULL;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_recycle() Device Srv DB Oper Delete End--- \r\n");
    }

    if (NULL != runthread->pDeviceLogDbOper)
    {
        delete runthread->pDeviceLogDbOper;
        runthread->pDeviceLogDbOper = NULL;
    }

    runthread->iDeviceLogDBOperConnectStatus = 0;

    if (NULL != runthread->pDeviceSrvMsgQueue)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_recycle() DeviceSrvMsgQueue Free \r\n");

        while (!runthread->pDeviceSrvMsgQueue->empty())
        {
            device_srv_msg = (device_srv_msg_t*) runthread->pDeviceSrvMsgQueue->front();
            runthread->pDeviceSrvMsgQueue->pop_front();

            if (NULL != device_srv_msg)
            {
                device_srv_msg_free(device_srv_msg);
                device_srv_msg = NULL;
            }
        }

        runthread->pDeviceSrvMsgQueue->clear();
        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_recycle() DeviceSrvMsgQueue Free End--- \r\n");
    }

    DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    //printf("\r\n device_srv_proc_thread_recycle() Exit--- \r\n");
    //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_recycle() Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : device_srv_proc_thread_start_all
 功能描述  : 启动前端设备业务处理线程
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
int device_srv_proc_thread_start_all()
{
    int i = 0;
    int index = 0;
    device_srv_proc_tl_t* runthread = NULL;

    if (NULL == g_DeviceSrvProcThreadList || NULL == g_DeviceSrvProcThreadList->pDeviceSrvProcList)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_start() exit---: Param Error 2 \r\n");
        return -1;
    }

    DEVICE_SRV_PROC_THREAD_SMUTEX_LOCK();

    for (index = 0; index < MAX_DEVICE_SRV_THREADS; index++)
    {
        i = device_srv_proc_thread_init(&runthread);
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "device_srv_proc_thread_start() device_srv_proc_thread_init:i=%d \r\n", i);

        if (i != 0)
        {
            //DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "device_srv_proc_thread_start() exit---: Device Srv Proc Thread Init Error \r\n");
            continue;
        }

        //添加到前端设备业务处理线程队列
        i = osip_list_add(g_DeviceSrvProcThreadList->pDeviceSrvProcList, runthread, -1); /* add to list tail */
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "device_srv_proc_thread_start() osip_list_add:i=%d \r\n", i);

        if (i < 0)
        {
            device_srv_proc_thread_free(runthread);
            runthread = NULL;
            //DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "device_srv_proc_thread_start() exit---: List Add Error \r\n");
            continue;
        }

        //启动处理线程
        runthread->thread = (osip_thread_t*)osip_thread_create(20000, device_srv_proc_thread_for_appoint_execute, (void*)runthread);

        if (runthread->thread == NULL)
        {
            osip_list_remove(g_DeviceSrvProcThreadList->pDeviceSrvProcList, i);
            device_srv_proc_thread_free(runthread);
            runthread = NULL;
            //DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "device_srv_proc_thread_start() exit---: Device Srv Proc Thread Create Error \r\n");
            continue;
        }

        //printf("\r\n device_srv_proc_thread_start:runthread->thread=0x%lx \r\n", (unsigned long)runthread->thread);
    }

    DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : device_srv_proc_thread_stop_all
 功能描述  : 停止所有前端设备业务处理线程
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
int device_srv_proc_thread_stop_all()
{
    int pos = 0;
    int i = 0;
    device_srv_proc_tl_t* runthread = NULL;

    if (NULL == g_DeviceSrvProcThreadList || NULL == g_DeviceSrvProcThreadList->pDeviceSrvProcList)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_stop_all() exit---: Param Error \r\n");
        return -1;
    }

    DEVICE_SRV_PROC_THREAD_SMUTEX_LOCK();

    //查找队列，停止线程
    for (pos = 0; pos < osip_list_size(g_DeviceSrvProcThreadList->pDeviceSrvProcList); pos++)
    {
        runthread = (device_srv_proc_tl_t*)osip_list_get(g_DeviceSrvProcThreadList->pDeviceSrvProcList, pos);

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

    DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : device_srv_proc_thread_restart
 功能描述  : 前端设备业务处理线程重新启动
 输入参数  : GBDevice_info_t* pDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月12日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int device_srv_proc_thread_restart(GBDevice_info_t* pDeviceInfo)
{
    int pos = 0;
    int iRet = 0;
    device_srv_proc_tl_t* runthread = NULL;

    if (NULL == pDeviceInfo || pDeviceInfo->device_id[0] == '\0' || pDeviceInfo->login_ip[0] == '\0' || pDeviceInfo->login_port <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_restart() exit---: Param Error \r\n");
        return -1;
    }

    DEVICE_SRV_PROC_THREAD_SMUTEX_LOCK();

    //查找队列
    for (pos = 0; pos < osip_list_size(g_DeviceSrvProcThreadList->pDeviceSrvProcList); pos++)
    {
        runthread = (device_srv_proc_tl_t*)osip_list_get(g_DeviceSrvProcThreadList->pDeviceSrvProcList, pos);

        if (runthread == NULL || 0 == runthread->iUsed || NULL == runthread->pDeviceInfo)
        {
            continue;
        }

        if ('\0' == runthread->pDeviceInfo->device_id[0] || '\0' == runthread->pDeviceInfo->login_ip[0] || runthread->pDeviceInfo->login_port <= 0)
        {
            continue;
        }

        if ((0 == sstrcmp(runthread->pDeviceInfo->device_id, pDeviceInfo->device_id))
            && (0 == sstrcmp(runthread->pDeviceInfo->login_ip, pDeviceInfo->login_ip))
            && runthread->pDeviceInfo->login_port == pDeviceInfo->login_port)
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
            runthread->thread = (osip_thread_t*)osip_thread_create(20000, device_srv_proc_thread_for_appoint_execute, (void*)runthread);

            if (runthread->thread == NULL)
            {
                osip_list_remove(g_DeviceSrvProcThreadList->pDeviceSrvProcList, pos);
                device_srv_proc_thread_free(runthread);
                runthread = NULL;
                continue;
            }
        }
    }

    DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : device_srv_proc_thread_find
 功能描述  : 查找前端设备业务处理线程
 输入参数  : GBDevice_info_t* pDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int device_srv_proc_thread_find(GBDevice_info_t* pDeviceInfo)
{
    int pos = 0;
    device_srv_proc_tl_t* runthread = NULL;

    if (NULL == pDeviceInfo || pDeviceInfo->device_id[0] == '\0' || pDeviceInfo->login_ip[0] == '\0' || pDeviceInfo->login_port <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_find() exit---: Param Error \r\n");
        return -1;
    }

    DEVICE_SRV_PROC_THREAD_SMUTEX_LOCK();

    //查找队列
    for (pos = 0; pos < osip_list_size(g_DeviceSrvProcThreadList->pDeviceSrvProcList); pos++)
    {
        runthread = (device_srv_proc_tl_t*)osip_list_get(g_DeviceSrvProcThreadList->pDeviceSrvProcList, pos);

        if (runthread == NULL || 0 == runthread->iUsed || NULL == runthread->pDeviceInfo)
        {
            continue;
        }

        if ('\0' == runthread->pDeviceInfo->device_id[0] || '\0' == runthread->pDeviceInfo->login_ip[0] || runthread->pDeviceInfo->login_port <= 0)
        {
            continue;
        }

        if ((0 == sstrcmp(runthread->pDeviceInfo->device_id, pDeviceInfo->device_id))
            && (0 == sstrcmp(runthread->pDeviceInfo->login_ip, pDeviceInfo->login_ip))
            && runthread->pDeviceInfo->login_port == pDeviceInfo->login_port)
        {
            DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return pos;
        }
    }

    DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return -1;
}

/*****************************************************************************
 函 数 名  : get_free_device_srv_proc_thread
 功能描述  : 获取空闲的前端设备业务处理线程
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月27日
    作    者   : 前端设备路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
device_srv_proc_tl_t* get_free_device_srv_proc_thread()
{
    int pos = 0;
    device_srv_proc_tl_t* runthread = NULL;

    //查找队列，将sipudp从接收线程队列中移除
    for (pos = 0; pos < osip_list_size(g_DeviceSrvProcThreadList->pDeviceSrvProcList); pos++)
    {
        runthread = (device_srv_proc_tl_t*)osip_list_get(g_DeviceSrvProcThreadList->pDeviceSrvProcList, pos);

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
 函 数 名  : get_device_srv_proc_thread
 功能描述  : 获取前端设备业务处理线程
 输入参数  : char* device_id
             char* device_ip
             int device_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月27日
    作    者   : 前端设备路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
device_srv_proc_tl_t* get_device_srv_proc_thread(char* device_id, char* device_ip, int device_port)
{
    int pos = 0;
    device_srv_proc_tl_t* runthread = NULL;

    if (NULL == device_id || NULL == device_ip || device_port <= 0)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "get_device_srv_proc_thread() exit---: Param Error \r\n");
        return NULL;
    }

    DEVICE_SRV_PROC_THREAD_SMUTEX_LOCK();

    for (pos = 0; pos < osip_list_size(g_DeviceSrvProcThreadList->pDeviceSrvProcList); pos++)
    {
        runthread = (device_srv_proc_tl_t*)osip_list_get(g_DeviceSrvProcThreadList->pDeviceSrvProcList, pos);

        if (runthread == NULL || 0 == runthread->iUsed || NULL == runthread->pDeviceInfo)
        {
            continue;
        }

        if ('\0' == runthread->pDeviceInfo->device_id[0] || '\0' == runthread->pDeviceInfo->login_ip[0] || runthread->pDeviceInfo->login_port <= 0)
        {
            continue;
        }

        if ((0 == sstrcmp(runthread->pDeviceInfo->device_id, device_id))
            && (0 == sstrcmp(runthread->pDeviceInfo->login_ip, device_ip))
            && runthread->pDeviceInfo->login_port == device_port)
        {
            DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "get_device_srv_proc_thread() exit---: pos=%d, device_id=%s, device_ip=%s, device_port=%d \r\n", pos, device_id, device_ip, device_port);
            return runthread;
        }
    }

    DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : get_device_srv_proc_thread2
 功能描述  : 获取前端设备业务处理线程
 输入参数  : char* device_id
             char* device_ip
             int device_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月13日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
device_srv_proc_tl_t* get_device_srv_proc_thread2(char* device_id, char* device_ip, int device_port)
{
    int pos = 0;
    device_srv_proc_tl_t* runthread = NULL;

    if (NULL == device_id || NULL == device_ip || device_port <= 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "get_device_srv_proc_thread2() exit---: Param Error \r\n");
        return NULL;
    }

    for (pos = 0; pos < osip_list_size(g_DeviceSrvProcThreadList->pDeviceSrvProcList); pos++)
    {
        runthread = (device_srv_proc_tl_t*)osip_list_get(g_DeviceSrvProcThreadList->pDeviceSrvProcList, pos);

        if (runthread == NULL || 0 == runthread->iUsed || NULL == runthread->pDeviceInfo)
        {
            continue;
        }

        if ('\0' == runthread->pDeviceInfo->device_id[0] || '\0' == runthread->pDeviceInfo->login_ip[0] || runthread->pDeviceInfo->login_port <= 0)
        {
            continue;
        }

        if ((0 == sstrcmp(runthread->pDeviceInfo->device_id, device_id))
            && (0 == sstrcmp(runthread->pDeviceInfo->login_ip, device_ip))
            && runthread->pDeviceInfo->login_port == device_port)
        {
            return runthread;
        }
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : device_srv_proc_thread_list_init
 功能描述  : 初始化前端设备业务处理线程队列
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
int device_srv_proc_thread_list_init()
{
    g_DeviceSrvProcThreadList = (device_srv_proc_tl_list_t*)osip_malloc(sizeof(device_srv_proc_tl_list_t));

    if (g_DeviceSrvProcThreadList == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_list_init() exit---: g_DeviceSrvProcThreadList Smalloc Error \r\n");
        return -1;
    }

    g_DeviceSrvProcThreadList->pDeviceSrvProcList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (g_DeviceSrvProcThreadList->pDeviceSrvProcList == NULL)
    {
        osip_free(g_DeviceSrvProcThreadList);
        g_DeviceSrvProcThreadList = NULL;
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_list_init() exit---: Device Srv Proc List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_DeviceSrvProcThreadList->pDeviceSrvProcList);

#ifdef MULTI_THR
    /* init smutex */
    g_DeviceSrvProcThreadList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_DeviceSrvProcThreadList->lock)
    {
        osip_free(g_DeviceSrvProcThreadList->pDeviceSrvProcList);
        g_DeviceSrvProcThreadList->pDeviceSrvProcList = NULL;
        osip_free(g_DeviceSrvProcThreadList);
        g_DeviceSrvProcThreadList = NULL;
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_list_init() exit---: Device Srv Proc List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : device_srv_proc_thread_list_free
 功能描述  : 释放前端设备业务处理线程队列
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
void device_srv_proc_thread_list_free()
{
    if (NULL == g_DeviceSrvProcThreadList)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_DeviceSrvProcThreadList->pDeviceSrvProcList)
    {
        osip_list_special_free(g_DeviceSrvProcThreadList->pDeviceSrvProcList, (void (*)(void*))&device_srv_proc_thread_free);
        osip_free(g_DeviceSrvProcThreadList->pDeviceSrvProcList);
        g_DeviceSrvProcThreadList->pDeviceSrvProcList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_DeviceSrvProcThreadList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_DeviceSrvProcThreadList->lock);
        g_DeviceSrvProcThreadList->lock = NULL;
    }

#endif

    osip_free(g_DeviceSrvProcThreadList);
    g_DeviceSrvProcThreadList = NULL;

}

/*****************************************************************************
 函 数 名  : device_srv_proc_thread_list_lock
 功能描述  : 前端设备业务处理线程队列加锁
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
int device_srv_proc_thread_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_DeviceSrvProcThreadList == NULL || g_DeviceSrvProcThreadList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_DeviceSrvProcThreadList->lock);
#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : device_srv_proc_thread_list_unlock
 功能描述  : 前端设备业务处理线程队列解锁
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
int device_srv_proc_thread_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_DeviceSrvProcThreadList == NULL || g_DeviceSrvProcThreadList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "device_srv_proc_thread_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_DeviceSrvProcThreadList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_device_srv_proc_thread_list_lock
 功能描述  : 前端设备业务处理线程队列加锁
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
int debug_device_srv_proc_thread_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_DeviceSrvProcThreadList == NULL || g_DeviceSrvProcThreadList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "debug_device_srv_proc_thread_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_DeviceSrvProcThreadList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_device_srv_proc_thread_list_unlock
 功能描述  : 前端设备业务处理线程队列解锁
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
int debug_device_srv_proc_thread_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_DeviceSrvProcThreadList == NULL || g_DeviceSrvProcThreadList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "debug_device_srv_proc_thread_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_DeviceSrvProcThreadList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : appoint_device_srv_msg_list_clean
 功能描述  : 前端设备业务消息队列清除
 输入参数  : device_srv_proc_tl_t* run
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年3月8日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void appoint_device_srv_msg_list_clean(device_srv_proc_tl_t* run)
{
    int iRet = 0;
    device_srv_msg_t* pDeviceSrvMsg = NULL;

    if (NULL == run)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "appoint_device_srv_msg_list_clean() Param Error \r\n");
        return;
    }

    if (NULL == run->pDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "appoint_device_srv_msg_list_clean() Device Info Error \r\n");
        return;
    }

    if (NULL == run->pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "appoint_device_srv_msg_list_clean() Device Srv dboper Error \r\n");
        return;
    }

    if (NULL == run->pDeviceSrvMsgQueue)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "appoint_device_srv_msg_list_clean() Device Srv Message Queue Error \r\n");
        return;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)run->pDeviceSrvMsgQueueLock);
#endif

    while (!run->pDeviceSrvMsgQueue->empty())
    {
        pDeviceSrvMsg = (device_srv_msg_t*) run->pDeviceSrvMsgQueue->front();
        run->pDeviceSrvMsgQueue->pop_front();

        if (NULL != pDeviceSrvMsg)
        {
            device_srv_msg_free(pDeviceSrvMsg);
            pDeviceSrvMsg = NULL;
        }
    }

    run->pDeviceSrvMsgQueue->clear();

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)run->pDeviceSrvMsgQueueLock);
#endif

    return;
}

/*****************************************************************************
 函 数 名  : scan_device_srv_proc_thread_list
 功能描述  : 扫描前端设备业务处理线程队列
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
void scan_device_srv_proc_thread_list()
{
    int i = 0;
    //int iRet = 0;
    device_srv_proc_tl_t* pThreadProc = NULL;
    needtoproc_devicesrvproc_queue needToProc;
    time_t now = time(NULL);

    if ((NULL == g_DeviceSrvProcThreadList) || (NULL == g_DeviceSrvProcThreadList->pDeviceSrvProcList))
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "scan_device_srv_proc_thread_list() exit---: Param Error \r\n");
        return;
    }

    needToProc.clear();

    DEVICE_SRV_PROC_THREAD_SMUTEX_LOCK();

    if (osip_list_size(g_DeviceSrvProcThreadList->pDeviceSrvProcList) <= 0)
    {
        DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_DeviceSrvProcThreadList->pDeviceSrvProcList); i++)
    {
        pThreadProc = (device_srv_proc_tl_t*)osip_list_get(g_DeviceSrvProcThreadList->pDeviceSrvProcList, i);

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

        if (NULL == pThreadProc->pDeviceInfo)
        {
            continue;
        }

        if (pThreadProc->run_time < now && now - pThreadProc->run_time > 3600)
        {
            needToProc.push_back(pThreadProc);
        }
    }

    DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    /* 处理需要开始的 */
    while (!needToProc.empty())
    {
        pThreadProc = (device_srv_proc_tl_t*) needToProc.front();
        needToProc.pop_front();

        if (NULL != pThreadProc)
        {
            if (NULL != pThreadProc->pDeviceInfo)
            {
                //iRet = device_srv_proc_thread_restart(pThreadProc->pDeviceInfo);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "前端设备业务处理线程监控线程检测到前端设备业务处理线程挂死, 前端设备ID=%s, 前端设备IP=%s, 前端设备端口=%d", pThreadProc->pDeviceInfo->device_id, pThreadProc->pDeviceInfo->login_ip, pThreadProc->pDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Device service processing thread monitor thread hanging dead restart device services processing threads,Device ID=%s, Device IP=%s, Device port=%d", pThreadProc->pDeviceInfo->device_id, pThreadProc->pDeviceInfo->login_ip, pThreadProc->pDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_DEVICE, LOG_FATAL, "scan_device_srv_proc_thread_list(): device_srv_proc_thread restart:Thread device_id=%s, device_ip=%s, device_port=%d, \r\n", pThreadProc->pDeviceInfo->device_id, pThreadProc->pDeviceInfo->login_ip, pThreadProc->pDeviceInfo->login_port);
                osip_usleep(5000000);
                system((char*)"killall cms; killall -9 cms");
            }
        }
    }

    needToProc.clear();

    return;
}

/*****************************************************************************
 函 数 名  : show_device_srv_proc_thread
 功能描述  : 显示前端设备线程的使用情况
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
void show_device_srv_proc_thread(int sock, int type)
{
    int i = 0;
    int pos = 0;
    char strLine[] = "\r-----------------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rThread Index  Thread TID  Used Flag  Device ID            Device IP       DevicePort SizeOfMsgQueue Run Time           \r\n";
    char rbuf[256] = {0};
    device_srv_proc_tl_t* runthread = NULL;
    char strRunTime[64] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    DEVICE_SRV_PROC_THREAD_SMUTEX_LOCK();

    if (NULL == g_DeviceSrvProcThreadList || osip_list_size(g_DeviceSrvProcThreadList->pDeviceSrvProcList) <= 0)
    {
        DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return;
    }

    for (pos = 0; pos < osip_list_size(g_DeviceSrvProcThreadList->pDeviceSrvProcList); pos++)
    {
        runthread = (device_srv_proc_tl_t*)osip_list_get(g_DeviceSrvProcThreadList->pDeviceSrvProcList, pos);

        if (runthread == NULL)
        {
            continue;
        }

        if (0 == type) /* 显示未使用的 */
        {
            if (1 == runthread->iUsed || NULL != runthread->pDeviceInfo)
            {
                continue;
            }
        }
        else if (1 == type) /* 显示已经使用的 */
        {
            if (0 == runthread->iUsed || NULL == runthread->pDeviceInfo)
            {
                continue;
            }
        }
        else if (2 == type) /* 显示全部 */
        {

        }

        i = format_time(runthread->run_time, strRunTime);

        if (NULL != runthread->pDeviceInfo)
        {
            if (NULL != runthread->pDeviceSrvMsgQueue)
            {
                snprintf(rbuf, 256, "\r%-12u  %-10u  %-9u  %-20s %-15s %-9d  %-14d %-19s\r\n", pos, *(runthread->thread), runthread->iUsed, runthread->pDeviceInfo->device_id, runthread->pDeviceInfo->login_ip, runthread->pDeviceInfo->login_port, (int)runthread->pDeviceSrvMsgQueue->size(), strRunTime);
            }
            else
            {
                snprintf(rbuf, 256, "\r%-12u  %-10u  %-9u  %-20s %-15s %-9d  %-14d %-19s\r\n", pos, *(runthread->thread), runthread->iUsed, runthread->pDeviceInfo->device_id, runthread->pDeviceInfo->login_ip, runthread->pDeviceInfo->login_port, 0, strRunTime);
            }
        }
        else
        {
            if (NULL != runthread->pDeviceSrvMsgQueue)
            {
                snprintf(rbuf, 256, "\r%-12u  %-10u  %-9u  %-20s %-15s %-9d  %-14d %-19s\r\n", pos, *(runthread->thread), runthread->iUsed, (char*)"NULL", (char*)"NULL", 0, (int)runthread->pDeviceSrvMsgQueue->size(), strRunTime);
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

    DEVICE_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

/*****************************************************************************
 函 数 名  : scan_appoint_device_srv_msg_list
 功能描述  : 扫描指定的多级互联消息队列
 输入参数  : device_srv_proc_tl_t* run
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月30日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int scan_appoint_device_srv_msg_list(device_srv_proc_tl_t* run)
{
    int iRet = 0;
    device_srv_msg_t* pDeviceSrvMsg = NULL;
    static int connect_interval = 0;

    if (NULL == run)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "scan_appoint_device_srv_msg_list() Param Error \r\n");
        return -1;
    }

    if (NULL == run->pDeviceInfo)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "scan_appoint_device_srv_msg_list() Device Info Error \r\n");
        return -1;
    }

    if (NULL == run->pDevice_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "scan_appoint_device_srv_msg_list() Device Srv dboper Error \r\n");
        return -1;
    }

    if (NULL == run->pDeviceSrvMsgQueue)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "scan_appoint_device_srv_msg_list() Device Srv Message Queue Error \r\n");
        return -1;
    }

    if (!run->iDeviceLogDBOperConnectStatus)
    {
        connect_interval++;

        if (connect_interval >= 60 * 200)
        {
            if (run->pDeviceLogDbOper->Connect(g_StrConLog, (char*)"") < 0)
            {
                run->iDeviceLogDBOperConnectStatus = 0;
            }
            else
            {
                run->iDeviceLogDBOperConnectStatus = 1;
            }

            connect_interval = 0;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)run->pDeviceSrvMsgQueueLock);
#endif

    while (!run->pDeviceSrvMsgQueue->empty())
    {
        pDeviceSrvMsg = (device_srv_msg_t*) run->pDeviceSrvMsgQueue->front();
        run->pDeviceSrvMsgQueue->pop_front();

        if (NULL != pDeviceSrvMsg)
        {
            break;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)run->pDeviceSrvMsgQueueLock);
#endif

    if (NULL != pDeviceSrvMsg)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_INFO,  "scan_appoint_device_srv_msg_list() \
        \r\n In Param: \
        \r\n msg_type=%d \
        \r\n caller_id=%s \
        \r\n callee_id=%s \
        \r\n response_code=%d \
        \r\n ua_dialog_index=%d \
        \r\n msg_body_len=%d \
        \r\n ", pDeviceSrvMsg->msg_type, pDeviceSrvMsg->caller_id, pDeviceSrvMsg->callee_id, pDeviceSrvMsg->response_code, pDeviceSrvMsg->ua_dialog_index, pDeviceSrvMsg->msg_body_len);

        iRet = device_srv_msg_proc(pDeviceSrvMsg, run->pDevice_Srv_dboper);

        device_srv_msg_free(pDeviceSrvMsg);
        pDeviceSrvMsg = NULL;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : device_srv_msg_add_for_appoint
 功能描述  : 添加前端设备业务消息到指定的线程队列中
 输入参数  : device_srv_proc_tl_t* pDeviceSrvProcThd
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
int device_srv_msg_add_for_appoint(device_srv_proc_tl_t* pDeviceSrvProcThd, msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, char* reasonphrase, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos)
{
    device_srv_msg_t* pDeviceSrvMsg = NULL;
    int iRet = 0;

    if (NULL == pDeviceSrvProcThd || caller_id == NULL || callee_id == NULL)
    {
        return -1;
    }

    if (NULL == pDeviceSrvProcThd->pDeviceInfo)
    {
        return -1;
    }

    if (NULL == pDeviceSrvProcThd->pDeviceSrvMsgQueue)
    {
        return -1;
    }

    iRet = device_srv_msg_init(&pDeviceSrvMsg);

    if (iRet != 0)
    {
        return -1;
    }

    pDeviceSrvMsg->msg_type = msg_type;
    pDeviceSrvMsg->pGBDeviceInfo = pDeviceSrvProcThd->pDeviceInfo;

    if (NULL != caller_id)
    {
        osip_strncpy(pDeviceSrvMsg->caller_id, caller_id, MAX_ID_LEN);
    }

    if (NULL != callee_id)
    {
        osip_strncpy(pDeviceSrvMsg->callee_id, callee_id, MAX_ID_LEN);
    }

    pDeviceSrvMsg->response_code = response_code;

    if (NULL != reasonphrase)
    {
        osip_strncpy(pDeviceSrvMsg->reasonphrase, reasonphrase, MAX_128CHAR_STRING_LEN);
    }

    pDeviceSrvMsg->ua_dialog_index = ua_dialog_index;

    if (NULL != msg_body)
    {
        osip_strncpy(pDeviceSrvMsg->msg_body, msg_body, MAX_MSG_BODY_STRING_LEN);
    }

    pDeviceSrvMsg->msg_body_len = msg_body_len;
    pDeviceSrvMsg->cr_pos = cr_pos;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)pDeviceSrvProcThd->pDeviceSrvMsgQueueLock);
#endif

    pDeviceSrvProcThd->pDeviceSrvMsgQueue->push_back(pDeviceSrvMsg);

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)pDeviceSrvProcThd->pDeviceSrvMsgQueueLock);
#endif

    return 0;
}
#endif

#if DECS("ZRV设备的TCP连接管理")
/*****************************************************************************
 函 数 名  : zrv_device_tcp_client_init
 功能描述  : ZRV设备TCP结构初始化
 输入参数  : user_tcp_client_t* sc
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void zrv_device_tcp_client_init(zrv_device_tcp_client_t* sc)
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
    sc->iRcvBuffLenCount = 0;
    return;
}

/*****************************************************************************
 函 数 名  : ZRVDeviceLoginServerInit
 功能描述  : ZRV设备TCP服务端初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ZRVDeviceLoginServerInit()
{
    int i = 0;
    int sock = -1;               /* socket to create */
    struct sockaddr_in ServAddr; /* Local address */
    int val = 1;
    int iSendBuf = 1024 * 1024;

    for (i = 0; i < MAX_ZRV_DEVICE_TCP_CLIENTS; i++)
    {
        zrv_device_tcp_client_init(&ZRVDeviceTCPClients[i]);
    }

    /* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("ZRVDeviceLoginServerInit create socket error!!!     reason:");
        return -1;
    }

    /*closesocket（一般不会立即关闭而经历TIME_WAIT的过程）后想继续重用该socket：*/
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    /* 设置接收缓存区 */
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&iSendBuf, sizeof(int));

    /* Construct local address structure */
    memset(&ServAddr, 0, sizeof(ServAddr));          /* Zero out structure */
    ServAddr.sin_family = AF_INET;                   /* Internet address family */
    ServAddr.sin_addr.s_addr = htonl(INADDR_ANY);    /* Any incoming interface */
    ServAddr.sin_port = htons(ZRV_SERVER_TCP_PORT);  /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr*) &ServAddr, sizeof(ServAddr)) < 0)
    {
        perror("ZRVDeviceLoginServerInit bind socket error!!!     reason:");
        close(sock);
        return -1;
    }

    /* Mark the socket so it will listen for incoming connections */
    if (listen(sock, MAX_ZRV_DEVICE_TCP_CLIENTS) < 0)
    {
        perror("ZRVDeviceLoginServerInit listen socket error!!!     reason:");
        close(sock);
        return -1;
    }

    ZRVDeviceClientServSock = sock;

    return 0;
}

/*****************************************************************************
 函 数 名  : UserLoginServerFree
 功能描述  : ZRV设备TCP服务端释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void ZRVDeviceLoginServerFree()
{
    int i;

    if (ZRVDeviceClientServSock <= 0)
    {
        return;
    }

    close(ZRVDeviceClientServSock);
    ZRVDeviceClientServSock = -1;

    for (i = 0; i < MAX_ZRV_DEVICE_TCP_CLIENTS; i++)
    {
        if (ZRVDeviceTCPClients[i].sock != -1)
        {
            close(ZRVDeviceTCPClients[i].sock);
        }

        zrv_device_tcp_client_init(&ZRVDeviceTCPClients[i]);
    }

    return;
}

/*****************************************************************************
 函 数 名  : UserLoginServerMain
 功能描述  : ZRV设备TCP服务端主程序
 输入参数  : DBOper* pDbOper
             int* run_thread_time
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void ZRVDeviceLoginServerMain(DBOper* pDbOper, int* run_thread_time)
{
    int i = 0;
    int p = 0;
    int  maxDescriptor = 0;              /* Maximum socket descriptor value */
    fd_set sockSet;                      /* Set of socket descriptors for select() */
    struct timeval val;                  /* Timeout for select() */
    char buff[1024 * 1024] = {0};        /* 接收消息缓存 */
    char tmp_buff[1024 * 1024] = {0};    /* 接收消息临时缓存 */
    char tmp1_buff[1024 * 1024] = {0};   /* 接收消息临时缓存 */
    char strBodyBuff[1024 * 1024] = {0}; /* 接收消息体*/
    char* tmp_prex_buff = NULL;
    char from_ip[16] = {0};
    int  clientSock = 0;                 /* Socket descriptor for client */
    int  recvSize = 0;
    int  remainSize = 0;                 /* 剩下的长度 */
    int  deal_count = 0;                 /* 处理次数 */
    int  iRet = 0;
    //int iSendBuf = 1024 * 1024;
    struct timeval iTimeout;
    TCP_COMMON_Head stMsgHead;
    int iBodyLen = 0;
    int enable = 1;

    if (ZRVDeviceClientServSock <= 0)
    {
        ZRVDeviceLoginServerInit();

        if (ZRVDeviceClientServSock <= 0)
        {
            return;
        }
    }

    FD_ZERO(&sockSet);
    FD_SET(ZRVDeviceClientServSock, &sockSet);
    maxDescriptor = ZRVDeviceClientServSock;

    for (i = 0; i < MAX_ZRV_DEVICE_TCP_CLIENTS; i++)
    {
        if (ZRVDeviceTCPClients[i].sock != -1)
        {
            FD_SET(ZRVDeviceTCPClients[i].sock, &sockSet);

            if (ZRVDeviceTCPClients[i].sock > maxDescriptor)
            {
                maxDescriptor = ZRVDeviceTCPClients[i].sock;
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

    if (FD_ISSET(ZRVDeviceClientServSock, &sockSet))
    {
        struct sockaddr_in ClntAddr;     /* Client address */
        unsigned int clntLen = 0;        /* Length of client address data structure */
        clntLen = sizeof(ClntAddr);

        /* Wait for a client to connect */
        if ((clientSock = accept(ZRVDeviceClientServSock, (struct sockaddr*) &ClntAddr,
                                 &clntLen)) < 0)
        {
            return;
        }

        for (p = 0; p < MAX_ZRV_DEVICE_TCP_CLIENTS; p++)
        {
            if (ZRVDeviceTCPClients[p].sock == -1)
            {
                break;
            }
        }

        if (p >= MAX_ZRV_DEVICE_TCP_CLIENTS)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ZRV设备TCP连接数达到上限, 关闭ZRV设备的TCP连接:TCP Socket=%d", clientSock);
            close(clientSock);

            /* 释放掉没用的TCP链接 */
            free_unused_zrv_device_tcp_connect();
            return;
        }
        else
        {
            if (NULL == inet_ntop(AF_INET, (void*) & (ClntAddr.sin_addr), from_ip, sizeof(from_ip)))
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "获取ZRV设备TCP连接的IP地址失败, 关闭ZRV设备的TCP连接:TCP Socket=%d", clientSock);
                close(clientSock);
                return;
            }

            /* 设置发送超时时间 */
            iTimeout.tv_sec = 1; /* 1秒超时 */
            iTimeout.tv_usec = 0;
            setsockopt(clientSock, SOL_SOCKET, SO_SNDTIMEO, (char *)&iTimeout, sizeof(struct timeval));

            /* 设置发送缓冲区，设置为0，立即发送，不需要缓冲，防止对端接收粘包 */
            //setsockopt(clientSock, SOL_SOCKET, SO_SNDBUF, (char *)&iSendBuf, sizeof(int));

            /* 选项TCP_NODELAY是禁用Nagle算法，即数据包立即发送出去，而选项TCP_CORK与此相反，可以认为它是Nagle算法的进一步增强，即阻塞数据包发送 */
            /* 禁止nagle算法，有需要发送的就立即发送 */
            setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));

            ZRVDeviceTCPClients[p].sock = clientSock;
            ZRVDeviceTCPClients[p].login_port = ntohs(ClntAddr.sin_port);
            osip_strncpy(ZRVDeviceTCPClients[p].login_ip, from_ip, MAX_IP_LEN);
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "ZRVDeviceLoginServerMain() ZRV Device: IP=%s, Port=%d, Socket=%d Connect \r\n", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, clientSock);
        }
    }

    for (p = 0; p < MAX_ZRV_DEVICE_TCP_CLIENTS; p++)
    {
        if (ZRVDeviceTCPClients[p].sock == -1)
        {
            continue;
        }

        if (FD_ISSET(ZRVDeviceTCPClients[p].sock, &sockSet))
        {
            clientSock = ZRVDeviceTCPClients[p].sock;

            memset(buff, 0, 1024 * 1024);
            recvSize = recv(clientSock, buff, sizeof(buff), 0);

            if (recvSize == 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收到ZRV发送的TCP消息长度为0, 主动关闭TCP链接:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, errno=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, errno);
                DEBUG_TRACE(MODULE_USER, LOG_INFO, "ZRVDeviceLoginServerMain() ZRV Device: IP=%s, Port=%d, Socket=%d, recvSize=%d Closed1 \r\n", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, clientSock, recvSize);
                close(clientSock);

                if (ZRVDeviceTCPClients[p].iRcvBuffLen > 0 && ZRVDeviceTCPClients[p].strRcvBuff[0] != '\0')
                {
                    /* 解析处理 */
                    iRet = ZRVDeviceParseTCPSocketDataProc(ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].strRcvBuff, ZRVDeviceTCPClients[p].iRcvBuffLen, pDbOper, run_thread_time);

                    if (0 != iRet)
                    {
                        if (-2 == iRet)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "解析ZRV发送的TCP消息失败, 不支持的消息格式:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "解析ZRV发送的TCP消息失败:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        }
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "解析ZRV发送的TCP消息成功:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                    }
                }

                /* 关闭 */
                zrv_device_tcp_client_init(&ZRVDeviceTCPClients[p]);
                free_zrv_device_info_by_tcp_socket(clientSock);
                continue;
            }
            else if (recvSize < 0)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收到ZRV发送的TCP消息错误, 主动关闭TCP链接:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, errno=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, errno);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "ZRVDeviceLoginServerMain() ZRV Device: IP=%s, Port=%d, Socket=%d, recvSize=%d Closed2 \r\n", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, clientSock, recvSize);
                    close(clientSock);

                    if (ZRVDeviceTCPClients[p].iRcvBuffLen > 0 && ZRVDeviceTCPClients[p].strRcvBuff[0] != '\0')
                    {
                        /* 解析处理 */
                        iRet = ZRVDeviceParseTCPSocketDataProc(ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].strRcvBuff, ZRVDeviceTCPClients[p].iRcvBuffLen, pDbOper, run_thread_time);

                        if (0 != iRet)
                        {
                            if (-2 == iRet)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "解析ZRV发送的TCP消息失败, 不支持的消息格式:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "解析ZRV发送的TCP消息失败:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                            }
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "解析ZRV发送的TCP消息成功:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        }
                    }

                    /* 关闭 */
                    zrv_device_tcp_client_init(&ZRVDeviceTCPClients[p]);
                    free_zrv_device_info_by_tcp_socket(clientSock);
                    continue;
                }
                else if (errno == EAGAIN) /* 接收超时 */
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "接收到ZRV发送的TCP消息超时, 继续接收:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                    continue;
                }
            }

            if (0 == strncmp(buff, "ZBITCLOUDMSG", MAX_TCPHEAD_MARK_LEN)) /* 一个消息的开始 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "接收到ZRV发送的TCP消息, 从消息头开始, 开始新的解析:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);

                memset(tmp_buff, 0, 1024 * 1024);
                memcpy(tmp_buff, buff, 1024 * 1024);
                remainSize = recvSize;
                deal_count = 0;

                while (remainSize > 0)
                {
                    /* 取出消息体 */
                    memset(&stMsgHead, 0, MAX_TCPHEAD_LEN);
                    memcpy(&stMsgHead, tmp_buff, MAX_TCPHEAD_LEN);

                    if (stMsgHead.iMsgBodyLen <= 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收到ZRV发送的TCP消息头里面携带的长度为0, 主动过滤掉:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收的消息内容=%s\r\n", tmp_buff);
                        break;
                    }

                    if (remainSize == stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN) /* 完整收完 */
                    {
                        /* 追加到字符串 */
                        strncat(ZRVDeviceTCPClients[p].strRcvBuff, &tmp_buff[MAX_TCPHEAD_LEN], stMsgHead.iMsgBodyLen);
                        ZRVDeviceTCPClients[p].iRcvBuffLenCount += stMsgHead.iMsgBodyLen;
                        ZRVDeviceTCPClients[p].iRcvBuffLen = stMsgHead.iMsgBodyLen;

                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收到ZRV发送的TCP消息正好是一个完整报文:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        break;
                    }
                    else if (remainSize < stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN) /* 没有收完 */
                    {
                        /* 追加到字符串 */
                        strncat(ZRVDeviceTCPClients[p].strRcvBuff, &tmp_buff[MAX_TCPHEAD_LEN], remainSize - MAX_TCPHEAD_LEN);
                        ZRVDeviceTCPClients[p].iRcvBuffLenCount += remainSize - MAX_TCPHEAD_LEN;
                        ZRVDeviceTCPClients[p].iRcvBuffLen = stMsgHead.iMsgBodyLen;

                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收到ZRV发送的TCP消息没有完整接收全, 等待下一个报文接收完全:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        break;
                    }
                    else if (remainSize > stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN) /* 收到不止一个报文 */
                    {
                        /* 追加到字符串 */
                        strncat(ZRVDeviceTCPClients[p].strRcvBuff, &tmp_buff[MAX_TCPHEAD_LEN], stMsgHead.iMsgBodyLen);
                        ZRVDeviceTCPClients[p].iRcvBuffLenCount += stMsgHead.iMsgBodyLen;
                        ZRVDeviceTCPClients[p].iRcvBuffLen = stMsgHead.iMsgBodyLen;

                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收到ZRV发送的TCP消息多余一个完整报文, 存在粘包, 分个处理:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);

                        /* 先解析处理完成第一个报文 */
                        deal_count++;
                        iRet = ZRVDeviceParseTCPSocketDataProc(ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].strRcvBuff, ZRVDeviceTCPClients[p].iRcvBuffLen, pDbOper, run_thread_time);

                        if (0 != iRet)
                        {
                            if (-2 == iRet)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "解析ZRV发送的TCP消息中的第%d个报文失败, 不支持的消息格式:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", deal_count, ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "解析ZRV发送的TCP消息中的第%d个报文失败:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", deal_count, ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                            }
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "解析ZRV发送的TCP消息中的第%d个报文成功:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", deal_count, ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        }

                        memset(ZRVDeviceTCPClients[p].strRcvBuff, 0, 1024 * 1024);
                        ZRVDeviceTCPClients[p].iRcvBuffLen = 0;
                        ZRVDeviceTCPClients[p].iRcvBuffLenCount = 0;

                        /* buffer重新赋值 */
                        remainSize = remainSize - (stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN);
                        memset(tmp1_buff, 0, 1024 * 1024);
                        memcpy(tmp1_buff, &tmp_buff[stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN], remainSize);
                        memset(tmp_buff, 0, 1024 * 1024);
                        memcpy(tmp_buff, tmp1_buff, 1024 * 1024);
                    }
                }
            }
            else /* 没有接收全的消息继续接收 */
            {
                if (ZRVDeviceTCPClients[p].iRcvBuffLen == 0) /* 没有收到头，过滤掉 */
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收到ZRV发送的TCP消息, 但是之前没有收到消息头, 主动过滤掉:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收的消息内容=%s\r\n", buff);
                    continue;
                }

                tmp_prex_buff = strstr(buff, "ZBITCLOUDMSG");

                if (NULL == tmp_prex_buff) /* 没有找到，说明该包里面没有下个报文 */
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收到ZRV发送的TCP消息, 但是没有消息头, 直接追加到上一条消息中进行处理:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);

                    /* 追加到字符串 */
                    strncat(ZRVDeviceTCPClients[p].strRcvBuff, buff, recvSize);
                    ZRVDeviceTCPClients[p].iRcvBuffLenCount += recvSize;
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收到ZRV发送的TCP消息, 其中存在消息头, 先把消息头前部消息追加到上一条消息处理:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);

                    /* 先把上一节消息处理完 */
                    /* 追加到字符串 */
                    strncat(ZRVDeviceTCPClients[p].strRcvBuff, buff, tmp_prex_buff - buff);
                    ZRVDeviceTCPClients[p].iRcvBuffLenCount += tmp_prex_buff - buff;

                    /* 解析处理 */
                    iRet = ZRVDeviceParseTCPSocketDataProc(ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].strRcvBuff, ZRVDeviceTCPClients[p].iRcvBuffLen, pDbOper, run_thread_time);

                    if (0 != iRet)
                    {
                        if (-2 == iRet)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "解析ZRV发送的TCP消息失败, 不支持的消息格式:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "解析ZRV发送的TCP消息失败:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        }
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "解析ZRV发送的TCP消息成功:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                    }

                    memset(ZRVDeviceTCPClients[p].strRcvBuff, 0, 1024 * 1024);
                    ZRVDeviceTCPClients[p].iRcvBuffLen = 0;
                    ZRVDeviceTCPClients[p].iRcvBuffLenCount = 0;

                    memset(tmp_buff, 0, 1024 * 1024);
                    memcpy(tmp_buff, tmp_prex_buff, 1024 * 1024);
                    remainSize = recvSize - (tmp_prex_buff - buff);
                    deal_count = 0;

                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收到ZRV发送的TCP消息, 继续处理消息头后部分的消息内容:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 剩余的消息长度=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, remainSize);

                    while (remainSize > 0)
                    {
                        /* 取出消息体 */
                        memset(&stMsgHead, 0, MAX_TCPHEAD_LEN);
                        memcpy(&stMsgHead, tmp_buff, MAX_TCPHEAD_LEN);

                        if (stMsgHead.iMsgBodyLen <= 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收到ZRV发送的TCP消息头里面携带的长度为0, 主动过滤掉:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收的消息内容=%s\r\n", tmp_buff);
                            break;
                        }

                        if (remainSize == stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN) /* 完整收完 */
                        {
                            /* 追加到字符串 */
                            strncat(ZRVDeviceTCPClients[p].strRcvBuff, &tmp_buff[MAX_TCPHEAD_LEN], stMsgHead.iMsgBodyLen);
                            ZRVDeviceTCPClients[p].iRcvBuffLenCount += stMsgHead.iMsgBodyLen;
                            ZRVDeviceTCPClients[p].iRcvBuffLen = stMsgHead.iMsgBodyLen;

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收到ZRV发送的TCP消息正好是一个完整报文:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                            break;
                        }
                        else if (remainSize < stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN) /* 没有收完 */
                        {
                            /* 追加到字符串 */
                            strncat(ZRVDeviceTCPClients[p].strRcvBuff, &tmp_buff[MAX_TCPHEAD_LEN], remainSize - MAX_TCPHEAD_LEN);
                            ZRVDeviceTCPClients[p].iRcvBuffLenCount += remainSize - MAX_TCPHEAD_LEN;
                            ZRVDeviceTCPClients[p].iRcvBuffLen = stMsgHead.iMsgBodyLen;

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收到ZRV发送的TCP消息没有完整接收全, 等待下一个报文接收完全:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                            break;
                        }
                        else if (remainSize > stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN) /* 收到不止一个报文 */
                        {
                            /* 追加到字符串 */
                            strncat(ZRVDeviceTCPClients[p].strRcvBuff, &tmp_buff[MAX_TCPHEAD_LEN], stMsgHead.iMsgBodyLen);
                            ZRVDeviceTCPClients[p].iRcvBuffLenCount += stMsgHead.iMsgBodyLen;
                            ZRVDeviceTCPClients[p].iRcvBuffLen = stMsgHead.iMsgBodyLen;

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收到ZRV发送的TCP消息多余一个完整报文, 存在粘包, 分个处理:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);

                            /* 先解析处理完成第一个报文 */
                            deal_count++;
                            iRet = ZRVDeviceParseTCPSocketDataProc(ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].strRcvBuff, ZRVDeviceTCPClients[p].iRcvBuffLen, pDbOper, run_thread_time);

                            if (0 != iRet)
                            {
                                if (-2 == iRet)
                                {
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "解析ZRV发送的TCP消息中的第%d个报文失败, 不支持的消息格式:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", deal_count, ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                                }
                                else
                                {
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "解析ZRV发送的TCP消息中的第%d个报文失败:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", deal_count, ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                                }
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "解析ZRV发送的TCP消息中的第%d个报文成功:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", deal_count, ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                            }

                            memset(ZRVDeviceTCPClients[p].strRcvBuff, 0, 1024 * 1024);
                            ZRVDeviceTCPClients[p].iRcvBuffLen = 0;
                            ZRVDeviceTCPClients[p].iRcvBuffLenCount = 0;

                            /* buffer重新赋值 */
                            remainSize = remainSize - (stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN);
                            memset(tmp1_buff, 0, 1024 * 1024);
                            memcpy(tmp1_buff, &tmp_buff[stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN], remainSize);
                            memset(tmp_buff, 0, 1024 * 1024);
                            memcpy(tmp_buff, tmp1_buff, 1024 * 1024);
                        }
                    }
                }
            }

            if (ZRVDeviceTCPClients[p].iRcvBuffLenCount < ZRVDeviceTCPClients[p].iRcvBuffLen) /* 需要继续接收 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "接收到ZRV发送的TCP消息, 没有完全接收完成, 继续接收:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 消息体长度=%d, 已经接收的消息长度=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, ZRVDeviceTCPClients[p].iRcvBuffLenCount);
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "ZRVDeviceLoginServerMain() ZRV Device: IP=%s, Port=%d, Socket=%d, RcvBuffLen=%d, RcvBuffLenCount=%d, Not Recv Complete \r\n", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, clientSock, ZRVDeviceTCPClients[p].iRcvBuffLen, ZRVDeviceTCPClients[p].iRcvBuffLenCount);
                continue;
            }

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "############################################\r\n");
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "ZRVDeviceLoginServerMain() ZRV Device: IP=%s, Port=%d, Socket=%d, RcvBuffLen=%d, RcvBuffLenCount=%d, Recv Complete, Begin Parse, BodyBuff:\r\n%s\r\n\r\n", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, clientSock, ZRVDeviceTCPClients[p].iRcvBuffLen, ZRVDeviceTCPClients[p].iRcvBuffLenCount, ZRVDeviceTCPClients[p].strRcvBuff);
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "############################################\r\n");

            /* 解析处理 */
            iRet = ZRVDeviceParseTCPSocketDataProc(ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].strRcvBuff, ZRVDeviceTCPClients[p].iRcvBuffLen, pDbOper, run_thread_time);

            if (0 != iRet)
            {
                if (-2 == iRet)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "解析ZRV发送的TCP消息失败, 不支持的消息格式:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "解析ZRV发送的TCP消息失败:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "解析ZRV发送的TCP消息成功:ZRV设备IP地址=%s, 端口号=%d, TCP Sock=%d, 收到的消息长度=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "接收的消息内容=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
            }

            memset(ZRVDeviceTCPClients[p].strRcvBuff, 0, 1024 * 1024);
            ZRVDeviceTCPClients[p].iRcvBuffLen = 0;
            ZRVDeviceTCPClients[p].iRcvBuffLenCount = 0;
        }
    }

    return;
}

/*****************************************************************************
 函 数 名  : ShowConnectTCPUser
 功能描述  : 显示ZRV设备连接的TCP
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void ShowConnectTCPZRVDevice(int sock)
{
    int p = 0;
    char strLine[] = "\r-----------------------------------------------------------\r\n";
    char strHead[] = "\rDevice Client Socket  Device Client IP   Device Client Port\r\n";

    char rbuf[256] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    for (p = 0; p < MAX_ZRV_DEVICE_TCP_CLIENTS; p++)
    {
        if (ZRVDeviceTCPClients[p].sock < 0)
        {
            continue;
        }

        snprintf(rbuf, 256, "\r%-20d  %-16s  %-18d\r\n", ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port);

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
 函 数 名  : free_unused_zrv_device_tcp_connect
 功能描述  : 释放掉没有使用的ZRV设备TCP连接
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年4月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void free_unused_zrv_device_tcp_connect()
{
    int p = 0;

    for (p = 0; p < MAX_ZRV_DEVICE_TCP_CLIENTS; p++)
    {
        if (ZRVDeviceTCPClients[p].sock == -1)
        {
            continue;
        }

        if (!is_zrv_device_tcp_socket_in_use(ZRVDeviceTCPClients[p].sock))
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ZRV设备TCP连接没有使用, 主动关闭:TCP Socket=%d", ZRVDeviceTCPClients[p].sock);
            close(ZRVDeviceTCPClients[p].sock);
            zrv_device_tcp_client_init(&ZRVDeviceTCPClients[p]);
        }
    }

    return;
}

void free_zrv_device_tcp_connect_by_socket(int socket)
{
    int p = 0;

    if (socket <= 0)
    {
        return;
    }

    for (p = 0; p < MAX_ZRV_DEVICE_TCP_CLIENTS; p++)
    {
        if (ZRVDeviceTCPClients[p].sock == -1)
        {
            continue;
        }

        if (ZRVDeviceTCPClients[p].sock == socket)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送消息失败, ZRV设备TCP连接异常, 主动关闭:TCP Socket=%d, device ip=%s, errno=%d", ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].login_ip, errno);
            close(ZRVDeviceTCPClients[p].sock);
            zrv_device_tcp_client_init(&ZRVDeviceTCPClients[p]);
        }
    }

    return;
}
#endif

