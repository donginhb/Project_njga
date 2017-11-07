/******************************************************************************

                  ��Ȩ���� (C), 2001-2013, ������Ѷ�������޹�˾

 ******************************************************************************
  �� �� ��   : log_proc.c
  �� �� ��   : ����
  ��    ��   : yanghaifeng
  ��������   : 2013��6��28�� ������
  ����޸�   :
  ��������   : ��־����
  �����б�   :
                              DebugTrace
                              log_init
  �޸���ʷ   :
  1.��    ��   : 2013��6��28�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ļ�

******************************************************************************/

/*----------------------------------------------*
 * ����ͷ�ļ�                                   *
 *----------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

//#include <log4cxx/logger.h>
//#include <log4cxx/propertyconfigurator.h>

#include <osipparser2/osip_port.h>

#include "common/log_proc.inc"
#include "common/gblconfig_proc.inc"
#include "common/gblfunc_proc.inc"

#include "config/telnetd.inc"

#include "user/user_info_mgn.inc"

//#include "IceServer_log.h"

//using namespace log4cxx;
//using namespace log4cxx::helpers;
//using namespace ::log4cxx::spi;
using namespace std;
/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
char log_file_time[20] = {0};
extern int g_LogQueryBufferSize; /* ��־������д�С��Ĭ��MAX_LOG_QUERY_SIZE */
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
cms_log_t* pGCMSLog = NULL;       /* ��־�ļ���¼ */
//TICEClientLog* pTICEClientLog = NULL;  /* ��¼�����ݿ���� */

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/
/* ����ģ���ӡ�ȼ� */
int g_CommonDbgLevel = LOG_ERROR;
int g_SIPStackDbgLevel = LOG_ERROR;
int g_UserDbgLevel = LOG_ERROR;
int g_DeviceDbgLevel = LOG_ERROR;
int g_RouteDbgLevel = LOG_ERROR;
int g_RecordDbgLevel = LOG_ERROR;
int g_ResourceDbgLevel = LOG_ERROR;
int g_CruiseDbgLevel = LOG_ERROR;
int g_PlanDbgLevel = LOG_ERROR;
int g_PollDbgLevel = LOG_ERROR;

int g_SystemLogLevel = EV9000_LOG_LEVEL_ERROR;   /* ϵͳ��־�ȼ� */
int g_SystemLog2DBLevel = EV9000_LOG_LEVEL_ERROR; /* ϵͳ��־��¼�����ݿ�ĵȼ� */

int g_IsLog2File = 1;            /* ��־�Ƿ��¼���ļ� */
int g_IsLog2DB = 1;              /* ��־�Ƿ��¼�����ݿ� */

int g_IsLogSIPMsg = 0;           /* �Ƿ��¼SIP Message��Ϣ */

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/
sip_msg_log2file_queue g_SIPMsgLog2FileQueue; /* SIP��Ϣ��־��¼�ļ����� */
#ifdef MULTI_THR
osip_mutex_t* g_SIPMsgLog2FileQueueLock = NULL;
#endif

trace_log2file_queue g_TraceLog2FileQueue;   /* ��־��¼�ļ����� */
#ifdef MULTI_THR
osip_mutex_t* g_TraceLog2FileQueueLock = NULL;
#endif

system_log2db_queue g_SystemLog2DBQueue;   /* ϵͳ��־��¼�����ݿ���Ϣ���� */
#ifdef MULTI_THR
osip_mutex_t* g_SystemLog2DBQueueLock = NULL;
#endif

user_log2db_queue g_UserLog2DBQueue;       /* �û���־��¼�����ݿ���Ϣ���� */
#ifdef MULTI_THR
osip_mutex_t* g_UserLog2DBQueueLock = NULL;
#endif

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

#define      SIPMSG_FILE_SIZE       20971520     /*20*1024*1024*/
#define      SIPERRMSG_FILE_SIZE    5242880      /*5*1024*1024*/
#define      RUN_LOG_FILE_SIZE      15728640     /*15*1024*1024*/
#define      DEBUG_LOG_FILE_SIZE    20971520     /*20*1024*1024*/

#if DECS("SIP��Ϣ��־��¼���ļ���Ϣ����")
/*****************************************************************************
 �� �� ��  : sip_msg_log2file_init
 ��������  : SIP��Ϣ��־��¼���ļ��ṹ��ʼ��
 �������  : sip_msg_log2file_t** sip_msg_log2file
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int sip_msg_log2file_init(sip_msg_log2file_t** sip_msg_log2file)
{
    *sip_msg_log2file = (sip_msg_log2file_t*)osip_malloc(sizeof(sip_msg_log2file_t));

    if (*sip_msg_log2file == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "sip_msg_log_init() exit---: *sip_msg_log Smalloc Error \r\n");
        return -1;
    }

    (*sip_msg_log2file)->type = -1;
    memset((*sip_msg_log2file)->msg, 0, MAX_2048CHAR_STRING_LEN + 4);
    memset((*sip_msg_log2file)->ipaddr, 0, MAX_IP_LEN);
    (*sip_msg_log2file)->port = 0;

    return 0;
}

/*****************************************************************************
 �� �� ��  : sip_msg_log2file_free
 ��������  : SIP��Ϣ��־��¼���ļ��ṹ�ͷ�
 �������  : sip_msg_log2file_t* sip_msg_log2file
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void sip_msg_log2file_free(sip_msg_log2file_t* sip_msg_log2file)
{
    if (sip_msg_log2file == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "sip_msg_log2file_free() exit---: Param Error \r\n");
        return;
    }

    sip_msg_log2file->type = -1;

    memset(sip_msg_log2file->msg, 0, MAX_2048CHAR_STRING_LEN + 4);
    memset(sip_msg_log2file->ipaddr, 0, MAX_IP_LEN);

    sip_msg_log2file->port = 0;

    return;
}

/*****************************************************************************
 �� �� ��  : sip_msg_log2file_list_init
 ��������  : SIP ��Ϣ��־д�ļ���Ϣ���г�ʼ��
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
int sip_msg_log2file_list_init()
{
    g_SIPMsgLog2FileQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_SIPMsgLog2FileQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_SIPMsgLog2FileQueueLock)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "sip_msg_log2file_list_init() exit---: SIP Message Log To File List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : sip_msg_log2file_list_free
 ��������  : SIP ��Ϣ��־д�ļ���Ϣ�����ͷ�
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
void sip_msg_log2file_list_free()
{
    int iRet = 0;
    sip_msg_log2file_t* pSIPMsgLog2File = NULL;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_SIPMsgLog2FileQueueLock);
#endif

    while (!g_SIPMsgLog2FileQueue.empty())
    {
        pSIPMsgLog2File = (sip_msg_log2file_t*) g_SIPMsgLog2FileQueue.front();
        g_SIPMsgLog2FileQueue.pop_front();

        if (NULL != pSIPMsgLog2File)
        {
            sip_msg_log2file_free(pSIPMsgLog2File);
            osip_free(pSIPMsgLog2File);
            pSIPMsgLog2File = NULL;
        }
    }

    g_SIPMsgLog2FileQueue.clear();

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_SIPMsgLog2FileQueueLock);
#endif

#ifdef MULTI_THR

    if (NULL != g_SIPMsgLog2FileQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_SIPMsgLog2FileQueueLock);
        g_SIPMsgLog2FileQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 �� �� ��  : sip_msg_log2file_add
 ��������  : ���SIP��Ϣ��־��¼���ļ���������
 �������  : int type
                            0:������ȷ��
                            1:������ȷ��
                            2:���ʹ����
                            3:���ս��������
                            4:������Ϣ�����
                            5:���մ�����������
                            char* msg
                            char* ipaddr
                            int port
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int sip_msg_log2file_add(int type, char* msg, char* ipaddr, int port)
{
    sip_msg_log2file_t* pSIPMsgLog2File = NULL;
    sip_msg_log2file_t* pTmpSIPMsgLog2File = NULL;
    int iRet = 0;

    if (!g_IsLogSIPMsg)
    {
        return 0;
    }

    if (msg == NULL || ipaddr == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "sip_msg_log2file_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = sip_msg_log2file_init(&pSIPMsgLog2File);

    if (iRet != 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "sip_msg_log2file_add() exit---: Message Init Error \r\n");
        return -1;
    }

    pSIPMsgLog2File->type = type;

    if (NULL != msg && '\0' != msg[0])
    {
        osip_strncpy(pSIPMsgLog2File->msg, msg, MAX_2048CHAR_STRING_LEN);
    }

    if (NULL != ipaddr && '\0' != ipaddr[0])
    {
        osip_strncpy(pSIPMsgLog2File->ipaddr, ipaddr, MAX_IP_LEN);
    }

    pSIPMsgLog2File->port = port;


#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_SIPMsgLog2FileQueueLock);
#endif

    /* �ж�һ����־���д�С������������ֵ�������ǰ��Ķ����� */
    if (g_LogQueryBufferSize > 0)
    {
        if (g_SIPMsgLog2FileQueue.size() >= g_LogQueryBufferSize)
        {
            pTmpSIPMsgLog2File = (sip_msg_log2file_t*) g_SIPMsgLog2FileQueue.front();
            g_SIPMsgLog2FileQueue.pop_front();

            if (NULL != pTmpSIPMsgLog2File)
            {
                sip_msg_log2file_free(pTmpSIPMsgLog2File);
                osip_free(pTmpSIPMsgLog2File);
                pTmpSIPMsgLog2File = NULL;
            }
        }
    }

    g_SIPMsgLog2FileQueue.push_back(pSIPMsgLog2File);

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_SIPMsgLog2FileQueueLock);
#endif

    return 0;
}


/*****************************************************************************
 �� �� ��  : scan_sip_msg_log2file_list
 ��������  : ɨ��SIP��Ϣ��־��¼���ļ���Ϣ����
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��23�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_sip_msg_log2file_list()
{
    int iRet = 0;
    sip_msg_log2file_t* pSIPMsgLog2File = NULL;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_SIPMsgLog2FileQueueLock);
#endif

    while (!g_SIPMsgLog2FileQueue.empty())
    {
        pSIPMsgLog2File = (sip_msg_log2file_t*) g_SIPMsgLog2FileQueue.front();
        g_SIPMsgLog2FileQueue.pop_front();

        if (NULL != pSIPMsgLog2File)
        {
            /*
                0:������ȷ��
                1:������ȷ��
                2:���ʹ����
                3:���ս��������
                4:������Ϣ�����
                5:���մ�����������
               */

            if (pSIPMsgLog2File->type == 0 || pSIPMsgLog2File->type == 1)
            {
                LogSipMsgToFile(pSIPMsgLog2File->type, pSIPMsgLog2File->msg, pSIPMsgLog2File->ipaddr, pSIPMsgLog2File->port);
            }
            else
            {
                LogSipErrMsgToFile(pSIPMsgLog2File->type, pSIPMsgLog2File->msg, pSIPMsgLog2File->ipaddr, pSIPMsgLog2File->port);
            }

            sip_msg_log2file_free(pSIPMsgLog2File);
            osip_free(pSIPMsgLog2File);
            pSIPMsgLog2File = NULL;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_SIPMsgLog2FileQueueLock);
#endif

    return;
}
#endif

#if DECS("��־��¼���ļ���Ϣ����")
/*****************************************************************************
 �� �� ��  : trace_log2file_init
 ��������  : ��־��¼���ļ��ṹ��ʼ��
 �������  : trace_log2file_t** trace_log2file
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int trace_log2file_init(trace_log2file_t** trace_log2file)
{
    *trace_log2file = (trace_log2file_t*)osip_malloc(sizeof(trace_log2file_t));

    if (*trace_log2file == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "trace_log2file_init() exit---: *trace_log2file Smalloc Error \r\n");
        return -1;
    }

    (*trace_log2file)->type = -1;
    memset((*trace_log2file)->msg, 0, MAX_2048CHAR_STRING_LEN + 4);

    return 0;
}

/*****************************************************************************
 �� �� ��  : trace_log2file_free
 ��������  : ��־��¼���ļ��ṹ�ͷ�
 �������  : trace_log2file_t* trace_log2file
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void trace_log2file_free(trace_log2file_t* trace_log2file)
{
    if (trace_log2file == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "trace_log2file_free() exit---: Param Error \r\n");
        return;
    }

    trace_log2file->type = -1;
    memset(trace_log2file->msg, 0, MAX_2048CHAR_STRING_LEN + 4);

    return;
}

/*****************************************************************************
 �� �� ��  : trace_log2file_list_init
 ��������  : ��־д�ļ���Ϣ���г�ʼ��
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
int trace_log2file_list_init()
{
    g_TraceLog2FileQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_TraceLog2FileQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_TraceLog2FileQueueLock)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "trace_log2file_list_init() exit---: Trace Message Log To File List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : trace_log2file_list_free
 ��������  : ��־д�ļ���Ϣ�����ͷ�
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
void trace_log2file_list_free()
{
    int iRet = 0;
    trace_log2file_t* pTraceLog2File = NULL;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_TraceLog2FileQueueLock);
#endif

    while (!g_TraceLog2FileQueue.empty())
    {
        pTraceLog2File = (trace_log2file_t*) g_TraceLog2FileQueue.front();
        g_TraceLog2FileQueue.pop_front();

        if (NULL != pTraceLog2File)
        {
            trace_log2file_free(pTraceLog2File);
            osip_free(pTraceLog2File);
            pTraceLog2File = NULL;
        }
    }

    g_TraceLog2FileQueue.clear();

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_TraceLog2FileQueueLock);
#endif

#ifdef MULTI_THR

    if (NULL != g_TraceLog2FileQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_TraceLog2FileQueueLock);
        g_TraceLog2FileQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 �� �� ��  : trace_log2file_add
 ��������  : ��� ��־��¼���ļ���������
 �������  : int type
                            0:Debug ������Ϣ
                            1:Run��Ϣ
                            char* msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int trace_log2file_add(int type, char* msg)
{
    trace_log2file_t* pTraceLog2File = NULL;
    trace_log2file_t* pTmpTraceLog2File = NULL;
    int iRet = 0;

    if (!g_IsLog2File)
    {
        return 0;
    }

    if (msg == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "trace_log2file_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = trace_log2file_init(&pTraceLog2File);

    if (iRet != 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "trace_log2file_add() exit---: Message Init Error \r\n");
        return -1;
    }

    pTraceLog2File->type = type;

    if (NULL != msg && '\0' != msg[0])
    {
        osip_strncpy(pTraceLog2File->msg, msg, MAX_2048CHAR_STRING_LEN);
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_TraceLog2FileQueueLock);
#endif

    /* �ж�һ����־���д�С������������ֵ�������ǰ��Ķ����� */
    if (g_LogQueryBufferSize > 0)
    {
        if (g_TraceLog2FileQueue.size() >= g_LogQueryBufferSize)
        {
            pTmpTraceLog2File = (trace_log2file_t*) g_TraceLog2FileQueue.front();
            g_TraceLog2FileQueue.pop_front();

            if (NULL != pTmpTraceLog2File)
            {
                trace_log2file_free(pTmpTraceLog2File);
                osip_free(pTmpTraceLog2File);
                pTmpTraceLog2File = NULL;
            }
        }
    }

    g_TraceLog2FileQueue.push_back(pTraceLog2File);

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_TraceLog2FileQueueLock);
#endif

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_trace_log2file_list
 ��������  : ɨ�� ��־��¼���ļ���Ϣ����
 �������  :
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��23�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_trace_log2file_list()
{
    int iRet = 0;
    trace_log2file_t* pTraceLog2File = NULL;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_TraceLog2FileQueueLock);
#endif

    while (!g_TraceLog2FileQueue.empty())
    {
        pTraceLog2File = (trace_log2file_t*) g_TraceLog2FileQueue.front();
        g_TraceLog2FileQueue.pop_front();

        if (NULL != pTraceLog2File)
        {
            /*
                0:Debug ������Ϣ
                1:Run��Ϣ
               */

            if (pTraceLog2File->type == 0)
            {
                LogDebugTraceToFile(pTraceLog2File->msg);
            }
            else if (pTraceLog2File->type == 1)
            {
                LogRunTraceToFile(pTraceLog2File->msg);
            }

            trace_log2file_free(pTraceLog2File);
            osip_free(pTraceLog2File);
            pTraceLog2File = NULL;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_TraceLog2FileQueueLock);
#endif

    return;
}
#endif

#if DECS("ϵͳ��־��¼�����ݿ���Ϣ����")
/*****************************************************************************
 �� �� ��  : system_log2db_init
 ��������  : ϵͳ��־��¼�����ݿ�ṹ��ʼ��
 �������  : system_log2db_t** system_log2db
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int system_log2db_init(system_log2db_t** system_log2db)
{
    *system_log2db = (system_log2db_t*)osip_malloc(sizeof(system_log2db_t));

    if (*system_log2db == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "system_log2db_init() exit---: *system_log2db Smalloc Error \r\n");
        return -1;
    }

    (*system_log2db)->iType = -1;
    (*system_log2db)->iLevel = -1;
    (*system_log2db)->iTime = -1;
    memset((*system_log2db)->msg, 0, MAX_2048CHAR_STRING_LEN + 4);

    return 0;
}

/*****************************************************************************
 �� �� ��  : system_log2db_free
 ��������  : ��־��¼���ļ��ṹ�ͷ�
 �������  : system_log2db_t* system_log2db
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void system_log2db_free(system_log2db_t* system_log2db)
{
    if (system_log2db == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "system_log2db_free() exit---: Param Error \r\n");
        return;
    }

    system_log2db->iType = -1;
    system_log2db->iLevel = -1;
    system_log2db->iTime = -1;
    memset(system_log2db->msg, 0, MAX_2048CHAR_STRING_LEN + 4);

    return;
}

/*****************************************************************************
 �� �� ��  : system_log2db_list_init
 ��������  : ��־д�ļ���Ϣ���г�ʼ��
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
int system_log2db_list_init()
{
    g_SystemLog2DBQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_SystemLog2DBQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_SystemLog2DBQueueLock)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "system_log2db_list_init() exit---: System Message Log To File List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : system_log2db_list_free
 ��������  : ��־д�ļ���Ϣ�����ͷ�
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
void system_log2db_list_free()
{
    int iRet = 0;
    system_log2db_t* pSystemLog2DB = NULL;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_SystemLog2DBQueueLock);
#endif

    while (!g_SystemLog2DBQueue.empty())
    {
        pSystemLog2DB = (system_log2db_t*) g_SystemLog2DBQueue.front();
        g_SystemLog2DBQueue.pop_front();

        if (NULL != pSystemLog2DB)
        {
            system_log2db_free(pSystemLog2DB);
            osip_free(pSystemLog2DB);
            pSystemLog2DB = NULL;
        }
    }

    g_SystemLog2DBQueue.clear();

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_SystemLog2DBQueueLock);
#endif

#ifdef MULTI_THR

    if (NULL != g_SystemLog2DBQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_SystemLog2DBQueueLock);
        g_SystemLog2DBQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 �� �� ��  : system_log2db_list_clean
 ��������  : ��־д�ļ���Ϣ�������
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
void system_log2db_list_clean()
{
    int iRet = 0;
    system_log2db_t* pSystemLog2DB = NULL;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_SystemLog2DBQueueLock);
#endif

    while (!g_SystemLog2DBQueue.empty())
    {
        pSystemLog2DB = (system_log2db_t*) g_SystemLog2DBQueue.front();
        g_SystemLog2DBQueue.pop_front();

        if (NULL != pSystemLog2DB)
        {
            system_log2db_free(pSystemLog2DB);
            osip_free(pSystemLog2DB);
            pSystemLog2DB = NULL;
        }
    }

    g_SystemLog2DBQueue.clear();

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_SystemLog2DBQueueLock);
#endif

    return;
}

/*****************************************************************************
 �� �� ��  : system_log2db_add
 ��������  : ���ϵͳ��־��¼�����ݿ⵽������
 �������  : int type
                            int level
                            int iTime
                            char* msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int system_log2db_add(int type, int level, int iTime, char* msg)
{
    system_log2db_t* pSystemLog2DB = NULL;
    system_log2db_t* pTmpSystemLog2DB = NULL;
    int iRet = 0;

    if (!g_IsLog2DB)
    {
        return 0;
    }

    if (msg == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "system_log2db_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = system_log2db_init(&pSystemLog2DB);

    if (iRet != 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "system_log2db_add() exit---: Message Init Error \r\n");
        return -1;
    }

    pSystemLog2DB->iType = type;
    pSystemLog2DB->iLevel = level;
    pSystemLog2DB->iTime = iTime;

    if (NULL != msg && '\0' != msg[0])
    {
        osip_strncpy(pSystemLog2DB->msg, msg, MAX_2048CHAR_STRING_LEN);
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_SystemLog2DBQueueLock);
#endif

    /* �ж�һ����־���д�С������������ֵ�������ǰ��Ķ����� */
    if (g_LogQueryBufferSize > 0)
    {
        if (g_SystemLog2DBQueue.size() >= g_LogQueryBufferSize)
        {
            pTmpSystemLog2DB = (system_log2db_t*) g_SystemLog2DBQueue.front();
            g_SystemLog2DBQueue.pop_front();

            if (NULL != pTmpSystemLog2DB)
            {
                system_log2db_free(pTmpSystemLog2DB);
                osip_free(pTmpSystemLog2DB);
                pTmpSystemLog2DB = NULL;

            }
        }
    }

    g_SystemLog2DBQueue.push_back(pSystemLog2DB);

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_SystemLog2DBQueueLock);
#endif

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_system_log2db_list
 ��������  : ɨ��ϵͳ��־��¼�����ݿ���Ϣ����
 �������  : thread_proc_t* run
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��23�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_system_log2db_list(thread_proc_t* run)
{
    int iRet = 0;
    system_log2db_t* pSystemLog2DB = NULL;
    static int connect_interval = 0;

    if (NULL == run)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_system_log2db_list() Log2DB DB Oper Error\r\n");
        return;
    }

    if (NULL == run->pLogDbOper)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_system_log2db_list() Log2DB DB Oper Error\r\n");
        return;
    }

    if (!run->iLogDBOperConnectStatus)
    {
        connect_interval++;

        if (connect_interval >= 60)
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
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_SystemLog2DBQueueLock);
#endif

    while (!g_SystemLog2DBQueue.empty())
    {
        pSystemLog2DB = (system_log2db_t*) g_SystemLog2DBQueue.front();
        g_SystemLog2DBQueue.pop_front();

        if (NULL != pSystemLog2DB)
        {
            if (run->iLogDBOperConnectStatus)
            {
                SystemLogToDB(run->pLogDbOper, pSystemLog2DB);
            }

            system_log2db_free(pSystemLog2DB);
            osip_free(pSystemLog2DB);
            pSystemLog2DB = NULL;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_SystemLog2DBQueueLock);
#endif

    return;
}
#endif

#if DECS("�û���־��¼�����ݿ���Ϣ����")
/*****************************************************************************
 �� �� ��  : user_log2db_init
 ��������  : �û���־��¼�����ݿ�ṹ��ʼ��
 �������  : user_log2db_t** user_log2db
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int user_log2db_init(user_log2db_t** user_log2db)
{
    *user_log2db = (user_log2db_t*)osip_malloc(sizeof(user_log2db_t));

    if (*user_log2db == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "user_log2db_init() exit---: *user_log2db Smalloc Error \r\n");
        return -1;
    }

    memset((*user_log2db)->pcUserID, 0, MAX_ID_LEN + 4);
    memset((*user_log2db)->pcUserIP, 0, MAX_IP_LEN);
    (*user_log2db)->iUserIndex = 0;
    (*user_log2db)->iType = -1;
    (*user_log2db)->iLevel = -1;
    (*user_log2db)->iTime = -1;
    memset((*user_log2db)->msg, 0, MAX_2048CHAR_STRING_LEN + 4);

    return 0;
}

/*****************************************************************************
 �� �� ��  : user_log2db_free
 ��������  : ��־��¼���ļ��ṹ�ͷ�
 �������  : user_log2db_t* user_log2db
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void user_log2db_free(user_log2db_t* user_log2db)
{
    if (user_log2db == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "user_log2db_free() exit---: Param Error \r\n");
        return;
    }

    memset(user_log2db->pcUserID, 0, MAX_ID_LEN + 4);
    memset(user_log2db->pcUserIP, 0, MAX_IP_LEN);
    user_log2db->iUserIndex = 0;
    user_log2db->iType = -1;
    user_log2db->iLevel = -1;
    user_log2db->iTime = -1;
    memset(user_log2db->msg, 0, MAX_2048CHAR_STRING_LEN + 4);

    return;
}

/*****************************************************************************
 �� �� ��  : user_log2db_list_init
 ��������  : ��־д�ļ���Ϣ���г�ʼ��
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
int user_log2db_list_init()
{
    g_UserLog2DBQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_UserLog2DBQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_UserLog2DBQueueLock)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "user_log2db_list_init() exit---: User Message Log To File List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : user_log2db_list_free
 ��������  : ��־д�ļ���Ϣ�����ͷ�
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
void user_log2db_list_free()
{
    int iRet = 0;
    user_log2db_t* pUserLog2DB = NULL;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_UserLog2DBQueueLock);
#endif

    while (!g_UserLog2DBQueue.empty())
    {
        pUserLog2DB = (user_log2db_t*) g_UserLog2DBQueue.front();
        g_UserLog2DBQueue.pop_front();

        if (NULL != pUserLog2DB)
        {
            user_log2db_free(pUserLog2DB);
            osip_free(pUserLog2DB);
            pUserLog2DB = NULL;
        }
    }

    g_UserLog2DBQueue.clear();

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_UserLog2DBQueueLock);
#endif

#ifdef MULTI_THR

    if (NULL != g_UserLog2DBQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_UserLog2DBQueueLock);
        g_UserLog2DBQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 �� �� ��  : user_log2db_list_clean
 ��������  : ��־д�ļ���Ϣ�������
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
void user_log2db_list_clean()
{
    int iRet = 0;
    user_log2db_t* pUserLog2DB = NULL;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_UserLog2DBQueueLock);
#endif

    while (!g_UserLog2DBQueue.empty())
    {
        pUserLog2DB = (user_log2db_t*) g_UserLog2DBQueue.front();
        g_UserLog2DBQueue.pop_front();

        if (NULL != pUserLog2DB)
        {
            user_log2db_free(pUserLog2DB);
            osip_free(pUserLog2DB);
            pUserLog2DB = NULL;
        }
    }

    g_UserLog2DBQueue.clear();

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_UserLog2DBQueueLock);
#endif

    return;
}

/*****************************************************************************
 �� �� ��  : user_log2db_add
 ��������  : ����û���־��¼�����ݿ⵽������
 �������  : int type
             int level
             user_info_t* pUserInfo
             int iTime
             char* msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int user_log2db_add(int type, int level, user_info_t* pUserInfo, int iTime, char* msg)
{
    user_log2db_t* pUserLog2DB = NULL;
    user_log2db_t* pTmpUserLog2DB = NULL;
    int iRet = 0;

    if (msg == NULL || NULL == pUserInfo)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "user_log2db_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = user_log2db_init(&pUserLog2DB);

    if (iRet != 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "user_log2db_add() exit---: Message Init Error \r\n");
        return -1;
    }

    osip_strncpy(pUserLog2DB->pcUserID, pUserInfo->user_id, MAX_ID_LEN);
    osip_strncpy(pUserLog2DB->pcUserIP, pUserInfo->login_ip, MAX_IP_LEN);
    pUserLog2DB->iUserIndex = pUserInfo->user_index;

    pUserLog2DB->iType = type;
    pUserLog2DB->iLevel = level;
    pUserLog2DB->iTime = iTime;

    if (NULL != msg && '\0' != msg[0])
    {
        osip_strncpy(pUserLog2DB->msg, msg, MAX_2048CHAR_STRING_LEN);
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_UserLog2DBQueueLock);
#endif

    /* �ж�һ����־���д�С������������ֵ�������ǰ��Ķ����� */
    if (g_LogQueryBufferSize > 0)
    {
        if (g_UserLog2DBQueue.size() >= g_LogQueryBufferSize)
        {
            pTmpUserLog2DB = (user_log2db_t*) g_UserLog2DBQueue.front();
            g_UserLog2DBQueue.pop_front();

            if (NULL != pTmpUserLog2DB)
            {
                user_log2db_free(pTmpUserLog2DB);
                osip_free(pTmpUserLog2DB);
                pTmpUserLog2DB = NULL;
            }
        }
    }

    g_UserLog2DBQueue.push_back(pUserLog2DB);

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_UserLog2DBQueueLock);
#endif

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_user_log2db_list
 ��������  : ɨ���û���־��¼�����ݿ���Ϣ����
 �������  : thread_proc_t* run
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��23�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_user_log2db_list(thread_proc_t* run)
{
    int iRet = 0;
    user_log2db_t* pUserLog2DB = NULL;
    static int connect_interval = 0;

    if (NULL == run)
    {
        return;
    }

    if (NULL == run->pLogDbOper)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_system_log2db_list() Log2DB DB Oper Error\r\n");
        return;
    }

    if (!run->iLogDBOperConnectStatus)
    {
        connect_interval++;

        if (connect_interval >= 60)
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
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_UserLog2DBQueueLock);
#endif

    while (!g_UserLog2DBQueue.empty())
    {
        pUserLog2DB = (user_log2db_t*) g_UserLog2DBQueue.front();
        g_UserLog2DBQueue.pop_front();

        if (NULL != pUserLog2DB)
        {
            if (run->iLogDBOperConnectStatus)
            {
                UserLogToDB(run->pLogDbOper, pUserLog2DB);
            }

            user_log2db_free(pUserLog2DB);
            osip_free(pUserLog2DB);
            pUserLog2DB = NULL;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_UserLog2DBQueueLock);
#endif

    return;
}
#endif

#if DECS("��־��¼���ļ�")
/*****************************************************************************
 �� �� ��  : GetLogFilename
 ��������  : ��ȡlog�ļ�����
 �������  : int type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
char* GetLogFilename(int type)
{
    static char fn[256] = {0};

    switch (type)
    {
        case 0:
            snprintf(fn, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_SIP_MSG, log_file_time);
            break;

        case 1:
            snprintf(fn, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_SIP_ERRORMSG, log_file_time);
            break;

        case 2:
            snprintf(fn, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_DEBUG, log_file_time);
            break;

        case 3:
            snprintf(fn, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_RUN, log_file_time);
            break;

        default:
            return NULL;
    }

    return fn;
}

/*****************************************************************************
 �� �� ��  : Log2FileInit
 ��������  : �ļ���־��ʼ��
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int Log2FileInit()
{
    int i = 0;
    DIR* dir = NULL;
    char filename[256] = {0};

    i = cms_log_init(&pGCMSLog);

    if (i != 0)
    {
        printf(" Log2FileInit() cms_log_init Error \r\n");
        return -1;
    }

    /* ���Ŀ¼�Ƿ���� */
    dir = opendir("/data");

    if (NULL == dir)
    {
        i = mkdir("/data", 0775);

        if (i != 0)
        {
            printf(" Log2FileInit() mkdir /data Error \r\n");
            goto error1;
        }
    }
    else
    {
        closedir(dir);
        dir = NULL;
    }

    dir = opendir(LOGFILE_DIR);

    if (NULL == dir)
    {
        i = mkdir(LOGFILE_DIR, 0775);

        if (i != 0)
        {
            printf(" Log2FileInit() mkdir /data/log Error \r\n");
        }
    }
    else
    {
        closedir(dir);
        dir = NULL;
    }

    /* SIP��Ϣ��־��¼��ʼ�� */
    i = sip_msglog_init(&pGCMSLog->sip_msglog);

    if (i != 0)
    {
        printf(" Log2FileInit() sip_msglog_init Error \r\n");
        goto error1;
    }

    if (g_IsLogSIPMsg)
    {
        memset(filename, 0, 256);
        snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_SIP_MSG, log_file_time);

        if (NULL == (pGCMSLog->sip_msglog->logfile = fopen(filename, "w+")))
        {
            printf(" Log2FileInit() Can not open \" sip message log file \": %s !\n", filename);
        }
    }

    /* SIP������Ϣ��־��¼��ʼ�� */
    i = sip_errorlog_init(&pGCMSLog->sip_errorlog);

    if (i != 0)
    {
        printf(" Log2FileInit() sip_errorlog_init Error \r\n");
        goto error1;
    }

    if (g_IsLogSIPMsg)
    {
        memset(filename, 0, 256);
        snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_SIP_ERRORMSG, log_file_time);

        if (NULL == (pGCMSLog->sip_errorlog->logfile = fopen(filename, "w+")))
        {
            printf(" Log2FileInit() Can not open \" sip error log file \": %s !\n", filename);
        }
    }

    /* ϵͳ�������Ӽ�¼��ʼ�� */
    i = debug_log_init(&pGCMSLog->debug_log);

    if (i != 0)
    {
        printf(" Log2FileInit() debug_log_init Error \r\n");
        goto error1;
    }

    if (g_IsLog2File)
    {
        memset(filename, 0, 256);
        snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_DEBUG, log_file_time);

        if (NULL == (pGCMSLog->debug_log->logfile = fopen(filename, "w+")))
        {
            printf(" Log2FileInit() Can not open \" debug log file \": %s !\n", filename);
        }
    }

    /* ϵͳ�������Ӽ�¼��ʼ�� */
    i = run_log_init(&pGCMSLog->run_log);

    if (i != 0)
    {
        printf(" Log2FileInit() run_log_init Error \r\n");
        goto error1;
    }

    if (g_IsLog2File)
    {
        memset(filename, 0, 256);
        snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_RUN, log_file_time);

        if (NULL == (pGCMSLog->run_log->logfile = fopen(filename, "w+")))
        {
            printf(" Log2FileInit() Can not open \" run log file \": %s !\n", filename);
        }
    }

    return 0;
error1:
    cs_log_free(pGCMSLog);
    osip_free(pGCMSLog);
    pGCMSLog = NULL;
    return -1;
}

/*****************************************************************************
 �� �� ��  : Log2FileFree
 ��������  : �ļ���־�ͷ�
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void Log2FileFree()
{
    if (pGCMSLog == NULL)
    {
        return;
    }

    cs_log_free(pGCMSLog);
    osip_free(pGCMSLog);
    pGCMSLog = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : cms_log_init
 ��������  : cms�ļ���־�ṹ��ʼ��
 �������  : cms_log_t** log
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int cms_log_init(cms_log_t** log)
{
    *log = (cms_log_t*)osip_malloc(sizeof(cms_log_t));

    if (*log == NULL)
    {
        return -1;
    }

    (*log)->sip_msglog = NULL;
    (*log)->sip_errorlog = NULL;
    (*log)->debug_log = NULL;
    (*log)->run_log = NULL;
    return 0;
}

/*****************************************************************************
 �� �� ��  : cs_log_free
 ��������  : cms�ļ���־�ṹ�ͷ�
 �������  : cms_log_t* log
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void cs_log_free(cms_log_t* log)
{
    if (log == NULL)
    {
        return;
    }

    if (NULL != log->sip_msglog)
    {
        sip_msglog_free(log->sip_msglog);
        osip_free(log->sip_msglog);
        log->sip_msglog = NULL;
    }

    if (NULL != log->sip_errorlog)
    {
        sip_errorlog_free(log->sip_errorlog);
        osip_free(log->sip_errorlog);
        log->sip_errorlog = NULL;
    }

    if (NULL != log->debug_log)
    {
        debug_log_free(log->debug_log);
        osip_free(log->debug_log);
        log->debug_log = NULL;
    }

    if (NULL != log->run_log)
    {
        debug_log_free(log->run_log);
        osip_free(log->run_log);
        log->run_log = NULL;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : sip_msglog_init
 ��������  : SIP��Ϣ��־��ʼ��
 �������  : sip_msglog_t** log
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int sip_msglog_init(sip_msglog_t** log)
{
    *log = (sip_msglog_t*)osip_malloc(sizeof(sip_msglog_t));

    if (*log == NULL)
    {
        return -1;
    }

    (*log)->rcvcnt = 0;
    (*log)->sndcnt = 0;
    (*log)->logfile = NULL;

    return 0;
}

/*****************************************************************************
 �� �� ��  : sip_msglog_free
 ��������  : SIP��Ϣ��־�ͷ�
 �������  : sip_msglog_t* log
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void sip_msglog_free(sip_msglog_t* log)
{
    if (log == NULL)
    {
        return;
    }

    log->rcvcnt = 0;
    log->sndcnt = 0;

    if (log->logfile != NULL)
    {
        fclose(log->logfile);
        log->logfile = NULL;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : sip_errorlog_init
 ��������  : SIP������Ϣ��־��ʼ��
 �������  : sip_errorlog_t** log
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int  sip_errorlog_init(sip_errorlog_t** log)
{
    *log = (sip_errorlog_t*)osip_malloc(sizeof(sip_errorlog_t));

    if (*log == NULL)
    {
        return -1;
    }

    (*log)->cnt = 0;
    (*log)->logfile = NULL;

    return 0;
}

/*****************************************************************************
 �� �� ��  : sip_errorlog_free
 ��������  : SIP������Ϣ��־�ͷ�
 �������  : sip_errorlog_t* log
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void sip_errorlog_free(sip_errorlog_t* log)
{
    if (log == NULL)
    {
        return;
    }

    log->cnt = 0;

    if (log->logfile != NULL)
    {
        fclose(log->logfile);
        log->logfile = NULL;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : debug_log_init
 ��������  : ϵͳ������־��ʼ��
 �������  : debug_log_t** log
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int debug_log_init(debug_log_t** log)
{
    *log = (debug_log_t*)osip_malloc(sizeof(debug_log_t));

    if (*log == NULL)
    {
        return -1;
    }

    (*log)->logfile = NULL;

    return 0;
}

/*****************************************************************************
 �� �� ��  : debug_log_free
 ��������  : ϵͳ������־�ͷ�
 �������  : debug_log_t* log
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void debug_log_free(debug_log_t* log)
{
    if (log == NULL)
    {
        return;
    }

    if (log->logfile != NULL)
    {
        fclose(log->logfile);
        log->logfile = NULL;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : run_log_init
 ��������  : ϵͳ������־��ʼ��
 �������  : run_log_init** log
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int run_log_init(run_log_t** log)
{
    *log = (run_log_t*)osip_malloc(sizeof(run_log_t));

    if (*log == NULL)
    {
        return -1;
    }

    (*log)->logfile = NULL;

    return 0;
}

/*****************************************************************************
 �� �� ��  : run_log_free
 ��������  : ϵͳ������־�ͷ�
 �������  : run_log_t* log
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void run_log_free(run_log_t* log)
{
    if (log == NULL)
    {
        return;
    }

    if (log->logfile != NULL)
    {
        fclose(log->logfile);
        log->logfile = NULL;
    }

    return;
}

/*****************************************************************************
 �� �� ��  : LogSipMsgToFile
 ��������  : SIP��Ϣ��¼���ļ�
 �������  : int type
                            0:����
                            1:����
                            char* msg
                            char* ipaddr
                            int port
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void LogSipMsgToFile(int type, char* msg, char* ipaddr, int port)
{
    int i = 0;
    struct stat statbuf;
    char filename[256] = {0};
    static int creat_interval = 0;
    time_t now = time(NULL);
    struct tm tp = {0};
    char strtm[20] = {0};
    DIR* dir = NULL;

    if (msg == NULL || ipaddr == NULL)
    {
        return;
    }

    if (pGCMSLog == NULL
        || pGCMSLog->sip_msglog == NULL)
    {
        return;
    }

    if (g_IsLogSIPMsg)
    {
        memset(filename, 0, 256);
        snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_SIP_MSG, log_file_time);

        /* �ж��Ƿ���Ҫ�����ļ� */
        if (NULL == pGCMSLog->sip_msglog->logfile)
        {
            creat_interval++;

            if (creat_interval >= 60)
            {
                /* ���Ŀ¼�Ƿ���� */
                dir = opendir("/data");

                if (NULL == dir)
                {
                    i = mkdir("/data", 0775);
                }
                else
                {
                    closedir(dir);
                    dir = NULL;
                }

                dir = opendir(LOGFILE_DIR);

                if (NULL == dir)
                {
                    i = mkdir(LOGFILE_DIR, 0775);
                }
                else
                {
                    closedir(dir);
                    dir = NULL;
                }

                /* �����ļ� */
                pGCMSLog->sip_msglog->logfile = fopen(filename, "w+");

                creat_interval = 0;
            }
        }
    }

    if (pGCMSLog->sip_msglog->logfile == NULL)
    {
        return;
    }

    if (g_IsLogSIPMsg)
    {
        if (stat(filename, &statbuf) == -1)
        {
            if (NULL != pGCMSLog->sip_msglog->logfile)
            {
                fclose(pGCMSLog->sip_msglog->logfile);
                pGCMSLog->sip_msglog->logfile = NULL;
            }

#if 0
            localtime_r(&now, &tp);
            memset(log_file_time, 0, 20);
            strftime(log_file_time, 20, "%Y_%m_%d_%H", &tp);

            memset(filename, 0, 256);
            snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_SIP_MSG, log_file_time);
#endif

            if (NULL == (pGCMSLog->sip_msglog->logfile = fopen(filename, "w+")))
            {
                return;
            }
        }

        if (statbuf.st_size >= SIPMSG_FILE_SIZE)
        {
            if (NULL != pGCMSLog->sip_msglog->logfile)
            {
                fclose(pGCMSLog->sip_msglog->logfile);
                pGCMSLog->sip_msglog->logfile = NULL;
            }

            if (NULL == (pGCMSLog->sip_msglog->logfile = fopen(filename, "w+")))
            {
                return;
            }
        }
    }

    localtime_r(&now, &tp);
    strftime(strtm, 20, "%Y/%m/%d %H:%M:%S", &tp);

    if (type == 0)
    {
        fprintf(pGCMSLog->sip_msglog->logfile, "\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
        fprintf(pGCMSLog->sip_msglog->logfile, "No.%i\n", ++pGCMSLog->sip_msglog->sndcnt);
        fprintf(pGCMSLog->sip_msglog->logfile, "Time: %s\n", strtm);
        fprintf(pGCMSLog->sip_msglog->logfile, "Sent to: IP=%s port=%d\n", ipaddr, port);
    }
    else
    {
        fprintf(pGCMSLog->sip_msglog->logfile, "\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
        fprintf(pGCMSLog->sip_msglog->logfile, "No.%i\n", ++pGCMSLog->sip_msglog->rcvcnt);
        fprintf(pGCMSLog->sip_msglog->logfile, "Time: %s\n", strtm);
        fprintf(pGCMSLog->sip_msglog->logfile, "Received from: IP=%s port=%d\n", ipaddr, port);
    }

    fprintf(pGCMSLog->sip_msglog->logfile, "\n%s\n", msg);
    fflush(pGCMSLog->sip_msglog->logfile);

    return;
}

/*****************************************************************************
 �� �� ��  : LogSipErrMsgToFile
 ��������  : SIP������Ϣ��¼���ļ�
 �������  : int type
                            1:���ʹ����
                            2:���ս��������
                            3:������Ϣ�����
                            4:���մ�����������
                            char* msg
                            char* ipaddr
                            int port
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void LogSipErrMsgToFile(int type, char* msg, char* ipaddr, int port)
{
    int i = 0;
    struct stat statbuf;
    char filename[256] = {0};
    static int creat_interval = 0;
    time_t now = time(NULL);
    struct tm tp = {0};
    char strtm[20] = {0};
    DIR* dir = NULL;

    if (msg == NULL)
    {
        return;
    }

    if (pGCMSLog == NULL
        || pGCMSLog->sip_errorlog == NULL)
    {
        return;
    }

    if (g_IsLogSIPMsg)
    {
        memset(filename, 0, 256);
        snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_SIP_ERRORMSG, log_file_time);

        /* �ж��Ƿ���Ҫ�����ļ� */
        if (NULL == pGCMSLog->sip_errorlog->logfile)
        {
            creat_interval++;

            if (creat_interval >= 60)
            {
                /* ���Ŀ¼�Ƿ���� */
                dir = opendir("/data");

                if (NULL == dir)
                {
                    i = mkdir("/data", 0775);
                }
                else
                {
                    closedir(dir);
                    dir = NULL;
                }

                dir = opendir(LOGFILE_DIR);

                if (NULL == dir)
                {
                    i = mkdir(LOGFILE_DIR, 0775);
                }
                else
                {
                    closedir(dir);
                    dir = NULL;
                }

                /* �����ļ� */
                pGCMSLog->sip_errorlog->logfile = fopen(filename, "w+");

                creat_interval = 0;
            }
        }
    }

    if (pGCMSLog->sip_errorlog->logfile == NULL)
    {
        return;
    }

    if (g_IsLogSIPMsg)
    {
        if (stat(filename, &statbuf) == -1)
        {
            if (NULL != pGCMSLog->sip_errorlog->logfile)
            {
                fclose(pGCMSLog->sip_errorlog->logfile);
                pGCMSLog->sip_errorlog->logfile = NULL;
            }

#if 0
            localtime_r(&now, &tp);
            memset(log_file_time, 0, 20);
            strftime(log_file_time, 20, "%Y_%m_%d_%H", &tp);

            memset(filename, 0, 256);
            snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_SIP_ERRORMSG, log_file_time);
#endif

            if (NULL == (pGCMSLog->sip_errorlog->logfile = fopen(filename, "w+")))
            {
                return;
            }
        }

        if (statbuf.st_size >= SIPERRMSG_FILE_SIZE)
        {
            if (NULL != pGCMSLog->sip_errorlog->logfile)
            {
                fclose(pGCMSLog->sip_errorlog->logfile);
                pGCMSLog->sip_errorlog->logfile = NULL;
            }

            if (NULL == (pGCMSLog->sip_errorlog->logfile = fopen(filename, "w+")))
            {
                return;
            }
        }
    }

    localtime_r(&now, &tp);
    strftime(strtm, 20, "%Y/%m/%d %H:%M:%S", &tp);

    if (type == 2)
    {
        fprintf(pGCMSLog->sip_errorlog->logfile, "\n+---------------------------------------------\n");
        fprintf(pGCMSLog->sip_errorlog->logfile, "No.%i\n", ++pGCMSLog->sip_errorlog->cnt);
        fprintf(pGCMSLog->sip_errorlog->logfile, "Time: %s\n", strtm);
        fprintf(pGCMSLog->sip_errorlog->logfile, "Error: Sent to IP=%s port=%d\n", ipaddr, port);
    }
    else if (type == 3)
    {
        fprintf(pGCMSLog->sip_errorlog->logfile, "\n+---------------------------------------------\n");
        fprintf(pGCMSLog->sip_errorlog->logfile, "No.%i\n", ++pGCMSLog->sip_errorlog->cnt);
        fprintf(pGCMSLog->sip_errorlog->logfile, "Time: %s\n", strtm);
        fprintf(pGCMSLog->sip_errorlog->logfile, "Parse Error: Received from IP=%s port=%d\n", ipaddr, port);
    }
    else if (type == 4)
    {
        fprintf(pGCMSLog->sip_errorlog->logfile, "\n+---------------------------------------------\n");
        fprintf(pGCMSLog->sip_errorlog->logfile, "No.%i\n", ++pGCMSLog->sip_errorlog->cnt);
        fprintf(pGCMSLog->sip_errorlog->logfile, "Time: %s\n", strtm);
        fprintf(pGCMSLog->sip_errorlog->logfile, "Message Error: Received from IP=%s port=%d\n", ipaddr, port);
    }
    else if (type == 5)
    {
        fprintf(pGCMSLog->sip_errorlog->logfile, "\n+---------------------------------------------\n");
        fprintf(pGCMSLog->sip_errorlog->logfile, "No.%i\n", ++pGCMSLog->sip_errorlog->cnt);
        fprintf(pGCMSLog->sip_errorlog->logfile, "Time: %s\n", strtm);
        fprintf(pGCMSLog->sip_errorlog->logfile, "Create Transaction Error: Received from IP=%s port=%d\n", ipaddr, port);
    }
    else
    {
        return;
    }

    fprintf(pGCMSLog->sip_errorlog->logfile, "\n%s\n", msg);
    fflush(pGCMSLog->sip_errorlog->logfile);

    return;
}

/*****************************************************************************
 �� �� ��  : LogDebugTraceToFile
 ��������  : ϵͳ�������Ӽ�¼���ļ�
 �������  : char* msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void LogDebugTraceToFile(char* msg)
{
    int i = 0;
    struct stat statbuf;
    char filename[256] = {0};
    static int creat_interval = 0;
    //time_t now = time(NULL);
    //struct tm tp = {0};
    DIR* dir = NULL;

    if (NULL == msg)
    {
        return;
    }

    if (pGCMSLog == NULL
        || pGCMSLog->debug_log == NULL)
    {
        return;
    }

    if (g_IsLog2File)
    {
        memset(filename, 0, 256);
        snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_DEBUG, log_file_time);

        /* �ж��Ƿ���Ҫ�����ļ� */
        if (NULL == pGCMSLog->debug_log->logfile)
        {
            creat_interval++;

            if (creat_interval >= 60)
            {
                /* ���Ŀ¼�Ƿ���� */
                dir = opendir("/data");

                if (NULL == dir)
                {
                    i = mkdir("/data", 0775);
                }
                else
                {
                    closedir(dir);
                    dir = NULL;
                }

                dir = opendir(LOGFILE_DIR);

                if (NULL == dir)
                {
                    i = mkdir(LOGFILE_DIR, 0775);
                }
                else
                {
                    closedir(dir);
                    dir = NULL;
                }

                /* �����ļ� */
                pGCMSLog->debug_log->logfile = fopen(filename, "w+");

                creat_interval = 0;
            }
        }
    }

    if (pGCMSLog->debug_log->logfile == NULL)
    {
        return;
    }

    if (g_IsLog2File)
    {
        if (stat(filename, &statbuf) == -1)
        {
            if (NULL != pGCMSLog->debug_log->logfile)
            {
                fclose(pGCMSLog->debug_log->logfile);
                pGCMSLog->debug_log->logfile = NULL;
            }

#if 0
            localtime_r(&now, &tp);
            memset(log_file_time, 0, 20);
            strftime(log_file_time, 20, "%Y_%m_%d_%H", &tp);

            memset(filename, 0, 256);
            snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_DEBUG, log_file_time);
#endif

            if (NULL == (pGCMSLog->debug_log->logfile = fopen(filename, "w+")))
            {
                return;
            }
        }

        if (statbuf.st_size >= DEBUG_LOG_FILE_SIZE)
        {
            if (NULL != pGCMSLog->debug_log->logfile)
            {
                fclose(pGCMSLog->debug_log->logfile);
                pGCMSLog->debug_log->logfile = NULL;
            }

            if (NULL == (pGCMSLog->debug_log->logfile = fopen(filename, "w+")))
            {
                return;
            }
        }
    }

    if (NULL != pGCMSLog && NULL != pGCMSLog->debug_log && NULL != pGCMSLog->debug_log->logfile)
    {
        fprintf(pGCMSLog->debug_log->logfile, "%s", msg);
        fflush(pGCMSLog->debug_log->logfile);
    }

    return;
}

/*****************************************************************************
 �� �� ��  : LogRunTraceToFile
 ��������  : ϵͳ�������Ӽ�¼���ļ�
 �������  : char* msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void LogRunTraceToFile(char* msg)
{
    int i = 0;
    struct stat statbuf;
    char filename[256] = {0};
    static int creat_interval = 0;
    //time_t now = time(NULL);
    //struct tm tp = {0};
    DIR* dir = NULL;

    if (NULL == msg)
    {
        return;
    }

    if (pGCMSLog == NULL
        || pGCMSLog->run_log == NULL)
    {
        return;
    }

    if (g_IsLog2File)
    {
        memset(filename, 0, 256);
        snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_RUN, log_file_time);

        /* �ж��Ƿ���Ҫ�����ļ� */
        if (NULL == pGCMSLog->run_log->logfile)
        {
            creat_interval++;

            if (creat_interval >= 60)
            {
                /* ���Ŀ¼�Ƿ���� */
                dir = opendir("/data");

                if (NULL == dir)
                {
                    i = mkdir("/data", 0775);
                }
                else
                {
                    closedir(dir);
                    dir = NULL;
                }

                dir = opendir(LOGFILE_DIR);

                if (NULL == dir)
                {
                    i = mkdir(LOGFILE_DIR, 0775);
                }
                else
                {
                    closedir(dir);
                    dir = NULL;
                }

                /* �����ļ� */
                pGCMSLog->run_log->logfile = fopen(filename, "w+");

                creat_interval = 0;
            }
        }
    }

    if (pGCMSLog->run_log->logfile == NULL)
    {
        return;
    }

    if (g_IsLog2File)
    {
        if (stat(filename, &statbuf) == -1)
        {
            if (NULL != pGCMSLog->run_log->logfile)
            {
                fclose(pGCMSLog->run_log->logfile);
                pGCMSLog->run_log->logfile = NULL;
            }

#if 0
            localtime_r(&now, &tp);
            memset(log_file_time, 0, 20);
            strftime(log_file_time, 20, "%Y_%m_%d_%H", &tp);

            memset(filename, 0, 256);
            snprintf(filename, 256, "%s%s_%s.log", LOGFILE_DIR, LOGFILE_RUN, log_file_time);
#endif

            if (NULL == (pGCMSLog->run_log->logfile = fopen(filename, "w+")))
            {
                return;
            }
        }

        if (statbuf.st_size >= RUN_LOG_FILE_SIZE)
        {
            if (NULL != pGCMSLog->run_log->logfile)
            {
                fclose(pGCMSLog->run_log->logfile);
                pGCMSLog->run_log->logfile = NULL;
            }

            if (NULL == (pGCMSLog->run_log->logfile = fopen(filename, "w+")))
            {
                return;
            }
        }
    }

    if (NULL != pGCMSLog && NULL != pGCMSLog->run_log && NULL != pGCMSLog->run_log->logfile)
    {
        fprintf(pGCMSLog->run_log->logfile, "%s", msg);
        fflush(pGCMSLog->run_log->logfile);
    }

    return;
}

void ListDirFile(char* path, int indent)  //indentΪ���ʱ������
{
    struct dirent* ent = NULL;
    DIR*     pDir = NULL;
    char    dir[512] = {0};
    struct stat    statbuf;

    if ((pDir = opendir(path)) == NULL)
    {
        fprintf(stderr, "Cannot open directory:%s\n", path);
        return;
    }

    while ((ent = readdir(pDir)) != NULL)
    {
        //�õ���ȡ�ļ��ľ���·����
        snprintf(dir, 512, "%s/%s", path, ent->d_name);
        //�õ��ļ���Ϣ
        lstat(dir, &statbuf);

        //�ж���Ŀ¼�����ļ�
        if (S_ISDIR(statbuf.st_mode))
        {
            //�ų���ǰĿ¼���ϼ�Ŀ¼
            if (strcmp(".", ent->d_name) == 0 ||
                strcmp("..", ent->d_name) == 0)
            {
                continue;
            }

            //�������Ŀ¼,�ݹ���ú�������,ʵ����Ŀ¼���ļ�����
            printf("%*s��Ŀ¼:%s/\n", indent, "", ent->d_name);
            //�ݹ����,������Ŀ¼���ļ�
            ListDirFile(dir, indent + 4);
        }
        else
        {
            printf("%*s�ļ�:%s\n", indent, "", ent->d_name);
        }
    }//while

    closedir(pDir);
}

void DeleteOutDateFile(int iYear, int iMonth, int iDay, int iHour)
{
    char strCmd[256] = {0};
    int tmpYear = 1900 + iYear;
    int tmpMonth = iMonth + 1;
    int tmpDay = iDay;
    int tmpHour = iHour;

    printf("\r\n DeleteOutDateFile:iYear=%d, iMonth=%d, iDay=%d, iHour=%d \r\n", tmpYear, tmpMonth, tmpDay, tmpHour);

    if ((28 == tmpDay && 2 == tmpMonth && tmpYear % 4 == 0)
        || (29 == tmpDay && 2 == tmpMonth && tmpYear % 4 != 0)
        || (30 == tmpDay && (4 == tmpMonth || 6 == tmpMonth || 9 == tmpMonth || 11 == tmpMonth))
        || (30 == tmpDay && (1 == tmpMonth || 3 == tmpMonth || 5 == tmpMonth || 7 == tmpMonth || 8 == tmpMonth || 10 == tmpMonth || 12 == tmpMonth)))
    {

    }

    if (1 == tmpDay) /* ÿ���µĵ�һ�� */
    {

    }
    else
    {
        memset(strCmd, 0, 256);
        snprintf(strCmd, 256, "%s%s_%d_%d_*.log", LOGFILE_DIR, LOGFILE_SIP_MSG, tmpYear - 1, 11);

        system(strCmd);
        printf("\r\n DeleteOutDateFile:delete file=%s \r\n", strCmd);

        memset(strCmd, 0, 256);
        snprintf(strCmd, 256, "%s%s_%d_%d_*.log", LOGFILE_DIR, LOGFILE_SIP_ERRORMSG, iYear + 1900 - 1, 11);

        system(strCmd);
        printf("\r\n DeleteOutDateFile:delete file=%s \r\n", strCmd);

        memset(strCmd, 0, 256);
        snprintf(strCmd, 256, "%s%s_%d_%d_*.log", LOGFILE_DIR, LOGFILE_RUN, iYear + 1900 - 1, 11);

        system(strCmd);
        printf("\r\n DeleteOutDateFile:delete file=%s \r\n", strCmd);

        memset(strCmd, 0, 256);
        snprintf(strCmd, 256, "%s%s_%d_%d_*.log", LOGFILE_DIR, LOGFILE_DEBUG, iYear + 1900 - 1, 11);

        system(strCmd);
        printf("\r\n DeleteOutDateFile:delete file=%s \r\n", strCmd);
    }
}
#endif

#if DECS("��־��¼�����ݿ�")
/*****************************************************************************
 �� �� ��  : SystemLogToDB
 ��������  : ϵͳ������־��¼�����ݿ�
 �������  : char* msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void SystemLogToDB(DBOper* pLog2DB_dboper, system_log2db_t* system_log)
{
    int iRet = 0;
    char* local_ip = NULL;
    string strInsertSQL = "";
    char strFromType[16] = {0};
    char strDeviceIndex[16] = {0};
    char strType[16] = {0};
    char strLevel[16] = {0};
    char strTime[64] = {0};

    if (NULL == pLog2DB_dboper || NULL == system_log)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SystemLogToDB() Log2DB DB Oper Error\r\n");
        return;
    }

    snprintf(strFromType, 16, "%d", EV9000_LOG_FROMTYPE_CMS);
    snprintf(strDeviceIndex, 16, "%d", local_cms_index_get());
    snprintf(strType, 16, "%d", system_log->iType);
    snprintf(strLevel, 16, "%d", system_log->iLevel);
    snprintf(strTime, 16, "%d", system_log->iTime);


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

    strInsertSQL += "'";
    strInsertSQL += strType;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    strInsertSQL += strLevel;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    strInsertSQL += strTime;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    strInsertSQL += system_log->msg;
    strInsertSQL += "'";

    strInsertSQL += ")";

    iRet = pLog2DB_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

    if (iRet < 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SystemLogToDB() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SystemLogToDB() ErrorMsg=%s\r\n", pLog2DB_dboper->GetLastDbErrorMsg());
    }

    return;
}

/*****************************************************************************
 �� �� ��  : UserLogToDB
 ��������  : �û���־��¼�����ݿ�
 �������  : char* msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void UserLogToDB(DBOper* pLog2DB_dboper, user_log2db_t* user_log)
{
    int iRet = 0;
    string strInsertSQL = "";
    char strUserIndex[32] = {0};
    char strType[16] = {0};
    char strLevel[16] = {0};
    char strTime[64] = {0};

    if (NULL == pLog2DB_dboper || NULL == user_log)
    {
        return;
    }

    sprintf(strUserIndex, "%u", user_log->iUserIndex);
    sprintf(strType, "%d", user_log->iType);
    sprintf(strLevel, "%d", user_log->iLevel);
    sprintf(strTime, "%d", user_log->iTime);

    /* ����SQL��� */
    strInsertSQL.clear();
    strInsertSQL = "insert into UserLogRecord (LogID,LogIP,UserIndex,Type,Level,Time,Info) values (";

    strInsertSQL += "'";
    strInsertSQL += user_log->pcUserID;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    strInsertSQL += user_log->pcUserIP;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    strInsertSQL += strUserIndex;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    strInsertSQL += strType;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    strInsertSQL += strLevel;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    strInsertSQL += strTime;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    strInsertSQL += user_log->msg;
    strInsertSQL += "'";

    strInsertSQL += ")";

    iRet = pLog2DB_dboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

    if (iRet < 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "UserLogToDB() DB Insert:strInsertSQL=%s,iRet=%d \r\n", strInsertSQL.c_str(), iRet);
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UserLogToDB() ErrorMsg=%s\r\n", pLog2DB_dboper->GetLastDbErrorMsg());
    }

    return;
}
#endif

#if DECS("��־����ں���")
/*****************************************************************************
 �� �� ��  : DebugTrace
 ��������  : Debug������־�������
 �������  : int iModule
                            int iLevel
                            const char* fmt
                            ...
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��6��29��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void DebugTrace(int iModule, int iLevel, const char* FILENAME, const char* FUNCTIONNAME, int CODELINE, const char* fmt, ...)
{
    int iRet = 0;
    int len = 0;
    time_t now = time(NULL);
    struct tm tp = {0};
    char strtm[20] = {0};
    va_list args;
    char s[MAX_2048CHAR_STRING_LEN + 4] = {0};
    char buf[MAX_2048CHAR_STRING_LEN + 4] = {0};
    char* pModelName = NULL;

    switch (iModule)
    {
        case MODULE_COMMON :
            if (iLevel < g_CommonDbgLevel)
            {
                return;
            }

            pModelName = (char*)"COMMON";
            break;

        case MODULE_SIPSTACK :
            if (iLevel < g_SIPStackDbgLevel)
            {
                return;
            }

            pModelName = (char*)"SIPSTACK";
            break;

        case MODULE_USER :
            if (iLevel < g_UserDbgLevel)
            {
                return;
            }

            pModelName = (char*)"USER";
            break;

        case MODULE_DEVICE :
            if (iLevel < g_DeviceDbgLevel)
            {
                return;
            }

            pModelName = (char*)"DEVICE";
            break;

        case MODULE_ROUTE :
            if (iLevel < g_RouteDbgLevel)
            {
                return;
            }

            pModelName = (char*)"ROUTE";
            break;

        case MODULE_RECORD :
            if (iLevel < g_RecordDbgLevel)
            {
                return;
            }

            pModelName = (char*)"RECORD";
            break;

        case MODULE_RESOURCE :
            if (iLevel < g_ResourceDbgLevel)
            {
                return;
            }

            pModelName = (char*)"RESOURCE";
            break;

        case MODULE_CRUISE_SRV :
            if (iLevel < g_CruiseDbgLevel)
            {
                return;
            }

            pModelName = (char*)"CRUISE";
            break;

        case MODULE_PLAN_SRV :
            if (iLevel < g_PlanDbgLevel)
            {
                return;
            }

            pModelName = (char*)"PLAN";
            break;

        case MODULE_POLL_SRV :
            if (iLevel < g_PollDbgLevel)
            {
                return;
            }

            pModelName = (char*)"POLL";
            break;

        default:
            return;
    }

    /* ֱ�Ӵ�ӡ */
#if 0
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif

    va_start(args, fmt);
    len = vsnprintf(s, MAX_2048CHAR_STRING_LEN, fmt, args);
    va_end(args);

    localtime_r(&now, &tp);
    strftime(strtm, 20, "%Y/%m/%d %H:%M:%S", &tp);

    snprintf(buf, MAX_2048CHAR_STRING_LEN + 4, "\r\n %s |%s (%s:%d) ->>> %s", strtm, pModelName, FILENAME, CODELINE, s);

    /* ��ӡ���ն� */
    //printf(buf);

    /* ��ӡ��Telnet �ն� */
    TelnetSend(buf);

    /* ��¼���ļ� */
    iRet = trace_log2file_add(0, buf);

    return;
}

/*****************************************************************************
 �� �� ��  : DebugRunTrace
 ��������  : ϵͳ������־
 �������  : const char* fmt
                             ...
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void DebugRunTrace(const char* fmt, ...)
{
    int iRet = 0;
    int len = 0;
    time_t now = time(NULL);
    struct tm tp = {0};
    char strtm[20] = {0};
    va_list args;
    char s[MAX_2048CHAR_STRING_LEN + 4] = {0};
    char buf[MAX_2048CHAR_STRING_LEN + 4] = {0};

    va_start(args, fmt);
    len = vsnprintf(s, MAX_2048CHAR_STRING_LEN, fmt, args);
    va_end(args);

    localtime_r(&now, &tp);
    strftime(strtm, 20, "%Y/%m/%d %H:%M:%S", &tp);

    snprintf(buf, MAX_2048CHAR_STRING_LEN + 4, "[%s]%s", strtm, s);

    /* ��ӡ��Telnet �ն� */
    TelnetRunTraceSend(buf);

    /* ��¼���ļ� */
    iRet = trace_log2file_add(1, buf);

    return;
}

/*****************************************************************************
 �� �� ��  : DebugSIPMessage
 ��������  : SIP��Ϣ���ٵ���
 �������  : int type:
                            0,��ȷ��
                            1:���ʹ����
                            2:���ս��������
                            3:������Ϣ�����
                            4:���մ�����������
                            int iDirect:
                            0:���͵�
                            1:���յ�
                            char* ipaddr
                            int port
                            char* msg
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��9��4�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void DebugSIPMessage(int type, int iDirect, char* ipaddr, int port, char* msg)
{
    char* pDirectParam = NULL;
    char* pErrorParam = NULL;

    if (0 == type) /* ��ȷ����Ϣ */
    {
        //LogSipMsgToFile(iDirect, msg, ipaddr, port);
        sip_msg_log2file_add(iDirect, msg, ipaddr, port);
    }
    else
    {
        //LogSipErrMsgToFile(type, msg, ipaddr, port);
        sip_msg_log2file_add(type + 1, msg, ipaddr, port);
    }

    if (0 == iDirect)
    {
        pDirectParam = (char*)">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\nSent To: ";
    }
    else if (1 == iDirect)
    {
        pDirectParam = (char*)"<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\r\nReceived From: ";
    }

    if (1 == type)
    {
        pErrorParam = (char*)"+---------------------------------------------\r\nError: Sent to ";
    }
    else if (2 == type)
    {
        pErrorParam = (char*)"+---------------------------------------------\r\nParse Error: Received from ";
    }
    else if (3 == type)
    {
        pErrorParam = (char*)"+---------------------------------------------\r\nMessage Error: Received from ";
    }
    else if (4 == type)
    {
        pErrorParam = (char*)"+---------------------------------------------\r\nCreate Transaction Error: Received from ";
    }

    /* ��ӡ��Telnet �ն� */
    if (0 == type)
    {
        TelnetSendSIPMessage("\r\n %s IP=%s port=%d\r\n%s\r\n", pDirectParam, ipaddr, port, msg);
    }
    else
    {
        TelnetSendSIPMessage("\r\n %s IP=%s port=%d\r\n%s\r\n", pErrorParam, ipaddr, port, msg);
    }

    return;
}

/*****************************************************************************
 �� �� ��  : SystemLog
 ��������  : ϵͳ��־��¼
 �������  : int iType
             int iLevel
             const char* fmt
             ...
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��20�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void EnSystemLog(int iType, int iLevel, const char* fmt, ...)
{
    int iRet = 0;
    int len = 0;
    va_list args;
    char s[MAX_2048CHAR_STRING_LEN + 4] = {0};
    time_t now = time(NULL);
    char* pLevelName = NULL;

    if (1 != g_Language)
    {
        return;
    }

    if (iLevel < g_SystemLogLevel)
    {
        goto log2db;
    }

    switch (iLevel)
    {
        case EV9000_LOG_LEVEL_NORMAL :
            pLevelName = (char*)"Normal";
            break;

        case EV9000_LOG_LEVEL_WARNING :
            pLevelName = (char*)"Warning";
            break;

        case EV9000_LOG_LEVEL_ERROR :
            pLevelName = (char*)"Error";
            break;

        default:
            return;
    }

    va_start(args, fmt);
    len = vsnprintf(s, MAX_2048CHAR_STRING_LEN, fmt, args);
    va_end(args);

    DebugRunTrace("[System Log][%s]: %s\r\n", pLevelName, s);

log2db:

    if (iLevel < g_SystemLog2DBLevel)
    {
        return;
    }

    len = strlen(s);

    if (len >= 512)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SystemLog() exit---: Message Length Error:Msg=%s, len=%d \r\n", s, len);
        return;
    }

    /* ��¼�����ݿ� */
    iRet = system_log2db_add(iType, iLevel, now, s);

    return;

}

/*****************************************************************************
 �� �� ��  : SystemLog
 ��������  : ϵͳ��־��¼
 �������  : int iType
             int iLevel
             const char* fmt
             ...
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��20�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void SystemLog(int iType, int iLevel, const char* fmt, ...)
{
    int iRet = 0;
    int len = 0;
    va_list args;
    char s[MAX_2048CHAR_STRING_LEN + 4] = {0};
    time_t now = time(NULL);
    char* pLevelName = NULL;

    if (0 != g_Language)
    {
        return;
    }

    if (iLevel < g_SystemLogLevel)
    {
        goto log2db;
    }

    switch (iLevel)
    {
        case EV9000_LOG_LEVEL_NORMAL :
            pLevelName = (char*)"һ��";
            break;

        case EV9000_LOG_LEVEL_WARNING :
            pLevelName = (char*)"����";
            break;

        case EV9000_LOG_LEVEL_ERROR :
            pLevelName = (char*)"����";
            break;

        case EV9000_LOG_LEVEL_IMPORTANT :
            pLevelName = (char*)"��Ҫ";
            break;

        default:
            return;
    }

    va_start(args, fmt);
    len = vsnprintf(s, MAX_2048CHAR_STRING_LEN, fmt, args);
    va_end(args);

    DebugRunTrace("[ϵͳ������־][%s]: %s\r\n", pLevelName, s);

log2db:

    if (iLevel < g_SystemLog2DBLevel)
    {
        return;
    }

    len = strlen(s);

    if (len >= 512)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SystemLog() exit---: Message Length Error:Msg=%s, len=%d \r\n", s, len);
        return;
    }

    /* ��¼�����ݿ� */
    iRet = system_log2db_add(iType, iLevel, now, s);

    return;
}

/*****************************************************************************
 �� �� ��  : UserLog
 ��������  : �û���־��¼
 �������  : int iType
             int iLevel
             user_info_t* pUserInfo
             const char* fmt
           ` ...
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��20�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void UserLog(int iType, int iLevel, user_info_t* pUserInfo, const char* fmt, ...)
{
    int iRet = 0;
    int len = 0;
    va_list args;
    char s[MAX_2048CHAR_STRING_LEN + 4] = {0};
    time_t now = time(NULL);
    char* pLevelName = NULL;

    if (0 != g_Language)
    {
        return;
    }

#if 0

    if (iLevel < g_SystemLogLevel)
    {
        return;
    }

#endif

    switch (iLevel)
    {
        case EV9000_LOG_LEVEL_NORMAL :
            pLevelName = (char*)"һ��";
            break;

        case EV9000_LOG_LEVEL_WARNING :
            pLevelName = (char*)"����";
            break;

        case EV9000_LOG_LEVEL_ERROR :
            pLevelName = (char*)"����";
            break;

        default:
            return;
    }

    va_start(args, fmt);
    len = vsnprintf(s, MAX_2048CHAR_STRING_LEN, fmt, args);
    va_end(args);

    DebugRunTrace("[�û�������־][%s]: %s:�û�ID=%s, IP��ַ=%s, �˿ں�=%d\r\n", pLevelName, s, pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

    len = strlen(s);

    if (len >= 512)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UserLog() exit---: Message Length Error:Msg=%s, len=%d \r\n", s, len);
        return;
    }

    /* ��¼�����ݿ� */
    iRet = user_log2db_add(iType, iLevel, pUserInfo, now, s);

    return;
}

/*****************************************************************************
 �� �� ��  : EnUserLog
 ��������  : �û���־��¼
 �������  : int iType
             int iLevel
             user_info_t* pUserInfo
             const char* fmt
           ` ...
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��8��20�� ���ڶ�
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void EnUserLog(int iType, int iLevel, user_info_t* pUserInfo, const char* fmt, ...)
{
    int iRet = 0;
    int len = 0;
    va_list args;
    char s[MAX_2048CHAR_STRING_LEN + 4] = {0};
    time_t now = time(NULL);
    char* pLevelName = NULL;

    if (1 != g_Language)
    {
        return;
    }

#if 0

    if (iLevel < g_SystemLogLevel)
    {
        return;
    }

#endif

    switch (iLevel)
    {
        case EV9000_LOG_LEVEL_NORMAL :
            pLevelName = (char*)"Normal";
            break;

        case EV9000_LOG_LEVEL_WARNING :
            pLevelName = (char*)"Warning";
            break;

        case EV9000_LOG_LEVEL_ERROR :
            pLevelName = (char*)"Error";
            break;

        default:
            return;
    }

    va_start(args, fmt);
    len = vsnprintf(s, MAX_2048CHAR_STRING_LEN, fmt, args);
    va_end(args);

    DebugRunTrace("[User Operator Log][%s]: %s:User ID=%s, User IP=%s, User Port=%d\r\n", pLevelName, s, pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

    len = strlen(s);

    if (len >= 512)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UserLog() exit---: Message Length Error:Msg=%s, len=%d \r\n", s, len);
        return;
    }

    /* ��¼�����ݿ� */
    iRet = user_log2db_add(iType, iLevel, pUserInfo, now, s);

    return;
}
#endif
