
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
#include "common/db_proc.h"

#include "route/platform_thread_proc.inc"
#include "route/route_srv_proc.inc"
#include "device/device_srv_proc.inc"
#include "service/compress_task_proc.inc"

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
#define MAX_PLATFORM_SRV_THREADS 10

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
platform_srv_proc_tl_list_t* g_PlatformSrvProcThreadList = NULL;            /* 上级路由业务处理线程队列 */

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#if DECS("上级路由业务处理线程")

/*****************************************************************************
 函 数 名  : get_platform_thread_info_from_db
 功能描述  : 从数据库中获取平台信息
 输入参数  : platform_srv_proc_tl_t* plat_thread
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年8月27日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int get_platform_thread_info_from_db(platform_srv_proc_tl_t* plat_thread)
{
    string strSQL = "";
    int record_count = 0;
    int task_mode = 0;
    int task_run_interval = 0;
    int task_get_interval = 0;

    int task_begin_time = 0;
    int task_end_time = 0;

    int last_task_time = 0;
    int compress_task_status = 0;

    if (NULL == plat_thread || plat_thread->platform_ip[0] == '\0' || NULL == plat_thread->pPlatform_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "get_platform_thread_info_from_db() exit---:  Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from VideoManagePlatformInfo WHERE PlatformIP LIKE '";
    strSQL += plat_thread->platform_ip;
    strSQL += "'";

    record_count = plat_thread->pPlatform_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "get_platform_thread_info_from_db() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "get_platform_thread_info_from_db() ErrorMsg=%s\r\n", plat_thread->pPlatform_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "get_platform_thread_info_from_db() exit---: No Record Count:strSQL=%s \r\n", strSQL.c_str());
        return 0;
    }

    /* 执行模式:0:自动执行, 1:手动执行, 默认为0 */
    task_mode = 0;
    plat_thread->pPlatform_Srv_dboper->GetFieldValue("TaskMode", task_mode);

    if (plat_thread->iTaskMode != task_mode)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "系统检测到压缩任务模式发生变化, 老模式=%d, 新模式=%d, 平台IP=%s", plat_thread->iTaskMode, task_mode, plat_thread->platform_ip);
        plat_thread->iTaskMode = task_mode;
    }

    /* 任务执行间隔时间, 单位秒, 默认为1800 */
    task_run_interval = 0;
    plat_thread->pPlatform_Srv_dboper->GetFieldValue("TaskRunInterval", task_run_interval);

    if (plat_thread->iTaskRunInterval != task_run_interval)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "系统检测到压缩任务运行时间间隔发生变化, 老间隔=%d, 新间隔=%d, 平台IP=%s", plat_thread->iTaskRunInterval, task_run_interval, plat_thread->platform_ip);
        plat_thread->iTaskRunInterval = task_run_interval;
    }

    /* 任务获取间隔时间, 单位秒, 默认为1800 */
    task_get_interval = 0;
    plat_thread->pPlatform_Srv_dboper->GetFieldValue("TaskGetInterval", task_get_interval);

    if (plat_thread->iTaskGetInterval != task_get_interval)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "系统检测到压缩任务获取时间间隔发生变化, 老间隔=%d, 新间隔=%d, 平台IP=%s", plat_thread->iTaskGetInterval, task_get_interval, plat_thread->platform_ip);
        plat_thread->iTaskGetInterval = task_get_interval;
    }

    /* 手动任务执行获取开始时间 */
    task_begin_time = 0;
    plat_thread->pPlatform_Srv_dboper->GetFieldValue("TaskBeginTime", task_begin_time);

    if (plat_thread->iTaskBeginTime != task_begin_time)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "系统检测到压缩任务开始时间发生变化, 老开始时间=%d, 新开始时间=%d, 平台IP=%s", plat_thread->iTaskBeginTime, task_begin_time, plat_thread->platform_ip);
        plat_thread->iTaskBeginTime = task_begin_time;
    }

    /* 手动任务执行获取结束时间 */
    task_end_time = 0;
    plat_thread->pPlatform_Srv_dboper->GetFieldValue("TaskEndTime", task_end_time);

    if (plat_thread->iTaskEndTime != task_end_time)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "系统检测到压缩任务结束时间发生变化, 老结束时间=%d, 新结束时间=%d, 平台IP=%s", plat_thread->iTaskEndTime, task_end_time, plat_thread->platform_ip);
        plat_thread->iTaskEndTime = task_end_time;
    }

    /* 上次获取任务执行时间 */
    last_task_time = 0;
    plat_thread->pPlatform_Srv_dboper->GetFieldValue("LastTaskTime", last_task_time);

    if (plat_thread->iLastGetTaskTime != last_task_time)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "系统检测到压缩任务最后获取时间发送变化, 老时间=%d, 新时间=%d, 平台IP=%s", plat_thread->iLastGetTaskTime, last_task_time, plat_thread->platform_ip);
        plat_thread->iLastGetTaskTime = last_task_time;
    }

    /* 任务状态:0:初始状态, 1:准备获取, 2:正在获取处理, 3:获取失败, 4:获取成功，准备分配, 5:正在分配处理, 6:没有分配成功, 7:分配成功，等待下一次任务启动 */
    compress_task_status = 0;
    plat_thread->pPlatform_Srv_dboper->GetFieldValue("CompressTaskStatus", compress_task_status);
    plat_thread->iCompressTaskStatus = compress_task_status;

    return 0;
}

/*****************************************************************************
 函 数 名  : platform_srv_proc_thread_for_appoint_execute
 功能描述  :  platform_srv_proc_thread_execute
 输入参数  : appoint_platform_srv_proc_tl_t * platform_srv_proc
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void* platform_srv_proc_thread_execute(void* p)
{
    int iRet = 0;
    int flag = 0;
    int flag2 = 0;
    int db_count = 0;
    platform_srv_proc_tl_t* run = (platform_srv_proc_tl_t*)p;
    vector<string> ZRVDeviceIP;
    int zrvdevice_count = 0;           /* 记录数 */
    static int check_db_interval = 0;

    int last_task_time = 0;
    int compress_task_status = 0;

    int task_mode = 0;
    int task_begin_time = 0;
    int task_end_time = 0;
    int change_flag = 0;  /* 变化标识 */

    char strTaskBeginTime[64] = {0};
    char strTaskEndTime[64] = {0};
    char strLastGetTaskTime[64] = {0};

    if (run == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_srv_proc_thread_execute() exit---: Param Error \r\n");
        return NULL;
    }

    while (!run->th_exit)
    {
        if (0 == cms_run_status)
        {
            run->run_time = time(NULL);
            osip_usleep(1000000);
            continue;
        }

        if (0 == run->iUsed)
        {
            run->run_time = time(NULL);
            osip_usleep(1000000);
            continue;
        }

        if (run->platform_index <= 0)
        {
            run->run_time = time(NULL);
            osip_usleep(1000000);
            continue;
        }

        if (run->platform_ip[0] == '\0')
        {
            run->run_time = time(NULL);
            osip_usleep(1000000);
            continue;
        }

        if (NULL == run->pPlatform_Srv_dboper)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "platform_srv_proc_thread_execute() Platform Srv DB Oper NULL: platform_index=%u,platform_ip=%s \r\n", run->platform_index, run->platform_ip);
            run->run_time = time(NULL);
            osip_usleep(1000000);
            continue;
        }

        run->run_time = time(NULL);

        /* 读取数据库 */
        if (8 == run->iCompressTaskStatus) /* 压缩任务完成的时候，读取数据库配置是否变化 */
        {
            get_platform_thread_info_from_db(run);
        }

        if (1 == run->iTaskMode) /* 手动模式 */
        {
            if (0 == run->iLastTaskRunTime) /* 系统第一次启动运行 */
            {
                run->iLastTaskRunTime = run->run_time;

                iRet = format_time(run->iTaskBeginTime, strTaskBeginTime);
                iRet = format_time(run->iTaskEndTime, strTaskEndTime);
                iRet = format_time(run->iLastGetTaskTime, strLastGetTaskTime);

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 当前压缩模式=%d, 系统获取压缩任务开始时间=%s, 系统获取压缩任务结束时间=%s, 系统之前获取压缩任务时间=%s, 执行压缩任务状态=%d, 平台IP=%s", run->iTaskMode, strTaskBeginTime, strTaskEndTime, strLastGetTaskTime, run->iCompressTaskStatus, run->platform_ip);

                if (run->iLastGetTaskTime <= 0) /* 系统之前没有执行过任务 */
                {
                    // TODO: OK
                    run->iCompressTaskStatus = 1;
                    run->iLastGetTaskTime = run->iTaskBeginTime;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前没有执行过压缩任务, 从上级平台获取压缩任务信息, 平台IP=%s", run->platform_ip);

                    /* 更新到数据库 */
                    UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                }
                else if (run->iLastGetTaskTime > 0) /* 系统之前执行过任务 */
                {
                    if (run->iTaskEndTime > 0) /* 有截止时间 */
                    {
                        if (run->iLastGetTaskTime >= run->iTaskBeginTime
                            && run->iLastGetTaskTime < run->iTaskEndTime) /* 系统中途重启之后，还没有执行到最后时间 */
                        {
                            if (0 == run->iCompressTaskStatus)
                            {
                                // TODO: OK
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，系统状态处于初始状态，并且没有到结束时间, 立刻重新获取, 平台IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* 更新到数据库 */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else if (3 == run->iCompressTaskStatus)
                            {
                                // TODO: OK
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，通过WebService获取压缩任务失败，并且没有到结束时间, 立刻重新获取, 平台IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* 更新到数据库 */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else if (8 == run->iCompressTaskStatus)
                            {
                                // TODO: OK:
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，上一轮执行已经结束，并且没有到结束时间, 立刻重新获取, 平台IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* 更新到数据库 */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else
                            {
                                // TODO: OK
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，但是还没有到结束时间, 等待任务结束, 平台IP=%s", run->platform_ip);
                            }
                        }
                        else if (run->iLastGetTaskTime == run->iTaskEndTime) /* 系统中途重启之后，但是已经执行结束了 */
                        {
                            if (8 == run->iCompressTaskStatus)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，已经到达结束时间, 等待下次任务开始, 平台IP=%s", run->platform_ip);
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，已经到达结束时间, 但是上次的任务没有处理完, 等待处理结束, 平台IP=%s", run->platform_ip);
                            }
                        }
                        else /* 时间段发生变化，跟之前的任务时间不匹配 */
                        {
                            flag = HasCompressTaskNotComplete(run->platform_ip);

                            if (!flag)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 上次没有未完成的任务, 从上级平台获取压缩任务信息, 平台IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;
                                run->iLastGetTaskTime = run->iTaskBeginTime;

                                /* 更新到数据库 */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，但是还有未完成的任务, 等待任务结束, 平台IP=%s", run->platform_ip);
                            }
                        }
                    }
                    else /* 没有截止时间 */
                    {
                        if (run->iLastGetTaskTime >= run->iTaskBeginTime) /* 系统中途重启之后，需要继续执行 */
                        {
                            if (0 == run->iCompressTaskStatus)
                            {
                                // TODO: OK
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，系统状态处于初始状态，并且没有结束时间, 立刻重新获取, 平台IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* 更新到数据库 */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else if (3 == run->iCompressTaskStatus)
                            {
                                // TODO: OK
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，通过WebService获取压缩任务失败，并且没有结束时间, 立刻重新获取, 平台IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* 更新到数据库 */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else if (8 == run->iCompressTaskStatus)
                            {
                                // TODO: OK:
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，上一轮执行已经结束，并且没有结束时间, 立刻重新获取, 平台IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* 更新到数据库 */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else
                            {
                                // TODO: OK
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，但是还没有结束时间, 等待任务结束, 平台IP=%s", run->platform_ip);
                            }
                        }
                        else /* 时间段发生变化，跟之前的任务时间不匹配 */
                        {
                            flag = HasCompressTaskNotComplete(run->platform_ip);

                            if (!flag)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 上次没有未完成的任务, 从新开始执行任务, 从上级平台获取压缩任务信息, 平台IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;
                                run->iLastGetTaskTime = run->iTaskBeginTime;

                                /* 更新到数据库 */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，但是还有未完成的任务, 等待任务结束, 平台IP=%s", run->platform_ip);
                            }
                        }
                    }
                }
            }
            else if (run->iLastTaskRunTime > 0)/* 系统启动执行之后 */
            {
                if (run->iLastGetTaskTime <= 0) /* 重新开始 */
                {
                    // TODO: OK
                    run->iCompressTaskStatus = 1;
                    run->iLastGetTaskTime = run->iTaskBeginTime;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统重新开始执行压缩任务, 从上级平台获取压缩任务信息, 平台IP=%s", run->platform_ip);

                    /* 更新到数据库 */
                    UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                }
                else if (run->iLastGetTaskTime > 0)
                {
                    if (run->iTaskEndTime > 0) /* 有截止时间 */
                    {
                        if (run->iLastGetTaskTime >= run->iTaskBeginTime
                            && run->iLastGetTaskTime < run->iTaskEndTime) /* 超过最终任务时间就不执行了 */
                        {
                            // TODO: OK:
                            if (8 == run->iCompressTaskStatus)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统重新开始执行压缩任务, 从上级平台获取压缩任务信息, 平台IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* 更新到数据库 */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                        }
                        else if (run->iLastGetTaskTime == run->iTaskEndTime) /* 系统中途重启之后，但是已经执行结束了 */
                        {
                            // TODO: OK
                            if (8 == run->iCompressTaskStatus)
                            {
                                check_db_interval++;

                                if (check_db_interval >= 5)
                                {
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统之前执行过压缩任务，已经到达结束时间, 等待下次任务开始, 平台IP=%s", run->platform_ip);
                                    check_db_interval = 0;
                                }
                            }
                            else
                            {
                                check_db_interval++;

                                if (check_db_interval >= 5)
                                {
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统之前执行过压缩任务，已经到达结束时间, 但是还有未完成的任务, 等待任务结束, 平台IP=%s", run->platform_ip);
                                    check_db_interval = 0;
                                }
                            }
                        }
                        else /* 时间段不匹配 */
                        {
                            check_db_interval++;

                            if (check_db_interval >= 5)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统之前执行过压缩任务，最后执行时间和时间段不匹配, 等待下次任务执行, 平台IP=%s", run->platform_ip);
                                check_db_interval = 0;
                            }
                        }
                    }
                    else /* 没有截止时间 */
                    {
                        if (run->iLastGetTaskTime >= run->iTaskBeginTime) /* 超过最终任务时间就不执行了 */
                        {
                            // TODO: OK:
                            if (8 == run->iCompressTaskStatus)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统重新开始执行压缩任务, 从上级平台获取压缩任务信息, 平台IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* 更新到数据库 */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                        }
                        else /* 时间段不匹配 */
                        {
                            check_db_interval++;

                            if (check_db_interval >= 5)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统之前执行过压缩任务，最后执行时间和时间段不匹配, 等待下次任务执行, 平台IP=%s", run->platform_ip);
                                check_db_interval = 0;
                            }
                        }
                    }
                }
            }
        }
        else /* 自动模式 */
        {
            if (0 == run->iLastTaskRunTime)/* 系统第一启动 */
            {
                run->iLastTaskRunTime = run->run_time;

                iRet = format_time(run->iLastGetTaskTime, strLastGetTaskTime);

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 当前压缩模式=%d, 系统之前获取压缩任务时间=%s, 执行压缩任务状态=%d, 平台IP=%s", run->iTaskMode, strLastGetTaskTime, run->iCompressTaskStatus, run->platform_ip);

                flag = HasCompressTaskNotComplete(run->platform_ip);

                if (!flag)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 上次没有未完成的任务, 从上级平台获取压缩任务信息, 平台IP=%s", run->platform_ip);
                    run->iCompressTaskStatus = 1;

                    /* 更新到数据库 */
                    UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                }
                else
                {
                    if (0 == run->iCompressTaskStatus)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，系统状态处于初始状态, 立刻重新获取, 平台IP=%s", run->platform_ip);
                        run->iCompressTaskStatus = 1;

                        /* 更新到数据库 */
                        UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                    }
                    else if (3 == run->iCompressTaskStatus)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，通过WebService获取压缩任务失败, 立刻重新获取, 平台IP=%s", run->platform_ip);
                        run->iCompressTaskStatus = 1;

                        /* 更新到数据库 */
                        UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                    }
                    else if (8 == run->iCompressTaskStatus)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，上一轮执行已经结束, 立刻重新获取, 平台IP=%s", run->platform_ip);
                        run->iCompressTaskStatus = 1;

                        /* 更新到数据库 */
                        UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统启动, 系统之前执行过压缩任务，但是还没有结束, 等待任务结束, 平台IP=%s", run->platform_ip);
                    }
                }
            }
            else if (run->iLastTaskRunTime > 0) /* 系统启动之后 */
            {
                if (8 == run->iCompressTaskStatus)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统重新开始执行压缩任务, 从上级平台获取压缩任务信息, 平台IP=%s", run->platform_ip);
                    run->iCompressTaskStatus = 1;

                    /* 更新到数据库 */
                    UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                }
            }
        }

        iRet = get_compress_task_from_webservice_thread_execute(run);

        /* 判断各种状态下的情况 */
        if (0 == run->iCompressTaskStatus)
        {
            if (run->run_time - run->iLastTaskRunTime >= run->iTaskRunInterval) /* 设置超时 */
            {
                run->iCompressTaskStatus = 8;
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "处理线程初始状态间隔达到%d秒, 重新启动获取压缩任务, 平台IP=%s", run->iTaskRunInterval, run->platform_ip);

                /* 更新到数据库 */
                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
            }
        }
        else if (3 == run->iCompressTaskStatus)
        {
            if (run->run_time - run->iLastTaskRunTime >= run->iTaskRunInterval) /* 设置超时 */
            {
                run->iCompressTaskStatus = 8;
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上次获取压缩任务信息失败间隔达到%d秒, 重新启动获取压缩任务, 平台IP=%s", run->iTaskRunInterval, run->platform_ip);

                /* 更新到数据库 */
                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
            }
        }
        else if (7 == run->iCompressTaskStatus) /* 分配完成的情况下，查看任务是否结束或者超时 */
        {
            if (1 == run->iTaskMode)
            {
                check_db_interval++;

                if (check_db_interval >= 5)
                {
                    /* 判断一下是否有等待结果的任务了,如果没有，则立刻执行下一轮操作 */
                    flag2 = HasCompressTaskNotComplete2(run->platform_ip);

                    if (!flag2)
                    {
                        run->iCompressTaskStatus = 8;
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上次压缩任务已经提前完成, 启动下一轮压缩任务, 平台IP=%s", run->platform_ip);

                        /* 更新到数据库 */
                        UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                    }

                    check_db_interval = 0;
                }
            }

            if (run->run_time - run->iLastTaskRunTime >= run->iTaskRunInterval) /* 设置超时 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上次获取压缩任务信息达到%d秒, 启动下一轮压缩任务, 平台IP=%s", run->iTaskRunInterval, run->platform_ip);

                /* 更新没有结果的任务超时 */
                //update_compress_task_list_for_wait_expire(run->pPlatform_Srv_dboper);

                run->iCompressTaskStatus = 8;

                /* 更新到数据库 */
                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
            }
        }

        osip_usleep(1000000);
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : platform_srv_proc_thread_init
 功能描述  : 上级路由业务处理线程初始化
 输入参数  : platform_srv_proc_tl_t** run
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int platform_srv_proc_thread_init(platform_srv_proc_tl_t** run)
{
    int i = 0;
    *run = (platform_srv_proc_tl_t*) osip_malloc(sizeof(platform_srv_proc_tl_t));

    if (*run == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_srv_proc_thread_init() exit---: *run Smalloc Error \r\n");
        return -1;
    }

    (*run)->iUsed = 0;
    (*run)->platform_index = 0;
    (*run)->platform_ip[0] = '\0';
    (*run)->thread = NULL;
    (*run)->th_exit = 0;
    (*run)->run_time = 0;
    (*run)->pPlatform_Srv_dboper = NULL;
    (*run)->pPlatformLogDbOper = NULL;
    (*run)->iPlatformLogDBOperConnectStatus = 0;
    (*run)->iTaskMode = 0;
    (*run)->iTaskRunInterval = 1800;
    (*run)->iTaskGetInterval = 1800;
    (*run)->iTaskBeginTime = 0;
    (*run)->iTaskEndTime = 0;
    (*run)->iLastGetTaskTime = 0;
    (*run)->iLastTaskRunTime = 0;
    (*run)->iCompressTaskStatus = 0;

    return 0;
}

/*****************************************************************************
 函 数 名  : platform_srv_proc_thread_free
 功能描述  : 上级路由业务处理线程释放
 输入参数  : platform_srv_proc_tl_t *run
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void platform_srv_proc_thread_free(platform_srv_proc_tl_t* run)
{
    int i = 0;

    if (run == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_srv_proc_thread_free() exit---: Param Error \r\n");
        return;
    }

    run->iUsed = 0;
    run->platform_index = 0;
    memset(run->platform_ip, 0, MAX_IP_LEN);

    if (run->thread)
    {
        osip_free(run->thread);
        run->thread = NULL;
    }

    run->run_time = 0;

    if (run->pPlatform_Srv_dboper != NULL)
    {
        delete run->pPlatform_Srv_dboper;
        run->pPlatform_Srv_dboper = NULL;
    }

    if (run->pPlatformLogDbOper != NULL)
    {
        delete run->pPlatformLogDbOper;
        run->pPlatformLogDbOper = NULL;
    }

    run->iPlatformLogDBOperConnectStatus = 0;
    run->iTaskMode = 0;
    run->iTaskRunInterval = 1800;
    run->iTaskGetInterval = 1800;
    run->iTaskBeginTime = 0;
    run->iTaskEndTime = 0;
    run->iLastGetTaskTime = 0;
    run->iLastTaskRunTime = 0;
    run->iCompressTaskStatus = 0;

    osip_free(run);
    run = NULL;

    return;
}
#endif

#if DECS("上级路由业务处理线程队列")
/*****************************************************************************
 函 数 名  : platform_srv_proc_thread_assign
 功能描述  : 上级路由业务处理线程分配
 输入参数  : platform_info_t* pPlatformInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月27日
    作    者   : 上级路由路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
int platform_srv_proc_thread_assign(platform_info_t* pPlatformInfo)
{
    int last_task_time = 0;
    int compress_task_status = 0;

    int task_mode = 0;
    int task_begin_time = 0;
    int task_end_time = 0;

    platform_srv_proc_tl_t* runthread = NULL;

    if (NULL == pPlatformInfo || pPlatformInfo->platform_ip[0] == '\0' || pPlatformInfo->id <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_srv_proc_thread_assign() exit---: Param Error \r\n");
        return -1;
    }

    //printf("\r\n platform_srv_proc_thread_assign() Enter--- \r\n");
    //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "platform_srv_proc_thread_assign() Enter--- \r\n");

    PLATFORM_SRV_PROC_THREAD_SMUTEX_LOCK();

    runthread = get_free_platform_srv_proc_thread();

    if (NULL == runthread)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "platform_srv_proc_thread_assign() exit---: Get Free Thread Error \r\n");
        PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return -1;
    }

    runthread->iUsed = 1;
    runthread->platform_index = pPlatformInfo->id;
    osip_strncpy(runthread->platform_ip, pPlatformInfo->platform_ip, MAX_IP_LEN);

    /* 初始化数据库连接 */
    if (NULL == runthread->pPlatform_Srv_dboper)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "platform_srv_proc_thread_assign() Platform Srv DB Oper New:platform_id=%s, platform_ip=%s, platform_port=%d \r\n", pPlatformInfo->server_id, pPlatformInfo->server_ip, pPlatformInfo->server_port);

        runthread->pPlatform_Srv_dboper = new DBOper();

        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "platform_srv_proc_thread_assign() Platform Srv DB Oper New End \r\n");

        if (runthread->pPlatform_Srv_dboper == NULL)
        {
            //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "platform_srv_proc_thread_assign() exit---: Platform Srv DB Oper NULL \r\n");
            PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return -1;
        }

        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "platform_srv_proc_thread_assign() Platform Srv DB Start Connect:g_StrCon[0]=%s, g_StrCon[1]=%s \r\n", g_StrCon[0], g_StrCon[1]);

        if (runthread->pPlatform_Srv_dboper->Connect(g_StrCon, (char*)"") < 0)
        {
            delete runthread->pPlatform_Srv_dboper;
            runthread->pPlatform_Srv_dboper = NULL;
            PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return -1;
        }

        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "platform_srv_proc_thread_assign() Platform Srv DB Connect End \r\n");
    }

    /* 初始化日志数据库连接 */
    if (NULL == runthread->pPlatformLogDbOper)
    {
        runthread->pPlatformLogDbOper = new DBOper();

        if (runthread->pPlatformLogDbOper == NULL)
        {
            delete runthread->pPlatform_Srv_dboper;
            runthread->pPlatform_Srv_dboper = NULL;
            PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return -1;
        }

        if (runthread->pPlatformLogDbOper->Connect(g_StrConLog, (char*)"") < 0)
        {
            runthread->iPlatformLogDBOperConnectStatus = 0;
        }
        else
        {
            runthread->iPlatformLogDBOperConnectStatus = 1;
        }
    }

    runthread->iCompressTaskStatus = 0;

    PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    /* 加载一下上次没有完成的任务 */
    set_db_data_to_compress_task_list(runthread->platform_ip, runthread->pPlatform_Srv_dboper);

    /* 获取上次的任务信息 */
    get_platform_thread_info_from_db(runthread);
    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "platform_srv_proc_thread_assign() iTaskMode=%d, iTaskBeginTime=%d, iTaskEndTime=%d \r\n", runthread->iTaskMode, runthread->iTaskBeginTime, runthread->iTaskEndTime);

    //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "platform_srv_proc_thread_assign() Exit--- \r\n");
    //printf("\r\n platform_srv_proc_thread_assign() Exit--- \r\n");
    return 0;
}

/*****************************************************************************
 函 数 名  : platform_srv_proc_thread_recycle
 功能描述  : 上级路由业务处理线程回收
 输入参数  : unsigned int platform_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月27日
    作    者   : 上级路由路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
int platform_srv_proc_thread_recycle(unsigned int platform_index)
{
    platform_srv_proc_tl_t* runthread = NULL;

    if (platform_index <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_srv_proc_thread_recycle() exit---: Param Error \r\n");
        return -1;
    }

    //rintf("\r\n platform_srv_proc_thread_recycle() Enter--- \r\n");
    //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "platform_srv_proc_thread_recycle() Enter--- \r\n");

    PLATFORM_SRV_PROC_THREAD_SMUTEX_LOCK();

    runthread = get_platform_srv_proc_thread2(platform_index);

    if (NULL == runthread)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "platform_srv_proc_thread_recycle() exit---: Get Thread Error \r\n");
        PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return 0;
    }

    runthread->iUsed = 0;
    runthread->platform_index = 0;
    memset(runthread->platform_ip, 0, MAX_IP_LEN);

    if (NULL != runthread->pPlatform_Srv_dboper)
    {
        delete runthread->pPlatform_Srv_dboper;
        runthread->pPlatform_Srv_dboper = NULL;
        //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "platform_srv_proc_thread_recycle() Platform Srv DB Oper Delete End--- \r\n");
    }

    if (NULL != runthread->pPlatformLogDbOper)
    {
        delete runthread->pPlatformLogDbOper;
        runthread->pPlatformLogDbOper = NULL;
    }

    runthread->iPlatformLogDBOperConnectStatus = 0;
    runthread->iCompressTaskStatus = 0;

    PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    //printf("\r\n platform_srv_proc_thread_recycle() Exit--- \r\n");
    //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "platform_srv_proc_thread_recycle() Exit--- \r\n");
    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "platform_srv_proc_thread_recycle() platform_index=%d \r\n", platform_index);

    return 0;
}

/*****************************************************************************
 函 数 名  : platform_srv_proc_thread_start_all
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
int platform_srv_proc_thread_start_all()
{
    int i = 0;
    int index = 0;
    platform_srv_proc_tl_t* runthread = NULL;

    if (NULL == g_PlatformSrvProcThreadList || NULL == g_PlatformSrvProcThreadList->pPlatformSrvProcList)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_srv_proc_thread_start() exit---: Param Error 2 \r\n");
        return -1;
    }

    PLATFORM_SRV_PROC_THREAD_SMUTEX_LOCK();

    for (index = 0; index < MAX_PLATFORM_SRV_THREADS; index++)
    {
        i = platform_srv_proc_thread_init(&runthread);
        //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "platform_srv_proc_thread_start() platform_srv_proc_thread_init:i=%d \r\n", i);

        if (i != 0)
        {
            //DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "platform_srv_proc_thread_start() exit---: Platform Srv Proc Thread Init Error \r\n");
            continue;
        }

        //添加到上级路由业务处理线程队列
        i = osip_list_add(g_PlatformSrvProcThreadList->pPlatformSrvProcList, runthread, -1); /* add to list tail */
        //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "platform_srv_proc_thread_start() osip_list_add:i=%d \r\n", i);

        if (i < 0)
        {
            platform_srv_proc_thread_free(runthread);
            runthread = NULL;
            //DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "platform_srv_proc_thread_start() exit---: List Add Error \r\n");
            continue;
        }

        //启动处理线程
        runthread->thread = (osip_thread_t*)osip_thread_create(20000, platform_srv_proc_thread_execute, (void*)runthread);

        if (runthread->thread == NULL)
        {
            osip_list_remove(g_PlatformSrvProcThreadList->pPlatformSrvProcList, i);
            platform_srv_proc_thread_free(runthread);
            runthread = NULL;
            //DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "platform_srv_proc_thread_start() exit---: Platform Srv Proc Thread Create Error \r\n");
            continue;
        }

        //printf("\r\n platform_srv_proc_thread_start:runthread->thread=0x%lx \r\n", (unsigned long)runthread->thread);
    }

    PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : platform_srv_proc_thread_stop_all
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
int platform_srv_proc_thread_stop_all()
{
    int pos = 0;
    int i = 0;
    platform_srv_proc_tl_t* runthread = NULL;

    if (NULL == g_PlatformSrvProcThreadList || NULL == g_PlatformSrvProcThreadList->pPlatformSrvProcList)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_srv_proc_thread_stop_all() exit---: Param Error \r\n");
        return -1;
    }

    PLATFORM_SRV_PROC_THREAD_SMUTEX_LOCK();

    //查找队列，停止线程
    for (pos = 0; pos < osip_list_size(g_PlatformSrvProcThreadList->pPlatformSrvProcList); pos++)
    {
        runthread = (platform_srv_proc_tl_t*)osip_list_get(g_PlatformSrvProcThreadList->pPlatformSrvProcList, pos);

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

    PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : platform_srv_proc_thread_find
 功能描述  : 查找上级路由业务处理线程
 输入参数  : unsigned int platform_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int platform_srv_proc_thread_find(unsigned int platform_index)
{
    int pos = 0;
    platform_srv_proc_tl_t* runthread = NULL;

    if (platform_index <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_srv_proc_thread_find() exit---: Param Error \r\n");
        return -1;
    }

    PLATFORM_SRV_PROC_THREAD_SMUTEX_LOCK();

    //查找队列
    for (pos = 0; pos < osip_list_size(g_PlatformSrvProcThreadList->pPlatformSrvProcList); pos++)
    {
        runthread = (platform_srv_proc_tl_t*)osip_list_get(g_PlatformSrvProcThreadList->pPlatformSrvProcList, pos);

        if (runthread == NULL || 0 == runthread->iUsed)
        {
            continue;
        }

        if ('\0' == runthread->platform_ip[0] || runthread->platform_index <= 0)
        {
            continue;
        }

        if (runthread->platform_index == platform_index)
        {
            PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            return pos;
        }
    }

    PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return -1;
}

/*****************************************************************************
 函 数 名  : get_free_platform_srv_proc_thread
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
platform_srv_proc_tl_t* get_free_platform_srv_proc_thread()
{
    int pos = 0;
    platform_srv_proc_tl_t* runthread = NULL;

    //查找队列，将sipudp从接收线程队列中移除
    for (pos = 0; pos < osip_list_size(g_PlatformSrvProcThreadList->pPlatformSrvProcList); pos++)
    {
        runthread = (platform_srv_proc_tl_t*)osip_list_get(g_PlatformSrvProcThreadList->pPlatformSrvProcList, pos);

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
 函 数 名  : get_platform_srv_proc_thread
 功能描述  : 获取上级路由业务处理线程
 输入参数  : unsigned int platform_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月27日
    作    者   : 上级路由路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
platform_srv_proc_tl_t* get_platform_srv_proc_thread(unsigned int platform_index)
{
    int pos = 0;
    platform_srv_proc_tl_t* runthread = NULL;

    if (platform_index <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "get_platform_srv_proc_thread() exit---: Param Error \r\n");
        return NULL;
    }

    PLATFORM_SRV_PROC_THREAD_SMUTEX_LOCK();

    for (pos = 0; pos < osip_list_size(g_PlatformSrvProcThreadList->pPlatformSrvProcList); pos++)
    {
        runthread = (platform_srv_proc_tl_t*)osip_list_get(g_PlatformSrvProcThreadList->pPlatformSrvProcList, pos);

        if (runthread == NULL || 0 == runthread->iUsed)
        {
            continue;
        }

        if ('\0' == runthread->platform_ip[0] || runthread->platform_index <= 0)
        {
            continue;
        }

        if (runthread->platform_index == platform_index)
        {
            PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();
            //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "get_platform_srv_proc_thread() exit---: pos=%d, platform_id=%s, platform_ip=%s, platform_port=%d \r\n", pos, platform_id, platform_ip, platform_port);
            return runthread;
        }
    }

    PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 函 数 名  : get_platform_srv_proc_thread2
 功能描述  : 获取上级路由业务处理线程
 输入参数  : unsigned int platform_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月13日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
platform_srv_proc_tl_t* get_platform_srv_proc_thread2(unsigned int platform_index)
{
    int pos = 0;
    platform_srv_proc_tl_t* runthread = NULL;

    if (platform_index <= 0)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "get_platform_srv_proc_thread2() exit---: Param Error \r\n");
        return NULL;
    }

    for (pos = 0; pos < osip_list_size(g_PlatformSrvProcThreadList->pPlatformSrvProcList); pos++)
    {
        runthread = (platform_srv_proc_tl_t*)osip_list_get(g_PlatformSrvProcThreadList->pPlatformSrvProcList, pos);

        if (runthread == NULL || 0 == runthread->iUsed)
        {
            continue;
        }

        if ('\0' == runthread->platform_ip[0] || runthread->platform_index <= 0)
        {
            continue;
        }

        if (runthread->platform_index == platform_index)
        {
            return runthread;
        }
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : platform_srv_proc_thread_list_init
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
int platform_srv_proc_thread_list_init()
{
    g_PlatformSrvProcThreadList = (platform_srv_proc_tl_list_t*)osip_malloc(sizeof(platform_srv_proc_tl_list_t));

    if (g_PlatformSrvProcThreadList == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_srv_proc_thread_list_init() exit---: g_PlatformSrvProcThreadList Smalloc Error \r\n");
        return -1;
    }

    g_PlatformSrvProcThreadList->pPlatformSrvProcList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (g_PlatformSrvProcThreadList->pPlatformSrvProcList == NULL)
    {
        osip_free(g_PlatformSrvProcThreadList);
        g_PlatformSrvProcThreadList = NULL;
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_srv_proc_thread_list_init() exit---: Platform Srv Proc List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_PlatformSrvProcThreadList->pPlatformSrvProcList);

#ifdef MULTI_THR
    /* init smutex */
    g_PlatformSrvProcThreadList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_PlatformSrvProcThreadList->lock)
    {
        osip_free(g_PlatformSrvProcThreadList->pPlatformSrvProcList);
        g_PlatformSrvProcThreadList->pPlatformSrvProcList = NULL;
        osip_free(g_PlatformSrvProcThreadList);
        g_PlatformSrvProcThreadList = NULL;
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_srv_proc_thread_list_init() exit---: Platform Srv Proc List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : platform_srv_proc_thread_list_free
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
void platform_srv_proc_thread_list_free()
{
    if (NULL == g_PlatformSrvProcThreadList)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_srv_proc_thread_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_PlatformSrvProcThreadList->pPlatformSrvProcList)
    {
        osip_list_special_free(g_PlatformSrvProcThreadList->pPlatformSrvProcList, (void (*)(void*))&platform_srv_proc_thread_free);
        osip_free(g_PlatformSrvProcThreadList->pPlatformSrvProcList);
        g_PlatformSrvProcThreadList->pPlatformSrvProcList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_PlatformSrvProcThreadList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_PlatformSrvProcThreadList->lock);
        g_PlatformSrvProcThreadList->lock = NULL;
    }

#endif

    osip_free(g_PlatformSrvProcThreadList);
    g_PlatformSrvProcThreadList = NULL;

}

/*****************************************************************************
 函 数 名  : platform_srv_proc_thread_list_lock
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
int platform_srv_proc_thread_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PlatformSrvProcThreadList == NULL || g_PlatformSrvProcThreadList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_srv_proc_thread_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_PlatformSrvProcThreadList->lock);
#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : platform_srv_proc_thread_list_unlock
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
int platform_srv_proc_thread_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PlatformSrvProcThreadList == NULL || g_PlatformSrvProcThreadList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "platform_srv_proc_thread_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_PlatformSrvProcThreadList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_platform_srv_proc_thread_list_lock
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
int debug_platform_srv_proc_thread_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PlatformSrvProcThreadList == NULL || g_PlatformSrvProcThreadList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "debug_platform_srv_proc_thread_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_PlatformSrvProcThreadList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_platform_srv_proc_thread_list_unlock
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
int debug_platform_srv_proc_thread_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_PlatformSrvProcThreadList == NULL || g_PlatformSrvProcThreadList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "debug_platform_srv_proc_thread_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_PlatformSrvProcThreadList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : scan_platform_srv_proc_thread_list
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
void scan_platform_srv_proc_thread_list()
{
    int i = 0;
    //int iRet = 0;
    platform_srv_proc_tl_t* pThreadProc = NULL;
    needtoproc_platformsrvproc_queue needToProc;
    time_t now = time(NULL);

    if ((NULL == g_PlatformSrvProcThreadList) || (NULL == g_PlatformSrvProcThreadList->pPlatformSrvProcList))
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "scan_platform_srv_proc_thread_list() exit---: Param Error \r\n");
        return;
    }

    needToProc.clear();

    PLATFORM_SRV_PROC_THREAD_SMUTEX_LOCK();

    if (osip_list_size(g_PlatformSrvProcThreadList->pPlatformSrvProcList) <= 0)
    {
        PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_PlatformSrvProcThreadList->pPlatformSrvProcList); i++)
    {
        pThreadProc = (platform_srv_proc_tl_t*)osip_list_get(g_PlatformSrvProcThreadList->pPlatformSrvProcList, i);

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

        if (pThreadProc->platform_index <= 0)
        {
            continue;
        }

        if (pThreadProc->platform_ip[0] == '\0')
        {
            continue;
        }

        if (pThreadProc->run_time < now && now - pThreadProc->run_time > 3600)
        {
            needToProc.push_back(pThreadProc);
        }
    }

    PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    /* 处理需要开始的 */
    while (!needToProc.empty())
    {
        pThreadProc = (platform_srv_proc_tl_t*) needToProc.front();
        needToProc.pop_front();

        if (NULL != pThreadProc)
        {
            //iRet = platform_srv_proc_thread_restart(pThreadProc->pPlatformInfo);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级路由业务处理线程监控线程检测到上级平台处理线程挂死, 上级平台ID=%u, 上级平台IP=%s", pThreadProc->platform_index, pThreadProc->platform_ip);
            osip_usleep(5000000);
            system((char*)"killall cms; killall -9 cms");
        }
    }

    needToProc.clear();

    return;
}

/*****************************************************************************
 函 数 名  : show_platform_srv_proc_thread
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
void show_platform_srv_proc_thread(int sock, int type)
{
    int i = 0;
    int pos = 0;
    char strLine[] = "\r------------------------------------------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rThread Index  Thread TID  Used Flag  Platform ID  Platform IP     Run Time            Last Task Run Time  Last Get Task Time  CompressTaskStatus\r\n";
    char rbuf[256] = {0};
    platform_srv_proc_tl_t* runthread = NULL;
    char strRunTime[64] = {0};
    char strLastTaskRunTime[64] = {0};
    char strLastTaskGetTime[64] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    PLATFORM_SRV_PROC_THREAD_SMUTEX_LOCK();

    if (NULL == g_PlatformSrvProcThreadList || osip_list_size(g_PlatformSrvProcThreadList->pPlatformSrvProcList) <= 0)
    {
        PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return;
    }

    for (pos = 0; pos < osip_list_size(g_PlatformSrvProcThreadList->pPlatformSrvProcList); pos++)
    {
        runthread = (platform_srv_proc_tl_t*)osip_list_get(g_PlatformSrvProcThreadList->pPlatformSrvProcList, pos);

        if (runthread == NULL)
        {
            continue;
        }

        if (0 == type) /* 显示未使用的 */
        {
            if (1 == runthread->iUsed)
            {
                continue;
            }
        }
        else if (1 == type) /* 显示已经使用的 */
        {
            if (0 == runthread->iUsed)
            {
                continue;
            }
        }
        else if (2 == type) /* 显示全部 */
        {

        }

        i = format_time(runthread->run_time, strRunTime);
        i = format_time(runthread->iLastTaskRunTime, strLastTaskRunTime);
        i = format_time(runthread->iLastGetTaskTime, strLastTaskGetTime);

        snprintf(rbuf, 256, "\r%-12u  %-10u  %-9u  %-11u  %-15s %-19s %-19s %-19s %-18d\r\n", pos, *(runthread->thread), runthread->iUsed, runthread->platform_index, runthread->platform_ip, strRunTime, strLastTaskRunTime, strLastTaskGetTime, runthread->iCompressTaskStatus);

        if (sock > 0)
        {
            send(sock, rbuf, strlen(rbuf), 0);
        }

    }

    PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

void set_platform_srv_proc_thread_compress_task_status(int status)
{
    int pos = 0;
    platform_srv_proc_tl_t* pThreadProc = NULL;

    PLATFORM_SRV_PROC_THREAD_SMUTEX_LOCK();

    if (NULL == g_PlatformSrvProcThreadList || osip_list_size(g_PlatformSrvProcThreadList->pPlatformSrvProcList) <= 0)
    {
        PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();
        return;
    }

    for (pos = 0; pos < osip_list_size(g_PlatformSrvProcThreadList->pPlatformSrvProcList); pos++)
    {
        pThreadProc = (platform_srv_proc_tl_t*)osip_list_get(g_PlatformSrvProcThreadList->pPlatformSrvProcList, pos);

        if (NULL == pThreadProc)
        {
            printf("set_platform_srv_proc_thread_compress_task_status() ThreadProc NULL, pos=%d\r\n", pos);
            continue;
        }

        if (0 == pThreadProc->iUsed)
        {
            printf("set_platform_srv_proc_thread_compress_task_status() iUsed 0, pos=%d\r\n", pos);
            continue;
        }

        if (1 == pThreadProc->th_exit)
        {
            printf("set_platform_srv_proc_thread_compress_task_status() th_exit 1, pos=%d\r\n", pos);
            continue;
        }

        if (pThreadProc->platform_index <= 0)
        {
            printf("set_platform_srv_proc_thread_compress_task_status() platform_index NULL, pos=%d\r\n", pos);
            continue;
        }

        if (pThreadProc->platform_ip[0] == '\0')
        {
            printf("set_platform_srv_proc_thread_compress_task_status() platform_ip NULL, pos=%d\r\n", pos);
            continue;
        }

        pThreadProc->iCompressTaskStatus = status;
        printf("set_platform_srv_proc_thread_compress_task_status() platform_index=%u, CompressTaskStatus=%d\r\n", pThreadProc->platform_index, pThreadProc->iCompressTaskStatus);
    }

    PLATFORM_SRV_PROC_THREAD_SMUTEX_UNLOCK();

    return;
}

/*****************************************************************************
 Prototype    : get_compress_task_from_webservice_thread_execute
 Description  : 通过WebServer接口从公安管理平台获取任务列表主线程
 Input        : void * p
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2016/4/25
    Author       : yanghaifeng
    Modification : Created function

*****************************************************************************/
int get_compress_task_from_webservice_thread_execute(void* p)
{
    int iRet = 0;
    platform_srv_proc_tl_t* run = (platform_srv_proc_tl_t*)p;
    static int check_interval = 0;
    int iBeginTime = 0;
    int iEndTime = 0;

    if (run == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "get_compress_task_from_webservice_thread_execute() exit---: Param Error \r\n");
        return -1;
    }

    if (1 == run->iCompressTaskStatus) /* 准备获取 */
    {
        /* 删除上次的任务信息, 删除上次的任务数据库表 */
        //iRet = DeleteCompressTask(run->platform_ip, run->pPlatform_Srv_dboper);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "get_compress_task_from_webservice_thread_execute() : DeleteCompressTask:platform_ip=%s, iRet=%d \r\n", run->platform_ip, iRet);

        /* 1、获取 */

        /* 确定时间 */
        if (1 == run->iTaskMode) /* 手动模式 */
        {
            if (run->iLastGetTaskTime < run->run_time) /* 上次获取的时间小于当前时间 */
            {
                iBeginTime = run->iLastGetTaskTime;
                iEndTime = iBeginTime + run->iTaskGetInterval; /* 一次获取半个小时 */

                if (run->iTaskEndTime > 0) /* 有截止时间 */
                {
                    if (iEndTime > run->iTaskEndTime)
                    {
                        iEndTime = run->iTaskEndTime;
                    }
                }
                else
                {
                    if (iEndTime > run->run_time)
                    {
                        iEndTime = run->run_time;
                    }
                }
            }
            else /* 上次获取的时间已经跑到当前时间之后 */
            {
                if (run->run_time - run->iLastGetTaskTime >= run->iTaskRunInterval)
                {
                    iBeginTime = run->iLastGetTaskTime;
                    iEndTime = iBeginTime + run->iTaskRunInterval; /* 超过时间之后就按照运行间隔时间获取 */
                }
                else
                {
                    run->iLastGetTaskTime = run->run_time;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上次获取的时间已经跑到当前时间之后, 等待下次获取时间间隔达到运行间隔时间, 平台IP=%s", run->platform_ip);
                    return 0;
                }
            }

            run->iLastGetTaskTime = iEndTime;
        }
        else
        {
            run->iLastGetTaskTime = run->run_time;

            iEndTime = run->iLastGetTaskTime;
            iBeginTime = iEndTime - run->iTaskRunInterval; /* 一次获取十分钟 */
        }

        run->iCompressTaskStatus = 2;

        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() iBeginTime=%d, iEndTime=%d\r\n", iBeginTime, iEndTime);

        /* 更新到数据库 */
        UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);

        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() get_compress_task_from_webservice_proc:Begin---\r\n");

        iRet = get_compress_task_from_webservice_proc(run->platform_ip, iBeginTime, iEndTime, run->pPlatform_Srv_dboper);

        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() get_compress_task_from_webservice_proc:End---:iRet=%d\r\n", iRet);

        if (0 != iRet)
        {
            run->iCompressTaskStatus = 3;

            /* 更新到数据库 */
            UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, iRet, run->pPlatform_Srv_dboper);
        }
        else
        {
            run->iCompressTaskStatus = 4;
            run->iLastTaskRunTime = run->run_time;

            /* 更新到数据库 */
            UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
        }
    }
    else if (3 == run->iCompressTaskStatus) /* 获取失败 */
    {
        check_interval++;

        /* 每隔30秒重新获取一次 */
        if (check_interval >= 30)
        {
            /* 1、获取 */
            /* 确定时间 */
            if (1 == run->iTaskMode) /* 手动模式 */
            {
                if (run->iLastGetTaskTime < run->run_time) /* 上次获取的时间小于当前时间 */
                {
                    iBeginTime = run->iLastGetTaskTime;
                    iEndTime = iBeginTime + run->iTaskGetInterval; /* 一次获取半个小时 */

                    if (run->iTaskEndTime > 0) /* 有截止时间 */
                    {
                        if (iEndTime > run->iTaskEndTime)
                        {
                            iEndTime = run->iTaskEndTime;
                        }
                    }
                    else
                    {
                        if (iEndTime > run->run_time)
                        {
                            iEndTime = run->run_time;
                        }
                    }
                }
                else /* 上次获取的时间已经跑到当前时间之后 */
                {
                    if (run->run_time - run->iLastGetTaskTime >= run->iTaskRunInterval)
                    {
                        iBeginTime = run->iLastGetTaskTime;
                        iEndTime = iBeginTime + run->iTaskRunInterval; /* 超过时间之后就按照运行间隔时间获取 */
                    }
                    else
                    {
                        run->iLastGetTaskTime = run->run_time;
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上次获取的时间已经跑到当前时间之后, 等待下次获取时间间隔达到运行间隔时间, 平台IP=%s", run->platform_ip);
                        return 0;
                    }
                }
            }
            else
            {
                iEndTime = run->iLastGetTaskTime;
                iBeginTime = iEndTime - run->iTaskRunInterval; /* 一次获取十分钟 */

                run->iLastGetTaskTime = iEndTime;
            }

            run->iCompressTaskStatus = 2;

            /* 更新到数据库 */
            UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() get_compress_task_from_webservice_proc:Begin---\r\n");

            iRet = get_compress_task_from_webservice_proc(run->platform_ip, iBeginTime, iEndTime, run->pPlatform_Srv_dboper);

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() get_compress_task_from_webservice_proc:End---:iRet=%d\r\n", iRet);

            if (0 != iRet)
            {
                run->iCompressTaskStatus = 3;

                /* 更新到数据库 */
                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, iRet, run->pPlatform_Srv_dboper);
            }
            else
            {
                run->iCompressTaskStatus = 4;
                run->iLastTaskRunTime = run->run_time;

                /* 更新到数据库 */
                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
            }

            check_interval = 0;
        }
    }
    else if (4 == run->iCompressTaskStatus) /* 准备分配 */
    {
        /* 2  下发任务到ZRV进行压缩 */
        run->iCompressTaskStatus = 5;

        /* 更新到数据库 */
        UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);

        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() assign_compress_task_to_zrv_device_proc:Begin---\r\n");
        iRet = assign_compress_task_to_zrv_device_proc(run->platform_ip, run->pPlatform_Srv_dboper, (int*) & (run->run_time));
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() assign_compress_task_to_zrv_device_proc:End---:iRet=%d\r\n", iRet);

        if (0 != iRet)
        {
            if (FILE_COMPRESS_COMPRESS_TASK_COUNT_ERROR == iRet) /* 没有任务 */
            {
                run->iCompressTaskStatus = 7;
            }
            else
            {
                run->iCompressTaskStatus = 6;
            }

            /* 更新到数据库 */
            UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, iRet, run->pPlatform_Srv_dboper);
        }
        else
        {
            run->iCompressTaskStatus = 7;

            /* 更新到数据库 */
            UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
        }

        run->iLastTaskRunTime = run->run_time;
    }
    else if (6 == run->iCompressTaskStatus) /* 分配失败 */
    {
        check_interval++;

        /* 每隔30秒重新分配一次 */
        if (check_interval >= 30)
        {
            /* 2  下发任务到ZRV进行压缩 */
            run->iCompressTaskStatus = 5;

            /* 更新到数据库 */
            UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() assign_compress_task_to_zrv_device_proc:Begin---\r\n");
            iRet = assign_compress_task_to_zrv_device_proc(run->platform_ip, run->pPlatform_Srv_dboper, (int*) & (run->run_time));
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() assign_compress_task_to_zrv_device_proc:End---:iRet=%d\r\n", iRet);

            if (0 != iRet)
            {
                if (FILE_COMPRESS_COMPRESS_TASK_COUNT_ERROR == iRet)
                {
                    run->iCompressTaskStatus = 7;
                }
                else
                {
                    run->iCompressTaskStatus = 6;
                }

                /* 更新到数据库 */
                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, iRet, run->pPlatform_Srv_dboper);
            }
            else
            {
                run->iCompressTaskStatus = 7;

                /* 更新到数据库 */
                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
            }

            run->iLastTaskRunTime = run->run_time;
            check_interval = 0;
        }
    }

    return iRet;
}

int UpdatePlatFormCompressTaskStatusToDB(char* platform_ip, int iLastGetTaskTime, int iCompressTaskStatus, int iErrorCode, DBOper* pDbOper)
{
    int iRet = 0;
    string strUpdateSQL = "";

    char strLastTaskTime[32] = {0};
    char strCompressTaskStatus[32] = {0};
    char strErrorCode[32] = {0};

    if (NULL == platform_ip || NULL == pDbOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "UpdatePlatFormCompressTaskStatusToDB() exit---:  Param Error \r\n");
        return -1;
    }

    if (platform_ip[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "UpdatePlatFormCompressTaskStatusToDB() exit---:  platform_ip NULL \r\n");
        return -1;
    }

    snprintf(strLastTaskTime, 32, "%d", iLastGetTaskTime);
    snprintf(strCompressTaskStatus, 32, "%d", iCompressTaskStatus);
    snprintf(strErrorCode, 32, "%d", iErrorCode);

    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE VideoManagePlatformInfo SET";

    strUpdateSQL += " LastTaskTime = ";
    strUpdateSQL += strLastTaskTime;
    strUpdateSQL += ",";

    strUpdateSQL += " CompressTaskStatus = ";
    strUpdateSQL += strCompressTaskStatus;
    strUpdateSQL += ",";

    strUpdateSQL += " ErrorCode = ";
    strUpdateSQL += strErrorCode;

    strUpdateSQL += " WHERE PlatformIP like '";
    strUpdateSQL += platform_ip;
    strUpdateSQL += "'";

    iRet = pDbOper->DB_Update(strUpdateSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdatePlatFormCompressTaskStatusToDB() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdatePlatFormCompressTaskStatusToDB() ErrorMsg=%s\r\n", pDbOper->GetLastDbErrorMsg());
        return -1;
    }

    return iRet;
}
#endif
