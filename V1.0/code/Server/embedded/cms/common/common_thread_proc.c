
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
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern char g_StrConLog[2][100];
extern char g_StrCon[2][100];

extern int cms_run_status;  /* 0:û������,1:�������� */
//extern char log_file_time[20];
extern cms_log_t* pGCMSLog;       /* ��־�ļ���¼ */
extern int g_IsSubscribe;         /* �Ƿ��Ͷ��ģ�Ĭ��0 */

extern user_reg_msg_queue g_UserRegMsgQueue;                /* �û�ע����Ϣ���� */
extern user_reg_msg_queue g_UserUnRegMsgQueue;              /* �û�ȥע����Ϣ���� */
extern user_log2db_queue g_UserLog2DBQueue;                 /* �û���־��¼�����ݿ���Ϣ���� */
extern record_srv_msg_queue g_RecordSrvMsgQueue;            /* ¼��ҵ����Ϣ���� */
extern GBDevice_reg_msg_queue g_GBDeviceRegMsgQueue;        /* ��׼�����豸ע����Ϣ���� */
extern GBDevice_reg_msg_queue g_GBDeviceUnRegMsgQueue;      /* ��׼�����豸ע����Ϣ���� */
extern device_srv_msg_queue g_DeviceSrvMsgQueue;            /* ��׼�豸ҵ����Ϣ���� */
extern device_srv_msg_queue g_DeviceMessageSrvMsgQueue;     /* ��׼�豸Message��Ϣ���� */
extern diagnosis_msg_queue g_DiagnosisMsgQueue;             /* �����Ϣ���� */
extern route_srv_msg_queue g_RouteSrvMsgQueue;              /* ����·��ҵ����Ϣ���� */
extern route_srv_msg_queue g_RouteMessageSrvMsgQueue;       /* ����·��Messageҵ����Ϣ���� */
extern alarm_msg_queue g_AlarmMsgQueue;                     /* ҵ�񱨾���Ϣ���� */
extern fault_msg_queue g_FaultMsgQueue;                     /* ���ϱ�����Ϣ���� */
extern alarm_duration_list_t* g_AlarmDurationList;          /* ��ʱ������ʱ������ */
extern ack_send_list_t* g_AckSendList;                      /* Ack ������Ϣ���� */
extern transfer_xml_msg_list_t* g_TransferXMLMsgList;       /* ������ת���� XML Message ��Ϣ����*/
extern preset_auto_back_list_t* g_PresetAutoBackList;       /* Ԥ��λ�Զ���λ���� */
extern device_auto_unlock_list_t* g_DeviceAutoUnlockList;   /* ��λ�Զ��������� */
extern tsu_creat_task_result_msg_queue  g_TSUCreatTaskResultMsgQueue; /* TSU ֪ͨ������������Ϣ���� */
extern trace_log2file_queue g_TraceLog2FileQueue;           /* ��־��¼�ļ����� */
extern sip_msg_log2file_queue g_SIPMsgLog2FileQueue;        /* SIP��Ϣ��־��¼�ļ����� */
extern system_log2db_queue g_SystemLog2DBQueue;             /* ϵͳ��־��¼�����ݿ���Ϣ���� */
extern int g_LogQueryBufferSize;                            /* ��־������д�С��Ĭ��MAX_LOG_QUERY_SIZE */

/*----------------------------------------------*
 * �ⲿ����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ȫ�ֱ���                                     *
 *----------------------------------------------*/
int g_GetChannelFlag = 0;

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/
thread_proc_list_t* g_ThreadProcList = NULL;           /* �̴߳������ */

thread_proc_t* g_Log2FileProcThread = NULL;            /* ��¼��־�ļ������߳� */
thread_proc_t* g_Log2DBProcThread = NULL;              /* ��־��¼�����ݿ⴦���߳� */
thread_proc_t* g_ThreadMonitorProcThread = NULL;       /* �̼߳�ش����߳� */

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
#if DECS("�̵߳Ĺ�������")
/*****************************************************************************
 Prototype    : thread_proc_t_init
 Description  : �����̳߳�ʼ��
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
 Description  : �����߳��ͷ�
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
 �� �� ��  : thread_proc_thread_start
 ��������  : �����߳�����
 �������  : proc_thread_type_t eThreadType
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��9�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ���ݿ���� */
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
        /* ��־���ݿ���� */
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
        case THREAD_ZRV_DEVICE_MGN_PROC: /* ZRV�豸�������߳� */
            ptProcThread->thread = (osip_thread_t*)osip_thread_create(20000, zrv_device_mgn_proc_thread_execute, (void*)ptProcThread);
            break;

        case THREAD_ZRV_DEVICE_TCP_MGN_PROC: /* ZRV�豸TCP���ӹ������߳� */
            ptProcThread->thread = (osip_thread_t*)osip_thread_create(20000, zrv_device_tcp_connect_proc_thread_execute, (void*)ptProcThread);
            break;

        case THREAD_ZRV_TASK_RESULT_MESSAGE_PROC: /* ZRV�豸��������Ϣ�ϱ������߳� */
            ptProcThread->thread = (osip_thread_t*)osip_thread_create(20000, zrv_task_result_msg_proc_thread_execute, (void*)ptProcThread);
            break;

        case THREAD_COMPRESS_TASK_MONITOR_PROC: /* ѹ�������ش����߳� */
            ptProcThread->thread = (osip_thread_t*)osip_thread_create(20000, zrv_compress_task_monitor_thread_execute, (void*)ptProcThread);
            break;

        case THREAD_CONFIG_MGN_PROC: /* ���ù������߳� */
            ptProcThread->thread = (osip_thread_t*)osip_thread_create(20000, config_mgn_proc_thread_execute, (void*)ptProcThread);
            break;

        case THREAD_TELNET_CLIENT_MONITOR_PROC: /* Telnet �ͻ��˼�ش����߳� */
            ptProcThread->thread = (osip_thread_t*)osip_thread_create(20000, telnet_client_monitor_thread_execute, (void*)ptProcThread);
            break;

        case THREAD_COMMON_DB_REFRESH_PROC: /* ���������ݿ�ˢ�´����߳� */
            ptProcThread->thread = (osip_thread_t*)osip_thread_create(20000, common_db_refresh_thread_execute, (void*)ptProcThread);
            break;

        case THREAD_PLATFORM_MGN_PROC: /* �ϼ�ƽ̨�����߳� */
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

    /* ��ӵ�����̶߳��� */
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
 �� �� ��  : thread_proc_thread_stop
 ��������  : �����߳�ֹͣ
 �������  : proc_thread_type_t eThreadType
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��9�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* �Ӽ���̶߳����Ƴ� */
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
 �� �� ��  : thread_proc_thread_start_all
 ��������  : ��������ҵ�����߳�
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
int thread_proc_thread_start_all()
{
    int i = 0;

    /* �豸�������߳� */
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

    /*  TSU��Ϣ�����߳� */
    i = thread_proc_thread_start(THREAD_ZRV_TASK_RESULT_MESSAGE_PROC);

    if (i != 0)
    {
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_MGN_PROC);
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_TCP_MGN_PROC);

        DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "thread_proc_thread_start_all() exit---: thread_proc_thread_start THREAD_ZRV_TASK_RESULT_MESSAGE_PROC Error \r\n");
        return -1;
    }

    /* TSUע�ᴦ���߳� */
    i = thread_proc_thread_start(THREAD_COMPRESS_TASK_MONITOR_PROC);

    if (i != 0)
    {
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_MGN_PROC);
        thread_proc_thread_stop(THREAD_ZRV_DEVICE_TCP_MGN_PROC);
        thread_proc_thread_stop(THREAD_ZRV_TASK_RESULT_MESSAGE_PROC);

        DEBUG_TRACE(MODULE_COMMON, LOG_FATAL, "thread_proc_thread_start_all() exit---: thread_proc_thread_start THREAD_COMPRESS_TASK_MONITOR_PROC Error \r\n");
        return -1;
    }

    /* ���ù������߳� */
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

    /* Telnet�ͻ��˼��Ӵ����߳� */
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

    /* ���������ݿ�ˢ�´����߳� */
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

    /* �ϼ�ƽ̨�������߳� */
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
 �� �� ��  : thread_proc_thread_stop_all
 ��������  : ֹͣ����ҵ�����߳�
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

#if DECS("д��־�ļ��߳�")
/*****************************************************************************
 Prototype    : log2file_proc_thread_execute
 Description  : д��־�ļ��������߳�
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

        /* ��������־�ļ� */
        if (tp.tm_min == 0 && tp.tm_sec == 0)
        {
            if (!iSChange)
            {
                memset(log_file_time, 0, 20);
                strftime(log_file_time, 20, "%Y_%m_%d_%H", &tp);

                //SIP��Ϣ
                memset(filename, 0, 256);
                snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_SIP_MSG, log_file_time);

                if (NULL != pGCMSLog->sip_msglog->logfile)
                {
                    fclose(pGCMSLog->sip_msglog->logfile);
                    pGCMSLog->sip_msglog->logfile = NULL;
                }

                pGCMSLog->sip_msglog->logfile = fopen(filename, "w+");

                //SIP������Ϣ
                memset(filename, 0, 256);
                snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_SIP_ERRORMSG, log_file_time);

                if (NULL != pGCMSLog->sip_errorlog->logfile)
                {
                    fclose(pGCMSLog->sip_errorlog->logfile);
                    pGCMSLog->sip_errorlog->logfile = NULL;
                }

                pGCMSLog->sip_errorlog->logfile = fopen(filename, "w+");

                //Debug ������Ϣ
                memset(filename, 0, 256);
                snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_DEBUG, log_file_time);

                if (NULL != pGCMSLog->debug_log->logfile)
                {
                    fclose(pGCMSLog->debug_log->logfile);
                    pGCMSLog->debug_log->logfile = NULL;
                }

                pGCMSLog->debug_log->logfile = fopen(filename, "w+");

                //������Ϣ
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

        /* ɾ���ϵ���־�ļ� */
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

        if (g_LogQueryBufferSize > 0) /* �������0,10����дһ���ļ� */
        {
            check_interval++;

            if (check_interval >= 600)
            {
                scan_trace_log2file_list();
                scan_sip_msg_log2file_list();

                check_interval = 0;
            }
        }
        else  /* 1��дһ���ļ� */
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
 Description  : ����д��־�ļ��߳�
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
 Description  : ֹͣд��־�ļ��߳�
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

#if DECS("��־��¼�����ݿ��߳�")
/*****************************************************************************
 Prototype    : log2db_proc_thread_execute
 Description  : д��־�ļ��������߳�
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

            if (g_LogQueryBufferSize > 0) /* �������0,10����дһ���ļ� */
            {
                check_interval++;

                if (check_interval >= 600)
                {
                    scan_system_log2db_list(run);

                    check_interval = 0;
                }
            }
            else  /* 1��дһ���ļ� */
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
        osip_usleep(100000); /* 100 ���� */
    }

    return NULL;
}

/*****************************************************************************
 Prototype    : log2db_proc_thread_start
 Description  : ����д��־�ļ��߳�
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

    /* ���ݿ���� */
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

    /* ��־���ݿ���� */
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
 Description  : ֹͣ��־��¼�����ݿ��߳�
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

#if DECS("�̼߳���̴߳���")
/*****************************************************************************
 �� �� ��  : thread_proc_thread_start
 ��������  : ���������߳�
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��8�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : thread_proc_thread_stop
 ��������  : ֹͣ�����߳�
 �������  : thread_proc_t* run
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��8�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : thread_monitor_proc_thread_execute
 ��������  : ����̴߳�����
 �������  : void* p
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��8�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
            /* ÿ����ɨ���̶߳��� */
            scan_thread_proc_list();

            /* �ϼ�ƽ̨ҵ������� */
            scan_platform_srv_proc_thread_list();

            /* ����CMS�������ڴ� */
            memset(&proc_mem, 0, sizeof(process_mem_info));
            i = get_progress_memory_usage((char*)"cms", &proc_mem);
            iVMem = osip_atoi(proc_mem.vmem);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "thread_monitor_proc_thread_execute(): CMS Phy Mem=%s, V Mem=%s, iVMem=%d \r\n", proc_mem.mem, proc_mem.vmem, iVMem);

            if (iVMem >= 4194304)/* CMS�ڴ����4G��ʱ���������� */
            {
                now = time(NULL);
                localtime_r(&now, &p_tm);

                if (p_tm.tm_hour >= 1 && p_tm.tm_hour <= 2) /* �賿һ�㵽����֮������ */
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "CMS�����ڴ����4G, CMS�Զ�����, CMS�����ڴ�=%s, CMS�����ڴ�=%s", proc_mem.mem, proc_mem.vmem);
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

#if DECS("���������̶߳��д���")
/*****************************************************************************
 �� �� ��  : thread_proc_list_init
 ��������  : �̴߳�����г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��8�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : thread_proc_list_free
 ��������  : �̴߳�������ͷ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��8�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : thread_proc_list_lock
 ��������  : �̴߳����������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��8�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : thread_proc_list_unlock
 ��������  : �̴߳�����н���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��8�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : debug_thread_proc_list_lock
 ��������  : �̴߳����������
 �������  : const char* file
             int line
             const char* func
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��8�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : debug_thread_proc_list_unlock
 ��������  : �̴߳�����н���
 �������  : const char* file
             int line
             const char* func
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��8�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : thread_proc_find
 ��������  : �����̲߳���
 �������  : proc_thread_type_t eThreadType
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��9�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : thread_proc_add
 ��������  : ��ӵ��̴߳������
 �������  : thread_proc_t* pThreadProc
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��8�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : thread_proc_remove
 ��������  : ���̴߳�������Ƴ�
 �������  : thread_proc_t* pThreadProc
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��8�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : scan_thread_proc_list
 ��������  : ɨ���̴߳������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��8�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_thread_proc_list()
{
    int i = 0;
    int iRet = 0;
    thread_proc_t* pThreadProc = NULL;
    needtoproc_threadproc_queue needToProc;
    time_t now = time(NULL);
    proc_thread_type_t eThreadType = THREAD_NULL;     /* �߳����� */
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

    /* ������Ҫ��ʼ�� */
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

            snprintf(strCMD, 1024, "%s �̼߳���̼߳�⵽�����߳�, Thread Type=%d, �߳�����=%s, �߳��������ʱ��=%s, �̵߳���Ϣ���г���=%d ,�����Զ�����", (char*)"/app/cms_log.sh", eThreadType, strThreadName, strRunTime, iMsgSize);
            system(strCMD);

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�̼߳���̼߳�⵽�����߳�, Thread Type=%d, �߳�����=%s, �߳��������ʱ��=%s, �̵߳���Ϣ���г���=%d ,�����Զ�����", eThreadType, strThreadName, strRunTime, iMsgSize);
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
 �� �� ��  : show_system_proc_thread
 ��������  : ��ʾϵͳ�߳�
 �������  : int sock
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��8��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /* ҵ���߳� */
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

    /* ϵͳ�ļ���־�߳� */
    iRet = format_time(g_Log2FileProcThread->run_time, strRunTime);

    snprintf(rbuf, 256, "\r%-12u  %-10u  %-48s %-19s %-14d\r\n", osip_list_size(g_ThreadProcList->pThreadProcList), *(g_Log2FileProcThread->thread), "ϵͳ�ļ���־�߳�", strRunTime, g_TraceLog2FileQueue.size());

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* ϵͳ���ݿ���־�߳� */
    iRet = format_time(g_Log2DBProcThread->run_time, strRunTime);

    snprintf(rbuf, 256, "\r%-12u  %-10u  %-48s %-19s %-14d\r\n", osip_list_size(g_ThreadProcList->pThreadProcList) + 2, *(g_Log2DBProcThread->thread), "ϵͳ���ݿ���־�߳�", strRunTime, g_SystemLog2DBQueue.size());

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
 �� �� ��  : get_thread_name_by_type
 ��������  : �����߳����ͻ�ȡ�߳�����
 �������  : proc_thread_type_t eThreadType
             char* strThreadName
             int* pMsgSize
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��8��7�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_thread_name_by_type(proc_thread_type_t eThreadType, char* strThreadName, int* pMsgSize)
{
    if (NULL == strThreadName)
    {
        return -1;
    }

    switch (eThreadType)
    {
        case THREAD_ZRV_DEVICE_MGN_PROC: /* ZRV�豸�������߳� */
            snprintf(strThreadName, 128, "%s", (char*)"ZRV�豸�������߳�");
            *pMsgSize = 0;
            break;

        case THREAD_ZRV_DEVICE_TCP_MGN_PROC: /* ZRV�豸TCP���ӹ������߳� */
            snprintf(strThreadName, 128, "%s", (char*)"ZRV�豸TCP���ӹ������߳�");
            *pMsgSize = 0;
            break;

        case THREAD_ZRV_TASK_RESULT_MESSAGE_PROC: /* ZRV�豸�����Ϣ�����߳� */
            snprintf(strThreadName, 128, "%s", (char*)"ZRV�豸�����Ϣ�����߳�");
            *pMsgSize = g_TSUCreatTaskResultMsgQueue.size();
            break;

        case THREAD_COMPRESS_TASK_MONITOR_PROC: /* ѹ�������ش����߳� */
            snprintf(strThreadName, 128, "%s", (char*)"ѹ�������ش����߳�");
            *pMsgSize = 0;
            break;

        case THREAD_CONFIG_MGN_PROC: /* ���ù������߳� */
            snprintf(strThreadName, 128, "%s", (char*)"���ù������߳�");
            *pMsgSize = 0;
            break;

        case THREAD_TELNET_CLIENT_MONITOR_PROC: /* Telnet �ͻ��˼�ش����߳� */
            snprintf(strThreadName, 128, "%s", (char*)"Telnet �ͻ��˼�ش����߳�");
            *pMsgSize = 0;
            break;

        case THREAD_COMMON_DB_REFRESH_PROC: /* ���������ݿ�ˢ�´����߳� */
            snprintf(strThreadName, 128, "%s", (char*)"���������ݿ�ˢ�´����߳�");
            *pMsgSize = 0;
            break;

        case THREAD_PLATFORM_MGN_PROC: /* �ϼ�ƽ̨�������߳� */
            snprintf(strThreadName, 128, "%s", (char*)"�ϼ�ƽ̨�������߳�");
            *pMsgSize = 0;
            break;

        default:
            return -1;
    }

    return 0;
}
#endif

#if DECS("ZRV�豸������")
/*****************************************************************************
 Prototype    : zrv_device_mgn_proc_thread_execute
 Description  : �豸�������������߳�
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

#if DECS("ZRVTCP���ӹ�����")
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

#if DECS("ZRV�豸��������Ϣ�ϱ�������")
/*****************************************************************************
 Prototype    : zrv_task_result_msg_proc_thread_execute
 Description  : ZRV�豸��������Ϣ�ϱ������߳�
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

            /* ɨ��TSUע����Ϣ����*/
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

#if DECS("TSU��Դ����")
/*****************************************************************************
 Prototype    : zrv_compress_task_monitor_thread_execute
 Description  : TSU��Դ�������������߳�
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
            /* ÿ��60��ɨ�������б�,���ӵȴ���Ӧ��������� */
            scan_compress_task_list_for_wait_expire(run->pDbOper, (int*)&(run->run_time));
        }

        run->run_time = time(NULL);
        osip_usleep(60000000); /* 60��ɨ��һ�� */
    }

    return NULL;
}
#endif

#if DECS("���ù�����")
/*****************************************************************************
 Prototype    : config_mgn_proc_thread_execute
 Description  : ���ù������������߳�
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

#if DECS("Telnet�ͻ��˼���")
/*****************************************************************************
 Prototype    : telnet_client_monitor_thread_execute
 Description  : Telnet �ͻ��˼���������߳�
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
            /* ÿ��һ��ɨ��Telnet �ͻ��� */
            ScanClientIfExpires();

            /* ɨ�������ļ�,ȫ�����ò����Ƿ��б仯 */
            refresh_config_file();
        }

        run->run_time = time(NULL);
        osip_usleep(1000000);
    }

    return NULL;
}
#endif

#if DECS("���������ݿ�ˢ�´���")
/*****************************************************************************
 Prototype    : common_db_refresh_thread_execute
 Description  : ���������ݿ�ˢ�´����������߳�
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

#if DECS("�ϼ�ƽ̨������")
/*****************************************************************************
 Prototype    : platform_mgn_proc_thread_execute
 Description  : �ϼ�ƽ̨������
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

                /* ���·����Ϣ�Ƿ���Ҫ���¼��� */
                check_PlatformInfoConfig_need_to_reload_begin(run->pDbOper);
            }

            /* ÿ��һ��ɨ��·����Ϣ */
            scan_platform_info_list();

            if (check_db_interval > 300)
            {
                /* ���·����Ϣ�Ƿ���Ҫ���¼��� */
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

