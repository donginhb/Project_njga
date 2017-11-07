
/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
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
 * ģ�鼶����                                   *
 *----------------------------------------------*/
device_srv_proc_tl_list_t* g_DeviceSrvProcThreadList = NULL;            /* ǰ���豸ҵ�����̶߳��� */
int ZRVDeviceClientServSock = 0;

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
#if DECS("ǰ���豸ҵ�����߳�")
/*****************************************************************************
 �� �� ��  : device_srv_proc_thread_for_appoint_execute
 ��������  : ǰ���豸ҵ�����߳�
 �������  : appoint_device_srv_proc_tl_t * device_srv_proc
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : device_srv_proc_thread_init
 ��������  : ǰ���豸ҵ�����̳߳�ʼ��
 �������  : device_srv_proc_tl_t** run
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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
 �� �� ��  : device_srv_proc_thread_free
 ��������  : ǰ���豸ҵ�����߳��ͷ�
 �������  : device_srv_proc_tl_t *run
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

#if DECS("ǰ���豸ҵ�����̶߳���")
/*****************************************************************************
 �� �� ��  : device_srv_proc_thread_assign
 ��������  : ǰ���豸ҵ�����̷߳���
 �������  : GBDevice_info_t* pDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��27��
    ��    ��   : ǰ���豸·����Ϣ����
    �޸�����   : �����ɺ���

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

    /* ��ʼ����Ϣ���� */
    if (NULL == runthread->pDeviceSrvMsgQueue)
    {
        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_assign() Device Srv Msg Queue: Begin--- \r\n");
        runthread->pDeviceSrvMsgQueue = new device_srv_msg_queue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_assign() Device Srv Msg Queue: End--- \r\n");
    }

    //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_assign() Device Srv Msg Queue Clear: Begin---\r\n");

    runthread->pDeviceSrvMsgQueue->clear();

    //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "device_srv_proc_thread_assign() Device Srv Msg Queue Clear: End--- \r\n");

    /* ��ʼ�����ݿ����� */
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

    /* ��ʼ����־���ݿ����� */
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
 �� �� ��  : device_srv_proc_thread_recycle
 ��������  : ǰ���豸ҵ�����̻߳���
 �������  : char* device_id
             char* device_ip
             int device_port
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��27��
    ��    ��   : ǰ���豸·����Ϣ����
    �޸�����   : �����ɺ���

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
 �� �� ��  : device_srv_proc_thread_start_all
 ��������  : ����ǰ���豸ҵ�����߳�
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

        //��ӵ�ǰ���豸ҵ�����̶߳���
        i = osip_list_add(g_DeviceSrvProcThreadList->pDeviceSrvProcList, runthread, -1); /* add to list tail */
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "device_srv_proc_thread_start() osip_list_add:i=%d \r\n", i);

        if (i < 0)
        {
            device_srv_proc_thread_free(runthread);
            runthread = NULL;
            //DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "device_srv_proc_thread_start() exit---: List Add Error \r\n");
            continue;
        }

        //���������߳�
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
 �� �� ��  : device_srv_proc_thread_stop_all
 ��������  : ֹͣ����ǰ���豸ҵ�����߳�
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

    //���Ҷ��У�ֹͣ�߳�
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
 �� �� ��  : device_srv_proc_thread_restart
 ��������  : ǰ���豸ҵ�����߳���������
 �������  : GBDevice_info_t* pDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��12�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    //���Ҷ���
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
            /* ֹͣ */
            runthread->th_exit = 1;

            if (runthread->thread != NULL)
            {
                iRet = osip_thread_join((struct osip_thread*)runthread->thread);
                osip_free(runthread->thread);
                runthread->thread = NULL;
            }

            /* ���� */
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
 �� �� ��  : device_srv_proc_thread_find
 ��������  : ����ǰ���豸ҵ�����߳�
 �������  : GBDevice_info_t* pDeviceInfo
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��10��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

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

    //���Ҷ���
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
 �� �� ��  : get_free_device_srv_proc_thread
 ��������  : ��ȡ���е�ǰ���豸ҵ�����߳�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��27��
    ��    ��   : ǰ���豸·����Ϣ����
    �޸�����   : �����ɺ���

*****************************************************************************/
device_srv_proc_tl_t* get_free_device_srv_proc_thread()
{
    int pos = 0;
    device_srv_proc_tl_t* runthread = NULL;

    //���Ҷ��У���sipudp�ӽ����̶߳������Ƴ�
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
 �� �� ��  : get_device_srv_proc_thread
 ��������  : ��ȡǰ���豸ҵ�����߳�
 �������  : char* device_id
             char* device_ip
             int device_port
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��27��
    ��    ��   : ǰ���豸·����Ϣ����
    �޸�����   : �����ɺ���

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
 �� �� ��  : get_device_srv_proc_thread2
 ��������  : ��ȡǰ���豸ҵ�����߳�
 �������  : char* device_id
             char* device_ip
             int device_port
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��1��13�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : device_srv_proc_thread_list_init
 ��������  : ��ʼ��ǰ���豸ҵ�����̶߳���
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
 �� �� ��  : device_srv_proc_thread_list_free
 ��������  : �ͷ�ǰ���豸ҵ�����̶߳���
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
 �� �� ��  : device_srv_proc_thread_list_lock
 ��������  : ǰ���豸ҵ�����̶߳��м���
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
 �� �� ��  : device_srv_proc_thread_list_unlock
 ��������  : ǰ���豸ҵ�����̶߳��н���
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
 �� �� ��  : debug_device_srv_proc_thread_list_lock
 ��������  : ǰ���豸ҵ�����̶߳��м���
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
 �� �� ��  : debug_device_srv_proc_thread_list_unlock
 ��������  : ǰ���豸ҵ�����̶߳��н���
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
 �� �� ��  : appoint_device_srv_msg_list_clean
 ��������  : ǰ���豸ҵ����Ϣ�������
 �������  : device_srv_proc_tl_t* run
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��3��8��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : scan_device_srv_proc_thread_list
 ��������  : ɨ��ǰ���豸ҵ�����̶߳���
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

    /* ������Ҫ��ʼ�� */
    while (!needToProc.empty())
    {
        pThreadProc = (device_srv_proc_tl_t*) needToProc.front();
        needToProc.pop_front();

        if (NULL != pThreadProc)
        {
            if (NULL != pThreadProc->pDeviceInfo)
            {
                //iRet = device_srv_proc_thread_restart(pThreadProc->pDeviceInfo);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ǰ���豸ҵ�����̼߳���̼߳�⵽ǰ���豸ҵ�����̹߳���, ǰ���豸ID=%s, ǰ���豸IP=%s, ǰ���豸�˿�=%d", pThreadProc->pDeviceInfo->device_id, pThreadProc->pDeviceInfo->login_ip, pThreadProc->pDeviceInfo->login_port);
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
 �� �� ��  : show_device_srv_proc_thread
 ��������  : ��ʾǰ���豸�̵߳�ʹ�����
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

        if (0 == type) /* ��ʾδʹ�õ� */
        {
            if (1 == runthread->iUsed || NULL != runthread->pDeviceInfo)
            {
                continue;
            }
        }
        else if (1 == type) /* ��ʾ�Ѿ�ʹ�õ� */
        {
            if (0 == runthread->iUsed || NULL == runthread->pDeviceInfo)
            {
                continue;
            }
        }
        else if (2 == type) /* ��ʾȫ�� */
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
 �� �� ��  : scan_appoint_device_srv_msg_list
 ��������  : ɨ��ָ���Ķ༶������Ϣ����
 �������  : device_srv_proc_tl_t* run
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��30��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : device_srv_msg_add_for_appoint
 ��������  : ���ǰ���豸ҵ����Ϣ��ָ�����̶߳�����
 �������  : device_srv_proc_tl_t* pDeviceSrvProcThd
             msg_type_t msg_type
             char* caller_id
             char* callee_id
             int response_code
             char* reasonphrase
             int ua_dialog_index
             char* msg_body
             int msg_body_len
             int cr_pos
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��30��
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

#if DECS("ZRV�豸��TCP���ӹ���")
/*****************************************************************************
 �� �� ��  : zrv_device_tcp_client_init
 ��������  : ZRV�豸TCP�ṹ��ʼ��
 �������  : user_tcp_client_t* sc
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : ZRVDeviceLoginServerInit
 ��������  : ZRV�豸TCP����˳�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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

    /*closesocket��һ�㲻�������رն�����TIME_WAIT�Ĺ��̣�����������ø�socket��*/
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    /* ���ý��ջ����� */
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
 �� �� ��  : UserLoginServerFree
 ��������  : ZRV�豸TCP������ͷ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : UserLoginServerMain
 ��������  : ZRV�豸TCP�����������
 �������  : DBOper* pDbOper
             int* run_thread_time
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void ZRVDeviceLoginServerMain(DBOper* pDbOper, int* run_thread_time)
{
    int i = 0;
    int p = 0;
    int  maxDescriptor = 0;              /* Maximum socket descriptor value */
    fd_set sockSet;                      /* Set of socket descriptors for select() */
    struct timeval val;                  /* Timeout for select() */
    char buff[1024 * 1024] = {0};        /* ������Ϣ���� */
    char tmp_buff[1024 * 1024] = {0};    /* ������Ϣ��ʱ���� */
    char tmp1_buff[1024 * 1024] = {0};   /* ������Ϣ��ʱ���� */
    char strBodyBuff[1024 * 1024] = {0}; /* ������Ϣ��*/
    char* tmp_prex_buff = NULL;
    char from_ip[16] = {0};
    int  clientSock = 0;                 /* Socket descriptor for client */
    int  recvSize = 0;
    int  remainSize = 0;                 /* ʣ�µĳ��� */
    int  deal_count = 0;                 /* ������� */
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
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ZRV�豸TCP�������ﵽ����, �ر�ZRV�豸��TCP����:TCP Socket=%d", clientSock);
            close(clientSock);

            /* �ͷŵ�û�õ�TCP���� */
            free_unused_zrv_device_tcp_connect();
            return;
        }
        else
        {
            if (NULL == inet_ntop(AF_INET, (void*) & (ClntAddr.sin_addr), from_ip, sizeof(from_ip)))
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "��ȡZRV�豸TCP���ӵ�IP��ַʧ��, �ر�ZRV�豸��TCP����:TCP Socket=%d", clientSock);
                close(clientSock);
                return;
            }

            /* ���÷��ͳ�ʱʱ�� */
            iTimeout.tv_sec = 1; /* 1�볬ʱ */
            iTimeout.tv_usec = 0;
            setsockopt(clientSock, SOL_SOCKET, SO_SNDTIMEO, (char *)&iTimeout, sizeof(struct timeval));

            /* ���÷��ͻ�����������Ϊ0���������ͣ�����Ҫ���壬��ֹ�Զ˽���ճ�� */
            //setsockopt(clientSock, SOL_SOCKET, SO_SNDBUF, (char *)&iSendBuf, sizeof(int));

            /* ѡ��TCP_NODELAY�ǽ���Nagle�㷨�������ݰ��������ͳ�ȥ����ѡ��TCP_CORK����෴��������Ϊ����Nagle�㷨�Ľ�һ����ǿ�����������ݰ����� */
            /* ��ֹnagle�㷨������Ҫ���͵ľ��������� */
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
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ�ZRV���͵�TCP��Ϣ����Ϊ0, �����ر�TCP����:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, errno=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, errno);
                DEBUG_TRACE(MODULE_USER, LOG_INFO, "ZRVDeviceLoginServerMain() ZRV Device: IP=%s, Port=%d, Socket=%d, recvSize=%d Closed1 \r\n", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, clientSock, recvSize);
                close(clientSock);

                if (ZRVDeviceTCPClients[p].iRcvBuffLen > 0 && ZRVDeviceTCPClients[p].strRcvBuff[0] != '\0')
                {
                    /* �������� */
                    iRet = ZRVDeviceParseTCPSocketDataProc(ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].strRcvBuff, ZRVDeviceTCPClients[p].iRcvBuffLen, pDbOper, run_thread_time);

                    if (0 != iRet)
                    {
                        if (-2 == iRet)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ZRV���͵�TCP��Ϣʧ��, ��֧�ֵ���Ϣ��ʽ:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ZRV���͵�TCP��Ϣʧ��:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        }
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "����ZRV���͵�TCP��Ϣ�ɹ�:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                    }
                }

                /* �ر� */
                zrv_device_tcp_client_init(&ZRVDeviceTCPClients[p]);
                free_zrv_device_info_by_tcp_socket(clientSock);
                continue;
            }
            else if (recvSize < 0)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ�ZRV���͵�TCP��Ϣ����, �����ر�TCP����:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, errno=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, errno);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "ZRVDeviceLoginServerMain() ZRV Device: IP=%s, Port=%d, Socket=%d, recvSize=%d Closed2 \r\n", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, clientSock, recvSize);
                    close(clientSock);

                    if (ZRVDeviceTCPClients[p].iRcvBuffLen > 0 && ZRVDeviceTCPClients[p].strRcvBuff[0] != '\0')
                    {
                        /* �������� */
                        iRet = ZRVDeviceParseTCPSocketDataProc(ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].strRcvBuff, ZRVDeviceTCPClients[p].iRcvBuffLen, pDbOper, run_thread_time);

                        if (0 != iRet)
                        {
                            if (-2 == iRet)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ZRV���͵�TCP��Ϣʧ��, ��֧�ֵ���Ϣ��ʽ:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ZRV���͵�TCP��Ϣʧ��:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                            }
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "����ZRV���͵�TCP��Ϣ�ɹ�:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        }
                    }

                    /* �ر� */
                    zrv_device_tcp_client_init(&ZRVDeviceTCPClients[p]);
                    free_zrv_device_info_by_tcp_socket(clientSock);
                    continue;
                }
                else if (errno == EAGAIN) /* ���ճ�ʱ */
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "���յ�ZRV���͵�TCP��Ϣ��ʱ, ��������:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                    continue;
                }
            }

            if (0 == strncmp(buff, "ZBITCLOUDMSG", MAX_TCPHEAD_MARK_LEN)) /* һ����Ϣ�Ŀ�ʼ */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���յ�ZRV���͵�TCP��Ϣ, ����Ϣͷ��ʼ, ��ʼ�µĽ���:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);

                memset(tmp_buff, 0, 1024 * 1024);
                memcpy(tmp_buff, buff, 1024 * 1024);
                remainSize = recvSize;
                deal_count = 0;

                while (remainSize > 0)
                {
                    /* ȡ����Ϣ�� */
                    memset(&stMsgHead, 0, MAX_TCPHEAD_LEN);
                    memcpy(&stMsgHead, tmp_buff, MAX_TCPHEAD_LEN);

                    if (stMsgHead.iMsgBodyLen <= 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ�ZRV���͵�TCP��Ϣͷ����Я���ĳ���Ϊ0, �������˵�:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ���Ϣ����=%s\r\n", tmp_buff);
                        break;
                    }

                    if (remainSize == stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN) /* �������� */
                    {
                        /* ׷�ӵ��ַ��� */
                        strncat(ZRVDeviceTCPClients[p].strRcvBuff, &tmp_buff[MAX_TCPHEAD_LEN], stMsgHead.iMsgBodyLen);
                        ZRVDeviceTCPClients[p].iRcvBuffLenCount += stMsgHead.iMsgBodyLen;
                        ZRVDeviceTCPClients[p].iRcvBuffLen = stMsgHead.iMsgBodyLen;

                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ�ZRV���͵�TCP��Ϣ������һ����������:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        break;
                    }
                    else if (remainSize < stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN) /* û������ */
                    {
                        /* ׷�ӵ��ַ��� */
                        strncat(ZRVDeviceTCPClients[p].strRcvBuff, &tmp_buff[MAX_TCPHEAD_LEN], remainSize - MAX_TCPHEAD_LEN);
                        ZRVDeviceTCPClients[p].iRcvBuffLenCount += remainSize - MAX_TCPHEAD_LEN;
                        ZRVDeviceTCPClients[p].iRcvBuffLen = stMsgHead.iMsgBodyLen;

                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ�ZRV���͵�TCP��Ϣû����������ȫ, �ȴ���һ�����Ľ�����ȫ:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        break;
                    }
                    else if (remainSize > stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN) /* �յ���ֹһ������ */
                    {
                        /* ׷�ӵ��ַ��� */
                        strncat(ZRVDeviceTCPClients[p].strRcvBuff, &tmp_buff[MAX_TCPHEAD_LEN], stMsgHead.iMsgBodyLen);
                        ZRVDeviceTCPClients[p].iRcvBuffLenCount += stMsgHead.iMsgBodyLen;
                        ZRVDeviceTCPClients[p].iRcvBuffLen = stMsgHead.iMsgBodyLen;

                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ�ZRV���͵�TCP��Ϣ����һ����������, ����ճ��, �ָ�����:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);

                        /* �Ƚ���������ɵ�һ������ */
                        deal_count++;
                        iRet = ZRVDeviceParseTCPSocketDataProc(ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].strRcvBuff, ZRVDeviceTCPClients[p].iRcvBuffLen, pDbOper, run_thread_time);

                        if (0 != iRet)
                        {
                            if (-2 == iRet)
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ZRV���͵�TCP��Ϣ�еĵ�%d������ʧ��, ��֧�ֵ���Ϣ��ʽ:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", deal_count, ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ZRV���͵�TCP��Ϣ�еĵ�%d������ʧ��:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", deal_count, ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                            }
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "����ZRV���͵�TCP��Ϣ�еĵ�%d�����ĳɹ�:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", deal_count, ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        }

                        memset(ZRVDeviceTCPClients[p].strRcvBuff, 0, 1024 * 1024);
                        ZRVDeviceTCPClients[p].iRcvBuffLen = 0;
                        ZRVDeviceTCPClients[p].iRcvBuffLenCount = 0;

                        /* buffer���¸�ֵ */
                        remainSize = remainSize - (stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN);
                        memset(tmp1_buff, 0, 1024 * 1024);
                        memcpy(tmp1_buff, &tmp_buff[stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN], remainSize);
                        memset(tmp_buff, 0, 1024 * 1024);
                        memcpy(tmp_buff, tmp1_buff, 1024 * 1024);
                    }
                }
            }
            else /* û�н���ȫ����Ϣ�������� */
            {
                if (ZRVDeviceTCPClients[p].iRcvBuffLen == 0) /* û���յ�ͷ�����˵� */
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ�ZRV���͵�TCP��Ϣ, ����֮ǰû���յ���Ϣͷ, �������˵�:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ���Ϣ����=%s\r\n", buff);
                    continue;
                }

                tmp_prex_buff = strstr(buff, "ZBITCLOUDMSG");

                if (NULL == tmp_prex_buff) /* û���ҵ���˵���ð�����û���¸����� */
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ�ZRV���͵�TCP��Ϣ, ����û����Ϣͷ, ֱ��׷�ӵ���һ����Ϣ�н��д���:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);

                    /* ׷�ӵ��ַ��� */
                    strncat(ZRVDeviceTCPClients[p].strRcvBuff, buff, recvSize);
                    ZRVDeviceTCPClients[p].iRcvBuffLenCount += recvSize;
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ�ZRV���͵�TCP��Ϣ, ���д�����Ϣͷ, �Ȱ���Ϣͷǰ����Ϣ׷�ӵ���һ����Ϣ����:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);

                    /* �Ȱ���һ����Ϣ������ */
                    /* ׷�ӵ��ַ��� */
                    strncat(ZRVDeviceTCPClients[p].strRcvBuff, buff, tmp_prex_buff - buff);
                    ZRVDeviceTCPClients[p].iRcvBuffLenCount += tmp_prex_buff - buff;

                    /* �������� */
                    iRet = ZRVDeviceParseTCPSocketDataProc(ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].strRcvBuff, ZRVDeviceTCPClients[p].iRcvBuffLen, pDbOper, run_thread_time);

                    if (0 != iRet)
                    {
                        if (-2 == iRet)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ZRV���͵�TCP��Ϣʧ��, ��֧�ֵ���Ϣ��ʽ:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ZRV���͵�TCP��Ϣʧ��:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                        }
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ZRV���͵�TCP��Ϣ�ɹ�:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                    }

                    memset(ZRVDeviceTCPClients[p].strRcvBuff, 0, 1024 * 1024);
                    ZRVDeviceTCPClients[p].iRcvBuffLen = 0;
                    ZRVDeviceTCPClients[p].iRcvBuffLenCount = 0;

                    memset(tmp_buff, 0, 1024 * 1024);
                    memcpy(tmp_buff, tmp_prex_buff, 1024 * 1024);
                    remainSize = recvSize - (tmp_prex_buff - buff);
                    deal_count = 0;

                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ�ZRV���͵�TCP��Ϣ, ����������Ϣͷ�󲿷ֵ���Ϣ����:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, ʣ�����Ϣ����=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, remainSize);

                    while (remainSize > 0)
                    {
                        /* ȡ����Ϣ�� */
                        memset(&stMsgHead, 0, MAX_TCPHEAD_LEN);
                        memcpy(&stMsgHead, tmp_buff, MAX_TCPHEAD_LEN);

                        if (stMsgHead.iMsgBodyLen <= 0)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ�ZRV���͵�TCP��Ϣͷ����Я���ĳ���Ϊ0, �������˵�:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ���Ϣ����=%s\r\n", tmp_buff);
                            break;
                        }

                        if (remainSize == stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN) /* �������� */
                        {
                            /* ׷�ӵ��ַ��� */
                            strncat(ZRVDeviceTCPClients[p].strRcvBuff, &tmp_buff[MAX_TCPHEAD_LEN], stMsgHead.iMsgBodyLen);
                            ZRVDeviceTCPClients[p].iRcvBuffLenCount += stMsgHead.iMsgBodyLen;
                            ZRVDeviceTCPClients[p].iRcvBuffLen = stMsgHead.iMsgBodyLen;

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ�ZRV���͵�TCP��Ϣ������һ����������:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                            break;
                        }
                        else if (remainSize < stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN) /* û������ */
                        {
                            /* ׷�ӵ��ַ��� */
                            strncat(ZRVDeviceTCPClients[p].strRcvBuff, &tmp_buff[MAX_TCPHEAD_LEN], remainSize - MAX_TCPHEAD_LEN);
                            ZRVDeviceTCPClients[p].iRcvBuffLenCount += remainSize - MAX_TCPHEAD_LEN;
                            ZRVDeviceTCPClients[p].iRcvBuffLen = stMsgHead.iMsgBodyLen;

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ�ZRV���͵�TCP��Ϣû����������ȫ, �ȴ���һ�����Ľ�����ȫ:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                            break;
                        }
                        else if (remainSize > stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN) /* �յ���ֹһ������ */
                        {
                            /* ׷�ӵ��ַ��� */
                            strncat(ZRVDeviceTCPClients[p].strRcvBuff, &tmp_buff[MAX_TCPHEAD_LEN], stMsgHead.iMsgBodyLen);
                            ZRVDeviceTCPClients[p].iRcvBuffLenCount += stMsgHead.iMsgBodyLen;
                            ZRVDeviceTCPClients[p].iRcvBuffLen = stMsgHead.iMsgBodyLen;

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ�ZRV���͵�TCP��Ϣ����һ����������, ����ճ��, �ָ�����:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock);
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);

                            /* �Ƚ���������ɵ�һ������ */
                            deal_count++;
                            iRet = ZRVDeviceParseTCPSocketDataProc(ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].strRcvBuff, ZRVDeviceTCPClients[p].iRcvBuffLen, pDbOper, run_thread_time);

                            if (0 != iRet)
                            {
                                if (-2 == iRet)
                                {
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ZRV���͵�TCP��Ϣ�еĵ�%d������ʧ��, ��֧�ֵ���Ϣ��ʽ:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", deal_count, ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                                }
                                else
                                {
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ZRV���͵�TCP��Ϣ�еĵ�%d������ʧ��:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", deal_count, ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                                }
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "����ZRV���͵�TCP��Ϣ�еĵ�%d�����ĳɹ�:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", deal_count, ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                            }

                            memset(ZRVDeviceTCPClients[p].strRcvBuff, 0, 1024 * 1024);
                            ZRVDeviceTCPClients[p].iRcvBuffLen = 0;
                            ZRVDeviceTCPClients[p].iRcvBuffLenCount = 0;

                            /* buffer���¸�ֵ */
                            remainSize = remainSize - (stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN);
                            memset(tmp1_buff, 0, 1024 * 1024);
                            memcpy(tmp1_buff, &tmp_buff[stMsgHead.iMsgBodyLen + MAX_TCPHEAD_LEN], remainSize);
                            memset(tmp_buff, 0, 1024 * 1024);
                            memcpy(tmp_buff, tmp1_buff, 1024 * 1024);
                        }
                    }
                }
            }

            if (ZRVDeviceTCPClients[p].iRcvBuffLenCount < ZRVDeviceTCPClients[p].iRcvBuffLen) /* ��Ҫ�������� */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "���յ�ZRV���͵�TCP��Ϣ, û����ȫ�������, ��������:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, ��Ϣ�峤��=%d, �Ѿ����յ���Ϣ����=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, ZRVDeviceTCPClients[p].iRcvBuffLenCount);
                DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "ZRVDeviceLoginServerMain() ZRV Device: IP=%s, Port=%d, Socket=%d, RcvBuffLen=%d, RcvBuffLenCount=%d, Not Recv Complete \r\n", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, clientSock, ZRVDeviceTCPClients[p].iRcvBuffLen, ZRVDeviceTCPClients[p].iRcvBuffLenCount);
                continue;
            }

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "############################################\r\n");
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "ZRVDeviceLoginServerMain() ZRV Device: IP=%s, Port=%d, Socket=%d, RcvBuffLen=%d, RcvBuffLenCount=%d, Recv Complete, Begin Parse, BodyBuff:\r\n%s\r\n\r\n", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, clientSock, ZRVDeviceTCPClients[p].iRcvBuffLen, ZRVDeviceTCPClients[p].iRcvBuffLenCount, ZRVDeviceTCPClients[p].strRcvBuff);
            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "############################################\r\n");

            /* �������� */
            iRet = ZRVDeviceParseTCPSocketDataProc(ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].strRcvBuff, ZRVDeviceTCPClients[p].iRcvBuffLen, pDbOper, run_thread_time);

            if (0 != iRet)
            {
                if (-2 == iRet)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ZRV���͵�TCP��Ϣʧ��, ��֧�ֵ���Ϣ��ʽ:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ZRV���͵�TCP��Ϣʧ��:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ZRV���͵�TCP��Ϣ�ɹ�:ZRV�豸IP��ַ=%s, �˿ں�=%d, TCP Sock=%d, �յ�����Ϣ����=%d, iRet=%d", ZRVDeviceTCPClients[p].login_ip, ZRVDeviceTCPClients[p].login_port, ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].iRcvBuffLen, iRet);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���յ���Ϣ����=%s\r\n", ZRVDeviceTCPClients[p].strRcvBuff);
            }

            memset(ZRVDeviceTCPClients[p].strRcvBuff, 0, 1024 * 1024);
            ZRVDeviceTCPClients[p].iRcvBuffLen = 0;
            ZRVDeviceTCPClients[p].iRcvBuffLenCount = 0;
        }
    }

    return;
}

/*****************************************************************************
 �� �� ��  : ShowConnectTCPUser
 ��������  : ��ʾZRV�豸���ӵ�TCP
 �������  : int sock
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
 �� �� ��  : free_unused_zrv_device_tcp_connect
 ��������  : �ͷŵ�û��ʹ�õ�ZRV�豸TCP����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��4��26�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

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
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ZRV�豸TCP����û��ʹ��, �����ر�:TCP Socket=%d", ZRVDeviceTCPClients[p].sock);
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
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "������Ϣʧ��, ZRV�豸TCP�����쳣, �����ر�:TCP Socket=%d, device ip=%s, errno=%d", ZRVDeviceTCPClients[p].sock, ZRVDeviceTCPClients[p].login_ip, errno);
            close(ZRVDeviceTCPClients[p].sock);
            zrv_device_tcp_client_init(&ZRVDeviceTCPClients[p]);
        }
    }

    return;
}
#endif

