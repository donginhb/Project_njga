
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
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern char g_StrCon[2][100];
extern char g_StrConLog[2][100];
extern int cms_run_status;  /* 0:û������,1:�������� */

/*----------------------------------------------*
 * �ⲿ����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ȫ�ֱ���                                     *
 *----------------------------------------------*/
#define MAX_PLATFORM_SRV_THREADS 10

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/
platform_srv_proc_tl_list_t* g_PlatformSrvProcThreadList = NULL;            /* �ϼ�·��ҵ�����̶߳��� */

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
#if DECS("�ϼ�·��ҵ�����߳�")

/*****************************************************************************
 �� �� ��  : get_platform_thread_info_from_db
 ��������  : �����ݿ��л�ȡƽ̨��Ϣ
 �������  : platform_srv_proc_tl_t* plat_thread
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��8��27��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ִ��ģʽ:0:�Զ�ִ��, 1:�ֶ�ִ��, Ĭ��Ϊ0 */
    task_mode = 0;
    plat_thread->pPlatform_Srv_dboper->GetFieldValue("TaskMode", task_mode);

    if (plat_thread->iTaskMode != task_mode)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ϵͳ��⵽ѹ������ģʽ�����仯, ��ģʽ=%d, ��ģʽ=%d, ƽ̨IP=%s", plat_thread->iTaskMode, task_mode, plat_thread->platform_ip);
        plat_thread->iTaskMode = task_mode;
    }

    /* ����ִ�м��ʱ��, ��λ��, Ĭ��Ϊ1800 */
    task_run_interval = 0;
    plat_thread->pPlatform_Srv_dboper->GetFieldValue("TaskRunInterval", task_run_interval);

    if (plat_thread->iTaskRunInterval != task_run_interval)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ϵͳ��⵽ѹ����������ʱ���������仯, �ϼ��=%d, �¼��=%d, ƽ̨IP=%s", plat_thread->iTaskRunInterval, task_run_interval, plat_thread->platform_ip);
        plat_thread->iTaskRunInterval = task_run_interval;
    }

    /* �����ȡ���ʱ��, ��λ��, Ĭ��Ϊ1800 */
    task_get_interval = 0;
    plat_thread->pPlatform_Srv_dboper->GetFieldValue("TaskGetInterval", task_get_interval);

    if (plat_thread->iTaskGetInterval != task_get_interval)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ϵͳ��⵽ѹ�������ȡʱ���������仯, �ϼ��=%d, �¼��=%d, ƽ̨IP=%s", plat_thread->iTaskGetInterval, task_get_interval, plat_thread->platform_ip);
        plat_thread->iTaskGetInterval = task_get_interval;
    }

    /* �ֶ�����ִ�л�ȡ��ʼʱ�� */
    task_begin_time = 0;
    plat_thread->pPlatform_Srv_dboper->GetFieldValue("TaskBeginTime", task_begin_time);

    if (plat_thread->iTaskBeginTime != task_begin_time)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ϵͳ��⵽ѹ������ʼʱ�䷢���仯, �Ͽ�ʼʱ��=%d, �¿�ʼʱ��=%d, ƽ̨IP=%s", plat_thread->iTaskBeginTime, task_begin_time, plat_thread->platform_ip);
        plat_thread->iTaskBeginTime = task_begin_time;
    }

    /* �ֶ�����ִ�л�ȡ����ʱ�� */
    task_end_time = 0;
    plat_thread->pPlatform_Srv_dboper->GetFieldValue("TaskEndTime", task_end_time);

    if (plat_thread->iTaskEndTime != task_end_time)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ϵͳ��⵽ѹ���������ʱ�䷢���仯, �Ͻ���ʱ��=%d, �½���ʱ��=%d, ƽ̨IP=%s", plat_thread->iTaskEndTime, task_end_time, plat_thread->platform_ip);
        plat_thread->iTaskEndTime = task_end_time;
    }

    /* �ϴλ�ȡ����ִ��ʱ�� */
    last_task_time = 0;
    plat_thread->pPlatform_Srv_dboper->GetFieldValue("LastTaskTime", last_task_time);

    if (plat_thread->iLastGetTaskTime != last_task_time)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ϵͳ��⵽ѹ����������ȡʱ�䷢�ͱ仯, ��ʱ��=%d, ��ʱ��=%d, ƽ̨IP=%s", plat_thread->iLastGetTaskTime, last_task_time, plat_thread->platform_ip);
        plat_thread->iLastGetTaskTime = last_task_time;
    }

    /* ����״̬:0:��ʼ״̬, 1:׼����ȡ, 2:���ڻ�ȡ����, 3:��ȡʧ��, 4:��ȡ�ɹ���׼������, 5:���ڷ��䴦��, 6:û�з���ɹ�, 7:����ɹ����ȴ���һ���������� */
    compress_task_status = 0;
    plat_thread->pPlatform_Srv_dboper->GetFieldValue("CompressTaskStatus", compress_task_status);
    plat_thread->iCompressTaskStatus = compress_task_status;

    return 0;
}

/*****************************************************************************
 �� �� ��  : platform_srv_proc_thread_for_appoint_execute
 ��������  :  platform_srv_proc_thread_execute
 �������  : appoint_platform_srv_proc_tl_t * platform_srv_proc
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void* platform_srv_proc_thread_execute(void* p)
{
    int iRet = 0;
    int flag = 0;
    int flag2 = 0;
    int db_count = 0;
    platform_srv_proc_tl_t* run = (platform_srv_proc_tl_t*)p;
    vector<string> ZRVDeviceIP;
    int zrvdevice_count = 0;           /* ��¼�� */
    static int check_db_interval = 0;

    int last_task_time = 0;
    int compress_task_status = 0;

    int task_mode = 0;
    int task_begin_time = 0;
    int task_end_time = 0;
    int change_flag = 0;  /* �仯��ʶ */

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

        /* ��ȡ���ݿ� */
        if (8 == run->iCompressTaskStatus) /* ѹ��������ɵ�ʱ�򣬶�ȡ���ݿ������Ƿ�仯 */
        {
            get_platform_thread_info_from_db(run);
        }

        if (1 == run->iTaskMode) /* �ֶ�ģʽ */
        {
            if (0 == run->iLastTaskRunTime) /* ϵͳ��һ���������� */
            {
                run->iLastTaskRunTime = run->run_time;

                iRet = format_time(run->iTaskBeginTime, strTaskBeginTime);
                iRet = format_time(run->iTaskEndTime, strTaskEndTime);
                iRet = format_time(run->iLastGetTaskTime, strLastGetTaskTime);

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ��ǰѹ��ģʽ=%d, ϵͳ��ȡѹ������ʼʱ��=%s, ϵͳ��ȡѹ���������ʱ��=%s, ϵͳ֮ǰ��ȡѹ������ʱ��=%s, ִ��ѹ������״̬=%d, ƽ̨IP=%s", run->iTaskMode, strTaskBeginTime, strTaskEndTime, strLastGetTaskTime, run->iCompressTaskStatus, run->platform_ip);

                if (run->iLastGetTaskTime <= 0) /* ϵͳ֮ǰû��ִ�й����� */
                {
                    // TODO: OK
                    run->iCompressTaskStatus = 1;
                    run->iLastGetTaskTime = run->iTaskBeginTime;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰû��ִ�й�ѹ������, ���ϼ�ƽ̨��ȡѹ��������Ϣ, ƽ̨IP=%s", run->platform_ip);

                    /* ���µ����ݿ� */
                    UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                }
                else if (run->iLastGetTaskTime > 0) /* ϵͳ֮ǰִ�й����� */
                {
                    if (run->iTaskEndTime > 0) /* �н�ֹʱ�� */
                    {
                        if (run->iLastGetTaskTime >= run->iTaskBeginTime
                            && run->iLastGetTaskTime < run->iTaskEndTime) /* ϵͳ��;����֮�󣬻�û��ִ�е����ʱ�� */
                        {
                            if (0 == run->iCompressTaskStatus)
                            {
                                // TODO: OK
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ������ϵͳ״̬���ڳ�ʼ״̬������û�е�����ʱ��, �������»�ȡ, ƽ̨IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* ���µ����ݿ� */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else if (3 == run->iCompressTaskStatus)
                            {
                                // TODO: OK
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ������ͨ��WebService��ȡѹ������ʧ�ܣ�����û�е�����ʱ��, �������»�ȡ, ƽ̨IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* ���µ����ݿ� */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else if (8 == run->iCompressTaskStatus)
                            {
                                // TODO: OK:
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ��������һ��ִ���Ѿ�����������û�е�����ʱ��, �������»�ȡ, ƽ̨IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* ���µ����ݿ� */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else
                            {
                                // TODO: OK
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ�����񣬵��ǻ�û�е�����ʱ��, �ȴ��������, ƽ̨IP=%s", run->platform_ip);
                            }
                        }
                        else if (run->iLastGetTaskTime == run->iTaskEndTime) /* ϵͳ��;����֮�󣬵����Ѿ�ִ�н����� */
                        {
                            if (8 == run->iCompressTaskStatus)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ�������Ѿ��������ʱ��, �ȴ��´�����ʼ, ƽ̨IP=%s", run->platform_ip);
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ�������Ѿ��������ʱ��, �����ϴε�����û�д�����, �ȴ��������, ƽ̨IP=%s", run->platform_ip);
                            }
                        }
                        else /* ʱ��η����仯����֮ǰ������ʱ�䲻ƥ�� */
                        {
                            flag = HasCompressTaskNotComplete(run->platform_ip);

                            if (!flag)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, �ϴ�û��δ��ɵ�����, ���ϼ�ƽ̨��ȡѹ��������Ϣ, ƽ̨IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;
                                run->iLastGetTaskTime = run->iTaskBeginTime;

                                /* ���µ����ݿ� */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ�����񣬵��ǻ���δ��ɵ�����, �ȴ��������, ƽ̨IP=%s", run->platform_ip);
                            }
                        }
                    }
                    else /* û�н�ֹʱ�� */
                    {
                        if (run->iLastGetTaskTime >= run->iTaskBeginTime) /* ϵͳ��;����֮����Ҫ����ִ�� */
                        {
                            if (0 == run->iCompressTaskStatus)
                            {
                                // TODO: OK
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ������ϵͳ״̬���ڳ�ʼ״̬������û�н���ʱ��, �������»�ȡ, ƽ̨IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* ���µ����ݿ� */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else if (3 == run->iCompressTaskStatus)
                            {
                                // TODO: OK
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ������ͨ��WebService��ȡѹ������ʧ�ܣ�����û�н���ʱ��, �������»�ȡ, ƽ̨IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* ���µ����ݿ� */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else if (8 == run->iCompressTaskStatus)
                            {
                                // TODO: OK:
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ��������һ��ִ���Ѿ�����������û�н���ʱ��, �������»�ȡ, ƽ̨IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* ���µ����ݿ� */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else
                            {
                                // TODO: OK
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ�����񣬵��ǻ�û�н���ʱ��, �ȴ��������, ƽ̨IP=%s", run->platform_ip);
                            }
                        }
                        else /* ʱ��η����仯����֮ǰ������ʱ�䲻ƥ�� */
                        {
                            flag = HasCompressTaskNotComplete(run->platform_ip);

                            if (!flag)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, �ϴ�û��δ��ɵ�����, ���¿�ʼִ������, ���ϼ�ƽ̨��ȡѹ��������Ϣ, ƽ̨IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;
                                run->iLastGetTaskTime = run->iTaskBeginTime;

                                /* ���µ����ݿ� */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ�����񣬵��ǻ���δ��ɵ�����, �ȴ��������, ƽ̨IP=%s", run->platform_ip);
                            }
                        }
                    }
                }
            }
            else if (run->iLastTaskRunTime > 0)/* ϵͳ����ִ��֮�� */
            {
                if (run->iLastGetTaskTime <= 0) /* ���¿�ʼ */
                {
                    // TODO: OK
                    run->iCompressTaskStatus = 1;
                    run->iLastGetTaskTime = run->iTaskBeginTime;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ���¿�ʼִ��ѹ������, ���ϼ�ƽ̨��ȡѹ��������Ϣ, ƽ̨IP=%s", run->platform_ip);

                    /* ���µ����ݿ� */
                    UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                }
                else if (run->iLastGetTaskTime > 0)
                {
                    if (run->iTaskEndTime > 0) /* �н�ֹʱ�� */
                    {
                        if (run->iLastGetTaskTime >= run->iTaskBeginTime
                            && run->iLastGetTaskTime < run->iTaskEndTime) /* ������������ʱ��Ͳ�ִ���� */
                        {
                            // TODO: OK:
                            if (8 == run->iCompressTaskStatus)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ���¿�ʼִ��ѹ������, ���ϼ�ƽ̨��ȡѹ��������Ϣ, ƽ̨IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* ���µ����ݿ� */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                        }
                        else if (run->iLastGetTaskTime == run->iTaskEndTime) /* ϵͳ��;����֮�󣬵����Ѿ�ִ�н����� */
                        {
                            // TODO: OK
                            if (8 == run->iCompressTaskStatus)
                            {
                                check_db_interval++;

                                if (check_db_interval >= 5)
                                {
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ֮ǰִ�й�ѹ�������Ѿ��������ʱ��, �ȴ��´�����ʼ, ƽ̨IP=%s", run->platform_ip);
                                    check_db_interval = 0;
                                }
                            }
                            else
                            {
                                check_db_interval++;

                                if (check_db_interval >= 5)
                                {
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ֮ǰִ�й�ѹ�������Ѿ��������ʱ��, ���ǻ���δ��ɵ�����, �ȴ��������, ƽ̨IP=%s", run->platform_ip);
                                    check_db_interval = 0;
                                }
                            }
                        }
                        else /* ʱ��β�ƥ�� */
                        {
                            check_db_interval++;

                            if (check_db_interval >= 5)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ֮ǰִ�й�ѹ���������ִ��ʱ���ʱ��β�ƥ��, �ȴ��´�����ִ��, ƽ̨IP=%s", run->platform_ip);
                                check_db_interval = 0;
                            }
                        }
                    }
                    else /* û�н�ֹʱ�� */
                    {
                        if (run->iLastGetTaskTime >= run->iTaskBeginTime) /* ������������ʱ��Ͳ�ִ���� */
                        {
                            // TODO: OK:
                            if (8 == run->iCompressTaskStatus)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ���¿�ʼִ��ѹ������, ���ϼ�ƽ̨��ȡѹ��������Ϣ, ƽ̨IP=%s", run->platform_ip);
                                run->iCompressTaskStatus = 1;

                                /* ���µ����ݿ� */
                                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                            }
                        }
                        else /* ʱ��β�ƥ�� */
                        {
                            check_db_interval++;

                            if (check_db_interval >= 5)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ֮ǰִ�й�ѹ���������ִ��ʱ���ʱ��β�ƥ��, �ȴ��´�����ִ��, ƽ̨IP=%s", run->platform_ip);
                                check_db_interval = 0;
                            }
                        }
                    }
                }
            }
        }
        else /* �Զ�ģʽ */
        {
            if (0 == run->iLastTaskRunTime)/* ϵͳ��һ���� */
            {
                run->iLastTaskRunTime = run->run_time;

                iRet = format_time(run->iLastGetTaskTime, strLastGetTaskTime);

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ��ǰѹ��ģʽ=%d, ϵͳ֮ǰ��ȡѹ������ʱ��=%s, ִ��ѹ������״̬=%d, ƽ̨IP=%s", run->iTaskMode, strLastGetTaskTime, run->iCompressTaskStatus, run->platform_ip);

                flag = HasCompressTaskNotComplete(run->platform_ip);

                if (!flag)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, �ϴ�û��δ��ɵ�����, ���ϼ�ƽ̨��ȡѹ��������Ϣ, ƽ̨IP=%s", run->platform_ip);
                    run->iCompressTaskStatus = 1;

                    /* ���µ����ݿ� */
                    UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                }
                else
                {
                    if (0 == run->iCompressTaskStatus)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ������ϵͳ״̬���ڳ�ʼ״̬, �������»�ȡ, ƽ̨IP=%s", run->platform_ip);
                        run->iCompressTaskStatus = 1;

                        /* ���µ����ݿ� */
                        UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                    }
                    else if (3 == run->iCompressTaskStatus)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ������ͨ��WebService��ȡѹ������ʧ��, �������»�ȡ, ƽ̨IP=%s", run->platform_ip);
                        run->iCompressTaskStatus = 1;

                        /* ���µ����ݿ� */
                        UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                    }
                    else if (8 == run->iCompressTaskStatus)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ��������һ��ִ���Ѿ�����, �������»�ȡ, ƽ̨IP=%s", run->platform_ip);
                        run->iCompressTaskStatus = 1;

                        /* ���µ����ݿ� */
                        UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ����, ϵͳ֮ǰִ�й�ѹ�����񣬵��ǻ�û�н���, �ȴ��������, ƽ̨IP=%s", run->platform_ip);
                    }
                }
            }
            else if (run->iLastTaskRunTime > 0) /* ϵͳ����֮�� */
            {
                if (8 == run->iCompressTaskStatus)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ���¿�ʼִ��ѹ������, ���ϼ�ƽ̨��ȡѹ��������Ϣ, ƽ̨IP=%s", run->platform_ip);
                    run->iCompressTaskStatus = 1;

                    /* ���µ����ݿ� */
                    UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                }
            }
        }

        iRet = get_compress_task_from_webservice_thread_execute(run);

        /* �жϸ���״̬�µ���� */
        if (0 == run->iCompressTaskStatus)
        {
            if (run->run_time - run->iLastTaskRunTime >= run->iTaskRunInterval) /* ���ó�ʱ */
            {
                run->iCompressTaskStatus = 8;
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�����̳߳�ʼ״̬����ﵽ%d��, ����������ȡѹ������, ƽ̨IP=%s", run->iTaskRunInterval, run->platform_ip);

                /* ���µ����ݿ� */
                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
            }
        }
        else if (3 == run->iCompressTaskStatus)
        {
            if (run->run_time - run->iLastTaskRunTime >= run->iTaskRunInterval) /* ���ó�ʱ */
            {
                run->iCompressTaskStatus = 8;
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϴλ�ȡѹ��������Ϣʧ�ܼ���ﵽ%d��, ����������ȡѹ������, ƽ̨IP=%s", run->iTaskRunInterval, run->platform_ip);

                /* ���µ����ݿ� */
                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
            }
        }
        else if (7 == run->iCompressTaskStatus) /* ������ɵ�����£��鿴�����Ƿ�������߳�ʱ */
        {
            if (1 == run->iTaskMode)
            {
                check_db_interval++;

                if (check_db_interval >= 5)
                {
                    /* �ж�һ���Ƿ��еȴ������������,���û�У�������ִ����һ�ֲ��� */
                    flag2 = HasCompressTaskNotComplete2(run->platform_ip);

                    if (!flag2)
                    {
                        run->iCompressTaskStatus = 8;
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϴ�ѹ�������Ѿ���ǰ���, ������һ��ѹ������, ƽ̨IP=%s", run->platform_ip);

                        /* ���µ����ݿ� */
                        UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
                    }

                    check_db_interval = 0;
                }
            }

            if (run->run_time - run->iLastTaskRunTime >= run->iTaskRunInterval) /* ���ó�ʱ */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϴλ�ȡѹ��������Ϣ�ﵽ%d��, ������һ��ѹ������, ƽ̨IP=%s", run->iTaskRunInterval, run->platform_ip);

                /* ����û�н��������ʱ */
                //update_compress_task_list_for_wait_expire(run->pPlatform_Srv_dboper);

                run->iCompressTaskStatus = 8;

                /* ���µ����ݿ� */
                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
            }
        }

        osip_usleep(1000000);
    }

    return NULL;
}

/*****************************************************************************
 �� �� ��  : platform_srv_proc_thread_init
 ��������  : �ϼ�·��ҵ�����̳߳�ʼ��
 �������  : platform_srv_proc_tl_t** run
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : platform_srv_proc_thread_free
 ��������  : �ϼ�·��ҵ�����߳��ͷ�
 �������  : platform_srv_proc_tl_t *run
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

#if DECS("�ϼ�·��ҵ�����̶߳���")
/*****************************************************************************
 �� �� ��  : platform_srv_proc_thread_assign
 ��������  : �ϼ�·��ҵ�����̷߳���
 �������  : platform_info_t* pPlatformInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��27��
    ��    ��   : �ϼ�·��·����Ϣ����
    �޸�����   : �����ɺ���

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

    /* ��ʼ�����ݿ����� */
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

    /* ��ʼ����־���ݿ����� */
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

    /* ����һ���ϴ�û����ɵ����� */
    set_db_data_to_compress_task_list(runthread->platform_ip, runthread->pPlatform_Srv_dboper);

    /* ��ȡ�ϴε�������Ϣ */
    get_platform_thread_info_from_db(runthread);
    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "platform_srv_proc_thread_assign() iTaskMode=%d, iTaskBeginTime=%d, iTaskEndTime=%d \r\n", runthread->iTaskMode, runthread->iTaskBeginTime, runthread->iTaskEndTime);

    //DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "platform_srv_proc_thread_assign() Exit--- \r\n");
    //printf("\r\n platform_srv_proc_thread_assign() Exit--- \r\n");
    return 0;
}

/*****************************************************************************
 �� �� ��  : platform_srv_proc_thread_recycle
 ��������  : �ϼ�·��ҵ�����̻߳���
 �������  : unsigned int platform_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��27��
    ��    ��   : �ϼ�·��·����Ϣ����
    �޸�����   : �����ɺ���

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
 �� �� ��  : platform_srv_proc_thread_start_all
 ��������  : �����ϼ�·��ҵ�����߳�
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

        //��ӵ��ϼ�·��ҵ�����̶߳���
        i = osip_list_add(g_PlatformSrvProcThreadList->pPlatformSrvProcList, runthread, -1); /* add to list tail */
        //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "platform_srv_proc_thread_start() osip_list_add:i=%d \r\n", i);

        if (i < 0)
        {
            platform_srv_proc_thread_free(runthread);
            runthread = NULL;
            //DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "platform_srv_proc_thread_start() exit---: List Add Error \r\n");
            continue;
        }

        //���������߳�
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
 �� �� ��  : platform_srv_proc_thread_stop_all
 ��������  : ֹͣ�����ϼ�·��ҵ�����߳�
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

    //���Ҷ��У�ֹͣ�߳�
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
 �� �� ��  : platform_srv_proc_thread_find
 ��������  : �����ϼ�·��ҵ�����߳�
 �������  : unsigned int platform_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

    //���Ҷ���
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
 �� �� ��  : get_free_platform_srv_proc_thread
 ��������  : ��ȡ���е��ϼ�·��ҵ�����߳�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��27��
    ��    ��   : �ϼ�·��·����Ϣ����
    �޸�����   : �����ɺ���

*****************************************************************************/
platform_srv_proc_tl_t* get_free_platform_srv_proc_thread()
{
    int pos = 0;
    platform_srv_proc_tl_t* runthread = NULL;

    //���Ҷ��У���sipudp�ӽ����̶߳������Ƴ�
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
 �� �� ��  : get_platform_srv_proc_thread
 ��������  : ��ȡ�ϼ�·��ҵ�����߳�
 �������  : unsigned int platform_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��27��
    ��    ��   : �ϼ�·��·����Ϣ����
    �޸�����   : �����ɺ���

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
 �� �� ��  : get_platform_srv_proc_thread2
 ��������  : ��ȡ�ϼ�·��ҵ�����߳�
 �������  : unsigned int platform_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��13�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : platform_srv_proc_thread_list_init
 ��������  : ��ʼ���ϼ�·��ҵ�����̶߳���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : platform_srv_proc_thread_list_free
 ��������  : �ͷ��ϼ�·��ҵ�����̶߳���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : platform_srv_proc_thread_list_lock
 ��������  : �ϼ�·��ҵ�����̶߳��м���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��20�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : platform_srv_proc_thread_list_unlock
 ��������  : �ϼ�·��ҵ�����̶߳��н���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��20�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : debug_platform_srv_proc_thread_list_lock
 ��������  : �ϼ�·��ҵ�����̶߳��м���
 �������  : const char* file
             int line
             const char* func
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��12�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : debug_platform_srv_proc_thread_list_unlock
 ��������  : �ϼ�·��ҵ�����̶߳��н���
 �������  : const char* file
             int line
             const char* func
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��12�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : scan_platform_srv_proc_thread_list
 ��������  : ɨ���ϼ�·��ҵ�����̶߳���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��12�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ������Ҫ��ʼ�� */
    while (!needToProc.empty())
    {
        pThreadProc = (platform_srv_proc_tl_t*) needToProc.front();
        needToProc.pop_front();

        if (NULL != pThreadProc)
        {
            //iRet = platform_srv_proc_thread_restart(pThreadProc->pPlatformInfo);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�ϼ�·��ҵ�����̼߳���̼߳�⵽�ϼ�ƽ̨�����̹߳���, �ϼ�ƽ̨ID=%u, �ϼ�ƽ̨IP=%s", pThreadProc->platform_index, pThreadProc->platform_ip);
            osip_usleep(5000000);
            system((char*)"killall cms; killall -9 cms");
        }
    }

    needToProc.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : show_platform_srv_proc_thread
 ��������  : ��ʾ�ϼ�·���̵߳�ʹ�����
 �������  : int sock
             int type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��8��6�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

        if (0 == type) /* ��ʾδʹ�õ� */
        {
            if (1 == runthread->iUsed)
            {
                continue;
            }
        }
        else if (1 == type) /* ��ʾ�Ѿ�ʹ�õ� */
        {
            if (0 == runthread->iUsed)
            {
                continue;
            }
        }
        else if (2 == type) /* ��ʾȫ�� */
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
 Description  : ͨ��WebServer�ӿڴӹ�������ƽ̨��ȡ�����б����߳�
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

    if (1 == run->iCompressTaskStatus) /* ׼����ȡ */
    {
        /* ɾ���ϴε�������Ϣ, ɾ���ϴε��������ݿ�� */
        //iRet = DeleteCompressTask(run->platform_ip, run->pPlatform_Srv_dboper);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "get_compress_task_from_webservice_thread_execute() : DeleteCompressTask:platform_ip=%s, iRet=%d \r\n", run->platform_ip, iRet);

        /* 1����ȡ */

        /* ȷ��ʱ�� */
        if (1 == run->iTaskMode) /* �ֶ�ģʽ */
        {
            if (run->iLastGetTaskTime < run->run_time) /* �ϴλ�ȡ��ʱ��С�ڵ�ǰʱ�� */
            {
                iBeginTime = run->iLastGetTaskTime;
                iEndTime = iBeginTime + run->iTaskGetInterval; /* һ�λ�ȡ���Сʱ */

                if (run->iTaskEndTime > 0) /* �н�ֹʱ�� */
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
            else /* �ϴλ�ȡ��ʱ���Ѿ��ܵ���ǰʱ��֮�� */
            {
                if (run->run_time - run->iLastGetTaskTime >= run->iTaskRunInterval)
                {
                    iBeginTime = run->iLastGetTaskTime;
                    iEndTime = iBeginTime + run->iTaskRunInterval; /* ����ʱ��֮��Ͱ������м��ʱ���ȡ */
                }
                else
                {
                    run->iLastGetTaskTime = run->run_time;
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϴλ�ȡ��ʱ���Ѿ��ܵ���ǰʱ��֮��, �ȴ��´λ�ȡʱ�����ﵽ���м��ʱ��, ƽ̨IP=%s", run->platform_ip);
                    return 0;
                }
            }

            run->iLastGetTaskTime = iEndTime;
        }
        else
        {
            run->iLastGetTaskTime = run->run_time;

            iEndTime = run->iLastGetTaskTime;
            iBeginTime = iEndTime - run->iTaskRunInterval; /* һ�λ�ȡʮ���� */
        }

        run->iCompressTaskStatus = 2;

        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() iBeginTime=%d, iEndTime=%d\r\n", iBeginTime, iEndTime);

        /* ���µ����ݿ� */
        UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);

        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() get_compress_task_from_webservice_proc:Begin---\r\n");

        iRet = get_compress_task_from_webservice_proc(run->platform_ip, iBeginTime, iEndTime, run->pPlatform_Srv_dboper);

        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() get_compress_task_from_webservice_proc:End---:iRet=%d\r\n", iRet);

        if (0 != iRet)
        {
            run->iCompressTaskStatus = 3;

            /* ���µ����ݿ� */
            UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, iRet, run->pPlatform_Srv_dboper);
        }
        else
        {
            run->iCompressTaskStatus = 4;
            run->iLastTaskRunTime = run->run_time;

            /* ���µ����ݿ� */
            UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
        }
    }
    else if (3 == run->iCompressTaskStatus) /* ��ȡʧ�� */
    {
        check_interval++;

        /* ÿ��30�����»�ȡһ�� */
        if (check_interval >= 30)
        {
            /* 1����ȡ */
            /* ȷ��ʱ�� */
            if (1 == run->iTaskMode) /* �ֶ�ģʽ */
            {
                if (run->iLastGetTaskTime < run->run_time) /* �ϴλ�ȡ��ʱ��С�ڵ�ǰʱ�� */
                {
                    iBeginTime = run->iLastGetTaskTime;
                    iEndTime = iBeginTime + run->iTaskGetInterval; /* һ�λ�ȡ���Сʱ */

                    if (run->iTaskEndTime > 0) /* �н�ֹʱ�� */
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
                else /* �ϴλ�ȡ��ʱ���Ѿ��ܵ���ǰʱ��֮�� */
                {
                    if (run->run_time - run->iLastGetTaskTime >= run->iTaskRunInterval)
                    {
                        iBeginTime = run->iLastGetTaskTime;
                        iEndTime = iBeginTime + run->iTaskRunInterval; /* ����ʱ��֮��Ͱ������м��ʱ���ȡ */
                    }
                    else
                    {
                        run->iLastGetTaskTime = run->run_time;
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�ϴλ�ȡ��ʱ���Ѿ��ܵ���ǰʱ��֮��, �ȴ��´λ�ȡʱ�����ﵽ���м��ʱ��, ƽ̨IP=%s", run->platform_ip);
                        return 0;
                    }
                }
            }
            else
            {
                iEndTime = run->iLastGetTaskTime;
                iBeginTime = iEndTime - run->iTaskRunInterval; /* һ�λ�ȡʮ���� */

                run->iLastGetTaskTime = iEndTime;
            }

            run->iCompressTaskStatus = 2;

            /* ���µ����ݿ� */
            UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() get_compress_task_from_webservice_proc:Begin---\r\n");

            iRet = get_compress_task_from_webservice_proc(run->platform_ip, iBeginTime, iEndTime, run->pPlatform_Srv_dboper);

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() get_compress_task_from_webservice_proc:End---:iRet=%d\r\n", iRet);

            if (0 != iRet)
            {
                run->iCompressTaskStatus = 3;

                /* ���µ����ݿ� */
                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, iRet, run->pPlatform_Srv_dboper);
            }
            else
            {
                run->iCompressTaskStatus = 4;
                run->iLastTaskRunTime = run->run_time;

                /* ���µ����ݿ� */
                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
            }

            check_interval = 0;
        }
    }
    else if (4 == run->iCompressTaskStatus) /* ׼������ */
    {
        /* 2  �·�����ZRV����ѹ�� */
        run->iCompressTaskStatus = 5;

        /* ���µ����ݿ� */
        UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);

        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() assign_compress_task_to_zrv_device_proc:Begin---\r\n");
        iRet = assign_compress_task_to_zrv_device_proc(run->platform_ip, run->pPlatform_Srv_dboper, (int*) & (run->run_time));
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_compress_task_from_webservice_thread_execute() assign_compress_task_to_zrv_device_proc:End---:iRet=%d\r\n", iRet);

        if (0 != iRet)
        {
            if (FILE_COMPRESS_COMPRESS_TASK_COUNT_ERROR == iRet) /* û������ */
            {
                run->iCompressTaskStatus = 7;
            }
            else
            {
                run->iCompressTaskStatus = 6;
            }

            /* ���µ����ݿ� */
            UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, iRet, run->pPlatform_Srv_dboper);
        }
        else
        {
            run->iCompressTaskStatus = 7;

            /* ���µ����ݿ� */
            UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, 0, run->pPlatform_Srv_dboper);
        }

        run->iLastTaskRunTime = run->run_time;
    }
    else if (6 == run->iCompressTaskStatus) /* ����ʧ�� */
    {
        check_interval++;

        /* ÿ��30�����·���һ�� */
        if (check_interval >= 30)
        {
            /* 2  �·�����ZRV����ѹ�� */
            run->iCompressTaskStatus = 5;

            /* ���µ����ݿ� */
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

                /* ���µ����ݿ� */
                UpdatePlatFormCompressTaskStatusToDB(run->platform_ip, run->iLastGetTaskTime, run->iCompressTaskStatus, iRet, run->pPlatform_Srv_dboper);
            }
            else
            {
                run->iCompressTaskStatus = 7;

                /* ���µ����ݿ� */
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
