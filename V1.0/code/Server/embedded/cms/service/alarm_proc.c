
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

#include "common/gblfunc_proc.inc"
#include "common/gblconfig_proc.inc"
#include "common/log_proc.inc"

#include "device/device_info_mgn.inc"

#include "service/alarm_proc.inc"
#include "service/plan_srv_proc.inc"
#include "user/user_info_mgn.inc"

/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern int g_AlarmMsgSendToUserFlag;         /* ������Ϣ�Ƿ��͸��û�,Ĭ�Ϸ��� */
extern char g_StrConLog[2][100];

/*----------------------------------------------*
 * �ⲿ����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ȫ�ֱ���                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/
unsigned int uFaultMsgSn = 0;
fault_msg_queue g_FaultMsgQueue;                          /* ������Ϣ���� */
#ifdef MULTI_THR
osip_mutex_t* g_FaultMsgQueueLock = NULL;
#endif

alarm_msg_queue g_AlarmMsgQueue;                          /* ������Ϣ���� */
#ifdef MULTI_THR
osip_mutex_t* g_AlarmMsgQueueLock = NULL;
#endif
alarm_timer_list_t* g_AlarmTimerList = NULL;              /* ��ʱ������ʱ������ */
alarm_duration_list_t* g_AlarmDurationList = NULL;        /* ��ʱ������ʱ������ */
alarm_deployment_list_t* g_AlarmDeploymentList = NULL;    /* ����ʱ����Զ��� */
int db_AlarmDeployment_reload_mark = 0;                   /* ����ʱ��������ݿ���±�ʶ:0:����Ҫ���£�1:��Ҫ�������ݿ� */

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/
#if DECS("������Ϣ����")
/*****************************************************************************
 �� �� ��  : alarm_msg_init
 ��������  : ������Ϣ�ṹ��ʼ��
 �������  : alarm_msg_t ** Alarm_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_msg_init(alarm_msg_t** alarm_msg)
{
    *alarm_msg = (alarm_msg_t*)osip_malloc(sizeof(alarm_msg_t));

    if (*alarm_msg == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_msg_init() exit---: *Alarm_msg Smalloc Error \r\n");
        return -1;
    }

    (*alarm_msg)->strSN[0] = '\0';
    (*alarm_msg)->uDeviceIndex = 0;
    (*alarm_msg)->strDeviceID[0] = '\0';
    (*alarm_msg)->strPriority[0] = '\0';
    (*alarm_msg)->strMethod[0] = '\0';
    (*alarm_msg)->strAlarmStartTime[0] = '\0';
    (*alarm_msg)->strAlarmEndTime[0] = '\0';
    (*alarm_msg)->iAlarmMsgType = 0;
    (*alarm_msg)->iAlarmLengthOfTime = 0;
    (*alarm_msg)->strDeseription[0] = '\0';
    (*alarm_msg)->iAlarmDeviceType = 0;

    return 0;
}

/*****************************************************************************
 �� �� ��  : alarm_msg_free
 ��������  : ������Ϣ�ṹ�ͷ�
 �������  : alarm_msg_t * Alarm_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void alarm_msg_free(alarm_msg_t* alarm_msg)
{
    if (alarm_msg == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_msg_free() exit---: Param Error \r\n");
        return;
    }

    memset(alarm_msg->strSN, 0, 32);
    alarm_msg->uDeviceIndex = 0;
    memset(alarm_msg->strDeviceID, 0, MAX_ID_LEN + 4);
    memset(alarm_msg->strPriority, 0, 32);
    memset(alarm_msg->strMethod, 0, 32);
    memset(alarm_msg->strAlarmStartTime, 0, 32);
    memset(alarm_msg->strAlarmEndTime, 0, 32);
    alarm_msg->iAlarmMsgType = 0;
    alarm_msg->iAlarmLengthOfTime = 0;
    memset(alarm_msg->strDeseription, 0, 512 + 4);
    alarm_msg->iAlarmDeviceType = 0;

    return;
}

/*****************************************************************************
 �� �� ��  : alarm_msg_list_init
 ��������  : ������Ϣ���г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_msg_list_init()
{
    g_AlarmMsgQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_AlarmMsgQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_AlarmMsgQueueLock)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_msg_list_init() exit---: Alarm Message List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : alarm_msg_list_free
 ��������  : ������Ϣ�����ͷ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void alarm_msg_list_free()
{
    alarm_msg_t* pAlarmMsg = NULL;

    while (!g_AlarmMsgQueue.empty())
    {
        pAlarmMsg = (alarm_msg_t*) g_AlarmMsgQueue.front();
        g_AlarmMsgQueue.pop_front();

        if (NULL != pAlarmMsg)
        {
            alarm_msg_free(pAlarmMsg);
            osip_free(pAlarmMsg);
            pAlarmMsg = NULL;
        }
    }

    g_AlarmMsgQueue.clear();

#ifdef MULTI_THR

    if (NULL != g_AlarmMsgQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_AlarmMsgQueueLock);
        g_AlarmMsgQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 �� �� ��  : alarm_msg_list_clean
 ��������  : ������Ϣ�������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��3��8��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void alarm_msg_list_clean()
{
    alarm_msg_t* pAlarmMsg = NULL;

    while (!g_AlarmMsgQueue.empty())
    {
        pAlarmMsg = (alarm_msg_t*) g_AlarmMsgQueue.front();
        g_AlarmMsgQueue.pop_front();

        if (NULL != pAlarmMsg)
        {
            alarm_msg_free(pAlarmMsg);
            osip_free(pAlarmMsg);
            pAlarmMsg = NULL;
        }
    }

    g_AlarmMsgQueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : alarm_msg_add
 ��������  : ��ӱ�����Ϣ��������
 �������  : char* strSN
             unsigned int uDeviceIndex
             char* strDeviceID
             char* strPriority
             char* strMethod
             char* strAlarmStartTime
             char* strAlarmEndTime
             int iAlarmMsgType
             int iAlarmLengthOfTime
             char* strDeseription
             int iAlarmDeviceType
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_msg_add(char* strSN, unsigned int uDeviceIndex, char* strDeviceID, char* strPriority, char* strMethod, char* strAlarmStartTime, char* strAlarmEndTime, int iAlarmMsgType, int iAlarmLengthOfTime, char* strDeseription, int iAlarmDeviceType)
{
    alarm_msg_t* pAlarmMsg = NULL;
    int iRet = 0;

    if (uDeviceIndex <= 0 || NULL == strDeviceID)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = alarm_msg_init(&pAlarmMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    if ('\0' != strSN[0])
    {
        osip_strncpy(pAlarmMsg->strSN, strSN, 32);
    }

    pAlarmMsg->uDeviceIndex = uDeviceIndex;

    if ('\0' != strDeviceID[0])
    {
        osip_strncpy(pAlarmMsg->strDeviceID, strDeviceID, MAX_ID_LEN);
    }

    if ('\0' != strPriority[0])
    {
        osip_strncpy(pAlarmMsg->strPriority, strPriority, 32);
    }

    if ('\0' != strMethod[0])
    {
        osip_strncpy(pAlarmMsg->strMethod, strMethod, 32);
    }

    if ('\0' != strAlarmStartTime[0])
    {
        osip_strncpy(pAlarmMsg->strAlarmStartTime, strAlarmStartTime, 32);
    }

    if ('\0' != strAlarmEndTime[0])
    {
        osip_strncpy(pAlarmMsg->strAlarmEndTime, strAlarmEndTime, 32);
    }

    pAlarmMsg->iAlarmMsgType = iAlarmMsgType;
    pAlarmMsg->iAlarmLengthOfTime = iAlarmLengthOfTime;

    if (NULL != strDeseription && '\0' != strDeseription[0])
    {
        osip_strncpy(pAlarmMsg->strDeseription, strDeseription, 512);
    }

    pAlarmMsg->iAlarmDeviceType = iAlarmDeviceType;

#ifdef MULTI_THR

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_AlarmMsgQueueLock);

#endif

    g_AlarmMsgQueue.push_back(pAlarmMsg);

#ifdef MULTI_THR

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AlarmMsgQueueLock);

#endif

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_alarm_msg_list
 ��������  : ɨ���豸ע����Ϣ����
 �������  : DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��23�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_alarm_msg_list(thread_proc_t* run)
{
    int iRet = 0;
    alarm_msg_t* pAlarmMsg = NULL;
    static int connect_interval = 0;

    if (NULL == run)
    {
        return;
    }

    if (NULL == run->pDbOper)
    {
        return;
    }

    if (!run->iLogDBOperConnectStatus)
    {
        connect_interval++;

        if (connect_interval >= 60 * 200)
        {
            if (run->pLogDbOper->Connect(g_StrConLog, (char*)"") < 0)
            {
                run->iLogDBOperConnectStatus = 0;
            }
            else
            {
                run->iLogDBOperConnectStatus = 1;
            }

            connect_interval = 0;
        }
    }

#ifdef MULTI_THR

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_AlarmMsgQueueLock);

#endif

    while (!g_AlarmMsgQueue.empty())
    {
        pAlarmMsg = (alarm_msg_t*) g_AlarmMsgQueue.front();
        g_AlarmMsgQueue.pop_front();

        if (NULL != pAlarmMsg)
        {
            break;
        }
    }

#ifdef MULTI_THR

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AlarmMsgQueueLock);

#endif

    if (NULL != pAlarmMsg)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "scan_alarm_msg_list() \
    \r\n In Param: \
    \r\n SN=%s \
    \r\n uDeviceIndex=%u \
    \r\n DeviceID=%s \
    \r\n Priority=%s \
    \r\n Method=%s \
    \r\n AlarmStartTime=%s \
    \r\n AlarmEndTime=%s \
    \r\n AlarmMsgType=%d \
    \r\n strDeseription=%s \
    \r\n ", pAlarmMsg->strSN, pAlarmMsg->uDeviceIndex, pAlarmMsg->strDeviceID, pAlarmMsg->strPriority, pAlarmMsg->strMethod, pAlarmMsg->strAlarmStartTime, pAlarmMsg->strAlarmEndTime, pAlarmMsg->iAlarmMsgType, pAlarmMsg->strDeseription);

        if (!run->iLogDBOperConnectStatus)
        {
            iRet = alarm_msg_proc(pAlarmMsg, run->pDbOper, NULL);
        }
        else
        {
            iRet = alarm_msg_proc(pAlarmMsg, run->pDbOper, run->pLogDbOper);
        }

        alarm_msg_free(pAlarmMsg);
        osip_free(pAlarmMsg);
        pAlarmMsg = NULL;
    }

    return;
}
#endif

#if DECS("������Ϣ����")
/*****************************************************************************
 �� �� ��  : fault_msg_init
 ��������  : ������Ϣ�ṹ��ʼ��
 �������  : alarm_msg_t ** fault_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int fault_msg_init(fault_msg_t** fault_msg)
{
    *fault_msg = (fault_msg_t*)osip_malloc(sizeof(fault_msg_t));

    if (*fault_msg == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "fault_msg_init() exit---: *fault_msg Smalloc Error \r\n");
        return -1;
    }

    (*fault_msg)->uLogicDeviceIndex = 0;
    (*fault_msg)->strLogicDeviceID[0] = '\0';
    (*fault_msg)->uFaultType = 0;
    (*fault_msg)->strPriority[0] = '\0';
    (*fault_msg)->strMethod[0] = '\0';
    (*fault_msg)->uLogTime = 0;
    (*fault_msg)->strDeseription[0] = '\0';

    return 0;
}

/*****************************************************************************
 �� �� ��  : fault_msg_free
 ��������  : ������Ϣ�ṹ�ͷ�
 �������  : fault_msg_t * fault_msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void fault_msg_free(fault_msg_t* fault_msg)
{
    if (fault_msg == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "fault_msg_free() exit---: Param Error \r\n");
        return;
    }

    fault_msg->uLogicDeviceIndex = 0;
    memset(fault_msg->strLogicDeviceID, 0, MAX_ID_LEN + 4);
    fault_msg->uFaultType = 0;
    memset(fault_msg->strPriority, 0, 32);
    memset(fault_msg->strMethod, 0, 32);
    fault_msg->uLogTime = 0;
    memset(fault_msg->strDeseription, 0, 512 + 4);
    return;
}

/*****************************************************************************
 �� �� ��  : fault_msg_list_init
 ��������  : ������Ϣ���г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int fault_msg_list_init()
{
    g_FaultMsgQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_FaultMsgQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_FaultMsgQueueLock)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "fault_msg_list_init() exit---: Fault Message List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : fault_msg_list_free
 ��������  : ������Ϣ�����ͷ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void fault_msg_list_free()
{
    fault_msg_t* pFaultMsg = NULL;

    while (!g_FaultMsgQueue.empty())
    {
        pFaultMsg = (fault_msg_t*) g_FaultMsgQueue.front();
        g_FaultMsgQueue.pop_front();

        if (NULL != pFaultMsg)
        {
            fault_msg_free(pFaultMsg);
            osip_free(pFaultMsg);
            pFaultMsg = NULL;
        }
    }

    g_FaultMsgQueue.clear();

#ifdef MULTI_THR

    if (NULL != g_FaultMsgQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_FaultMsgQueueLock);
        g_FaultMsgQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 �� �� ��  : fault_msg_list_clean
 ��������  : ������Ϣ�������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��3��8��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void fault_msg_list_clean()
{
    fault_msg_t* pFaultMsg = NULL;

    while (!g_FaultMsgQueue.empty())
    {
        pFaultMsg = (fault_msg_t*) g_FaultMsgQueue.front();
        g_FaultMsgQueue.pop_front();

        if (NULL != pFaultMsg)
        {
            fault_msg_free(pFaultMsg);
            osip_free(pFaultMsg);
            pFaultMsg = NULL;
        }
    }

    g_FaultMsgQueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : fault_msg_add
 ��������  : ��ӹ�����Ϣ��������
 �������  : unsigned int uLogicDeviceIndex
             char* strLogicDeviceID
             unsigned int uFaultType
             char* strPriority
             char* strMethod
             char* strDeseription
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int fault_msg_add(unsigned int uLogicDeviceIndex, char* strLogicDeviceID, unsigned int uFaultType, char* strPriority, char* strMethod, char* strDeseription)
{
    fault_msg_t* pFaultMsg = NULL;
    int iRet = 0;
    time_t utc_time;
    utc_time = time(NULL);

    if (uLogicDeviceIndex < 0 || NULL == strLogicDeviceID)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "fault_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = fault_msg_init(&pFaultMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "fault_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    pFaultMsg->uLogicDeviceIndex = uLogicDeviceIndex;

    if ('\0' != strLogicDeviceID[0])
    {
        osip_strncpy(pFaultMsg->strLogicDeviceID, strLogicDeviceID, MAX_ID_LEN);
    }

    pFaultMsg->uFaultType = uFaultType;

    if ('\0' != strPriority[0])
    {
        osip_strncpy(pFaultMsg->strPriority, strPriority, 32);
    }

    if ('\0' != strMethod[0])
    {
        osip_strncpy(pFaultMsg->strMethod, strMethod, 32);
    }

    pFaultMsg->uLogTime = utc_time;

    if (NULL != strDeseription && '\0' != strDeseription[0])
    {
        osip_strncpy(pFaultMsg->strDeseription, strDeseription, 512);
    }

#ifdef MULTI_THR

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_FaultMsgQueueLock);

#endif

    g_FaultMsgQueue.push_back(pFaultMsg);

#ifdef MULTI_THR

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_FaultMsgQueueLock);

#endif

    return 0;
}

/*****************************************************************************
 �� �� ��  : SystemFaultAlarm
 ��������  : ��¼ϵͳ���ϸ澯��Ϣ
 �������  : unsigned int uDeviceIndex
             char* strDeviceID
             unsigned int uFaultType
             char* strPriority
             char* strMethod
             const char* fmt
             ...
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��16�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int SystemFaultAlarm(unsigned int uLogicDeviceIndex, char* strLogicDeviceID, unsigned int uFaultType, char* strPriority, char* strMethod, const char* fmt, ...)
{
    int iRet = 0;
    int len = 0;
    va_list args;
    char s[MAX_2048CHAR_STRING_LEN + 4] = {0};

    if (0 != g_Language)
    {
        return 0;
    }

    va_start(args, fmt);
    len = vsnprintf(s, MAX_2048CHAR_STRING_LEN, fmt, args);
    va_end(args);

    len = strlen(s);

    if (len >= 512)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SystemFaultAlarm() exit---: Message Length Error:Msg=%s, len=%d \r\n", s, len);
        return -1;
    }

    iRet = fault_msg_add(uLogicDeviceIndex, strLogicDeviceID, uFaultType, strPriority, strMethod, s);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SystemFaultAlarm() fault_msg_add Error:Msg=%s, iRet=%d \r\n", s, iRet);
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : SystemFaultAlarm
 ��������  : ��¼ϵͳ���ϸ澯��Ϣ
 �������  : unsigned int uDeviceIndex
             char* strDeviceID
             unsigned int uFaultType
             char* strPriority
             char* strMethod
             const char* fmt
             ...
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��16�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int EnSystemFaultAlarm(unsigned int uLogicDeviceIndex, char* strLogicDeviceID, unsigned int uFaultType, char* strPriority, char* strMethod, const char* fmt, ...)
{
    int iRet = 0;
    int len = 0;
    va_list args;
    char s[MAX_2048CHAR_STRING_LEN + 4] = {0};

    if (1 != g_Language)
    {
        return 0;
    }

    va_start(args, fmt);
    len = vsnprintf(s, MAX_2048CHAR_STRING_LEN, fmt, args);
    va_end(args);

    len = strlen(s);

    if (len >= 512)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "EnSystemFaultAlarm() exit---: Message Length Error:Msg=%s, len=%d \r\n", s, len);
        return -1;
    }

    iRet = fault_msg_add(uLogicDeviceIndex, strLogicDeviceID, uFaultType, strPriority, strMethod, s);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "EnSystemFaultAlarm() fault_msg_add Error:Msg=%s, iRet=%d \r\n", s, iRet);
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : scan_fault_msg_list
 ��������  : ɨ�����ע����Ϣ����
 �������  : DBOper* pDboper
             DBOper* pLogDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��23�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_fault_msg_list(thread_proc_t* run)
{
    int iRet = 0;
    fault_msg_t* pFaultMsg = NULL;
    static int connect_interval = 0;

    if (NULL == run)
    {
        return;
    }

    if (NULL == run->pDbOper)
    {
        return;
    }

    if (!run->iLogDBOperConnectStatus)
    {
        connect_interval++;

        if (connect_interval >= 60 * 200)
        {
            if (run->pLogDbOper->Connect(g_StrConLog, (char*)"") < 0)
            {
                run->iLogDBOperConnectStatus = 0;
            }
            else
            {
                run->iLogDBOperConnectStatus = 1;
            }

            connect_interval = 0;
        }
    }

#ifdef MULTI_THR

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_FaultMsgQueueLock);

#endif

    while (!g_FaultMsgQueue.empty())
    {
        pFaultMsg = (fault_msg_t*) g_FaultMsgQueue.front();
        g_FaultMsgQueue.pop_front();

        if (NULL != pFaultMsg)
        {
            break;
        }
    }

#ifdef MULTI_THR

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_FaultMsgQueueLock);

#endif

    if (NULL != pFaultMsg)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "scan_fault_msg_list() \
    \r\n In Param: \
    \r\n DeviceIndex=%u \
    \r\n DeviceID=%s \
    \r\n FaultType=%u \
    \r\n Priority=%s \
    \r\n Method=%s \
    \r\n LogTime=%u \
    \r\n Deseription=%s \
    \r\n ", pFaultMsg->uLogicDeviceIndex, pFaultMsg->strLogicDeviceID, pFaultMsg->uFaultType, pFaultMsg->strPriority, pFaultMsg->strMethod, pFaultMsg->uLogTime, pFaultMsg->strDeseription);

        if (!run->iLogDBOperConnectStatus)
        {
            iRet = fault_msg_proc(pFaultMsg, run->pDbOper, NULL);
        }
        else
        {
            iRet = fault_msg_proc(pFaultMsg, run->pDbOper, run->pLogDbOper);
        }

        fault_msg_free(pFaultMsg);
        osip_free(pFaultMsg);
        pFaultMsg = NULL;
    }

    return;
}
#endif


#if DECS("��ʱ��������")
int alarm_timer_init(alarm_timer_t** node)
{
    *node = (alarm_timer_t*)osip_malloc(sizeof(alarm_timer_t));

    if (*node == NULL)
    {
        return -1;
    }

    (*node)->iAlarmTimers = 0;
    (*node)->iAlarmInterval = 0;
    (*node)->iAlarmTimersCount = 0;
    (*node)->iAlarmIntervalCount = 0;

    (*node)->count = 0;
    (*node)->pos = -1;

    return 0;
}

void alarm_timer_free(alarm_timer_t* node)
{
    if (node == NULL)
    {
        return;
    }

    node->iAlarmTimers = 0;
    node->iAlarmInterval = 0;
    node->iAlarmTimersCount = 0;
    node->iAlarmIntervalCount = 0;

    node->count = 0;
    node->pos = -1;
    return;
}

int alarm_timer_list_init()
{
    g_AlarmTimerList = (alarm_timer_list_t*)osip_malloc(sizeof(alarm_timer_list_t));

    if (g_AlarmTimerList == NULL)
    {
        return -1;
    }

    /* init timer list*/
    g_AlarmTimerList->timer_list = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_AlarmTimerList->timer_list)
    {
        osip_free(g_AlarmTimerList);
        g_AlarmTimerList = NULL;
        return -1;
    }

    osip_list_init(g_AlarmTimerList->timer_list);

#ifdef MULTI_THR
    /* init smutex */
    g_AlarmTimerList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_AlarmTimerList->lock)
    {
        osip_free(g_AlarmTimerList->timer_list);
        g_AlarmTimerList->timer_list = NULL;
        osip_free(g_AlarmTimerList);
        g_AlarmTimerList = NULL;
        return -1;
    }

#endif
    return 0;
}

int alarm_timer_list_clean()
{
    alarm_timer_t* timer_node = NULL;

    if (NULL == g_AlarmTimerList)
    {
        return -1;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_AlarmTimerList->lock);
#endif

    while (!osip_list_eol(g_AlarmTimerList->timer_list, 0))
    {
        timer_node = (alarm_timer_t*)osip_list_get(g_AlarmTimerList->timer_list, 0);

        if (NULL != timer_node)
        {
            osip_list_remove(g_AlarmTimerList->timer_list, 0);
            alarm_timer_free(timer_node);
            osip_free(timer_node);
            timer_node = NULL;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AlarmTimerList->lock);
#endif

    return 0;

}

void alarm_timer_list_free()
{
    if (NULL != g_AlarmTimerList)
    {
        alarm_timer_list_clean();
        osip_free(g_AlarmTimerList->timer_list);
        g_AlarmTimerList->timer_list = NULL;
#ifdef MULTI_THR
        osip_mutex_destroy((struct osip_mutex*)g_AlarmTimerList->lock);
        g_AlarmTimerList->lock = NULL;
#endif
    }

    return;
}

int alarm_timer_list_add(alarm_timer_t* node)
{
    if (NULL == g_AlarmTimerList || NULL == node)
    {
        return -1;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_AlarmTimerList->lock);
#endif

    osip_list_add(g_AlarmTimerList->timer_list, node, -1);

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AlarmTimerList->lock);
#endif

    return 0;
}

alarm_timer_t* alarm_timer_find(alarm_timer_type type, int pos)
{
    int i;
    alarm_timer_t* node = NULL;

    if (NULL == g_AlarmTimerList)
    {
        return NULL;
    }

    switch (type)
    {
        case DEVICE_OFFLINE:
            if (pos < 0)
            {
                return NULL;
            }

            CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_AlarmTimerList->lock);

            i = 0;

            while (!osip_list_eol(g_AlarmTimerList->timer_list, i))
            {
                node = (alarm_timer_t*)osip_list_get(g_AlarmTimerList->timer_list, i);

                if (NULL == node)
                {
                    i++;
                    continue;
                }

                if (node->type == DEVICE_OFFLINE && node->pos == pos)
                {
                    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AlarmTimerList->lock);
                    return node;
                }

                i++;
            }

            CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AlarmTimerList->lock);
            break;

        default:
            break;
    }

    return NULL;
}

int alarm_timer_use(alarm_timer_type type, int pos, int iAlarmTimers, int iAlarmInterval)
{
    alarm_timer_t* node;

    switch (type)
    {
        case DEVICE_OFFLINE:
            if (pos < 0)
            {
                return -1;
            }

            node = alarm_timer_find(DEVICE_OFFLINE, pos);

            if (node == NULL)
            {
                alarm_timer_init(&node);
                node->type = DEVICE_OFFLINE;
                node->iAlarmTimers = iAlarmTimers;
                node->iAlarmInterval = iAlarmInterval;
                node->pos = pos;
                alarm_timer_list_add(node);
            }

            break;

        default:
            break;
    }

    return 0;
}

int alarm_timer_remove(alarm_timer_type type, int pos)
{
    alarm_timer_t* node;

    switch (type)
    {
        case DEVICE_OFFLINE:
            if (pos < 0)
            {
                return -1;
            }

            node = alarm_timer_find(DEVICE_OFFLINE, pos);

            if (node != NULL)
            {
                node->pos = -1;
            }

            break;

        default:
            break;
    }

    return 0;
}

void scan_alarm_timer_list(DBOper* pAlarm_dboper)
{
    int pos = 0;

    alarm_timer_t* timer_node = NULL;

    if (NULL == g_AlarmTimerList || NULL == pAlarm_dboper)
    {
        return;
    }

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_AlarmTimerList->lock);

    if (osip_list_size(g_AlarmTimerList->timer_list) <= 0)
    {
        CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AlarmTimerList->lock);
        return;
    }

    pos = 0;

    while (!osip_list_eol(g_AlarmTimerList->timer_list, pos))
    {
        timer_node = (alarm_timer_t*)osip_list_get(g_AlarmTimerList->timer_list, pos);

        if (NULL == timer_node)
        {
            pos++;
            continue;
        }

        switch (timer_node->type)
        {
            case DEVICE_OFFLINE:
                if (timer_node->pos < 0) /*invalid pos then remove the timer */
                {
                    osip_list_remove(g_AlarmTimerList->timer_list, pos);
                    alarm_timer_free(timer_node);
                    osip_free(timer_node);
                    timer_node = NULL;
                    pos++;
                    continue;
                }

                break;


            default:
                break;
        }

        pos++;
    }

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AlarmTimerList->lock);

    return;
}
#endif

#if DECS("��ʱ��������")
/*****************************************************************************
 �� �� ��  : alarm_duration_init
 ��������  : �澯��ʱ����ṹ��ʼ��
 �������  : alarm_duration_t** node
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_duration_init(alarm_duration_t** node)
{
    *node = (alarm_duration_t*)osip_malloc(sizeof(alarm_duration_t));

    if (*node == NULL)
    {
        return -1;
    }

    (*node)->uDeviceIndex = 0;
    (*node)->iPriority = 0;
    (*node)->iMethod = 0;
    (*node)->strDeseription[0] = '\0';
    (*node)->uStartTime = 0;
    (*node)->iDurationTime = 0;
    (*node)->iDurationTimeCount = 0;
    (*node)->iFlag = 0;
    return 0;
}

/*****************************************************************************
 �� �� ��  : alarm_duration_free
 ��������  : �澯��ʱ����ṹ�ͷ�
 �������  : alarm_duration_t* node
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void alarm_duration_free(alarm_duration_t* node)
{
    if (node == NULL)
    {
        return;
    }

    node->uDeviceIndex = 0;
    node->iPriority = 0;
    node->iMethod = 0;
    memset(node->strDeseription, 0, 512 + 4);
    node->uStartTime = 0;
    node->iDurationTime = 0;
    node->iDurationTimeCount = 0;
    node->iFlag = 0;

    return;
}

/*****************************************************************************
 �� �� ��  : alarm_duration_list_init
 ��������  : �澯��ʱ������г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_duration_list_init()
{
    g_AlarmDurationList = (alarm_duration_list_t*)osip_malloc(sizeof(alarm_duration_list_t));

    if (g_AlarmDurationList == NULL)
    {
        return -1;
    }

    /* init duration list*/
    g_AlarmDurationList->duration_list = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_AlarmDurationList->duration_list)
    {
        osip_free(g_AlarmDurationList);
        g_AlarmDurationList = NULL;
        return -1;
    }

    osip_list_init(g_AlarmDurationList->duration_list);

#ifdef MULTI_THR
    /* init smutex */
    g_AlarmDurationList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_AlarmDurationList->lock)
    {
        osip_free(g_AlarmDurationList->duration_list);
        g_AlarmDurationList->duration_list = NULL;
        osip_free(g_AlarmDurationList);
        g_AlarmDurationList = NULL;
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : alarm_duration_list_clean
 ��������  : �澯��ʱ�����������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_duration_list_clean()
{
    alarm_duration_t* duration_node = NULL;

    if (NULL == g_AlarmDurationList)
    {
        return -1;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_AlarmDurationList->lock);
#endif

    while (!osip_list_eol(g_AlarmDurationList->duration_list, 0))
    {
        duration_node = (alarm_duration_t*)osip_list_get(g_AlarmDurationList->duration_list, 0);

        if (NULL != duration_node)
        {
            osip_list_remove(g_AlarmDurationList->duration_list, 0);
            alarm_duration_free(duration_node);
            osip_free(duration_node);
            duration_node = NULL;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AlarmDurationList->lock);
#endif

    return 0;

}

/*****************************************************************************
 �� �� ��  : alarm_duration_list_clean
 ��������  : �澯��ʱ��������ͷ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void alarm_duration_list_free()
{
    if (NULL != g_AlarmDurationList)
    {
        alarm_duration_list_clean();
        osip_free(g_AlarmDurationList->duration_list);
        g_AlarmDurationList->duration_list = NULL;
#ifdef MULTI_THR
        osip_mutex_destroy((struct osip_mutex*)g_AlarmDurationList->lock);
        g_AlarmDurationList->lock = NULL;
#endif
    }

    return;
}

/*****************************************************************************
 �� �� ��  : alarm_duration_find
 ��������  : ��ʱ�澯����
 �������  : unsigned int uDeviceIndex
             int iPriority
             int iMethod
             char* strDeseription
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
alarm_duration_t* alarm_duration_find(unsigned int uDeviceIndex, int iPriority, int iMethod, char* strDeseription)
{
    int i = 0;
    alarm_duration_t* node = NULL;

    if (NULL == g_AlarmDurationList)
    {
        return NULL;
    }

    i = 0;

    while (!osip_list_eol(g_AlarmDurationList->duration_list, i))
    {
        node = (alarm_duration_t*)osip_list_get(g_AlarmDurationList->duration_list, i);

        if (NULL == node)
        {
            i++;
            continue;
        }

        if (node->uDeviceIndex == uDeviceIndex
            && node->iPriority == iPriority
            && node->iMethod == iMethod
            && 0 == sstrcmp(node->strDeseription, strDeseription))
        {
            return node;
        }

        i++;
    }

    return NULL;
}

/*****************************************************************************
 �� �� ��  : alarm_duration_use
 ��������  : ��ʱ�澯ʹ��
 �������  : unsigned int uDeviceIndex
             int iPriority
             int iMethod
             char* strDeseription
             unsigned int uStartTime
             int iAlarmDuration
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_duration_use(unsigned int uDeviceIndex, int iPriority, int iMethod, char* strDeseription, unsigned int uStartTime, int iAlarmDuration)
{
    alarm_duration_t* node = NULL;

    if (NULL == g_AlarmDurationList || NULL == g_AlarmDurationList->duration_list)
    {
        return -1;
    }

    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_AlarmDurationList->lock);

    node = alarm_duration_find(uDeviceIndex, iPriority, iMethod, strDeseription);

    if (node == NULL)
    {
        alarm_duration_init(&node);
        node->uDeviceIndex = uDeviceIndex;
        node->iPriority = iPriority;
        node->iMethod = iMethod;
        node->iFlag = 0;

        if (NULL != strDeseription && '\0' != strDeseription[0])
        {
            osip_strncpy(node->strDeseription, strDeseription, 512);
        }

        node->uStartTime = uStartTime;
        node->iDurationTime = iAlarmDuration;
        node->iDurationTimeCount = iAlarmDuration;
        node->iFlag = 0;

        osip_list_add(g_AlarmDurationList->duration_list, node, -1);
    }
    else
    {
        node->uStartTime = uStartTime;
        node->iDurationTime = iAlarmDuration;
        node->iDurationTimeCount = iAlarmDuration;
    }

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AlarmDurationList->lock);

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_alarm_duration_list
 ��������  : ɨ��澯��ʱ�������
 �������  : DBOper* pAlarm_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_alarm_duration_list(DBOper* pAlarm_dboper)
{
    int iRet = 0;
    int pos = 0;
    alarm_duration_t* duration_node = NULL;
    alarm_duration_t* proc_duration_node = NULL;
    needtoproc_alarmduration_queue needproc;

    if (NULL == g_AlarmDurationList || NULL == pAlarm_dboper)
    {
        return;
    }

    needproc.clear();
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_AlarmDurationList->lock);

    if (osip_list_size(g_AlarmDurationList->duration_list) <= 0)
    {
        CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AlarmDurationList->lock);
        return;
    }

    pos = 0;

    while (!osip_list_eol(g_AlarmDurationList->duration_list, pos))
    {
        duration_node = (alarm_duration_t*)osip_list_get(g_AlarmDurationList->duration_list, pos);

        if (NULL == duration_node)
        {
            osip_list_remove(g_AlarmDurationList->duration_list, pos);
            continue;
        }

        if (duration_node->iFlag == 1) /*invalid pos then remove the duration */
        {
            osip_list_remove(g_AlarmDurationList->duration_list, pos);
            alarm_duration_free(duration_node);
            osip_free(duration_node);
            duration_node = NULL;
            continue;
        }

        duration_node->iDurationTimeCount--;

        if (duration_node->iDurationTimeCount <= 0)
        {
            needproc.push_back(duration_node);
        }

        pos++;
    }

    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AlarmDurationList->lock);

    /* ������Ҫ¼��� */
    while (!needproc.empty())
    {
        proc_duration_node = (alarm_duration_t*) needproc.front();
        needproc.pop_front();

        if (NULL != proc_duration_node)
        {
            iRet = alarm_endtime_proc(proc_duration_node->uDeviceIndex, proc_duration_node->uStartTime + proc_duration_node->iDurationTime, pAlarm_dboper);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_alarm_duration_list() alarm_endtime_proc Error:DeviceIndex=%u, iRet=%d \r\n", proc_duration_node->uDeviceIndex, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_alarm_duration_list() alarm_endtime_proc OK:DeviceIndex=%u, iRet=%d \r\n", proc_duration_node->uDeviceIndex, iRet);
            }

            proc_duration_node->iFlag = 1;
        }
    }

    needproc.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : alarm_endtime_proc
 ��������  : �澯��������
 �������  : unsigned int uDeviceIndex
             unsigned int uAlarmEndTime
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��18�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_endtime_proc(unsigned int uDeviceIndex, unsigned int uAlarmEndTime, DBOper* pDboper)
{
    int iRet = 0;
    int iInAlarmDeployment = 0;
    int iAlarmEndDayTime = 0;
    time_t now = time(NULL);
    struct tm tp = {0};

    now = uAlarmEndTime;

    localtime_r(&now, &tp);
    iAlarmEndDayTime = tp.tm_hour * 3600 + tp.tm_min * 60 + tp.tm_sec;

    iInAlarmDeployment = find_device_is_in_alarm_deployment(uDeviceIndex, iAlarmEndDayTime);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "alarm_endtime_proc() find_device_is_in_alarm_deployment:DeviceIndex=%u, tm_hour=%d, tm_min=%d, tm_sec=%d, AlarmDayTime=%d, InAlarmDeployment=%d \r\n", uDeviceIndex, tp.tm_hour, tp.tm_min, tp.tm_sec, iAlarmEndDayTime, iInAlarmDeployment);

    if (iInAlarmDeployment)
    {
        /* �鿴�Ƿ��б����������� */
        iRet = FindHasPlanLinkageForAutoEndAlarmProc(uDeviceIndex, pDboper);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_endtime_proc() FindHasPlanLinkageForAutoEndAlarmProc Error:iRet=%d \r\n", iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_endtime_proc() FindHasPlanLinkageForAutoEndAlarmProc OK:iRet=%d \r\n", iRet);
        }
    }

    return iRet;
}
#endif


#if DECS("����ʱ�̲��Զ���")

/*****************************************************************************
 �� �� ��  : alarm_time_sched_init
 ��������  : ����ʱ����Խṹ��ʼ��
 �������  : alarm_time_sched_t** alarm_time_sched
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��29�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_time_sched_init(alarm_time_sched_t** alarm_time_sched)
{
    *alarm_time_sched = (alarm_time_sched_t*)osip_malloc(sizeof(alarm_time_sched_t));

    if (*alarm_time_sched == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_time_sched_init() exit---: *alarm_time_sched Smalloc Error \r\n");
        return -1;
    }

    (*alarm_time_sched)->UUID[0] = '\0';
    (*alarm_time_sched)->iBeginTime = 0;
    (*alarm_time_sched)->iEndTime = 0;
    (*alarm_time_sched)->del_mark = 0;

    return 0;
}

/*****************************************************************************
 �� �� ��  : alarm_time_sched_free
 ��������  : ����ʱ����Խṹ��ʼ���ͷ�
 �������  : alarm_time_sched_t* alarm_time_sched
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void alarm_time_sched_free(alarm_time_sched_t* alarm_time_sched)
{
    if (alarm_time_sched == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_time_sched_free() exit---: Param Error \r\n");
        return;
    }

    memset(alarm_time_sched->UUID, 0, 36);
    alarm_time_sched->iBeginTime = 0;
    alarm_time_sched->iEndTime = 0;
    alarm_time_sched->del_mark = 0;

    osip_free(alarm_time_sched);
    alarm_time_sched = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : alarm_time_sched_add
 ��������  : ����ʱ����Խṹ��ʼ�����
 �������  : osip_list_t* pDayOfWeekTimeList
             unsigned int uID
             int iBeginTime
             int iEndTime
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_time_sched_add(osip_list_t* pDayOfWeekTimeList, char* pUUID, int iBeginTime, int iEndTime)
{
    int i = 0;
    alarm_time_sched_t* pAlarmTimeSched = NULL;

    if (pDayOfWeekTimeList == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_time_sched_add() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pUUID || iBeginTime < 0 || iEndTime < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_time_sched_add() exit---: Param 2 Error \r\n");
        return -1;
    }

    if (iEndTime - iBeginTime <= 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_time_sched_add() exit---: EndTime - BeginTime Error:BeginTime=%d, EndTime=%d \r\n", iBeginTime, iEndTime);
        return -1;
    }

    i = alarm_time_sched_init(&pAlarmTimeSched);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_time_sched_add() exit---: Time Config Init Error \r\n");
        return -1;
    }

    osip_strncpy(pAlarmTimeSched->UUID, pUUID, 36);
    pAlarmTimeSched->iBeginTime = iBeginTime;
    pAlarmTimeSched->iEndTime = iEndTime;

    i = osip_list_add(pDayOfWeekTimeList, pAlarmTimeSched, -1); /* add to list tail */

    if (i == -1)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_time_sched_add() exit---: osip_list_add Error \r\n");
        alarm_time_sched_free(pAlarmTimeSched);
        pAlarmTimeSched = NULL;
        return -1;
    }

    return i - 1;
}

/*****************************************************************************
 �� �� ��  : alarm_time_sched_get
 ��������  : ��ȡ����ʱ�����
 �������  : osip_list_t* pDayOfWeekTimeList
             char* pUUID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��29�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
alarm_time_sched_t* alarm_time_sched_get(osip_list_t* pDayOfWeekTimeList, char* pUUID)
{
    int pos = -1;
    alarm_time_sched_t* pAlarmTimeSched = NULL;

    if (NULL == pDayOfWeekTimeList || NULL == pUUID)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_time_sched_get() exit---: Param Error \r\n");
        return NULL;
    }

    if (osip_list_size(pDayOfWeekTimeList) <= 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_time_sched_get() exit---: Day Of Week Time List NULL \r\n");
        return NULL;
    }

    for (pos = 0; pos < osip_list_size(pDayOfWeekTimeList); pos++)
    {
        pAlarmTimeSched = (alarm_time_sched_t*)osip_list_get(pDayOfWeekTimeList, pos);

        if ((NULL == pAlarmTimeSched) || (pAlarmTimeSched->UUID[0] == '\0'))
        {
            continue;
        }

        if (0 == sstrcmp(pAlarmTimeSched->UUID, pUUID))
        {
            return pAlarmTimeSched;
        }
    }

    return NULL;
}

/*****************************************************************************
 �� �� ��  : alarm_deployment_init
 ��������  : ����ʱ�̲��Խṹ��ʼ��
 �������  : alarm_deployment_t** alarm_deployment
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_deployment_init(alarm_deployment_t** alarm_deployment)
{
    *alarm_deployment = (alarm_deployment_t*)osip_malloc(sizeof(alarm_deployment_t));

    if (*alarm_deployment == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_deployment_init() exit---: *alarm_deployment Smalloc Error \r\n");
        return -1;
    }

    (*alarm_deployment)->LogicDeviceIndex = 0;
    (*alarm_deployment)->del_mark = 0;

    /* һ���ʱ����Ϣ���г�ʼ�� */
    (*alarm_deployment)->pDayOfWeekTimeList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == (*alarm_deployment)->pDayOfWeekTimeList)
    {
        osip_free(*alarm_deployment);
        (*alarm_deployment) = NULL;
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_deployment_init() exit---: DayOfWeekTime List Init Error \r\n");
        return -1;
    }

    osip_list_init((*alarm_deployment)->pDayOfWeekTimeList);

    return 0;
}

/*****************************************************************************
 �� �� ��  : alarm_deployment_free
 ��������  : ����ʱ�̲��Խṹ�ͷ�
 �������  : alarm_deployment_t* alarm_deployment
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void alarm_deployment_free(alarm_deployment_t* alarm_deployment)
{
    if (alarm_deployment == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_deployment_free() exit---: Param Error \r\n");
        return;
    }

    alarm_deployment->LogicDeviceIndex = 0;
    alarm_deployment->del_mark = 0;

    /* һ���ʱ����Ϣ���г��ͷ� */
    if (NULL != alarm_deployment->pDayOfWeekTimeList)
    {
        osip_list_special_free(alarm_deployment->pDayOfWeekTimeList, (void (*)(void*))&alarm_time_sched_free);
        osip_free(alarm_deployment->pDayOfWeekTimeList);
        alarm_deployment->pDayOfWeekTimeList = NULL;
    }

    osip_free(alarm_deployment);
    alarm_deployment = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : alarm_deployment_list_init
 ��������  : ����ʱ�̲��Զ��г�ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_deployment_list_init()
{
    g_AlarmDeploymentList = (alarm_deployment_list_t*)osip_malloc(sizeof(alarm_deployment_list_t));

    if (g_AlarmDeploymentList == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_deployment_list_init() exit---: g_AlarmDeploymentList Smalloc Error \r\n");
        return -1;
    }

    g_AlarmDeploymentList->pAlarmDeploymentList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_AlarmDeploymentList->pAlarmDeploymentList)
    {
        osip_free(g_AlarmDeploymentList);
        g_AlarmDeploymentList = NULL;
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_deployment_list_init() exit---: Alarm Deployment List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_AlarmDeploymentList->pAlarmDeploymentList);

#ifdef MULTI_THR
    /* init smutex */
    g_AlarmDeploymentList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_AlarmDeploymentList->lock)
    {
        osip_free(g_AlarmDeploymentList->pAlarmDeploymentList);
        g_AlarmDeploymentList->pAlarmDeploymentList = NULL;
        osip_free(g_AlarmDeploymentList);
        g_AlarmDeploymentList = NULL;
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_deployment_list_init() exit---: Alarm Deployment List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : alarm_deployment_list_free
 ��������  : ����ʱ�̲��Զ����ͷ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void alarm_deployment_list_free()
{
    if (NULL == g_AlarmDeploymentList)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_deployment_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_AlarmDeploymentList->pAlarmDeploymentList)
    {
        osip_list_special_free(g_AlarmDeploymentList->pAlarmDeploymentList, (void (*)(void*))&alarm_deployment_free);
        osip_free(g_AlarmDeploymentList->pAlarmDeploymentList);
        g_AlarmDeploymentList->pAlarmDeploymentList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_AlarmDeploymentList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_AlarmDeploymentList->lock);
        g_AlarmDeploymentList->lock = NULL;
    }

#endif
    osip_free(g_AlarmDeploymentList);
    g_AlarmDeploymentList = NULL;
    return;
}

/*****************************************************************************
 �� �� ��  : alarm_deployment_list_lock
 ��������  : ����ʱ�̲��Զ�������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_deployment_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_AlarmDeploymentList == NULL || g_AlarmDeploymentList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_deployment_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_AlarmDeploymentList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : alarm_deployment_list_unlock
 ��������  : ����ʱ�̲��Զ��н���
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_deployment_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_AlarmDeploymentList == NULL || g_AlarmDeploymentList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_deployment_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_AlarmDeploymentList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_alarm_deployment_list_lock
 ��������  : ����ʱ�̲��Զ�������
 �������  : const char* file
             int line
             const char* func
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int debug_alarm_deployment_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_AlarmDeploymentList == NULL || g_AlarmDeploymentList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "debug_alarm_deployment_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_AlarmDeploymentList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_alarm_deployment_list_unlock
 ��������  : ����ʱ�̲��Զ��н���
 �������  : const char* file
             int line
             const char* func
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int debug_alarm_deployment_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_AlarmDeploymentList == NULL || g_AlarmDeploymentList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "debug_alarm_deployment_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_AlarmDeploymentList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : alarm_deployment_add
 ��������  : ����ʱ�̲������
 �������  : unsigned uID
             int record_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_deployment_add(unsigned int uLogicDeviceIndex)
{
    int i = 0;
    alarm_deployment_t* pAlarmDeployment = NULL;

    if ((NULL == g_AlarmDeploymentList) || (NULL == g_AlarmDeploymentList->pAlarmDeploymentList))
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_deployment_add() exit---: Alarm Deployment List NULL \r\n");
        return -1;
    }

    i = alarm_deployment_init(&pAlarmDeployment);

    if (i != 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_deployment_add() exit---: Alarm Deployment Init Error \r\n");
        return -1;
    }

    pAlarmDeployment->LogicDeviceIndex = uLogicDeviceIndex;

    ALARM_DEPLOYMENT_SMUTEX_LOCK();

    i = osip_list_add(g_AlarmDeploymentList->pAlarmDeploymentList, pAlarmDeployment, -1); /* add to list tail */

    if (i < 0)
    {
        ALARM_DEPLOYMENT_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_deployment_add() exit---: List Add Error \r\n");
        return -1;
    }

    ALARM_DEPLOYMENT_SMUTEX_UNLOCK();
    return i - 1;
}

/*****************************************************************************
 �� �� ��  : alarm_deployment_get
 ��������  : ��ȡ����ʱ�̲���
 �������  : unsigned int uID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
alarm_deployment_t* alarm_deployment_get(unsigned int uLogicDeviceIndex)
{
    int pos = -1;
    alarm_deployment_t* pAlarmDeployment = NULL;

    if (NULL == g_AlarmDeploymentList || NULL == g_AlarmDeploymentList->pAlarmDeploymentList || uLogicDeviceIndex <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_deployment_get() exit---: Param Error \r\n");
        return NULL;
    }

    ALARM_DEPLOYMENT_SMUTEX_LOCK();

    if (osip_list_size(g_AlarmDeploymentList->pAlarmDeploymentList) <= 0)
    {
        ALARM_DEPLOYMENT_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_deployment_get() exit---: Alarm Deployment List NULL \r\n");
        return NULL;
    }

    for (pos = 0; pos < osip_list_size(g_AlarmDeploymentList->pAlarmDeploymentList); pos++)
    {
        pAlarmDeployment = (alarm_deployment_t*)osip_list_get(g_AlarmDeploymentList->pAlarmDeploymentList, pos);

        if ((NULL == pAlarmDeployment) || (pAlarmDeployment->LogicDeviceIndex <= 0))
        {
            continue;
        }

        if (pAlarmDeployment->LogicDeviceIndex == uLogicDeviceIndex)
        {
            ALARM_DEPLOYMENT_SMUTEX_UNLOCK();
            return pAlarmDeployment;
        }
    }

    ALARM_DEPLOYMENT_SMUTEX_UNLOCK();
    return NULL;
}

int get_alarm_deployment_logic_device_index_from_db(vector<unsigned int>& LogicDeviceIndexVector, DBOper* pDBOper)
{
    int iRet = 0;
    int record_count = 0;
    int while_count = 0;
    string strSQL = "";

    if (NULL == pDBOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "get_alarm_deployment_logic_device_index_from_db() exit---: Param Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "SELECT DISTINCT LogicDeviceIndex FROM AlarmDeployment ORDER BY LogicDeviceIndex ASC";

    record_count = pDBOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "check_alarm_deployment_sched_config_from_db_to_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "check_alarm_deployment_sched_config_from_db_to_list() ErrorMsg=%s\r\n", pDBOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "check_alarm_deployment_sched_config_from_db_to_list() AlarmDeploymentSchedConfig No Record \r\n");
        return 0;
    }

    /* ѭ���������ݿ�*/
    do
    {
        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "check_alarm_deployment_sched_config_from_db_to_list() While Count=%d \r\n", while_count);
        }

        unsigned int uLogicDeviceIndex = 0;

        pDBOper->GetFieldValue("LogicDeviceIndex", uLogicDeviceIndex);

        iRet = AddDeviceIndexToDeviceIndexVector(LogicDeviceIndexVector, uLogicDeviceIndex);
    }
    while (pDBOper->MoveNext() >= 0);

    return 0;
}

int set_alarm_deployment_list_del_mark(int del_mark)
{
    int pos1 = -1;
    int pos2 = -1;
    alarm_deployment_t* pAlarmDeployment = NULL;
    alarm_time_sched_t* pAlarmTimeSched = NULL;

    if ((NULL == g_AlarmDeploymentList) || (NULL == g_AlarmDeploymentList->pAlarmDeploymentList))
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "set_alarm_deployment_list_del_mark() exit---: Param Error \r\n");
        return -1;
    }

    ALARM_DEPLOYMENT_SMUTEX_LOCK();

    if (osip_list_size(g_AlarmDeploymentList->pAlarmDeploymentList) <= 0)
    {
        ALARM_DEPLOYMENT_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "set_alarm_deployment_list_del_mark() exit---: Alarm Deployment List NULL \r\n");
        return 0;
    }

    for (pos1 = 0; pos1 < osip_list_size(g_AlarmDeploymentList->pAlarmDeploymentList); pos1++)
    {
        pAlarmDeployment = (alarm_deployment_t*)osip_list_get(g_AlarmDeploymentList->pAlarmDeploymentList, pos1);

        if (NULL == pAlarmDeployment)
        {
            continue;
        }

        pAlarmDeployment->del_mark = del_mark;

        /* ʱ����Ϣ���� */
        if (NULL == pAlarmDeployment->pDayOfWeekTimeList)
        {
            continue;
        }

        if (osip_list_size(pAlarmDeployment->pDayOfWeekTimeList) <= 0)
        {
            continue;
        }

        for (pos2 = 0; pos2 < osip_list_size(pAlarmDeployment->pDayOfWeekTimeList); pos2++)
        {
            pAlarmTimeSched = (alarm_time_sched_t*)osip_list_get(pAlarmDeployment->pDayOfWeekTimeList, pos2);

            if (NULL == pAlarmTimeSched)
            {
                continue;
            }

            pAlarmTimeSched->del_mark = del_mark;
        }
    }

    ALARM_DEPLOYMENT_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : delete_alarm_deployment_from_list_by_mark
 ��������  : ���ݱ���ʱ�̲���ɾ����ʶ�Ƴ�����Ҫ������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int delete_alarm_deployment_from_list_by_mark()
{
    int pos1 = -1;
    int pos2 = -1;
    alarm_deployment_t* pAlarmDeployment = NULL;
    alarm_time_sched_t* pAlarmTimeSched = NULL;

    if ((NULL == g_AlarmDeploymentList) || (NULL == g_AlarmDeploymentList->pAlarmDeploymentList))
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "delete_alarm_deployment_from_list_by_mark() exit---: Param Error \r\n");
        return -1;
    }

    ALARM_DEPLOYMENT_SMUTEX_LOCK();

    if (osip_list_size(g_AlarmDeploymentList->pAlarmDeploymentList) <= 0)
    {
        ALARM_DEPLOYMENT_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "delete_alarm_deployment_from_list_by_mark() exit---: Alarm Deployment List NULL \r\n");
        return 0;
    }

    pos1 = 0;

    while (!osip_list_eol(g_AlarmDeploymentList->pAlarmDeploymentList, pos1))
    {
        pAlarmDeployment = (alarm_deployment_t*)osip_list_get(g_AlarmDeploymentList->pAlarmDeploymentList, pos1);

        if (NULL == pAlarmDeployment)
        {
            osip_list_remove(g_AlarmDeploymentList->pAlarmDeploymentList, pos1);
            continue;
        }

        if (pAlarmDeployment->del_mark == 1)
        {
            osip_list_remove(g_AlarmDeploymentList->pAlarmDeploymentList, pos1);
            alarm_deployment_free(pAlarmDeployment);
            pAlarmDeployment = NULL;
        }
        else
        {
            /* ʱ����Ϣ���� */
            if (NULL == pAlarmDeployment->pDayOfWeekTimeList)
            {
                pos1++;
                continue;
            }

            if (osip_list_size(pAlarmDeployment->pDayOfWeekTimeList) <= 0)
            {
                pos1++;
                continue;
            }

            pos2 = 0;

            while (!osip_list_eol(pAlarmDeployment->pDayOfWeekTimeList, pos2))
            {
                pAlarmTimeSched = (alarm_time_sched_t*)osip_list_get(pAlarmDeployment->pDayOfWeekTimeList, pos2);

                if (NULL == pAlarmTimeSched)
                {
                    osip_list_remove(pAlarmDeployment->pDayOfWeekTimeList, pos2);
                    continue;
                }

                if (pAlarmTimeSched->del_mark == 1)
                {
                    osip_list_remove(pAlarmDeployment->pDayOfWeekTimeList, pos2);
                    alarm_time_sched_free(pAlarmTimeSched);
                    pAlarmTimeSched = NULL;
                }
                else
                {
                    //printf("delete_record_info_from_list_by_mark() Not Del RecordInfo:device_index=%u, record_type=%d, stream_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pRecordInfo->stream_type, pos);
                    pos2++;
                }
            }

            pos1++;
        }
    }

    ALARM_DEPLOYMENT_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : check_alarm_time_sched_config_from_db_to_list
 ��������  : �����ݿ�ı���ʱ�����ͬ�����ڴ���
 �������  : DBOper* pDBOper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_alarm_time_sched_config_from_db_to_list(alarm_deployment_t* pAlarmDeployment, DBOper* pDBOper)
{
    int iRet = 0;
    time_t now;
    struct tm p_tm = { 0 };
    string strSQL = "";
    int iWeekDay = 0;
    char strWeekDay[16] = {0};
    char strLogicDeviceIndex[32] = {0};
    int record_count = 0;
    int while_count = 0;
    alarm_time_sched_t* pAlarmTimeSched = NULL;

    if (NULL == pAlarmDeployment || NULL == pDBOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "check_alarm_time_sched_config_from_db_to_list() exit---: DB Oper Error \r\n");
        return -1;
    }

    /* ��ȡ����ʱ��ʱ��, �齨��ѯ���� */
    now = time(NULL);
    localtime_r(&now, &p_tm);

    /* p_tm.tm_wday; ����:ȡֵ����Ϊ[0,6]������0���������죬1��������һ���Դ����� */

    if (p_tm.tm_wday == 0)
    {
        iWeekDay = 7;
    }
    else
    {
        iWeekDay = p_tm.tm_wday;
    }

    snprintf(strWeekDay, 16, "%d", iWeekDay);
    snprintf(strLogicDeviceIndex, 32, "%u", pAlarmDeployment->LogicDeviceIndex);

    /* ѭ������ʱ�����Ϣ */
    strSQL.clear();
    strSQL = "select * from AlarmDeployment WHERE LogicDeviceIndex = ";
    strSQL += strLogicDeviceIndex;
    strSQL += " AND DayInWeek = ";
    strSQL += strWeekDay;
    strSQL += " order by BeginTime asc";

    record_count = pDBOper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "check_alarm_time_sched_config_from_db_to_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "check_alarm_time_sched_config_from_db_to_list() ErrorMsg=%s\r\n", pDBOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "check_alarm_time_sched_config_from_db_to_list() No Record Found \r\n");
        return 0;
    }

    /* ѭ���������ݿ�*/
    do
    {
        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "check_alarm_time_sched_config_from_db_to_list() While Count=%d \r\n", while_count);
        }

        string strUUID = "";
        int iBeginTime = 0;
        int iEndTime = 0;

        pDBOper->GetFieldValue("UUID", strUUID);
        pDBOper->GetFieldValue("BeginTime", iBeginTime);
        pDBOper->GetFieldValue("EndTime", iEndTime);

        if (iEndTime - iBeginTime <= 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "check_alarm_time_sched_config_from_db_to_list() EndTime Small to BeginTime:BeginTime=%d, EndTime=%d \r\n", iBeginTime, iEndTime);
            continue;
        }

        pAlarmTimeSched = alarm_time_sched_get(pAlarmDeployment->pDayOfWeekTimeList, (char*)strUUID.c_str());

        if (NULL != pAlarmTimeSched)
        {
            pAlarmTimeSched->del_mark = 0;

            if (pAlarmTimeSched->iBeginTime != iBeginTime)
            {
                pAlarmTimeSched->iBeginTime = iBeginTime;
            }

            if (pAlarmTimeSched->iEndTime != iEndTime)
            {
                pAlarmTimeSched->iEndTime = iEndTime;
            }
        }
        else
        {
            /* ��ӵ����� */
            iRet = alarm_time_sched_add(pAlarmDeployment->pDayOfWeekTimeList, (char*)strUUID.c_str(), iBeginTime, iEndTime);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "check_alarm_time_sched_config_from_db_to_list() alarm_time_sched_add Error:iRet=%d, BeginTime=%d, EndTime=%d \r\n", iRet, iBeginTime, iEndTime);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "check_alarm_time_sched_config_from_db_to_list() alarm_time_sched_add OK:iRet=%d, BeginTime=%d, EndTime=%d \r\n", iRet, iBeginTime, iEndTime);
            }
        }
    }
    while (pDBOper->MoveNext() >= 0);

    return 0;
}

/*****************************************************************************
 �� �� ��  : check_alarm_deployment_from_db_to_list
 ��������  : ����������Ƿ����µ��������ݵ�����ʱ�̲��Ա�
 �������  : DBOper* pDBOper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_alarm_deployment_from_db_to_list(DBOper* pDBOper)
{
    int iRet = 0;
    int index = 0;
    int iAlarmDeploymentPos = 0;
    unsigned int uLogicDeviceIndex = 0;
    int iLogicDeviceIndexCount = 0;
    alarm_deployment_t* pAlarmDeployment = NULL;
    vector<unsigned int> LogicDeviceIndexVector;

    if (NULL == pDBOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "check_alarm_deployment_from_db_to_list() exit---: DB Oper Error \r\n");
        return -1;
    }

    /* ��ȡ���ݿ����߼��豸���� */
    LogicDeviceIndexVector.clear();
    iRet = get_alarm_deployment_logic_device_index_from_db(LogicDeviceIndexVector, pDBOper);

    iLogicDeviceIndexCount = LogicDeviceIndexVector.size();

    if (iLogicDeviceIndexCount <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "check_alarm_deployment_from_db_to_list() exit---: Get Logic Device Index NULL \r\n");
        return 0;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "check_alarm_deployment_from_db_to_list() LogicDeviceIndexCount=%d \r\n", iLogicDeviceIndexCount);

    /* ����¼����Ա���������ݣ�ѭ����ȡ¼��ʱ����Ա���������� */
    for (index = 0; index < iLogicDeviceIndexCount; index++)
    {
        /* ��ȡ¼��������� */
        uLogicDeviceIndex = LogicDeviceIndexVector[index];

        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "check_alarm_deployment_from_db_to_list() index=%d, LogicDeviceIndex=%u \r\n", index, uLogicDeviceIndex);

        /* ���Ҷ�Ӧ��ʱ�������Ϣ */
        pAlarmDeployment = alarm_deployment_get(uLogicDeviceIndex);

        if (NULL == pAlarmDeployment) /* ��������ڣ������ */
        {
            iAlarmDeploymentPos = alarm_deployment_add(uLogicDeviceIndex);

            if (iAlarmDeploymentPos < 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "check_alarm_deployment_from_db_to_list() alarm_deployment_add Error: LogicDeviceIndex=%u\r\n", uLogicDeviceIndex);
                continue;
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "check_alarm_deployment_from_db_to_list() alarm_deployment_add OK: LogicDeviceIndex=%u\r\n", uLogicDeviceIndex);

                pAlarmDeployment = alarm_deployment_get(uLogicDeviceIndex);

                if (NULL == pAlarmDeployment) /* ��������� */
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "check_alarm_deployment_from_db_to_list() alarm_deployment_get Error: LogicDeviceIndex=%u\r\n", uLogicDeviceIndex);
                    continue;
                }
            }
        }
        else
        {
            pAlarmDeployment->del_mark = 0;
        }

        iRet |= check_alarm_time_sched_config_from_db_to_list(pAlarmDeployment, pDBOper);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "check_alarm_deployment_from_db_to_list() check_alarm_time_sched_config_from_db_to_list Error:iRet=%d\r\n", iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "check_alarm_deployment_from_db_to_list() check_alarm_time_sched_config_from_db_to_list OK:iRet=%d\r\n", iRet);
        }
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : find_device_is_in_alarm_deployment
 ��������  : ����豸�Ƿ��ڲ�����ʱ���
 �������  : unsigned int uDeviceIndex
             int iTime
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��3�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int find_device_is_in_alarm_deployment(unsigned int uDeviceIndex, int iAlarmTime)
{
    int pos1 = 0;
    int pos2 = 0;
    alarm_deployment_t* pAlarmDeployment = NULL;
    alarm_time_sched_t* pAlarmTimeSched = NULL;

    if (uDeviceIndex <= 0 || iAlarmTime <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "find_device_is_in_alarm_deployment() exit---: Param 1 Error \r\n");
        return -1;
    }

    if ((NULL == g_AlarmDeploymentList) || (NULL == g_AlarmDeploymentList->pAlarmDeploymentList))
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "find_device_is_in_alarm_deployment() exit---: Param 2 Error \r\n");
        return 0;
    }

    ALARM_DEPLOYMENT_SMUTEX_LOCK();

    if (osip_list_size(g_AlarmDeploymentList->pAlarmDeploymentList) <= 0)
    {
        ALARM_DEPLOYMENT_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "find_device_is_in_alarm_deployment() exit---: Alarm Deployment List NULL \r\n");
        return 0;
    }

    for (pos1 = 0; pos1 < osip_list_size(g_AlarmDeploymentList->pAlarmDeploymentList); pos1++)
    {
        pAlarmDeployment = (alarm_deployment_t*)osip_list_get(g_AlarmDeploymentList->pAlarmDeploymentList, pos1);

        if (NULL == pAlarmDeployment || pAlarmDeployment->del_mark == 1)
        {
            continue;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "find_device_is_in_alarm_deployment() LogicDeviceIndex=%u \r\n", pAlarmDeployment->LogicDeviceIndex);

        if (pAlarmDeployment->LogicDeviceIndex == uDeviceIndex)
        {
            for (pos2 = 0; pos2 < osip_list_size(pAlarmDeployment->pDayOfWeekTimeList); pos2++)
            {
                pAlarmTimeSched = (alarm_time_sched_t*)osip_list_get(pAlarmDeployment->pDayOfWeekTimeList, pos2);

                if ((NULL == pAlarmTimeSched) || (pAlarmTimeSched->del_mark == 1))
                {
                    continue;
                }

                //DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "find_device_is_in_alarm_deployment() pos2=%d:BeginTime=%d, EndTime=%d \r\n", pos2, pAlarmTimeSched->iBeginTime, pAlarmTimeSched->iEndTime);

                if (iAlarmTime >= pAlarmTimeSched->iBeginTime && iAlarmTime <= pAlarmTimeSched->iEndTime)
                {
                    ALARM_DEPLOYMENT_SMUTEX_UNLOCK();
                    return 1;
                }
            }
        }
    }

    ALARM_DEPLOYMENT_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : show_alarm_time_deployment
 ��������  : ��ʾ����ʱ�̲�����Ϣ
 �������  : int sock
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��12��9��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void show_alarm_time_deployment(int sock)
{
    int pos1 = 0;
    int pos2 = 0;
    alarm_deployment_t* pAlarmDeployment = NULL;
    alarm_time_sched_t* pAlarmTimeSched = NULL;
    char strLine[] = "\r---------------------------------------------------------------------\r\n";
    char strHead[] = "\rDevice Index Begin Time End Time UUID                                \r\n";
    char rbuf[256] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if ((NULL == g_AlarmDeploymentList) || (NULL == g_AlarmDeploymentList->pAlarmDeploymentList))
    {
        return;
    }

    if (osip_list_size(g_AlarmDeploymentList->pAlarmDeploymentList) <= 0)
    {
        return;
    }

    for (pos1 = 0; pos1 < osip_list_size(g_AlarmDeploymentList->pAlarmDeploymentList); pos1++)
    {
        pAlarmDeployment = (alarm_deployment_t*)osip_list_get(g_AlarmDeploymentList->pAlarmDeploymentList, pos1);

        if (NULL == pAlarmDeployment || pAlarmDeployment->del_mark == 1)
        {
            continue;
        }

        for (pos2 = 0; pos2 < osip_list_size(pAlarmDeployment->pDayOfWeekTimeList); pos2++)
        {
            pAlarmTimeSched = (alarm_time_sched_t*)osip_list_get(pAlarmDeployment->pDayOfWeekTimeList, pos2);

            if ((NULL == pAlarmTimeSched) || (pAlarmTimeSched->del_mark == 1))
            {
                continue;
            }


            snprintf(rbuf, 256, "\r%-12u %-10d %-8d %-36s\r\n", pAlarmDeployment->LogicDeviceIndex, pAlarmTimeSched->iBeginTime, pAlarmTimeSched->iEndTime, pAlarmTimeSched->UUID);

            if (sock > 0)
            {
                send(sock, rbuf, strlen(rbuf), 0);
            }
        }
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

/*****************************************************************************
 �� �� ��  : scan_alarm_deployment_list
 ��������  : ɨ�豨��ʱ�̲��Զ�������
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_alarm_deployment_list()
{
    int i = 0;
    alarm_deployment_t* pAlarmDeployment = NULL;

    if ((NULL == g_AlarmDeploymentList) || (NULL == g_AlarmDeploymentList->pAlarmDeploymentList))
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "scan_alarm_deployment_list() exit---: Param Error \r\n");
        return;
    }

    if (osip_list_size(g_AlarmDeploymentList->pAlarmDeploymentList) <= 0)
    {
        return;
    }

    for (i = 0; i < osip_list_size(g_AlarmDeploymentList->pAlarmDeploymentList); i++)
    {
        pAlarmDeployment = (alarm_deployment_t*)osip_list_get(g_AlarmDeploymentList->pAlarmDeploymentList, i);

        if (NULL == pAlarmDeployment)
        {
            continue;
        }

        /* ʱ��α������� */
        //iRet = AlarmDeploymentSchedConfigProc(pAlarmDeployment);
    }

    return;
}

/*****************************************************************************
 �� �� ��  : AlarmDeploymentConfig_db_refresh_proc
 ��������  : ���ñ���ʱ�����������Ϣ���ݿ���²�����ʶ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��10��18��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int AlarmDeploymentConfig_db_refresh_proc()
{
    if (1 == db_AlarmDeployment_reload_mark) /* ����ִ�� */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ʱ������������ݿ���Ϣ����ͬ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Alarm Deployment Info database information are synchronized");
        return 0;
    }

    db_AlarmDeployment_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 �� �� ��  : check_AlarmDeploymentConfig_need_to_reload_begin
 ��������  : ����Ƿ���Ҫͬ������ʱ��������ÿ�ʼ
 �������  : DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��10��18��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void check_AlarmDeploymentConfig_need_to_reload_begin(DBOper* pDboper)
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_AlarmDeployment_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ������ʱ������������ݿ���Ϣ: ��ʼ---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization Alarm Deployment info database information: begain---");

    /* ���ò���ʱ�̶��е�ɾ����ʶ */
    set_alarm_deployment_list_del_mark(1);

    /* �����ݿ��еı仯����ͬ�����ڴ� */
    check_alarm_deployment_from_db_to_list(pDboper);

    return;
}

/*****************************************************************************
 �� �� ��  : check_AlarmDeploymentConfig_need_to_reload_end
 ��������  : ����Ƿ���Ҫͬ������ʱ��������ñ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��10��18��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void check_AlarmDeploymentConfig_need_to_reload_end()
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_AlarmDeployment_reload_mark)
    {
        return;
    }

    /* ɾ������Ĳ�����ʱ����Ϣ */
    delete_alarm_deployment_from_list_by_mark();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ������ʱ������������ݿ���Ϣ: ����---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization Alarm Deployment info database information: end---");
    db_AlarmDeployment_reload_mark = 0;

    return;
}
#endif

/*****************************************************************************
 �� �� ��  : alarm_msg_proc
 ��������  : ������Ϣ����
 �������  : alarm_msg_t* pAlarmMsg
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��3�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int alarm_msg_proc(alarm_msg_t* pAlarmMsg, DBOper* pDboper, DBOper* pDbLogoper)
{
    int iRet = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    int iInAlarmDeployment = 0;
    unsigned int uAlarmStartTime = 0;
    unsigned int uAlarmEndTime = 0;
    unsigned int uAlarmDurationTime = 0;
    int iAlarmStartDayTime = 0;
    //int iAlarmEndDayTime = 0;
    int iPriority = 0;
    int iMethod = 0;
    time_t now = time(NULL);
    struct tm tp = {0};
    int iAlarmType = 0;
    string strNote = "";

    if (NULL == pAlarmMsg || NULL == pDboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "alarm_msg_proc() exit---: Alarm Msg Error \r\n");
        return -1;
    }

    if (EV9000_DEVICETYPE_DECODER == pAlarmMsg->iAlarmDeviceType) /* �������ı�����Ϣ */
    {
        if (g_AlarmMsgSendToUserFlag)
        {
            if (pAlarmMsg->strPriority[0] != '\0'
                && 0 != sstrcmp(pAlarmMsg->strPriority, (char*)"0")) /* �����ȼ�Ϊ0�ı�����֪ͨ���ͻ��� */
            {
                /* ���ͱ������ݸ��ͻ����û� */
                outPacket.SetRootTag("Notify");

                AccNode = outPacket.CreateElement((char*)"CmdType");
                outPacket.SetElementValue(AccNode, (char*)"Alarm");

                AccNode = outPacket.CreateElement((char*)"SN");
                outPacket.SetElementValue(AccNode, pAlarmMsg->strSN);

                AccNode = outPacket.CreateElement((char*)"DeviceID");
                outPacket.SetElementValue(AccNode, pAlarmMsg->strDeviceID);

                AccNode = outPacket.CreateElement((char*)"AlarmPriority");
                outPacket.SetElementValue(AccNode, pAlarmMsg->strPriority);

                AccNode = outPacket.CreateElement((char*)"AlarmMethod");
                outPacket.SetElementValue(AccNode, pAlarmMsg->strMethod);

                AccNode = outPacket.CreateElement((char*)"AlarmTime");
                outPacket.SetElementValue(AccNode, pAlarmMsg->strAlarmStartTime);

                AccNode = outPacket.CreateElement((char*)"StopTime");
                outPacket.SetElementValue(AccNode, pAlarmMsg->strAlarmEndTime);

                AccNode = outPacket.CreateElement((char*)"AlarmDescription");
                outPacket.SetElementValue(AccNode, pAlarmMsg->strDeseription);

                iRet = send_alarm_to_user_proc2(pAlarmMsg->uDeviceIndex, outPacket, pDboper);

                if (iRet < 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "���͸澯��Ϣ�������û�ʧ��");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "send_alarm_to_user_proc2 Error.");
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_msg_proc() send_alarm_to_user_proc2 Error\r\n");
                }
                else if (iRet > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���͸澯��Ϣ�������û��ɹ�");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "send_alarm_to_user_proc2 OK.");
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() send_alarm_to_user_proc2 OK\r\n");
                }
            }
        }

        return iRet;
    }

    iAlarmType = strtol(pAlarmMsg->strMethod, NULL, 10);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "alarm_msg_proc() iAlarmType=%d \r\n", iAlarmType);

    if (0 == pAlarmMsg->iAlarmMsgType) /* ��ʼ������Ϣ */
    {
        /* �ж��Ƿ�Ϊ���ܷ��� */
        if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pAlarmMsg->iAlarmDeviceType
            || EV9000_DEVICETYPE_VIDEODIAGNOSIS == pAlarmMsg->iAlarmDeviceType)
        {
            if (EV9000_BEHAVIORANALYSIS_UNDIRECTEDTRIPWIRE == iAlarmType
                || EV9000_BEHAVIORANALYSIS_DIRECTEDTRIPWIRE == iAlarmType
                || EV9000_BEHAVIORANALYSIS_IRRUPT == iAlarmType
                || EV9000_BEHAVIORANALYSIS_TRANSBOUNDARY == iAlarmType
                || EV9000_BEHAVIORANALYSIS_ABANDON == iAlarmType
                || EV9000_BEHAVIORANALYSIS_OBJLOST == iAlarmType
                || EV9000_BEHAVIORANALYSIS_WANDER == iAlarmType
                || EV9000_BEHAVIORANALYSIS_STOPCAR == iAlarmType
                || EV9000_BEHAVIORANALYSIS_OUTDOOR == iAlarmType
                || EV9000_BEHAVIORANALYSIS_LEAVEPOS == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_DEFINITIONABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_LIGHTABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_COLORFULABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_SNOWABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOHUNGS == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_SIGNALLOSSABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_SCREENTINGLEABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOOCCLUSIONABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_NIGHTMODE == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_LUMLOW == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_CONTRASTLOW == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_UPHEAVAL == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_MOSAIC == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_STRIPE == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_PTZ == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_SCENECHANGE == iAlarmType)
            {
                /* ������ʼʱ�� */
                if (pAlarmMsg->strAlarmStartTime[0] != '\0')
                {
                    uAlarmStartTime = analysis_time(pAlarmMsg->strAlarmStartTime);

                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() AlarmStartTime=%u \r\n", uAlarmStartTime);

                    /* ��鱨����Ϣ�Ƿ��ڲ�����ʱ��� */
                    if (uAlarmStartTime > 0)
                    {
                        now = uAlarmStartTime;

                        localtime_r(&now, &tp);
                        iAlarmStartDayTime = tp.tm_hour * 3600 + tp.tm_min * 60 + tp.tm_sec;

                        iInAlarmDeployment = find_device_is_in_alarm_deployment(pAlarmMsg->uDeviceIndex, iAlarmStartDayTime);

                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "alarm_msg_proc() find_device_is_in_alarm_deployment:DeviceIndex=%u, tm_hour=%d, tm_min=%d, tm_sec=%d, AlarmDayTime=%d, InAlarmDeployment=%d \r\n", pAlarmMsg->uDeviceIndex, tp.tm_hour, tp.tm_min, tp.tm_sec, iAlarmStartDayTime, iInAlarmDeployment);

                        if (iInAlarmDeployment)
                        {
                            /* �鿴�Ƿ��б����������� */
                            iRet = FindHasPlanLinkageForAlarmProc(pAlarmMsg, 0, pDboper);

                            if (0 != iRet)
                            {
                                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_msg_proc() FindHasPlanLinkageForAlarmProc Error:iRet=%d \r\n", iRet);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() FindHasPlanLinkageForAlarmProc OK:iRet=%d \r\n", iRet);
                            }
                        }

                        /* ��������ʱ�� */
                        if (pAlarmMsg->strAlarmEndTime[0] != '\0')
                        {
                            uAlarmEndTime = analysis_time(pAlarmMsg->strAlarmEndTime);
                        }
                        else
                        {
                            uAlarmEndTime = uAlarmStartTime + pAlarmMsg->iAlarmLengthOfTime;
                        }

                        uAlarmDurationTime = uAlarmEndTime - uAlarmStartTime;

                        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() AlarmEndTime=%u, AlarmDurationTime=%u \r\n", uAlarmEndTime, uAlarmDurationTime);

                        if (uAlarmEndTime > 0 && uAlarmDurationTime > 0)
                        {
                            /* ��ӵ���ʱ������ */
                            iPriority = osip_atoi(pAlarmMsg->strPriority);
                            iMethod = osip_atoi(pAlarmMsg->strMethod);

                            iRet = alarm_duration_use(pAlarmMsg->uDeviceIndex, iPriority, iMethod, pAlarmMsg->strDeseription, uAlarmStartTime, uAlarmDurationTime);

                            if (0 != iRet)
                            {
                                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_msg_proc() alarm_duration_use Error:iRet=%d \r\n", iRet);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() alarm_duration_use OK:iRet=%d \r\n", iRet);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            /* ������ʼʱ�� */
            if (pAlarmMsg->strAlarmStartTime[0] != '\0')
            {
                uAlarmStartTime = analysis_time(pAlarmMsg->strAlarmStartTime);

                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() AlarmStartTime=%u \r\n", uAlarmStartTime);

                /* ��鱨����Ϣ�Ƿ��ڲ�����ʱ��� */
                if (uAlarmStartTime > 0)
                {
                    now = uAlarmStartTime;

                    localtime_r(&now, &tp);
                    iAlarmStartDayTime = tp.tm_hour * 3600 + tp.tm_min * 60 + tp.tm_sec;

                    iInAlarmDeployment = find_device_is_in_alarm_deployment(pAlarmMsg->uDeviceIndex, iAlarmStartDayTime);

                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "alarm_msg_proc() find_device_is_in_alarm_deployment:DeviceIndex=%u, tm_hour=%d, tm_min=%d, tm_sec=%d, AlarmDayTime=%d, InAlarmDeployment=%d \r\n", pAlarmMsg->uDeviceIndex, tp.tm_hour, tp.tm_min, tp.tm_sec, iAlarmStartDayTime, iInAlarmDeployment);

                    if (iInAlarmDeployment)
                    {
                        /* �鿴�Ƿ��б����������� */
                        iRet = FindHasPlanLinkageForAlarmProc(pAlarmMsg, 0, pDboper);

                        if (0 != iRet)
                        {
                            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_msg_proc() FindHasPlanLinkageForAlarmProc Error:iRet=%d \r\n", iRet);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() FindHasPlanLinkageForAlarmProc OK:iRet=%d \r\n", iRet);
                        }
                    }

                    /* ��������ʱ�� */
                    if (pAlarmMsg->strAlarmEndTime[0] != '\0')
                    {
                        uAlarmEndTime = analysis_time(pAlarmMsg->strAlarmEndTime);
                    }
                    else
                    {
                        uAlarmEndTime = uAlarmStartTime + pAlarmMsg->iAlarmLengthOfTime;
                    }

                    uAlarmDurationTime = uAlarmEndTime - uAlarmStartTime;

                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() AlarmEndTime=%u, AlarmDurationTime=%u \r\n", uAlarmEndTime, uAlarmDurationTime);

                    if (uAlarmEndTime > 0 && uAlarmDurationTime > 0)
                    {
                        /* ��ӵ���ʱ������ */
                        iPriority = osip_atoi(pAlarmMsg->strPriority);
                        iMethod = osip_atoi(pAlarmMsg->strMethod);

                        iRet = alarm_duration_use(pAlarmMsg->uDeviceIndex, iPriority, iMethod, pAlarmMsg->strDeseription, uAlarmStartTime, uAlarmDurationTime);

                        if (0 != iRet)
                        {
                            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_msg_proc() alarm_duration_use Error:iRet=%d \r\n", iRet);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() alarm_duration_use OK:iRet=%d \r\n", iRet);
                        }
                    }
                }
            }
        }

        if (iAlarmType == EV9000_BEHAVIORANALYSIS_IRRUPT
            || iAlarmType == EV9000_ALARM_MOTION_DETECTION)
        {
            strNote = "ϵͳ�յ��ⲿ�������ƶ���ⱨ����Ϣ:��λID=";
            strNote += pAlarmMsg->strDeviceID;
            strNote += ", ��������=";
            strNote += pAlarmMsg->strMethod;
            strNote += ", ��������=";
            strNote += pAlarmMsg->strPriority;
            strNote += ", ������������=";
            strNote += pAlarmMsg->strDeseription;

            //iRet = shdb_system_operate_cmd_proc(EV9000_SHDB_DVR_MOTION_DETECTION, (char*)strNote.c_str());
        }
        else
        {
            if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pAlarmMsg->iAlarmDeviceType
                || EV9000_DEVICETYPE_VIDEODIAGNOSIS == pAlarmMsg->iAlarmDeviceType)
            {
                if (EV9000_BEHAVIORANALYSIS_UNDIRECTEDTRIPWIRE == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_DIRECTEDTRIPWIRE == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_IRRUPT == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_TRANSBOUNDARY == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_ABANDON == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_OBJLOST == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_WANDER == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_STOPCAR == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_OUTDOOR == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_LEAVEPOS == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_DEFINITIONABNORMALITIES == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_LIGHTABNORMALITIES == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_COLORFULABNORMALITIES == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_SNOWABNORMALITIES == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOHUNGS == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_SIGNALLOSSABNORMALITIES == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_SCREENTINGLEABNORMALITIES == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOOCCLUSIONABNORMALITIES == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_NIGHTMODE == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_LUMLOW == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_CONTRASTLOW == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_UPHEAVAL == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_MOSAIC == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_STRIPE == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_PTZ == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_SCENECHANGE == iAlarmType)
                {
                    strNote = "ϵͳ�յ��ⲿ�����ı�����Ϣ:��λID=";
                    strNote += pAlarmMsg->strDeviceID;
                    strNote += ", ��������=";
                    strNote += pAlarmMsg->strMethod;
                    strNote += ", ��������=";
                    strNote += pAlarmMsg->strPriority;
                    strNote += ", ������������=";
                    strNote += pAlarmMsg->strDeseription;

                    //iRet = shdb_system_operate_cmd_proc(EV9000_SHDB_DVR_EXTERN_TRIGGER, (char*)strNote.c_str());
                }
            }
            else
            {
                strNote = "ϵͳ�յ��ⲿ�����ı�����Ϣ:��λID=";
                strNote += pAlarmMsg->strDeviceID;
                strNote += ", ��������=";
                strNote += pAlarmMsg->strMethod;
                strNote += ", ��������=";
                strNote += pAlarmMsg->strPriority;
                strNote += ", ������������=";
                strNote += pAlarmMsg->strDeseription;

                //iRet = shdb_system_operate_cmd_proc(EV9000_SHDB_DVR_EXTERN_TRIGGER, (char*)strNote.c_str());
            }
        }
    }
    else if (1 == pAlarmMsg->iAlarmMsgType) /* ��ʼ������Ϣ */
    {
        /* �ж��Ƿ�Ϊ���ܷ��� */
        if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pAlarmMsg->iAlarmDeviceType
            || EV9000_DEVICETYPE_VIDEODIAGNOSIS == pAlarmMsg->iAlarmDeviceType)
        {
            if (EV9000_BEHAVIORANALYSIS_UNDIRECTEDTRIPWIRE == iAlarmType
                || EV9000_BEHAVIORANALYSIS_DIRECTEDTRIPWIRE == iAlarmType
                || EV9000_BEHAVIORANALYSIS_IRRUPT == iAlarmType
                || EV9000_BEHAVIORANALYSIS_TRANSBOUNDARY == iAlarmType
                || EV9000_BEHAVIORANALYSIS_ABANDON == iAlarmType
                || EV9000_BEHAVIORANALYSIS_OBJLOST == iAlarmType
                || EV9000_BEHAVIORANALYSIS_WANDER == iAlarmType
                || EV9000_BEHAVIORANALYSIS_STOPCAR == iAlarmType
                || EV9000_BEHAVIORANALYSIS_OUTDOOR == iAlarmType
                || EV9000_BEHAVIORANALYSIS_LEAVEPOS == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_DEFINITIONABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_LIGHTABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_COLORFULABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_SNOWABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOHUNGS == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_SIGNALLOSSABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_SCREENTINGLEABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOOCCLUSIONABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_NIGHTMODE == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_LUMLOW == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_CONTRASTLOW == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_UPHEAVAL == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_MOSAIC == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_STRIPE == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_PTZ == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_SCENECHANGE == iAlarmType)
            {
                /* ������ʼʱ�� */
                if (pAlarmMsg->strAlarmStartTime[0] != '\0')
                {
                    uAlarmStartTime = analysis_time(pAlarmMsg->strAlarmStartTime);

                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() AlarmStartTime=%u \r\n", uAlarmStartTime);

                    /* ��鱨����Ϣ�Ƿ��ڲ�����ʱ��� */
                    if (uAlarmStartTime > 0)
                    {
                        now = uAlarmStartTime;

                        localtime_r(&now, &tp);
                        iAlarmStartDayTime = tp.tm_hour * 3600 + tp.tm_min * 60 + tp.tm_sec;

                        iInAlarmDeployment = find_device_is_in_alarm_deployment(pAlarmMsg->uDeviceIndex, iAlarmStartDayTime);

                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "alarm_msg_proc() find_device_is_in_alarm_deployment:DeviceIndex=%u, tm_hour=%d, tm_min=%d, tm_sec=%d, AlarmDayTime=%d, InAlarmDeployment=%d \r\n", pAlarmMsg->uDeviceIndex, tp.tm_hour, tp.tm_min, tp.tm_sec, iAlarmStartDayTime, iInAlarmDeployment);

                        if (iInAlarmDeployment)
                        {
                            /* �鿴�Ƿ��б����������� */
                            iRet = FindHasPlanLinkageForAlarmProc(pAlarmMsg, 0, pDboper);

                            if (0 != iRet)
                            {
                                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_msg_proc() FindHasPlanLinkageForAlarmProc Error:iRet=%d \r\n", iRet);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() FindHasPlanLinkageForAlarmProc OK:iRet=%d \r\n", iRet);
                            }
                        }

                        /* ��������ʱ�� */
                        if (pAlarmMsg->strAlarmEndTime[0] != '\0')
                        {
                            uAlarmEndTime = analysis_time(pAlarmMsg->strAlarmEndTime);
                        }
                        else
                        {
                            uAlarmEndTime = uAlarmStartTime + pAlarmMsg->iAlarmLengthOfTime;
                        }

                        uAlarmDurationTime = uAlarmEndTime - uAlarmStartTime;

                        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() AlarmEndTime=%u, AlarmDurationTime=%u \r\n", uAlarmEndTime, uAlarmDurationTime);

                        if (uAlarmEndTime > 0 && uAlarmDurationTime > 0)
                        {
                            /* ��ӵ���ʱ������ */
                            iPriority = osip_atoi(pAlarmMsg->strPriority);
                            iMethod = osip_atoi(pAlarmMsg->strMethod);

                            iRet = alarm_duration_use(pAlarmMsg->uDeviceIndex, iPriority, iMethod, pAlarmMsg->strDeseription, uAlarmStartTime, uAlarmDurationTime);

                            if (0 != iRet)
                            {
                                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_msg_proc() alarm_duration_use Error:iRet=%d \r\n", iRet);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() alarm_duration_use OK:iRet=%d \r\n", iRet);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            /* ������ʼʱ�� */
            if (pAlarmMsg->strAlarmStartTime[0] != '\0')
            {
                uAlarmStartTime = analysis_time(pAlarmMsg->strAlarmStartTime);

                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() AlarmStartTime=%u \r\n", uAlarmStartTime);

                /* ��鱨����Ϣ�Ƿ��ڲ�����ʱ��� */
                if (uAlarmStartTime > 0)
                {
                    now = uAlarmStartTime;

                    localtime_r(&now, &tp);
                    iAlarmStartDayTime = tp.tm_hour * 3600 + tp.tm_min * 60 + tp.tm_sec;

                    iInAlarmDeployment = find_device_is_in_alarm_deployment(pAlarmMsg->uDeviceIndex, iAlarmStartDayTime);

                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "alarm_msg_proc() find_device_is_in_alarm_deployment:DeviceIndex=%u, tm_hour=%d, tm_min=%d, tm_sec=%d, AlarmDayTime=%d, InAlarmDeployment=%d \r\n", pAlarmMsg->uDeviceIndex, tp.tm_hour, tp.tm_min, tp.tm_sec, iAlarmStartDayTime, iInAlarmDeployment);

                    if (iInAlarmDeployment)
                    {
                        /* �鿴�Ƿ��б����������� */
                        iRet = FindHasPlanLinkageForAlarmProc(pAlarmMsg, 0, pDboper);

                        if (0 != iRet)
                        {
                            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_msg_proc() FindHasPlanLinkageForAlarmProc Error:iRet=%d \r\n", iRet);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() FindHasPlanLinkageForAlarmProc OK:iRet=%d \r\n", iRet);
                        }
                    }

                    /* ��������ʱ�� */
                    if (pAlarmMsg->strAlarmEndTime[0] != '\0')
                    {
                        uAlarmEndTime = analysis_time(pAlarmMsg->strAlarmEndTime);
                    }
                    else
                    {
                        uAlarmEndTime = uAlarmStartTime + pAlarmMsg->iAlarmLengthOfTime;
                    }

                    uAlarmDurationTime = uAlarmEndTime - uAlarmStartTime;

                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() AlarmEndTime=%u, AlarmDurationTime=%u \r\n", uAlarmEndTime, uAlarmDurationTime);

                    if (uAlarmEndTime > 0 && uAlarmDurationTime > 0)
                    {
                        /* ��ӵ���ʱ������ */
                        iPriority = osip_atoi(pAlarmMsg->strPriority);
                        iMethod = osip_atoi(pAlarmMsg->strMethod);

                        iRet = alarm_duration_use(pAlarmMsg->uDeviceIndex, iPriority, iMethod, pAlarmMsg->strDeseription, uAlarmStartTime, uAlarmDurationTime);

                        if (0 != iRet)
                        {
                            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_msg_proc() alarm_duration_use Error:iRet=%d \r\n", iRet);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() alarm_duration_use OK:iRet=%d \r\n", iRet);
                        }
                    }
                }
            }
        }

        if (iAlarmType == EV9000_BEHAVIORANALYSIS_IRRUPT
            || iAlarmType == EV9000_ALARM_MOTION_DETECTION)
        {
            strNote = "ϵͳ�յ��ⲿ�������ƶ���ⱨ����Ϣ:��λID=";
            strNote += pAlarmMsg->strDeviceID;
            strNote += ", ��������=";
            strNote += pAlarmMsg->strMethod;
            strNote += ", ��������=";
            strNote += pAlarmMsg->strPriority;
            strNote += ", ������������=";
            strNote += pAlarmMsg->strDeseription;

            //iRet = shdb_system_operate_cmd_proc(EV9000_SHDB_DVR_MOTION_DETECTION, (char*)strNote.c_str());
        }
        else
        {
            if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pAlarmMsg->iAlarmDeviceType
                || EV9000_DEVICETYPE_VIDEODIAGNOSIS == pAlarmMsg->iAlarmDeviceType)
            {
                if (EV9000_BEHAVIORANALYSIS_UNDIRECTEDTRIPWIRE == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_DIRECTEDTRIPWIRE == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_IRRUPT == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_TRANSBOUNDARY == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_ABANDON == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_OBJLOST == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_WANDER == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_STOPCAR == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_OUTDOOR == iAlarmType
                    || EV9000_BEHAVIORANALYSIS_LEAVEPOS == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_DEFINITIONABNORMALITIES == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_LIGHTABNORMALITIES == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_COLORFULABNORMALITIES == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_SNOWABNORMALITIES == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOHUNGS == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_SIGNALLOSSABNORMALITIES == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_SCREENTINGLEABNORMALITIES == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOOCCLUSIONABNORMALITIES == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_NIGHTMODE == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_LUMLOW == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_CONTRASTLOW == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_UPHEAVAL == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_MOSAIC == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_STRIPE == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_PTZ == iAlarmType
                    || EV9000_VIDEOQUALITYDIAGNOSTIC_SCENECHANGE == iAlarmType)
                {
                    strNote = "ϵͳ�յ��ⲿ�����ı�����Ϣ:��λID=";
                    strNote += pAlarmMsg->strDeviceID;
                    strNote += ", ��������=";
                    strNote += pAlarmMsg->strMethod;
                    strNote += ", ��������=";
                    strNote += pAlarmMsg->strPriority;
                    strNote += ", ������������=";
                    strNote += pAlarmMsg->strDeseription;

                    //iRet = shdb_system_operate_cmd_proc(EV9000_SHDB_DVR_EXTERN_TRIGGER, (char*)strNote.c_str());
                }
            }
            else
            {
                strNote = "ϵͳ�յ��ⲿ�����ı�����Ϣ:��λID=";
                strNote += pAlarmMsg->strDeviceID;
                strNote += ", ��������=";
                strNote += pAlarmMsg->strMethod;
                strNote += ", ��������=";
                strNote += pAlarmMsg->strPriority;
                strNote += ", ������������=";
                strNote += pAlarmMsg->strDeseription;

                //iRet = shdb_system_operate_cmd_proc(EV9000_SHDB_DVR_EXTERN_TRIGGER, (char*)strNote.c_str());
            }
        }
    }
    else if (2 == pAlarmMsg->iAlarmMsgType) /* ����������Ϣ */
    {
        /* �ж��Ƿ�Ϊ���ܷ��� */
        if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pAlarmMsg->iAlarmDeviceType
            || EV9000_DEVICETYPE_VIDEODIAGNOSIS == pAlarmMsg->iAlarmDeviceType)
        {
            if (EV9000_BEHAVIORANALYSIS_UNDIRECTEDTRIPWIRE == iAlarmType
                || EV9000_BEHAVIORANALYSIS_DIRECTEDTRIPWIRE == iAlarmType
                || EV9000_BEHAVIORANALYSIS_IRRUPT == iAlarmType
                || EV9000_BEHAVIORANALYSIS_TRANSBOUNDARY == iAlarmType
                || EV9000_BEHAVIORANALYSIS_ABANDON == iAlarmType
                || EV9000_BEHAVIORANALYSIS_OBJLOST == iAlarmType
                || EV9000_BEHAVIORANALYSIS_WANDER == iAlarmType
                || EV9000_BEHAVIORANALYSIS_STOPCAR == iAlarmType
                || EV9000_BEHAVIORANALYSIS_OUTDOOR == iAlarmType
                || EV9000_BEHAVIORANALYSIS_LEAVEPOS == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_DEFINITIONABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_LIGHTABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_COLORFULABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_SNOWABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOHUNGS == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_SIGNALLOSSABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_SCREENTINGLEABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOOCCLUSIONABNORMALITIES == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_NIGHTMODE == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_LUMLOW == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_CONTRASTLOW == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_UPHEAVAL == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_MOSAIC == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_STRIPE == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_PTZ == iAlarmType
                || EV9000_VIDEOQUALITYDIAGNOSTIC_SCENECHANGE == iAlarmType)
            {
                /* ������ʼʱ�� */
                if (pAlarmMsg->strAlarmStartTime[0] != '\0')
                {
                    uAlarmEndTime = analysis_time(pAlarmMsg->strAlarmStartTime);

                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() AlarmEndTime=%u \r\n", uAlarmEndTime);

                    /* ��鱨����Ϣ�Ƿ��ڲ�����ʱ��� */
                    if (uAlarmEndTime > 0)
                    {
                        now = uAlarmEndTime;

                        localtime_r(&now, &tp);
                        iAlarmStartDayTime = tp.tm_hour * 3600 + tp.tm_min * 60 + tp.tm_sec;

                        iInAlarmDeployment = find_device_is_in_alarm_deployment(pAlarmMsg->uDeviceIndex, iAlarmStartDayTime);

                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "alarm_msg_proc() find_device_is_in_alarm_deployment:DeviceIndex=%u, tm_hour=%d, tm_min=%d, tm_sec=%d, AlarmDayTime=%d, InAlarmDeployment=%d \r\n", pAlarmMsg->uDeviceIndex, tp.tm_hour, tp.tm_min, tp.tm_sec, iAlarmStartDayTime, iInAlarmDeployment);

                        if (iInAlarmDeployment)
                        {
                            /* �鿴�Ƿ��б����������� */
                            iRet = FindHasPlanLinkageForAlarmProc(pAlarmMsg, 1, pDboper);

                            if (0 != iRet)
                            {
                                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_msg_proc() FindHasPlanLinkageForAlarmProc Error:iRet=%d \r\n", iRet);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() FindHasPlanLinkageForAlarmProc OK:iRet=%d \r\n", iRet);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            /* ������ʼʱ�� */
            if (pAlarmMsg->strAlarmStartTime[0] != '\0')
            {
                uAlarmEndTime = analysis_time(pAlarmMsg->strAlarmStartTime);

                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() AlarmEndTime=%u \r\n", uAlarmEndTime);

                /* ��鱨����Ϣ�Ƿ��ڲ�����ʱ��� */
                if (uAlarmEndTime > 0)
                {
                    now = uAlarmEndTime;

                    localtime_r(&now, &tp);
                    iAlarmStartDayTime = tp.tm_hour * 3600 + tp.tm_min * 60 + tp.tm_sec;

                    iInAlarmDeployment = find_device_is_in_alarm_deployment(pAlarmMsg->uDeviceIndex, iAlarmStartDayTime);

                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "alarm_msg_proc() find_device_is_in_alarm_deployment:DeviceIndex=%u, tm_hour=%d, tm_min=%d, tm_sec=%d, AlarmDayTime=%d, InAlarmDeployment=%d \r\n", pAlarmMsg->uDeviceIndex, tp.tm_hour, tp.tm_min, tp.tm_sec, iAlarmStartDayTime, iInAlarmDeployment);

                    if (iInAlarmDeployment)
                    {
                        /* �鿴�Ƿ��б����������� */
                        iRet = FindHasPlanLinkageForAlarmProc(pAlarmMsg, 1, pDboper);

                        if (0 != iRet)
                        {
                            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_msg_proc() FindHasPlanLinkageForAlarmProc Error:iRet=%d \r\n", iRet);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() FindHasPlanLinkageForAlarmProc OK:iRet=%d \r\n", iRet);
                        }
                    }
                }
            }
        }

        strNote = "ϵͳ�յ��ⲿ�����ı��������Ϣ:��λID=";
        strNote += pAlarmMsg->strDeviceID;
        strNote += ", ��������=";
        strNote += pAlarmMsg->strMethod;
        strNote += ", ��������=";
        strNote += pAlarmMsg->strPriority;
        strNote += ", ������������=";
        strNote += pAlarmMsg->strDeseription;

        //iRet = shdb_system_operate_cmd_proc(EV9000_SHDB_DVR_SYSTEM_ALARM_RESET, (char*)strNote.c_str());
    }

    /* �ж��Ƿ�Ϊ���ܷ��� */
    if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pAlarmMsg->iAlarmDeviceType
        || EV9000_DEVICETYPE_VIDEODIAGNOSIS == pAlarmMsg->iAlarmDeviceType)
    {
        if (EV9000_BEHAVIORANALYSIS_UNDIRECTEDTRIPWIRE == iAlarmType
            && EV9000_BEHAVIORANALYSIS_DIRECTEDTRIPWIRE == iAlarmType
            && EV9000_BEHAVIORANALYSIS_IRRUPT == iAlarmType
            && EV9000_BEHAVIORANALYSIS_TRANSBOUNDARY == iAlarmType
            && EV9000_BEHAVIORANALYSIS_ABANDON == iAlarmType
            && EV9000_BEHAVIORANALYSIS_OBJLOST == iAlarmType
            && EV9000_BEHAVIORANALYSIS_WANDER == iAlarmType
            && EV9000_BEHAVIORANALYSIS_STOPCAR == iAlarmType
            && EV9000_BEHAVIORANALYSIS_OUTDOOR == iAlarmType
            && EV9000_BEHAVIORANALYSIS_LEAVEPOS == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_DEFINITIONABNORMALITIES == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_LIGHTABNORMALITIES == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_COLORFULABNORMALITIES == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_SNOWABNORMALITIES == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOHUNGS == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_SIGNALLOSSABNORMALITIES == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_SCREENTINGLEABNORMALITIES == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_VIDEOOCCLUSIONABNORMALITIES == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_NIGHTMODE == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_LUMLOW == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_CONTRASTLOW == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_UPHEAVAL == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_MOSAIC == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_STRIPE == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_PTZ == iAlarmType
            && EV9000_VIDEOQUALITYDIAGNOSTIC_SCENECHANGE == iAlarmType)
        {
            /* ������ʼʱ�� */
            if (pAlarmMsg->strAlarmStartTime[0] != '\0')
            {
                uAlarmStartTime = analysis_time(pAlarmMsg->strAlarmStartTime);

                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() uAlarmStartTime=%u \r\n", uAlarmStartTime);
            }

            /* ��������ʱ�� */
            if (pAlarmMsg->strAlarmEndTime[0] != '\0')
            {
                uAlarmEndTime = analysis_time(pAlarmMsg->strAlarmEndTime);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() AlarmEndTime=%u \r\n", uAlarmEndTime);
            }
        }
    }

    /* ����������д�뱨����¼��*/
    iRet = write_alarm_to_db_proc(pAlarmMsg, uAlarmStartTime, uAlarmEndTime, pDbLogoper);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_msg_proc() write_alarm_to_db_proc Error:iRet=%d \r\n", iRet);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() write_alarm_to_db_proc OK:iRet=%d \r\n", iRet);
    }

    if (g_AlarmMsgSendToUserFlag)
    {
        if (pAlarmMsg->strPriority[0] != '\0'
            && 0 != sstrcmp(pAlarmMsg->strPriority, (char*)"0")) /* �����ȼ�Ϊ0�ı�����֪ͨ���ͻ��� */
        {
            /* ���ͱ������ݸ��ͻ����û� */
            outPacket.SetRootTag("Notify");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"Alarm");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, pAlarmMsg->strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, pAlarmMsg->strDeviceID);

            AccNode = outPacket.CreateElement((char*)"AlarmPriority");
            outPacket.SetElementValue(AccNode, pAlarmMsg->strPriority);

            AccNode = outPacket.CreateElement((char*)"AlarmMethod");
            outPacket.SetElementValue(AccNode, pAlarmMsg->strMethod);

            AccNode = outPacket.CreateElement((char*)"AlarmTime");
            outPacket.SetElementValue(AccNode, pAlarmMsg->strAlarmStartTime);

            AccNode = outPacket.CreateElement((char*)"StopTime");
            outPacket.SetElementValue(AccNode, pAlarmMsg->strAlarmEndTime);

            AccNode = outPacket.CreateElement((char*)"AlarmDescription");
            outPacket.SetElementValue(AccNode, pAlarmMsg->strDeseription);

            iRet = send_alarm_to_user_proc2(pAlarmMsg->uDeviceIndex, outPacket, pDboper);

            if (iRet < 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "���͸澯��Ϣ�������û�ʧ��");
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "send_alarm_to_user_proc2 Error.");
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "alarm_msg_proc() send_alarm_to_user_proc2 Error\r\n");
            }
            else if (iRet > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���͸澯��Ϣ�������û��ɹ�");
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "send_alarm_to_user_proc2 OK.");
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "alarm_msg_proc() send_alarm_to_user_proc2 OK\r\n");
            }
        }
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : write_alarm_to_db_proc
 ��������  : ���澯��Ϣд�����ݿ�
 �������  : alarm_msg_t* pAlarmMsg
             unsigned int uAlarmTime
             unsigned int uAlarmEndTime
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��23�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int write_alarm_to_db_proc(alarm_msg_t* pAlarmMsg, unsigned int uAlarmStartTime, unsigned int uAlarmEndTime, DBOper* pDboper)
{
    int iRet = 0;
    string strInsertSQL = "";
    string strUpdateSQL = "";
    char strAlarmStartTime[32] = {0};
    char strAlarmEndTime[32] = {0};
    char strDeviceIndex[32] = {0};

    if (NULL == pAlarmMsg || NULL == pDboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "write_alarm_to_db_proc() exit---: Param Error \r\n");
        return -1;
    }

    snprintf(strDeviceIndex, 32, "%u", pAlarmMsg->uDeviceIndex);
    snprintf(strAlarmStartTime, 32, "%u", uAlarmStartTime);
    snprintf(strAlarmEndTime, 32, "%u", uAlarmEndTime);

    if (0 == pAlarmMsg->iAlarmMsgType || 1 == pAlarmMsg->iAlarmMsgType) /* ��ʼ������Ϣ */
    {
        /* ����SQL��� */
        strInsertSQL.clear();
        strInsertSQL = "insert into AlarmRecord (LogicDeviceIndex,Type,Level,StartTime,StopTime,Info,Resved1) values (";

        /* �豸���� */
        strInsertSQL += strDeviceIndex;

        strInsertSQL += ",";

        /* �������� */
        if (pAlarmMsg->strMethod[0] != '\0')
        {
            strInsertSQL += pAlarmMsg->strMethod;
        }
        else
        {
            strInsertSQL += "0";
        }

        strInsertSQL += ",";

        /* �������� */
        if (pAlarmMsg->strPriority[0] != '\0')
        {
            strInsertSQL += pAlarmMsg->strPriority;
        }
        else
        {
            strInsertSQL += "0";
        }

        strInsertSQL += ",";

        /* ������ʼʱ�� */
        strInsertSQL += strAlarmStartTime;

        strInsertSQL += ",";

        /* ��������ʱ�� */
        strInsertSQL += strAlarmEndTime;

        strInsertSQL += ",";

        /* �������� */
        strInsertSQL += "'";
        strInsertSQL += pAlarmMsg->strDeseription;
        strInsertSQL += "'";

        strInsertSQL += ",";

        strInsertSQL += "0";

        strInsertSQL += ")";

        iRet = pDboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "write_alarm_to_db_proc() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "write_alarm_to_db_proc() ErrorMsg=%s\r\n", pDboper->GetLastDbErrorMsg());
        }
    }
    else if (2 == pAlarmMsg->iAlarmMsgType) /* ����������Ϣ */
    {
        /* ����SQL��� */
        strUpdateSQL.clear();
        strUpdateSQL = "UPDATE `AlarmRecord` a INNER JOIN (SELECT MAX(`ID`) AS `ID` FROM `AlarmRecord` WHERE `LogicDeviceIndex` = ";
        strUpdateSQL += strDeviceIndex;
        strUpdateSQL += " AND Type = ";
        strUpdateSQL += pAlarmMsg->strMethod;
        strUpdateSQL += " AND Level = ";
        strUpdateSQL += pAlarmMsg->strPriority;
        strUpdateSQL += ") b ON a.ID = b.ID SET `StopTime` = ";
        strUpdateSQL += strAlarmEndTime;

        iRet = pDboper->DB_Update(strUpdateSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "write_alarm_to_db_proc() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "write_alarm_to_db_proc() ErrorMsg=%s\r\n", pDboper->GetLastDbErrorMsg());
        }
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "write_alarm_to_db_proc() Message Error:MsgType=%d\r\n", pAlarmMsg->iAlarmMsgType);
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : send_alarm_to_user_proc
 ��������  : ���澯��Ϣ���͸���Ӧ�Ŀͻ���
 �������  : CPacket& inPacket
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��23�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int send_alarm_to_user_proc(CPacket& inPacket)
{
    int i = 0;

    /* ���͸澯��Ϣ�����߿ͻ��� */
    i = SendMessageToOnlineUser((char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length(), 1);

    return i;
}

/*****************************************************************************
 �� �� ��  : send_alarm_to_user_proc2
 ��������  : ���͸澯��Ϣ�������û����ж��û��Ƿ��иõ�λȨ��
 �������  : unsigned int device_index
             CPacket& inPacket
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��8��15��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int send_alarm_to_user_proc2(unsigned int device_index, CPacket& inPacket, DBOper* pDboper)
{
    int i = 0;

    /* ���͸澯��Ϣ�����߿ͻ��� */
    i = SendMessageToOnlineUser2(device_index, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length(), 1, pDboper);

    return i;
}

/*****************************************************************************
 �� �� ��  : FindHasPlanLinkageForAutoEndAlarmProc
 ��������  : �Զ�����������ʱ������Ƿ��б���������Ϣ������У��򴥷�
 �������  : unsigned int uDeviceIndex
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��04�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int FindHasPlanLinkageForAutoEndAlarmProc(unsigned int uDeviceIndex, DBOper* pDboper)
{
    int i = 0;
    string strSQL = "";
    int record_count = 0;
    int while_count = 0;
    char strDeviceIndex[32] = {0};

    if (uDeviceIndex <= 0 || NULL == pDboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG,  "FindHasPlanLinkageForAutoEndAlarmProc() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    snprintf(strDeviceIndex, 32, "%u", uDeviceIndex);

    /* ����caller_id����ѯԤ����������ȡԤ���ľ������� */
    strSQL.clear();
    strSQL = "select * from PlanLinkageConfig WHERE AlarmSourceID = ";
    strSQL += strDeviceIndex;
    record_count = pDboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "FindHasPlanLinkageForAutoEndAlarmProc() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "FindHasPlanLinkageForAutoEndAlarmProc() ErrorMsg=%s\r\n", pDboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "FindHasPlanLinkageForAutoEndAlarmProc() exit---: No Record Count \r\n");
        return 0;
    }

    /* ѭ�������Ѳ�������� */
    do
    {
        int iStartPlanID = -1;
        int iStopPlanID = -1;
        int iRepeatEnable = -1;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "FindHasPlanLinkageForAutoEndAlarmProc() While Count=%d \r\n", while_count);
        }

        pDboper->GetFieldValue("StartPlanID", iStartPlanID);
        pDboper->GetFieldValue("StopPlanID", iStopPlanID);
        pDboper->GetFieldValue("RepeatEnable", iRepeatEnable);

        /* �澯�������� */
        if (iStopPlanID > 0)
        {
            i = StartPlanActionForAutoEndAlarm(iStopPlanID, pDboper);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "������ʱ�Զ���������Ԥ��,����Դ����=%u,Ԥ��ID=%d", uDeviceIndex, iStopPlanID);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Alarm stop triggering off the plan��Alarm source index = %u,Plan ID=%d", uDeviceIndex, iStopPlanID);
        }
    }
    while (pDboper->MoveNext() >= 0);

    return 0;
}

/*****************************************************************************
 �� �� ��  : FindHasPlanLinkageForAlarmProc
 ��������  : �����Ƿ��б���������Ϣ������У��򴥷�
 �������  : alarm_msg_t* pAlarmMsg
             int iPlanActionType
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��12��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int FindHasPlanLinkageForAlarmProc(alarm_msg_t* pAlarmMsg, int iPlanActionType, DBOper* pDboper)
{
    int i = 0;
    string strSQL = "";
    int record_count = 0;
    int while_count = 0;
    char strDeviceIndex[32] = {0};

    if (NULL == pAlarmMsg || pAlarmMsg->uDeviceIndex <= 0 || NULL == pDboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG,  "FindHasPlanLinkageForAlarmProc() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    snprintf(strDeviceIndex, 32, "%u", pAlarmMsg->uDeviceIndex);

    /* ����caller_id����ѯԤ����������ȡԤ���ľ������� */
    strSQL.clear();
    strSQL = "select * from PlanLinkageConfig WHERE AlarmSourceID = ";
    strSQL += strDeviceIndex;
    record_count = pDboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "FindHasPlanLinkageForAlarmProc() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "FindHasPlanLinkageForAlarmProc() ErrorMsg=%s\r\n", pDboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "FindHasPlanLinkageForAlarmProc() exit---: No Record Count \r\n");
        return 0;
    }

    /* ѭ�������Ѳ�������� */
    do
    {
        int iStartPlanID = -1;
        int iStopPlanID = -1;
        int iRepeatEnable = -1;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "FindHasPlanLinkageForAlarmProc() While Count=%d \r\n", while_count);
        }

        pDboper->GetFieldValue("StartPlanID", iStartPlanID);
        pDboper->GetFieldValue("StopPlanID", iStopPlanID);
        pDboper->GetFieldValue("RepeatEnable", iRepeatEnable);

        /* �澯��ʼ���� */
        if (0 == iPlanActionType && iStartPlanID > 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "������ʼ����Ԥ��,����Դ����=%u,Ԥ��ID=%d", pAlarmMsg->uDeviceIndex, iStartPlanID);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Alarm begins to trigger off the plan��Alarm source index = %u,Plan ID=%d", pAlarmMsg->uDeviceIndex, iStartPlanID);
            i = StartPlanActionForAlarm(iStartPlanID, pAlarmMsg, pDboper);
        }

        /* �澯�������� */
        if (1 == iPlanActionType && iStopPlanID > 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "������������Ԥ��,����Դ����=%u,Ԥ��ID=%d", pAlarmMsg->uDeviceIndex, iStopPlanID);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Alarm stop triggering off the plan��Alarm source index = %u,Plan ID=%d", pAlarmMsg->uDeviceIndex, iStopPlanID);
            i = StartPlanActionForAlarm(iStopPlanID, pAlarmMsg, pDboper);
        }
    }
    while (pDboper->MoveNext() >= 0);

    return 0;
}

/*****************************************************************************
 �� �� ��  : FindHasPlanLinkageForFaultProc
 ��������  : ���ݹ������Ͳ����Ƿ�����������
 �������  : fault_msg_t* pFaultMsg
             int iPlanActionType
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��9��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int FindHasPlanLinkageForFaultProc(fault_msg_t* pFaultMsg, int iPlanActionType, DBOper* pDboper)
{
    int i = 0;
    string strSQL = "";
    int record_count = 0;
    int while_count = 0;
    char strType[32] = {0};

    if (NULL == pFaultMsg || pFaultMsg->uFaultType <= 0 || NULL == pDboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG,  "FindHasPlanLinkageForFaultProc() exit---: DB Oper Error \r\n");
        return -1;
    }

    snprintf(strType, 32, "%u", pFaultMsg->uFaultType);

    /* ���ݹ�������Type����ѯԤ����������ȡԤ���ľ������� */
    strSQL.clear();
    strSQL = "select * from PlanLinkageConfig WHERE AlarmSourceID = ";
    strSQL += strType;
    strSQL += " and Type = ";
    strSQL += strType;
    record_count = pDboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "FindHasPlanLinkageForFaultProc() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "FindHasPlanLinkageForFaultProc() ErrorMsg=%s\r\n", pDboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "FindHasPlanLinkageForFaultProc() exit---: No Record Count \r\n");
        return 0;
    }

    /* ѭ�������Ѳ�������� */
    do
    {
        int iStartPlanID = -1;
        int iStopPlanID = -1;
        int iRepeatEnable = -1;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "FindHasPlanLinkageForFaultProc() While Count=%d \r\n", while_count);
        }

        pDboper->GetFieldValue("StartPlanID", iStartPlanID);
        pDboper->GetFieldValue("StopPlanID", iStopPlanID);
        pDboper->GetFieldValue("RepeatEnable", iRepeatEnable);

        /* �澯��ʼ���� */
        if (0 == iPlanActionType && iStartPlanID > 0)
        {
            i = StartPlanActionForFault(iStartPlanID, pFaultMsg, pDboper);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Triggering off the plan failed,Failure type =%u, Plan ID=%d", pFaultMsg->uFaultType, iStartPlanID);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ϴ���Ԥ��,��������=%u,Ԥ��ID=%d", pFaultMsg->uFaultType, iStartPlanID);
        }

        /* �澯�������� */
        if (1 == iPlanActionType && iStopPlanID > 0)
        {
            i = StartPlanActionForFault(iStopPlanID, pFaultMsg, pDboper);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Triggering off the plan failed,Failure type =%u, Plan ID=%d", pFaultMsg->uFaultType, iStopPlanID);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ϴ���Ԥ��,��������=%u,Ԥ��ID=%d", pFaultMsg->uFaultType, iStopPlanID);
        }
    }
    while (pDboper->MoveNext() >= 0);

    return 0;
}

/*****************************************************************************
 �� �� ��  : StartPlanActionForAutoEndAlarm
 ��������  : �澯�Զ���������������������
 �������  : int iPlanID
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��4�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int StartPlanActionForAutoEndAlarm(int iPlanID, DBOper* pDboper)
{
    int iRet = -1;
    string strSQL = "";
    string strPlanName = "";
    int record_count = 0;
    char strPlanID[32] = {0};
    char pcPlanName[32] = {0};

    if (NULL == pDboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG,  "StartPlanActionForAutoEndAlarm() exit---: DB Oper Error \r\n");
        return -1;
    }

    snprintf(strPlanID, 32, "%d", iPlanID);

    /* ����plan_id����ѯԤ����������ȡԤ���ľ������� */
    strSQL.clear();
    strSQL = "select * from PlanConfig WHERE ID = ";
    strSQL += strPlanID;
    record_count = pDboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StartPlanActionForAutoEndAlarm() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StartPlanActionForAutoEndAlarm() ErrorMsg=%s\r\n", pDboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StartPlanActionForAutoEndAlarm() exit---: No Record Count \r\n");
        return 0;
    }

    pDboper->GetFieldValue("PlanName", strPlanName);

    if (!strPlanName.empty())
    {
        snprintf(pcPlanName, 32, "%s", (char*)strPlanName.c_str());
    }

    iRet = StartPlanActionByPlanIDForAutoEndAlarm(strPlanID, pcPlanName, NULL, pDboper);

    if (0 != iRet)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StartPlanActionForAutoEndAlarm() StartPlanActionByPlanIDForAutoEndAlarm Error:plan_id=%s, iRet=%d\r\n", strPlanID, iRet);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StartPlanActionForAutoEndAlarm() StartPlanActionByPlanIDForAutoEndAlarm OK:plan_id=%s, iRet=%d\r\n", strPlanID, iRet);
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : StartPlanActionForAlarm
 ��������  : ��������������������
 �������  : int iPlanID
             alarm_msg_t* pAlarmMsg
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��12��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int StartPlanActionForAlarm(int iPlanID, alarm_msg_t* pAlarmMsg, DBOper* pDboper)
{
    int iRet = -1;
    string strSQL = "";
    string strPlanName = "";
    int record_count = 0;
    char strPlanID[32] = {0};
    char pcPlanName[32] = {0};

    if (NULL == pDboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG,  "StartPlanActionForAlarm() exit---: DB Oper Error \r\n");
        return -1;
    }

    snprintf(strPlanID, 32, "%d", iPlanID);

    /* ����plan_id����ѯԤ����������ȡԤ���ľ������� */
    strSQL.clear();
    strSQL = "select * from PlanConfig WHERE ID = ";
    strSQL += strPlanID;
    record_count = pDboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StartPlanActionForAlarm() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StartPlanActionForAlarm() ErrorMsg=%s\r\n", pDboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StartPlanActionForAlarm() exit---: No Record Count \r\n");
        return 0;
    }

    pDboper->GetFieldValue("PlanName", strPlanName);

    if (!strPlanName.empty())
    {
        snprintf(pcPlanName, 32, "%s", (char*)strPlanName.c_str());
    }

    iRet = StartPlanActionByPlanIDForAlarm(strPlanID, pcPlanName, pAlarmMsg, pDboper);

    if (0 != iRet)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StartPlanActionForAlarm() StartPlanActionByPlanIDForAlarm Error:plan_id=%s, iRet=%d\r\n", strPlanID, iRet);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StartPlanActionForAlarm() StartPlanActionByPlanIDForAlarm OK:plan_id=%s, iRet=%d\r\n", strPlanID, iRet);
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : StartPlanActionForFault
 ��������  : ��������������������
 �������  : int iPlanID
             fault_msg_t* pFaultMsg
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��12��16��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int StartPlanActionForFault(int iPlanID, fault_msg_t* pFaultMsg, DBOper* pDboper)
{
    int iRet = -1;
    string strSQL = "";
    string strPlanName = "";
    int record_count = 0;
    char strPlanID[32] = {0};
    char pcPlanName[32] = {0};

    if (NULL == pDboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG,  "StartPlanActionForFault() exit---: DB Oper Error \r\n");
        return -1;
    }

    snprintf(strPlanID, 32, "%d", iPlanID);

    /* ����plan_id����ѯԤ����������ȡԤ���ľ������� */
    strSQL.clear();
    strSQL = "select * from PlanConfig WHERE ID = ";
    strSQL += strPlanID;
    record_count = pDboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StartPlanActionForFault() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StartPlanActionForFault() ErrorMsg=%s\r\n", pDboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StartPlanActionForFault() exit---: No Record Count \r\n");
        return 0;
    }

    pDboper->GetFieldValue("PlanName", strPlanName);

    if (!strPlanName.empty())
    {
        snprintf(pcPlanName, 32, "%s", (char*)strPlanName.c_str());
    }

    iRet = StartPlanActionByPlanIDForFault(strPlanID, pcPlanName, pFaultMsg, pDboper);

    if (0 != iRet)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StartPlanActionForFault() StartPlanActionByPlanIDForFault Error:plan_id=%s, iRet=%d\r\n", strPlanID, iRet);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StartPlanActionForFault() StartPlanActionByPlanIDForFault OK:plan_id=%s, iRet=%d\r\n", strPlanID, iRet);
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : write_fault_to_db_proc
 ��������  : ��������Ϣд�����ݿ�
 �������  : fault_msg_t* pFaultMsg
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��16�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int write_fault_to_db_proc(fault_msg_t* pFaultMsg, DBOper* pDboper)
{
    int iRet = 0;
    char* local_ip = NULL;
    string strInsertSQL = "";
    char strFromType[16] = {0};
    char strLogTime[32] = {0};
    char strDeviceIndex[32] = {0};

    if (NULL == pFaultMsg || NULL == pDboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "write_fault_to_db_proc() exit---: Param Error \r\n");
        return -1;
    }

    snprintf(strFromType, 16, "%d", EV9000_LOG_FROMTYPE_CMS);
    snprintf(strDeviceIndex, 16, "%d", local_cms_index_get());
    snprintf(strLogTime, 32, "%u", pFaultMsg->uLogTime);

    /* ����SQL��� */
    strInsertSQL.clear();

    strInsertSQL = "insert into SystemLogRecord (FromType,DeviceIndex,DeviceIP,LogType,LogLevel,LogTime,LogInfo) values (";

    strInsertSQL += "'";
    strInsertSQL += strFromType;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    strInsertSQL += strDeviceIndex;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    local_ip = local_ip_get(default_eth_name_get());

    if (NULL != local_ip)
    {
        strInsertSQL += local_ip;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";

    /* ���� */
    if (pFaultMsg->strMethod[0] != '\0')
    {
        strInsertSQL += pFaultMsg->strMethod;
    }
    else
    {
        strInsertSQL += "0";
    }

    strInsertSQL += ",";

    /* ���� */
    if (pFaultMsg->strPriority[0] != '\0')
    {
        strInsertSQL += pFaultMsg->strPriority;
    }
    else
    {
        strInsertSQL += "0";
    }

    strInsertSQL += ",";

    /* ʱ�� */
    strInsertSQL += strLogTime;

    strInsertSQL += ",";

    /* ���� */
    strInsertSQL += "'";
    strInsertSQL += pFaultMsg->strDeseription;
    strInsertSQL += "'";

    strInsertSQL += ")";

    iRet = pDboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "write_fault_to_db_proc() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "write_fault_to_db_proc() ErrorMsg=%s\r\n", pDboper->GetLastDbErrorMsg());
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : fault_msg_proc
 ��������  : ������Ϣ����
 �������  : fault_msg_t* pFaultMsg
             DBOper* pDboper
             DBOper* pLogDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��16�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int fault_msg_proc(fault_msg_t* pFaultMsg, DBOper* pDboper, DBOper* pLogDboper)
{
    int iRet = 0;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    time_t utc_time;
    struct tm local_time = { 0 };
    char str_date[12] = {0};
    char str_time[12] = {0};
    char strTime[32] = {0};
    char strSN[32] = {0};
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pFaultMsg || NULL == pDboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "fault_msg_proc() exit---: Fault Msg Error \r\n");
        return -1;
    }

    /* ��ѯ�����Ƿ����������� */
    iRet = fault_msg_linkage_proc(pFaultMsg, pDboper);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "fault_msg_proc() fault_msg_linkage_proc Error:iRet=%d \r\n", iRet);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "fault_msg_proc() fault_msg_linkage_proc OK:iRet=%d \r\n", iRet);
    }


    /* ����������д�뱨����¼��*/
    iRet = write_fault_to_db_proc(pFaultMsg, pLogDboper);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "fault_msg_proc() write_alarm_to_db_proc Error:iRet=%d \r\n", iRet);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "fault_msg_proc() write_alarm_to_db_proc OK:iRet=%d \r\n", iRet);
    }

    return iRet; /* Ŀǰ������Ϣֻ���豸������Ϣ����ʱ�����͸��ͻ��� */

    if (g_AlarmMsgSendToUserFlag)
    {
        /* ���ͱ������ݸ��ͻ����û� */
        outPacket.SetRootTag("Notify");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"Alarm");

        AccNode = outPacket.CreateElement((char*)"SN");
        uFaultMsgSn++;
        snprintf(strSN, 32, "%u", uFaultMsgSn);
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, pFaultMsg->strLogicDeviceID);

        AccNode = outPacket.CreateElement((char*)"AlarmPriority");
        outPacket.SetElementValue(AccNode, pFaultMsg->strPriority);

        AccNode = outPacket.CreateElement((char*)"AlarmMethod");
        outPacket.SetElementValue(AccNode, pFaultMsg->strMethod);

        AccNode = outPacket.CreateElement((char*)"AlarmTime");
        utc_time = (time_t)pFaultMsg->uLogTime;
        localtime_r(&utc_time, &local_time);
        strftime(str_date, sizeof(str_date), "%Y-%m-%d", &local_time);
        strftime(str_time, sizeof(str_time), "%H:%M:%S", &local_time);
        snprintf(strTime, 32, "%sT%s", str_date, str_time);
        outPacket.SetElementValue(AccNode, strTime);

        AccNode = outPacket.CreateElement((char*)"AlarmDescription");
        outPacket.SetElementValue(AccNode, pFaultMsg->strDeseription);

        pGBLogicDeviceInfo = GBLogicDevice_info_find(pFaultMsg->strLogicDeviceID);

        if (NULL != pGBLogicDeviceInfo)
        {
            iRet = send_alarm_to_user_proc2(pFaultMsg->uLogicDeviceIndex, outPacket, pDboper);
        }
        else
        {
            iRet = send_alarm_to_user_proc(outPacket);
        }

        if (iRet < 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "���͹��ϸ澯��Ϣ�������û�ʧ��");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "send_alarm_to_user_proc Error.");
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "fault_msg_proc() send_alarm_to_user_proc2 Error\r\n");
        }
        else if (iRet > 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���͹��ϸ澯��Ϣ�������û��ɹ�");
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "send_alarm_to_user_proc OK.");
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "fault_msg_proc() send_alarm_to_user_proc2 OK\r\n");
        }
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : fault_msg_linkage_proc
 ��������  : ������Ϣ��������
 �������  : fault_msg_t* pFaultMsg
             DBOper* pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��9��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int fault_msg_linkage_proc(fault_msg_t* pFaultMsg, DBOper* pDboper)
{
    int iRet = 0;
    int iInAlarmDeployment = 0;
    int iFaultDayTime = 0;
    time_t now = time(NULL);
    struct tm tp = {0};

    if (NULL == pFaultMsg || NULL == pDboper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "fault_msg_linkage_proc() exit---: Alarm Msg Error \r\n");
        return -1;
    }

    /* ��������Ϣ�Ƿ��ڲ�����ʱ��� */
    if (pFaultMsg->uLogTime > 0)
    {
        now = pFaultMsg->uLogTime;

        localtime_r(&now, &tp);
        iFaultDayTime = tp.tm_hour * 3600 + tp.tm_min * 60 + tp.tm_sec;

        iInAlarmDeployment = find_device_is_in_alarm_deployment(pFaultMsg->uFaultType, iFaultDayTime);

        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "fault_msg_linkage_proc() find_device_is_in_alarm_deployment:DeviceIndex=%u, tm_hour=%d, tm_min=%d, tm_sec=%d, AlarmDayTime=%d, InAlarmDeployment=%d \r\n", pFaultMsg->uFaultType, tp.tm_hour, tp.tm_min, tp.tm_sec, iFaultDayTime, iInAlarmDeployment);

        if (iInAlarmDeployment)
        {
            /* �鿴�Ƿ��б����������� */
            iRet = FindHasPlanLinkageForFaultProc(pFaultMsg, 0, pDboper);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "fault_msg_linkage_proc() FindHasPlanLinkageForFaultProc Error:iRet=%d \r\n", iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "fault_msg_linkage_proc() FindHasPlanLinkageForFaultProc OK:iRet=%d \r\n", iRet);
            }
        }
    }

    return iRet;
}

