
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
#include "common/log_proc.inc"
#include "common/gblfunc_proc.inc"
#include "common/gblconfig_proc.inc"

#include "record/record_info_mgn.inc"
#include "record/record_srv_proc.inc"

#include "resource/resource_info_mgn.inc"
#include "device/device_info_mgn.inc"
#include "user/user_info_mgn.inc"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/
unsigned long long iRecordInfoLockCount = 0;
unsigned long long iRecordInfoUnLockCount = 0;

unsigned long long iRecordTimeInfoLockCount = 0;
unsigned long long iRecordTimeInfoUnLockCount = 0;

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
record_info_list_t* g_RecordInfoList = NULL;               /* 录像信息队列 */
int current_record_pos = 0;                                /* 当前录像位置 */

record_time_sched_list_t* g_RecordTimeSchedList = NULL;    /* 录像时间策略队列 */
int db_RecordInfo_reload_mark = 0;                         /* 录像信息数据库更新标识:0:不需要更新，1:需要更新数据库 */
int db_RecordTimeSched_reload_mark = 0;                    /* 录像时刻策略信息数据库更新标识:0:不需要更新，1:需要更新数据库 */

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#if DECS("录像信息队列")
/*****************************************************************************
 函 数 名  : record_info_init
 功能描述  : 录像信息结构初始化
 输入参数  : record_info_t ** record_info
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : record_info_free
 功能描述  : 录像信息结构释放
 输入参数  : record_info_t * record_info
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : record_info_list_init
 功能描述  : 录像信息队列初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : record_info_list_free
 功能描述  : 录像信息队列释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : record_info_list_lock
 功能描述  : 录像信息队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : record_info_list_unlock
 功能描述  : 录像信息队列解锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : debug_record_info_list_lock
 功能描述  : 录像信息队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : debug_record_info_list_unlock
 功能描述  : 录像信息队列解锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : record_info_add
 功能描述  : 添加录像信息到队列中
 输入参数  : char* record_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : record_info_remove
 功能描述  : 从队列中移除录像信息
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : record_info_find_by_stream_type
 功能描述  : 查找录像信息
 输入参数  : unsigned int device_index
             int stream_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月5日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : record_info_find_by_stream_type2
 功能描述  : 查找录像信息
 输入参数  : unsigned int device_index
             int stream_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月5日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : record_info_get_by_stream_type_and_record_type
 功能描述  : 根据流类型和录像类型获取数据库配置的录像信息
 输入参数  : unsigned int device_index
             int stream_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年12月23日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : record_info_get_by_stream_type_and_record_type2
 功能描述  : 根据流类型和录像类型获取其他条件配置的录像信息
 输入参数  : unsigned int device_index
             int stream_type
             int record_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月23日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : has_record_info_find_by_stream_type
 功能描述  : 根据录像流类型查找已经录像的录像信息
 输入参数  : unsigned int device_index
             int stream_type
             int record_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月13日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : record_info_find_by_cr_index
 功能描述  : 根据呼叫索引查找录像信息
 输入参数  : int cr_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年9月1日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : find_is_cr_index_used_by_other_record_info
 功能描述  : 查找cr_index是否被录像业务重复使用
 输入参数  : int current_cr_index
             int current_pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月5日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

        if (pos == current_pos) /* 当前使用的跳过 */
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
 函 数 名  : record_info_find_by_cr_index_for_response
 功能描述  : 根据记录查询录像信息，在录像回应消息处理中
 输入参数  : int cr_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月5日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : get_record_cr_index_by_record_index
 功能描述  : 根据录像信息索引查找呼叫索引
 输入参数  : unsigned int uID
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年9月1日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : record_info_get
 功能描述  : 获取录像信息
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月5日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : record_info_get_by_record_index
 功能描述  : 根据录像信息索引获取录像信息
 输入参数  : unsigned int index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月3日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : record_info_get_by_record_type
 功能描述  : 通过设备索引和录像类型查找录像信息
 输入参数  : unsigned int device_index
             int record_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年9月28日 星期日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : record_info_get_by_record_type2
 功能描述  : 通过设备索引和录像类型查找录像信息
 输入参数  : unsigned int device_index
             int record_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年9月28日 星期日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : get_record_info_index_from_list
 功能描述  : 获取录像策略索引
 输入参数  : vector<unsigned int>& RecordInfoIndexVector
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : add_record_info_by_message_cmd
 功能描述  : 根据消息命令增加录像信息
 输入参数  : unsigned int device_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月5日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

            /* 更新数据库的数据 */
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
    else /* 不存在 */
    {
        iRet = record_info_init(&pRecordInfo);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "add_record_info_by_message_cmd() exit---: record_info_init Error \r\n");
            return iRet;
        }

        pRecordInfo->device_index = device_index;               /* 设备索引 */
        pRecordInfo->record_enable = 1;                         /* 是否启动录像 */
        pRecordInfo->record_days = 5;                           /* 录像天数 */
        pRecordInfo->record_timeLen = 10;                       /* 录像时长 */
        pRecordInfo->record_type = EV9000_RECORD_TYPE_NORMAL;   /* 录像类型 */
        pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;   /* 码流类型 */
        pRecordInfo->bandwidth = 20;                            /* 前端带宽*/
        pRecordInfo->TimeOfAllWeek = 1;                         /* 是否全周录像 */

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

        /* 更新数据库的数据 */
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
 函 数 名  : del_record_info_by_message_cmd
 功能描述  : 根据消息命令删除录像信息
 输入参数  : unsigned int device_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月5日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

            /* 删除数据库的数据 */
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
    else /* 不存在 */
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "add_record_info_by_message_cmd() exit---: Record Info Find Error \r\n");
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : set_record_info_list_del_mark
 功能描述  : 设置录像信息删除标识
 输入参数  : int del_mark
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月12日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : delete_record_info_from_list_by_mark
 功能描述  : 根据删除标识，删除录像信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月12日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : check_record_info_from_db_to_list
 功能描述  : 检测录像策略表数据是否有变化，并同步到内存
 输入参数  : DBOper* pRecord_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月12日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 循环查找数据库*/
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

        /* 数据库索引*/
        tmp_uivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("ID", tmp_uivalue);

        pRecordInfo->uID = tmp_uivalue;
        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->uID:%d", pRecordInfo->uID);


        /* 逻辑设备统索引*/
        tmp_uivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("DeviceIndex", tmp_uivalue);

        pRecordInfo->device_index = tmp_uivalue;
        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->device_index:%d", pRecordInfo->device_index);


        /* 是否启动录像*/
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("RecordEnable", tmp_ivalue);

        pRecordInfo->record_enable = tmp_ivalue;
        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->record_enable:%d", pRecordInfo->record_enable);


        /* 录像天数 */
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("Days", tmp_ivalue);

        pRecordInfo->record_days = tmp_ivalue;
        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->record_days:%d", pRecordInfo->record_days);


        /* 录像时长 */
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("TimeLength", tmp_ivalue);

        pRecordInfo->record_timeLen = tmp_ivalue;
        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->record_timeLen:%d", pRecordInfo->record_timeLen);


        /* 录像类型 */
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("Type", tmp_ivalue);

        pRecordInfo->record_type = tmp_ivalue;

        if (pRecordInfo->record_type < EV9000_RECORD_TYPE_NORMAL || pRecordInfo->record_type > EV9000_RECORD_TYPE_BACKUP)
        {
            pRecordInfo->record_type = EV9000_RECORD_TYPE_NORMAL;
        }

        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->record_type:%d", pRecordInfo->record_type);


        /* 是否指定录像 */
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("AssignRecord", tmp_ivalue);

        pRecordInfo->assign_record = tmp_ivalue;

        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->assign_record:%d", pRecordInfo->assign_record);


        /* 指定录像的TSU索引 */
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("TSUIndex", tmp_ivalue);

        pRecordInfo->assign_tsu_index = tmp_ivalue;

        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->assign_tsu_index:%d", pRecordInfo->assign_tsu_index);


        /* 码流类型 */
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


        /* 是否全周录像 */
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("TimeOfAllWeek", tmp_ivalue);

        pRecordInfo->TimeOfAllWeek = tmp_ivalue;
        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->TimeOfAllWeek:%d", pRecordInfo->TimeOfAllWeek);


        /* 所需要的带宽 */
        tmp_ivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("BandWidth", tmp_ivalue);

        pRecordInfo->bandwidth = tmp_ivalue;
        //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() pRecordInfo->bandwidth:%d \r\n", pRecordInfo->bandwidth);

        /* 根据录像类型和逻辑设备索引查找录像信息 */
        pOldRecordInfo = record_info_get_by_stream_type_and_record_type(pRecordInfo->device_index, pRecordInfo->stream_type, pRecordInfo->record_type);

        if (NULL != pOldRecordInfo)
        {
            pOldRecordInfo->del_mark = 0;
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_record_info_from_db_to_list() Set Record Info Del Mark Zero:device_index=%u, record_type=%d, stream_type=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pRecordInfo->stream_type);

            if (pOldRecordInfo->uID != pRecordInfo->uID) /* 数据库索引 */
            {
                pOldRecordInfo->uID = pRecordInfo->uID;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() uID change:device_index=%u, uID=%u \r\n", pRecordInfo->device_index, pRecordInfo->uID);
            }

            if (pOldRecordInfo->record_enable != pRecordInfo->record_enable) /* 是否启动录像 */
            {
                pOldRecordInfo->record_enable = pRecordInfo->record_enable;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() record_enable change:device_index=%u, record_enable=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_enable);
            }

            if (pOldRecordInfo->record_days != pRecordInfo->record_days) /* 录像天数 */
            {
                pOldRecordInfo->record_days = pRecordInfo->record_days;
                pOldRecordInfo->del_mark = 2;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() record_days change:device_index=%u, record_days=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_days);
            }

            if (pOldRecordInfo->record_timeLen != pRecordInfo->record_timeLen) /* 录像时长 */
            {
                pOldRecordInfo->record_timeLen = pRecordInfo->record_timeLen;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() record_timeLen change:device_index=%u, record_timeLen=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_timeLen);
            }

            if (pOldRecordInfo->record_type != pRecordInfo->record_type) /* 录像类型 */
            {
                pOldRecordInfo->record_type = pRecordInfo->record_type;
                pOldRecordInfo->del_mark = 2;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() record_type change:device_index=%u, record_type=%d \r\n", pRecordInfo->device_index, pRecordInfo->record_type);
            }

            if (pOldRecordInfo->assign_record != pRecordInfo->assign_record) /* 是否指定录像 */
            {
                pOldRecordInfo->assign_record = pRecordInfo->assign_record;
                pOldRecordInfo->del_mark = 2;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() assign_record change:device_index=%u, assign_record=%d \r\n", pRecordInfo->device_index, pRecordInfo->assign_record);
            }

            if (pOldRecordInfo->assign_tsu_index != pRecordInfo->assign_tsu_index) /* 指定录像的TSU索引 */
            {
                pOldRecordInfo->assign_tsu_index = pRecordInfo->assign_tsu_index;
                pOldRecordInfo->del_mark = 2;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() assign_record change:device_index=%u, assign_tsu_index=%d \r\n", pRecordInfo->device_index, pRecordInfo->assign_tsu_index);
            }

            if (pOldRecordInfo->stream_type != pRecordInfo->stream_type) /* 码流类型 */
            {
                pOldRecordInfo->stream_type = pRecordInfo->stream_type;
                pOldRecordInfo->del_mark = 2;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() stream_type change:device_index=%u, stream_type=%d \r\n", pRecordInfo->device_index, pRecordInfo->stream_type);
            }

            if (pOldRecordInfo->bandwidth != pRecordInfo->bandwidth) /* 前端带宽*/
            {
                pOldRecordInfo->bandwidth = pRecordInfo->bandwidth;
                pOldRecordInfo->del_mark = 2;
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() bandwidth change:device_index=%u, bandwidth=%d \r\n", pRecordInfo->device_index, pRecordInfo->bandwidth);
            }

            if (pOldRecordInfo->TimeOfAllWeek != pRecordInfo->TimeOfAllWeek) /* 是否全周录像 */
            {
                /* 如果原来没有全周录像，现在变成全周录像了，需要调用一下TSU的恢复接口 */
                if (0 == pOldRecordInfo->TimeOfAllWeek && 1 == pRecordInfo->TimeOfAllWeek)
                {
                    pOldRecordInfo->del_mark = 3;
                    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "check_record_info_from_db_to_list() TimeOfAllWeek change:device_index=%u, TimeOfAllWeek=%d, Need To Notify TSU Resume \r\n", pRecordInfo->device_index, pRecordInfo->TimeOfAllWeek);
                }

                /* 如果原来全周录像，现在变成不是全周录像了，需要调用一下TSU的暂停接口 */
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
            /* 添加到队列 */
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
 函 数 名  : check_plan_action_record_info_from_db_to_list
 功能描述  : 检测预案动作中的报警录像否有变化，并同步到内存
 输入参数  : DBOper* pRecord_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月3日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
    strSQL = "SELECT DISTINCT DeviceIndex FROM PlanActionConfig WHERE TYPE=3 ORDER BY DeviceIndex ASC"; /* 加载预案动作类型为3的报警录像，加入到录像队列 */

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

    /* 循环查找数据库*/
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

        /* 索引固定为0 */
        pRecordInfo->uID = 0;

        /* 逻辑设备统索引*/
        tmp_uivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("DeviceIndex", tmp_uivalue);

        pRecordInfo->device_index = tmp_uivalue;

        /* 是否启动录像*/
        pRecordInfo->record_enable = 1;

        /* 录像天数 */
        pRecordInfo->record_days = 1;

        /* 录像时长 */
        pRecordInfo->record_timeLen = 10;

        /* 录像类型 */
        pRecordInfo->record_type = EV9000_RECORD_TYPE_ALARM;

        /* 码流类型 */
        pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;

        /* 是否指定录像 */
        pRecordInfo->assign_record = 0;

        /* 是否全周录像 */
        pRecordInfo->TimeOfAllWeek = 0;

        /* 前端带宽 */
        pRecordInfo->bandwidth = 1;

        pRecordInfo->del_mark = 0;

        /* 根据报警录像类型和逻辑设备索引查找录像信息 */
        pOldRecordInfo = record_info_get_by_record_type(pRecordInfo->device_index, pRecordInfo->record_type);

        if (NULL == pOldRecordInfo)
        {
            /* 根据主流类型查找录像信息，可能已经存在主流录像，就不需要再增加报警录像信息 */
            pMasterRecordInfo = record_info_get_by_stream_type_and_record_type(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

            if (NULL != pMasterRecordInfo)
            {
                if (pMasterRecordInfo->del_mark == 0)
                {
                    /* 启用状态情况下 */
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

            /* 添加到队列 */
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
            /* 根据主流类型查找录像信息，可能已经存在主流录像，原来老的报警录像信息需要删除掉  */
            pMasterRecordInfo = record_info_get_by_stream_type_and_record_type(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

            if (NULL != pMasterRecordInfo)
            {
                if (pMasterRecordInfo->del_mark == 0)
                {
                    /* 启用状态情况下 */
                    if (1 == pMasterRecordInfo->record_enable)
                    {
                        /* 删除标识不置为0 */
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_plan_action_record_info_from_db_to_list() Record Info Has Exist 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                    }
                    else /* 禁用状态情况下 */
                    {
                        if (pOldRecordInfo->del_mark == 1)
                        {
                            /* 删除标识 */
                            pOldRecordInfo->del_mark = 0;
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_plan_action_record_info_from_db_to_list() Set Record Info Del Mark Zero 1:device_index=%u, record_type=%d, stream_type=%d\r\n", pOldRecordInfo->device_index, pOldRecordInfo->record_type, pOldRecordInfo->stream_type);

                            if (0 == pOldRecordInfo->record_enable)
                            {
                                /* 启用标识 */
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
                else /* 要删除的情况下 */
                {
                    if (pOldRecordInfo->del_mark == 1)
                    {
                        /* 删除标识 */
                        pOldRecordInfo->del_mark = 0;
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_plan_action_record_info_from_db_to_list() Set Record Info Del Mark Zero 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pOldRecordInfo->device_index, pOldRecordInfo->record_type, pOldRecordInfo->stream_type);

                        if (0 == pOldRecordInfo->record_enable)
                        {
                            /* 启用标识 */
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
                    /* 删除标识 */
                    pOldRecordInfo->del_mark = 0;
                    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_plan_action_record_info_from_db_to_list() Set Record Info Del Mark Zero 3:device_index=%u, record_type=%d, stream_type=%d\r\n", pOldRecordInfo->device_index, pOldRecordInfo->record_type, pOldRecordInfo->stream_type);

                    if (0 == pOldRecordInfo->record_enable)
                    {
                        /* 启用标识 */
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
 函 数 名  : check_shdb_daily_upload_pic_record_info_from_db_to_list
 功能描述  : 根据上海地标定时截图任务检查录像任务信息
 输入参数  : DBOper* pRecord_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月19日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 循环查找数据库*/
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

        /* 索引固定为0 */
        pRecordInfo->uID = 0;

        /* 逻辑设备统索引*/
        tmp_uivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("DeviceIndex", tmp_uivalue);

        pRecordInfo->device_index = tmp_uivalue;

        /* 是否启动录像*/
        pRecordInfo->record_enable = 1;

        /* 录像天数 */
        pRecordInfo->record_days = 1;

        /* 录像时长 */
        pRecordInfo->record_timeLen = 10;

        /* 录像类型 */
        pRecordInfo->record_type = EV9000_RECORD_TYPE_NORMAL;

        /* 码流类型 */
        pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;

        /* 是否指定录像 */
        pRecordInfo->assign_record = 0;

        /* 是否全周录像 */
        pRecordInfo->TimeOfAllWeek = 0;

        /* 前端带宽 */
        pRecordInfo->bandwidth = 1;

        pRecordInfo->del_mark = 0;

        /* 根据录像类型和逻辑设备索引查找录像信息 */
        pOldRecordInfo = record_info_get_by_stream_type_and_record_type(pRecordInfo->device_index, pRecordInfo->stream_type, pRecordInfo->record_type);

        if (NULL == pOldRecordInfo)
        {
            /* 可能已经存在报警录像，就不需要再增加录像信息 */
            pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_ALARM);

            if (NULL != pMasterRecordInfo)
            {
                if (pMasterRecordInfo->del_mark == 0)
                {
                    /* 启用状态情况下 */
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
                /* 可能已经存在图片上传录像，就不需要再增加图片录像信息 */
                pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

                if (NULL != pMasterRecordInfo)
                {
                    if (pMasterRecordInfo->del_mark == 0)
                    {
                        /* 启用状态情况下 */
                        if (1 == pMasterRecordInfo->record_enable)
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Has Exist 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                        }
                        else
                        {
                            //printf("check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Not Enable 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);

                            /* 启用标识 */
                            pMasterRecordInfo->record_enable = 1;

                            /* 删除标识 */
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

                        /* 删除标识 */
                        pMasterRecordInfo->del_mark = 0;
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Set Record Info Del Mark Zero 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                    }

                    record_info_free(pRecordInfo);
                    pRecordInfo = NULL;
                    continue;
                }
            }

            /* 添加到队列 */
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
            if (pOldRecordInfo->del_mark == 1) /* 原来的要删除 */
            {
                /* 可能已经存在报警录像，就不需要再增加录像信息 */
                pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_ALARM);

                if (NULL != pMasterRecordInfo)
                {
                    if (pMasterRecordInfo->del_mark == 0)
                    {
                        /* 启用状态情况下 */
                        if (1 == pMasterRecordInfo->record_enable)
                        {
                            /* OldRecordInfo 删除标识不置为0 */
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Has Exist 3:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                        }
                        else /* 禁用状态情况下 */
                        {
                            /* 添加到队列 */
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
                    else /* 要删除的情况下 */
                    {
                        /* 添加到队列 */
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
                    /* 可能已经存在图片上传录像，原来老的图片上传信息需要删除掉  */
                    pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

                    if (NULL != pMasterRecordInfo)
                    {
                        if (pMasterRecordInfo->del_mark == 0)
                        {
                            /* 启用状态情况下 */
                            if (1 == pMasterRecordInfo->record_enable)
                            {
                                /* OldRecordInfo 删除标识不置为0 */
                                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Has Exist 4:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                            }
                            else /* 禁用状态情况下 */
                            {
                                /* 添加到队列 */
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
                        else /* 要删除的情况下 */
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
                        /* 添加到队列 */
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
                if (pOldRecordInfo->record_enable == 0) /* 原来的被禁用了 */
                {
                    /* 可能已经存在报警录像，就不需要再增加录像信息 */
                    pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_ALARM);

                    if (NULL != pMasterRecordInfo)
                    {
                        if (pMasterRecordInfo->del_mark == 0)
                        {
                            /* 启用状态情况下 */
                            if (1 == pMasterRecordInfo->record_enable)
                            {
                                /* OldRecordInfo 删除标识不置为0 */
                                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Has Exist 5:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                            }
                            else /* 禁用状态情况下 */
                            {
                                /* 添加到队列 */
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
                        else /* 要删除的情况下 */
                        {
                            /* 添加到队列 */
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
                        /* 可能已经存在图片上传录像，原来老的图片上传信息需要删除掉  */
                        pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

                        if (NULL != pMasterRecordInfo)
                        {
                            if (pMasterRecordInfo->del_mark == 0)
                            {
                                /* 启用状态情况下 */
                                if (1 == pMasterRecordInfo->record_enable)
                                {
                                    /* OldRecordInfo 删除标识不置为0 */
                                    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_daily_upload_pic_record_info_from_db_to_list() Record Info Has Exist 6:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                                }
                                else /* 禁用状态情况下 */
                                {
                                    /* 添加到队列 */
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
                            else /* 要删除的情况下 */
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
                            /* 添加到队列 */
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
 函 数 名  : check_shdb_alarm_upload_pic_record_info_from_db_to_list
 功能描述  : 根据上海地标报警截图任务检查录像任务信息
 输入参数  : DBOper* pRecord_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月22日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
    strSQL = "SELECT DISTINCT DeviceIndex FROM PlanActionConfig WHERE TYPE=5 ORDER BY DeviceIndex ASC"; /* 加载预案动作类型为5的截图上传，加入到录像队列 */

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

    /* 循环查找数据库*/
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

        /* 索引固定为0 */
        pRecordInfo->uID = 0;

        /* 逻辑设备统索引*/
        tmp_uivalue = 0;
        pRecord_Srv_dboper->GetFieldValue("DeviceIndex", tmp_uivalue);

        pRecordInfo->device_index = tmp_uivalue;

        /* 是否启动录像*/
        pRecordInfo->record_enable = 1;

        /* 录像天数 */
        pRecordInfo->record_days = 1;

        /* 录像时长 */
        pRecordInfo->record_timeLen = 10;

        /* 录像类型 */
        pRecordInfo->record_type = EV9000_RECORD_TYPE_NORMAL;

        /* 码流类型 */
        pRecordInfo->stream_type = EV9000_STREAM_TYPE_MASTER;

        /* 是否指定录像 */
        pRecordInfo->assign_record = 0;

        /* 是否全周录像 */
        pRecordInfo->TimeOfAllWeek = 0;

        /* 前端带宽 */
        pRecordInfo->bandwidth = 1;

        pRecordInfo->del_mark = 0;

        /* 根据录像类型和逻辑设备索引查找录像信息 */
        pOldRecordInfo = record_info_get_by_stream_type_and_record_type(pRecordInfo->device_index, pRecordInfo->stream_type, pRecordInfo->record_type);

        if (NULL == pOldRecordInfo)
        {
            /* 可能已经存在报警录像，就不需要再增加录像信息 */
            pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_ALARM);

            if (NULL != pMasterRecordInfo)
            {
                if (pMasterRecordInfo->del_mark == 0)
                {
                    /* 启用状态情况下 */
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
                /* 可能已经存在图片上传录像，就不需要再增加图片录像信息 */
                pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

                if (NULL != pMasterRecordInfo)
                {
                    if (pMasterRecordInfo->del_mark == 0)
                    {
                        /* 启用状态情况下 */
                        if (1 == pMasterRecordInfo->record_enable)
                        {
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Has Exist 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                        }
                        else
                        {
                            //printf("check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Not Enable 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);

                            /* 启用标识 */
                            pMasterRecordInfo->record_enable = 1;

                            /* 删除标识 */
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

                        /* 删除标识 */
                        pMasterRecordInfo->del_mark = 0;
                        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Set Record Info Del Mark Zero 2:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                    }

                    record_info_free(pRecordInfo);
                    pRecordInfo = NULL;
                    continue;
                }
            }

            /* 添加到队列 */
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
            if (pOldRecordInfo->del_mark == 1) /* 原来的要删除 */
            {
                /* 可能已经存在报警录像，就不需要再增加录像信息 */
                pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_ALARM);

                if (NULL != pMasterRecordInfo)
                {
                    if (pMasterRecordInfo->del_mark == 0)
                    {
                        /* 启用状态情况下 */
                        if (1 == pMasterRecordInfo->record_enable)
                        {
                            /* OldRecordInfo 删除标识不置为0 */
                            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Has Exist 3:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                        }
                        else /* 禁用状态情况下 */
                        {
                            /* 添加到队列 */
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
                    else /* 要删除的情况下 */
                    {
                        /* 添加到队列 */
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
                    /* 可能已经存在图片上传录像，原来老的图片上传信息需要删除掉  */
                    pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

                    if (NULL != pMasterRecordInfo)
                    {
                        if (pMasterRecordInfo->del_mark == 0)
                        {
                            /* 启用状态情况下 */
                            if (1 == pMasterRecordInfo->record_enable)
                            {
                                /* OldRecordInfo 删除标识不置为0 */
                                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Has Exist 4:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                            }
                            else /* 禁用状态情况下 */
                            {
                                /* 添加到队列 */
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
                        else /* 要删除的情况下 */
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
                        /* 添加到队列 */
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
                if (pOldRecordInfo->record_enable == 0) /* 原来的被禁用了 */
                {
                    /* 可能已经存在报警录像，就不需要再增加录像信息 */
                    pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_ALARM);

                    if (NULL != pMasterRecordInfo)
                    {
                        if (pMasterRecordInfo->del_mark == 0)
                        {
                            /* 启用状态情况下 */
                            if (1 == pMasterRecordInfo->record_enable)
                            {
                                /* OldRecordInfo 删除标识不置为0 */
                                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Has Exist 5:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                            }
                            else /* 禁用状态情况下 */
                            {
                                /* 添加到队列 */
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
                        else /* 要删除的情况下 */
                        {
                            /* 添加到队列 */
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
                        /* 可能已经存在图片上传录像，原来老的图片上传信息需要删除掉  */
                        pMasterRecordInfo = record_info_get_by_stream_type_and_record_type2(pRecordInfo->device_index, EV9000_STREAM_TYPE_MASTER, EV9000_RECORD_TYPE_NORMAL);

                        if (NULL != pMasterRecordInfo)
                        {
                            if (pMasterRecordInfo->del_mark == 0)
                            {
                                /* 启用状态情况下 */
                                if (1 == pMasterRecordInfo->record_enable)
                                {
                                    /* OldRecordInfo 删除标识不置为0 */
                                    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_shdb_alarm_upload_pic_record_info_from_db_to_list() Record Info Has Exist 6:device_index=%u, record_type=%d, stream_type=%d\r\n", pMasterRecordInfo->device_index, pMasterRecordInfo->record_type, pMasterRecordInfo->stream_type);
                                }
                                else /* 禁用状态情况下 */
                                {
                                    /* 添加到队列 */
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
                            else /* 要删除的情况下 */
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
                            /* 添加到队列 */
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
 函 数 名  : scan_record_info_list
 功能描述  : 扫描录像配置信息数据
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月16日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "录像任务启动, 当前录像位置:current_record_pos=%d", current_record_pos);

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        return;
    }

    /*  以后增加从数据库表里面对录像任务表，增加新的点位可以及时录像 */
    printf("scan_record_info_list() Begin--- current_record_pos=%d \r\n", current_record_pos);

    for (i = current_record_pos; i < osip_list_size(g_RecordInfoList->pRecordInfoList); i++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, i);

        if (NULL == pRecordInfo)
        {
            continue;
        }

        if (1 == pRecordInfo->del_mark) /* 需要删除的 */
        {
            if (pRecordInfo->record_cr_index >= 0) /* 已经录像的 */
            {
                needstoprecord.push_back(pRecordInfo);
            }

            continue;
        }
        else if (2 == pRecordInfo->del_mark) /* 需要先停止掉的 */
        {
            if (pRecordInfo->record_cr_index >= 0) /* 已经录像的 */
            {
                needstoprecord.push_back(pRecordInfo);
                pRecordInfo->del_mark = 0;
            }

            continue;
        }
        else if (3 == pRecordInfo->del_mark) /* 需要通知TSU恢复 */
        {
            if (pRecordInfo->record_cr_index >= 0) /* 已经录像的 */
            {
                needresumerecord.push_back(pRecordInfo);
                pRecordInfo->del_mark = 0;
            }

            continue;
        }
        else if (4 == pRecordInfo->del_mark) /* 需要通知TSU恢复 */
        {
            if (pRecordInfo->record_cr_index >= 0) /* 已经录像的 */
            {
                needpauserecord.push_back(pRecordInfo);
                pRecordInfo->del_mark = 0;
            }

            continue;
        }

        if (0 == pRecordInfo->record_enable) /* 未启用的 */
        {
            if (pRecordInfo->record_cr_index >= 0) /* 已经录像的 */
            {
                needstoprecord.push_back(pRecordInfo);
            }

            continue;
        }

        if (pRecordInfo->record_cr_index >= 0) /* 已经录像的 */
        {
            /* 查看该record_index是否被其他RecordInfo使用，如果使用，则释放 */
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
                /* 查找逻辑设备信息 */
                pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index2(pRecordInfo->device_index);

                if (NULL != pGBLogicDeviceInfo)
                {
                    if (1 == pGBLogicDeviceInfo->record_type) /* 前端录像 */
                    {
                        needstoprecord.push_back(pRecordInfo);
                    }
                }
            }

            /* 看看TSU是否正常,不正常的停止掉重新录像 */
            if (RECORD_STATUS_COMPLETE == pRecordInfo->record_status
                && pRecordInfo->tsu_index < 0)
            {
                needstoprecord.push_back(pRecordInfo);
            }

            /* 不是全周录像，看TSU状态是否暂停，如果没有暂停，则需要再次暂停 */ //不太明白

            if (RECORD_STATUS_COMPLETE == pRecordInfo->record_status
                && 0 == pRecordInfo->TimeOfAllWeek)
            {
                if (0 == pRecordInfo->iTSUPauseStatus)
                {
                    needpauserecord.push_back(pRecordInfo);
                }
            }

            /* 全周录像，看TSU状态是否恢复，如果没有恢复，则需要再次恢复 */
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
        else /* 没有录像 */
        {
            /* 查找逻辑设备信息 */
            pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index2(pRecordInfo->device_index);

            if (NULL != pGBLogicDeviceInfo)
            {
                if (1 == pGBLogicDeviceInfo->record_type) /* 前端录像 */
                {
                    continue;
                }
            }
        }

        /* 录像策略表中如果没有录像的点位，先全部启动录像 */
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
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "录像任务启动开始调度, 本次需要启动录像调度任务数=%d, 本次需要停止录像调度任务数=%d, 本次需要通知TSU恢复录像调度任务数=%d, 本次需要通知TSU暂停录像调度任务数=%d", needrecord.size(), needstoprecord.size(), needresumerecord.size(), needpauserecord.size());

    /* 处理需要录像的 */
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

    /* 处理需要停止录像的 */
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

    /* 处理需要恢复的 */
    while (!needresumerecord.empty())
    {
        pProcRecordInfo = (record_info_t*) needresumerecord.front();
        needresumerecord.pop_front();

        if (NULL != pProcRecordInfo)
        {
            pCrData = call_record_get(pProcRecordInfo->record_cr_index);

            if (NULL != pCrData)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "录像策略从时间段录像修改为全周录像，通知TSU恢复录像: 点位ID=%s, TSU IP地址=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video strategy change from selected time slot record to weekly record，notify TSU to recover videro: point ID=%s, TSU IPaddress=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);

                /* 恢复录像 */
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

    /* 处理需要暂停的 */
    while (!needpauserecord.empty())
    {
        pProcRecordInfo = (record_info_t*) needpauserecord.front();
        needpauserecord.pop_front();

        if (NULL != pProcRecordInfo)
        {
            pCrData = call_record_get(pProcRecordInfo->record_cr_index);

            if (NULL != pCrData)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "录像策略从全周录像修改为时间段录像，通知TSU暂停录像: 点位ID=%s, TSU IP地址=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video strategy change from weekly record to selected time slot record, notify TSU to pause video: point ID=%s, TSU IP address=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);

                /* 暂停录像 */
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

                /* 去除策略中的标志位 */
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
 函 数 名  : scan_record_info_list_for_monitor_print
 功能描述  : 扫描录像配置信息数据
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月16日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
    needtoproc_recordinfo_queue not_enable_record_info; /* 没有启用的 */
    needtoproc_recordinfo_queue front_record_info; /* 前端录像的 */
    needtoproc_recordinfo_queue has_record_info; /* 正在录像的 */
    needtoproc_recordinfo_queue not_record_info; /* 没有录像的 */
    cr_t* pCrData = NULL;

    if ((NULL == g_RecordInfoList) || (NULL == g_RecordInfoList->pRecordInfoList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "scan_record_info_list_for_monitor_print() exit---: Param Error \r\n");
        return;
    }

    not_enable_record_info.clear();/* 没有启用的 */
    front_record_info.clear();/* 前端录像的 */
    has_record_info.clear(); /* 正在录像的 */
    not_record_info.clear(); /* 没有录像的 */

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
            /* 查找逻辑设备信息 */
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

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "系统当前录像任务信息:配置的总录像条数=%d, 已录的录像任务数=%d, 未录的录像任务数=%d, 未启用的录像任务数=%d, 前端录像的任务数=%d", osip_list_size(g_RecordInfoList->pRecordInfoList), has_record_info.size(), not_record_info.size(), not_enable_record_info.size(), front_record_info.size());
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "System Current Reocrd Task:Total Record Task=%d, Has Record Task=%d, Not Record Task=%d, Not Enable Record Task=%d, Front Record Task=%d", osip_list_size(g_RecordInfoList->pRecordInfoList), has_record_info.size(), not_record_info.size(), not_enable_record_info.size(), front_record_info.size());

    if (!has_record_info.empty())
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "已录的录像任务数=%d, 详细信息如下:", has_record_info.size());
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Has Record Task=%d, Detail Info As Flow:", has_record_info.size());

        while (!has_record_info.empty())
        {
            pProcRecordInfo = (record_info_t*) has_record_info.front();
            has_record_info.pop_front();

            if (NULL != pProcRecordInfo)
            {
                /* 查找逻辑设备信息 */
                pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(pProcRecordInfo->device_index);

                if (NULL == pGBLogicDeviceInfo)
                {
                    continue;
                }

                /* 查找TSU信息信息 */
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

                /* 检测录像任务是否正常  */
                pCrData = call_record_get(pProcRecordInfo->record_cr_index);

                if (NULL == pCrData)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "录像点位ID=%s, 点位名称=%s, 录像类型=%d, 码流类型=%d, 录像的TSU ID=%s, TSU IP=%s, 录像的呼叫任务不存在, 移除录像信息", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, pTsuResourceInfo->tsu_device_id, tsu_ip);
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
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "录像点位ID=%s, 点位名称=%s, 录像类型=%d, 码流类型=%d, 录像的TSU ID=%s, TSU IP=%s, 录像的呼叫任务类型不匹配, 移除录像信息", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, pTsuResourceInfo->tsu_device_id, tsu_ip);
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
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "录像点位ID=%s, 点位名称=%s, 录像类型=%d, 码流类型=%d, 录像的TSU ID=%s, TSU IP=%s, 录像的呼叫任务中点位ID和录像信息中的ID不匹配, 移除录像信息", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, pTsuResourceInfo->tsu_device_id, tsu_ip);
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

                /* 检查是否还有其他录像的呼叫任务存在 */
                other_cr_pos = find_recordinfo_has_other_cr_data(pGBLogicDeviceInfo->device_id, pProcRecordInfo->stream_type, pProcRecordInfo->record_cr_index);

                if (other_cr_pos >= 0)
                {
                    i = StopDeviceRecord(other_cr_pos);
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "去除掉重复的录像任务信息:点位ID=%s, 点位名称=%s, 录像类型=%d, 码流类型=%d, 录像任务record_cr_index=%d, 重复的任务cr_pos=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, pProcRecordInfo->record_cr_index, other_cr_pos);
                }

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "已录的点位ID=%s, 点位名称=%s, 录像类型=%d, 码流类型=%d, 录像的TSU ID=%s, TSU IP=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, pTsuResourceInfo->tsu_device_id, tsu_ip);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Has Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, TSU ID=%s, TSU IP=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, pTsuResourceInfo->tsu_device_id, tsu_ip);
            }
        }

        has_record_info.clear();
    }

    if (!not_record_info.empty())
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "未录的录像任务数=%d, 详细信息如下:", not_record_info.size());
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Not Record Task=%d, Detail Info As Flow:", not_record_info.size());

        while (!not_record_info.empty())
        {
            pProcRecordInfo = (record_info_t*) not_record_info.front();
            not_record_info.pop_front();

            if (NULL != pProcRecordInfo)
            {
                /* 查找逻辑设备信息 */
                pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(pProcRecordInfo->device_index);

                if (NULL == pGBLogicDeviceInfo)
                {
                    continue;
                }

                if (RECORD_STATUS_INIT == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "未录的点位ID=%s, 点位名称=%s, 录像类型=%d, 码流类型=%d, 未录像原因=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"录像没有成功");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"Not Success");
                }
                else if (RECORD_STATUS_OFFLINE == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "未录的点位ID=%s, 点位名称=%s, 录像类型=%d, 码流类型=%d, 未录像原因=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"点位不在线");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"Device Off Line");
                }
                else if (RECORD_STATUS_NOSTREAM == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "未录的点位ID=%s, 点位名称=%s, 录像类型=%d, 码流类型=%d, 未录像原因=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"点位没有码流");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"Device No Stream");
                }
                else if (RECORD_STATUS_NETWORK_ERROR == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "未录的点位ID=%s, 点位名称=%s, 录像类型=%d, 码流类型=%d, 未录像原因=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"前端设备网络不可达");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"NetWork Unreached");
                }
                else if (RECORD_STATUS_NO_TSU == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "未录的点位ID=%s, 点位名称=%s, 录像类型=%d, 码流类型=%d, 未录像原因=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"没有空闲的TSU");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"No Idle TSU");
                }
                else if (RECORD_STATUS_NOT_SUPPORT_MULTI_STREAM == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "未录的点位ID=%s, 点位名称=%s, 录像类型=%d, 码流类型=%d, 未录像原因=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"录像的点位不支持多码流");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"No Multi Stream");
                }
                else if (RECORD_STATUS_PROC == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "未录的点位ID=%s, 点位名称=%s, 录像类型=%d, 码流类型=%d, 未录像原因=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"录像流程没有结束");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"Recording Proc");
                }
                else if (RECORD_STATUS_NULL == pProcRecordInfo->record_status)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "未录的点位ID=%s, 点位名称=%s, 录像类型=%d, 码流类型=%d, 未录像原因=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"还没有开始调度录像");
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Not Record Device ID=%s, Device Name=%s, Record Type=%d, Stream Type=%d, Not Record Reason=%s", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, (char*)"Not Start Record");
                }
            }
        }

        not_record_info.clear();
    }

    return;
}

/*****************************************************************************
 函 数 名  : RecordInfo_db_refresh_proc
 功能描述  : 设置录像策略信息数据库更新操作标识
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月18日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int RecordInfo_db_refresh_proc()
{
    if (1 == db_RecordInfo_reload_mark) /* 正在执行 */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "录像策略配置数据库信息正在同步");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Record Info database information are synchronized");
        return 0;
    }

    db_RecordInfo_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 函 数 名  : check_RecordInfo_need_to_reload_begin
 功能描述  : 检查是否需要同步录像策略配置开始
 输入参数  : DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月18日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void check_RecordInfo_need_to_reload_begin(DBOper* pDboper)
{
    /* 检查是否需要更新数据库标识 */
    if (!db_RecordInfo_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步录像策略配置数据库信息: 开始---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization record info database information: begain---");
    printf("check_RecordInfo_need_to_reload_begin() Begin--- \r\n");

    /* 设置录像队列的删除标识 */
    set_record_info_list_del_mark(1);
    printf("check_RecordInfo_need_to_reload_begin() set_record_info_list_del_mark End--- \r\n");

    /* 将数据库中的变化数据同步到内存 */
    check_record_info_from_db_to_list(pDboper);
    printf("check_RecordInfo_need_to_reload_begin() check_record_info_from_db_to_list End--- \r\n");

    /* 将预案动作表中的报警录像数据库中的变化数据同步到内存 */
    check_plan_action_record_info_from_db_to_list(pDboper);
    printf("check_RecordInfo_need_to_reload_begin() check_plan_action_record_info_from_db_to_list End--- \r\n");

    /* 将上海地标定时任务截图上传录像数据库中的变化数据同步到内存 */
    check_shdb_daily_upload_pic_record_info_from_db_to_list(pDboper);
    printf("check_RecordInfo_need_to_reload_begin() check_shdb_daily_upload_pic_record_info_from_db_to_list End--- \r\n");

    /* 将上海地标报警截图上传录像数据库中的变化数据同步到内存 */
    check_shdb_alarm_upload_pic_record_info_from_db_to_list(pDboper);
    printf("check_RecordInfo_need_to_reload_begin() End--- \r\n");

    return;
}

/*****************************************************************************
 函 数 名  : check_RecordInfo_need_to_reload_end
 功能描述  : 检查是否需要同步录像策略配置表结束
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月18日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void check_RecordInfo_need_to_reload_end()
{
    /* 检查是否需要更新数据库标识 */
    if (!db_RecordInfo_reload_mark)
    {
        return;
    }

    printf("check_RecordInfo_need_to_reload_end() Begin--- \r\n");

    /* 删除多余的录像信息 */
    delete_record_info_from_list_by_mark();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步录像策略配置数据库信息: 结束---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization record info database information: end---");
    db_RecordInfo_reload_mark = 0;
    printf("check_RecordInfo_need_to_reload_end() End--- \r\n");

    return;
}
#endif

#if DECS("录像时刻策略队列")
/*****************************************************************************
 函 数 名  : record_time_config_init
 功能描述  : 时间策略结构体初始化
 输入参数  : record_time_config_t** time_config
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : record_time_config_free
 功能描述  : 时间策略结构体释放
 输入参数  : record_time_config_t* time_config
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : record_time_config_add
 功能描述  : 时间策略添加
 输入参数  : osip_list_t* pDayOfWeekTimeList
             unsigned int uID
             int iBeginTime
             int iEndTime
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : record_time_config_get
 功能描述  : 时间策略获取
 输入参数  : osip_list_t* pDayOfWeekTimeList
             unsigned int uID
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : get_record_status_from_record_time_config
 功能描述  : 获取分段录像的录像状态
 输入参数  : osip_list_t * pDayOfWeekTimeList
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月3日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : record_time_sched_init
 功能描述  : 录像时刻策略结构初始化
 输入参数  : record_time_sched_t** record_time_sched
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 一天的时间信息队列初始化 */
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
 函 数 名  : record_time_sched_free
 功能描述  : 录像时刻策略结构释放
 输入参数  : record_time_sched_t* record_time_sched
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 一天的时间信息队列初释放 */
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
 函 数 名  : record_time_sched_list_init
 功能描述  : 录像时刻策略队列初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : record_time_sched_list_free
 功能描述  : 录像时刻策略队列释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : record_time_sched_list_lock
 功能描述  : 录像时刻策略队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : record_time_sched_list_unlock
 功能描述  : 录像时刻策略队列解锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : debug_record_time_sched_list_lock
 功能描述  : 录像时刻策略队列锁定
 输入参数  : const char* file
             int line
             const char* func
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : debug_record_time_sched_list_unlock
 功能描述  : 录像时刻策略队列解锁
 输入参数  : const char* file
             int line
             const char* func
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : record_time_sched_add
 功能描述  : 录像时刻策略添加
 输入参数  : unsigned int uID
             int record_cr_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : record_time_sched_get
 功能描述  : 获取录像时刻策略
 输入参数  : unsigned int uID
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : set_record_time_sched_list_del_mark
 功能描述  : 设置录像时刻策略删除标识
 输入参数  : int del_mark
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

        /* 时间信息队列 */
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
 函 数 名  : delete_record_time_sched_from_list_by_mark
 功能描述  : 根据录像时刻策略删除标识移除不必要的内容
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
            /* 通知TSU暂停 */
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
            /* 时间信息队列 */
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
                    /* 通知TSU暂停 */
                    if (1 == pTimeConfig->iStatus) /* 如果正在录像，通知TSU暂停 */
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

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "录像策略时间段删除, 通知TSU暂停录像: 点位ID=%s, TSU IP地址=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video strategy time slot delete, notify TSU to pause video: point ID=%s, TSU IP address=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);

            /* 暂停录像 */
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
 函 数 名  : check_record_time_sched_config_from_db_to_list
 功能描述  : 将数据库的录像时间策略同步到内存中
 输入参数  : record_time_sched_t* pRecordTimeSched
             DBOper* pDBOper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 获取本地时区时间, 组建查询条件 */
    now = time(NULL);
    localtime_r(&now, &p_tm);

    /* p_tm.tm_wday; 星期:取值区间为[0,6]，其中0代表星期天，1代表星期一，以此类推 */

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

    /* 循环查找时间段信息 */
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

    /* 循环查找数据库*/
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
                if (1 == pTimeConfig->iStatus) /* 如果正在录像，先暂停一下 */
                {
                    pCrData = call_record_get(pRecordTimeSched->record_cr_index);

                    if (NULL != pCrData)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "录像策略时间段发生变化, 通知TSU暂停录像: 点位ID=%s, TSU IP地址=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video strategy time slot changed, notify TSU to pause video: point ID=%s, TSU IP address=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);

                        /* 暂停录像 */
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

            /* 添加到队列 */
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
 函 数 名  : check_record_time_sched_from_db_to_list
 功能描述  : 检查数据中是否有新的配置数据到录像时刻策略表
 输入参数  : DBOper* pDBOper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 获取录像策略的索引 */
    RecordInfoIndexVector.clear();
    iRet = get_record_info_index_from_list(RecordInfoIndexVector);

    iRecordInfoIndexCount = RecordInfoIndexVector.size();

    if (iRecordInfoIndexCount <= 0)
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "check_record_time_sched_from_db_to_list() exit---: Record Info Index NULL \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_record_time_sched_from_db_to_list:iRecordInfoIndexCount=%d \r\n", iRecordInfoIndexCount);

    /* 根据录像策略表里面的数据，循环获取录像时间策略表里面的数据 */
    for (index = 0; index < iRecordInfoIndexCount; index++)
    {
        /* 获取录像策略索引 */
        uRecordInfoIndex = RecordInfoIndexVector[index];

        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_record_time_sched_from_db_to_list() uRecordInfoIndex=%u \r\n", uRecordInfoIndex);

        iRecordCRIndex = get_record_cr_index_by_record_index(uRecordInfoIndex);

        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "check_record_time_sched_from_db_to_list() get_record_cr_index_by_record_index:iRecordCRIndex=%d \r\n", iRecordCRIndex);

        if (iRecordCRIndex < 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "check_record_time_sched_from_db_to_list() Get Record CR Index Error: RecordInfoIndex=%u\r\n", uRecordInfoIndex);
            continue;
        }

        /* 查找对应的时间策略信息 */
        pRecordTimeSched = record_time_sched_get(uRecordInfoIndex);

        if (NULL == pRecordTimeSched) /* 如果不存在，则添加 */
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

            /* 再次获取一下 */
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
 函 数 名  : scan_record_time_sched_list
 功能描述  : 扫描录像时刻策略队列数据
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月27日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 处理需要录像的 */
    while (!needprocqueue.empty())
    {
        pRecordTimeSched = (record_time_sched_t*) needprocqueue.front();
        needprocqueue.pop_front();

        if (NULL != pRecordTimeSched)
        {
            /* 时间段录像处理 */
            iRet = RecordTimeSchedConfigProc(pRecordTimeSched);
        }
    }

    needprocqueue.clear();

    return;
}

/*****************************************************************************
 函 数 名  : RecordTimeSchedConfig_db_refresh_proc
 功能描述  : 设置录像时刻策略配置信息数据库更新操作标识
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月18日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int RecordTimeSchedConfig_db_refresh_proc()
{
    if (1 == db_RecordTimeSched_reload_mark) /* 正在执行 */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "录像时刻策略配置数据库信息正在同步");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Record Time Sched Info database information are synchronized");
        return 0;
    }

    db_RecordTimeSched_reload_mark = 1;
    return 0;
}

/*****************************************************************************
 函 数 名  : check_RecordTimeSchedConfig_need_to_reload_begin
 功能描述  : 检查是否需要同步录像时刻策略配置开始
 输入参数  : DBOper* pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月18日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void check_RecordTimeSchedConfig_need_to_reload(DBOper* pDboper)
{
    /* 检查是否需要更新数据库标识 */
    if (!db_RecordTimeSched_reload_mark)
    {
        return;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步录像时刻策略配置数据库信息: 开始---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization record time sched info database information: begain---");

    /* 设置录像时间策略队列的删除标识 */
    set_record_time_sched_list_del_mark(1);

    /* 将数据库中的变化数据同步到内存 */
    check_record_time_sched_from_db_to_list(pDboper);

    /* 删除多余的录像时刻信息,需要放在下面执行前面执行，不然可能执行了恢复之后又被删除了 */
    delete_record_time_sched_from_list_by_mark();

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "同步录像时刻策略配置数据库信息: 结束---");
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Synchronization record time sched info database information: end---");
    db_RecordTimeSched_reload_mark = 0;

    return;
}

#endif
/*****************************************************************************
 函 数 名  : AddRecordInfo2DB
 功能描述  : 添加录像任务到数据库
 输入参数  : record_info_t* pRecordInfo
                            DBOper* pRecord_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月5日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 1、查询SQL 语句*/
    strQuerySQL.clear();
    strQuerySQL = "select * from RecordSchedConfig WHERE DeviceIndex = ";
    strQuerySQL += strDeviceIndex;


    /* 2、插入SQL 语句*/
    strInsertSQL.clear();
    strInsertSQL = "insert into RecordSchedConfig (DeviceIndex,RecordEnable,Days,TimeLength,Type,TimeOfAllWeek,BandWidth) values (";

    /* 逻辑设备索引*/
    strInsertSQL += strDeviceIndex;

    strInsertSQL += ",";

    /* 是否启用 */
    strInsertSQL += strRecordEnable;

    strInsertSQL += ",";

    /* 录像天数*/
    strInsertSQL += strDays;

    strInsertSQL += ",";

    /* 录像时长*/
    strInsertSQL += strTimeLength;

    strInsertSQL += ",";

    /* 录像类型*/
    strInsertSQL += strType;

    strInsertSQL += ",";

    /* 全录*/
    strInsertSQL += strTimeOfAllWeek;

    strInsertSQL += ",";

    /* 带宽*/
    strInsertSQL += strBandWidth;

    strInsertSQL += ")";


    /* 3、更新SQL 语句*/
    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE RecordSchedConfig SET ";

    /* 是否录像*/
    strUpdateSQL += "RecordEnable = ";
    strUpdateSQL += strRecordEnable;

    strUpdateSQL += ",";

    /* 全录 */
    strUpdateSQL += "TimeOfAllWeek = ";
    strUpdateSQL += strTimeOfAllWeek;

    strUpdateSQL += " WHERE DeviceIndex = ";
    strUpdateSQL += strDeviceIndex;

    /* 查询数据库 */
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
 函 数 名  : delRecordInfo2DB
 功能描述  : 从数据库中删除录像任务
 输入参数  : unsigned int device_index
             DBOper* pRecord_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月5日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 更新 SQL 语句*/
    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE RecordSchedConfig SET RecordEnable = 0 WHERE DeviceIndex = ";
    strUpdateSQL += strDeviceIndex;

    /* 操作数据库 */
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
 函 数 名  : RecordTimeSchedConfigProc
 功能描述  : 根据录像时刻策略表启动或者停止录像任务
 输入参数  : record_time_sched_t* pRecordTimeSched
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月29日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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

    /* 计算当前时间 */
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

        if (0 == pTimeConfig->iStatus) /* 看是否需要开始录像 */
        {
            if (iTimeNow >= pTimeConfig->iBeginTime - 30 && iTimeNow < pTimeConfig->iEndTime + 30)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "录像策略时间段录像通知TSU恢复录像: 点位ID=%s, TSU IP地址=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video strategy time slot record notify TSU to recover video: point ID=%s, TSU IP address=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);

                /* 查找录像记录, 看是否已经在报警录像 */
                pRecordInfo = record_info_get_by_record_index(pTimeConfig->uID);

                if (NULL != pRecordInfo)
                {
                    if (0 == pRecordInfo->iTSUAlarmRecordStatus)
                    {
                        /* 启动录像 */
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
                    /* 启动录像 */
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

        if (1 == pTimeConfig->iStatus) /* 看是否需要停止录像 */
        {
            if (iTimeNow >= pTimeConfig->iEndTime + 30)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "录像策略时间段录像通知TSU暂停录像: 点位ID=%s, TSU IP地址=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video strategy time slot record notify TSU to pause video: point ID=%s, TSU IP address=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);

                /* 查找录像记录, 看是否已经在报警录像, 如果有报警录像在，则不能暂停 */
                pRecordInfo = record_info_get_by_record_index(pTimeConfig->uID);

                if (NULL != pRecordInfo)
                {
                    if (0 == pRecordInfo->iTSUAlarmRecordStatus)
                    {
                        /* 暂停录像 */
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
                    /* 暂停录像 */
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
 函 数 名  : RemoveRecordTimeSchedConfig
 功能描述  : 移除录像策略标识
 输入参数  : record_time_sched_t* pRecordTimeSched
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月11日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : RemoveRecordTimeSchedMark
 功能描述  : 移除录像策略标识
 输入参数  : unsigned int uID
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月11日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 处理需要录像的 */
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
 函 数 名  : RemoveDeviceRecordInfo
 功能描述  : 移除设备录像信息
 输入参数  : char* device_id
             int stream_type
             int tsu_resource_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年12月2日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

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

    /* 获取逻辑设备索引 */
    device_index = Get_GBLogicDevice_Index_By_Device_ID(device_id);

    if (device_index <= 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "RemoveDeviceRecordInfo() exit---: Get_GBLogicDevice_Index_By_Device_ID Error:device_id=%s \r\n", device_id);
        return -1;
    }

    /* 获取TSU资源信息 */
    pTsuResourceInfo = tsu_resource_info_get(tsu_resource_index);

    if (NULL == pTsuResourceInfo)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "RemoveDeviceRecordInfo() exit---: Get TSU Resource Info Error:tsu_resource_index=%d \r\n", tsu_resource_index);
        return -1;
    }

    /* 移除录像信息 */
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
 函 数 名  : find_record_config_by_device_index
 功能描述  : 根据点位索引查找点位是否配置了录像
 输入参数  : unsigned int uDeviceIndex
             DBOper * pDboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月17日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : ShowRecordInfo
 功能描述  : 显示录像信息
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月27日 星期六
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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

        if (0 == type) /* 显示未录像的 */
        {
            if (pRecordInfo->record_cr_index >= 0)
            {
                continue;
            }
        }
        else if (1 == type) /* 显示已录像的 */
        {
            if (pRecordInfo->record_cr_index < 0)
            {
                continue;
            }
        }

        /* 查找逻辑设备信息 */
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
 函 数 名  : ShowRecordTimeSchedInfo
 功能描述  : 显示录像时刻信息
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月3日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 获取录像策略的索引 */
    RecordInfoIndexVector.clear();
    iRet = get_record_info_index_from_list(RecordInfoIndexVector);

    iRecordInfoIndexCount = RecordInfoIndexVector.size();

    if (iRecordInfoIndexCount <= 0)
    {
        return;
    }

    //printf("\r\n ShowRecordTimeSchedInfo:iRecordInfoIndexCount=%d \r\n", iRecordInfoIndexCount);

    /* 根据录像策略表里面的数据，循环获取录像时间策略表里面的数据 */
    for (index = 0; index < iRecordInfoIndexCount; index++)
    {
        /* 获取录像策略索引 */
        uRecordInfoIndex = RecordInfoIndexVector[index];

        //printf("\r\n ShowRecordTimeSchedInfo:uRecordInfoIndex=%u \r\n", uRecordInfoIndex);

        /* 查找录像记录 */
        pRecordInfo = record_info_get_by_record_index(uRecordInfoIndex);

        if (NULL == pRecordInfo)
        {
            continue;
        }

        /* 查找逻辑设备信息 */
        pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(pRecordInfo->device_index);

        if (NULL == pGBLogicDeviceInfo)
        {
            continue;
        }

        //printf("\r\n ShowRecordTimeSchedInfo:device_index=%u \r\n", pRecordInfo->device_index);

        /* 查找对应的时间策略信息 */
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
                /* 计算开始时间 */
                iBeginHour = pTimeConfig->iBeginTime / 3600;
                iBeginMin = (pTimeConfig->iBeginTime - iBeginHour * 3600) / 60;
                iBeginSec = pTimeConfig->iBeginTime - iBeginHour * 3600 - iBeginMin * 60;

                /* 计算结束时间 */
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
