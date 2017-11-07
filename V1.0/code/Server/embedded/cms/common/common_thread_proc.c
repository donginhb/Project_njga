
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
#include "common/gblconfig_proc.inc"
#include "common/gblfunc_proc.inc"
#include "common/log_proc.inc"
#include "common/common_thread_proc.inc"
#include "common/db_proc.h"
#include "common/systeminfo_proc.inc"

#include "config/telnetd.inc"

#include "user/user_reg_proc.inc"

#include "record/record_srv_proc.inc"

#include "device/device_reg_proc.inc"
#include "device/device_thread_proc.inc"

#include "route/route_info_mgn.inc"
#include "route/route_thread_proc.inc"
#include "route/platform_thread_proc.inc"

#include "service/poll_srv_proc.inc"
#include "service/plan_srv_proc.inc"
#include "service/cruise_srv_proc.inc"
#include "service/alarm_proc.inc"
#include "service/preset_proc.inc"
#include "service/compress_task_proc.inc"

#include "resource/resource_info_mgn.inc"

#include "user/user_thread_proc.inc"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern char g_StrConLog[2][100];
extern char g_StrCon[2][100];

extern int cms_run_status;  /* 0:没有运行,1:正常运行 */
//extern char log_file_time[20];
extern cms_log_t* pGCMSLog;       /* 日志文件记录 */
extern int g_IsSubscribe;         /* 是否发送订阅，默认0 */

extern user_reg_msg_queue g_UserRegMsgQueue;                /* 用户注册消息队列 */
extern user_reg_msg_queue g_UserUnRegMsgQueue;              /* 用户去注册消息队列 */
extern user_log2db_queue g_UserLog2DBQueue;                 /* 用户日志记录到数据库消息队列 */
extern record_srv_msg_queue g_RecordSrvMsgQueue;            /* 录像业务消息队列 */
extern GBDevice_reg_msg_queue g_GBDeviceRegMsgQueue;        /* 标准物理设备注册消息队列 */
extern GBDevice_reg_msg_queue g_GBDeviceUnRegMsgQueue;      /* 标准物理设备注销消息队列 */
extern device_srv_msg_queue g_DeviceSrvMsgQueue;            /* 标准设备业务消息队列 */
extern device_srv_msg_queue g_DeviceMessageSrvMsgQueue;     /* 标准设备Message消息队列 */
extern diagnosis_msg_queue g_DiagnosisMsgQueue;             /* 诊断消息队列 */
extern route_srv_msg_queue g_RouteSrvMsgQueue;              /* 互联路由业务消息队列 */
extern route_srv_msg_queue g_RouteMessageSrvMsgQueue;       /* 互联路由Message业务消息队列 */
extern alarm_msg_queue g_AlarmMsgQueue;                     /* 业务报警消息队列 */
extern fault_msg_queue g_FaultMsgQueue;                     /* 故障报警消息队列 */
extern alarm_duration_list_t* g_AlarmDurationList;          /* 延时报警定时器队列 */
extern ack_send_list_t* g_AckSendList;                      /* Ack 发送消息队列 */
extern transfer_xml_msg_list_t* g_TransferXMLMsgList;       /* 服务器转发的 XML Message 消息队列*/
extern preset_auto_back_list_t* g_PresetAutoBackList;       /* 预置位自动归位队列 */
extern device_auto_unlock_list_t* g_DeviceAutoUnlockList;   /* 点位自动解锁队列 */
extern tsu_creat_task_result_msg_queue  g_TSUCreatTaskResultMsgQueue; /* TSU 通知创建任务结果消息队列 */
extern trace_log2file_queue g_TraceLog2FileQueue;           /* 日志记录文件队列 */
extern sip_msg_log2file_queue g_SIPMsgLog2FileQueue;        /* SIP消息日志记录文件队列 */
extern system_log2db_queue g_SystemLog2DBQueue;             /* 系统日志记录到数据库消息队列 */
extern int g_LogQueryBufferSize;                            /* 日志缓存队列大小，默认MAX_LOG_QUERY_SIZE */

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/
int g_GetChannelFlag = 0;

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
thread_proc_list_t* g_ThreadProcList = NULL;           /* 线程处理队列 */

thread_proc_t* g_Log2FileProcThread = NULL;            /* 记录日志文件处理线程 */
thread_proc_t* g_Log2DBProcThread = NULL;              /* 日志记录到数据库处理线程 */
thread_proc_t* g_ThreadMonitorProcThread = NULL;       /* 线程监控处理线程 */

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#if DECS("线程的公共处理")
/*****************************************************************************
 Prototype    : thread_proc_t_init
 Description  : 处理线程初始化
 Input        : thread_proc_t ** run
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/27
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int thread_proc_t_init(thread_proc_t** run)
{
    *run = (thread_proc_t*)osip_malloc(sizeof(thread_proc_t));

    if (NULL == *run)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "thread_proc_t_init() exit---: *run Smalloc Error \r\n");
        return -1;
    }

    (*run)->eThreadType = THREAD_NULL;
    (*run)->thread = NULL;
    (*run)->th_exit = 0;
    (*run)->run_time = 0;
    (*run)->pDbOper = NULL;
    (*run)->pLogDbOper = NULL;
    (*run)->iLogDBOperConnectStatus = 0;

    return 0;
}

/*****************************************************************************
 Prototype    : thread_proc_t_free
 Description  : 处理线程释放
 Input        : thread_proc_t * run
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/27
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void thread_proc_t_free(thread_proc_t* run)
{
    if (run == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "thread_proc_t_free() exit---: Param Error \r\n");
        return;
    }

    run->eThreadType = THREAD_NULL;

    if (run->thread)
    {
        osip_free(run->thread);
        run->thread = NULL;
    }

    run->run_time = 0;

    if (run->pDbOper != NULL)
    {
        delete run->pDbOper;
        run->pDbOper = NULL;
    }

    if (run->pLogDbOper != NULL)
    {
        delete run->pLogDbOper;
        run->pLogDbOper = NULL;
    }

    run->iLogDBOperConnectStatus = 0;

    return;
}

/*****************************************************************************
 函 数 名  : thread_proc_thread_start
 功能描述  : 处理线程启动
 输入参数  : proc_thread_type_t eThreadType
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月9日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int thread_proc_thread_start(proc_thread_type_t eThreadType)
{
    int i = 0;
    thread_proc_t* ptProcThread = NULL;

    if (eThreadType >= THREAD_NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "thread_proc_thread_start() exit---: Param Error \r\n");
        return -1;
    }

    i = thread_proc_t_init(&ptProcThread);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "thread_proc_thread_start() exit---: Proc Thread Init Error \r\n");
        return -1;
    }

    /* 数据库操作 */
    ptProcThread->pDbOper = new DBOper();

    if (ptProcThread->pDbOper == NULL)
    {
        thread_proc_t_free(ptProcThread);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "thread_proc_thread_start() exit---: DB Oper Error \r\n");
        return -1;
    }

    if (ptProcThread->pDbOper->Connect(g_StrCon, (char*)"") < 0)
    {
        thread_proc_t_free(ptProcThread);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "thread_proc_thread_start() exit---: DB Oper Connect Error \r\n");
        return -1;
    }

    if (THREAD_ZRV_TASK_RESULT_MESSAGE_PROC == eThreadType)
    {
        /* 日志数据库操作 */
        ptProcThread->pLogDbOper = new DBOper();

        if (ptProcThread->pLogDbOper == NULL)
        {
            thread_proc_t_free(ptProcThread);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "thread_proc_thread_start() exit---: Log DB Oper Error \r\n");
            return -1;
        }

        if (ptProcThread->pLogDbOper->Connect(g_StrConLog, (char*)"") < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "thread_proc_thread_start() exit---: Log DB Oper Connect Error \r\n");
            ptProcThread->iLogDBOperConnectStatus = 0;
        }
        else
        {
            ptProcThread->iLogDBOperConnectStatus = 1;
        }
    }

    ptProcThread->eThreadType = eThreadType;
    ptProcThread->run_time = time(NULL);

    switch (eThreadType)
    {
        case THREAD_ZRV_DEVICE_MGN_PROC: /* ZRV设备管理处理线程 */
            ptProcThread->thread = (osip_thread_t*)osip_thread_create(20000, zrv_device_mgn_proc_thread_execute, (void*)ptProcThread);
            break;

        case THREAD_ZRV_DEVICE_TCP_MGN_PROC: /* ZRV设备TCP连接管理处理线程 */
            ptProcThread->thread = (osip_thread_t*)osip_thread_create(20000, zrv_device_tcp_connect_proc_thread_execute, (void*)ptProcThread);
            break;

        case THREAD_ZRV_TASK_RESULT_MESSAGE_PROC: /* ZRV设备任务结果消息上报处理线程 */
            ptProcThread->thread = (osip_thread_t*)osip_thread_create(20000, zrv_task_result_msg_proc_thread_execute, (void*)ptProcThread);
            break;

        case THREAD_COMPRESS_TASK_MONITOR_PROC: /* 压缩任务监控处理线程 */
            ptProcThread->thread = (osip_thread_t*)osip_thread_create(20000, zrv_compress_task_monitor_thread_execute, (void*)ptProcThread);
            break;

        case THREAD_CONFIG_MGN_PROC: /* 配置管理处理线程 */
            ptProcThread->thread = (osip_thread_t*)osip_thread_create(20000, config_mgn_proc_thread_execute, (void*)ptProcThread);
            break;

        case THREAD_TELNET_CLIENT_MONITOR_PROC: /* Telnet 客户端监控处理线程 */
            ptProcThread->thread = (osip_thread_t*)osip_thread_create(20000, telnet_client_monitor_thread_execute, (void*)ptProcThread);
            break;

        case THREAD_COMMON_DB_REFRESH_PROC: /* 公共的数据库刷新处理线程 */
            ptProcThread->thread = (osip_thread_t*)osip_thread_create(20000, common_db_refresh_thread_execute, (void*)ptProcThread);
            break;

        case THREAD_PLATFORM_MGN_PROC: /* 上级平台管理线程 */
            ptProcThread->thread = (osip_thread_t*)osip_thread_create(20000, platform_mgn_proc_thread_execute, (void*)ptProcThread);
            break;

        default:
            thread_proc_t_free(ptProcThread);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "thread_proc_thread_start() exit---: Proc Thread Init Error \r\n");
            return -1;
    }

    if (NULL == ptProcThread->thread)
    {
        thread_proc_t_free(ptProcThread);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "thread_proc_thread_start() exit---: Proc Thread Create Error \r\n");
        return -1;
    }

    /* 添加到监控线程队列 */
    i = thread_proc_add(ptProcThread);

    if (i < 0)
    {
        thread_proc_t_free(ptProcThread);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "thread_proc_thread_start() exit---: Proc Thread Add To Thread List Error \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : thread_proc_thread_stop
 功能描述  : 处理线程停止
 输入参数  : proc_thread_type_t eThreadType
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月9日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void thread_proc_thread_stop(proc_thread_type_t eThreadType)
{
    int i = 0;
    thread_proc_t* ptProcThread = NULL;

    if (eThreadType >= THREAD_NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "thread_proc_thread_stop() exit---: Param Error \r\n");
        return;
    }

    ptProcThread = thread_proc_find(eThreadType);

    if (NULL == ptProcThread)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "thread_proc_thread_stop() exit---: Thread Proc Find Error \r\n");
        return;
    }

    ptProcThread->th_exit = 1;

    if (ptProcThread->thread != NULL)
    {
        i = osip_thread_join((struct osip_thread*)ptProcThread->thread);
    }

    /* 从监控线程队列移除 */
    i = thread_proc_remove(eThreadType);

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "thread_proc_thread_stop() exit---: Thread Proc Thread Remove From Thread List Error \r\n");
    }

    thread_proc_t_free(ptProcThread);
    osip_free(ptProcThread);
    ptProcThread = NULL;
    return;
}

/*****************************************************************************
 函 数 名  : thread_proc_thread_start_all
 功能描述  : 启动所有业务处理线程
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
int thread_proc_thread_start_all()
{
    int i = 0;

    /* 设备管理处理线程 */
    i = thread_proc_thread_start(THREAD_ZRV_DEVICE_MGN_PROC);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "thread_proc_thread_start_all() exit---: thread_proc_thread_start THREAD_ZRV_DEVICE_MGN_PROC Error \r\n");
        return -1;
    }

    i = thread_proc_thread_start(THREAD_ZRV_DEVICE_TCP_MGN_PROC);

    if (i != 0)
    {
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_MGN_PROC);

        DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "thread_proc_thread_start_all() exit---: thread_proc_thread_start THREAD_ZRV_DEVICE_TCP_MGN_PROC Error \r\n");
        return -1;
    }

    /*  TSU消息处理线程 */
    i = thread_proc_thread_start(THREAD_ZRV_TASK_RESULT_MESSAGE_PROC);

    if (i != 0)
    {
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_MGN_PROC);
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_TCP_MGN_PROC);

        DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "thread_proc_thread_start_all() exit---: thread_proc_thread_start THREAD_ZRV_TASK_RESULT_MESSAGE_PROC Error \r\n");
        return -1;
    }

    /* TSU注册处理线程 */
    i = thread_proc_thread_start(THREAD_COMPRESS_TASK_MONITOR_PROC);

    if (i != 0)
    {
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_MGN_PROC);
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_TCP_MGN_PROC);
        thread_proc_thread_stop(THREAD_ZRV_TASK_RESULT_MESSAGE_PROC);

        DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "thread_proc_thread_start_all() exit---: thread_proc_thread_start THREAD_COMPRESS_TASK_MONITOR_PROC Error \r\n");
        return -1;
    }

    /* 配置管理处理线程 */
    i = thread_proc_thread_start(THREAD_CONFIG_MGN_PROC);

    if (i != 0)
    {
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_MGN_PROC);
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_TCP_MGN_PROC);
        thread_proc_thread_stop(THREAD_ZRV_TASK_RESULT_MESSAGE_PROC);
        thread_proc_thread_stop(THREAD_COMPRESS_TASK_MONITOR_PROC);

        DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "thread_proc_thread_start_all() exit---: thread_proc_thread_start THREAD_COMPRESS_TASK_MONITOR_PROC Error \r\n");
        return -1;
    }

    /* Telnet客户端监视处理线程 */
    i = thread_proc_thread_start(THREAD_TELNET_CLIENT_MONITOR_PROC);

    if (i != 0)
    {
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_MGN_PROC);
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_TCP_MGN_PROC);
        thread_proc_thread_stop(THREAD_ZRV_TASK_RESULT_MESSAGE_PROC);
        thread_proc_thread_stop(THREAD_COMPRESS_TASK_MONITOR_PROC);
        thread_proc_thread_stop(THREAD_CONFIG_MGN_PROC);

        DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "thread_proc_thread_start_all() exit---: thread_proc_thread_start THREAD_COMPRESS_TASK_MONITOR_PROC Error \r\n");
        return -1;
    }

    /* 公共的数据库刷新处理线程 */
    i = thread_proc_thread_start(THREAD_COMMON_DB_REFRESH_PROC);

    if (i != 0)
    {
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_MGN_PROC);
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_TCP_MGN_PROC);
        thread_proc_thread_stop(THREAD_ZRV_TASK_RESULT_MESSAGE_PROC);
        thread_proc_thread_stop(THREAD_COMPRESS_TASK_MONITOR_PROC);
        thread_proc_thread_stop(THREAD_CONFIG_MGN_PROC);
        thread_proc_thread_stop(THREAD_TELNET_CLIENT_MONITOR_PROC);

        DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "thread_proc_thread_start_all() exit---: thread_proc_thread_start THREAD_COMMON_DB_REFRESH_PROC Error \r\n");
        return -1;
    }

    /* 上级平台管理处理线程 */
    i = thread_proc_thread_start(THREAD_PLATFORM_MGN_PROC);

    if (i != 0)
    {
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_MGN_PROC);
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_TCP_MGN_PROC);
        thread_proc_thread_stop(THREAD_ZRV_TASK_RESULT_MESSAGE_PROC);
        thread_proc_thread_stop(THREAD_COMPRESS_TASK_MONITOR_PROC);
        thread_proc_thread_stop(THREAD_CONFIG_MGN_PROC);
        thread_proc_thread_stop(THREAD_TELNET_CLIENT_MONITOR_PROC);
        thread_proc_thread_stop(THREAD_COMMON_DB_REFRESH_PROC);

        DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "thread_proc_thread_start_all() exit---: thread_proc_thread_start THREAD_PLATFORM_MGN_PROC Error \r\n");
        return -1;
    }

    return i;
}

/*****************************************************************************
 函 数 名  : thread_proc_thread_stop_all
 功能描述  : 停止所有业务处理线程
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
void thread_proc_thread_stop_all()
{
    thread_proc_thread_stop(THREAD_ZRV_DEVICE_MGN_PROC);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "thread_proc_thread_stop_all() THREAD_ZRV_DEVICE_MGN_PROC Stop OK \r\n");

    thread_proc_thread_stop(THREAD_ZRV_DEVICE_TCP_MGN_PROC);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "thread_proc_thread_stop_all() THREAD_ZRV_DEVICE_TCP_MGN_PROC Stop OK \r\n");

    thread_proc_thread_stop(THREAD_ZRV_TASK_RESULT_MESSAGE_PROC);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "thread_proc_thread_stop_all() THREAD_ZRV_TASK_RESULT_MESSAGE_PROC Stop OK \r\n");

    thread_proc_thread_stop(THREAD_COMPRESS_TASK_MONITOR_PROC);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "thread_proc_thread_stop_all() THREAD_COMPRESS_TASK_MONITOR_PROC Stop OK \r\n");

    thread_proc_thread_stop(THREAD_CONFIG_MGN_PROC);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "thread_proc_thread_stop_all() THREAD_CONFIG_MGN_PROC Stop OK \r\n");

    thread_proc_thread_stop(THREAD_TELNET_CLIENT_MONITOR_PROC);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "thread_proc_thread_stop_all() THREAD_TELNET_CLIENT_MONITOR_PROC Stop OK \r\n");

    thread_proc_thread_stop(THREAD_COMMON_DB_REFRESH_PROC);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "thread_proc_thread_stop_all() THREAD_COMMON_DB_REFRESH_PROC Stop OK \r\n");

    thread_proc_thread_stop(THREAD_PLATFORM_MGN_PROC);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "thread_proc_thread_stop_all() THREAD_PLATFORM_MGN_PROC Stop OK \r\n");
}
#endif

#if DECS("写日志文件线程")
/*****************************************************************************
 Prototype    : log2file_proc_thread_execute
 Description  : 写日志文件运行主线程
 Input        : void * p
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/27
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void* log2file_proc_thread_execute(void* p)
{
    //int iSChange = 0;
    //int iSDelete = 0;
    thread_proc_t* run = (thread_proc_t*)p;
    //time_t now = 0;
    //struct tm tp = {0};
    //char filename[256] = {0};
    //char strDelCommond[256] = {0};
    static int check_interval = 0;

    if (run == NULL)
    {
        return NULL;
    }

    while (!run->th_exit)
    {
#if 0
        now = time(NULL);
        localtime_r(&now, &tp);

        /* 新生成日志文件 */
        if (tp.tm_min == 0 && tp.tm_sec == 0)
        {
            if (!iSChange)
            {
                memset(log_file_time, 0, 20);
                strftime(log_file_time, 20, "%Y_%m_%d_%H", &tp);

                //SIP消息
                memset(filename, 0, 256);
                snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_SIP_MSG, log_file_time);

                if (NULL != pGCMSLog->sip_msglog->logfile)
                {
                    fclose(pGCMSLog->sip_msglog->logfile);
                    pGCMSLog->sip_msglog->logfile = NULL;
                }

                pGCMSLog->sip_msglog->logfile = fopen(filename, "w+");

                //SIP错误消息
                memset(filename, 0, 256);
                snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_SIP_ERRORMSG, log_file_time);

                if (NULL != pGCMSLog->sip_errorlog->logfile)
                {
                    fclose(pGCMSLog->sip_errorlog->logfile);
                    pGCMSLog->sip_errorlog->logfile = NULL;
                }

                pGCMSLog->sip_errorlog->logfile = fopen(filename, "w+");

                //Debug 调试信息
                memset(filename, 0, 256);
                snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_DEBUG, log_file_time);

                if (NULL != pGCMSLog->debug_log->logfile)
                {
                    fclose(pGCMSLog->debug_log->logfile);
                    pGCMSLog->debug_log->logfile = NULL;
                }

                pGCMSLog->debug_log->logfile = fopen(filename, "w+");

                //运行信息
                memset(filename, 0, 256);
                snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_RUN, log_file_time);

                if (NULL != pGCMSLog->run_log->logfile)
                {
                    fclose(pGCMSLog->run_log->logfile);
                    pGCMSLog->run_log->logfile = NULL;
                }

                pGCMSLog->run_log->logfile = fopen(filename, "w+");

                iSChange = 1;
                //ListDirFile(LOGFILE_DIR, 2);
            }
        }
        else if (tp.tm_min == 0 && tp.tm_sec == 1)
        {
            iSChange = 0;
        }

        /* 删除老的日志文件 */
        if (tp.tm_hour == 0 && tp.tm_min == 0 && tp.tm_sec == 0)
        {
            if (!iSDelete)
            {
                //DeleteOutDateFile(tp.tm_year, tp.tm_mon, tp.tm_mday, tp.tm_hour);
                memset(strDelCommond, 0, 256);
                sprintf(strDelCommond, "find %s -mtime +1 -type f | xargs rm -rf", LOGFILE_DIR);
                system(strDelCommond);
                //printf("\r\n DeleteOutDateFile:=%s \r\n", strDelCommond);
                iSDelete = 1;
            }
        }
        else if (tp.tm_hour == 0 && tp.tm_min == 0 && tp.tm_sec == 1)
        {
            iSDelete = 0;
        }

#endif

        if (g_LogQueryBufferSize > 0) /* 如果大于0,10分钟写一次文件 */
        {
            check_interval++;

            if (check_interval >= 600)
            {
                scan_trace_log2file_list();
                scan_sip_msg_log2file_list();

                check_interval = 0;
            }
        }
        else  /* 1秒写一次文件 */
        {
            scan_trace_log2file_list();
            scan_sip_msg_log2file_list();

            check_interval = 0;
        }

        run->run_time = time(NULL);
        osip_usleep(1000000);
    }

    return NULL;
}

/*****************************************************************************
 Prototype    : log2file_proc_thread_start
 Description  : 启动写日志文件线程
 Input        :  None
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/27
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int log2file_proc_thread_start()
{
    int i = 0;
    i = thread_proc_t_init(&g_Log2FileProcThread);

    if (i != 0)
    {
        return -1;
    }

    g_Log2FileProcThread->thread = (osip_thread_t*)osip_thread_create(20000, log2file_proc_thread_execute, (void*)g_Log2FileProcThread);

    if (g_Log2FileProcThread->thread == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "log2file_proc_thread_start() exit---: Log2File Proc Thread Create Error \r\n");
        thread_proc_t_free(g_Log2FileProcThread);
        return -1;
    }

    g_Log2FileProcThread->run_time = time(NULL);

    return 0;
}

/*****************************************************************************
 Prototype    : log2file_proc_thread_stop
 Description  : 停止写日志文件线程
 Input        :  None
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/27
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void  log2file_proc_thread_stop()
{
    int i = 0;

    if (g_Log2FileProcThread == NULL)
    {
        return;
    }

    g_Log2FileProcThread->th_exit = 1;

    if (g_Log2FileProcThread->thread != NULL)
    {
        i = osip_thread_join((struct osip_thread*)g_Log2FileProcThread->thread);
    }

    thread_proc_t_free(g_Log2FileProcThread);
    osip_free(g_Log2FileProcThread);
    g_Log2FileProcThread = NULL;
    return;
}
#endif

#if DECS("日志记录到数据库线程")
/*****************************************************************************
 Prototype    : log2db_proc_thread_execute
 Description  : 写日志文件运行主线程
 Input        : void * p
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/27
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void* log2db_proc_thread_execute(void* p)
{
    thread_proc_t* run = (thread_proc_t*)p;
    static int check_interval = 0;
    static int clean_flag = 0;

    if (run == NULL)
    {
        return NULL;
    }

    while (!run->th_exit)
    {
        if (1 == cms_run_status)
        {
            if (1 == clean_flag)
            {
                clean_flag = 0;
            }

            if (g_LogQueryBufferSize > 0) /* 如果大于0,10分钟写一次文件 */
            {
                check_interval++;

                if (check_interval >= 600)
                {
                    scan_system_log2db_list(run);

                    check_interval = 0;
                }
            }
            else  /* 1秒写一次文件 */
            {
                scan_system_log2db_list(run);

                check_interval = 0;
            }
        }
        else
        {
            osip_usleep(10000000);

            if (0 == clean_flag)
            {
                clean_flag = 1;
                system_log2db_list_clean();
            }
        }

        run->run_time = time(NULL);
        osip_usleep(100000); /* 100 毫秒 */
    }

    return NULL;
}

/*****************************************************************************
 Prototype    : log2db_proc_thread_start
 Description  : 启动写日志文件线程
 Input        :  None
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2014/3/26
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int log2db_proc_thread_start()
{
    int i = 0;
    i = thread_proc_t_init(&g_Log2DBProcThread);

    if (i != 0)
    {
        return -1;
    }

    /* 数据库操作 */
    g_Log2DBProcThread->pDbOper = new DBOper();

    if (g_Log2DBProcThread->pDbOper == NULL)
    {
        thread_proc_t_free(g_Log2DBProcThread);
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "log2db_proc_thread_start() exit---: DB Oper Error \r\n");
        return -1;
    }

    if (g_Log2DBProcThread->pDbOper->Connect(g_StrCon, (char*)"") < 0)
    {
        thread_proc_t_free(g_Log2DBProcThread);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "log2db_proc_thread_start() exit---: DB Oper Connect Error \r\n");
        return -1;
    }

    /* 日志数据库操作 */
    g_Log2DBProcThread->pLogDbOper = new DBOper();

    if (g_Log2DBProcThread->pLogDbOper == NULL)
    {
        thread_proc_t_free(g_Log2DBProcThread);
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "log2db_proc_thread_start() exit---: Log DB Oper Error \r\n");
        return -1;
    }

    if (g_Log2DBProcThread->pLogDbOper->Connect(g_StrConLog, (char*)"") < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "log2db_proc_thread_start() exit---: Log DB Oper Connect Error \r\n");
        g_Log2DBProcThread->iLogDBOperConnectStatus = 0;
    }
    else
    {
        g_Log2DBProcThread->iLogDBOperConnectStatus = 1;
    }

    g_Log2DBProcThread->thread = (osip_thread_t*)osip_thread_create(20000, log2db_proc_thread_execute, (void*)g_Log2DBProcThread);

    if (g_Log2DBProcThread->thread == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "log2db_proc_thread_start() exit---: Log2DB Proc Thread Create Error \r\n");
        thread_proc_t_free(g_Log2DBProcThread);
        return -1;
    }

    g_Log2DBProcThread->run_time = time(NULL);

    return 0;
}

/*****************************************************************************
 Prototype    : log2db_proc_thread_stop
 Description  : 停止日志记录到数据库线程
 Input        :  None
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/27
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void  log2db_proc_thread_stop()
{
    int i = 0;

    if (g_Log2DBProcThread == NULL)
    {
        return;
    }

    g_Log2DBProcThread->th_exit = 1;

    if (g_Log2DBProcThread->thread != NULL)
    {
        i = osip_thread_join((struct osip_thread*)g_Log2DBProcThread->thread);
    }

    thread_proc_t_free(g_Log2DBProcThread);
    osip_free(g_Log2DBProcThread);
    g_Log2DBProcThread = NULL;
    return;
}
#endif

#if DECS("线程监控线程处理")
/*****************************************************************************
 函 数 名  : thread_proc_thread_start
 功能描述  : 启动处理线程
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月8日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int thread_monitor_proc_thread_start()
{
    int i;

    i = thread_proc_t_init(&g_ThreadMonitorProcThread);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "thread_monitor_proc_thread_start() exit---: Thread Monitor Proc Thread Init Error \r\n");
        return -1;
    }

    g_ThreadMonitorProcThread->thread = (osip_thread_t*)osip_thread_create(20000, thread_monitor_proc_thread_execute, (void*)g_ThreadMonitorProcThread);

    if (g_ThreadMonitorProcThread->thread == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "thread_monitor_proc_thread_start() exit---: Thread Monitor Proc Thread Create Error \r\n");
        thread_proc_t_free(g_ThreadMonitorProcThread);
        return -1;
    }

    g_ThreadMonitorProcThread->run_time = time(NULL);

    return 0;
}

/*****************************************************************************
 函 数 名  : thread_proc_thread_stop
 功能描述  : 停止处理线程
 输入参数  : thread_proc_t* run
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月8日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void  thread_monitor_proc_thread_stop()
{
    int i = 0;

    if (g_ThreadMonitorProcThread == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "thread_monitor_proc_thread_stop() exit---: Param Error \r\n");
        return;
    }

    g_ThreadMonitorProcThread->th_exit = 1;

    if (g_ThreadMonitorProcThread->thread != NULL)
    {
        i = osip_thread_join((struct osip_thread*)g_ThreadMonitorProcThread->thread);
    }

    thread_proc_t_free(g_ThreadMonitorProcThread);
    osip_free(g_ThreadMonitorProcThread);
    g_ThreadMonitorProcThread = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : thread_monitor_proc_thread_execute
 功能描述  : 监控线程处理函数
 输入参数  : void* p
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月8日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void* thread_monitor_proc_thread_execute(void* p)
{
    int i = 0;
    int iVMem = 0;
    thread_proc_t* run = (thread_proc_t*)p;
    process_mem_info proc_mem;
    time_t now;
    struct tm p_tm = { 0 };

#if 0
    struct tm
    {
        int tm_sec;                   /* Seconds.     [0-60] (1 leap second) */
        int tm_min;                   /* Minutes.     [0-59] */
        int tm_hour;                  /* Hours.       [0-23] */
        int tm_mday;                  /* Day.         [1-31] */
        int tm_mon;                   /* Month.       [0-11] */
        int tm_year;                  /* Year - 1900.  */
        int tm_wday;                  /* Day of week. [0-6] */
        int tm_yday;                  /* Days in year.[0-365] */
        int tm_isdst;                 /* DST.         [-1/0/1]*/

#ifdef  __USE_BSD
        long int tm_gmtoff;           /* Seconds east of UTC.  */
        __const char *tm_zone;        /* Timezone abbreviation.  */
#else
        long int __tm_gmtoff;         /* Seconds east of UTC.  */
        __const char *__tm_zone;      /* Timezone abbreviation.  */
#endif
    };
#endif

    if (run == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "thread_monitor_proc_thread_execute() exit---: Param Error \r\n");
        return NULL;
    }

    while (!run->th_exit)
    {
        if (1 == cms_run_status)
        {
            /* 每五秒扫描线程队列 */
            scan_thread_proc_list();

            /* 上级平台业务处理队列 */
            scan_platform_srv_proc_thread_list();

            /* 监视CMS的虚拟内存 */
            memset(&proc_mem, 0, sizeof(process_mem_info));
            i = get_progress_memory_usage((char*)"cms", &proc_mem);
            iVMem = osip_atoi(proc_mem.vmem);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "thread_monitor_proc_thread_execute(): CMS Phy Mem=%s, V Mem=%s, iVMem=%d \r\n", proc_mem.mem, proc_mem.vmem, iVMem);

            if (iVMem >= 4194304)/* CMS内存大于4G的时候，重启进程 */
            {
                now = time(NULL);
                localtime_r(&now, &p_tm);

                if (p_tm.tm_hour >= 1 && p_tm.tm_hour <= 2) /* 凌晨一点到两点之间重启 */
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "CMS进程内存大于4G, CMS自动重启, CMS物理内存=%s, CMS虚拟内存=%s", proc_mem.mem, proc_mem.vmem);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "CMSprocess memory more than2G, CMS auto restart, CMS physical memory=%s, CMS Virtual memory=%s", proc_mem.mem, proc_mem.vmem);
                    DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "thread_monitor_proc_thread_execute(): CMS Auto Restart\r\n");
                    osip_usleep(5000000);
                    system((char*)"killall cms; killall -9 cms");
                }
            }
        }

        osip_usleep(20000000);
    }

    return NULL;
}
#endif

#if DECS("公共处理线程队列处理")
/*****************************************************************************
 函 数 名  : thread_proc_list_init
 功能描述  : 线程处理队列初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月8日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int thread_proc_list_init()
{
    g_ThreadProcList = (thread_proc_list_t*)osip_malloc(sizeof(thread_proc_list_t));

    if (g_ThreadProcList == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "thread_proc_list_init() exit---: g_ThreadProcList Smalloc Error \r\n");
        return -1;
    }

    g_ThreadProcList->pThreadProcList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_ThreadProcList->pThreadProcList)
    {
        osip_free(g_ThreadProcList);
        g_ThreadProcList = NULL;
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "thread_proc_list_init() exit---: Thread Proc List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_ThreadProcList->pThreadProcList);

#ifdef MULTI_THR
    /* init smutex */
    g_ThreadProcList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_ThreadProcList->lock)
    {
        osip_free(g_ThreadProcList->pThreadProcList);
        g_ThreadProcList->pThreadProcList = NULL;
        osip_free(g_ThreadProcList);
        g_ThreadProcList = NULL;
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "thread_proc_list_init() exit---: Thread Proc List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : thread_proc_list_free
 功能描述  : 线程处理队列释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月8日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void thread_proc_list_free()
{
    if (NULL == g_ThreadProcList)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "thread_proc_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_ThreadProcList->pThreadProcList)
    {
        while (!osip_list_eol(g_ThreadProcList->pThreadProcList, 0))
        {
            osip_list_remove(g_ThreadProcList->pThreadProcList, 0);
        }

        osip_free(g_ThreadProcList->pThreadProcList);
        g_ThreadProcList->pThreadProcList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_ThreadProcList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_ThreadProcList->lock);
        g_ThreadProcList->lock = NULL;
    }

#endif
    osip_free(g_ThreadProcList);
    g_ThreadProcList = NULL;
    return;
}

/*****************************************************************************
 函 数 名  : thread_proc_list_lock
 功能描述  : 线程处理队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月8日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int thread_proc_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_ThreadProcList == NULL || g_ThreadProcList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "thread_proc_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_ThreadProcList->lock);
#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : thread_proc_list_unlock
 功能描述  : 线程处理队列解锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月8日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int thread_proc_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_ThreadProcList == NULL || g_ThreadProcList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "thread_proc_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_ThreadProcList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_thread_proc_list_lock
 功能描述  : 线程处理队列锁定
 输入参数  : const char* file
             int line
             const char* func
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月8日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int debug_thread_proc_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_ThreadProcList == NULL || g_ThreadProcList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "debug_thread_proc_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_ThreadProcList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_thread_proc_list_unlock
 功能描述  : 线程处理队列解锁
 输入参数  : const char* file
             int line
             const char* func
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月8日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int debug_thread_proc_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_ThreadProcList == NULL || g_ThreadProcList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "debug_thread_proc_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_ThreadProcList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : thread_proc_find
 功能描述  : 处理线程查找
 输入参数  : proc_thread_type_t eThreadType
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月9日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
thread_proc_t* thread_proc_find(proc_thread_type_t eThreadType)
{
    int i = 0;
    thread_proc_t* pTmpThreadProc = NULL;

    if (eThreadType >= THREAD_NULL || NULL == g_ThreadProcList || NULL == g_ThreadProcList->pThreadProcList)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "thread_proc_find() exit---: Thread Proc List Error \r\n");
        return NULL;
    }

    THREAD_PROC_SMUTEX_LOCK();

    for (i = 0; i < osip_list_size(g_ThreadProcList->pThreadProcList); i++)
    {
        pTmpThreadProc = (thread_proc_t*)osip_list_get(g_ThreadProcList->pThreadProcList, i);

        if (NULL == pTmpThreadProc || NULL == pTmpThreadProc->thread)
        {
            continue;
        }

        if (pTmpThreadProc->eThreadType == eThreadType)
        {
            THREAD_PROC_SMUTEX_UNLOCK();
            return pTmpThreadProc;
        }
    }

    THREAD_PROC_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : thread_proc_add
 功能描述  : 添加到线程处理队列
 输入参数  : thread_proc_t* pThreadProc
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月8日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int thread_proc_add(thread_proc_t* pThreadProc)
{
    int i = 0;

    if (pThreadProc == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "thread_proc_add() exit---: Param Error \r\n");
        return -1;
    }

    THREAD_PROC_SMUTEX_LOCK();

    i = osip_list_add(g_ThreadProcList->pThreadProcList, pThreadProc, -1); /* add to list tail */

    if (i < 0)
    {
        THREAD_PROC_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "thread_proc_add() exit---: List Add Error \r\n");
        return -1;
    }

    THREAD_PROC_SMUTEX_UNLOCK();
    return i - 1;
}

/*****************************************************************************
 函 数 名  : thread_proc_remove
 功能描述  : 从线程处理队列移除
 输入参数  : thread_proc_t* pThreadProc
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月8日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int thread_proc_remove(proc_thread_type_t eThreadType)
{
    int i = 0;
    thread_proc_t* pTmpThreadProc = NULL;

    if (eThreadType >= THREAD_NULL || NULL == g_ThreadProcList || NULL == g_ThreadProcList->pThreadProcList)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "thread_proc_find() exit---: Thread Proc List Error \r\n");
        return -1;
    }

    THREAD_PROC_SMUTEX_LOCK();

    for (i = 0; i < osip_list_size(g_ThreadProcList->pThreadProcList); i++)
    {
        pTmpThreadProc = (thread_proc_t*)osip_list_get(g_ThreadProcList->pThreadProcList, i);

        if (NULL == pTmpThreadProc || NULL == pTmpThreadProc->thread)
        {
            continue;
        }

        if (pTmpThreadProc->eThreadType == eThreadType)
        {
            osip_list_remove(g_ThreadProcList->pThreadProcList, i);
            THREAD_PROC_SMUTEX_UNLOCK();
            return i;
        }
    }

    THREAD_PROC_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : scan_thread_proc_list
 功能描述  : 扫描线程处理队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月8日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void scan_thread_proc_list()
{
    int i = 0;
    int iRet = 0;
    thread_proc_t* pThreadProc = NULL;
    needtoproc_threadproc_queue needToProc;
    time_t now = time(NULL);
    proc_thread_type_t eThreadType = THREAD_NULL;     /* 线程类型 */
    char strRunTime[64] = {0};
    char strThreadName[128] = {0};
    int iMsgSize = 0;
    char strCMD[1024] = {0};

    if ((NULL == g_ThreadProcList) || (NULL == g_ThreadProcList->pThreadProcList))
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "scan_thread_proc_list() exit---: Param Error \r\n");
        return;
    }

    needToProc.clear();

    THREAD_PROC_SMUTEX_LOCK();

    if (osip_list_size(g_ThreadProcList->pThreadProcList) <= 0)
    {
        THREAD_PROC_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_ThreadProcList->pThreadProcList); i++)
    {
        pThreadProc = (thread_proc_t*)osip_list_get(g_ThreadProcList->pThreadProcList, i);

        if (NULL == pThreadProc)
        {
            continue;
        }

        if (1 == pThreadProc->th_exit)
        {
            continue;
        }

        if (pThreadProc->run_time < now && now - pThreadProc->run_time > 300)
        {
            needToProc.push_back(pThreadProc);
        }
        else
        {
            //DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_thread_proc_list(): srv_proc_thread Running OK: Thread Type=%d, run_time=%d \r\n", pThreadProc->eThreadType, pThreadProc->run_time);
        }
    }

    THREAD_PROC_SMUTEX_UNLOCK();

    /* 处理需要开始的 */
    while (!needToProc.empty())
    {
        pThreadProc = (thread_proc_t*) needToProc.front();
        needToProc.pop_front();

        if (NULL != pThreadProc)
        {
            eThreadType = pThreadProc->eThreadType;

            //thread_proc_thread_stop(eThreadType);
            //thread_proc_thread_start(eThreadType);

            iRet = get_thread_name_by_type(eThreadType, strThreadName, &iMsgSize);
            iRet = format_time(pThreadProc->run_time, strRunTime);

            snprintf(strCMD, 1024, "%s 线程监控线程检测到挂死线程, Thread Type=%d, 线程名称=%s, 线程最后运行时间=%s, 线程的消息队列长度=%d ,程序自动重启", (char*)"/app/cms_log.sh", eThreadType, strThreadName, strRunTime, iMsgSize);
            system(strCMD);

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "线程监控线程检测到挂死线程, Thread Type=%d, 线程名称=%s, 线程最后运行时间=%s, 线程的消息队列长度=%d ,程序自动重启", eThreadType, strThreadName, strRunTime, iMsgSize);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Thread monitoring thread find hanging thread, Thread Type=%d, Thread Last Run Time=%s, Thread Message Queue Size=%d, cms auto restart", eThreadType, strRunTime, iMsgSize);
            DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "scan_thread_proc_list(): srv_proc_thread:Thread Type=%d cms auto restart\r\n", eThreadType);

            osip_usleep(5000000);
            system((char*)"killall cms; killall -9 cms");
        }
    }

    needToProc.clear();

    return;
}

/*****************************************************************************
 函 数 名  : show_system_proc_thread
 功能描述  : 显示系统线程
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月7日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void show_system_proc_thread(int sock)
{
    int i = 0;
    int iRet = 0;
    char strLine[] = "\r-------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rThread Index  Thread TID  Thread Name                                      Run Time            SizeOfMsgQueue\r\n";
    char rbuf[256] = {0};
    thread_proc_t* pThreadProc = NULL;
    char strRunTime[64] = {0};
    char strThreadName[128] = {0};
    int iMsgSize = 0;

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    /* 业务线程 */
    THREAD_PROC_SMUTEX_LOCK();

    if ((NULL == g_ThreadProcList) || (NULL == g_ThreadProcList->pThreadProcList))
    {
        THREAD_PROC_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_ThreadProcList->pThreadProcList); i++)
    {
        pThreadProc = (thread_proc_t*)osip_list_get(g_ThreadProcList->pThreadProcList, i);

        if (NULL == pThreadProc)
        {
            continue;
        }

        iRet = format_time(pThreadProc->run_time, strRunTime);

        iRet = get_thread_name_by_type(pThreadProc->eThreadType, strThreadName, &iMsgSize);

        snprintf(rbuf, 256, "\r%-12u  %-10u  %-48s %-19s %-14d\r\n", i, *(pThreadProc->thread), strThreadName, strRunTime, iMsgSize);

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    THREAD_PROC_SMUTEX_UNLOCK();

    /* 系统文件日志线程 */
    iRet = format_time(g_Log2FileProcThread->run_time, strRunTime);

    snprintf(rbuf, 256, "\r%-12u  %-10u  %-48s %-19s %-14d\r\n", osip_list_size(g_ThreadProcList->pThreadProcList), *(g_Log2FileProcThread->thread), "系统文件日志线程", strRunTime, g_TraceLog2FileQueue.size());

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* 系统数据库日志线程 */
    iRet = format_time(g_Log2DBProcThread->run_time, strRunTime);

    snprintf(rbuf, 256, "\r%-12u  %-10u  %-48s %-19s %-14d\r\n", osip_list_size(g_ThreadProcList->pThreadProcList) + 2, *(g_Log2DBProcThread->thread), "系统数据库日志线程", strRunTime, g_SystemLog2DBQueue.size());

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

/*****************************************************************************
 函 数 名  : get_thread_name_by_type
 功能描述  : 根据线程类型获取线程名称
 输入参数  : proc_thread_type_t eThreadType
             char* strThreadName
             int* pMsgSize
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月7日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int get_thread_name_by_type(proc_thread_type_t eThreadType, char* strThreadName, int* pMsgSize)
{
    if (NULL == strThreadName)
    {
        return -1;
    }

    switch (eThreadType)
    {
        case THREAD_ZRV_DEVICE_MGN_PROC: /* ZRV设备管理处理线程 */
            snprintf(strThreadName, 128, "%s", (char*)"ZRV设备管理处理线程");
            *pMsgSize = 0;
            break;

        case THREAD_ZRV_DEVICE_TCP_MGN_PROC: /* ZRV设备TCP连接管理处理线程 */
            snprintf(strThreadName, 128, "%s", (char*)"ZRV设备TCP连接管理处理线程");
            *pMsgSize = 0;
            break;

        case THREAD_ZRV_TASK_RESULT_MESSAGE_PROC: /* ZRV设备结果消息处理线程 */
            snprintf(strThreadName, 128, "%s", (char*)"ZRV设备结果消息处理线程");
            *pMsgSize = g_TSUCreatTaskResultMsgQueue.size();
            break;

        case THREAD_COMPRESS_TASK_MONITOR_PROC: /* 压缩任务监控处理线程 */
            snprintf(strThreadName, 128, "%s", (char*)"压缩任务监控处理线程");
            *pMsgSize = 0;
            break;

        case THREAD_CONFIG_MGN_PROC: /* 配置管理处理线程 */
            snprintf(strThreadName, 128, "%s", (char*)"配置管理处理线程");
            *pMsgSize = 0;
            break;

        case THREAD_TELNET_CLIENT_MONITOR_PROC: /* Telnet 客户端监控处理线程 */
            snprintf(strThreadName, 128, "%s", (char*)"Telnet 客户端监控处理线程");
            *pMsgSize = 0;
            break;

        case THREAD_COMMON_DB_REFRESH_PROC: /* 公共的数据库刷新处理线程 */
            snprintf(strThreadName, 128, "%s", (char*)"公共的数据库刷新处理线程");
            *pMsgSize = 0;
            break;

        case THREAD_PLATFORM_MGN_PROC: /* 上级平台管理处理线程 */
            snprintf(strThreadName, 128, "%s", (char*)"上级平台管理处理线程");
            *pMsgSize = 0;
            break;

        default:
            return -1;
    }

    return 0;
}
#endif

#if DECS("ZRV设备管理处理")
/*****************************************************************************
 Prototype    : zrv_device_mgn_proc_thread_execute
 Description  : 设备管理处理运行主线程
 Input        : void * p
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/5/30
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void* zrv_device_mgn_proc_thread_execute(void* p)
{
    thread_proc_t* run = (thread_proc_t*)p;
    static int check_expires_interval = 0;
    static int check_db_interval = 0;

    if (run == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "zrv_device_mgn_proc_thread_execute() exit---: Param Error \r\n");
        return NULL;
    }

    while (!run->th_exit)
    {
        if (1 == cms_run_status)
        {
            scan_ZRVDevice_info_list_for_expires();
        }

        run->run_time = time(NULL);
        osip_usleep(1000000);
    }

    return NULL;
}
#endif

#if DECS("ZRVTCP连接管理处理")
void* zrv_device_tcp_connect_proc_thread_execute(void* p)
{
    thread_proc_t* run = (thread_proc_t*)p;

    if (run == NULL)
    {
        return NULL;
    }

    while (!run->th_exit)
    {
        if (1 == cms_run_status)
        {
            ZRVDeviceLoginServerMain(run->pDbOper, (int*)&(run->run_time));
        }

        run->run_time = time(NULL);
        osip_usleep(5000);
    }

    return NULL;
}
#endif

#if DECS("ZRV设备任务结果消息上报处理线")
/*****************************************************************************
 Prototype    : zrv_task_result_msg_proc_thread_execute
 Description  : ZRV设备任务结果消息上报处理线程
 Input        : void * p
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/11/13
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void* zrv_task_result_msg_proc_thread_execute(void* p)
{
    thread_proc_t* run = (thread_proc_t*)p;
    static int clean_flag = 0;

    if (run == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "zrv_task_result_msg_proc_thread_execute() exit---: Param Error \r\n");
        return NULL;
    }

    while (!run->th_exit)
    {
        if (1 == cms_run_status)
        {
            if (1 == clean_flag)
            {
                clean_flag = 0;
            }

            /* 扫描TSU注册消息队列*/
            scan_tsu_creat_task_result_msg_list(run->pDbOper);
        }
        else
        {
            osip_usleep(10000000);

            if (0 == clean_flag)
            {
                clean_flag = 1;
                tsu_creat_task_result_msg_list_free();
            }
        }

        run->run_time = time(NULL);
        osip_usleep(5000);
    }

    return NULL;
}
#endif

#if DECS("TSU资源管理")
/*****************************************************************************
 Prototype    : zrv_compress_task_monitor_thread_execute
 Description  : TSU资源管理监测运行主线程
 Input        : void * p
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/7/3
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void* zrv_compress_task_monitor_thread_execute(void* p)
{
    thread_proc_t* run = (thread_proc_t*)p;

    if (run == NULL)
    {
        DEBUG_TRACE(MODULE_RESOURCE, LOG_DEBUG, "zrv_compress_task_monitor_thread_execute() exit---: Param Error \r\n");
        return NULL;
    }

    while (!run->th_exit)
    {
        if (1 == cms_run_status)
        {
            /* 每隔60秒扫描任务列表,监视等待响应的任务队列 */
            scan_compress_task_list_for_wait_expire(run->pDbOper, (int*)&(run->run_time));
        }

        run->run_time = time(NULL);
        osip_usleep(60000000); /* 60秒扫描一次 */
    }

    return NULL;
}
#endif

#if DECS("配置管理处理")
/*****************************************************************************
 Prototype    : config_mgn_proc_thread_execute
 Description  : 配置管理处理运行主线程
 Input        : void * p
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/3/27
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void* config_mgn_proc_thread_execute(void* p)
{
    thread_proc_t* run = (thread_proc_t*)p;

    if (run == NULL)
    {
        return NULL;
    }

    while (!run->th_exit)
    {
        if (1 == cms_run_status)
        {
            TelnetServerMain();
        }

        run->run_time = time(NULL);
        osip_usleep(5000);
    }

    return NULL;
}
#endif

#if DECS("Telnet客户端监视")
/*****************************************************************************
 Prototype    : telnet_client_monitor_thread_execute
 Description  : Telnet 客户端监测运行主线程
 Input        : void * p
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/7/3
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void* telnet_client_monitor_thread_execute(void* p)
{
    thread_proc_t* run = (thread_proc_t*)p;

    if (run == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "telnet_client_monitor_thread_execute() exit---: Param Error \r\n");
        return NULL;
    }

    while (!run->th_exit)
    {
        if (1 == cms_run_status)
        {
            /* 每隔一秒扫描Telnet 客户端 */
            ScanClientIfExpires();

            /* 扫描配置文件,全局配置参数是否有变化 */
            refresh_config_file();
        }

        run->run_time = time(NULL);
        osip_usleep(1000000);
    }

    return NULL;
}
#endif

#if DECS("公共的数据库刷新处理")
/*****************************************************************************
 Prototype    : common_db_refresh_thread_execute
 Description  : 公共的数据库刷新处理运行主线程
 Input        : void * p
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/7/3
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void* common_db_refresh_thread_execute(void* p)
{
    thread_proc_t* run = (thread_proc_t*)p;
    static int check_db_interval = 0;

    if (run == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "common_db_refresh_thread_execute() exit---: Param Error \r\n");
        return NULL;
    }

    while (!run->th_exit)
    {
        if (1 == cms_run_status)
        {
            check_db_interval++;

            if (check_db_interval > 300)
            {

            }
        }

        run->run_time = time(NULL);
        osip_usleep(1000000);
    }

    return NULL;
}
#endif

#if DECS("上级平台管理处理")
/*****************************************************************************
 Prototype    : platform_mgn_proc_thread_execute
 Description  : 上级平台管理处理
 Input        : void * p
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2013/5/30
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
void* platform_mgn_proc_thread_execute(void* p)
{
    thread_proc_t* run = (thread_proc_t*)p;
    static int check_db_interval = 0;

    if (run == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_mgn_proc_thread_execute() exit---: Param Error \r\n");
        return NULL;
    }

    while (!run->th_exit)
    {
        if (1 == cms_run_status)
        {
            check_db_interval++;

            if (check_db_interval > 300)
            {
                PlatformInfoConfig_db_refresh_proc();

                /* 检查路由信息是否需要重新加载 */
                check_PlatformInfoConfig_need_to_reload_begin(run->pDbOper);
            }

            /* 每隔一秒扫描路由信息 */
            scan_platform_info_list();

            if (check_db_interval > 300)
            {
                /* 检查路由信息是否需要重新加载 */
                check_PlatformInfoConfig_need_to_reload_end();
                check_db_interval = 0;
            }
        }

        run->run_time = time(NULL);
        osip_usleep(1000000);
    }

    return NULL;
}
#endif

