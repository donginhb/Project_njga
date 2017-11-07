
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
#include "common/log_proc.inc"
#include "common/gblfunc_proc.inc"
#include "common/gblconfig_proc.inc"

#include "record/record_info_mgn.inc"
#include "record/record_srv_proc.inc"

#include "resource/resource_info_mgn.inc"
#include "device/device_info_mgn.inc"
#include "user/user_info_mgn.inc"

/*----------------------------------------------*
 * �ⲿ����˵��                                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �ⲿ����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �ڲ�����ԭ��˵��                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * ȫ�ֱ���                                     *
 *----------------------------------------------*/
unsigned long long iRecordInfoLockCount = 0;
unsigned long long iRecordInfoUnLockCount = 0;

unsigned long long iRecordTimeInfoLockCount = 0;
unsigned long long iRecordTimeInfoUnLockCount = 0;

/*----------------------------------------------*
 * ģ�鼶����                                   *
 *----------------------------------------------*/
record_info_list_t* g_RecordInfoList = NULL;               /* ¼����Ϣ���� */
int current_record_pos = 0;                                /* ��ǰ¼��λ�� */

record_time_sched_list_t* g_RecordTimeSchedList = NULL;    /* ¼��ʱ����Զ��� */
int db_RecordInfo_reload_mark = 0;                         /* ¼����Ϣ���ݿ���±�ʶ:0:����Ҫ���£�1:��Ҫ�������ݿ� */
int db_RecordTimeSched_reload_mark = 0;                    /* ¼��ʱ�̲�����Ϣ���ݿ���±�ʶ:0:����Ҫ���£�1:��Ҫ�������ݿ� */

/*----------------------------------------------*
 * ��������                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * �궨��                                       *
 *----------------------------------------------*/

#if DECS("¼����Ϣ����")
/*****************************************************************************
 �� �� ��  : record_info_init
 ��������  : ¼����Ϣ�ṹ��ʼ��
 �������  : record_info_t ** record_info
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int record_info_init(record_info_t** record_info)
{
    *record_info = (record_info_t*)osip_malloc(sizeof(record_info_t));

    if (*record_info == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_init() exit---: *record_info Smalloc Error \r\n");
        return -1;
    }

    (*record_info)->uID = 0;
    (*record_info)->device_index = 0;
    (*record_info)->stream_type = EV9000_STREAM_TYPE_MASTER;
    (*record_info)->record_enable = 0;
    (*record_info)->record_days = 0;
    (*record_info)->record_timeLen = 0;
    (*record_info)->record_type = EV9000_RECORD_TYPE_NORMAL;
    (*record_info)->assign_record = 0;
    (*record_info)->assign_tsu_index = 0;
    (*record_info)->bandwidth = 0;
    (*record_info)->TimeOfAllWeek = 0;
    (*record_info)->hasRecordDays = 0;
    (*record_info)->tsu_index = -1;
    (*record_info)->record_cr_index = -1;
    (*record_info)->record_try_count = 0;
    (*record_info)->record_retry_interval = 0;
    (*record_info)->record_status = RECORD_STATUS_NULL;
    (*record_info)->record_start_time = 0;
    (*record_info)->iTSUPauseStatus = 0;
    (*record_info)->iTSUResumeStatus = 0;
    (*record_info)->iTSUAlarmRecordStatus = 0;
    (*record_info)->del_mark = 0;

    return 0;
}

/*****************************************************************************
 �� �� ��  : record_info_free
 ��������  : ¼����Ϣ�ṹ�ͷ�
 �������  : record_info_t * record_info
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void record_info_free(record_info_t* record_info)
{
    if (record_info == NULL)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_free() exit---: Param Error \r\n");
        return;
    }

    record_info->uID = 0;
    record_info->device_index = 0;
    record_info->stream_type = EV9000_STREAM_TYPE_MASTER;
    record_info->record_enable = 0;
    record_info->record_days = 0;
    record_info->record_timeLen = 0;
    record_info->record_type = EV9000_RECORD_TYPE_NORMAL;
    record_info->assign_record = 0;
    record_info->assign_tsu_index = 0;
    record_info->bandwidth = 0;
    record_info->TimeOfAllWeek = 0;
    record_info->hasRecordDays = 0;
    record_info->tsu_index = -1;
    record_info->record_cr_index = -1;
    record_info->record_try_count = 0;
    record_info->record_retry_interval = 0;
    record_info->record_status = RECORD_STATUS_NULL;
    record_info->record_start_time = 0;
    record_info->iTSUPauseStatus = 0;
    record_info->iTSUResumeStatus = 0;
    record_info->iTSUAlarmRecordStatus = 0;
    record_info->del_mark = 0;

    osip_free(record_info);
    record_info = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : record_info_list_init
 ��������  : ¼����Ϣ���г�ʼ��
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
int record_info_list_init()
{
    g_RecordInfoList = (record_info_list_t*)osip_malloc(sizeof(record_info_list_t));

    if (g_RecordInfoList == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_list_init() exit---: g_RecordInfoList Smalloc Error \r\n");
        return -1;
    }

    g_RecordInfoList->pRecordInfoList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_RecordInfoList->pRecordInfoList)
    {
        osip_free(g_RecordInfoList);
        g_RecordInfoList = NULL;
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_list_init() exit---: Record Info List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_RecordInfoList->pRecordInfoList);

#ifdef MULTI_THR
    /* init smutex */
    g_RecordInfoList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_RecordInfoList->lock)
    {
        osip_free(g_RecordInfoList->pRecordInfoList);
        g_RecordInfoList->pRecordInfoList = NULL;
        osip_free(g_RecordInfoList);
        g_RecordInfoList = NULL;
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_list_init() exit---: Record Info List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : record_info_list_free
 ��������  : ¼����Ϣ�����ͷ�
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
void record_info_list_free()
{
    if (NULL == g_RecordInfoList)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_RecordInfoList->pRecordInfoList)
    {
        osip_list_special_free(g_RecordInfoList->pRecordInfoList, (void (*)(void*))&record_info_free);
        osip_free(g_RecordInfoList->pRecordInfoList);
        g_RecordInfoList->pRecordInfoList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_RecordInfoList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_RecordInfoList->lock);
        g_RecordInfoList->lock = NULL;
    }

#endif
    osip_free(g_RecordInfoList);
    g_RecordInfoList = NULL;
    return;
}

/*****************************************************************************
 �� �� ��  : record_info_list_lock
 ��������  : ¼����Ϣ��������
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
int record_info_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RecordInfoList == NULL || g_RecordInfoList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_RecordInfoList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : record_info_list_unlock
 ��������  : ¼����Ϣ���н���
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
int record_info_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RecordInfoList == NULL || g_RecordInfoList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_RecordInfoList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_record_info_list_lock
 ��������  : ¼����Ϣ��������
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
int debug_record_info_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RecordInfoList == NULL || g_RecordInfoList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "debug_record_info_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_RecordInfoList->lock, file, line, func);

    iRecordInfoLockCount++;

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_record_info_list_lock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iRecordInfoLockCount != iRecordInfoUnLockCount + 1)
        {
            //printf("\r\n**********%s:%d:%s:debug_record_info_list_lock:iRet=%d, iRecordInfoLockCount=%lld**********\r\n", file, line, func, iRet, iRecordInfoLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_record_info_list_lock:iRet=%d, iRecordInfoLockCount=%lld", file, line, func, iRet, iRecordInfoLockCount);
        }
    }

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_record_info_list_unlock
 ��������  : ¼����Ϣ���н���
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
int debug_record_info_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RecordInfoList == NULL || g_RecordInfoList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "debug_record_info_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRecordInfoUnLockCount++;

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_RecordInfoList->lock, file, line, func);

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_record_info_list_unlock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iRecordInfoLockCount != iRecordInfoUnLockCount)
        {
            //printf("\r\n**********%s:%d:%s:debug_record_info_list_unlock:iRet=%d, iRecordInfoUnLockCount=%lld**********\r\n", file, line, func, iRet, iRecordInfoUnLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_record_info_list_unlock:iRet=%d, iRecordInfoUnLockCount=%lld", file, line, func, iRet, iRecordInfoUnLockCount);
        }
    }

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : record_info_add
 ��������  : ���¼����Ϣ��������
 �������  : char* record_id
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��4��11��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
//int record_info_add(char* device_id)
//{
//    record_info_t* pRecordInfo = NULL;
//    int i = 0;

//    if (g_RecordInfoList == NULL || device_id == NULL)
//    {
//        return -1;
//    }

//    i = record_info_init(&pRecordInfo);

//    if (i != 0)
//    {
//        return -1;
//    }

//    pRecordInfo->device_id = sgetcopy(device_id);

//    record_info_list_lock();
//    i = list_add(g_RecordInfoList->pRecordInfoList, pRecordInfo, -1); /* add to list tail */

//    if (i == -1)
//    {
//        record_info_list_unlock();
//        record_info_free(pRecordInfo);
//        sfree(pRecordInfo);
//        return -1;
//    }

//    record_info_list_unlock();
//    return i;
//}

int record_info_add(record_info_t* pRecordInfo)
{
    int i = 0;

    if (pRecordInfo == NULL)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_add() exit---: Param Error \r\n");
        return -1;
    }

    if ((NULL == g_RecordInfoList) || (NULL == g_RecordInfoList->pRecordInfoList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_add() exit---: Record Info List NULL \r\n");
        return -1;
    }

    RECORD_INFO_SMUTEX_LOCK();

    i = osip_list_add(g_RecordInfoList->pRecordInfoList, pRecordInfo, -1); /* add to list tail */

    if (i < 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_info_add() exit---: List Add Error \r\n");
        return -1;
    }

    RECORD_INFO_SMUTEX_UNLOCK();
    return i - 1;
}

/*****************************************************************************
 �� �� ��  : record_info_remove
 ��������  : �Ӷ������Ƴ�¼����Ϣ
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
int record_info_remove(int pos)
{
    record_info_t* pRecordInfo = NULL;

    RECORD_INFO_SMUTEX_LOCK();

    if (g_RecordInfoList == NULL || pos < 0 || (pos >= osip_list_size(g_RecordInfoList->pRecordInfoList)))
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_remove() exit---: Param Error \r\n");
        return -1;
    }

    pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

    if (NULL == pRecordInfo)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_info_remove() exit---: List Get Error \r\n");
        return -1;
    }

    osip_list_remove(g_RecordInfoList->pRecordInfoList, pos);
    record_info_free(pRecordInfo);
    pRecordInfo = NULL;
    RECORD_INFO_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : record_info_find_by_stream_type
 ��������  : ����¼����Ϣ
 �������  : unsigned int device_index
             int stream_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��5��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int record_info_find_by_stream_type(unsigned int device_index, int stream_type)
{
    int pos = -1;
    record_info_t* pRecordInfo = NULL;

    if (NULL == g_RecordInfoList || NULL == g_RecordInfoList->pRecordInfoList || device_index <= 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_find_by_stream_type() exit---: Param Error \r\n");
        return -1;
    }

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_info_find_by_stream_type() exit---: Record Info List NULL \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_RecordInfoList->pRecordInfoList); pos++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

        if ((NULL == pRecordInfo) || (pRecordInfo->device_index <= 0))
        {
            continue;
        }

        if (pRecordInfo->record_enable == 0)
        {
            continue;
        }

        if (pRecordInfo->device_index == device_index && pRecordInfo->stream_type == stream_type)
        {
            RECORD_INFO_SMUTEX_UNLOCK();
            return pos;
        }
    }

    RECORD_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : record_info_find_by_stream_type2
 ��������  : ����¼����Ϣ
 �������  : unsigned int device_index
             int stream_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��5��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int record_info_find_by_stream_type2(unsigned int device_index, int stream_type)
{
    int pos = -1;
    record_info_t* pRecordInfo = NULL;

    if (NULL == g_RecordInfoList || NULL == g_RecordInfoList->pRecordInfoList || device_index <= 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_find_by_stream_type2() exit---: Param Error \r\n");
        return -1;
    }

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_info_find_by_stream_type2() exit---: Record Info List NULL \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_RecordInfoList->pRecordInfoList); pos++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

        if ((NULL == pRecordInfo) || (pRecordInfo->device_index <= 0))
        {
            continue;
        }

        if (pRecordInfo->device_index == device_index && pRecordInfo->stream_type == stream_type)
        {
            RECORD_INFO_SMUTEX_UNLOCK();
            return pos;
        }
    }

    RECORD_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : record_info_get_by_stream_type_and_record_type
 ��������  : ���������ͺ�¼�����ͻ�ȡ���ݿ����õ�¼����Ϣ
 �������  : unsigned int device_index
             int stream_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��12��23��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
record_info_t* record_info_get_by_stream_type_and_record_type(unsigned int device_index, int stream_type, int record_type)
{
    int pos = -1;
    record_info_t* pRecordInfo = NULL;

    if (NULL == g_RecordInfoList || NULL == g_RecordInfoList->pRecordInfoList || device_index <= 0 || stream_type <= 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_get_by_stream_type_and_record_type() exit---: Param Error \r\n");
        return NULL;
    }

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_info_get_by_stream_type_and_record_type() exit---: Record Info List NULL \r\n");
        return NULL;
    }

    for (pos = 0; pos < osip_list_size(g_RecordInfoList->pRecordInfoList); pos++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

        if ((NULL == pRecordInfo) || (pRecordInfo->device_index <= 0))
        {
            continue;
        }

        if (pRecordInfo->uID == 0)
        {
            continue;
        }

        if (pRecordInfo->device_index == device_index
            && pRecordInfo->stream_type == stream_type
            && pRecordInfo->record_type == record_type)
        {
            RECORD_INFO_SMUTEX_UNLOCK();
            return pRecordInfo;
        }
    }

    RECORD_INFO_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 �� �� ��  : record_info_get_by_stream_type_and_record_type2
 ��������  : ���������ͺ�¼�����ͻ�ȡ�����������õ�¼����Ϣ
 �������  : unsigned int device_index
             int stream_type
             int record_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��3��23��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
record_info_t* record_info_get_by_stream_type_and_record_type2(unsigned int device_index, int stream_type, int record_type)
{
    int pos = -1;
    record_info_t* pRecordInfo = NULL;

    if (NULL == g_RecordInfoList || NULL == g_RecordInfoList->pRecordInfoList || device_index <= 0 || stream_type <= 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_get_by_stream_type_and_record_type() exit---: Param Error \r\n");
        return NULL;
    }

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_info_get_by_stream_type_and_record_type() exit---: Record Info List NULL \r\n");
        return NULL;
    }

    for (pos = 0; pos < osip_list_size(g_RecordInfoList->pRecordInfoList); pos++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

        if ((NULL == pRecordInfo) || (pRecordInfo->device_index <= 0))
        {
            continue;
        }

        if (pRecordInfo->uID != 0)
        {
            continue;
        }

        if (pRecordInfo->device_index == device_index
            && pRecordInfo->stream_type == stream_type
            && pRecordInfo->record_type == record_type)
        {
            RECORD_INFO_SMUTEX_UNLOCK();
            return pRecordInfo;
        }
    }

    RECORD_INFO_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 �� �� ��  : has_record_info_find_by_stream_type
 ��������  : ����¼�������Ͳ����Ѿ�¼���¼����Ϣ
 �������  : unsigned int device_index
             int stream_type
             int record_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��11��13�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int has_record_info_find_by_stream_type(unsigned int device_index, int stream_type, int record_type)
{
    int pos = -1;
    record_info_t* pRecordInfo = NULL;

    if (NULL == g_RecordInfoList || NULL == g_RecordInfoList->pRecordInfoList || device_index <= 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "has_record_info_find_by_stream_type() exit---: Param Error \r\n");
        return -1;
    }

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "has_record_info_find_by_stream_type() exit---: Record Info List NULL \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_RecordInfoList->pRecordInfoList); pos++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

        if ((NULL == pRecordInfo) || (pRecordInfo->device_index <= 0))
        {
            continue;
        }

        if (pRecordInfo->record_enable == 0)
        {
            continue;
        }

        if (pRecordInfo->record_cr_index < 0
            || pRecordInfo->record_type < EV9000_RECORD_TYPE_NORMAL
            || pRecordInfo->record_type > EV9000_RECORD_TYPE_BACKUP)
        {
            continue;
        }

        if (pRecordInfo->device_index == device_index
            && pRecordInfo->stream_type == stream_type
            && pRecordInfo->record_type != record_type)
        {
            RECORD_INFO_SMUTEX_UNLOCK();
            return pos;
        }
    }

    RECORD_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : record_info_find_by_cr_index
 ��������  : ���ݺ�����������¼����Ϣ
 �������  : int cr_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��1�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int record_info_find_by_cr_index(int cr_index)
{
    int pos = -1;
    record_info_t* pRecordInfo = NULL;

    if (NULL == g_RecordInfoList || NULL == g_RecordInfoList->pRecordInfoList || cr_index < 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_find_by_cr_index() exit---: Param Error \r\n");
        return -1;
    }

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_info_find_by_cr_index() exit---: Record Info List NULL \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_RecordInfoList->pRecordInfoList); pos++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

        if ((NULL == pRecordInfo) || (pRecordInfo->record_cr_index < 0))
        {
            continue;
        }

        if (pRecordInfo->record_cr_index == cr_index)
        {
            RECORD_INFO_SMUTEX_UNLOCK();
            return pos;
        }
    }

    RECORD_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : find_is_cr_index_used_by_other_record_info
 ��������  : ����cr_index�Ƿ�¼��ҵ���ظ�ʹ��
 �������  : int current_cr_index
             int current_pos
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��6��5�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int find_is_cr_index_used_by_other_record_info(int current_cr_index, int current_pos)
{
    int pos = -1;
    record_info_t* pRecordInfo = NULL;

    if (NULL == g_RecordInfoList || NULL == g_RecordInfoList->pRecordInfoList || current_cr_index < 0 || current_pos < 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "find_is_cr_index_used_by_other_record_info() exit---: Param Error \r\n");
        return -1;
    }

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "find_is_cr_index_used_by_other_record_info() exit---: Record Info List NULL \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_RecordInfoList->pRecordInfoList); pos++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

        if ((NULL == pRecordInfo) || (pRecordInfo->record_cr_index < 0))
        {
            continue;
        }

        if (pos == current_pos) /* ��ǰʹ�õ����� */
        {
            continue;
        }

        if (pRecordInfo->record_cr_index == current_cr_index)
        {
            return pos;
        }
    }

    return -1;
}

/*****************************************************************************
 �� �� ��  : record_info_find_by_cr_index_for_response
 ��������  : ���ݼ�¼��ѯ¼����Ϣ����¼���Ӧ��Ϣ������
 �������  : int cr_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��6��5�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int record_info_find_by_cr_index_for_response(int cr_index)
{
    int pos = -1;
    record_info_t* pRecordInfo = NULL;

    if (NULL == g_RecordInfoList || NULL == g_RecordInfoList->pRecordInfoList || cr_index < 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_find_by_cr_index_for_response() exit---: Param Error \r\n");
        return -1;
    }

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_info_find_by_cr_index_for_response() exit---: Record Info List NULL \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_RecordInfoList->pRecordInfoList); pos++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

        if (NULL == pRecordInfo)
        {
            continue;
        }

        if (pRecordInfo->record_cr_index < 0)
        {
            continue;
        }

        if (pRecordInfo->record_cr_index != cr_index)
        {
            continue;
        }
        else
        {
            if (0 == pRecordInfo->record_enable)
            {
                RECORD_INFO_SMUTEX_UNLOCK();
                return -3;
            }

            if (pRecordInfo->tsu_index < 0)
            {
                RECORD_INFO_SMUTEX_UNLOCK();
                return -4;
            }

            if (RECORD_STATUS_PROC != pRecordInfo->record_status)
            {
                RECORD_INFO_SMUTEX_UNLOCK();
                return -5;
            }

            RECORD_INFO_SMUTEX_UNLOCK();
            return pos;
        }
    }

    RECORD_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : get_record_cr_index_by_record_index
 ��������  : ����¼����Ϣ�������Һ�������
 �������  : unsigned int uID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��1�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_record_cr_index_by_record_index(unsigned int uID)
{
    int pos = -1;
    record_info_t* pRecordInfo = NULL;

    if (NULL == g_RecordInfoList || NULL == g_RecordInfoList->pRecordInfoList || uID <= 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "get_record_cr_index_by_record_index() exit---: Param Error \r\n");
        return -1;
    }

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "get_record_cr_index_by_record_index() exit---: Record Info List NULL \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_RecordInfoList->pRecordInfoList); pos++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

        if ((NULL == pRecordInfo) || (pRecordInfo->device_index <= 0))
        {
            continue;
        }

        if (pRecordInfo->uID == uID)
        {
            RECORD_INFO_SMUTEX_UNLOCK();
            return pRecordInfo->record_cr_index;
        }
    }

    RECORD_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : record_info_get
 ��������  : ��ȡ¼����Ϣ
 �������  : int pos
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��5��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
record_info_t* record_info_get(int pos)
{
    record_info_t* pRecordInfo = NULL;

    if (g_RecordInfoList == NULL || NULL == g_RecordInfoList->pRecordInfoList || pos < 0 || pos >= osip_list_size(g_RecordInfoList->pRecordInfoList))
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_get() exit---: Param Error \r\n");
        return NULL;
    }

    pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

    return pRecordInfo;
}

/*****************************************************************************
 �� �� ��  : record_info_get_by_record_index
 ��������  : ����¼����Ϣ������ȡ¼����Ϣ
 �������  : unsigned int index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��8��3�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
record_info_t* record_info_get_by_record_index(unsigned int index)
{
    int pos = 0;
    record_info_t* pRecordInfo = NULL;

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        return NULL;
    }

    for (pos = 0; pos < osip_list_size(g_RecordInfoList->pRecordInfoList); pos++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

        if ((NULL == pRecordInfo) || (pRecordInfo->uID <= 0))
        {
            continue;
        }

        if (pRecordInfo->uID == index)
        {
            RECORD_INFO_SMUTEX_UNLOCK();
            return pRecordInfo;
        }

    }

    RECORD_INFO_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 �� �� ��  : record_info_get_by_record_type
 ��������  : ͨ���豸������¼�����Ͳ���¼����Ϣ
 �������  : unsigned int device_index
             int record_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��28�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
record_info_t* record_info_get_by_record_type(unsigned int device_index, int record_type)
{
    int pos = -1;
    record_info_t* pRecordInfo = NULL;

    if (NULL == g_RecordInfoList || NULL == g_RecordInfoList->pRecordInfoList || device_index <= 0 || record_type <= 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_get_by_record_type() exit---: Param Error \r\n");
        return NULL;
    }

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_info_get_by_record_type() exit---: Record Info List NULL \r\n");
        return NULL;
    }

    for (pos = 0; pos < osip_list_size(g_RecordInfoList->pRecordInfoList); pos++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

        if ((NULL == pRecordInfo) || (pRecordInfo->device_index <= 0))
        {
            continue;
        }

        if (pRecordInfo->device_index == device_index && pRecordInfo->record_type == record_type)
        {
            RECORD_INFO_SMUTEX_UNLOCK();
            return pRecordInfo;
        }
    }

    RECORD_INFO_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 �� �� ��  : record_info_get_by_record_type2
 ��������  : ͨ���豸������¼�����Ͳ���¼����Ϣ
 �������  : unsigned int device_index
             int record_type
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��9��28�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
record_info_t* record_info_get_by_record_type2(unsigned int device_index, int record_type)
{
    int pos = -1;
    record_info_t* pRecordInfo = NULL;

    if (NULL == g_RecordInfoList || NULL == g_RecordInfoList->pRecordInfoList || device_index <= 0 || record_type <= 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_info_get_by_record_type2() exit---: Param Error \r\n");
        return NULL;
    }

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_info_get_by_record_type2() exit---: Record Info List NULL \r\n");
        return NULL;
    }

    for (pos = 0; pos < osip_list_size(g_RecordInfoList->pRecordInfoList); pos++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

        if ((NULL == pRecordInfo) || (pRecordInfo->device_index <= 0))
        {
            continue;
        }

        if (pRecordInfo->record_enable == 0)
        {
            continue;
        }

        if (pRecordInfo->device_index == device_index && pRecordInfo->record_type == record_type)
        {
            RECORD_INFO_SMUTEX_UNLOCK();
            return pRecordInfo;
        }
    }

    RECORD_INFO_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 �� �� ��  : get_record_info_index_from_list
 ��������  : ��ȡ¼���������
 �������  : vector<unsigned int>& RecordInfoIndexVector
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_record_info_index_from_list(vector<unsigned int>& RecordInfoIndexVector)
{
    int i = 0;
    int pos = -1;
    record_info_t* pRecordInfo = NULL;

    if ((NULL == g_RecordInfoList) || (NULL == g_RecordInfoList->pRecordInfoList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "get_record_info_index_from_list() exit---: Param Error \r\n");
        return -1;
    }


    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_RecordInfoList->pRecordInfoList); pos++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

        if ((NULL == pRecordInfo) || (pRecordInfo->uID <= 0))
        {
            continue;
        }

        if (pRecordInfo->record_enable == 0)
        {
            continue;
        }

        if (pRecordInfo->TimeOfAllWeek == 1)
        {
            continue;
        }

        i = AddDeviceIndexToDeviceIndexVector(RecordInfoIndexVector, pRecordInfo->uID);
        //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "get_record_info_index_from_list() AddDeviceIndexToDeviceIndexVector:uID=%u \r\n", pRecordInfo->uID);
    }

    RECORD_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : add_record_info_by_message_cmd
 ��������  : ������Ϣ��������¼����Ϣ
 �������  : unsigned int device_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��6��5�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int add_record_info_by_message_cmd(unsigned int device_index, DBOper* pRecord_Srv_dboper)
{
    int iRet = 0;
    int record_info_pos = -1;
    record_info_t* pRecordInfo = NULL;
    int record_cr_index = -1;

    if (device_index <= 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "add_record_info_by_message_cmd() exit---: Device Index Error:device_index=%u \r\n", device_index);
        return -1;
    }

    record_info_pos = record_info_find_by_stream_type2(device_index, EV9000_STREAM_TYPE_MASTER);
    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "add_record_info_by_message_cmd() record_info_find_by_stream_type:device_index=%u, stream_type=%d, record_info_pos=%d \r\n", device_index, EV9000_STREAM_TYPE_MASTER, record_info_pos);

    if (record_info_pos >= 0)
    {
        pRecordInfo = record_info_get(record_info_pos);

        if (NULL != pRecordInfo)
        {
            if (pRecordInfo->record_enable == 0)
            {
                pRecordInfo->record_enable = 1;
            }

            if (pRecordInfo->del_mark == 1)
            {
                pRecordInfo->del_mark = 0;
            }

            if (0 == pRecordInfo->TimeOfAllWeek)
            {
                pRecordInfo->TimeOfAllWeek = 1;
            }

            if (RECORD_STATUS_INIT == pRecordInfo->record_status
                || pRecordInfo->record_cr_index < 0)
            {
                record_cr_index = StartDeviceRecord(pRecordInfo);
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "add_record_info_by_message_cmd() StartDeviceRecord:record_cr_index=%d \r\n", record_cr_index);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "add_record_info_by_message_cmd() device_index=%u, stream_type=%d, record_cr_index=%d has record \r\n", device_index, EV9000_STREAM_TYPE_MASTER, pRecordInfo->record_cr_index);
            }

            /* �������ݿ������ */
            iRet = AddRecordInfo2DB(pRecordInfo, pRecord_Srv_dboper);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "add_record_info_by_message_cmd() exit---: AddRecordInfo2DB Error \r\n");
                return iRet;
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "add_record_info_by_message_cmd() Get Record Info Error:record_info_pos=%d \r\n", record_info_pos);
        }
    }
    else /* ������ */
    {
        iRet = record_info_init(&pRecordInfo);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "add_record_info_by_message_cmd() exit---: record_info_init Error \r\n");
            return iRet;
        }

        pRecordInfo->device_index = device_index;               /* �豸���� */
        pRecordInfo->record_enable = 1;                         /* �Ƿ�����¼�� */
        pRecordInfo->record_days = 5;                           /* ¼������ */
        pRecordInfo->record_timeLen = 10;                       /* ¼��ʱ�� */
        pRecordInfo->record_type = EV9000_RECORD_TYPE_NORMAL;   /* ¼������ */
        pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;   /* �������� */
        pRecordInfo->bandwidth = 20;                            /* ǰ�˴���*/
        pRecordInfo->TimeOfAllWeek = 1;                         /* �Ƿ�ȫ��¼�� */

        iRet = record_info_add(pRecordInfo);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "add_record_info_by_message_cmd() exit---: record_info_add Error \r\n");
            record_info_free(pRecordInfo);
            pRecordInfo = NULL;
            return iRet;
        }

        if (RECORD_STATUS_INIT == pRecordInfo->record_status
            || pRecordInfo->record_cr_index < 0)
        {
            record_cr_index = StartDeviceRecord(pRecordInfo);
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "add_record_info_by_message_cmd() StartDeviceRecord:record_cr_index=%d \r\n", record_cr_index);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "add_record_info_by_message_cmd() device_index=%u, stream_type=%d, record_cr_index=%d has record \r\n", device_index, EV9000_STREAM_TYPE_MASTER, pRecordInfo->record_cr_index);
        }

        /* �������ݿ������ */
        iRet = AddRecordInfo2DB(pRecordInfo, pRecord_Srv_dboper);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "add_record_info_by_message_cmd() exit---: AddRecordInfo2DB Error \r\n");
            return iRet;
        }
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : del_record_info_by_message_cmd
 ��������  : ������Ϣ����ɾ��¼����Ϣ
 �������  : unsigned int device_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��6��5�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int del_record_info_by_message_cmd(unsigned int device_index, DBOper* pRecord_Srv_dboper)
{
    int iRet = 0;
    int record_info_pos = -1;
    record_info_t* pRecordInfo = NULL;

    if (device_index <= 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "del_record_info_by_message_cmd() exit---: Device Index Error:device_index=%u \r\n", device_index);
        return -1;
    }

    record_info_pos = record_info_find_by_stream_type2(device_index, EV9000_STREAM_TYPE_MASTER);
    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "del_record_info_by_message_cmd() record_info_find_by_stream_type:device_index=%u, stream_type=%d, record_info_pos=%d \r\n", device_index, EV9000_STREAM_TYPE_MASTER, record_info_pos);

    if (record_info_pos >= 0)
    {
        pRecordInfo = record_info_get(record_info_pos);

        if (NULL != pRecordInfo)
        {
            if (RECORD_STATUS_INIT != pRecordInfo->record_status
                && pRecordInfo->record_cr_index >= 0)
            {
                iRet = StopDeviceRecord(pRecordInfo->record_cr_index);

                pRecordInfo->tsu_index = -1;
                pRecordInfo->record_cr_index = -1;
                pRecordInfo->record_status = RECORD_STATUS_INIT;
                pRecordInfo->record_start_time = 0;
                pRecordInfo->record_try_count = 0;
                pRecordInfo->record_retry_interval = 5;
                pRecordInfo->iTSUPauseStatus = 0;
                pRecordInfo->iTSUResumeStatus = 0;
                pRecordInfo->iTSUAlarmRecordStatus = 0;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "device_device_control_proc() StopDeviceRecord:record_cr_index=%d, i=%d \r\n", pRecordInfo->record_cr_index, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "device_device_control_proc() device_index=%s, stream_type=%d, has not record \r\n", device_index, EV9000_STREAM_TYPE_MASTER);
            }

            pRecordInfo->record_enable = 0;

            /* ɾ�����ݿ������ */
            iRet = delRecordInfo2DB(device_index, pRecord_Srv_dboper);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "add_record_info_by_message_cmd() exit---: delRecordInfo2DB Error \r\n");
                return iRet;
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "device_device_control_proc() Get Record Info Error:record_info_pos=%d \r\n", record_info_pos);
        }
    }
    else /* ������ */
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "add_record_info_by_message_cmd() exit---: Record Info Find Error \r\n");
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : set_record_info_list_del_mark
 ��������  : ����¼����Ϣɾ����ʶ
 �������  : int del_mark
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��12�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int set_record_info_list_del_mark(int del_mark)
{
    int pos = -1;
    record_info_t* pRecordInfo = NULL;

    if ((NULL == g_RecordInfoList) || (NULL == g_RecordInfoList->pRecordInfoList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "set_record_info_list_del_mark() exit---: Param Error \r\n");
        return -1;
    }

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "set_record_info_list_del_mark() exit---: Record Info List NULL \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_RecordInfoList->pRecordInfoList); pos++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

        if ((NULL == pRecordInfo) || (pRecordInfo->device_index <= 0))
        {
            continue;
        }

        pRecordInfo->del_mark = del_mark;
        //printf("set_record_info_list_del_mark() RecordInfo:device_index=%u, record_type=%d, stream_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pRecordInfo->stream_type, pos);
    }

    RECORD_INFO_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 �� �� ��  : delete_record_info_from_list_by_mark
 ��������  : ����ɾ����ʶ��ɾ��¼����Ϣ
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��12�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int delete_record_info_from_list_by_mark()
{
    int pos = -1;
    record_info_t* pRecordInfo = NULL;

    if ((NULL == g_RecordInfoList) || (NULL == g_RecordInfoList->pRecordInfoList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "delete_record_info_from_list_by_mark() exit---: Param Error \r\n");
        return -1;
    }

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "delete_record_info_from_list_by_mark() exit---: Record Info List NULL \r\n");
        return 0;
    }

    pos = 0;

    while (!osip_list_eol(g_RecordInfoList->pRecordInfoList, pos))
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, pos);

        if (NULL == pRecordInfo)
        {
            osip_list_remove(g_RecordInfoList->pRecordInfoList, pos);
            continue;
        }

        if (pRecordInfo->device_index <= 0 || pRecordInfo->del_mark == 1)
        {
            osip_list_remove(g_RecordInfoList->pRecordInfoList, pos);
            record_info_free(pRecordInfo);
            pRecordInfo = NULL;
        }
        else
        {
            pos++;
        }
    }

    RECORD_INFO_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : check_record_info_from_db_to_list
 ��������  : ���¼����Ա������Ƿ��б仯����ͬ�����ڴ�
 �������  : DBOper* pRecord_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��2��12�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_record_info_from_db_to_list(DBOper* pRecord_Srv_dboper)
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;
    record_info_t* pOldRecordInfo = NULL;
    int while_count = 0;
    int pos = -1;

    if (NULL == pRecord_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "check_record_info_from_db_to_list() exit---: Record Srv db Oper Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from RecordSchedConfig";

    record_count = pRecord_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_info_from_db_to_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_info_from_db_to_list() ErrorMsg=%s\r\n", pRecord_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "check_record_info_from_db_to_list() exit---: No Record Count \r\n");
        return 0;
    }

    /* ѭ���������ݿ�*/
    do
    {
        record_info_t* pRecordInfo = NULL;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "check_record_info_from_db_to_list() While Count=%d \r\n", while_count);
        }

        int i_ret = record_info_init(&pRecordInfo);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_info_from_db_to_list() record_info_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        int tmp_ivalue = 0;
        unsigned int tmp_uivalue = 0;

        /* ���ݿ�����*/
        tmp_uivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("ID", tmp_uivalue);

        pRecordInfo->uID = tmp_uivalue;
        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->uID:%d", pRecordInfo->uID);


        /* �߼��豸ͳ����*/
        tmp_uivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("DeviceIndex", tmp_uivalue);

        pRecordInfo->device_index = tmp_uivalue;
        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->device_index:%d", pRecordInfo->device_index);


        /* �Ƿ�����¼��*/
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("RecordEnable", tmp_ivalue);

        pRecordInfo->record_enable = tmp_ivalue;
        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->record_enable:%d", pRecordInfo->record_enable);


        /* ¼������ */
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("Days", tmp_ivalue);

        pRecordInfo->record_days = tmp_ivalue;
        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->record_days:%d", pRecordInfo->record_days);


        /* ¼��ʱ�� */
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("TimeLength", tmp_ivalue);

        pRecordInfo->record_timeLen = tmp_ivalue;
        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->record_timeLen:%d", pRecordInfo->record_timeLen);


        /* ¼������ */
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("Type", tmp_ivalue);

        pRecordInfo->record_type = tmp_ivalue;

        if (pRecordInfo->record_type < EV9000_RECORD_TYPE_NORMAL || pRecordInfo->record_type > EV9000_RECORD_TYPE_BACKUP)
        {
            pRecordInfo->record_type = EV9000_RECORD_TYPE_NORMAL;
        }

        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->record_type:%d", pRecordInfo->record_type);


        /* �Ƿ�ָ��¼�� */
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("AssignRecord", tmp_ivalue);

        pRecordInfo->assign_record = tmp_ivalue;

        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->assign_record:%d", pRecordInfo->assign_record);


        /* ָ��¼���TSU���� */
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("TSUIndex", tmp_ivalue);

        pRecordInfo->assign_tsu_index = tmp_ivalue;

        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->assign_tsu_index:%d", pRecordInfo->assign_tsu_index);


        /* �������� */
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("StreamType", tmp_ivalue);

        pRecordInfo->stream_type = tmp_ivalue;

        if (pRecordInfo->stream_type != EV9000_STREAM_TYPE_MASTER
            && pRecordInfo->stream_type != EV9000_STREAM_TYPE_SLAVE
            && pRecordInfo->stream_type != EV9000_STREAM_TYPE_INTELLIGENCE)
        {
            pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;
        }

        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->stream_type:%d", pRecordInfo->stream_type);


        /* �Ƿ�ȫ��¼�� */
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("TimeOfAllWeek", tmp_ivalue);

        pRecordInfo->TimeOfAllWeek = tmp_ivalue;
        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->TimeOfAllWeek:%d", pRecordInfo->TimeOfAllWeek);


        /* ����Ҫ�Ĵ��� */
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("BandWidth", tmp_ivalue);

        pRecordInfo->bandwidth = tmp_ivalue;
        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->bandwidth:%d \r\n", pRecordInfo->bandwidth);

        /* ����¼�����ͺ��߼��豸��������¼����Ϣ */
        pOldRecordInfo = record_info_get_by_stream_type_and_record_type(pRecordInfo->device_index, pRecordInfo->stream_type, pRecordInfo->record_type);

        if (NULL != pOldRecordInfo)
        {
            pOldRecordInfo->del_mark = 0;
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_record_info_from_db_to_list() Set Record Info Del Mark Zero:device_index=%u, record_type=%d, stream_type=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pRecordInfo->stream_type);

            if (pOldRecordInfo->uID != pRecordInfo->uID) /* ���ݿ����� */
            {
                pOldRecordInfo->uID = pRecordInfo->uID;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() uID change:device_index=%u, uID=%u \r\n", pRecordInfo->device_index, pRecordInfo->uID);
            }

            if (pOldRecordInfo->record_enable != pRecordInfo->record_enable) /* �Ƿ�����¼�� */
            {
                pOldRecordInfo->record_enable = pRecordInfo->record_enable;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() record_enable change:device_index=%u, record_enable=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_enable);
            }

            if (pOldRecordInfo->record_days != pRecordInfo->record_days) /* ¼������ */
            {
                pOldRecordInfo->record_days = pRecordInfo->record_days;
                pOldRecordInfo->del_mark = 2;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() record_days change:device_index=%u, record_days=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_days);
            }

            if (pOldRecordInfo->record_timeLen != pRecordInfo->record_timeLen) /* ¼��ʱ�� */
            {
                pOldRecordInfo->record_timeLen = pRecordInfo->record_timeLen;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() record_timeLen change:device_index=%u, record_timeLen=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_timeLen);
            }

            if (pOldRecordInfo->record_type != pRecordInfo->record_type) /* ¼������ */
            {
                pOldRecordInfo->record_type = pRecordInfo->record_type;
                pOldRecordInfo->del_mark = 2;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() record_type change:device_index=%u, record_type=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type);
            }

            if (pOldRecordInfo->assign_record != pRecordInfo->assign_record) /* �Ƿ�ָ��¼�� */
            {
                pOldRecordInfo->assign_record = pRecordInfo->assign_record;
                pOldRecordInfo->del_mark = 2;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() assign_record change:device_index=%u, assign_record=%d \r\n", pRecordInfo->device_index, pRecordInfo->assign_record);
            }

            if (pOldRecordInfo->assign_tsu_index != pRecordInfo->assign_tsu_index) /* ָ��¼���TSU���� */
            {
                pOldRecordInfo->assign_tsu_index = pRecordInfo->assign_tsu_index;
                pOldRecordInfo->del_mark = 2;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() assign_record change:device_index=%u, assign_tsu_index=%d \r\n", pRecordInfo->device_index, pRecordInfo->assign_tsu_index);
            }

            if (pOldRecordInfo->stream_type != pRecordInfo->stream_type) /* �������� */
            {
                pOldRecordInfo->stream_type = pRecordInfo->stream_type;
                pOldRecordInfo->del_mark = 2;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() stream_type change:device_index=%u, stream_type=%d \r\n", pRecordInfo->device_index, pRecordInfo->stream_type);
            }

            if (pOldRecordInfo->bandwidth != pRecordInfo->bandwidth) /* ǰ�˴���*/
            {
                pOldRecordInfo->bandwidth = pRecordInfo->bandwidth;
                pOldRecordInfo->del_mark = 2;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() bandwidth change:device_index=%u, bandwidth=%d \r\n", pRecordInfo->device_index, pRecordInfo->bandwidth);
            }

            if (pOldRecordInfo->TimeOfAllWeek != pRecordInfo->TimeOfAllWeek) /* �Ƿ�ȫ��¼�� */
            {
                /* ���ԭ��û��ȫ��¼�����ڱ��ȫ��¼���ˣ���Ҫ����һ��TSU�Ļָ��ӿ� */
                if (0 == pOldRecordInfo->TimeOfAllWeek && 1 == pRecordInfo->TimeOfAllWeek)
                {
                    pOldRecordInfo->del_mark = 3;
                    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() TimeOfAllWeek change:device_index=%u, TimeOfAllWeek=%d, Need To Notify TSU Resume \r\n", pRecordInfo->device_index, pRecordInfo->TimeOfAllWeek);
                }

                /* ���ԭ��ȫ��¼�����ڱ�ɲ���ȫ��¼���ˣ���Ҫ����һ��TSU����ͣ�ӿ� */
                if (1 == pOldRecordInfo->TimeOfAllWeek && 0 == pRecordInfo->TimeOfAllWeek)
                {
                    pOldRecordInfo->del_mark = 4;
                    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() TimeOfAllWeek change:device_index=%u, TimeOfAllWeek=%d, Need To Notify TSU Pause \r\n", pRecordInfo->device_index, pRecordInfo->TimeOfAllWeek);
                }

                pOldRecordInfo->TimeOfAllWeek = pRecordInfo->TimeOfAllWeek;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() TimeOfAllWeek change:device_index=%u, TimeOfAllWeek=%d \r\n", pRecordInfo->device_index, pRecordInfo->TimeOfAllWeek);
            }

            record_info_free(pRecordInfo);
            pRecordInfo = NULL;
            continue;
        }
        else
        {
            /* ��ӵ����� */
            pos = record_info_add(pRecordInfo);

            if (pos < 0)
            {
                record_info_free(pRecordInfo);
                pRecordInfo = NULL;
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_info_from_db_to_list() Record Info Add Error");
                continue;
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() New Record Info:device_index=%u, record_type=%d, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pRecordInfo->record_type, pos);
            }
        }
    }
    while (pRecord_Srv_dboper->MoveNext() >= 0);

    return ret;
}

/*****************************************************************************
 �� �� ��  : check_plan_action_record_info_from_db_to_list
 ��������  : ���Ԥ�������еı���¼����б仯����ͬ�����ڴ�
 �������  : DBOper* pRecord_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��11��3��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_plan_action_record_info_from_db_to_list(DBOper* pRecord_Srv_dboper)
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;
    record_info_t* pOldRecordInfo = NULL;
    record_info_t* pMasterRecordInfo = NULL;
    int while_count = 0;
    int pos = -1;

    if (NULL == pRecord_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "check_plan_action_record_info_from_db_to_list() exit---: Record Srv db Oper Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "SELECT DISTINCT DeviceIndex FROM PlanActionConfig WHERE TYPE=3 ORDER BY DeviceIndex ASC"; /* ����Ԥ����������Ϊ3�ı���¼�񣬼��뵽¼����� */

    record_count = pRecord_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_plan_action_record_info_from_db_to_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_plan_action_record_info_from_db_to_list() ErrorMsg=%s\r\n", pRecord_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "check_plan_action_record_info_from_db_to_list() exit---: No Record Count \r\n");
        return 0;
    }

    /* ѭ���������ݿ�*/
    do
    {
        record_info_t* pRecordInfo = NULL;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "check_plan_action_record_info_from_db_to_list() While Count=%d \r\n", while_count);
        }

        int i_ret = record_info_init(&pRecordInfo);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_plan_action_record_info_from_db_to_list() record_info_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        unsigned int tmp_uivalue = 0;

        /* �����̶�Ϊ0 */
        pRecordInfo->uID = 0;

        /* �߼��豸ͳ����*/
        tmp_uivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("DeviceIndex", tmp_uivalue);

        pRecordInfo->device_index = tmp_uivalue;

        /* �Ƿ�����¼��*/
        pRecordInfo->record_enable = 1;

        /* ¼������ */
        pRecordInfo->record_days = 1;

        /* ¼��ʱ�� */
        pRecordInfo->record_timeLen = 10;

        /* ¼������ */
        pRecordInfo->record_type = EV9000_RECORD_TYPE_ALARM;

        /* �������� */
        pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;

        /* �Ƿ�ָ��¼�� */
        pRecordInfo->assign_record = 0;

        /* �Ƿ�ȫ��¼�� */
        pRecordInfo->TimeOfAllWeek = 0;

        /* ǰ�˴��� */
        pRecordInfo->bandwidth = 1;

        pRecordInfo->del_mark = 0;

        /* ���ݱ���¼�����ͺ��߼��豸��������¼����Ϣ */
        pOldRecordInfo = record_info_get_by_record_type(pRecordInfo->device_index, pRecordInfo->record_type);

        if (NULL == pOldRecordInfo)
        {
            /* �����������Ͳ���¼����Ϣ�������Ѿ���������¼�񣬾Ͳ���Ҫ�����ӱ���¼����Ϣ */
            pMasterRecordInfo = record_info_get_by_stream_type_and_record_type(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

            if (NULL != pMasterRecordInfo)
            {
                if (pMasterRecordInfo->del_mark == 0)
                {
                    /* ����״̬����� */
                    if (1 == pMasterRecordInfo->record_enable)
                    {
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_plan_action_record_info_from_db_to_list() Record Info Has Exist 1:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                        record_info_free(pRecordInfo);
                        pRecordInfo = NULL;
                        continue;
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_plan_action_record_info_from_db_to_list() Record Info Not Enable:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_plan_action_record_info_from_db_to_list() Record Info Has Del:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                }
            }

            /* ��ӵ����� */
            pos = record_info_add(pRecordInfo);

            if (pos < 0)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_plan_action_record_info_from_db_to_list() Record Info Add Error");
                record_info_free(pRecordInfo);
                pRecordInfo = NULL;
                continue;
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_plan_action_record_info_from_db_to_list() New Alarm Record Info:device_index=%u, record_type=%d, stream_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pRecordInfo->stream_type, pos);
            }
        }
        else
        {
            /* �����������Ͳ���¼����Ϣ�������Ѿ���������¼��ԭ���ϵı���¼����Ϣ��Ҫɾ����  */
            pMasterRecordInfo = record_info_get_by_stream_type_and_record_type(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

            if (NULL != pMasterRecordInfo)
            {
                if (pMasterRecordInfo->del_mark == 0)
                {
                    /* ����״̬����� */
                    if (1 == pMasterRecordInfo->record_enable)
                    {
                        /* ɾ����ʶ����Ϊ0 */
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_plan_action_record_info_from_db_to_list() Record Info Has Exist 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                    }
                    else /* ����״̬����� */
                    {
                        if (pOldRecordInfo->del_mark == 1)
                        {
                            /* ɾ����ʶ */
                            pOldRecordInfo->del_mark = 0;
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_plan_action_record_info_from_db_to_list() Set Record Info Del Mark Zero 1:device_index=%u, record_type=%d, stream_type=%d\r\n", pOldRecordInfo->device_index, pOldRecordInfo->record_type, pOldRecordInfo->stream_type);

                            if (0 == pOldRecordInfo->record_enable)
                            {
                                /* ���ñ�ʶ */
                                pOldRecordInfo->record_enable = 1;

                                pOldRecordInfo->uID = pRecordInfo->uID;
                                pOldRecordInfo->assign_record = pRecordInfo->assign_record;
                                pOldRecordInfo->assign_tsu_index = pRecordInfo->assign_tsu_index;
                                pOldRecordInfo->record_days = pRecordInfo->record_days;
                                pOldRecordInfo->record_timeLen = pRecordInfo->record_timeLen;
                                pOldRecordInfo->record_type = pRecordInfo->record_type;
                                pOldRecordInfo->bandwidth = pRecordInfo->bandwidth;
                                pOldRecordInfo->TimeOfAllWeek = pRecordInfo->TimeOfAllWeek;

                                pOldRecordInfo->tsu_index = -1;
                                pOldRecordInfo->record_cr_index = -1;
                                pOldRecordInfo->record_retry_interval = 5;
                                pOldRecordInfo->record_try_count = 0;
                                pOldRecordInfo->iTSUPauseStatus = 0;
                                pOldRecordInfo->iTSUResumeStatus = 0;
                                pOldRecordInfo->iTSUAlarmRecordStatus = 0;
                                pOldRecordInfo->record_status = RECORD_STATUS_INIT;
                                pOldRecordInfo->record_start_time = 0;
                            }
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_plan_action_record_info_from_db_to_list() Record Info Has Exist 3:device_index=%u, record_type=%d, stream_type=%d\r\n", pOldRecordInfo->device_index, pOldRecordInfo->record_type, pOldRecordInfo->stream_type);
                        }
                    }
                }
                else /* Ҫɾ��������� */
                {
                    if (pOldRecordInfo->del_mark == 1)
                    {
                        /* ɾ����ʶ */
                        pOldRecordInfo->del_mark = 0;
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_plan_action_record_info_from_db_to_list() Set Record Info Del Mark Zero 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pOldRecordInfo->device_index, pOldRecordInfo->record_type, pOldRecordInfo->stream_type);

                        if (0 == pOldRecordInfo->record_enable)
                        {
                            /* ���ñ�ʶ */
                            pOldRecordInfo->record_enable = 1;

                            pOldRecordInfo->uID = pRecordInfo->uID;
                            pOldRecordInfo->assign_record = pRecordInfo->assign_record;
                            pOldRecordInfo->assign_tsu_index = pRecordInfo->assign_tsu_index;
                            pOldRecordInfo->record_days = pRecordInfo->record_days;
                            pOldRecordInfo->record_timeLen = pRecordInfo->record_timeLen;
                            pOldRecordInfo->record_type = pRecordInfo->record_type;
                            pOldRecordInfo->bandwidth = pRecordInfo->bandwidth;
                            pOldRecordInfo->TimeOfAllWeek = pRecordInfo->TimeOfAllWeek;

                            pOldRecordInfo->tsu_index = -1;
                            pOldRecordInfo->record_cr_index = -1;
                            pOldRecordInfo->record_retry_interval = 5;
                            pOldRecordInfo->record_try_count = 0;
                            pOldRecordInfo->iTSUPauseStatus = 0;
                            pOldRecordInfo->iTSUResumeStatus = 0;
                            pOldRecordInfo->iTSUAlarmRecordStatus = 0;
                            pOldRecordInfo->record_status = RECORD_STATUS_INIT;
                            pOldRecordInfo->record_start_time = 0;
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_plan_action_record_info_from_db_to_list() Record Info Has Exist 4:device_index=%u, record_type=%d, stream_type=%d\r\n", pOldRecordInfo->device_index, pOldRecordInfo->record_type, pOldRecordInfo->stream_type);
                        }
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_plan_action_record_info_from_db_to_list() Record Info Has Exist 5:device_index=%u, record_type=%d, stream_type=%d\r\n", pOldRecordInfo->device_index, pOldRecordInfo->record_type, pOldRecordInfo->stream_type);
                    }
                }
            }
            else
            {
                if (pOldRecordInfo->del_mark == 1)
                {
                    /* ɾ����ʶ */
                    pOldRecordInfo->del_mark = 0;
                    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_plan_action_record_info_from_db_to_list() Set Record Info Del Mark Zero 3:device_index=%u, record_type=%d, stream_type=%d\r\n", pOldRecordInfo->device_index, pOldRecordInfo->record_type, pOldRecordInfo->stream_type);

                    if (0 == pOldRecordInfo->record_enable)
                    {
                        /* ���ñ�ʶ */
                        pOldRecordInfo->record_enable = 1;

                        pOldRecordInfo->uID = pRecordInfo->uID;
                        pOldRecordInfo->assign_record = pRecordInfo->assign_record;
                        pOldRecordInfo->assign_tsu_index = pRecordInfo->assign_tsu_index;
                        pOldRecordInfo->record_days = pRecordInfo->record_days;
                        pOldRecordInfo->record_timeLen = pRecordInfo->record_timeLen;
                        pOldRecordInfo->record_type = pRecordInfo->record_type;
                        pOldRecordInfo->bandwidth = pRecordInfo->bandwidth;
                        pOldRecordInfo->TimeOfAllWeek = pRecordInfo->TimeOfAllWeek;

                        pOldRecordInfo->tsu_index = -1;
                        pOldRecordInfo->record_cr_index = -1;
                        pOldRecordInfo->record_retry_interval = 5;
                        pOldRecordInfo->record_try_count = 0;
                        pOldRecordInfo->iTSUPauseStatus = 0;
                        pOldRecordInfo->iTSUResumeStatus = 0;
                        pOldRecordInfo->iTSUAlarmRecordStatus = 0;
                        pOldRecordInfo->record_status = RECORD_STATUS_INIT;
                        pOldRecordInfo->record_start_time = 0;
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_plan_action_record_info_from_db_to_list() Record Info Has Exist 6:device_index=%u, record_type=%d, stream_type=%d\r\n", pOldRecordInfo->device_index, pOldRecordInfo->record_type, pOldRecordInfo->stream_type);
                }
            }

            record_info_free(pRecordInfo);
            pRecordInfo = NULL;
            continue;
        }
    }
    while (pRecord_Srv_dboper->MoveNext() >= 0);

    return ret;
}

/*****************************************************************************
 �� �� ��  : check_shdb_daily_upload_pic_record_info_from_db_to_list
 ��������  : �����Ϻ��ر궨ʱ��ͼ������¼��������Ϣ
 �������  : DBOper* pRecord_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��3��19��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_shdb_daily_upload_pic_record_info_from_db_to_list(DBOper* pRecord_Srv_dboper)
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;
    record_info_t* pOldRecordInfo = NULL;
    record_info_t* pMasterRecordInfo = NULL;
    int while_count = 0;
    int pos = -1;

    if (NULL == pRecord_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "check_shdb_daily_upload_pic_record_info_from_db_to_list() exit---: Record Srv db Oper Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "SELECT DISTINCT DeviceIndex FROM DiBiaoUploadPicMapConfig ORDER BY DeviceIndex ASC";

    record_count = pRecord_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_daily_upload_pic_record_info_from_db_to_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_daily_upload_pic_record_info_from_db_to_list() ErrorMsg=%s\r\n", pRecord_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "check_plan_action_record_info_from_db_to_list() exit---: No Record Count \r\n");
        return 0;
    }

    /* ѭ���������ݿ�*/
    do
    {
        record_info_t* pRecordInfo = NULL;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "check_shdb_daily_upload_pic_record_info_from_db_to_list() While Count=%d \r\n", while_count);
        }

        int i_ret = record_info_init(&pRecordInfo);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_daily_upload_pic_record_info_from_db_to_list() record_info_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        unsigned int tmp_uivalue = 0;

        /* �����̶�Ϊ0 */
        pRecordInfo->uID = 0;

        /* �߼��豸ͳ����*/
        tmp_uivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("DeviceIndex", tmp_uivalue);

        pRecordInfo->device_index = tmp_uivalue;

        /* �Ƿ�����¼��*/
        pRecordInfo->record_enable = 1;

        /* ¼������ */
        pRecordInfo->record_days = 1;

        /* ¼��ʱ�� */
        pRecordInfo->record_timeLen = 10;

        /* ¼������ */
        pRecordInfo->record_type = EV9000_RECORD_TYPE_NORMAL;

        /* �������� */
        pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;

        /* �Ƿ�ָ��¼�� */
        pRecordInfo->assign_record = 0;

        /* �Ƿ�ȫ��¼�� */
        pRecordInfo->TimeOfAllWeek = 0;

        /* ǰ�˴��� */
        pRecordInfo->bandwidth = 1;

        pRecordInfo->del_mark = 0;

        /* ����¼�����ͺ��߼��豸��������¼����Ϣ */
        pOldRecordInfo = record_info_get_by_stream_type_and_record_type(pRecordInfo->device_index, pRecordInfo->stream_type, pRecordInfo->record_type);

        if (NULL == pOldRecordInfo)
        {
            /* �����Ѿ����ڱ���¼�񣬾Ͳ���Ҫ������¼����Ϣ */
            pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_ALARM);

            if (NULL != pMasterRecordInfo)
            {
                if (pMasterRecordInfo->del_mark == 0)
                {
                    /* ����״̬����� */
                    if (1 == pMasterRecordInfo->record_enable)
                    {
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Has Exist 1:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                        record_info_free(pRecordInfo);
                        pRecordInfo = NULL;
                        continue;
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Not Enable 1:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Del 1:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                }
            }
            else
            {
                /* �����Ѿ�����ͼƬ�ϴ�¼�񣬾Ͳ���Ҫ������ͼƬ¼����Ϣ */
                pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

                if (NULL != pMasterRecordInfo)
                {
                    if (pMasterRecordInfo->del_mark == 0)
                    {
                        /* ����״̬����� */
                        if (1 == pMasterRecordInfo->record_enable)
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Has Exist 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                        }
                        else
                        {
                            //printf("check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Not Enable 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);

                            /* ���ñ�ʶ */
                            pMasterRecordInfo->record_enable = 1;

                            /* ɾ����ʶ */
                            pMasterRecordInfo->del_mark = 0;
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Set Record Info Del Mark Zero 1:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);

                            pMasterRecordInfo->uID = pRecordInfo->uID;
                            pMasterRecordInfo->assign_record = pRecordInfo->assign_record;
                            pMasterRecordInfo->assign_tsu_index = pRecordInfo->assign_tsu_index;
                            pMasterRecordInfo->record_days = pRecordInfo->record_days;
                            pMasterRecordInfo->record_timeLen = pRecordInfo->record_timeLen;
                            pMasterRecordInfo->record_type = pRecordInfo->record_type;
                            pMasterRecordInfo->bandwidth = pRecordInfo->bandwidth;
                            pMasterRecordInfo->TimeOfAllWeek = pRecordInfo->TimeOfAllWeek;

                            pMasterRecordInfo->tsu_index = -1;
                            pMasterRecordInfo->record_cr_index = -1;
                            pMasterRecordInfo->record_retry_interval = 5;
                            pMasterRecordInfo->record_try_count = 0;
                            pMasterRecordInfo->iTSUPauseStatus = 0;
                            pMasterRecordInfo->iTSUResumeStatus = 0;
                            pMasterRecordInfo->iTSUAlarmRecordStatus = 0;
                            pMasterRecordInfo->record_status = RECORD_STATUS_INIT;
                            pMasterRecordInfo->record_start_time = 0;
                        }
                    }
                    else
                    {
                        //printf("check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Del 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);

                        /* ɾ����ʶ */
                        pMasterRecordInfo->del_mark = 0;
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Set Record Info Del Mark Zero 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                    }

                    record_info_free(pRecordInfo);
                    pRecordInfo = NULL;
                    continue;
                }
            }

            /* ��ӵ����� */
            pos = record_info_add(pRecordInfo);

            if (pos < 0)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Add Error 1");
                record_info_free(pRecordInfo);
                pRecordInfo = NULL;
                continue;
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_daily_upload_pic_record_info_from_db_to_list() New Daily Upload pic Record Info 1:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
            }
        }
        else
        {
            if (pOldRecordInfo->del_mark == 1) /* ԭ����Ҫɾ�� */
            {
                /* �����Ѿ����ڱ���¼�񣬾Ͳ���Ҫ������¼����Ϣ */
                pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_ALARM);

                if (NULL != pMasterRecordInfo)
                {
                    if (pMasterRecordInfo->del_mark == 0)
                    {
                        /* ����״̬����� */
                        if (1 == pMasterRecordInfo->record_enable)
                        {
                            /* OldRecordInfo ɾ����ʶ����Ϊ0 */
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Has Exist 3:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                        }
                        else /* ����״̬����� */
                        {
                            /* ��ӵ����� */
                            pos = record_info_add(pRecordInfo);

                            if (pos < 0)
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Add Error 2");
                                record_info_free(pRecordInfo);
                                pRecordInfo = NULL;
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_daily_upload_pic_record_info_from_db_to_list() New Daily Upload pic Record Info 2:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                            }

                            continue;
                        }
                    }
                    else /* Ҫɾ��������� */
                    {
                        /* ��ӵ����� */
                        pos = record_info_add(pRecordInfo);

                        if (pos < 0)
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Add Error 3");
                            record_info_free(pRecordInfo);
                            pRecordInfo = NULL;
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_daily_upload_pic_record_info_from_db_to_list() New Daily Upload pic Record Info 3:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                        }

                        continue;
                    }
                }
                else
                {
                    /* �����Ѿ�����ͼƬ�ϴ�¼��ԭ���ϵ�ͼƬ�ϴ���Ϣ��Ҫɾ����  */
                    pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

                    if (NULL != pMasterRecordInfo)
                    {
                        if (pMasterRecordInfo->del_mark == 0)
                        {
                            /* ����״̬����� */
                            if (1 == pMasterRecordInfo->record_enable)
                            {
                                /* OldRecordInfo ɾ����ʶ����Ϊ0 */
                                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Has Exist 4:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                            }
                            else /* ����״̬����� */
                            {
                                /* ��ӵ����� */
                                pos = record_info_add(pRecordInfo);

                                if (pos < 0)
                                {
                                    DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Add Error 4");
                                    record_info_free(pRecordInfo);
                                    pRecordInfo = NULL;
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_daily_upload_pic_record_info_from_db_to_list() New Daily Upload pic Record Info 4:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                                }

                                continue;
                            }
                        }
                        else /* Ҫɾ��������� */
                        {
                            pos = record_info_add(pRecordInfo);

                            if (pos < 0)
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Add Error 5");
                                record_info_free(pRecordInfo);
                                pRecordInfo = NULL;
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_daily_upload_pic_record_info_from_db_to_list() New Daily Upload pic Record Info 5:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                            }

                            continue;
                        }
                    }
                    else
                    {
                        /* ��ӵ����� */
                        pos = record_info_add(pRecordInfo);

                        if (pos < 0)
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Add Error 6");
                            record_info_free(pRecordInfo);
                            pRecordInfo = NULL;
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_daily_upload_pic_record_info_from_db_to_list() New Daily Upload pic Record Info 6:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                        }

                        continue;
                    }
                }
            }
            else
            {
                if (pOldRecordInfo->record_enable == 0) /* ԭ���ı������� */
                {
                    /* �����Ѿ����ڱ���¼�񣬾Ͳ���Ҫ������¼����Ϣ */
                    pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_ALARM);

                    if (NULL != pMasterRecordInfo)
                    {
                        if (pMasterRecordInfo->del_mark == 0)
                        {
                            /* ����״̬����� */
                            if (1 == pMasterRecordInfo->record_enable)
                            {
                                /* OldRecordInfo ɾ����ʶ����Ϊ0 */
                                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Has Exist 5:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                            }
                            else /* ����״̬����� */
                            {
                                /* ��ӵ����� */
                                pos = record_info_add(pRecordInfo);

                                if (pos < 0)
                                {
                                    DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Add Error 7");
                                    record_info_free(pRecordInfo);
                                    pRecordInfo = NULL;
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_daily_upload_pic_record_info_from_db_to_list() New Daily Upload pic Record Info 7:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                                }

                                continue;
                            }
                        }
                        else /* Ҫɾ��������� */
                        {
                            /* ��ӵ����� */
                            pos = record_info_add(pRecordInfo);

                            if (pos < 0)
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Add Error 8");
                                record_info_free(pRecordInfo);
                                pRecordInfo = NULL;
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_daily_upload_pic_record_info_from_db_to_list() New Daily Upload pic Record Info 8:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                            }

                            continue;
                        }
                    }
                    else
                    {
                        /* �����Ѿ�����ͼƬ�ϴ�¼��ԭ���ϵ�ͼƬ�ϴ���Ϣ��Ҫɾ����  */
                        pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

                        if (NULL != pMasterRecordInfo)
                        {
                            if (pMasterRecordInfo->del_mark == 0)
                            {
                                /* ����״̬����� */
                                if (1 == pMasterRecordInfo->record_enable)
                                {
                                    /* OldRecordInfo ɾ����ʶ����Ϊ0 */
                                    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Has Exist 6:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                                }
                                else /* ����״̬����� */
                                {
                                    /* ��ӵ����� */
                                    pos = record_info_add(pRecordInfo);

                                    if (pos < 0)
                                    {
                                        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Add Error 9");
                                        record_info_free(pRecordInfo);
                                        pRecordInfo = NULL;
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_daily_upload_pic_record_info_from_db_to_list() New Daily Upload pic Record Info 9:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                                    }

                                    continue;
                                }
                            }
                            else /* Ҫɾ��������� */
                            {
                                pos = record_info_add(pRecordInfo);

                                if (pos < 0)
                                {
                                    DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Add Error 10");
                                    record_info_free(pRecordInfo);
                                    pRecordInfo = NULL;
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_daily_upload_pic_record_info_from_db_to_list() New Daily Upload pic Record Info 10:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                                }

                                continue;
                            }
                        }
                        else
                        {
                            /* ��ӵ����� */
                            pos = record_info_add(pRecordInfo);

                            if (pos < 0)
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Add Error 11");
                                record_info_free(pRecordInfo);
                                pRecordInfo = NULL;
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_daily_upload_pic_record_info_from_db_to_list() New Daily Upload pic Record Info 11:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                            }

                            continue;
                        }
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Has Exist 7:device_index=%u, record_type=%d, stream_type=%d\r\n", pOldRecordInfo->device_index, pOldRecordInfo->record_type, pOldRecordInfo->stream_type);
                }
            }

            record_info_free(pRecordInfo);
            pRecordInfo = NULL;
            continue;
        }
    }
    while (pRecord_Srv_dboper->MoveNext() >= 0);

    return ret;
}

/*****************************************************************************
 �� �� ��  : check_shdb_alarm_upload_pic_record_info_from_db_to_list
 ��������  : �����Ϻ��ر걨����ͼ������¼��������Ϣ
 �������  : DBOper* pRecord_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��3��22��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_shdb_alarm_upload_pic_record_info_from_db_to_list(DBOper* pRecord_Srv_dboper)
{
    int ret = 0;
    string strSQL = "";
    int record_count = 0;
    record_info_t* pOldRecordInfo = NULL;
    record_info_t* pMasterRecordInfo = NULL;
    int while_count = 0;
    int pos = -1;

    if (NULL == pRecord_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() exit---: Record Srv db Oper Error \r\n");
        return -1;
    }

    strSQL.clear();
    strSQL = "SELECT DISTINCT DeviceIndex FROM PlanActionConfig WHERE TYPE=5 ORDER BY DeviceIndex ASC"; /* ����Ԥ����������Ϊ5�Ľ�ͼ�ϴ������뵽¼����� */

    record_count = pRecord_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() ErrorMsg=%s\r\n", pRecord_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() exit---: No Record Count \r\n");
        return 0;
    }

    /* ѭ���������ݿ�*/
    do
    {
        record_info_t* pRecordInfo = NULL;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() While Count=%d \r\n", while_count);
        }

        int i_ret = record_info_init(&pRecordInfo);

        if (i_ret != 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() record_info_init:i_ret=%d \r\n", i_ret);
            continue;
        }

        unsigned int tmp_uivalue = 0;

        /* �����̶�Ϊ0 */
        pRecordInfo->uID = 0;

        /* �߼��豸ͳ����*/
        tmp_uivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("DeviceIndex", tmp_uivalue);

        pRecordInfo->device_index = tmp_uivalue;

        /* �Ƿ�����¼��*/
        pRecordInfo->record_enable = 1;

        /* ¼������ */
        pRecordInfo->record_days = 1;

        /* ¼��ʱ�� */
        pRecordInfo->record_timeLen = 10;

        /* ¼������ */
        pRecordInfo->record_type = EV9000_RECORD_TYPE_NORMAL;

        /* �������� */
        pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;

        /* �Ƿ�ָ��¼�� */
        pRecordInfo->assign_record = 0;

        /* �Ƿ�ȫ��¼�� */
        pRecordInfo->TimeOfAllWeek = 0;

        /* ǰ�˴��� */
        pRecordInfo->bandwidth = 1;

        pRecordInfo->del_mark = 0;

        /* ����¼�����ͺ��߼��豸��������¼����Ϣ */
        pOldRecordInfo = record_info_get_by_stream_type_and_record_type(pRecordInfo->device_index, pRecordInfo->stream_type, pRecordInfo->record_type);

        if (NULL == pOldRecordInfo)
        {
            /* �����Ѿ����ڱ���¼�񣬾Ͳ���Ҫ������¼����Ϣ */
            pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_ALARM);

            if (NULL != pMasterRecordInfo)
            {
                if (pMasterRecordInfo->del_mark == 0)
                {
                    /* ����״̬����� */
                    if (1 == pMasterRecordInfo->record_enable)
                    {
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Has Exist 1:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                        record_info_free(pRecordInfo);
                        pRecordInfo = NULL;
                        continue;
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Not Enable 1:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Del 1:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                }
            }
            else
            {
                /* �����Ѿ�����ͼƬ�ϴ�¼�񣬾Ͳ���Ҫ������ͼƬ¼����Ϣ */
                pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

                if (NULL != pMasterRecordInfo)
                {
                    if (pMasterRecordInfo->del_mark == 0)
                    {
                        /* ����״̬����� */
                        if (1 == pMasterRecordInfo->record_enable)
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Has Exist 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                        }
                        else
                        {
                            //printf("check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Not Enable 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);

                            /* ���ñ�ʶ */
                            pMasterRecordInfo->record_enable = 1;

                            /* ɾ����ʶ */
                            pMasterRecordInfo->del_mark = 0;
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Set Record Info Del Mark Zero 1:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);

                            pMasterRecordInfo->uID = pRecordInfo->uID;
                            pMasterRecordInfo->assign_record = pRecordInfo->assign_record;
                            pMasterRecordInfo->assign_tsu_index = pRecordInfo->assign_tsu_index;
                            pMasterRecordInfo->record_days = pRecordInfo->record_days;
                            pMasterRecordInfo->record_timeLen = pRecordInfo->record_timeLen;
                            pMasterRecordInfo->record_type = pRecordInfo->record_type;
                            pMasterRecordInfo->bandwidth = pRecordInfo->bandwidth;
                            pMasterRecordInfo->TimeOfAllWeek = pRecordInfo->TimeOfAllWeek;

                            pMasterRecordInfo->tsu_index = -1;
                            pMasterRecordInfo->record_cr_index = -1;
                            pMasterRecordInfo->record_retry_interval = 5;
                            pMasterRecordInfo->record_try_count = 0;
                            pMasterRecordInfo->iTSUPauseStatus = 0;
                            pMasterRecordInfo->iTSUResumeStatus = 0;
                            pMasterRecordInfo->iTSUAlarmRecordStatus = 0;
                            pMasterRecordInfo->record_status = RECORD_STATUS_INIT;
                            pMasterRecordInfo->record_start_time = 0;
                        }
                    }
                    else
                    {
                        //printf("check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Del 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);

                        /* ɾ����ʶ */
                        pMasterRecordInfo->del_mark = 0;
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Set Record Info Del Mark Zero 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                    }

                    record_info_free(pRecordInfo);
                    pRecordInfo = NULL;
                    continue;
                }
            }

            /* ��ӵ����� */
            pos = record_info_add(pRecordInfo);

            if (pos < 0)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Add Error 1");
                record_info_free(pRecordInfo);
                pRecordInfo = NULL;
                continue;
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() New Alarm Upload pic Record Info 1:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
            }
        }
        else
        {
            if (pOldRecordInfo->del_mark == 1) /* ԭ����Ҫɾ�� */
            {
                /* �����Ѿ����ڱ���¼�񣬾Ͳ���Ҫ������¼����Ϣ */
                pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_ALARM);

                if (NULL != pMasterRecordInfo)
                {
                    if (pMasterRecordInfo->del_mark == 0)
                    {
                        /* ����״̬����� */
                        if (1 == pMasterRecordInfo->record_enable)
                        {
                            /* OldRecordInfo ɾ����ʶ����Ϊ0 */
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Has Exist 3:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                        }
                        else /* ����״̬����� */
                        {
                            /* ��ӵ����� */
                            pos = record_info_add(pRecordInfo);

                            if (pos < 0)
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Add Error 2");
                                record_info_free(pRecordInfo);
                                pRecordInfo = NULL;
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() New Alarm Upload pic Record Info 2:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                            }

                            continue;
                        }
                    }
                    else /* Ҫɾ��������� */
                    {
                        /* ��ӵ����� */
                        pos = record_info_add(pRecordInfo);

                        if (pos < 0)
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Add Error 3");
                            record_info_free(pRecordInfo);
                            pRecordInfo = NULL;
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() New Alarm Upload pic Record Info 3:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                        }

                        continue;
                    }
                }
                else
                {
                    /* �����Ѿ�����ͼƬ�ϴ�¼��ԭ���ϵ�ͼƬ�ϴ���Ϣ��Ҫɾ����  */
                    pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

                    if (NULL != pMasterRecordInfo)
                    {
                        if (pMasterRecordInfo->del_mark == 0)
                        {
                            /* ����״̬����� */
                            if (1 == pMasterRecordInfo->record_enable)
                            {
                                /* OldRecordInfo ɾ����ʶ����Ϊ0 */
                                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Has Exist 4:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                            }
                            else /* ����״̬����� */
                            {
                                /* ��ӵ����� */
                                pos = record_info_add(pRecordInfo);

                                if (pos < 0)
                                {
                                    DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Add Error 4");
                                    record_info_free(pRecordInfo);
                                    pRecordInfo = NULL;
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() New Alarm Upload pic Record Info 4:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                                }

                                continue;
                            }
                        }
                        else /* Ҫɾ��������� */
                        {
                            pos = record_info_add(pRecordInfo);

                            if (pos < 0)
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Add Error 5");
                                record_info_free(pRecordInfo);
                                pRecordInfo = NULL;
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() New Alarm Upload pic Record Info 5:device_index=%u, record_type=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type);
                            }

                            continue;
                        }
                    }
                    else
                    {
                        /* ��ӵ����� */
                        pos = record_info_add(pRecordInfo);

                        if (pos < 0)
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Add Error 6");
                            record_info_free(pRecordInfo);
                            pRecordInfo = NULL;
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() New Alarm Upload pic Record Info 6:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                        }

                        continue;
                    }
                }
            }
            else
            {
                if (pOldRecordInfo->record_enable == 0) /* ԭ���ı������� */
                {
                    /* �����Ѿ����ڱ���¼�񣬾Ͳ���Ҫ������¼����Ϣ */
                    pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_ALARM);

                    if (NULL != pMasterRecordInfo)
                    {
                        if (pMasterRecordInfo->del_mark == 0)
                        {
                            /* ����״̬����� */
                            if (1 == pMasterRecordInfo->record_enable)
                            {
                                /* OldRecordInfo ɾ����ʶ����Ϊ0 */
                                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Has Exist 5:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                            }
                            else /* ����״̬����� */
                            {
                                /* ��ӵ����� */
                                pos = record_info_add(pRecordInfo);

                                if (pos < 0)
                                {
                                    DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Add Error 7");
                                    record_info_free(pRecordInfo);
                                    pRecordInfo = NULL;
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() New Alarm Upload pic Record Info 7:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                                }

                                continue;
                            }
                        }
                        else /* Ҫɾ��������� */
                        {
                            /* ��ӵ����� */
                            pos = record_info_add(pRecordInfo);

                            if (pos < 0)
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Add Error 8");
                                record_info_free(pRecordInfo);
                                pRecordInfo = NULL;
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() New Alarm Upload pic Record Info 8:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                            }

                            continue;
                        }
                    }
                    else
                    {
                        /* �����Ѿ�����ͼƬ�ϴ�¼��ԭ���ϵ�ͼƬ�ϴ���Ϣ��Ҫɾ����  */
                        pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

                        if (NULL != pMasterRecordInfo)
                        {
                            if (pMasterRecordInfo->del_mark == 0)
                            {
                                /* ����״̬����� */
                                if (1 == pMasterRecordInfo->record_enable)
                                {
                                    /* OldRecordInfo ɾ����ʶ����Ϊ0 */
                                    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Has Exist 6:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                                }
                                else /* ����״̬����� */
                                {
                                    /* ��ӵ����� */
                                    pos = record_info_add(pRecordInfo);

                                    if (pos < 0)
                                    {
                                        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Add Error 9");
                                        record_info_free(pRecordInfo);
                                        pRecordInfo = NULL;
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() New Alarm Upload pic Record Info 9:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                                    }

                                    continue;
                                }
                            }
                            else /* Ҫɾ��������� */
                            {
                                pos = record_info_add(pRecordInfo);

                                if (pos < 0)
                                {
                                    DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Add Error 10");
                                    record_info_free(pRecordInfo);
                                    pRecordInfo = NULL;
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() New Alarm Upload pic Record Info 10:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                                }

                                continue;
                            }
                        }
                        else
                        {
                            /* ��ӵ����� */
                            pos = record_info_add(pRecordInfo);

                            if (pos < 0)
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Add Error 11");
                                record_info_free(pRecordInfo);
                                pRecordInfo = NULL;
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() New Alarm Upload pic Record Info 11:device_index=%u, record_type=%d, pos=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pos);
                            }

                            continue;
                        }
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Has Exist 7:device_index=%u, record_type=%d, stream_type=%d\r\n", pOldRecordInfo->device_index, pOldRecordInfo->record_type, pOldRecordInfo->stream_type);
                }
            }

            record_info_free(pRecordInfo);
            pRecordInfo = NULL;
            continue;
        }
    }
    while (pRecord_Srv_dboper->MoveNext() >= 0);

    return ret;
}

/*****************************************************************************
 �� �� ��  : scan_record_info_list
 ��������  : ɨ��¼��������Ϣ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��16��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_record_info_list()
{
    int i = 0;
    int record_count = 0;
    int has_pos = 0;
    record_info_t* pRecordInfo = NULL;
    record_info_t* pHasRecordInfo = NULL;
    record_info_t* pProcRecordInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    int record_cr_index = -1;
    needtoproc_recordinfo_queue needrecord;
    needtoproc_recordinfo_queue needstoprecord;
    needtoproc_recordinfo_queue needresumerecord;
    needtoproc_recordinfo_queue needpauserecord;
    cr_t* pCrData = NULL;

    if ((NULL == g_RecordInfoList) || (NULL == g_RecordInfoList->pRecordInfoList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "scan_record_info_list() exit---: Param Error \r\n");
        return;
    }

    needrecord.clear();
    needstoprecord.clear();
    needresumerecord.clear();
    needpauserecord.clear();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "¼����������, ��ǰ¼��λ��:current_record_pos=%d", current_record_pos);

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        return;
    }

    /*  �Ժ����Ӵ����ݿ�������¼������������µĵ�λ���Լ�ʱ¼�� */
    printf("scan_record_info_list() Begin--- current_record_pos=%d \r\n", current_record_pos);

    for (i = current_record_pos; i < osip_list_size(g_RecordInfoList->pRecordInfoList); i++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, i);

        if (NULL == pRecordInfo)
        {
            continue;
        }

        if (1 == pRecordInfo->del_mark) /* ��Ҫɾ���� */
        {
            if (pRecordInfo->record_cr_index >= 0) /* �Ѿ�¼��� */
            {
                needstoprecord.push_back(pRecordInfo);
            }

            continue;
        }
        else if (2 == pRecordInfo->del_mark) /* ��Ҫ��ֹͣ���� */
        {
            if (pRecordInfo->record_cr_index >= 0) /* �Ѿ�¼��� */
            {
                needstoprecord.push_back(pRecordInfo);
                pRecordInfo->del_mark = 0;
            }

            continue;
        }
        else if (3 == pRecordInfo->del_mark) /* ��Ҫ֪ͨTSU�ָ� */
        {
            if (pRecordInfo->record_cr_index >= 0) /* �Ѿ�¼��� */
            {
                needresumerecord.push_back(pRecordInfo);
                pRecordInfo->del_mark = 0;
            }

            continue;
        }
        else if (4 == pRecordInfo->del_mark) /* ��Ҫ֪ͨTSU�ָ� */
        {
            if (pRecordInfo->record_cr_index >= 0) /* �Ѿ�¼��� */
            {
                needpauserecord.push_back(pRecordInfo);
                pRecordInfo->del_mark = 0;
            }

            continue;
        }

        if (0 == pRecordInfo->record_enable) /* δ���õ� */
        {
            if (pRecordInfo->record_cr_index >= 0) /* �Ѿ�¼��� */
            {
                needstoprecord.push_back(pRecordInfo);
            }

            continue;
        }

        if (pRecordInfo->record_cr_index >= 0) /* �Ѿ�¼��� */
        {
            /* �鿴��record_index�Ƿ�����RecordInfoʹ�ã����ʹ�ã����ͷ� */
            has_pos = find_is_cr_index_used_by_other_record_info(pRecordInfo->record_cr_index, i);

            if (has_pos >= 0)
            {
                needstoprecord.push_back(pRecordInfo);

                pHasRecordInfo = record_info_get(has_pos);

                if (NULL != pHasRecordInfo)
                {
                    needstoprecord.push_back(pHasRecordInfo);
                }
            }
            else
            {
                /* �����߼��豸��Ϣ */
                pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index2(pRecordInfo->device_index);

                if (NULL != pGBLogicDeviceInfo)
                {
                    if (1 == pGBLogicDeviceInfo->record_type) /* ǰ��¼�� */
                    {
                        needstoprecord.push_back(pRecordInfo);
                    }
                }
            }

            /* ����TSU�Ƿ�����,��������ֹͣ������¼�� */
            if (RECORD_STATUS_COMPLETE == pRecordInfo->record_status
                && pRecordInfo->tsu_index < 0)
            {
                needstoprecord.push_back(pRecordInfo);
            }

            /* ����ȫ��¼�񣬿�TSU״̬�Ƿ���ͣ�����û����ͣ������Ҫ�ٴ���ͣ */ //��̫����

            if (RECORD_STATUS_COMPLETE == pRecordInfo->record_status
                && 0 == pRecordInfo->TimeOfAllWeek)
            {
                if (0 == pRecordInfo->iTSUPauseStatus)
                {
                    needpauserecord.push_back(pRecordInfo);
                }
            }

            /* ȫ��¼�񣬿�TSU״̬�Ƿ�ָ������û�лָ�������Ҫ�ٴλָ� */
            if (RECORD_STATUS_COMPLETE == pRecordInfo->record_status
                && 1 == pRecordInfo->TimeOfAllWeek)
            {
                if (0 == pRecordInfo->iTSUResumeStatus)
                {
                    needresumerecord.push_back(pRecordInfo);
                }
            }

            continue;
        }
        else /* û��¼�� */
        {
            /* �����߼��豸��Ϣ */
            pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index2(pRecordInfo->device_index);

            if (NULL != pGBLogicDeviceInfo)
            {
                if (1 == pGBLogicDeviceInfo->record_type) /* ǰ��¼�� */
                {
                    continue;
                }
            }
        }

        /* ¼����Ա������û��¼��ĵ�λ����ȫ������¼�� */
        if (pRecordInfo->record_retry_interval > 0)
        {
            pRecordInfo->record_retry_interval--;
            continue;
        }

        //printf("\r\n needrecord.push_back:pos=%d \r\n", i);
        needrecord.push_back(pRecordInfo);
        record_count++;

        if (record_count >= 100)
        {
            current_record_pos = i + 1;
            record_count = 0;
            break;
        }
    }

    if (i >= osip_list_size(g_RecordInfoList->pRecordInfoList) - 1)
    {
        current_record_pos = 0;
    }

    RECORD_INFO_SMUTEX_UNLOCK();

    printf("scan_record_info_list() needrecord=%d, needstoprecord=%d, needresumerecord=%d, needpauserecord=%d \r\n", needrecord.size(), needstoprecord.size(), needresumerecord.size(), needpauserecord.size());
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "¼������������ʼ����, ������Ҫ����¼�����������=%d, ������Ҫֹͣ¼�����������=%d, ������Ҫ֪ͨTSU�ָ�¼�����������=%d, ������Ҫ֪ͨTSU��ͣ¼�����������=%d", needrecord.size(), needstoprecord.size(), needresumerecord.size(), needpauserecord.size());

    /* ������Ҫ¼��� */
    while (!needrecord.empty())
    {
        pProcRecordInfo = (record_info_t*) needrecord.front();
        needrecord.pop_front();

        if (NULL != pProcRecordInfo)
        {
            record_cr_index = StartDeviceRecord(pProcRecordInfo);
            printf("scan_record_info_list() StartDeviceRecord---:device_index=%u, record_type=%d, stream_type=%d, del_mark=%d, tsu_index=%d, record_cr_index=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pRecordInfo->stream_type, pRecordInfo->del_mark, pRecordInfo->tsu_index, record_cr_index);

            if (record_cr_index >= 0)
            {
                pProcRecordInfo->record_retry_interval = 5;
                pProcRecordInfo->record_try_count = 0;
                pProcRecordInfo->iTSUPauseStatus = 0;
                pProcRecordInfo->iTSUResumeStatus = 0;
                pProcRecordInfo->iTSUAlarmRecordStatus = 0;
            }
            else
            {
                pProcRecordInfo->tsu_index = -1;
                pProcRecordInfo->iTSUPauseStatus = 0;
                pProcRecordInfo->iTSUResumeStatus = 0;
                pProcRecordInfo->iTSUAlarmRecordStatus = 0;

                if (-2 == record_cr_index)
                {
                    pProcRecordInfo->record_status = RECORD_STATUS_OFFLINE;
                }
                else if (-3 == record_cr_index)
                {
                    pProcRecordInfo->record_status = RECORD_STATUS_NOSTREAM;
                }
                else if (-5 == record_cr_index)
                {
                    pProcRecordInfo->record_status = RECORD_STATUS_NETWORK_ERROR;
                }
                else if (-4 == record_cr_index)
                {
                    pProcRecordInfo->record_status = RECORD_STATUS_NO_TSU;
                }
                else if (-6 == record_cr_index)
                {
                    pProcRecordInfo->record_status = RECORD_STATUS_NOT_SUPPORT_MULTI_STREAM;
                }
                else
                {
                    pProcRecordInfo->record_status = RECORD_STATUS_INIT;
                }

                pProcRecordInfo->record_start_time = 0;

                pProcRecordInfo->record_try_count++;

                if (pProcRecordInfo->record_try_count >= 3)
                {
                    pProcRecordInfo->record_try_count = 0;
                    pProcRecordInfo->record_retry_interval = 5;
                }
            }

            osip_usleep(5000);
        }
    }

    if (!needrecord.empty())
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "scan_record_info_list() Start Device Record:Count=%d\r\n ", needrecord.size());
    }

    needrecord.clear();
    printf("scan_record_info_list() needrecord End \r\n");

    /* ������Ҫֹͣ¼��� */
    while (!needstoprecord.empty())
    {
        pProcRecordInfo = (record_info_t*) needstoprecord.front();
        needstoprecord.pop_front();

        if (NULL != pProcRecordInfo)
        {
            i = StopDeviceRecord(pProcRecordInfo->record_cr_index);

            pProcRecordInfo->tsu_index = -1;
            pProcRecordInfo->record_cr_index = -1;
            pProcRecordInfo->record_status = RECORD_STATUS_INIT;
            pProcRecordInfo->record_start_time = 0;
            pProcRecordInfo->record_try_count = 0;
            pProcRecordInfo->record_retry_interval = 5;
            pProcRecordInfo->iTSUPauseStatus = 0;
            pProcRecordInfo->iTSUResumeStatus = 0;
            pProcRecordInfo->iTSUAlarmRecordStatus = 0;
        }
    }

    needstoprecord.clear();
    printf("scan_record_info_list() needstoprecord End \r\n");

    /* ������Ҫ�ָ��� */
    while (!needresumerecord.empty())
    {
        pProcRecordInfo = (record_info_t*) needresumerecord.front();
        needresumerecord.pop_front();

        if (NULL != pProcRecordInfo)
        {
            pCrData = call_record_get(pProcRecordInfo->record_cr_index);

            if (NULL != pCrData)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "¼����Դ�ʱ���¼���޸�Ϊȫ��¼��֪ͨTSU�ָ�¼��: ��λID=%s, TSU IP��ַ=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video strategy change from selected time slot record to weekly record��notify TSU to recover videro: point ID=%s, TSU IPaddress=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);

                /* �ָ�¼�� */
                i = notify_tsu_resume_record(pCrData->tsu_ip, pCrData->task_id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "scan_record_info_list() notify_tsu_resume_record Error: tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    pProcRecordInfo->iTSUResumeStatus = 0;
                }
                else
                {
                    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "scan_record_info_list() notify_tsu_resume_record OK: tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    pProcRecordInfo->iTSUResumeStatus = 1;
                }
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "scan_record_info_list() call_record_get Error: record_cr_index=%d \r\n", pProcRecordInfo->record_cr_index);
            }
        }
    }

    needresumerecord.clear();
    printf("scan_record_info_list() needresumerecord End \r\n");

    /* ������Ҫ��ͣ�� */
    while (!needpauserecord.empty())
    {
        pProcRecordInfo = (record_info_t*) needpauserecord.front();
        needpauserecord.pop_front();

        if (NULL != pProcRecordInfo)
        {
            pCrData = call_record_get(pProcRecordInfo->record_cr_index);

            if (NULL != pCrData)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "¼����Դ�ȫ��¼���޸�Ϊʱ���¼��֪ͨTSU��ͣ¼��: ��λID=%s, TSU IP��ַ=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video strategy change from weekly record to selected time slot record, notify TSU to pause video: point ID=%s, TSU IP address=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);

                /* ��ͣ¼�� */
                i = notify_tsu_pause_record(pCrData->tsu_ip, pCrData->task_id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "scan_record_info_list() notify_tsu_pause_record Error: tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    pProcRecordInfo->iTSUPauseStatus = 0;
                }
                else
                {
                    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "scan_record_info_list() notify_tsu_pause_record OK: tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    pProcRecordInfo->iTSUPauseStatus = 1;
                }

                /* ȥ�������еı�־λ */
                i = RemoveRecordTimeSchedMark(pProcRecordInfo->uID);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "scan_record_info_list() call_record_get Error: record_cr_index=%d \r\n", pProcRecordInfo->record_cr_index);
            }
        }
    }

    needpauserecord.clear();
    printf("scan_record_info_list() End--- \r\n");

    return;
}

/*****************************************************************************
 �� �� ��  : scan_record_info_list_for_monitor_print
 ��������  : ɨ��¼��������Ϣ����
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��16��
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void scan_record_info_list_for_monitor_print()
{
    int i = 0;
    int other_cr_pos = -1;
    char* tsu_ip = NULL;
    needtoproc_recordinfo_queue needtoproc;
    record_info_t* pProcRecordInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    tsu_resource_info_t* pTsuResourceInfo = NULL;
    needtoproc_recordinfo_queue not_enable_record_info; /* û�����õ� */
    needtoproc_recordinfo_queue front_record_info; /* ǰ��¼��� */
    needtoproc_recordinfo_queue has_record_info; /* ����¼��� */
    needtoproc_recordinfo_queue not_record_info; /* û��¼��� */
    cr_t* pCrData = NULL;

    if ((NULL == g_RecordInfoList) || (NULL == g_RecordInfoList->pRecordInfoList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "scan_record_info_list_for_monitor_print() exit---: Param Error \r\n");
        return;
    }

    not_enable_record_info.clear();/* û�����õ� */
    front_record_info.clear();/* ǰ��¼��� */
    has_record_info.clear(); /* ����¼��� */
    not_record_info.clear(); /* û��¼��� */

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_RecordInfoList->pRecordInfoList); i++)
    {
        pProcRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, i);

        if (NULL == pProcRecordInfo)
        {
            continue;
        }

        needtoproc.push_back(pProcRecordInfo);
    }

    RECORD_INFO_SMUTEX_UNLOCK();

    while (!needtoproc.empty())
    {
        pProcRecordInfo = (record_info_t*) needtoproc.front();
        needtoproc.pop_front();

        if (NULL != pProcRecordInfo)
        {
            /* �����߼��豸��Ϣ */
            pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(pProcRecordInfo->device_index);

            if (NULL == pGBLogicDeviceInfo)
            {
                continue;
            }

            if (0 == pProcRecordInfo->record_enable)
            {
                not_enable_record_info.push_back(pProcRecordInfo);
            }
            else if (1 == pGBLogicDeviceInfo->record_type)
            {
                front_record_info.push_back(pProcRecordInfo);
            }
            else
            {
                if (RECORD_STATUS_COMPLETE == pProcRecordInfo->record_status)
                {
                    has_record_info.push_back(pProcRecordInfo);
                }
                else
                {
                    not_record_info.push_back(pProcRecordInfo);
                }
            }
        }
    }

    needtoproc.clear();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ϵͳ��ǰ¼��������Ϣ:���õ���¼������=%d, ��¼��¼��������=%d, δ¼��¼��������=%d, δ���õ�¼��������=%d, ǰ��¼���������=%d", osip_list_size(g_RecordInfoList->pRecordInfoList), has_record_info.size(), not_record_info.size(), not_enable_record_info.size(), front_record_info.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "System Current Reocrd Task:Total Record Task=%d, Has Record Task=%d, Not Record Task=%d, Not Enable Record Task=%d, Front Record Task=%d", osip_list_size(g_RecordInfoList->pRecordInfoList), has_record_info.size(), not_record_info.size(), not_enable_record_info.size(), front_record_info.size());

    if (!has_record_info.empty())
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "��¼��¼��������=%d, ��ϸ��Ϣ����:", has_record_info.size());
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Has Record Task=%d, Detail Info As Flow:", has_record_info.size());

        while (!has_record_info.empty())
        {
            pProcRecordInfo = (record_info_t*) has_record_info.front();
            has_record_info.pop_front();

            if (NULL != pProcRecordInfo)
            {
                /* �����߼��豸��Ϣ */
                pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(pProcRecordInfo->device_index);

                if (NULL == pGBLogicDeviceInfo)
                {
                    continue;
                }

                /* ����TSU��Ϣ��Ϣ */
                pTsuResourceInfo = tsu_resource_info_get(pProcRecordInfo->tsu_index);

                if (NULL == pTsuResourceInfo)
                {
                    continue;
                }

                tsu_ip = get_tsu_ip(pTsuResourceInfo, default_eth_name_get());

                if (NULL == tsu_ip)
                {
                    continue;
                }

                /* ���¼�������Ƿ�����  */
                pCrData = call_record_get(pProcRecordInfo->record_cr_index);

                if (NULL == pCrData)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "¼���λID=%s, ��λ����=%s, ¼������=%d, ��������=%d, ¼���TSU ID=%s, TSU IP=%s, ¼��ĺ������񲻴���, �Ƴ�¼����Ϣ", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, pTsuResourceInfo->tsu_device_id, tsu_ip);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Video point ID = % s, point name = % s, video type = % d, code flow type = % d, video TSU ID = % s, TSU IP = % s, video call task does not exist, remove the video information", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, pTsuResourceInfo->tsu_device_id, tsu_ip);

                    pProcRecordInfo->tsu_index = -1;
                    pProcRecordInfo->record_cr_index = -1;
                    pProcRecordInfo->record_status = RECORD_STATUS_INIT;
                    pProcRecordInfo->record_start_time = 0;
                    pProcRecordInfo->record_try_count = 0;
                    pProcRecordInfo->record_retry_interval = 5;
                    pProcRecordInfo->iTSUPauseStatus = 0;
                    pProcRecordInfo->iTSUResumeStatus = 0;
                    pProcRecordInfo->iTSUAlarmRecordStatus = 0;
                    continue;
                }

                if (CALL_TYPE_RECORD != pCrData->call_type)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "¼���λID=%s, ��λ����=%s, ¼������=%d, ��������=%d, ¼���TSU ID=%s, TSU IP=%s, ¼��ĺ����������Ͳ�ƥ��, �Ƴ�¼����Ϣ", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, pTsuResourceInfo->tsu_device_id, tsu_ip);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Video point ID = % s, point name = % s, video type = % d, code flow type = % d, video TSU ID = % s, TSU IP = % s, video call task type mismatch, remove the video information", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, pTsuResourceInfo->tsu_device_id, tsu_ip);

                    pProcRecordInfo->tsu_index = -1;
                    pProcRecordInfo->record_cr_index = -1;
                    pProcRecordInfo->record_status = RECORD_STATUS_INIT;
                    pProcRecordInfo->record_start_time = 0;
                    pProcRecordInfo->record_try_count = 0;
                    pProcRecordInfo->record_retry_interval = 5;
                    pProcRecordInfo->iTSUPauseStatus = 0;
                    pProcRecordInfo->iTSUResumeStatus = 0;
                    pProcRecordInfo->iTSUAlarmRecordStatus = 0;
                    continue;
                }

                if (0 != sstrcmp(pCrData->callee_id, pGBLogicDeviceInfo->device_id))
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "¼���λID=%s, ��λ����=%s, ¼������=%d, ��������=%d, ¼���TSU ID=%s, TSU IP=%s, ¼��ĺ��������е�λID��¼����Ϣ�е�ID��ƥ��, �Ƴ�¼����Ϣ", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, pTsuResourceInfo->tsu_device_id, tsu_ip);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Video point ID = % s, point name = % s, video type = % d, code flow type = % d, video TSU ID = % s, TSU IP = % s, video call mission point ID and ID does not match in the video information, remove the video information", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, pTsuResourceInfo->tsu_device_id, tsu_ip);

                    pProcRecordInfo->tsu_index = -1;
                    pProcRecordInfo->record_cr_index = -1;
                    pProcRecordInfo->record_status = RECORD_STATUS_INIT;
                    pProcRecordInfo->record_start_time = 0;
                    pProcRecordInfo->record_try_count = 0;
                    pProcRecordInfo->record_retry_interval = 5;
                    pProcRecordInfo->iTSUPauseStatus = 0;
                    pProcRecordInfo->iTSUResumeStatus = 0;
                    pProcRecordInfo->iTSUAlarmRecordStatus = 0;
                    continue;
                }

                /* ����Ƿ�������¼��ĺ���������� */
                other_cr_pos = find_recordinfo_has_other_cr_data(pGBLogicDeviceInfo->device_id, pProcRecordInfo->stream_type, pProcRecordInfo->record_cr_index);

                if (other_cr_pos >= 0)
                {
                    i = StopDeviceRecord(other_cr_pos);
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "ȥ�����ظ���¼��������Ϣ:��λID=%s, ��λ����=%s, ¼������=%d, ��������=%d, ¼������record_cr_index=%d, �ظ�������cr_pos=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, pProcRecordInfo->record_cr_index, other_cr_pos);
                }

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "��¼�ĵ�λID=%s, ��λ����=%s, ¼������=%d, ��������=%d, ¼���TSU ID=%s, TSU IP=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, pTsuResourceInfo->tsu_device_id, tsu_ip);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Has Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, TSU ID=%s, TSU IP=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, pTsuResourceInfo->tsu_device_id, tsu_ip);
            }
        }

        has_record_info.clear();
    }

    if (!not_record_info.empty())
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "δ¼��¼��������=%d, ��ϸ��Ϣ����:", not_record_info.size());
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Not Record Task=%d, Detail Info As Flow:", not_record_info.size());

        while (!not_record_info.empty())
        {
            pProcRecordInfo = (record_info_t*) not_record_info.front();
            not_record_info.pop_front();

            if (NULL != pProcRecordInfo)
            {
                /* �����߼��豸��Ϣ */
                pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(pProcRecordInfo->device_index);

                if (NULL == pGBLogicDeviceInfo)
                {
                    continue;
                }

                if (RECORD_STATUS_INIT == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "δ¼�ĵ�λID=%s, ��λ����=%s, ¼������=%d, ��������=%d, δ¼��ԭ��=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"¼��û�гɹ�");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"Not Success");
                }
                else if (RECORD_STATUS_OFFLINE == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "δ¼�ĵ�λID=%s, ��λ����=%s, ¼������=%d, ��������=%d, δ¼��ԭ��=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"��λ������");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"Device Off Line");
                }
                else if (RECORD_STATUS_NOSTREAM == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "δ¼�ĵ�λID=%s, ��λ����=%s, ¼������=%d, ��������=%d, δ¼��ԭ��=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"��λû������");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"Device No Stream");
                }
                else if (RECORD_STATUS_NETWORK_ERROR == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "δ¼�ĵ�λID=%s, ��λ����=%s, ¼������=%d, ��������=%d, δ¼��ԭ��=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"ǰ���豸���粻�ɴ�");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"NetWork Unreached");
                }
                else if (RECORD_STATUS_NO_TSU == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "δ¼�ĵ�λID=%s, ��λ����=%s, ¼������=%d, ��������=%d, δ¼��ԭ��=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"û�п��е�TSU");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"No Idle TSU");
                }
                else if (RECORD_STATUS_NOT_SUPPORT_MULTI_STREAM == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "δ¼�ĵ�λID=%s, ��λ����=%s, ¼������=%d, ��������=%d, δ¼��ԭ��=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"¼��ĵ�λ��֧�ֶ�����");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"No Multi Stream");
                }
                else if (RECORD_STATUS_PROC == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "δ¼�ĵ�λID=%s, ��λ����=%s, ¼������=%d, ��������=%d, δ¼��ԭ��=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"¼������û�н���");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"Recording Proc");
                }
                else if (RECORD_STATUS_NULL == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "δ¼�ĵ�λID=%s, ��λ����=%s, ¼������=%d, ��������=%d, δ¼��ԭ��=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"��û�п�ʼ����¼��");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"Not Start Record");
                }
            }
        }

        not_record_info.clear();
    }

    return;
}

/*****************************************************************************
 �� �� ��  : RecordInfo_db_refresh_proc
 ��������  : ����¼�������Ϣ���ݿ���²�����ʶ
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
int RecordInfo_db_refresh_proc()
{
    if (1 == db_RecordInfo_reload_mark) /* ����ִ�� */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "¼������������ݿ���Ϣ����ͬ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Record Info database information are synchronized");
        return 0;
    }

    db_RecordInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 �� �� ��  : check_RecordInfo_need_to_reload_begin
 ��������  : ����Ƿ���Ҫͬ��¼��������ÿ�ʼ
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
void check_RecordInfo_need_to_reload_begin(DBOper* pDboper)
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_RecordInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ��¼������������ݿ���Ϣ: ��ʼ---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization record info database information: begain---");
    printf("check_RecordInfo_need_to_reload_begin() Begin--- \r\n");

    /* ����¼����е�ɾ����ʶ */
    set_record_info_list_del_mark(1);
    printf("check_RecordInfo_need_to_reload_begin() set_record_info_list_del_mark End--- \r\n");

    /* �����ݿ��еı仯����ͬ�����ڴ� */
    check_record_info_from_db_to_list(pDboper);
    printf("check_RecordInfo_need_to_reload_begin() check_record_info_from_db_to_list End--- \r\n");

    /* ��Ԥ���������еı���¼�����ݿ��еı仯����ͬ�����ڴ� */
    check_plan_action_record_info_from_db_to_list(pDboper);
    printf("check_RecordInfo_need_to_reload_begin() check_plan_action_record_info_from_db_to_list End--- \r\n");

    /* ���Ϻ��ر궨ʱ�����ͼ�ϴ�¼�����ݿ��еı仯����ͬ�����ڴ� */
    check_shdb_daily_upload_pic_record_info_from_db_to_list(pDboper);
    printf("check_RecordInfo_need_to_reload_begin() check_shdb_daily_upload_pic_record_info_from_db_to_list End--- \r\n");

    /* ���Ϻ��ر걨����ͼ�ϴ�¼�����ݿ��еı仯����ͬ�����ڴ� */
    check_shdb_alarm_upload_pic_record_info_from_db_to_list(pDboper);
    printf("check_RecordInfo_need_to_reload_begin() End--- \r\n");

    return;
}

/*****************************************************************************
 �� �� ��  : check_RecordInfo_need_to_reload_end
 ��������  : ����Ƿ���Ҫͬ��¼��������ñ����
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
void check_RecordInfo_need_to_reload_end()
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_RecordInfo_reload_mark)
    {
        return;
    }

    printf("check_RecordInfo_need_to_reload_end() Begin--- \r\n");

    /* ɾ�������¼����Ϣ */
    delete_record_info_from_list_by_mark();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ��¼������������ݿ���Ϣ: ����---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization record info database information: end---");
    db_RecordInfo_reload_mark = 0;
    printf("check_RecordInfo_need_to_reload_end() End--- \r\n");

    return;
}
#endif

#if DECS("¼��ʱ�̲��Զ���")
/*****************************************************************************
 �� �� ��  : record_time_config_init
 ��������  : ʱ����Խṹ���ʼ��
 �������  : record_time_config_t** time_config
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int record_time_config_init(record_time_config_t** time_config)
{
    *time_config = (record_time_config_t*)osip_malloc(sizeof(record_time_config_t));

    if (*time_config == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_config_init() exit---: *time_config Smalloc Error \r\n");
        return -1;
    }

    (*time_config)->uID = 0;
    (*time_config)->iBeginTime = 0;
    (*time_config)->iEndTime = 0;
    (*time_config)->iStatus = 0;
    (*time_config)->del_mark = 0;

    return 0;
}

/*****************************************************************************
 �� �� ��  : record_time_config_free
 ��������  : ʱ����Խṹ���ͷ�
 �������  : record_time_config_t* time_config
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void record_time_config_free(record_time_config_t * time_config)
{
    if (time_config == NULL)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_config_free() exit---: Param Error \r\n");
        return;
    }

    time_config->uID = 0;
    time_config->iBeginTime = 0;
    time_config->iEndTime = 0;
    time_config->iStatus = 0;
    time_config->del_mark = 0;

    osip_free(time_config);
    time_config = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : record_time_config_add
 ��������  : ʱ��������
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
int record_time_config_add(osip_list_t * pDayOfWeekTimeList, unsigned int uID, int iBeginTime, int iEndTime)
{
    int i = 0;
    record_time_config_t* pTimeConfig = NULL;

    if (pDayOfWeekTimeList == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_config_add() exit---: Param Error \r\n");
        return -1;
    }

    if (uID <= 0 || iBeginTime < 0 || iEndTime < 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_config_add() exit---: Param 2 Error \r\n");
        return -1;
    }

    if (iEndTime - iBeginTime <= 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_time_config_add() exit---: Param 3 Error \r\n");
        return -1;
    }

    i = record_time_config_init(&pTimeConfig);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_time_config_add() exit---: Time Config Init Error \r\n");
        return -1;
    }

    pTimeConfig->uID = uID;
    pTimeConfig->iBeginTime = iBeginTime;
    pTimeConfig->iEndTime = iEndTime;
    pTimeConfig->iStatus = 0;
    pTimeConfig->del_mark = 0;

    i = osip_list_add(pDayOfWeekTimeList, pTimeConfig, -1); /* add to list tail */

    if (i == -1)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_time_config_add() exit---: osip_list_add Error \r\n");
        record_time_config_free(pTimeConfig);
        pTimeConfig = NULL;
        return -1;
    }

    return i - 1;
}

/*****************************************************************************
 �� �� ��  : record_time_config_get
 ��������  : ʱ����Ի�ȡ
 �������  : osip_list_t* pDayOfWeekTimeList
             unsigned int uID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
record_time_config_t* record_time_config_get(osip_list_t * pDayOfWeekTimeList, unsigned int uID)
{
    int pos = -1;
    record_time_config_t* pTimeConfig = NULL;

    if (NULL == pDayOfWeekTimeList || uID <= 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_config_find() exit---: Param Error \r\n");
        return NULL;
    }

    if (osip_list_size(pDayOfWeekTimeList) <= 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_time_config_get() exit---: Day Of Week Time List NULL \r\n");
        return NULL;
    }

    for (pos = 0; pos < osip_list_size(pDayOfWeekTimeList); pos++)
    {
        pTimeConfig = (record_time_config_t*)osip_list_get(pDayOfWeekTimeList, pos);

        if ((NULL == pTimeConfig) || (pTimeConfig->uID <= 0))
        {
            continue;
        }

        if (pTimeConfig->uID == uID)
        {
            return pTimeConfig;
        }
    }

    return NULL;
}

/*****************************************************************************
 �� �� ��  : get_record_status_from_record_time_config
 ��������  : ��ȡ�ֶ�¼���¼��״̬
 �������  : osip_list_t * pDayOfWeekTimeList
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��11��3��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int get_record_status_from_record_time_config(osip_list_t * pDayOfWeekTimeList)
{
    int pos = -1;
    record_time_config_t* pTimeConfig = NULL;

    if (NULL == pDayOfWeekTimeList)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_config_find() exit---: Param Error \r\n");
        return 0;
    }

    if (osip_list_size(pDayOfWeekTimeList) <= 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_time_config_get() exit---: Day Of Week Time List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(pDayOfWeekTimeList); pos++)
    {
        pTimeConfig = (record_time_config_t*)osip_list_get(pDayOfWeekTimeList, pos);

        if ((NULL == pTimeConfig) || (pTimeConfig->uID <= 0))
        {
            continue;
        }

        if (pTimeConfig->iStatus == 1)
        {
            return 1;
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : record_time_sched_init
 ��������  : ¼��ʱ�̲��Խṹ��ʼ��
 �������  : record_time_sched_t** record_time_sched
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int record_time_sched_init(record_time_sched_t** record_time_sched)
{
    *record_time_sched = (record_time_sched_t*)osip_malloc(sizeof(record_time_sched_t));

    if (*record_time_sched == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_sched_init() exit---: *record_time_sched Smalloc Error \r\n");
        return -1;
    }

    (*record_time_sched)->uID = 0;
    (*record_time_sched)->record_cr_index = -1;
    (*record_time_sched)->del_mark = 0;

    /* һ���ʱ����Ϣ���г�ʼ�� */
    (*record_time_sched)->pDayOfWeekTimeList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == (*record_time_sched)->pDayOfWeekTimeList)
    {
        osip_free(*record_time_sched);
        (*record_time_sched) = NULL;
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_sched_init() exit---: DayOfWeekTime List Init Error \r\n");
        return -1;
    }

    osip_list_init((*record_time_sched)->pDayOfWeekTimeList);

    return 0;
}

/*****************************************************************************
 �� �� ��  : record_time_sched_free
 ��������  : ¼��ʱ�̲��Խṹ�ͷ�
 �������  : record_time_sched_t* record_time_sched
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void record_time_sched_free(record_time_sched_t * record_time_sched)
{
    if (record_time_sched == NULL)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_sched_free() exit---: Param Error \r\n");
        return;
    }

    record_time_sched->uID = 0;
    record_time_sched->record_cr_index = -1;
    record_time_sched->del_mark = 0;

    /* һ���ʱ����Ϣ���г��ͷ� */
    if (NULL != record_time_sched->pDayOfWeekTimeList)
    {
        osip_list_special_free(record_time_sched->pDayOfWeekTimeList, (void (*)(void*))&record_time_config_free);
        osip_free(record_time_sched->pDayOfWeekTimeList);
        record_time_sched->pDayOfWeekTimeList = NULL;
    }

    osip_free(record_time_sched);
    record_time_sched = NULL;

    return;
}

/*****************************************************************************
 �� �� ��  : record_time_sched_list_init
 ��������  : ¼��ʱ�̲��Զ��г�ʼ��
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
int record_time_sched_list_init()
{
    g_RecordTimeSchedList = (record_time_sched_list_t*)osip_malloc(sizeof(record_time_sched_list_t));

    if (g_RecordTimeSchedList == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_sched_list_init() exit---: g_RecordTimeSchedList Smalloc Error \r\n");
        return -1;
    }

    g_RecordTimeSchedList->pRecordTimeSchedList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_RecordTimeSchedList->pRecordTimeSchedList)
    {
        osip_free(g_RecordTimeSchedList);
        g_RecordTimeSchedList = NULL;
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_sched_list_init() exit---: Record Time Sched List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_RecordTimeSchedList->pRecordTimeSchedList);

#ifdef MULTI_THR
    /* init smutex */
    g_RecordTimeSchedList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_RecordTimeSchedList->lock)
    {
        osip_free(g_RecordTimeSchedList->pRecordTimeSchedList);
        g_RecordTimeSchedList->pRecordTimeSchedList = NULL;
        osip_free(g_RecordTimeSchedList);
        g_RecordTimeSchedList = NULL;
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_sched_list_init() exit---: Record Time Sched List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 �� �� ��  : record_time_sched_list_free
 ��������  : ¼��ʱ�̲��Զ����ͷ�
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
void record_time_sched_list_free()
{
    if (NULL == g_RecordTimeSchedList)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_sched_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_RecordTimeSchedList->pRecordTimeSchedList)
    {
        osip_list_special_free(g_RecordTimeSchedList->pRecordTimeSchedList, (void (*)(void*))&record_time_sched_free);
        osip_free(g_RecordTimeSchedList->pRecordTimeSchedList);
        g_RecordTimeSchedList->pRecordTimeSchedList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_RecordTimeSchedList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_RecordTimeSchedList->lock);
        g_RecordTimeSchedList->lock = NULL;
    }

#endif
    osip_free(g_RecordTimeSchedList);
    g_RecordTimeSchedList = NULL;
    return;
}

/*****************************************************************************
 �� �� ��  : record_time_sched_list_lock
 ��������  : ¼��ʱ�̲��Զ�������
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
int record_time_sched_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RecordTimeSchedList == NULL || g_RecordTimeSchedList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_sched_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_RecordTimeSchedList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : record_time_sched_list_unlock
 ��������  : ¼��ʱ�̲��Զ��н���
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
int record_time_sched_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RecordTimeSchedList == NULL || g_RecordTimeSchedList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_sched_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_RecordTimeSchedList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_record_time_sched_list_lock
 ��������  : ¼��ʱ�̲��Զ�������
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
int debug_record_time_sched_list_lock(const char * file, int line, const char * func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RecordTimeSchedList == NULL || g_RecordTimeSchedList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "debug_record_time_sched_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_RecordTimeSchedList->lock, file, line, func);

    iRecordTimeInfoLockCount++;

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_record_time_sched_list_lock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iRecordTimeInfoLockCount != iRecordTimeInfoUnLockCount + 1)
        {
            //printf("\r\n**********%s:%d:%s:debug_record_time_sched_list_lock:iRet=%d, iRecordTimeInfoLockCount=%lld**********\r\n", file, line, func, iRet, iRecordTimeInfoLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_record_time_sched_list_lock:iRet=%d, iRecordTimeInfoLockCount=%lld", file, line, func, iRet, iRecordTimeInfoLockCount);
        }
    }

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : debug_record_time_sched_list_unlock
 ��������  : ¼��ʱ�̲��Զ��н���
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
int debug_record_time_sched_list_unlock(const char * file, int line, const char * func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_RecordTimeSchedList == NULL || g_RecordTimeSchedList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "debug_record_time_sched_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_RecordTimeSchedList->lock, file, line, func);

    iRecordTimeInfoUnLockCount++;

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_record_time_sched_list_unlock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iRecordTimeInfoLockCount != iRecordTimeInfoUnLockCount)
        {
            //printf("\r\n**********%s:%d:%s:debug_record_time_sched_list_unlock:iRet=%d, iRecordTimeInfoUnLockCount=%lld**********\r\n", file, line, func, iRet, iRecordTimeInfoUnLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_record_time_sched_list_unlock:iRet=%d, iRecordTimeInfoUnLockCount=%lld", file, line, func, iRet, iRecordTimeInfoUnLockCount);
        }
    }

#endif
    return iRet;
}

/*****************************************************************************
 �� �� ��  : record_time_sched_add
 ��������  : ¼��ʱ�̲������
 �������  : unsigned int uID
             int record_cr_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int record_time_sched_add(unsigned int uID, int record_cr_index)
{
    int i = 0;
    record_time_sched_t* pRecordTimeSched = NULL;

    if ((NULL == g_RecordTimeSchedList) || (NULL == g_RecordTimeSchedList->pRecordTimeSchedList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_sched_add() exit---: Record Time Sched List NULL \r\n");
        return -1;
    }

    i = record_time_sched_init(&pRecordTimeSched);

    if (i != 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_time_sched_add() exit---: Record Time Sched Init Error \r\n");
        return -1;
    }

    pRecordTimeSched->uID = uID;
    pRecordTimeSched->record_cr_index = record_cr_index;

    pRecordTimeSched->del_mark = 0;

    RECORD_TIME_SCHED_SMUTEX_LOCK();

    i = osip_list_add(g_RecordTimeSchedList->pRecordTimeSchedList, pRecordTimeSched, -1); /* add to list tail */

    if (i < 0)
    {
        RECORD_TIME_SCHED_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_time_sched_add() exit---: List Add Error \r\n");
        return -1;
    }

    RECORD_TIME_SCHED_SMUTEX_UNLOCK();
    return i - 1;
}

/*****************************************************************************
 �� �� ��  : record_time_sched_get
 ��������  : ��ȡ¼��ʱ�̲���
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
record_time_sched_t* record_time_sched_get(unsigned int uID)
{
    int pos = -1;
    record_time_sched_t* pRecordTimeSched = NULL;

    if (NULL == g_RecordTimeSchedList || NULL == g_RecordTimeSchedList->pRecordTimeSchedList || uID <= 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_time_sched_get() exit---: Param Error \r\n");
        return NULL;
    }

    RECORD_TIME_SCHED_SMUTEX_LOCK();

    if (osip_list_size(g_RecordTimeSchedList->pRecordTimeSchedList) <= 0)
    {
        RECORD_TIME_SCHED_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_time_sched_get() exit---: Record Time Sched List NULL \r\n");
        return NULL;
    }

    for (pos = 0; pos < osip_list_size(g_RecordTimeSchedList->pRecordTimeSchedList); pos++)
    {
        pRecordTimeSched = (record_time_sched_t*)osip_list_get(g_RecordTimeSchedList->pRecordTimeSchedList, pos);

        if ((NULL == pRecordTimeSched) || (pRecordTimeSched->uID <= 0))
        {
            continue;
        }

        if (pRecordTimeSched->uID == uID)
        {
            RECORD_TIME_SCHED_SMUTEX_UNLOCK();
            return pRecordTimeSched;
        }
    }

    RECORD_TIME_SCHED_SMUTEX_UNLOCK();
    return NULL;
}

/*****************************************************************************
 �� �� ��  : set_record_time_sched_list_del_mark
 ��������  : ����¼��ʱ�̲���ɾ����ʶ
 �������  : int del_mark
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int set_record_time_sched_list_del_mark(int del_mark)
{
    int pos1 = -1;
    int pos2 = -1;
    record_time_sched_t* pRecordTimeSched = NULL;
    record_time_config_t* pTimeConfig = NULL;

    if ((NULL == g_RecordTimeSchedList) || (NULL == g_RecordTimeSchedList->pRecordTimeSchedList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "set_record_time_sched_list_del_mark() exit---: Param Error \r\n");
        return -1;
    }

    RECORD_TIME_SCHED_SMUTEX_LOCK();

    if (osip_list_size(g_RecordTimeSchedList->pRecordTimeSchedList) <= 0)
    {
        RECORD_TIME_SCHED_SMUTEX_UNLOCK();
        return 0;
    }

    for (pos1 = 0; pos1 < osip_list_size(g_RecordTimeSchedList->pRecordTimeSchedList); pos1++)
    {
        pRecordTimeSched = (record_time_sched_t*)osip_list_get(g_RecordTimeSchedList->pRecordTimeSchedList, pos1);

        if (NULL == pRecordTimeSched)
        {
            continue;
        }

        pRecordTimeSched->del_mark = del_mark;

        /* ʱ����Ϣ���� */
        if (NULL == pRecordTimeSched->pDayOfWeekTimeList)
        {
            continue;
        }

        if (osip_list_size(pRecordTimeSched->pDayOfWeekTimeList) <= 0)
        {
            continue;
        }

        for (pos2 = 0; pos2 < osip_list_size(pRecordTimeSched->pDayOfWeekTimeList); pos2++)
        {
            pTimeConfig = (record_time_config_t*)osip_list_get(pRecordTimeSched->pDayOfWeekTimeList, pos2);

            if (NULL == pTimeConfig)
            {
                continue;
            }

            pTimeConfig->del_mark = del_mark;
        }
    }

    RECORD_TIME_SCHED_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 �� �� ��  : delete_record_time_sched_from_list_by_mark
 ��������  : ����¼��ʱ�̲���ɾ����ʶ�Ƴ�����Ҫ������
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
int delete_record_time_sched_from_list_by_mark()
{
    int i = 0;
    int pos1 = -1;
    int pos2 = -1;
    int index = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    record_time_sched_t* pRecordTimeSched = NULL;
    record_time_config_t* pTimeConfig = NULL;
    vector<int> CRIndexVector;

    if ((NULL == g_RecordTimeSchedList) || (NULL == g_RecordTimeSchedList->pRecordTimeSchedList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "delete_record_info_from_list_by_mark() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    RECORD_TIME_SCHED_SMUTEX_LOCK();

    if (osip_list_size(g_RecordTimeSchedList->pRecordTimeSchedList) <= 0)
    {
        RECORD_TIME_SCHED_SMUTEX_UNLOCK();
        return 0;
    }

    pos1 = 0;

    while (!osip_list_eol(g_RecordTimeSchedList->pRecordTimeSchedList, pos1))
    {
        pRecordTimeSched = (record_time_sched_t*)osip_list_get(g_RecordTimeSchedList->pRecordTimeSchedList, pos1);

        if (NULL == pRecordTimeSched)
        {
            osip_list_remove(g_RecordTimeSchedList->pRecordTimeSchedList, pos1);
            continue;
        }

        if (pRecordTimeSched->del_mark == 1)
        {
            /* ֪ͨTSU��ͣ */
            pCrData = call_record_get(pRecordTimeSched->record_cr_index);

            if (NULL != pCrData)
            {
                CRIndexVector.push_back(pRecordTimeSched->record_cr_index);
            }

            osip_list_remove(g_RecordTimeSchedList->pRecordTimeSchedList, pos1);
            record_time_sched_free(pRecordTimeSched);
            pRecordTimeSched = NULL;
        }
        else
        {
            /* ʱ����Ϣ���� */
            if (NULL == pRecordTimeSched->pDayOfWeekTimeList)
            {
                pos1++;
                continue;
            }

            if (osip_list_size(pRecordTimeSched->pDayOfWeekTimeList) <= 0)
            {
                pos1++;
                continue;
            }

            pos2 = 0;

            while (!osip_list_eol(pRecordTimeSched->pDayOfWeekTimeList, pos2))
            {
                pTimeConfig = (record_time_config_t*)osip_list_get(pRecordTimeSched->pDayOfWeekTimeList, pos2);

                if (NULL == pTimeConfig)
                {
                    osip_list_remove(pRecordTimeSched->pDayOfWeekTimeList, pos2);
                    continue;
                }

                if (pTimeConfig->del_mark == 1)
                {
                    /* ֪ͨTSU��ͣ */
                    if (1 == pTimeConfig->iStatus) /* �������¼��֪ͨTSU��ͣ */
                    {
                        pCrData = call_record_get(pRecordTimeSched->record_cr_index);

                        if (NULL != pCrData)
                        {
                            CRIndexVector.push_back(pRecordTimeSched->record_cr_index);

                            pTimeConfig->iStatus = 0;
                        }
                    }

                    osip_list_remove(pRecordTimeSched->pDayOfWeekTimeList, pos2);
                    record_time_config_free(pTimeConfig);
                    pTimeConfig = NULL;
                }
                else
                {
                    pos2++;
                }
            }

            pos1++;
        }
    }

    RECORD_TIME_SCHED_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "¼�����ʱ���ɾ��, ֪ͨTSU��ͣ¼��: ��λID=%s, TSU IP��ַ=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video strategy time slot delete, notify TSU to pause video: point ID=%s, TSU IP address=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);

            /* ��ͣ¼�� */
            i = notify_tsu_pause_record(pCrData->tsu_ip, pCrData->task_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_time_sched_config_from_db_to_list() notify_tsu_pause_record Error: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_record_time_sched_config_from_db_to_list() notify_tsu_pause_record OK: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 �� �� ��  : check_record_time_sched_config_from_db_to_list
 ��������  : �����ݿ��¼��ʱ�����ͬ�����ڴ���
 �������  : record_time_sched_t* pRecordTimeSched
             DBOper* pDBOper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��8��27�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int check_record_time_sched_config_from_db_to_list(record_time_sched_t * pRecordTimeSched, DBOper * pDBOper)
{
    int iRet = 0;
    time_t now;
    struct tm p_tm = { 0 };
    string strSQL = "";
    int iWeekDay = 0;
    char strWeekDay[16] = {0};
    char strRecordInfoIndex[32] = {0};
    int record_count = 0;
    int while_count = 0;
    record_time_config_t* pTimeConfig = NULL;
    cr_t* pCrData = NULL;

    if (NULL == pRecordTimeSched || NULL == pDBOper)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "check_record_time_sched_config_from_db_to_list() exit---: Record Srv db Oper Error \r\n");
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
    snprintf(strRecordInfoIndex, 32, "%u", pRecordTimeSched->uID);

    /* ѭ������ʱ�����Ϣ */
    strSQL.clear();
    strSQL = "select * from RecordTimeSchedConfig WHERE RecordSchedIndex = ";
    strSQL += strRecordInfoIndex;
    strSQL += " AND DayInWeek = ";
    strSQL += strWeekDay;
    strSQL += " order by BeginTime asc";

    record_count = pDBOper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_record_time_sched_config_from_db_to_list:record_count=%d, strSQL=%s \r\n", record_count, strSQL.c_str());

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_time_sched_config_from_db_to_list() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_time_sched_config_from_db_to_list() ErrorMsg=%s\r\n", pDBOper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_time_sched_config_from_db_to_list() RecordTimeSchedConfig No Record \r\n");
        return 0;
    }

    /* ѭ���������ݿ�*/
    do
    {
        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "check_record_time_sched_config_from_db_to_list() While Count=%d \r\n", while_count);
        }

        int i = 0;
        unsigned int uID = 0;
        int iBeginTime = 0;
        int iEndTime = 0;

        pDBOper->GetFieldValue("ID", uID);
        pDBOper->GetFieldValue("BeginTime", iBeginTime);
        pDBOper->GetFieldValue("EndTime", iEndTime);

        if (iEndTime - iBeginTime <= 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_time_sched_config_from_db_to_list() EndTime Small to BeginTime:BeginTime=%d, EndTime=%d \r\n", iBeginTime, iEndTime);
            continue;
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_record_time_sched_config_from_db_to_list:uID=%u, BeginTime=%d, EndTime=%d \r\n", uID, iBeginTime, iEndTime);

        pTimeConfig = record_time_config_get(pRecordTimeSched->pDayOfWeekTimeList, uID);

        if (NULL != pTimeConfig)
        {
            pTimeConfig->del_mark = 0;

            if (pTimeConfig->iBeginTime != iBeginTime
                || pTimeConfig->iEndTime != iEndTime)
            {
                if (1 == pTimeConfig->iStatus) /* �������¼������ͣһ�� */
                {
                    pCrData = call_record_get(pRecordTimeSched->record_cr_index);

                    if (NULL != pCrData)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "¼�����ʱ��η����仯, ֪ͨTSU��ͣ¼��: ��λID=%s, TSU IP��ַ=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video strategy time slot changed, notify TSU to pause video: point ID=%s, TSU IP address=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);

                        /* ��ͣ¼�� */
                        i = notify_tsu_pause_record(pCrData->tsu_ip, pCrData->task_id);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_time_sched_config_from_db_to_list() notify_tsu_pause_record Error: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_record_time_sched_config_from_db_to_list() notify_tsu_pause_record OK: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
                        }

                        pTimeConfig->iStatus = 0;
                    }
                }
            }

            if (pTimeConfig->iBeginTime != iBeginTime)
            {
                pTimeConfig->iBeginTime = iBeginTime;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_time_sched_config_from_db_to_list() BeginTime Changed:uID=%u,iBeginTime=%d,iEndTime=%d \r\n", pTimeConfig->uID, pTimeConfig->iBeginTime, pTimeConfig->iEndTime);
            }

            if (pTimeConfig->iEndTime != iEndTime)
            {
                pTimeConfig->iEndTime = iEndTime;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_time_sched_config_from_db_to_list() EndTime Changed:uID=%u,iBeginTime=%d,iEndTime=%d \r\n", pTimeConfig->uID, pTimeConfig->iBeginTime, pTimeConfig->iEndTime);
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_time_sched_config_from_db_to_list:record_time_config_add uID=%u,iBeginTime=%d,iEndTime=%d \r\n", uID, iBeginTime, iEndTime);

            /* ��ӵ����� */
            iRet = record_time_config_add(pRecordTimeSched->pDayOfWeekTimeList, uID, iBeginTime, iEndTime);

            if (iRet < 0)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_time_sched_config_from_db_to_list() record_time_config_add Error:iRet=%d", iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_record_time_sched_config_from_db_to_list() record_time_config_add OK:iRet=%d", iRet);
            }
        }
    }
    while (pDBOper->MoveNext() >= 0);

    return 0;
}

/*****************************************************************************
 �� �� ��  : check_record_time_sched_from_db_to_list
 ��������  : ����������Ƿ����µ��������ݵ�¼��ʱ�̲��Ա�
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
int check_record_time_sched_from_db_to_list(DBOper * pDBOper)
{
    int iRet = 0;
    int index = 0;
    int iRecordTimePos = 0;
    int iRecordCRIndex = -1;
    unsigned int uRecordInfoIndex = 0;
    int iRecordInfoIndexCount = 0;
    record_time_sched_t* pRecordTimeSched = NULL;
    vector<unsigned int> RecordInfoIndexVector;

    if (NULL == pDBOper)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "check_record_time_sched_from_db_to_list() exit---: Record Srv db Oper Error \r\n");
        return -1;
    }

    /* ��ȡ¼����Ե����� */
    RecordInfoIndexVector.clear();
    iRet = get_record_info_index_from_list(RecordInfoIndexVector);

    iRecordInfoIndexCount = RecordInfoIndexVector.size();

    if (iRecordInfoIndexCount <= 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "check_record_time_sched_from_db_to_list() exit---: Record Info Index NULL \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_record_time_sched_from_db_to_list:iRecordInfoIndexCount=%d \r\n", iRecordInfoIndexCount);

    /* ����¼����Ա���������ݣ�ѭ����ȡ¼��ʱ����Ա���������� */
    for (index = 0; index < iRecordInfoIndexCount; index++)
    {
        /* ��ȡ¼��������� */
        uRecordInfoIndex = RecordInfoIndexVector[index];

        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_record_time_sched_from_db_to_list() uRecordInfoIndex=%u \r\n", uRecordInfoIndex);

        iRecordCRIndex = get_record_cr_index_by_record_index(uRecordInfoIndex);

        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_record_time_sched_from_db_to_list() get_record_cr_index_by_record_index:iRecordCRIndex=%d \r\n", iRecordCRIndex);

        if (iRecordCRIndex < 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_time_sched_from_db_to_list() Get Record CR Index Error: RecordInfoIndex=%u\r\n", uRecordInfoIndex);
            continue;
        }

        /* ���Ҷ�Ӧ��ʱ�������Ϣ */
        pRecordTimeSched = record_time_sched_get(uRecordInfoIndex);

        if (NULL == pRecordTimeSched) /* ��������ڣ������ */
        {
            iRecordTimePos = record_time_sched_add(uRecordInfoIndex, iRecordCRIndex);

            if (iRecordTimePos < 0)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_time_sched_from_db_to_list() record_time_sched_add Error: RecordInfoIndex=%u, RecordCRIndex=%d\r\n", uRecordInfoIndex, iRecordCRIndex);
                continue;
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_time_sched_from_db_to_list() record_time_sched_add:iRecordTimePos=%d, RecordInfoIndex=%u, RecordCRIndex=%d \r\n", iRecordTimePos, uRecordInfoIndex, iRecordCRIndex);
            }

            /* �ٴλ�ȡһ�� */
            pRecordTimeSched = record_time_sched_get(uRecordInfoIndex);

            if (NULL == pRecordTimeSched)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_time_sched_from_db_to_list() record_time_sched_get Error: RecordInfoIndex=%u, RecordCRIndex=%d\r\n", uRecordInfoIndex, iRecordCRIndex);
                continue;
            }
        }
        else
        {
            pRecordTimeSched->del_mark = 0;
            pRecordTimeSched->record_cr_index = iRecordCRIndex;
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_record_time_sched_from_db_to_list() check_record_time_sched_config_from_db_to_list:RecordTimeSched uID=%u, record_cr_index=%d \r\n", pRecordTimeSched->uID, pRecordTimeSched->record_cr_index);

        iRet = check_record_time_sched_config_from_db_to_list(pRecordTimeSched, pDBOper);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_time_sched_from_db_to_list() check_record_time_sched_config_from_db_to_list Error:iRet=%d\r\n", iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_record_time_sched_from_db_to_list() check_record_time_sched_config_from_db_to_list OK:iRet=%d\r\n", iRet);
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : scan_record_time_sched_list
 ��������  : ɨ��¼��ʱ�̲��Զ�������
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
void scan_record_time_sched_list()
{
    int i = 0;
    int iRet = 0;
    record_time_sched_t* pRecordTimeSched = NULL;
    needtoproc_recordtimesched_queue needprocqueue;

    needprocqueue.clear();

    RECORD_TIME_SCHED_SMUTEX_LOCK();

    if ((NULL == g_RecordTimeSchedList) || (NULL == g_RecordTimeSchedList->pRecordTimeSchedList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "scan_record_time_sched_list() exit---: Param Error \r\n");
        RECORD_TIME_SCHED_SMUTEX_UNLOCK();
        return;
    }

    if (osip_list_size(g_RecordTimeSchedList->pRecordTimeSchedList) <= 0)
    {
        RECORD_TIME_SCHED_SMUTEX_UNLOCK();
        return;
    }

    for (i = 0; i < osip_list_size(g_RecordTimeSchedList->pRecordTimeSchedList); i++)
    {
        pRecordTimeSched = (record_time_sched_t*)osip_list_get(g_RecordTimeSchedList->pRecordTimeSchedList, i);

        if (NULL == pRecordTimeSched)
        {
            continue;
        }

        //printf("\r\n scan_record_time_sched_list:uID=%u,start_mark=%d,stop_mark=%d \r\n", pRecordTimeSched->uID, pRecordTimeSched->start_mark, pRecordTimeSched->stop_mark);

        needprocqueue.push_back(pRecordTimeSched);
    }

    RECORD_TIME_SCHED_SMUTEX_UNLOCK();

    /* ������Ҫ¼��� */
    while (!needprocqueue.empty())
    {
        pRecordTimeSched = (record_time_sched_t*) needprocqueue.front();
        needprocqueue.pop_front();

        if (NULL != pRecordTimeSched)
        {
            /* ʱ���¼���� */
            iRet = RecordTimeSchedConfigProc(pRecordTimeSched);
        }
    }

    needprocqueue.clear();

    return;
}

/*****************************************************************************
 �� �� ��  : RecordTimeSchedConfig_db_refresh_proc
 ��������  : ����¼��ʱ�̲���������Ϣ���ݿ���²�����ʶ
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
int RecordTimeSchedConfig_db_refresh_proc()
{
    if (1 == db_RecordTimeSched_reload_mark) /* ����ִ�� */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "¼��ʱ�̲����������ݿ���Ϣ����ͬ��");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Record Time Sched Info database information are synchronized");
        return 0;
    }

    db_RecordTimeSched_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 �� �� ��  : check_RecordTimeSchedConfig_need_to_reload_begin
 ��������  : ����Ƿ���Ҫͬ��¼��ʱ�̲������ÿ�ʼ
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
void check_RecordTimeSchedConfig_need_to_reload(DBOper* pDboper)
{
    /* ����Ƿ���Ҫ�������ݿ��ʶ */
    if (!db_RecordTimeSched_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ��¼��ʱ�̲����������ݿ���Ϣ: ��ʼ---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization record time sched info database information: begain---");

    /* ����¼��ʱ����Զ��е�ɾ����ʶ */
    set_record_time_sched_list_del_mark(1);

    /* �����ݿ��еı仯����ͬ�����ڴ� */
    check_record_time_sched_from_db_to_list(pDboper);

    /* ɾ�������¼��ʱ����Ϣ,��Ҫ��������ִ��ǰ��ִ�У���Ȼ����ִ���˻ָ�֮���ֱ�ɾ���� */
    delete_record_time_sched_from_list_by_mark();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ͬ��¼��ʱ�̲����������ݿ���Ϣ: ����---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization record time sched info database information: end---");
    db_RecordTimeSched_reload_mark = 0;

    return;
}

#endif
/*****************************************************************************
 �� �� ��  : AddRecordInfo2DB
 ��������  : ���¼���������ݿ�
 �������  : record_info_t* pRecordInfo
                            DBOper* pRecord_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��6��5�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int AddRecordInfo2DB(record_info_t * pRecordInfo, DBOper * pRecord_Srv_dboper)
{
    int iRet = 0;
    int record_count = 0;
    string strQuerySQL = "";
    string strInsertSQL = "";
    string strUpdateSQL = "";

    char strDeviceIndex[64] = {0};
    char strRecordEnable[16] = {0};
    char strDays[16] = {0};
    char strTimeLength[16] = {0};
    char strType[16] = {0};
    char strTimeOfAllWeek[16] = {0};
    char strBandWidth[16] = {0};

    if (NULL == pRecordInfo || NULL == pRecord_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "AddRecordInfo2DB() exit---: Param Error \r\n");
        return -1;
    }

    memset(strDeviceIndex, 0 , 64);
    snprintf(strDeviceIndex, 64, "%u", pRecordInfo->device_index);

    memset(strRecordEnable, 0 , 16);
    snprintf(strRecordEnable, 16, "%d", pRecordInfo->record_enable);

    memset(strDays, 0 , 16);
    snprintf(strDays, 16, "%d", pRecordInfo->record_days);

    memset(strTimeLength, 0 , 16);
    snprintf(strTimeLength, 16, "%d", pRecordInfo->record_timeLen);

    memset(strType, 0 , 16);
    snprintf(strType, 16, "%d", pRecordInfo->record_type);

    memset(strTimeOfAllWeek, 0 , 16);
    snprintf(strTimeOfAllWeek, 16, "%d", pRecordInfo->TimeOfAllWeek);

    memset(strBandWidth, 0 , 16);
    snprintf(strBandWidth, 16, "%d", pRecordInfo->bandwidth);

    /* 1����ѯSQL ���*/
    strQuerySQL.clear();
    strQuerySQL = "select * from RecordSchedConfig WHERE DeviceIndex = ";
    strQuerySQL += strDeviceIndex;


    /* 2������SQL ���*/
    strInsertSQL.clear();
    strInsertSQL = "insert into RecordSchedConfig (DeviceIndex,RecordEnable,Days,TimeLength,Type,TimeOfAllWeek,BandWidth) values (";

    /* �߼��豸����*/
    strInsertSQL += strDeviceIndex;

    strInsertSQL += ",";

    /* �Ƿ����� */
    strInsertSQL += strRecordEnable;

    strInsertSQL += ",";

    /* ¼������*/
    strInsertSQL += strDays;

    strInsertSQL += ",";

    /* ¼��ʱ��*/
    strInsertSQL += strTimeLength;

    strInsertSQL += ",";

    /* ¼������*/
    strInsertSQL += strType;

    strInsertSQL += ",";

    /* ȫ¼*/
    strInsertSQL += strTimeOfAllWeek;

    strInsertSQL += ",";

    /* ����*/
    strInsertSQL += strBandWidth;

    strInsertSQL += ")";


    /* 3������SQL ���*/
    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE RecordSchedConfig SET ";

    /* �Ƿ�¼��*/
    strUpdateSQL += "RecordEnable = ";
    strUpdateSQL += strRecordEnable;

    strUpdateSQL += ",";

    /* ȫ¼ */
    strUpdateSQL += "TimeOfAllWeek = ";
    strUpdateSQL += strTimeOfAllWeek;

    strUpdateSQL += " WHERE DeviceIndex = ";
    strUpdateSQL += strDeviceIndex;

    /* ��ѯ���ݿ� */
    record_count = pRecord_Srv_dboper->DB_Select(strQuerySQL.c_str(), 1);

    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "AddRecordInfo2DB() DB Select:record_count=%d,DeviceIndex=%s \r\n", record_count, strDeviceIndex);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "AddRecordInfo2DB() DB Oper Error:strQuerySQL=%s, record_count=%d \r\n", strQuerySQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "AddRecordInfo2DB() ErrorMsg=%s\r\n", pRecord_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        iRet = pRecord_Srv_dboper->DB_Insert(strQuerySQL.c_str(), strUpdateSQL.c_str(), strInsertSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "AddRecordInfo2DB() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "AddRecordInfo2DB() ErrorMsg=%s\r\n", pRecord_Srv_dboper->GetLastDbErrorMsg());
        }
    }
    else
    {
        iRet = pRecord_Srv_dboper->DB_Update(strUpdateSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "AddRecordInfo2DB() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "AddRecordInfo2DB() ErrorMsg=%s\r\n", pRecord_Srv_dboper->GetLastDbErrorMsg());
        }
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : delRecordInfo2DB
 ��������  : �����ݿ���ɾ��¼������
 �������  : unsigned int device_index
             DBOper* pRecord_Srv_dboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2014��6��5�� ������
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int delRecordInfo2DB(unsigned int device_index, DBOper * pRecord_Srv_dboper)
{
    int iRet = 0;
    string strUpdateSQL = "";

    char strDeviceIndex[64] = {0};

    if (device_index <= 0 || NULL == pRecord_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "delRecordInfo2DB() exit---: Param Error \r\n");
        return -1;
    }

    memset(strDeviceIndex, 0 , 64);
    snprintf(strDeviceIndex, 64, "%u", device_index);

    /* ���� SQL ���*/
    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE RecordSchedConfig SET RecordEnable = 0 WHERE DeviceIndex = ";
    strUpdateSQL += strDeviceIndex;

    /* �������ݿ� */
    iRet = pRecord_Srv_dboper->DB_Update(strUpdateSQL.c_str(), 1);

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "AddRecordInfo2DB() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "AddRecordInfo2DB() ErrorMsg=%s\r\n", pRecord_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 �� �� ��  : RecordTimeSchedConfigProc
 ��������  : ����¼��ʱ�̲��Ա���������ֹͣ¼������
 �������  : record_time_sched_t* pRecordTimeSched
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��5��29�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
int RecordTimeSchedConfigProc(record_time_sched_t * pRecordTimeSched)
{
    int i = 0;
    int pos = -1;
    cr_t* pCrData = NULL;
    record_time_config_t* pTimeConfig = NULL;

    time_t now = time(NULL);
    int iTimeNow = 0;
    struct tm tp = {0};

    record_info_t* pRecordInfo = NULL;

    if (NULL == pRecordTimeSched || NULL == pRecordTimeSched->pDayOfWeekTimeList)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "RecordTimeSchedConfigProc() exit---: Param Error \r\n");
        return -1;
    }

    if (osip_list_size(pRecordTimeSched->pDayOfWeekTimeList) <= 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "RecordTimeSchedConfigProc() exit---: Day Of Week Time List NULL \r\n");
        return 0;
    }

    //printf("\r\n RecordTimeSchedConfigProc:DayOfWeekTimeList=%d \r\n", osip_list_size(pRecordTimeSched->pDayOfWeekTimeList));

    pCrData = call_record_get(pRecordTimeSched->record_cr_index);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "RecordTimeSchedConfigProc() exit---: Get Record Srv Error:record_cr_index=%d \r\n", pRecordTimeSched->record_cr_index);
        return -1;
    }

    /* ���㵱ǰʱ�� */
    localtime_r(&now, &tp);
    iTimeNow = tp.tm_hour * 3600 + tp.tm_min * 60 + tp.tm_sec;

    for (pos = 0; pos < osip_list_size(pRecordTimeSched->pDayOfWeekTimeList); pos++)
    {
        pTimeConfig = (record_time_config_t*)osip_list_get(pRecordTimeSched->pDayOfWeekTimeList, pos);

        if ((NULL == pTimeConfig) || (pTimeConfig->uID <= 0))
        {
            continue;
        }

        //printf("\r\n RecordTimeSchedConfigProc:TimeConfig uID=%u, iBeginTime=%d,iEndTime=%d  \r\n", pTimeConfig->uID, pTimeConfig->iBeginTime, pTimeConfig->iEndTime);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "RecordTimeSchedConfigProc() RecordTimeSched: uID=%u, record_cr_index=%d, TimeNow=%d, BeginTime=%d, EndTime=%d, Status=%d \r\n", pRecordTimeSched->uID, pRecordTimeSched->record_cr_index, iTimeNow, pTimeConfig->iBeginTime, pTimeConfig->iEndTime, pTimeConfig->iStatus);

        if (0 == pTimeConfig->iStatus) /* ���Ƿ���Ҫ��ʼ¼�� */
        {
            if (iTimeNow >= pTimeConfig->iBeginTime - 30 && iTimeNow < pTimeConfig->iEndTime + 30)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "¼�����ʱ���¼��֪ͨTSU�ָ�¼��: ��λID=%s, TSU IP��ַ=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video strategy time slot record notify TSU to recover video: point ID=%s, TSU IP address=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);

                /* ����¼���¼, ���Ƿ��Ѿ��ڱ���¼�� */
                pRecordInfo = record_info_get_by_record_index(pTimeConfig->uID);

                if (NULL != pRecordInfo)
                {
                    if (0 == pRecordInfo->iTSUAlarmRecordStatus)
                    {
                        /* ����¼�� */
                        i = notify_tsu_resume_record(pCrData->tsu_ip, pCrData->task_id);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "RecordTimeSchedConfigProc() notify_tsu_resume_record Error: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "RecordTimeSchedConfigProc() notify_tsu_resume_record OK: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
                        }
                    }
                }
                else
                {
                    /* ����¼�� */
                    i = notify_tsu_resume_record(pCrData->tsu_ip, pCrData->task_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "RecordTimeSchedConfigProc() notify_tsu_resume_record Error: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "RecordTimeSchedConfigProc() notify_tsu_resume_record OK: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
                    }
                }

                pTimeConfig->iStatus = 1;
            }
        }

        if (1 == pTimeConfig->iStatus) /* ���Ƿ���Ҫֹͣ¼�� */
        {
            if (iTimeNow >= pTimeConfig->iEndTime + 30)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "¼�����ʱ���¼��֪ͨTSU��ͣ¼��: ��λID=%s, TSU IP��ַ=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video strategy time slot record notify TSU to pause video: point ID=%s, TSU IP address=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);

                /* ����¼���¼, ���Ƿ��Ѿ��ڱ���¼��, ����б���¼���ڣ�������ͣ */
                pRecordInfo = record_info_get_by_record_index(pTimeConfig->uID);

                if (NULL != pRecordInfo)
                {
                    if (0 == pRecordInfo->iTSUAlarmRecordStatus)
                    {
                        /* ��ͣ¼�� */
                        i = notify_tsu_pause_record(pCrData->tsu_ip, pCrData->task_id);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "RecordTimeSchedConfigProc() notify_tsu_pause_record Error: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "RecordTimeSchedConfigProc() notify_tsu_pause_record OK: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
                        }
                    }
                }
                else
                {
                    /* ��ͣ¼�� */
                    i = notify_tsu_pause_record(pCrData->tsu_ip, pCrData->task_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "RecordTimeSchedConfigProc() notify_tsu_pause_record Error: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "RecordTimeSchedConfigProc() notify_tsu_pause_record OK: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
                    }
                }

                pTimeConfig->iStatus = 0;
            }
        }
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : RemoveRecordTimeSchedConfig
 ��������  : �Ƴ�¼����Ա�ʶ
 �������  : record_time_sched_t* pRecordTimeSched
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��8��11�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int RemoveRecordTimeSchedConfig(record_time_sched_t * pRecordTimeSched)
{
    int pos = -1;
    record_time_config_t* pTimeConfig = NULL;

    if (NULL == pRecordTimeSched || NULL == pRecordTimeSched->pDayOfWeekTimeList)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "RemoveRecordTimeSchedConfig() exit---: Param Error \r\n");
        return -1;
    }

    if (osip_list_size(pRecordTimeSched->pDayOfWeekTimeList) <= 0)
    {
        return 0;
    }

    for (pos = 0; pos < osip_list_size(pRecordTimeSched->pDayOfWeekTimeList); pos++)
    {
        pTimeConfig = (record_time_config_t*)osip_list_get(pRecordTimeSched->pDayOfWeekTimeList, pos);

        if ((NULL == pTimeConfig) || (pTimeConfig->uID <= 0))
        {
            continue;
        }

        pTimeConfig->iStatus = 0;
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : RemoveRecordTimeSchedMark
 ��������  : �Ƴ�¼����Ա�ʶ
 �������  : unsigned int uID
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��8��11�� ���ڶ�
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int RemoveRecordTimeSchedMark(unsigned int uID)
{
    int i = 0;
    int iRet = 0;
    record_time_sched_t* pRecordTimeSched = NULL;
    needtoproc_recordtimesched_queue needprocqueue;

    needprocqueue.clear();

    RECORD_TIME_SCHED_SMUTEX_LOCK();

    if ((NULL == g_RecordTimeSchedList) || (NULL == g_RecordTimeSchedList->pRecordTimeSchedList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "RemoveRecordTimeSchedMark() exit---: Param Error \r\n");
        RECORD_TIME_SCHED_SMUTEX_UNLOCK();
        return -1;
    }

    if (osip_list_size(g_RecordTimeSchedList->pRecordTimeSchedList) <= 0)
    {
        RECORD_TIME_SCHED_SMUTEX_UNLOCK();
        return 0;
    }

    for (i = 0; i < osip_list_size(g_RecordTimeSchedList->pRecordTimeSchedList); i++)
    {
        pRecordTimeSched = (record_time_sched_t*)osip_list_get(g_RecordTimeSchedList->pRecordTimeSchedList, i);

        if (NULL == pRecordTimeSched)
        {
            continue;
        }

        if (pRecordTimeSched->uID != uID)
        {
            continue;
        }

        needprocqueue.push_back(pRecordTimeSched);
    }

    RECORD_TIME_SCHED_SMUTEX_UNLOCK();

    /* ������Ҫ¼��� */
    while (!needprocqueue.empty())
    {
        pRecordTimeSched = (record_time_sched_t*) needprocqueue.front();
        needprocqueue.pop_front();

        if (NULL != pRecordTimeSched)
        {
            iRet = RemoveRecordTimeSchedConfig(pRecordTimeSched);

            pRecordTimeSched->record_cr_index = -1;
        }
    }

    needprocqueue.clear();

    return iRet;
}

/*****************************************************************************
 �� �� ��  : RemoveDeviceRecordInfo
 ��������  : �Ƴ��豸¼����Ϣ
 �������  : char* device_id
             int stream_type
             int tsu_resource_index
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��12��2��
    ��    ��   : �û�·����Ϣ����
    �޸�����   : �����ɺ���

*****************************************************************************/
int RemoveDeviceRecordInfo(char * device_id, int stream_type, int tsu_resource_index)
{
    int i = 0;
    unsigned int device_index = 0;
    tsu_resource_info_t* pTsuResourceInfo = NULL;

    if (NULL == device_id || tsu_resource_index < 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "RemoveDeviceRecordInfo() exit---: Param Error \r\n");
        return -1;
    }

    /* ��ȡ�߼��豸���� */
    device_index = Get_GBLogicDevice_Index_By_Device_ID(device_id);

    if (device_index <= 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "RemoveDeviceRecordInfo() exit---: Get_GBLogicDevice_Index_By_Device_ID Error:device_id=%s \r\n", device_id);
        return -1;
    }

    /* ��ȡTSU��Դ��Ϣ */
    pTsuResourceInfo = tsu_resource_info_get(tsu_resource_index);

    if (NULL == pTsuResourceInfo)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "RemoveDeviceRecordInfo() exit---: Get TSU Resource Info Error:tsu_resource_index=%d \r\n", tsu_resource_index);
        return -1;
    }

    /* �Ƴ�¼����Ϣ */
    i = RemoveRecordInfoFromTSU(pTsuResourceInfo, device_index, stream_type);

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "RemoveDeviceRecordInfo() RemoveRecordInfoFromTSU Error: device_index=%u, stream_type=%d, i=%d", device_index, stream_type, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "RemoveDeviceRecordInfo() RemoveRecordInfoFromTSU OK: device_index=%u, stream_type=%d, i=%d", device_index, stream_type, i);
    }

    return i;
}

/*****************************************************************************
 �� �� ��  : find_record_config_by_device_index
 ��������  : ���ݵ�λ�������ҵ�λ�Ƿ�������¼��
 �������  : unsigned int uDeviceIndex
             DBOper * pDboper
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��3��17��
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
int find_record_config_by_device_index(unsigned int uDeviceIndex, DBOper * pDboper)
{
    int record_count = 0;
    string strSQL = "";
    char strDeviceIndex[32] = {0};

    if (uDeviceIndex <= 0 || NULL == pDboper)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR,  "find_record_config_by_device_index() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    snprintf(strDeviceIndex, 32, "%u", uDeviceIndex);

    strSQL.clear();
    strSQL = "select * from RecordSchedConfig WHERE RecordEnable = 1 and DeviceIndex = ";
    strSQL += strDeviceIndex;

    record_count = pDboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "find_record_config_by_device_index() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "find_record_config_by_device_index() ErrorMsg=%s\r\n", pDboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "find_record_config_by_device_index() exit---: No Record Count \r\n");
        return 0;
    }
    else if (record_count > 0)
    {
        return 1;
    }

    return 0;
}

/*****************************************************************************
 �� �� ��  : ShowRecordInfo
 ��������  : ��ʾ¼����Ϣ
 �������  : int sock
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2013��7��27�� ������
    ��    ��   : yanghaifeng
    �޸�����   : �����ɺ���

*****************************************************************************/
void ShowRecordInfo(int sock, int type)
{
    int i = 0;
    char strLine[] = "\r---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rDevice Index Device ID            Device Name                      Record Type Stream Type Record Enable Record Index TSU Index Status           TimeOfAllWeek PauseStatus ResumeStatus AlarmRecordStatus\r\n";
    record_info_t* pRecordInfo = NULL;
    char rbuf[256] = {0};
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if (NULL == g_RecordInfoList || osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        return;
    }

    for (i = 0; i < osip_list_size(g_RecordInfoList->pRecordInfoList); i++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, i);

        if (NULL == pRecordInfo)
        {
            continue;
        }

        if (0 == type) /* ��ʾδ¼��� */
        {
            if (pRecordInfo->record_cr_index >= 0)
            {
                continue;
            }
        }
        else if (1 == type) /* ��ʾ��¼��� */
        {
            if (pRecordInfo->record_cr_index < 0)
            {
                continue;
            }
        }

        /* �����߼��豸��Ϣ */
        pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(pRecordInfo->device_index);

        if (NULL == pGBLogicDeviceInfo)
        {
            continue;
        }

        if (0 == pRecordInfo->record_enable)
        {
            snprintf(rbuf, 256, "\r%-12u %-20s %-32s %-11d %-11d %-13d %-12d %-9d %-16s %-13d %-11d %-11d %-17d\r\n", pRecordInfo->device_index, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type, pRecordInfo->record_enable, pRecordInfo->record_cr_index, pRecordInfo->tsu_index, (char*)"Not Enable", pRecordInfo->TimeOfAllWeek, pRecordInfo->iTSUPauseStatus, pRecordInfo->iTSUResumeStatus, pRecordInfo->iTSUAlarmRecordStatus);
        }
        else if (1 == pGBLogicDeviceInfo->record_type)
        {
            snprintf(rbuf, 256, "\r%-12u %-20s %-32s %-11d %-11d %-13d %-12d %-9d %-16s %-13d %-11d %-11d %-17d\r\n", pRecordInfo->device_index, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type, pRecordInfo->record_enable, pRecordInfo->record_cr_index, pRecordInfo->tsu_index, (char*)"Front Record", pRecordInfo->TimeOfAllWeek, pRecordInfo->iTSUPauseStatus, pRecordInfo->iTSUResumeStatus, pRecordInfo->iTSUAlarmRecordStatus);
        }
        else
        {
            if (RECORD_STATUS_INIT == pRecordInfo->record_status)
            {
                snprintf(rbuf, 256, "\r%-12u %-20s %-32s %-11d %-11d %-13d %-12d %-9d %-16s %-13d %-11d %-11d %-17d\r\n", pRecordInfo->device_index, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type, pRecordInfo->record_enable, pRecordInfo->record_cr_index, pRecordInfo->tsu_index, (char*)"Not Success", pRecordInfo->TimeOfAllWeek, pRecordInfo->iTSUPauseStatus, pRecordInfo->iTSUResumeStatus, pRecordInfo->iTSUAlarmRecordStatus);
            }
            else if (RECORD_STATUS_OFFLINE == pRecordInfo->record_status)
            {
                snprintf(rbuf, 256, "\r%-12u %-20s %-32s %-11d %-11d %-13d %-12d %-9d %-16s %-13d %-11d %-11d %-17d\r\n", pRecordInfo->device_index, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type, pRecordInfo->record_enable, pRecordInfo->record_cr_index, pRecordInfo->tsu_index, (char*)"Device Off Line", pRecordInfo->TimeOfAllWeek, pRecordInfo->iTSUPauseStatus, pRecordInfo->iTSUResumeStatus, pRecordInfo->iTSUAlarmRecordStatus);
            }
            else if (RECORD_STATUS_NOSTREAM == pRecordInfo->record_status)
            {
                snprintf(rbuf, 256, "\r%-12u %-20s %-32s %-11d %-11d %-13d %-12d %-9d %-16s %-13d %-11d %-11d %-17d\r\n", pRecordInfo->device_index, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type, pRecordInfo->record_enable, pRecordInfo->record_cr_index, pRecordInfo->tsu_index, (char*)"Device No Stream", pRecordInfo->TimeOfAllWeek, pRecordInfo->iTSUPauseStatus, pRecordInfo->iTSUResumeStatus, pRecordInfo->iTSUAlarmRecordStatus);
            }
            else if (RECORD_STATUS_NETWORK_ERROR == pRecordInfo->record_status)
            {
                snprintf(rbuf, 256, "\r%-12u %-20s %-32s %-11d %-11d %-13d %-12d %-9d %-16s %-13d %-11d %-11d %-17d\r\n", pRecordInfo->device_index, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type, pRecordInfo->record_enable, pRecordInfo->record_cr_index, pRecordInfo->tsu_index, (char*)"NetWork Unreached", pRecordInfo->TimeOfAllWeek, pRecordInfo->iTSUPauseStatus, pRecordInfo->iTSUResumeStatus, pRecordInfo->iTSUAlarmRecordStatus);
            }
            else if (RECORD_STATUS_NO_TSU == pRecordInfo->record_status)
            {
                snprintf(rbuf, 256, "\r%-12u %-20s %-32s %-11d %-11d %-13d %-12d %-9d %-16s %-13d %-11d %-11d %-17d\r\n", pRecordInfo->device_index, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type, pRecordInfo->record_enable, pRecordInfo->record_cr_index, pRecordInfo->tsu_index, (char*)"No Idle TSU", pRecordInfo->TimeOfAllWeek, pRecordInfo->iTSUPauseStatus, pRecordInfo->iTSUResumeStatus, pRecordInfo->iTSUAlarmRecordStatus);
            }
            else if (RECORD_STATUS_NOT_SUPPORT_MULTI_STREAM == pRecordInfo->record_status)
            {
                snprintf(rbuf, 256, "\r%-12u %-20s %-32s %-11d %-11d %-13d %-12d %-9d %-16s %-13d %-11d %-11d %-17d\r\n", pRecordInfo->device_index, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type, pRecordInfo->record_enable, pRecordInfo->record_cr_index, pRecordInfo->tsu_index, (char*)"No Multi Stream", pRecordInfo->TimeOfAllWeek, pRecordInfo->iTSUPauseStatus, pRecordInfo->iTSUResumeStatus, pRecordInfo->iTSUAlarmRecordStatus);
            }
            else if (RECORD_STATUS_PROC == pRecordInfo->record_status)
            {
                snprintf(rbuf, 256, "\r%-12u %-20s %-32s %-11d %-11d %-13d %-12d %-9d %-16s %-13d %-11d %-11d %-17d\r\n", pRecordInfo->device_index, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type, pRecordInfo->record_enable, pRecordInfo->record_cr_index, pRecordInfo->tsu_index, (char*)"Recording Proc", pRecordInfo->TimeOfAllWeek, pRecordInfo->iTSUPauseStatus, pRecordInfo->iTSUResumeStatus, pRecordInfo->iTSUAlarmRecordStatus);
            }
            else if (RECORD_STATUS_COMPLETE == pRecordInfo->record_status)
            {
                snprintf(rbuf, 256, "\r%-12u %-20s %-32s %-11d %-11d %-13d %-12d %-9d %-16s %-13d %-11d %-11d %-17d\r\n", pRecordInfo->device_index, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type, pRecordInfo->record_enable, pRecordInfo->record_cr_index, pRecordInfo->tsu_index, (char*)"Recording", pRecordInfo->TimeOfAllWeek, pRecordInfo->iTSUPauseStatus, pRecordInfo->iTSUResumeStatus, pRecordInfo->iTSUAlarmRecordStatus);
            }
            else if (RECORD_STATUS_NULL == pRecordInfo->record_status)
            {
                snprintf(rbuf, 256, "\r%-12u %-20s %-32s %-11d %-11d %-13d %-12d %-9d %-16s %-13d %-11d %-11d %-17d\r\n", pRecordInfo->device_index, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type, pRecordInfo->record_enable, pRecordInfo->record_cr_index, pRecordInfo->tsu_index, (char*)"Not Start Record", pRecordInfo->TimeOfAllWeek, pRecordInfo->iTSUPauseStatus, pRecordInfo->iTSUResumeStatus, pRecordInfo->iTSUAlarmRecordStatus);
            }
        }

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
 �� �� ��  : ShowRecordTimeSchedInfo
 ��������  : ��ʾ¼��ʱ����Ϣ
 �������  : int sock
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2015��8��3�� ����һ
    ��    ��   : ���
    �޸�����   : �����ɺ���

*****************************************************************************/
void ShowRecordTimeSchedInfo(int sock)
{
    //int i = 0;
    int iRet = 0;
    int index = 0;
    int pos = 0;
    //int iRecordCRIndex = -1;
    unsigned int uRecordInfoIndex = 0;
    int iRecordInfoIndexCount = 0;
    char strLine[] = "\r----------------------------------------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rDevice Index Device ID            Device Name                                     Record Type Stream Type TSU Index Begin Time End Time Status\r\n";
    char rbuf[256] = {0};
    record_info_t* pRecordInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    record_time_sched_t* pRecordTimeSched = NULL;
    record_time_config_t* pTimeConfig = NULL;
    vector<unsigned int> RecordInfoIndexVector;
    int iBeginHour = 0;
    int iBeginMin = 0;
    int iBeginSec = 0;
    int iEndHour = 0;
    int iEndMin = 0;
    int iEndSec = 0;

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    /* ��ȡ¼����Ե����� */
    RecordInfoIndexVector.clear();
    iRet = get_record_info_index_from_list(RecordInfoIndexVector);

    iRecordInfoIndexCount = RecordInfoIndexVector.size();

    if (iRecordInfoIndexCount <= 0)
    {
        return;
    }

    //printf("\r\n ShowRecordTimeSchedInfo:iRecordInfoIndexCount=%d \r\n", iRecordInfoIndexCount);

    /* ����¼����Ա���������ݣ�ѭ����ȡ¼��ʱ����Ա���������� */
    for (index = 0; index < iRecordInfoIndexCount; index++)
    {
        /* ��ȡ¼��������� */
        uRecordInfoIndex = RecordInfoIndexVector[index];

        //printf("\r\n ShowRecordTimeSchedInfo:uRecordInfoIndex=%u \r\n", uRecordInfoIndex);

        /* ����¼���¼ */
        pRecordInfo = record_info_get_by_record_index(uRecordInfoIndex);

        if (NULL == pRecordInfo)
        {
            continue;
        }

        /* �����߼��豸��Ϣ */
        pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(pRecordInfo->device_index);

        if (NULL == pGBLogicDeviceInfo)
        {
            continue;
        }

        //printf("\r\n ShowRecordTimeSchedInfo:device_index=%u \r\n", pRecordInfo->device_index);

        /* ���Ҷ�Ӧ��ʱ�������Ϣ */
        pRecordTimeSched = record_time_sched_get(uRecordInfoIndex);

        if (NULL == pRecordTimeSched || NULL == pRecordTimeSched->pDayOfWeekTimeList)
        {
            continue;
        }

        //printf("\r\n ShowRecordTimeSchedInfo:osip_list_size(pRecordTimeSched->pDayOfWeekTimeList)=%d \r\n", osip_list_size(pRecordTimeSched->pDayOfWeekTimeList));

        if (osip_list_size(pRecordTimeSched->pDayOfWeekTimeList) > 0)
        {
            for (pos = 0; pos < osip_list_size(pRecordTimeSched->pDayOfWeekTimeList); pos++)
            {
                pTimeConfig = (record_time_config_t*)osip_list_get(pRecordTimeSched->pDayOfWeekTimeList, pos);

                if ((NULL == pTimeConfig) || (pTimeConfig->uID <= 0))
                {
                    continue;
                }

                //printf("\r\n ShowRecordTimeSchedInfo:uID=%d, iBeginTime=%d, iEndTime=%d \r\n", pTimeConfig->uID, pTimeConfig->iBeginTime, pTimeConfig->iEndTime);
                /* ���㿪ʼʱ�� */
                iBeginHour = pTimeConfig->iBeginTime / 3600;
                iBeginMin = (pTimeConfig->iBeginTime - iBeginHour * 3600) / 60;
                iBeginSec = pTimeConfig->iBeginTime - iBeginHour * 3600 - iBeginMin * 60;

                /* �������ʱ�� */
                iEndHour = pTimeConfig->iEndTime / 3600;
                iEndMin = (pTimeConfig->iEndTime - iEndHour * 3600) / 60;
                iEndSec = pTimeConfig->iEndTime - iEndHour * 3600 - iEndMin * 60;

                if (1 == pTimeConfig->iStatus)
                {
                    snprintf(rbuf, 256, "\r%-12u %-20s %-47s %-11d %-11d %-9d %02d:%02d:%02d   %02d:%02d:%02d %-6s\r\n", pRecordInfo->device_index, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type, pRecordInfo->tsu_index, iBeginHour, iBeginMin, iBeginSec, iEndHour, iEndMin, iEndSec, (char*)"Record");
                }
                else
                {
                    snprintf(rbuf, 256, "\r%-12u %-20s %-47s %-11d %-11d %-9d %02d:%02d:%02d   %02d:%02d:%02d %-6s\r\n", pRecordInfo->device_index, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type, pRecordInfo->tsu_index, iBeginHour, iBeginMin, iBeginSec, iEndHour, iEndMin, iEndSec, (char*)"Pause");
                }

                if (sock > 0)
                {
                    send(sock, rbuf, strlen(rbuf), 0);
                }
            }
        }
        else
        {
            snprintf(rbuf, 256, "\r%-12u %-20s %-47s %-11d %-11d %-9d %-10s %-8s %-6s\r\n", pRecordInfo->device_index, pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type, pRecordInfo->tsu_index, (char*)"NULL", (char*)"NULL", (char*)"Pause");

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
