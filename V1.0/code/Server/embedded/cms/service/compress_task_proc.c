
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
#include "common/gblfunc_proc.inc"
#include "common/log_proc.inc"
#include "common/gblconfig_proc.inc"

#include "service/compress_task_proc.inc"
#include "resource/resource_info_mgn.inc"

#include "user/user_info_mgn.inc"
#include "user/user_srv_proc.inc"

#include "device/device_info_mgn.inc"
#include "device/device_srv_proc.inc"

#include "route/route_info_mgn.inc"
#include "record/record_srv_proc.inc"
#include "jwpt_interface/interface_operate.inc"

/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/
extern GBLogicDevice_Info_MAP g_GBLogicDeviceInfoMap; /* ��׼�߼��豸��Ϣ���� */
extern int g_DECMediaTransferFlag;                    /* �¼�ý���е���ǽ�Ƿ񾭹�����TSUת��,Ĭ��ת�� */
extern GBDevice_Info_MAP g_GBDeviceInfoMap;           /* ��׼�����豸��Ϣ���� */
extern route_info_list_t* g_RouteInfoList ;           /* ·����Ϣ���� */

/*----------------------------------------------*
 * �ⲿ����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ȫ�ֱ���                                     *
 *----------------------------------------------*/
unsigned long long iCompressTaskDataLockCount = 0;
unsigned long long iCompressTaskDataUnLockCount = 0;

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/
COMPRESS_TASK_MAP g_CompressTaskMap; /* �����������ݶ��� */
#ifdef MULTI_THR
osip_mutex_t* g_CompressTaskMapLock = NULL;
#endif

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

#if DECS("ѹ���������")
/*****************************************************************************
 �� �� ��  : compress_task_init
 ��������  : �������ӽṹ��ʼ��
 �������  : compress_task_t** compress_task
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��19��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int compress_task_init(compress_task_t** compress_task)
{
    *compress_task = (compress_task_t*)osip_malloc(sizeof(compress_task_t));

    if (*compress_task == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "compress_task_init() exit---: *compress_task Smalloc Error \r\n");
        return -1;
    }

    memset(&((*compress_task)->stYSPB), 0, sizeof(jly_yspb_t));
    (*compress_task)->iAssignFlag = 0;
    (*compress_task)->strPlatformIP[0] = '\0';
    (*compress_task)->strZRVDeviceIP[0] = '\0';
    (*compress_task)->iTaskCreateTime = 0;
    (*compress_task)->iTaskStatus = 0;
    (*compress_task)->iTaskResult = 0;
    (*compress_task)->wait_answer_expire = 0;
    (*compress_task)->resend_count = 0;

    return 0;
}

/*****************************************************************************
 �� �� ��  : compress_task_free
 ��������  : �������ӽṹ�ͷ�
 �������  : compress_task_t * compress_task
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void compress_task_free(compress_task_t* compress_task)
{
    if (compress_task == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "compress_task_free() exit---: Param Error \r\n");
        return;
    }

    memset(&(compress_task->stYSPB), 0, sizeof(jly_yspb_t));
    compress_task->iAssignFlag = 0;
    memset(compress_task->strPlatformIP, 0, MAX_IP_LEN);
    memset(compress_task->strZRVDeviceIP, 0, MAX_IP_LEN);
    compress_task->iTaskCreateTime = 0;
    compress_task->iTaskStatus = 0;
    compress_task->iTaskResult = 0;
    compress_task->wait_answer_expire = 0;
    compress_task->resend_count = 0;

    return;
}

/*****************************************************************************
 �� �� ��  : compress_task_list_init
 ��������  : �������Ӷ��г�ʼ��
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
int compress_task_list_init()
{
    g_CompressTaskMap.clear();

#ifdef MULTI_THR
    g_CompressTaskMapLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_CompressTaskMapLock)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "compress_task_list_init() exit---: Call Record Map Lock Init Error \r\n");
        return -1;
    }

#endif

    return 0;
}

/*****************************************************************************
 �� �� ��  : compress_task_list_free
 ��������  : �������Ӷ����ͷ�
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
void compress_task_list_free()
{
    COMPRESS_TASK_Iterator Itr;
    compress_task_t* pCompressTaskData = NULL;

    for (Itr = g_CompressTaskMap.begin(); Itr != g_CompressTaskMap.end(); Itr++)
    {
        pCompressTaskData = Itr->second;

        if (NULL != pCompressTaskData)
        {
            compress_task_free(pCompressTaskData);
            osip_free(pCompressTaskData);
            pCompressTaskData = NULL;
        }
    }

    g_CompressTaskMap.clear();

#ifdef MULTI_THR

    if (NULL != g_CompressTaskMapLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_CompressTaskMapLock);
        g_CompressTaskMapLock = NULL;
    }

#endif
    return;
}

/*****************************************************************************
 �� �� ��  : compress_task_list_lock
 ��������  : �������Ӷ�������
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
int compress_task_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CompressTaskMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "compress_task_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_CompressTaskMapLock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : compress_task_list_unlock
 ��������  : �������Ӷ��н���
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
int compress_task_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CompressTaskMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "compress_task_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_CompressTaskMapLock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_compress_task_list_lock
 ��������  : �������Ӷ�������
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
int debug_compress_task_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CompressTaskMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "debug_compress_task_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_CompressTaskMapLock, file, line, func);

    iCompressTaskDataLockCount++;

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_compress_task_list_lock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iCompressTaskDataLockCount != iCompressTaskDataUnLockCount + 1)
        {
            //printf("\r\n**********%s:%d:%s:debug_compress_task_list_lock:iRet=%d, iCompressTaskDataLockCount=%lld**********\r\n", file, line, func, iRet, iCompressTaskDataLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_compress_task_list_lock:iRet=%d, iCompressTaskDataLockCount=%lld", file, line, func, iRet, iCompressTaskDataLockCount);
        }
    }

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_compress_task_list_unlock
 ��������  : �������Ӷ��н���
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
int debug_compress_task_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CompressTaskMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "debug_compress_task_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iCompressTaskDataUnLockCount++;

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_CompressTaskMapLock, file, line, func);

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_compress_task_list_unlock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iCompressTaskDataLockCount != iCompressTaskDataUnLockCount)
        {
            //printf("\r\n**********%s:%d:%s:debug_compress_task_list_unlock:iRet=%d, iCompressTaskDataUnLockCount=%lld**********\r\n", file, line, func, iRet, iCompressTaskDataUnLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_compress_task_list_unlock:iRet=%d, iCompressTaskDataUnLockCount=%lld", file, line, func, iRet, iCompressTaskDataUnLockCount);
        }
    }

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : compress_task_add
 ��������  : ��Ӻ������ӵ�������
 �������  : call_type_t call_type
                            char* call_id
                            int caller_ua_index
                            char* caller_id
                            char* callee_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int compress_task_add(compress_task_t* pCompressTaskData)
{
    if (pCompressTaskData == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "ZRVDevice_info_add() exit---: Param Error \r\n");
        return -1;
    }

    COMPRESS_TASK_SMUTEX_LOCK();

    g_CompressTaskMap[pCompressTaskData->stYSPB.jlbh] = pCompressTaskData;

    COMPRESS_TASK_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : compress_task_remove
 ��������  : �Ӷ������Ƴ���������
 �������  : int pos
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int compress_task_remove(char* task_id)
{
    compress_task_t* pCompressTaskData = NULL;
    COMPRESS_TASK_Iterator Itr;

    if (NULL == task_id || task_id[0] == '\0')
    {
        return -1;
    }

    COMPRESS_TASK_SMUTEX_LOCK();

    if (g_CompressTaskMap.size() <= 0)
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        return -1;
    }

    Itr = g_CompressTaskMap.find(task_id);

    if (Itr == g_CompressTaskMap.end())
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        return -1;
    }
    else
    {
        pCompressTaskData = Itr->second;
        g_CompressTaskMap.erase(task_id);

        if (NULL != pCompressTaskData)
        {
            compress_task_free(pCompressTaskData);
            osip_free(pCompressTaskData);
            pCompressTaskData = NULL;
        }
    }

    COMPRESS_TASK_SMUTEX_UNLOCK();
    return 0;
}

int compress_task_set_task_assign_info(char* task_id, int iAssignFlag, char* zrv_device_ip)
{
    compress_task_t* pCompressTaskData = NULL;
    COMPRESS_TASK_Iterator Itr;

    if (NULL == task_id || task_id[0] == '\0')
    {
        return -1;
    }

    COMPRESS_TASK_SMUTEX_LOCK();

    if (g_CompressTaskMap.size() <= 0)
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ѹ������ZRV������Ϣʧ��, ѹ���������Ϊ��:��¼���=%s, AssignFlag=%d, zrv_device_ip=%s", task_id, iAssignFlag, zrv_device_ip);
        return -1;
    }

    Itr = g_CompressTaskMap.find(task_id);

    if (Itr == g_CompressTaskMap.end())
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ѹ������ZRV������Ϣʧ��, û���ҵ���Ӧ��ѹ�������¼:��¼���=%s, AssignFlag=%d, zrv_device_ip=%s", task_id, iAssignFlag, zrv_device_ip);
        return -1;
    }
    else
    {
        pCompressTaskData = Itr->second;

        if (NULL != pCompressTaskData)
        {
            pCompressTaskData->iAssignFlag = iAssignFlag;

            if (pCompressTaskData->strZRVDeviceIP[0] == '\0')
            {
                osip_strncpy(pCompressTaskData->strZRVDeviceIP, zrv_device_ip, MAX_IP_LEN);
            }
            else
            {
                if (0 != sstrcmp(pCompressTaskData->strZRVDeviceIP, zrv_device_ip))
                {
                    memset(pCompressTaskData->strZRVDeviceIP, 0, MAX_IP_LEN);
                    osip_strncpy(pCompressTaskData->strZRVDeviceIP, zrv_device_ip, MAX_IP_LEN);
                }
            }
        }
    }

    COMPRESS_TASK_SMUTEX_UNLOCK();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ѹ������ZRV������Ϣ���ڴ�ɹ�:��¼���=%s, AssignFlag=%d, zrv_device_ip=%s", task_id, iAssignFlag, zrv_device_ip);
    return 0;
}

int compress_task_set_task_result_info(char* task_id, int iTaskStatus, int iTaskResult, int iYSHFileSize, char* pcDestUrl)
{
    int iRet = 0;
    compress_task_t* pCompressTaskData = NULL;
    COMPRESS_TASK_Iterator Itr;

    if (NULL == task_id || task_id[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "compress_task_set_task_result_info() exit---: task_id NULL \r\n");
        return -1;
    }

    COMPRESS_TASK_SMUTEX_LOCK();

    if (g_CompressTaskMap.size() <= 0)
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "compress_task_set_task_result_info() exit---: No CompressTask \r\n");
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ѹ����������Ϣʧ��, ѹ���������Ϊ��:��¼���=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
        return -1;
    }

    Itr = g_CompressTaskMap.find(task_id);

    if (Itr == g_CompressTaskMap.end())
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "compress_task_set_task_result_info() exit---: task_id=%s Not Found \r\n", task_id);
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ѹ����������Ϣʧ��, û���ҵ���Ӧ��ѹ�������¼:��¼���=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
        return -1;
    }
    else
    {
        pCompressTaskData = Itr->second;

        if (NULL != pCompressTaskData)
        {
            pCompressTaskData->iTaskStatus = iTaskStatus;
            pCompressTaskData->iTaskResult = iTaskResult;
            pCompressTaskData->stYSPB.yshdx = iYSHFileSize;

            if (NULL != pcDestUrl)
            {
                memset(pCompressTaskData->stYSPB.yshlj, 0, 128);
                osip_strncpy(pCompressTaskData->stYSPB.yshlj, pcDestUrl, 128);
            }
        }
    }

    COMPRESS_TASK_SMUTEX_UNLOCK();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ѹ����������Ϣ���ڴ�ɹ�:��¼���=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : compress_task_find
 ��������  : ��������ID���Һ��м�¼
 �������  : char* task_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��11��29��
    ��    ��   : �û�·����Ϣ����
    �޸�����   : �����ɺ���

*****************************************************************************/
compress_task_t* compress_task_find(char* task_id)
{
    compress_task_t* pCompressTaskData = NULL;
    COMPRESS_TASK_Iterator Itr;

    if (NULL == task_id || task_id[0] == '\0')
    {
        return NULL;
    }

    COMPRESS_TASK_SMUTEX_LOCK();

    if (g_CompressTaskMap.size() <= 0)
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        return NULL;
    }

    Itr = g_CompressTaskMap.find(task_id);

    if (Itr == g_CompressTaskMap.end())
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        return NULL;
    }
    else
    {
        pCompressTaskData = Itr->second;

        if (NULL != pCompressTaskData)
        {
            COMPRESS_TASK_SMUTEX_UNLOCK();
            return pCompressTaskData;
        }
    }

    COMPRESS_TASK_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 �� �� ��  : scan_compress_task_list_for_wait_expire
 ��������  : ɨ��ȴ���Ӧ���������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��12��26��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int scan_compress_task_list_for_wait_expire(DBOper* pDbOper, int* run_thread_time)
{
    int i = 0;
    int iRet = 0;
    int index = -1;
    compress_task_t* pCompressTaskData = NULL;
    COMPRESS_TASK_Iterator Itr;
    vector<string> COMPRESS_TASKIDVector;
    time_t now = time(NULL);

    ZRVDevice_info_t* pZRVDeviceInfo = NULL;
    char* device_ip = NULL;

    COMPRESS_TASKIDVector.clear();

    COMPRESS_TASK_SMUTEX_LOCK();

    if (g_CompressTaskMap.size() <= 0)
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "scan_compress_task_list_for_wait_expire() exit---: Call Record Srv Map NULL \r\n");
        return 0;
    }

    for (Itr = g_CompressTaskMap.begin(); Itr != g_CompressTaskMap.end(); Itr++)
    {
        pCompressTaskData = Itr->second;

        if (NULL == pCompressTaskData)
        {
            continue;
        }

        if (pCompressTaskData->stYSPB.jlbh[0] == '\0')
        {
            continue;
        }

        if (1 != pCompressTaskData->iTaskStatus)
        {
            continue;
        }

        pCompressTaskData->wait_answer_expire++;

        if (pCompressTaskData->wait_answer_expire >= 300) /* ���Сʱ��ʱ */
        {
            //pCompressTaskData->iTaskStatus = 3;
            //pCompressTaskData->iTaskResult = 2;
            pCompressTaskData->resend_count++;
            pCompressTaskData->wait_answer_expire = 0;
            COMPRESS_TASKIDVector.push_back(pCompressTaskData->stYSPB.jlbh);
        }
    }

    COMPRESS_TASK_SMUTEX_UNLOCK();

    if (COMPRESS_TASKIDVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_compress_task_list_for_wait_expire() COMPRESS_TASKIDVector.size()=%d \r\n", (int)COMPRESS_TASKIDVector.size());

        for (index = 0; index < (int)COMPRESS_TASKIDVector.size(); index++)
        {
            pCompressTaskData = compress_task_find((char*)COMPRESS_TASKIDVector[index].c_str());

            if (NULL == pCompressTaskData)
            {
                continue;
            }

            if (pCompressTaskData->strZRVDeviceIP[0] == '\0')
            {
                device_ip = get_zrv_device_ip_by_min_task_count(pCompressTaskData->strPlatformIP);

                if (NULL == device_ip)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ѹ������ȴ���Ӧ��ʱ, �����·���ZRV�豸ʧ��, ԭ��ZRV�豸IP��ַʧЧ, ���»�ȡ���õ�ZRV�豸IP��ַʧ��, ��¼���=%s, ԭ��ZRV�豸IP=%s", (char*)COMPRESS_TASKIDVector[index].c_str(), pCompressTaskData->strZRVDeviceIP);
                    pCompressTaskData->wait_answer_expire = 300;
                    continue;
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "ѹ������ȴ���Ӧ��ʱ, �����·���ZRV�豸ʧ��, ԭ��ZRV�豸IP��ַʧЧ, ���»�ȡ���õ�ZRV�豸IP��ַ, ��¼���=%s, ԭ��ZRV�豸IP=%s, �µ�ZRV�豸IP=%s", (char*)COMPRESS_TASKIDVector[index].c_str(), pCompressTaskData->strZRVDeviceIP, device_ip);
                    iRet = UpdateCompressTaskAssignInfo((char*)COMPRESS_TASKIDVector[index].c_str(), 1, device_ip, pDbOper);
                }
            }
            else
            {
                /* �鿴ZRV�豸�Ƿ��Ѿ����ߣ����������·��������ZRV�豸 */
                pZRVDeviceInfo = ZRVDevice_info_find(pCompressTaskData->strZRVDeviceIP);

                if (NULL == pZRVDeviceInfo)
                {
                    /* ���·��� */
                    device_ip = get_zrv_device_ip_by_min_task_count(pCompressTaskData->strPlatformIP);

                    if (NULL == device_ip)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ѹ������ȴ���Ӧ��ʱ, �����·���ZRV�豸ʧ��, ԭ��ZRV�豸ʧЧ, ���»�ȡ���õ�ZRV�豸IP��ַʧ��, ��¼���=%s, ԭ��ZRV�豸IP=%s", (char*)COMPRESS_TASKIDVector[index].c_str(), pCompressTaskData->strZRVDeviceIP);
                        pCompressTaskData->wait_answer_expire = 300;
                        continue;
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "ѹ������ȴ���Ӧ��ʱ, �����·���ZRV�豸ʧ��, ԭ��ZRV�豸ʧЧ, ���»�ȡ���õ�ZRV�豸IP��ַ, ��¼���=%s, ԭ��ZRV�豸IP=%s, �µ�ZRV�豸IP=%s", (char*)COMPRESS_TASKIDVector[index].c_str(), pCompressTaskData->strZRVDeviceIP, device_ip);
                        iRet = UpdateCompressTaskAssignInfo((char*)COMPRESS_TASKIDVector[index].c_str(), 1, device_ip, pDbOper);
                    }
                }
                else
                {
                    if (pZRVDeviceInfo->reg_status <= 0 || pZRVDeviceInfo->tcp_sock <= 0)
                    {
                        /* ���·��� */
                        device_ip = get_zrv_device_ip_by_min_task_count(pCompressTaskData->strPlatformIP);

                        if (NULL == device_ip)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ѹ������ȴ���Ӧ��ʱ, �����·���ZRV�豸ʧ��, ԭ��ZRV�豸����, ���»�ȡ���õ�ZRV�豸IP��ַʧ��, ��¼���=%s, ԭ��ZRV�豸IP=%s", (char*)COMPRESS_TASKIDVector[index].c_str(), pCompressTaskData->strZRVDeviceIP);
                            pCompressTaskData->wait_answer_expire = 300;
                            continue;
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "ѹ������ȴ���Ӧ��ʱ, �����·���ZRV�豸ʧ��, ԭ��ZRV�豸����, ���»�ȡ���õ�ZRV�豸IP��ַ, ��¼���=%s, ԭ��ZRV�豸IP=%s, �µ�ZRV�豸IP=%s", (char*)COMPRESS_TASKIDVector[index].c_str(), pCompressTaskData->strZRVDeviceIP, device_ip);
                            iRet = UpdateCompressTaskAssignInfo((char*)COMPRESS_TASKIDVector[index].c_str(), 1, device_ip, pDbOper);
                        }
                    }
                }
            }

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ѹ������ȴ���Ӧ��ʱ, �����·�����ZRV�豸, �����·�����=%d, ��¼���=%s, ƽ̨IP��ַ=%s, ZRV�豸IP��ַ=%s", pCompressTaskData->resend_count, pCompressTaskData->stYSPB.jlbh, pCompressTaskData->strPlatformIP, pCompressTaskData->strZRVDeviceIP);

            iRet = SendNotCompleteTaskToZRVDeviceForTimeOut((char*)COMPRESS_TASKIDVector[index].c_str(), run_thread_time);
        }
    }

    COMPRESS_TASKIDVector.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : update_compress_task_list_for_wait_expire
 ��������  : ����ѹ������ʱ
 �������  : DBOper* pDbOper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��8��21��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int update_compress_task_list_for_wait_expire(DBOper* pDbOper)
{
    int i = 0;
    int iRet = 0;
    int index = -1;
    compress_task_t* pCompressTaskData = NULL;
    COMPRESS_TASK_Iterator Itr;
    vector<string> COMPRESS_TASKIDVector;
    time_t now = time(NULL);

    COMPRESS_TASKIDVector.clear();

    COMPRESS_TASK_SMUTEX_LOCK();

    if (g_CompressTaskMap.size() <= 0)
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "scan_compress_task_list_for_wait_expire() exit---: Call Record Srv Map NULL \r\n");
        return 0;
    }

    for (Itr = g_CompressTaskMap.begin(); Itr != g_CompressTaskMap.end(); Itr++)
    {
        pCompressTaskData = Itr->second;

        if (NULL == pCompressTaskData)
        {
            continue;
        }

        if (pCompressTaskData->stYSPB.jlbh[0] == '\0')
        {
            continue;
        }

        if (1 != pCompressTaskData->iTaskStatus)
        {
            continue;
        }

        pCompressTaskData->iTaskStatus = 3;
        pCompressTaskData->iTaskResult = 2;
        COMPRESS_TASKIDVector.push_back(pCompressTaskData->stYSPB.jlbh);
    }

    COMPRESS_TASK_SMUTEX_UNLOCK();

    if (COMPRESS_TASKIDVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "update_compress_task_list_for_wait_expire() COMPRESS_TASKIDVector.size()=%d \r\n", (int)COMPRESS_TASKIDVector.size());

        for (index = 0; index < (int)COMPRESS_TASKIDVector.size(); index++)
        {
            pCompressTaskData = compress_task_find((char*)COMPRESS_TASKIDVector[index].c_str());

            if (NULL == pCompressTaskData)
            {
                continue;
            }

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ѹ������ȴ���Ӧ��ʱ, �����ر�:��¼���=%s, ƽ̨IP��ַ=%s, ZRV�豸IP��ַ=%s", pCompressTaskData->stYSPB.jlbh, pCompressTaskData->strPlatformIP, pCompressTaskData->strZRVDeviceIP);

            /* ����״̬ */
            iRet = UpdateCompressTaskResultInfo(pCompressTaskData->stYSPB.jlbh, 3, 2, FILE_COMPRESS_TIMEOUT_ERROR, 0, 0, 0, NULL, pDbOper);
        }
    }

    COMPRESS_TASKIDVector.clear();

    return 0;
}
#endif

/*****************************************************************************
 �� �� ��  : GetAllProcCompressTaskByPlatformIP
 ��������  : ����ƽ̨IP��ַ��ȡ���ڴ����������Ϣ
 �������  : char* platform_ip
             vector<string>& CompressTaskID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��9��1��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GetAllProcCompressTaskByPlatformIP(char* platform_ip, vector<string>& CompressTaskID)
{
    compress_task_t* pCompressTaskData = NULL;
    COMPRESS_TASK_Iterator Itr;

    if (NULL == platform_ip || platform_ip[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetAllProcCompressTaskByPlatformIP() exit---: platform_ip NULL \r\n");
        return -1;
    }

    CompressTaskID.clear();
    COMPRESS_TASK_SMUTEX_LOCK();

    if (g_CompressTaskMap.size() <= 0)
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "GetAllProcCompressTaskByPlatformIP() exit---: g_CompressTaskMap NULL \r\n");
        return 0;
    }

    for (Itr = g_CompressTaskMap.begin(); Itr != g_CompressTaskMap.end(); Itr++)
    {
        pCompressTaskData = Itr->second;

        if (NULL == pCompressTaskData)
        {
            continue;
        }

        if (pCompressTaskData->stYSPB.jlbh[0] == '\0')
        {
            continue;
        }

        if (pCompressTaskData->strPlatformIP[0] == '\0')
        {
            continue;
        }

        if (pCompressTaskData->iTaskStatus != 0
            && pCompressTaskData->iTaskStatus != 1)
        {
            continue;
        }

        if (0 == sstrcmp(pCompressTaskData->strPlatformIP, platform_ip))
        {
            CompressTaskID.push_back(pCompressTaskData->stYSPB.jlbh);
        }
    }

    COMPRESS_TASK_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 �� �� ��  : GetNeedAssignCompressTaskByPlatformIP
 ��������  : ����ƽ̨IP��ַ��ȡ��Ҫ�����������Ϣ
 �������  : char* platform_ip
             vector<string>& CompressTaskID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��9��1��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GetNeedAssignCompressTaskByPlatformIP(char* platform_ip, vector<string>& CompressTaskID)
{
    compress_task_t* pCompressTaskData = NULL;
    COMPRESS_TASK_Iterator Itr;

    if (NULL == platform_ip || platform_ip[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetNeedAssignCompressTaskByPlatformIP() exit---: platform_ip NULL \r\n");
        return -1;
    }

    CompressTaskID.clear();
    COMPRESS_TASK_SMUTEX_LOCK();

    if (g_CompressTaskMap.size() <= 0)
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "GetNeedAssignCompressTaskByPlatformIP() exit---: g_CompressTaskMap NULL \r\n");
        return 0;
    }

    for (Itr = g_CompressTaskMap.begin(); Itr != g_CompressTaskMap.end(); Itr++)
    {
        pCompressTaskData = Itr->second;

        if (NULL == pCompressTaskData)
        {
            continue;
        }

        if (pCompressTaskData->stYSPB.jlbh[0] == '\0')
        {
            continue;
        }

        if (pCompressTaskData->strPlatformIP[0] == '\0')
        {
            continue;
        }

        if (pCompressTaskData->iTaskStatus != 0)
        {
            continue;
        }

        if (0 == sstrcmp(pCompressTaskData->strPlatformIP, platform_ip))
        {
            CompressTaskID.push_back(pCompressTaskData->stYSPB.jlbh);
        }
    }

    COMPRESS_TASK_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 �� �� ��  : GetAllAssignedCompressTaskByZRVDeviceIP
 ��������  : ����ZRV�豸��IP��ַ��ȡ�Ѿ������������
 �������  : char* device_ip
             vector<string>& CompressTaskID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��9��1��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GetAllAssignedCompressTaskByZRVDeviceIP(char* device_ip, vector<string>& CompressTaskID)
{
    compress_task_t* pCompressTaskData = NULL;
    COMPRESS_TASK_Iterator Itr;

    if (NULL == device_ip || device_ip[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "GetAllAssignedCompressTaskByZRVDeviceIP() exit---: device_ip NULL \r\n");
        return -1;
    }

    CompressTaskID.clear();
    COMPRESS_TASK_SMUTEX_LOCK();

    if (g_CompressTaskMap.size() <= 0)
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "GetAllAssignedCompressTaskByZRVDeviceIP() exit---: g_CompressTaskMap NULL \r\n");
        return 0;
    }

    for (Itr = g_CompressTaskMap.begin(); Itr != g_CompressTaskMap.end(); Itr++)
    {
        pCompressTaskData = Itr->second;

        if (NULL == pCompressTaskData)
        {
            continue;
        }

        if (pCompressTaskData->stYSPB.jlbh[0] == '\0')
        {
            continue;
        }

        if (pCompressTaskData->strPlatformIP[0] == '\0')
        {
            continue;
        }

        if (pCompressTaskData->iTaskStatus != 1)
        {
            continue;
        }

        if (0 == sstrcmp(pCompressTaskData->strZRVDeviceIP, device_ip))
        {
            CompressTaskID.push_back(pCompressTaskData->stYSPB.jlbh);
        }
    }

    COMPRESS_TASK_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 �� �� ��  : GetCurrentCompressTaskCountByZRVDeviceIP
 ��������  : ����ZRV�豸��IP��ַ��ȡ��ǰѹ����������
 �������  : char* platform_ip
             char* zrv_device_ip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��9��1��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int GetCurrentCompressTaskCountByZRVDeviceIP(char* platform_ip, char* zrv_device_ip)
{
    int task_count = 0;
    compress_task_t* pCompressTaskData = NULL;
    COMPRESS_TASK_Iterator Itr;

    if (NULL == platform_ip || platform_ip[0] == '\0')
    {
        return 0;
    }

    if (NULL == zrv_device_ip || zrv_device_ip[0] == '\0')
    {
        return 0;
    }

    COMPRESS_TASK_SMUTEX_LOCK();

    if (g_CompressTaskMap.size() <= 0)
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "GetCurrentCompressTaskCountByZRVDeviceIP() exit---: Call Record Srv Map NULL \r\n");
        return 0;
    }

    for (Itr = g_CompressTaskMap.begin(); Itr != g_CompressTaskMap.end(); Itr++)
    {
        pCompressTaskData = Itr->second;

        if (NULL == pCompressTaskData)
        {
            continue;
        }

        if (pCompressTaskData->stYSPB.jlbh[0] == '\0')
        {
            continue;
        }

        if (pCompressTaskData->strPlatformIP[0] == '\0')
        {
            continue;
        }

        if (pCompressTaskData->iTaskStatus != 1)
        {
            continue;
        }

        if (0 == sstrcmp(pCompressTaskData->strPlatformIP, platform_ip)
            && 0 == sstrcmp(pCompressTaskData->strZRVDeviceIP, zrv_device_ip))
        {
            task_count++;
        }
    }

    COMPRESS_TASK_SMUTEX_UNLOCK();

    return task_count;
}

/*****************************************************************************
 �� �� ��  : HasCompressTaskNotComplete
 ��������  : �Ƿ���û�д������ѹ������
 �������  : char* platform_ip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��8��21��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int HasCompressTaskNotComplete(char* platform_ip)
{
    int iRet = 0;
    compress_task_t* pCompressTaskData = NULL;
    COMPRESS_TASK_Iterator Itr;

    if (NULL == platform_ip || platform_ip[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "HasCompressTaskNotComplete() exit---: platform_ip Error \r\n");
        return 0;
    }

    COMPRESS_TASK_SMUTEX_LOCK();

    if (g_CompressTaskMap.size() <= 0)
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "scan_compress_task_list_for_wait_expire() exit---: Call Record Srv Map NULL \r\n");
        return 0;
    }

    for (Itr = g_CompressTaskMap.begin(); Itr != g_CompressTaskMap.end(); Itr++)
    {
        pCompressTaskData = Itr->second;

        if (NULL == pCompressTaskData)
        {
            continue;
        }

        if (pCompressTaskData->stYSPB.jlbh[0] == '\0')
        {
            continue;
        }

        if (pCompressTaskData->strPlatformIP[0] == '\0')
        {
            continue;
        }

        if (0 == sstrcmp(pCompressTaskData->strPlatformIP, platform_ip))
        {
            COMPRESS_TASK_SMUTEX_UNLOCK();
            return 1;
        }
    }

    COMPRESS_TASK_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 �� �� ��  : HasCompressTaskNotComplete2
 ��������  : �Ƿ���û�д�����ĵȴ�ZRV�����ѹ������
 �������  : char* platform_ip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��8��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int HasCompressTaskNotComplete2(char* platform_ip)
{
    int iRet = 0;
    compress_task_t* pCompressTaskData = NULL;
    COMPRESS_TASK_Iterator Itr;

    if (NULL == platform_ip || platform_ip[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "HasCompressTaskNotComplete() exit---: platform_ip Error \r\n");
        return 0;
    }

    COMPRESS_TASK_SMUTEX_LOCK();

    if (g_CompressTaskMap.size() <= 0)
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "scan_compress_task_list_for_wait_expire() exit---: Call Record Srv Map NULL \r\n");
        return 0;
    }

    for (Itr = g_CompressTaskMap.begin(); Itr != g_CompressTaskMap.end(); Itr++)
    {
        pCompressTaskData = Itr->second;

        if (NULL == pCompressTaskData)
        {
            continue;
        }

        if (pCompressTaskData->stYSPB.jlbh[0] == '\0')
        {
            continue;
        }

        if (pCompressTaskData->strPlatformIP[0] == '\0')
        {
            continue;
        }

        if (0 != sstrcmp(pCompressTaskData->strPlatformIP, platform_ip))
        {
            continue;
        }

        if (1 == pCompressTaskData->iTaskStatus) /* �����ڵȴ�ѹ����������� */
        {
            COMPRESS_TASK_SMUTEX_UNLOCK();
            return 1;
        }
    }

    COMPRESS_TASK_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 �� �� ��  : DeleteCompressTaskByPlatformIP
 ��������  : ����ƽ̨IP��ַɾ��ѹ������
 �������  : char* platform_ip
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��6��11��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int DeleteCompressTaskByPlatformIP(char* platform_ip)
{
    int iRet = 0;
    compress_task_t* pCompressTaskData = NULL;
    COMPRESS_TASK_Iterator Itr;
    int task_index = 0;
    vector<string> CompressTaskID;

    if (NULL == platform_ip || platform_ip[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "DeleteCompressTaskByPlatformIP() exit---: platform_ip Error \r\n");
        return -1;
    }

    CompressTaskID.clear();

    COMPRESS_TASK_SMUTEX_LOCK();

    if (g_CompressTaskMap.size() <= 0)
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "scan_compress_task_list_for_wait_expire() exit---: Call Record Srv Map NULL \r\n");
        return 0;
    }

    for (Itr = g_CompressTaskMap.begin(); Itr != g_CompressTaskMap.end(); Itr++)
    {
        pCompressTaskData = Itr->second;

        if (NULL == pCompressTaskData)
        {
            continue;
        }

        if (pCompressTaskData->stYSPB.jlbh[0] == '\0')
        {
            continue;
        }

        if (pCompressTaskData->strPlatformIP[0] == '\0')
        {
            continue;
        }

        if (0 == sstrcmp(pCompressTaskData->strPlatformIP, platform_ip))
        {
            CompressTaskID.push_back(pCompressTaskData->stYSPB.jlbh);
        }
    }

    COMPRESS_TASK_SMUTEX_UNLOCK();

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "DeleteCompressTaskByPlatformIP() CompressTaskID.size()=%d \r\n", CompressTaskID.size());

    for (task_index = 0; task_index < CompressTaskID.size(); task_index++)
    {
        iRet = compress_task_remove((char*)CompressTaskID[task_index].c_str());
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "DeleteCompressTaskByPlatformIP() compress_task_remove: task_index=%d, RecordNumber=%s, iRet=%d \r\n", task_index, (char*)CompressTaskID[task_index].c_str(), iRet);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : DeleteCompressTaskFromDBByPlatformIP
 ��������  : ����ƽ̨IP��ַ�����ݿ�ɾ��ѹ������
 �������  : char* platform_ip
             DBOper* pDbOper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��6��11��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int DeleteCompressTaskFromDBByPlatformIP(char* platform_ip, DBOper* pDbOper)
{
    int iRet = 0;
    int record_count = -1;
    string strDeleteSQL = "";

    char strStatus[32] = {0};
    char strResult[32] = {0};

    if (NULL == platform_ip || platform_ip[0] == '\0' || NULL == pDbOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "DeleteCompressTaskFromDBByPlatformIP() exit---:  Param Error \r\n");
        return -1;
    }

    strDeleteSQL.clear();
    strDeleteSQL = "DELETE FROM ZRVCompressTaskAssignInfo";

    strDeleteSQL += " WHERE PlatformIP like '";
    strDeleteSQL += platform_ip;
    strDeleteSQL += "'";

    iRet = pDbOper->DB_Delete(strDeleteSQL.c_str(), 1);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "DeleteCompressTaskFromDBByPlatformIP() : ZRVCompressTaskAssignInfo:platform_ip=%s, iRet=%d \r\n", platform_ip, iRet);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "DeleteCompressTaskFromDBByPlatformIP() DB Oper Error:strDeleteSQL=%s, iRet=%d \r\n", strDeleteSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "DeleteCompressTaskFromDBByPlatformIP() ErrorMsg=%s\r\n", pDbOper->GetLastDbErrorMsg());
        return -1;
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : DeleteCompressTask
 ��������  : ɾ��ѹ������
 �������  : char* platform_ip
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��6��11��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int DeleteCompressTask(char* platform_ip, DBOper* ptDBoper)
{
    int iRet = 0;

    iRet = DeleteCompressTaskByPlatformIP(platform_ip);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "DeleteCompressTask() : DeleteCompressTaskByPlatformIP:platform_ip=%s, iRet=%d \r\n", platform_ip, iRet);

    iRet = DeleteCompressTaskFromDBByPlatformIP(platform_ip, ptDBoper);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "DeleteCompressTask() : DeleteCompressTaskFromDBByPlatformIP:platform_ip=%s, iRet=%d \r\n", platform_ip, iRet);

    return 0;
}

/*****************************************************************************
 �� �� ��  : AddCompressTaskToAssignDB
 ��������  : ���ѹ���������ݿ�
 �������  : compress_task_t* compress_task
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��6��11��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddCompressTaskToAssignDB(compress_task_t* compress_task, DBOper* ptDBoper)
{
    int iRet = 0;
    int record_count = -1;
    string strQuerySQL = "";
    string strInsertSQL = "";
    string strUpdateSQL = "";
    char strFileSize[32] = {0};
    char strUploadTime[32] = {0};
    char strTaskCreateTime[32] = {0};

    if (NULL == compress_task || NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "AddCompressTaskToAssignDB() exit---:  Param Error \r\n");
        return -1;
    }

    if (compress_task->stYSPB.jlbh[0] == '\0' || compress_task->strPlatformIP[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "AddCompressTaskToAssignDB() exit---: RecordNum Or PlatformIP NULL \r\n");
        return -1;
    }

    snprintf(strFileSize, 32, "%d", compress_task->stYSPB.wjdx);
    snprintf(strUploadTime, 32, "%d", compress_task->stYSPB.scsj);
    format_time(compress_task->iTaskCreateTime, strTaskCreateTime);

    /* 1������������:�Ȳ�ѯ�����Ƿ���� */
    strQuerySQL.clear();
    strQuerySQL = "select * from ZRVCompressTaskAssignInfo WHERE RecordNum like '";
    strQuerySQL += compress_task->stYSPB.jlbh;
    strQuerySQL += "'";

    record_count = ptDBoper->DB_Select(strQuerySQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToAssignDB() DB Oper Error:strQuerySQL=%s, record_count=%d \r\n", strQuerySQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToAssignDB() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����������Ϣ�����ݿ������ZRVCompressTaskAssignInfo��ʧ��, ��ѯ���ݿ����ʧ��:��¼���=%s", compress_task->stYSPB.jlbh);
        return -1;
    }
    else if (record_count == 0)
    {
        strInsertSQL.clear();
        strInsertSQL = "insert into ZRVCompressTaskAssignInfo (RecordNum,FileName,FileSuffixName,FileSize,UploadUnit,UploadTime,StoragePath,YSHStoragePath,PlatformIP,TaskCreateTime) values (";

        strInsertSQL += "'";
        strInsertSQL += compress_task->stYSPB.jlbh;
        strInsertSQL += "'";
        strInsertSQL += ",";

        strInsertSQL += "'";
        strInsertSQL += compress_task->stYSPB.wjmc;
        strInsertSQL += "'";
        strInsertSQL += ",";

        strInsertSQL += "'";
        strInsertSQL += compress_task->stYSPB.kzm;
        strInsertSQL += "'";
        strInsertSQL += ",";

        strInsertSQL += strFileSize;
        strInsertSQL += ",";

        strInsertSQL += "'";
        strInsertSQL += compress_task->stYSPB.scdw;
        strInsertSQL += "'";
        strInsertSQL += ",";

        strInsertSQL += strUploadTime;
        strInsertSQL += ",";

        strInsertSQL += "'";
        strInsertSQL += compress_task->stYSPB.cclj;
        strInsertSQL += "'";
        strInsertSQL += ",";

        strInsertSQL += "'";
        strInsertSQL += compress_task->stYSPB.yshlj;
        strInsertSQL += "'";
        strInsertSQL += ",";

        strInsertSQL += "'";
        strInsertSQL += compress_task->strPlatformIP;
        strInsertSQL += "'";

        strInsertSQL += ",";
        strInsertSQL += "'";
        strInsertSQL += strTaskCreateTime;
        strInsertSQL += "'";

        strInsertSQL += ")";

        iRet = ptDBoper->DB_Insert("", "", strInsertSQL.c_str(), 1);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "AddCompressTaskToAssignDB() DB_Insert:InsertSQL=%s, iRet=%d\r\n", (char*)strInsertSQL.c_str(), iRet);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToAssignDB() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToAssignDB() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����������Ϣ�����ݿ������ZRVCompressTaskAssignInfo��ʧ��, �������ݿ����ʧ��:��¼���=%s", compress_task->stYSPB.jlbh);
        }
        else if (iRet == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����������Ϣ�����ݿ������ZRVCompressTaskAssignInfo��ʧ��, δ�ҵ���Ӧ�����ݿ��¼:��¼���=%s", compress_task->stYSPB.jlbh);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������Ϣ�����ݿ������ZRVCompressTaskAssignInfo�гɹ�:��¼���=%s", compress_task->stYSPB.jlbh);
        }
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "��ӵ�ѹ��������Ϣ�Ѿ�������ZRVCompressTaskAssignInfo���ݿ�, ��¼���=%s", compress_task->stYSPB.jlbh);
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "AddCompressTaskToAssignDB() compress_task->stYSPB.jlbh=%s has exist in ZRVCompressTaskAssignInfo db\r\n", compress_task->stYSPB.jlbh);

        /* ����ѹ�� */
        if (0 == compress_task->iTaskStatus || 0 == compress_task->iTaskResult)
        {
            strUpdateSQL.clear();
            strUpdateSQL = "UPDATE ZRVCompressTaskAssignInfo SET";

            strUpdateSQL += " AssignFlag = 0";
            strUpdateSQL += ",";

            strUpdateSQL += " ZRVDeviceIP = ''";
            strUpdateSQL += ",";

            strUpdateSQL += " TaskStatus = 0";
            strUpdateSQL += ",";

            strUpdateSQL += " TaskResult = 0";
            strUpdateSQL += ",";

            strUpdateSQL += " ErrorCode = 0";
            strUpdateSQL += ",";

            strUpdateSQL += " YSHFileSize = 0";
            strUpdateSQL += ",";

            strUpdateSQL += " YSHStoragePath = ''";
            strUpdateSQL += ",";

            strUpdateSQL += " TaskCreateTime = ";
            strUpdateSQL += "'";
            strUpdateSQL += strTaskCreateTime;
            strUpdateSQL += "'";
            strUpdateSQL += ",";

            strUpdateSQL += " ZRVCompressBeginTime = 0";
            strUpdateSQL += ",";

            strUpdateSQL += " ZRVCompressEndTime = 0";
            strUpdateSQL += " WHERE RecordNum like '";
            strUpdateSQL += compress_task->stYSPB.jlbh;
            strUpdateSQL += "'";

            iRet = ptDBoper->DB_Update(strUpdateSQL.c_str(), 1);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToAssignDB() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToAssignDB() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����������Ϣ�����ݿ������ZRVCompressTaskAssignInfo��ʧ��, �������ݿ����ʧ��:��¼���=%s", compress_task->stYSPB.jlbh);
            }
            else if (iRet == 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����������Ϣ�����ݿ������ZRVCompressTaskAssignInfo��ʧ��, δ�ҵ���Ӧ�����ݿ��¼����û�пɸ��µ��ֶ�:��¼���=%s", compress_task->stYSPB.jlbh);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������Ϣ�����ݿ������ZRVCompressTaskAssignInfo�гɹ�:��¼���=%s", compress_task->stYSPB.jlbh);
            }
        }
    }

    return iRet;
}

int AddCompressTaskToDB(char* task_id, int iTaskStatus, int iTaskResult, int iErrorCode, int iCompressBeginTime, int iCompressEndTime, int iYSHFileSize, char* pcDestUrl, DBOper* ptDBoper)
{
    int iRet = 0;
    int record_count = -1;
    string strQuerySQL = "";
    string strInsertSQL = "";
    string strUpdateSQL = "";
    char strFileSize[32] = {0};
    char strUploadTime[32] = {0};

    char strStatus[32] = {0};
    char strResult[32] = {0};
    char strErrorCode[32] = {0};
    char strYSHFileSize[32] = {0};
    char strTaskCreateTime[32] = {0};
    char strCompressBeginTime[32] = {0};
    char strCompressEndTime[32] = {0};

    compress_task_t* compress_task = NULL;

    if (NULL == task_id || NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToDB() exit---:  Param Error \r\n");
        return -1;
    }

    compress_task = compress_task_find(task_id);

    if (NULL == compress_task)
    {
        /* ������֮ǰ�Ѿ����������ݿ�ļ�¼������һ�� */
        snprintf(strStatus, 32, "%d", iTaskStatus);
        snprintf(strResult, 32, "%d", iTaskResult);
        snprintf(strErrorCode, 32, "%d", iErrorCode);
        snprintf(strYSHFileSize, 32, "%d", iYSHFileSize);

        if (iCompressBeginTime > 0)
        {
            snprintf(strCompressBeginTime, 32, "%d", iCompressBeginTime);
        }
        else
        {
            snprintf(strCompressBeginTime, 32, "%d", 0);
        }

        if (iCompressEndTime > 0)
        {
            snprintf(strCompressEndTime, 32, "%d", iCompressEndTime);
        }
        else
        {
            snprintf(strCompressEndTime, 32, "%d", 0);
        }

        strUpdateSQL.clear();
        strUpdateSQL = "UPDATE ZRVCompressTaskInfo SET";

        strUpdateSQL += " TaskStatus = ";
        strUpdateSQL += strStatus;
        strUpdateSQL += ",";

        strUpdateSQL += " TaskResult = ";
        strUpdateSQL += strResult;
        strUpdateSQL += ",";

        strUpdateSQL += " ErrorCode = ";
        strUpdateSQL += strErrorCode;
        strUpdateSQL += ",";

        strUpdateSQL += " YSHFileSize = ";
        strUpdateSQL += strYSHFileSize;

        if (NULL != pcDestUrl)
        {
            strUpdateSQL += ",";

            strUpdateSQL += " YSHStoragePath = '";
            strUpdateSQL += pcDestUrl;
            strUpdateSQL += "'";
        }

        if (iCompressBeginTime > 0)
        {
            strUpdateSQL += ",";

            strUpdateSQL += " ZRVCompressBeginTime = ";
            strUpdateSQL += strCompressBeginTime;
        }

        if (iCompressEndTime > 0)
        {
            strUpdateSQL += ",";

            strUpdateSQL += " ZRVCompressEndTime = ";
            strUpdateSQL += strCompressEndTime;
        }

        strUpdateSQL += " WHERE RecordNum like '";
        strUpdateSQL += task_id;
        strUpdateSQL += "'";

        iRet = ptDBoper->DB_Update(strUpdateSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToDB() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToDB() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����������Ϣ�����ݿ������ZRVCompressTaskInfo��ʧ��, �������ݿ����ʧ��:��¼���=%s", task_id);
        }
        else if (iRet == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����������Ϣ�����ݿ������ZRVCompressTaskInfo��ʧ��, δ�ҵ���Ӧ�����ݿ��¼����û�пɸ��µ��ֶ�:��¼���=%s", task_id);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������Ϣ�����ݿ������ZRVCompressTaskInfo�гɹ�:��¼���=%s", task_id);
        }
    }
    else
    {
        if (compress_task->stYSPB.jlbh[0] == '\0' || compress_task->strPlatformIP[0] == '\0')
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToDB() exit---: RecordNum Or PlatformIP NULL \r\n");
            return -1;
        }

        snprintf(strFileSize, 32, "%d", compress_task->stYSPB.wjdx);
        snprintf(strUploadTime, 32, "%d", compress_task->stYSPB.scsj);

        snprintf(strStatus, 32, "%d", compress_task->iTaskStatus);
        snprintf(strResult, 32, "%d", compress_task->iTaskResult);
        snprintf(strErrorCode, 32, "%d", iErrorCode);
        snprintf(strYSHFileSize, 32, "%d", compress_task->stYSPB.yshdx);
        format_time(compress_task->iTaskCreateTime, strTaskCreateTime);

        if (iCompressBeginTime > 0)
        {
            snprintf(strCompressBeginTime, 32, "%d", iCompressBeginTime);
        }
        else
        {
            snprintf(strCompressBeginTime, 32, "%d", 0);
        }

        if (iCompressEndTime > 0)
        {
            snprintf(strCompressEndTime, 32, "%d", iCompressEndTime);
        }
        else
        {
            snprintf(strCompressEndTime, 32, "%d", 0);
        }

        /* 1�������ܱ�:�Ȳ�ѯ�����Ƿ���� */
        strQuerySQL.clear();
        strQuerySQL = "select * from ZRVCompressTaskInfo WHERE RecordNum like '";
        strQuerySQL += compress_task->stYSPB.jlbh;
        strQuerySQL += "'";

        record_count = ptDBoper->DB_Select(strQuerySQL.c_str(), 1);

        if (record_count < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToDB() DB Oper Error:strQuerySQL=%s, record_count=%d \r\n", strQuerySQL.c_str(), record_count);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToDB() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����������Ϣ�����ݿ������ZRVCompressTaskInfo��ʧ��, ��ѯ���ݿ����ʧ��:��¼���=%s", task_id);
            return -1;
        }
        else if (record_count == 0)
        {
            strInsertSQL.clear();
            strInsertSQL = "insert into ZRVCompressTaskInfo (RecordNum,FileName,FileSuffixName,FileSize,UploadUnit,UploadTime,StoragePath,YSHStoragePath,PlatformIP,ZRVDeviceIP,TaskCreateTime,TaskStatus,TaskResult,ErrorCode,YSHFileSize,ZRVCompressBeginTime,ZRVCompressEndTime) values (";

            strInsertSQL += "'";
            strInsertSQL += compress_task->stYSPB.jlbh;
            strInsertSQL += "'";
            strInsertSQL += ",";

            strInsertSQL += "'";
            strInsertSQL += compress_task->stYSPB.wjmc;
            strInsertSQL += "'";
            strInsertSQL += ",";

            strInsertSQL += "'";
            strInsertSQL += compress_task->stYSPB.kzm;
            strInsertSQL += "'";
            strInsertSQL += ",";

            strInsertSQL += strFileSize;
            strInsertSQL += ",";

            strInsertSQL += "'";
            strInsertSQL += compress_task->stYSPB.scdw;
            strInsertSQL += "'";
            strInsertSQL += ",";

            strInsertSQL += strUploadTime;
            strInsertSQL += ",";

            strInsertSQL += "'";
            strInsertSQL += compress_task->stYSPB.cclj;
            strInsertSQL += "'";
            strInsertSQL += ",";

            strInsertSQL += "'";
            strInsertSQL += compress_task->stYSPB.yshlj;
            strInsertSQL += "'";

            strInsertSQL += ",";

            strInsertSQL += "'";
            strInsertSQL += compress_task->strPlatformIP;
            strInsertSQL += "'";

            strInsertSQL += ",";

            strInsertSQL += "'";
            strInsertSQL += compress_task->strZRVDeviceIP;
            strInsertSQL += "'";

            strInsertSQL += ",";

            strInsertSQL += "'";
            strInsertSQL += strTaskCreateTime;
            strInsertSQL += "'";

            strInsertSQL += ",";

            strInsertSQL += strStatus;
            strInsertSQL += ",";

            strInsertSQL += strResult;
            strInsertSQL += ",";

            strInsertSQL += strErrorCode;
            strInsertSQL += ",";

            strInsertSQL += strYSHFileSize;
            strInsertSQL += ",";

            strInsertSQL += strCompressBeginTime;
            strInsertSQL += ",";

            strInsertSQL += strCompressEndTime;

            strInsertSQL += ")";

            iRet = ptDBoper->DB_Insert("", "", strInsertSQL.c_str(), 1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "AddCompressTaskToDB() DB_Insert:InsertSQL=%s, iRet=%d\r\n", (char*)strInsertSQL.c_str(), iRet);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToDB() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToDB() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����������Ϣ�����ݿ������ZRVCompressTaskInfo��ʧ��, �������ݿ����ʧ��:��¼���=%s", task_id);
            }
            else if (iRet == 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����������Ϣ�����ݿ������ZRVCompressTaskInfo��ʧ��, δ�ҵ���Ӧ�����ݿ��¼:��¼���=%s", task_id);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������Ϣ�����ݿ������ZRVCompressTaskInfo�гɹ�:��¼���=%s", task_id);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "��ӵ�ѹ��������Ϣ�Ѿ�������ZRVCompressTaskInfo���ݿ����, ��¼���=%s", compress_task->stYSPB.jlbh);
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "AddCompressTaskToDB() compress_task->stYSPB.jlbh=%s has exist in ZRVCompressTaskInfo db\r\n", compress_task->stYSPB.jlbh);

            strUpdateSQL.clear();
            strUpdateSQL = "UPDATE ZRVCompressTaskInfo SET";

            strUpdateSQL += " TaskStatus = ";
            strUpdateSQL += strStatus;
            strUpdateSQL += ",";

            strUpdateSQL += " TaskResult = ";
            strUpdateSQL += strResult;
            strUpdateSQL += ",";

            strUpdateSQL += " ErrorCode = ";
            strUpdateSQL += strErrorCode;
            strUpdateSQL += ",";

            strUpdateSQL += " YSHFileSize = ";
            strUpdateSQL += strYSHFileSize;

            if (compress_task->stYSPB.yshlj[0] != '\0')
            {
                strUpdateSQL += ",";

                strUpdateSQL += " YSHStoragePath = '";
                strUpdateSQL += compress_task->stYSPB.yshlj;
                strUpdateSQL += "'";
            }

            if (compress_task->iTaskCreateTime > 0)
            {
                strUpdateSQL += ",";

                strUpdateSQL += " TaskCreateTime = ";
                strUpdateSQL += "'";
                strUpdateSQL += strTaskCreateTime;
                strUpdateSQL += "'";
            }

            if (iCompressBeginTime > 0)
            {
                strUpdateSQL += ",";

                strUpdateSQL += " ZRVCompressBeginTime = ";
                strUpdateSQL += strCompressBeginTime;
            }

            if (iCompressEndTime > 0)
            {
                strUpdateSQL += ",";

                strUpdateSQL += " ZRVCompressEndTime = ";
                strUpdateSQL += strCompressEndTime;
            }

            strUpdateSQL += " WHERE RecordNum like '";
            strUpdateSQL += task_id;
            strUpdateSQL += "'";

            iRet = ptDBoper->DB_Update(strUpdateSQL.c_str(), 1);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToDB() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToDB() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����������Ϣ�����ݿ������ZRVCompressTaskInfo��ʧ��, �������ݿ����ʧ��:��¼���=%s", task_id);
            }
            else if (iRet == 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����������Ϣ�����ݿ������ZRVCompressTaskInfo��ʧ��, δ�ҵ���Ӧ�����ݿ��¼����û�пɸ��µ��ֶ�:��¼���=%s", task_id);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����������Ϣ�����ݿ������ZRVCompressTaskInfo�гɹ�:��¼���=%s", task_id);
            }
        }
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : AddCompressTask
 ��������  : ���ѹ������
 �������  : char* platform_ip
             jly_yspb_t* pstYSPB
             DBOper* ptDBoper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��6��11��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddCompressTask(char* platform_ip, jly_yspb_t* pstYSPB, DBOper* ptDBoper)
{
    int iRet = 0;
    compress_task_t* compress_task = NULL;
    //char guid[37] = {0};
    time_t now = time(NULL);

    if (NULL == platform_ip || NULL == pstYSPB || NULL == ptDBoper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "AddCompressTask() exit---:  Param Error \r\n");
        return -1;
    }

    if (platform_ip[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "AddCompressTask() exit---: platform_ip NULL \r\n");
        return -1;
    }

    /* �Ȳ��� */
    compress_task = compress_task_find(pstYSPB->jlbh);

    if (NULL == compress_task)
    {
        iRet = compress_task_init(&compress_task);

        if (iRet != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTask() compress_task_init Error");
            return -1;
        }

        /* ����GUID */
        //random_uuid(guid);
        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "AddCompressTask() random_uuid: guid=%s", guid);
        //osip_strncpy(compress_task->strRecordNum, guid, 36);

        memcpy(&(compress_task->stYSPB), pstYSPB, sizeof(jly_yspb_t));

        compress_task->iAssignFlag = 0;
        osip_strncpy(compress_task->strPlatformIP, platform_ip, MAX_IP_LEN);
        compress_task->iTaskStatus = 0;
        compress_task->iTaskResult = 0;
        compress_task->wait_answer_expire = 0;
        compress_task->resend_count = 0;
        compress_task->iTaskCreateTime = now;

        // TODO:  ��Ҫȷ��ѹ����Ĵ洢·��
        //compress_task->stYSPB.yshlj;

        if (compress_task_add(compress_task) < 0)
        {
            compress_task_free(compress_task);
            osip_free(compress_task);
            compress_task = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTask() compress_task_add Error");
            return -1;
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ѹ��������Ϣ�ɹ�, �����¼���=%s", compress_task->stYSPB.jlbh);
    }
    else
    {
        if (0 == compress_task->iTaskStatus) /* ��ʼ״̬ */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "��ӵ�ѹ��������Ϣ�Ѿ�����,���ǻ�û�п�ʼ����ѹ������, ���¿�ʼѹ��, �����¼���=%s", compress_task->strZRVDeviceIP, compress_task->stYSPB.jlbh);
        }
        else if (1 == compress_task->iTaskStatus) /* ���ڽ���ѹ�� */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "��ӵ�ѹ��������Ϣ�Ѿ�����,�����������ڵȴ�ZRV=%s�豸����������, ���¿�ʼѹ��, �����¼���=%s", compress_task->strZRVDeviceIP, compress_task->stYSPB.jlbh);
        }
        else /* ѹ������� */
        {
            if (1 == compress_task->iTaskResult) /* ѹ���ɹ� */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "��ӵ�ѹ��������Ϣ�Ѿ�����,�Ѿ�ѹ���ɹ�, ZRV=%s, ���¿�ʼѹ��, �����¼���=%s", compress_task->strZRVDeviceIP, compress_task->stYSPB.jlbh);
            }
            else if (2 == compress_task->iTaskResult) /* ʧ�ܵ�����ѹ�� */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "��ӵ�ѹ��������Ϣ�Ѿ�����, ֮ǰѹ��ʧ��, ZRV=%s, ���¿�ʼѹ��, �����¼���=%s", compress_task->strZRVDeviceIP, compress_task->stYSPB.jlbh);
            }
        }

        /* ����ѹ�� */
        compress_task->iAssignFlag = 0;
        memset(compress_task->strZRVDeviceIP, 0, MAX_IP_LEN);
        compress_task->iTaskStatus = 0;
        compress_task->iTaskResult = 0;
        compress_task->wait_answer_expire = 0;
        compress_task->resend_count = 0;
        compress_task->iTaskCreateTime = now;
    }

    /* ���ӵ����ݿ���ʱ�� */
    iRet = AddCompressTaskToAssignDB(compress_task, ptDBoper);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "AddCompressTask() AddCompressTaskToAssignDB: iRet=%d\r\n", iRet);

    return 0;
}

/*****************************************************************************
 �� �� ��  : UpdateCompressTaskAssignInfo
 ��������  : ����ѹ��������Ϣ
 �������  : char* task_id
             int iAssignFlag
             char* zrv_device_ip
             DBOper* pDbOper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��8��23��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int UpdateCompressTaskAssignInfo(char* task_id, int iAssignFlag, char* zrv_device_ip, DBOper* pDbOper)
{
    int iRet = 0;
    compress_task_t* pCompressTaskData = NULL;

    if (NULL == task_id || NULL == pDbOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateCompressTaskAssignInfo() exit---:  Param Error \r\n");
        return -1;
    }

    if (task_id[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateCompressTaskAssignInfo() exit---: task_id NULL \r\n");
        return -1;
    }

    iRet = compress_task_set_task_assign_info(task_id, iAssignFlag, zrv_device_ip);

    if (0 != iRet)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateCompressTaskAssignInfo() exit---: compress_task_set_task_assign_info Error \r\n");
    }

    iRet = UpdateCompressTaskAssignInfoToDB(task_id, iAssignFlag, zrv_device_ip, pDbOper);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "UpdateCompressTaskAssignInfo() exit---: UpdateCompressTaskAssignInfoToDB:task_id=%s, iRet=%d \r\n", task_id, iRet);

    return 0;
}

/*****************************************************************************
 �� �� ��  : UpdateCompressTaskAssignInfoToDB
 ��������  : ����ѹ�������ʶ�����ݿ�
 �������  : char* task_id
             int iAssignFlag
             char* zrv_device_ip
             DBOper* pDbOper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��8��3��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int UpdateCompressTaskAssignInfoToDB(char* task_id, int iAssignFlag, char* zrv_device_ip, DBOper* pDbOper)
{
    int iRet = 0;
    string strUpdateSQL = "";
    char strAssignFlag[32] = {0};

    if (NULL == task_id || NULL == pDbOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "UpdateCompressTaskAssignInfoToDB() exit---:  Param Error \r\n");
        return -1;
    }

    if (task_id[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "UpdateCompressTaskAssignInfoToDB() exit---:  task_id NULL \r\n");
        return -1;
    }

    snprintf(strAssignFlag, 32, "%d", iAssignFlag);

    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE ZRVCompressTaskAssignInfo SET";

    strUpdateSQL += " AssignFlag = ";
    strUpdateSQL += strAssignFlag;
    strUpdateSQL += ",";

    strUpdateSQL += " ZRVDeviceIP = '";
    strUpdateSQL += zrv_device_ip;
    strUpdateSQL += "'";

    strUpdateSQL += " WHERE RecordNum like '";
    strUpdateSQL += task_id;
    strUpdateSQL += "'";

    iRet = pDbOper->DB_Update(strUpdateSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateCompressTaskAssignInfoToDB() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateCompressTaskAssignInfoToDB() ErrorMsg=%s\r\n", pDbOper->GetLastDbErrorMsg());
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ѹ������ZRV������Ϣ�����ݿ��ZRVCompressTaskAssignInfoʧ��, �������ݿ����ʧ��:��¼���=%s, AssignFlag=%d, zrv_device_ip=%s", task_id, iAssignFlag, zrv_device_ip);
    }
    else if (iRet == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����ѹ������ZRV������Ϣ�����ݿ��ZRVCompressTaskAssignInfoʧ��, δ�ҵ���Ӧ�����ݿ��¼����û�пɸ��µ��ֶ�:��¼���=%s, AssignFlag=%d, zrv_device_ip=%s", task_id, iAssignFlag, zrv_device_ip);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ѹ������ZRV������Ϣ�����ݿ��ZRVCompressTaskAssignInfo�ɹ�:��¼���=%s, AssignFlag=%d, zrv_device_ip=%s", task_id, iAssignFlag, zrv_device_ip);
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : UpdateCompressTaskResultInfo
 ��������  : ����ѹ�������Ϣ
 �������  : char* task_id
             int iTaskStatus
             int iTaskResult
             int iErrorCode
             int iCompressBeginTime
             int iCompressEndTime
             int iYSHFileSize
             char* pcDestUrl
             DBOper* pDbOper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��8��23��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int UpdateCompressTaskResultInfo(char* task_id, int iTaskStatus, int iTaskResult, int iErrorCode, int iCompressBeginTime, int iCompressEndTime, int iYSHFileSize, char* pcDestUrl, DBOper* pDbOper)
{
    int iRet = 0;
    string strResultXML = "";

    if (NULL == task_id || NULL == pDbOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateCompressTaskResultInfo() exit---:  Param Error \r\n");
        return -1;
    }

    if (task_id[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateCompressTaskResultInfo() exit---: task_id NULL \r\n");
        return -1;
    }

    iRet = compress_task_set_task_result_info(task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);

    if (0 != iRet)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateCompressTaskResultInfo() exit---: compress_task_set_task_result_info Error \r\n");
    }

    iRet = UpdateCompressTaskResultInfoToDB(task_id, iTaskStatus, iTaskResult, iErrorCode, iCompressBeginTime, iCompressEndTime, iYSHFileSize, pcDestUrl, pDbOper);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "UpdateCompressTaskResultInfo() exit---: UpdateCompressTaskResultInfoToDB:task_id=%s, iTaskStatus=%d, iTaskResult=%d, iRet=%d \r\n", task_id, iTaskStatus, iTaskResult, iRet);

    if (2 == iTaskStatus || 3 == iTaskStatus) /* ѹ����ɻ��߳�ʱ��ɾ����������Ϣ */
    {
        iRet = compress_task_remove(task_id);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "UpdateCompressTaskResultInfoToDB() compress_task_remove: RecordNum=%s, iRet=%d\r\n", task_id, iRet);

        if (iRet < 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "���ڴ���ɾ������ʧ��, û���ҵ���¼:��¼���=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
        }
        else if (iRet == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "���ڴ���ɾ�����ݳɹ�:��¼���=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
        }
    }

    /* ���ýӿڸ���Զ�� */
    if (1 == iTaskResult)
    {
        iRet = interface_updateObjectInfo(task_id, iYSHFileSize, pcDestUrl, strResultXML);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "compress_task_set_task_result_info: interface_updateObjectInfo :iRet=%d, Response XML msg=\r\n%s \r\n", iRet, (char*)strResultXML.c_str());

        if (0 != iRet)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ѹ����������Ϣʧ��, ����WebService�ӿڸ���ʧ��:��¼���=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : UpdateCompressTaskResultInfoToDB
 ��������  : ����ѹ����������ݿ�
 �������  : char* task_id
             int iTaskStatus
             int iTaskResult
             int iErrorCode
             int iCompressBeginTime
             int iCompressEndTime
             int iYSHFileSize
             char* pcDestUrl
             DBOper* pDbOper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��8��3��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int UpdateCompressTaskResultInfoToDB(char* task_id, int iTaskStatus, int iTaskResult, int iErrorCode, int iCompressBeginTime, int iCompressEndTime, int iYSHFileSize, char* pcDestUrl, DBOper* pDbOper)
{
    int iRet = 0;
    string strUpdateSQL = "";
    string strDeleteSQL = "";

    char strStatus[32] = {0};
    char strResult[32] = {0};
    char strErrorCode[32] = {0};
    char strYSHFileSize[32] = {0};
    char strCompressBeginTime[32] = {0};
    char strCompressEndTime[32] = {0};

    if (NULL == task_id || NULL == pDbOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "UpdateCompressTaskResultInfoToDB() exit---:  Param Error \r\n");
        return -1;
    }

    if (task_id[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "UpdateCompressTaskResultInfoToDB() exit---:  task_id NULL \r\n");
        return -1;
    }

    snprintf(strStatus, 32, "%d", iTaskStatus);
    snprintf(strResult, 32, "%d", iTaskResult);
    snprintf(strErrorCode, 32, "%d", iErrorCode);
    snprintf(strYSHFileSize, 32, "%d", iYSHFileSize);

    if (iCompressBeginTime > 0)
    {
        snprintf(strCompressBeginTime, 32, "%d", iCompressBeginTime);
    }
    else
    {
        snprintf(strCompressBeginTime, 32, "%d", 0);
    }

    if (iCompressEndTime > 0)
    {
        snprintf(strCompressEndTime, 32, "%d", iCompressEndTime);
    }
    else
    {
        snprintf(strCompressEndTime, 32, "%d", 0);
    }

    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE ZRVCompressTaskAssignInfo SET";

    strUpdateSQL += " TaskStatus = ";
    strUpdateSQL += strStatus;
    strUpdateSQL += ",";

    strUpdateSQL += " TaskResult = ";
    strUpdateSQL += strResult;
    strUpdateSQL += ",";

    strUpdateSQL += " ErrorCode = ";
    strUpdateSQL += strErrorCode;
    strUpdateSQL += ",";

    strUpdateSQL += " YSHFileSize = ";
    strUpdateSQL += strYSHFileSize;

    if (NULL != pcDestUrl)
    {
        strUpdateSQL += ",";

        strUpdateSQL += " YSHStoragePath = '";
        strUpdateSQL += pcDestUrl;
        strUpdateSQL += "'";
    }

    if (iCompressBeginTime > 0)
    {
        strUpdateSQL += ",";

        strUpdateSQL += " ZRVCompressBeginTime = ";
        strUpdateSQL += strCompressBeginTime;
    }

    if (iCompressEndTime > 0)
    {
        strUpdateSQL += ",";

        strUpdateSQL += " ZRVCompressEndTime = ";
        strUpdateSQL += strCompressEndTime;
    }

    strUpdateSQL += " WHERE RecordNum like '";
    strUpdateSQL += task_id;
    strUpdateSQL += "'";

    iRet = pDbOper->DB_Update(strUpdateSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateCompressTaskResultInfoToDB() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateCompressTaskResultInfoToDB() ErrorMsg=%s\r\n", pDbOper->GetLastDbErrorMsg());
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "����ѹ����������Ϣ��Ϣ�����ݿ��ZRVCompressTaskAssignInfoʧ��, �������ݿ����ʧ��:��¼���=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
    }
    else if (iRet == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "����ѹ����������Ϣ��Ϣ�����ݿ��ZRVCompressTaskAssignInfoʧ��, δ�ҵ���Ӧ�����ݿ��¼����û�пɸ��µ��ֶ�:��¼���=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "����ѹ����������Ϣ��Ϣ�����ݿ��ZRVCompressTaskAssignInfo�ɹ�:��¼���=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
    }

    if (2 == iTaskStatus || 3 == iTaskStatus) /* ѹ����ɻ��߳�ʱ��ɾ������������������ */
    {
        strDeleteSQL.clear();
        strDeleteSQL = "DELETE FROM ZRVCompressTaskAssignInfo";

        strDeleteSQL += " WHERE RecordNum like '";
        strDeleteSQL += task_id;
        strDeleteSQL += "'";

        iRet = pDbOper->DB_Delete(strDeleteSQL.c_str(), 1);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "UpdateCompressTaskResultInfoToDB() :DB_Delete:RecordNum=%s, iRet=%d \r\n", task_id, iRet);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateCompressTaskResultInfoToDB() DB Oper Error:strDeleteSQL=%s, iRet=%d \r\n", strDeleteSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "UpdateCompressTaskResultInfoToDB() ErrorMsg=%s\r\n", pDbOper->GetLastDbErrorMsg());
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "�����ݿ���������ZRVCompressTaskAssignInfo��ɾ������ʧ��, ɾ�����ݿ����ʧ��:��¼���=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
        }
        else if (iRet == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "�����ݿ���������ZRVCompressTaskAssignInfo��ɾ������ʧ��, δ�ҵ���Ӧ�����ݿ��¼:��¼���=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "�����ݿ���������ZRVCompressTaskAssignInfo��ɾ�����ݳɹ�:��¼���=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
        }

        /* ���ӵ������ܱ� */
        iRet = AddCompressTaskToDB(task_id, iTaskStatus, iTaskResult, iErrorCode, iCompressBeginTime, iCompressEndTime, iYSHFileSize, pcDestUrl, pDbOper);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "UpdateCompressTaskResultInfoToDB() :AddCompressTaskToDB:RecordNum=%s, iRet=%d \r\n", task_id, iRet);
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : get_complete_compress_task_from_assign_table
 ��������  : �ӷ�����л�ȡ�Ѿ���ɵ�����
 �������  : vector<string>& RecordNumVector
             vector<int>& ErrorCodeVector
             vector<int>& CompressBeginTimeVector
             vector<int>& CompressEndTimeVector
             DBOper* pDbOper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��8��23��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_complete_compress_task_from_assign_table(vector<string>& RecordNumVector, vector<int>& ErrorCodeVector, vector<int>& CompressBeginTimeVector, vector<int>& CompressEndTimeVector, DBOper* pDbOper)
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;

    if (NULL == pDbOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "get_complete_compress_task_from_assign_table() exit---:  Param Error \r\n");
        return -1;
    }

    RecordNumVector.clear();
    ErrorCodeVector.clear();
    CompressBeginTimeVector.clear();
    CompressEndTimeVector.clear();

    strSQL.clear();
    strSQL = "select * from ZRVCompressTaskAssignInfo WHERE TaskStatus=2 OR TaskStatus=3";

    record_count = pDbOper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_complete_compress_task_from_assign_table() Load compress task info: count=%d", record_count);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "get_complete_compress_task_from_assign_table() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "get_complete_compress_task_from_assign_table() ErrorMsg=%s\r\n", pDbOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "get_complete_compress_task_from_assign_table() exit---: No Record Count:strSQL=%s \r\n", strSQL.c_str());
        return 0;
    }

    record_count = 0;

    do
    {
        record_count++;
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "get_complete_compress_task_from_assign_table() Load route info: count=%d", record_count);

        compress_task_t* compress_task = NULL;
        int i_ret = compress_task_init(&compress_task);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "get_complete_compress_task_from_assign_table() route_info_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        unsigned int tmp_ivalue = 0;
        string tmp_svalue;
        string strRecordNum;
        string strTaskCreateTime;
        int iErrorCode = 0;
        int iTaskCreateTime = 0;
        int iZRVCompressBeginTime = 0;
        int iZRVCompressEndTime = 0;
        time_t now = time(NULL);

        /* ��¼��� */
        strRecordNum.clear();
        pDbOper->GetFieldValue("RecordNum", strRecordNum);
        osip_strncpy(compress_task->stYSPB.jlbh, (char*)strRecordNum.c_str(), MAX_TSU_TASK_LEN);

        /* �ļ����� */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("FileName", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.wjmc, (char*)tmp_svalue.c_str(), 128);

        /* �ļ���׺ */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("FileSuffixName", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.kzm, (char*)tmp_svalue.c_str(), 32);

        /* �ļ���С */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("FileSize", tmp_ivalue);
        compress_task->stYSPB.wjdx = tmp_ivalue;

        /* �ϴ���λ */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("UploadUnit", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.scdw, (char*)tmp_svalue.c_str(), 128);

        /* �ϴ�ʱ�� */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("UploadTime", tmp_ivalue);
        compress_task->stYSPB.scsj = tmp_ivalue;

        /* �洢·�� */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("StoragePath", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.cclj, (char*)tmp_svalue.c_str(), 128);

        /* �����ʶ */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("AssignFlag", tmp_ivalue);
        compress_task->iAssignFlag = tmp_ivalue;

        /* ƽ̨IP��ַ */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("PlatformIP", tmp_svalue);
        osip_strncpy(compress_task->strPlatformIP, (char*)tmp_svalue.c_str(), MAX_IP_LEN);

        /* ZRV IP��ַ */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("ZRVDeviceIP", tmp_svalue);
        osip_strncpy(compress_task->strZRVDeviceIP, (char*)tmp_svalue.c_str(), MAX_IP_LEN);

        /* ����״̬ */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("TaskStatus", tmp_ivalue);
        compress_task->iTaskStatus = tmp_ivalue;

        /* ������ */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("TaskResult", tmp_ivalue);
        compress_task->iTaskResult = tmp_ivalue;

        /* ������ */
        iErrorCode = 0;
        pDbOper->GetFieldValue("ErrorCode", iErrorCode);

        /* ѹ������ʱ�� */
        strTaskCreateTime.clear();
        pDbOper->GetFieldValue("TaskCreateTime", strTaskCreateTime);
        iTaskCreateTime = 0;
        iTaskCreateTime = analysis_time2((char*)strTaskCreateTime.c_str());

        if (iTaskCreateTime > 0)
        {
            compress_task->iTaskCreateTime = iTaskCreateTime;
        }
        else
        {
            compress_task->iTaskCreateTime = now;
        }

        /* ѹ����ʼʱ�� */
        iZRVCompressEndTime = 0;
        pDbOper->GetFieldValue("ZRVCompressBeginTime", iZRVCompressBeginTime);

        /* ѹ������ʱ�� */
        iZRVCompressEndTime = 0;
        pDbOper->GetFieldValue("ZRVCompressEndTime", iZRVCompressEndTime);

        compress_task->wait_answer_expire = 0;
        compress_task->resend_count = 0;

        /* ��ӵ����� */
        if (compress_task_add(compress_task) < 0)
        {
            compress_task_free(compress_task);
            osip_free(compress_task);
            compress_task = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "get_complete_compress_task_from_assign_table() compress_task_add Error");
            continue;
        }

        RecordNumVector.push_back(strRecordNum);
        ErrorCodeVector.push_back(iErrorCode);
        CompressBeginTimeVector.push_back(iZRVCompressBeginTime);
        CompressEndTimeVector.push_back(iZRVCompressEndTime);
    }
    while (pDbOper->MoveNext() >= 0);

    return ret;
}

/*****************************************************************************
 �� �� ��  : DeleteCompressTaskFromDBForStart
 ��������  : ����������ʱ��ɾ�������ݿ��г�ʱ������ɵ�����
 �������  : DBOper* pDbOper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2017��8��21��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int DeleteCompressTaskFromDBForStart(DBOper* pDbOper)
{
    int iRet = 0;
    string strDeleteSQL = "";
    int index = 0;

    vector<string> RecordNumVector;
    vector<int> ErrorCodeVector;
    vector<int> CompressBeginTimeVector;
    vector<int> CompressEndTimeVector;

    if (NULL == pDbOper)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "DeleteCompressTaskFromDBForStart() exit---:  Param Error \r\n");
        return -1;
    }

    /* ��ȡ���ڴ��� */
    RecordNumVector.clear();
    ErrorCodeVector.clear();
    CompressBeginTimeVector.clear();
    CompressEndTimeVector.clear();

    get_complete_compress_task_from_assign_table(RecordNumVector, ErrorCodeVector, CompressBeginTimeVector, CompressEndTimeVector, pDbOper);

    /* ���µ����ݿ� */
    if (RecordNumVector.size() > 0)
    {
        for (index = 0; index < (int)RecordNumVector.size(); index++)
        {
            /* ���ӵ����ݿ� */
            iRet = AddCompressTaskToDB((char*)RecordNumVector[index].c_str(), 0, 0, ErrorCodeVector[index], CompressBeginTimeVector[index], CompressEndTimeVector[index], 0, NULL, pDbOper);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "DeleteCompressTaskFromDBForStart() AddCompressTaskToDB: RecordNum=%s, iRet=%d\r\n", (char*)RecordNumVector[index].c_str(), iRet);

            /* ���ڴ���ɾ���� */
            iRet = compress_task_remove((char*)RecordNumVector[index].c_str());
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "DeleteCompressTaskFromDBForStart() compress_task_remove: RecordNum=%s, iRet=%d\r\n", (char*)RecordNumVector[index].c_str(), iRet);
        }
    }

    RecordNumVector.clear();
    ErrorCodeVector.clear();
    CompressBeginTimeVector.clear();
    CompressEndTimeVector.clear();

    /* ɾ����������е����� */
    strDeleteSQL.clear();
    strDeleteSQL = "DELETE FROM ZRVCompressTaskAssignInfo WHERE TaskStatus=2 OR TaskStatus=3";

    iRet = pDbOper->DB_Delete(strDeleteSQL.c_str(), 1);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "DeleteCompressTaskFromDBForStart() :DB_Delete:iRet=%d \r\n", iRet);

    return iRet;
}

/*****************************************************************************
 �� �� ��  : ShowCallTask
 ��������  : ��ʾ��������
 �������  : int sock
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��29�� ����һ
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void ShowCompressTask(int sock)
{
    char strLine[] = "\r-------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rTaskID                               AssignFlag PlatformIP      ZRVDeviceIP     TaskStatus TaskResult Expires\r\n";
    compress_task_t* pCompressTaskData = NULL;
    COMPRESS_TASK_Iterator Itr;
    char rbuf[256] = {0};

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if (g_CompressTaskMap.size() <= 0)
    {
        if (sock > 0)
        {
            send(sock, (char*)"No Compress Task Data", strlen((char*)"No Compress Task Data"), 0);
        }

        return;
    }

    for (Itr = g_CompressTaskMap.begin(); Itr != g_CompressTaskMap.end(); Itr++)
    {
        pCompressTaskData = Itr->second;

        if ((NULL == pCompressTaskData) || (pCompressTaskData->stYSPB.jlbh[0] == '\0'))
        {
            continue;
        }

        memset(rbuf, 0, 256);
        snprintf(rbuf, 255, "\r%-36s %-10d %-15s %-15s %-10d %-10d %-7d\r\n", pCompressTaskData->stYSPB.jlbh, pCompressTaskData->iAssignFlag, pCompressTaskData->strPlatformIP, pCompressTaskData->strZRVDeviceIP, pCompressTaskData->iTaskStatus, pCompressTaskData->iTaskResult, pCompressTaskData->wait_answer_expire);

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
