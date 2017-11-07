
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
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern GBLogicDevice_Info_MAP g_GBLogicDeviceInfoMap; /* 标准逻辑设备信息队列 */
extern int g_DECMediaTransferFlag;                    /* 下级媒体切电视墙是否经过本地TSU转发,默认转发 */
extern GBDevice_Info_MAP g_GBDeviceInfoMap;           /* 标准物理设备信息队列 */
extern route_info_list_t* g_RouteInfoList ;           /* 路由信息队列 */

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/
unsigned long long iCompressTaskDataLockCount = 0;
unsigned long long iCompressTaskDataUnLockCount = 0;

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
COMPRESS_TASK_MAP g_CompressTaskMap; /* 呼叫链接数据队列 */
#ifdef MULTI_THR
osip_mutex_t* g_CompressTaskMapLock = NULL;
#endif

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#if DECS("压缩任务队列")
/*****************************************************************************
 函 数 名  : compress_task_init
 功能描述  : 呼叫链接结构初始化
 输入参数  : compress_task_t** compress_task
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月19日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : compress_task_free
 功能描述  : 呼叫链接结构释放
 输入参数  : compress_task_t * compress_task
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : compress_task_list_init
 功能描述  : 呼叫链接队列初始化
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
 函 数 名  : compress_task_list_free
 功能描述  : 呼叫链接队列释放
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
 函 数 名  : compress_task_list_lock
 功能描述  : 呼叫链接队列锁定
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
 函 数 名  : compress_task_list_unlock
 功能描述  : 呼叫链接队列解锁
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
 函 数 名  : debug_compress_task_list_lock
 功能描述  : 呼叫链接队列锁定
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
 函 数 名  : debug_compress_task_list_unlock
 功能描述  : 呼叫链接队列解锁
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
 函 数 名  : compress_task_add
 功能描述  : 添加呼叫链接到队列中
 输入参数  : call_type_t call_type
                            char* call_id
                            int caller_ua_index
                            char* caller_id
                            char* callee_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
 函 数 名  : compress_task_remove
 功能描述  : 从队列中移除呼叫链接
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
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "更新压缩任务ZRV分配信息失败, 压缩任务队列为空:记录编号=%s, AssignFlag=%d, zrv_device_ip=%s", task_id, iAssignFlag, zrv_device_ip);
        return -1;
    }

    Itr = g_CompressTaskMap.find(task_id);

    if (Itr == g_CompressTaskMap.end())
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "更新压缩任务ZRV分配信息失败, 没有找到对应的压缩任务记录:记录编号=%s, AssignFlag=%d, zrv_device_ip=%s", task_id, iAssignFlag, zrv_device_ip);
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

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "更新压缩任务ZRV分配信息到内存成功:记录编号=%s, AssignFlag=%d, zrv_device_ip=%s", task_id, iAssignFlag, zrv_device_ip);
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
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "更新压缩任务结果信息失败, 压缩任务队列为空:记录编号=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
        return -1;
    }

    Itr = g_CompressTaskMap.find(task_id);

    if (Itr == g_CompressTaskMap.end())
    {
        COMPRESS_TASK_SMUTEX_UNLOCK();
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "compress_task_set_task_result_info() exit---: task_id=%s Not Found \r\n", task_id);
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "更新压缩任务结果信息失败, 没有找到对应的压缩任务记录:记录编号=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
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

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "更新压缩任务结果信息到内存成功:记录编号=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);

    return iRet;
}

/*****************************************************************************
 函 数 名  : compress_task_find
 功能描述  : 根据任务ID查找呼叫记录
 输入参数  : char* task_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年11月29日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

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
 函 数 名  : scan_compress_task_list_for_wait_expire
 功能描述  : 扫描等待响应的任务队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年12月26日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

        if (pCompressTaskData->wait_answer_expire >= 300) /* 五个小时超时 */
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
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "压缩任务等待响应超时, 重新下发给ZRV设备失败, 原有ZRV设备IP地址失效, 重新获取可用的ZRV设备IP地址失败, 记录编号=%s, 原有ZRV设备IP=%s", (char*)COMPRESS_TASKIDVector[index].c_str(), pCompressTaskData->strZRVDeviceIP);
                    pCompressTaskData->wait_answer_expire = 300;
                    continue;
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "压缩任务等待响应超时, 重新下发给ZRV设备失败, 原有ZRV设备IP地址失效, 重新获取可用的ZRV设备IP地址, 记录编号=%s, 原有ZRV设备IP=%s, 新的ZRV设备IP=%s", (char*)COMPRESS_TASKIDVector[index].c_str(), pCompressTaskData->strZRVDeviceIP, device_ip);
                    iRet = UpdateCompressTaskAssignInfo((char*)COMPRESS_TASKIDVector[index].c_str(), 1, device_ip, pDbOper);
                }
            }
            else
            {
                /* 查看ZRV设备是否已经掉线，掉线了重新分配给其他ZRV设备 */
                pZRVDeviceInfo = ZRVDevice_info_find(pCompressTaskData->strZRVDeviceIP);

                if (NULL == pZRVDeviceInfo)
                {
                    /* 重新分配 */
                    device_ip = get_zrv_device_ip_by_min_task_count(pCompressTaskData->strPlatformIP);

                    if (NULL == device_ip)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "压缩任务等待响应超时, 重新下发给ZRV设备失败, 原有ZRV设备失效, 重新获取可用的ZRV设备IP地址失败, 记录编号=%s, 原有ZRV设备IP=%s", (char*)COMPRESS_TASKIDVector[index].c_str(), pCompressTaskData->strZRVDeviceIP);
                        pCompressTaskData->wait_answer_expire = 300;
                        continue;
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "压缩任务等待响应超时, 重新下发给ZRV设备失败, 原有ZRV设备失效, 重新获取可用的ZRV设备IP地址, 记录编号=%s, 原有ZRV设备IP=%s, 新的ZRV设备IP=%s", (char*)COMPRESS_TASKIDVector[index].c_str(), pCompressTaskData->strZRVDeviceIP, device_ip);
                        iRet = UpdateCompressTaskAssignInfo((char*)COMPRESS_TASKIDVector[index].c_str(), 1, device_ip, pDbOper);
                    }
                }
                else
                {
                    if (pZRVDeviceInfo->reg_status <= 0 || pZRVDeviceInfo->tcp_sock <= 0)
                    {
                        /* 重新分配 */
                        device_ip = get_zrv_device_ip_by_min_task_count(pCompressTaskData->strPlatformIP);

                        if (NULL == device_ip)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "压缩任务等待响应超时, 重新下发给ZRV设备失败, 原有ZRV设备掉线, 重新获取可用的ZRV设备IP地址失败, 记录编号=%s, 原有ZRV设备IP=%s", (char*)COMPRESS_TASKIDVector[index].c_str(), pCompressTaskData->strZRVDeviceIP);
                            pCompressTaskData->wait_answer_expire = 300;
                            continue;
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "压缩任务等待响应超时, 重新下发给ZRV设备失败, 原有ZRV设备掉线, 重新获取可用的ZRV设备IP地址, 记录编号=%s, 原有ZRV设备IP=%s, 新的ZRV设备IP=%s", (char*)COMPRESS_TASKIDVector[index].c_str(), pCompressTaskData->strZRVDeviceIP, device_ip);
                            iRet = UpdateCompressTaskAssignInfo((char*)COMPRESS_TASKIDVector[index].c_str(), 1, device_ip, pDbOper);
                        }
                    }
                }
            }

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "压缩任务等待响应超时, 重新下发任务到ZRV设备, 重新下发次数=%d, 记录编号=%s, 平台IP地址=%s, ZRV设备IP地址=%s", pCompressTaskData->resend_count, pCompressTaskData->stYSPB.jlbh, pCompressTaskData->strPlatformIP, pCompressTaskData->strZRVDeviceIP);

            iRet = SendNotCompleteTaskToZRVDeviceForTimeOut((char*)COMPRESS_TASKIDVector[index].c_str(), run_thread_time);
        }
    }

    COMPRESS_TASKIDVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : update_compress_task_list_for_wait_expire
 功能描述  : 更新压缩任务超时
 输入参数  : DBOper* pDbOper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年8月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "压缩任务等待响应超时, 主动关闭:记录编号=%s, 平台IP地址=%s, ZRV设备IP地址=%s", pCompressTaskData->stYSPB.jlbh, pCompressTaskData->strPlatformIP, pCompressTaskData->strZRVDeviceIP);

            /* 更新状态 */
            iRet = UpdateCompressTaskResultInfo(pCompressTaskData->stYSPB.jlbh, 3, 2, FILE_COMPRESS_TIMEOUT_ERROR, 0, 0, 0, NULL, pDbOper);
        }
    }

    COMPRESS_TASKIDVector.clear();

    return 0;
}
#endif

/*****************************************************************************
 函 数 名  : GetAllProcCompressTaskByPlatformIP
 功能描述  : 根据平台IP地址获取正在处理的任务信息
 输入参数  : char* platform_ip
             vector<string>& CompressTaskID
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年9月1日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : GetNeedAssignCompressTaskByPlatformIP
 功能描述  : 根据平台IP地址获取需要分配的任务信息
 输入参数  : char* platform_ip
             vector<string>& CompressTaskID
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年9月1日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : GetAllAssignedCompressTaskByZRVDeviceIP
 功能描述  : 根据ZRV设备的IP地址获取已经分配的任务数
 输入参数  : char* device_ip
             vector<string>& CompressTaskID
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年9月1日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : GetCurrentCompressTaskCountByZRVDeviceIP
 功能描述  : 根据ZRV设备的IP地址获取当前压缩任务总数
 输入参数  : char* platform_ip
             char* zrv_device_ip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年9月1日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : HasCompressTaskNotComplete
 功能描述  : 是否有没有处理完的压缩任务
 输入参数  : char* platform_ip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年8月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : HasCompressTaskNotComplete2
 功能描述  : 是否有没有处理完的等待ZRV结果的压缩任务
 输入参数  : char* platform_ip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年8月22日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

        if (1 == pCompressTaskData->iTaskStatus) /* 有正在等待压缩结果的任务 */
        {
            COMPRESS_TASK_SMUTEX_UNLOCK();
            return 1;
        }
    }

    COMPRESS_TASK_SMUTEX_UNLOCK();

    return 0;
}

/*****************************************************************************
 函 数 名  : DeleteCompressTaskByPlatformIP
 功能描述  : 根据平台IP地址删除压缩任务
 输入参数  : char* platform_ip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年6月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : DeleteCompressTaskFromDBByPlatformIP
 功能描述  : 根据平台IP地址从数据库删除压缩任务
 输入参数  : char* platform_ip
             DBOper* pDbOper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年6月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : DeleteCompressTask
 功能描述  : 删除压缩任务
 输入参数  : char* platform_ip
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年6月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : AddCompressTaskToAssignDB
 功能描述  : 添加压缩任务到数据库
 输入参数  : compress_task_t* compress_task
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年6月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 1、插入待分配表:先查询任务是否存在 */
    strQuerySQL.clear();
    strQuerySQL = "select * from ZRVCompressTaskAssignInfo WHERE RecordNum like '";
    strQuerySQL += compress_task->stYSPB.jlbh;
    strQuerySQL += "'";

    record_count = ptDBoper->DB_Select(strQuerySQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToAssignDB() DB Oper Error:strQuerySQL=%s, record_count=%d \r\n", strQuerySQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToAssignDB() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "增加任务信息到数据库任务表ZRVCompressTaskAssignInfo中失败, 查询数据库操作失败:记录编号=%s", compress_task->stYSPB.jlbh);
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
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "增加任务信息到数据库任务表ZRVCompressTaskAssignInfo中失败, 插入数据库操作失败:记录编号=%s", compress_task->stYSPB.jlbh);
        }
        else if (iRet == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "增加任务信息到数据库任务表ZRVCompressTaskAssignInfo中失败, 未找到对应的数据库记录:记录编号=%s", compress_task->stYSPB.jlbh);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "增加任务信息到数据库任务表ZRVCompressTaskAssignInfo中成功:记录编号=%s", compress_task->stYSPB.jlbh);
        }
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加的压缩任务信息已经存在于ZRVCompressTaskAssignInfo数据库, 记录编号=%s", compress_task->stYSPB.jlbh);
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "AddCompressTaskToAssignDB() compress_task->stYSPB.jlbh=%s has exist in ZRVCompressTaskAssignInfo db\r\n", compress_task->stYSPB.jlbh);

        /* 重新压缩 */
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
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "更新任务信息到数据库任务表ZRVCompressTaskAssignInfo中失败, 更新数据库操作失败:记录编号=%s", compress_task->stYSPB.jlbh);
            }
            else if (iRet == 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "更新任务信息到数据库任务表ZRVCompressTaskAssignInfo中失败, 未找到对应的数据库记录或者没有可更新的字段:记录编号=%s", compress_task->stYSPB.jlbh);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "更新任务信息到数据库任务表ZRVCompressTaskAssignInfo中成功:记录编号=%s", compress_task->stYSPB.jlbh);
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
        /* 可能是之前已经存在于数据库的记录，更新一下 */
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
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "更新任务信息到数据库任务表ZRVCompressTaskInfo中失败, 更新数据库操作失败:记录编号=%s", task_id);
        }
        else if (iRet == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "更新任务信息到数据库任务表ZRVCompressTaskInfo中失败, 未找到对应的数据库记录或者没有可更新的字段:记录编号=%s", task_id);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "更新任务信息到数据库任务表ZRVCompressTaskInfo中成功:记录编号=%s", task_id);
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

        /* 1、插入总表:先查询任务是否存在 */
        strQuerySQL.clear();
        strQuerySQL = "select * from ZRVCompressTaskInfo WHERE RecordNum like '";
        strQuerySQL += compress_task->stYSPB.jlbh;
        strQuerySQL += "'";

        record_count = ptDBoper->DB_Select(strQuerySQL.c_str(), 1);

        if (record_count < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToDB() DB Oper Error:strQuerySQL=%s, record_count=%d \r\n", strQuerySQL.c_str(), record_count);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTaskToDB() ErrorMsg=%s\r\n", ptDBoper->GetLastDbErrorMsg());
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "增加任务信息到数据库任务表ZRVCompressTaskInfo中失败, 查询数据库操作失败:记录编号=%s", task_id);
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
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "增加任务信息到数据库任务表ZRVCompressTaskInfo中失败, 插入数据库操作失败:记录编号=%s", task_id);
            }
            else if (iRet == 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "增加任务信息到数据库任务表ZRVCompressTaskInfo中失败, 未找到对应的数据库记录:记录编号=%s", task_id);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "增加任务信息到数据库任务表ZRVCompressTaskInfo中成功:记录编号=%s", task_id);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加的压缩任务信息已经存在于ZRVCompressTaskInfo数据库表中, 记录编号=%s", compress_task->stYSPB.jlbh);
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
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "更新任务信息到数据库任务表ZRVCompressTaskInfo中失败, 更新数据库操作失败:记录编号=%s", task_id);
            }
            else if (iRet == 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "更新任务信息到数据库任务表ZRVCompressTaskInfo中失败, 未找到对应的数据库记录或者没有可更新的字段:记录编号=%s", task_id);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "更新任务信息到数据库任务表ZRVCompressTaskInfo中成功:记录编号=%s", task_id);
            }
        }
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : AddCompressTask
 功能描述  : 添加压缩任务
 输入参数  : char* platform_ip
             jly_yspb_t* pstYSPB
             DBOper* ptDBoper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年6月11日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 先查找 */
    compress_task = compress_task_find(pstYSPB->jlbh);

    if (NULL == compress_task)
    {
        iRet = compress_task_init(&compress_task);

        if (iRet != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTask() compress_task_init Error");
            return -1;
        }

        /* 生成GUID */
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

        // TODO:  需要确定压缩后的存储路径
        //compress_task->stYSPB.yshlj;

        if (compress_task_add(compress_task) < 0)
        {
            compress_task_free(compress_task);
            osip_free(compress_task);
            compress_task = NULL;
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "AddCompressTask() compress_task_add Error");
            return -1;
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "添加压缩任务信息成功, 任务记录编号=%s", compress_task->stYSPB.jlbh);
    }
    else
    {
        if (0 == compress_task->iTaskStatus) /* 初始状态 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加的压缩任务信息已经存在,但是还没有开始进行压缩任务, 重新开始压缩, 任务记录编号=%s", compress_task->strZRVDeviceIP, compress_task->stYSPB.jlbh);
        }
        else if (1 == compress_task->iTaskStatus) /* 正在进行压缩 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加的压缩任务信息已经存在,并且正在正在等待ZRV=%s设备返回任务结果, 重新开始压缩, 任务记录编号=%s", compress_task->strZRVDeviceIP, compress_task->stYSPB.jlbh);
        }
        else /* 压缩完成了 */
        {
            if (1 == compress_task->iTaskResult) /* 压缩成功 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加的压缩任务信息已经存在,已经压缩成功, ZRV=%s, 重新开始压缩, 任务记录编号=%s", compress_task->strZRVDeviceIP, compress_task->stYSPB.jlbh);
            }
            else if (2 == compress_task->iTaskResult) /* 失败的重新压缩 */
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "添加的压缩任务信息已经存在, 之前压缩失败, ZRV=%s, 重新开始压缩, 任务记录编号=%s", compress_task->strZRVDeviceIP, compress_task->stYSPB.jlbh);
            }
        }

        /* 重新压缩 */
        compress_task->iAssignFlag = 0;
        memset(compress_task->strZRVDeviceIP, 0, MAX_IP_LEN);
        compress_task->iTaskStatus = 0;
        compress_task->iTaskResult = 0;
        compress_task->wait_answer_expire = 0;
        compress_task->resend_count = 0;
        compress_task->iTaskCreateTime = now;
    }

    /* 增加到数据库临时表 */
    iRet = AddCompressTaskToAssignDB(compress_task, ptDBoper);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "AddCompressTask() AddCompressTaskToAssignDB: iRet=%d\r\n", iRet);

    return 0;
}

/*****************************************************************************
 函 数 名  : UpdateCompressTaskAssignInfo
 功能描述  : 更新压缩分配信息
 输入参数  : char* task_id
             int iAssignFlag
             char* zrv_device_ip
             DBOper* pDbOper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年8月23日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
 函 数 名  : UpdateCompressTaskAssignInfoToDB
 功能描述  : 更新压缩分配标识到数据库
 输入参数  : char* task_id
             int iAssignFlag
             char* zrv_device_ip
             DBOper* pDbOper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年8月3日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "更新压缩任务ZRV分配信息到数据库表ZRVCompressTaskAssignInfo失败, 更新数据库操作失败:记录编号=%s, AssignFlag=%d, zrv_device_ip=%s", task_id, iAssignFlag, zrv_device_ip);
    }
    else if (iRet == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "更新压缩任务ZRV分配信息到数据库表ZRVCompressTaskAssignInfo失败, 未找到对应的数据库记录或者没有可更新的字段:记录编号=%s, AssignFlag=%d, zrv_device_ip=%s", task_id, iAssignFlag, zrv_device_ip);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "更新压缩任务ZRV分配信息到数据库表ZRVCompressTaskAssignInfo成功:记录编号=%s, AssignFlag=%d, zrv_device_ip=%s", task_id, iAssignFlag, zrv_device_ip);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : UpdateCompressTaskResultInfo
 功能描述  : 更新压缩结果信息
 输入参数  : char* task_id
             int iTaskStatus
             int iTaskResult
             int iErrorCode
             int iCompressBeginTime
             int iCompressEndTime
             int iYSHFileSize
             char* pcDestUrl
             DBOper* pDbOper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年8月23日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    if (2 == iTaskStatus || 3 == iTaskStatus) /* 压缩完成或者超时，删除掉任务信息 */
    {
        iRet = compress_task_remove(task_id);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "UpdateCompressTaskResultInfoToDB() compress_task_remove: RecordNum=%s, iRet=%d\r\n", task_id, iRet);

        if (iRet < 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "从内存中删除数据失败, 没有找到记录:记录编号=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
        }
        else if (iRet == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "从内存中删除数据成功:记录编号=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
        }
    }

    /* 调用接口更新远端 */
    if (1 == iTaskResult)
    {
        iRet = interface_updateObjectInfo(task_id, iYSHFileSize, pcDestUrl, strResultXML);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "compress_task_set_task_result_info: interface_updateObjectInfo :iRet=%d, Response XML msg=\r\n%s \r\n", iRet, (char*)strResultXML.c_str());

        if (0 != iRet)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "更新压缩任务结果信息失败, 调用WebService接口更新失败:记录编号=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : UpdateCompressTaskResultInfoToDB
 功能描述  : 更新压缩结果到数据库
 输入参数  : char* task_id
             int iTaskStatus
             int iTaskResult
             int iErrorCode
             int iCompressBeginTime
             int iCompressEndTime
             int iYSHFileSize
             char* pcDestUrl
             DBOper* pDbOper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年8月3日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "更新压缩任务结果信息信息到数据库表ZRVCompressTaskAssignInfo失败, 更新数据库操作失败:记录编号=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
    }
    else if (iRet == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "更新压缩任务结果信息信息到数据库表ZRVCompressTaskAssignInfo失败, 未找到对应的数据库记录或者没有可更新的字段:记录编号=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "更新压缩任务结果信息信息到数据库表ZRVCompressTaskAssignInfo成功:记录编号=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
    }

    if (2 == iTaskStatus || 3 == iTaskStatus) /* 压缩完成或者超时，删除掉分配表里面的数据 */
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
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "从数据库分配任务表ZRVCompressTaskAssignInfo中删除数据失败, 删除数据库操作失败:记录编号=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
        }
        else if (iRet == 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "从数据库分配任务表ZRVCompressTaskAssignInfo中删除数据失败, 未找到对应的数据库记录:记录编号=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "从数据库分配任务表ZRVCompressTaskAssignInfo中删除数据成功:记录编号=%s, TaskStatus=%d, TaskResult=%d, iYSHFileSize=%d, DestUrl=%s", task_id, iTaskStatus, iTaskResult, iYSHFileSize, pcDestUrl);
        }

        /* 增加到任务总表 */
        iRet = AddCompressTaskToDB(task_id, iTaskStatus, iTaskResult, iErrorCode, iCompressBeginTime, iCompressEndTime, iYSHFileSize, pcDestUrl, pDbOper);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "UpdateCompressTaskResultInfoToDB() :AddCompressTaskToDB:RecordNum=%s, iRet=%d \r\n", task_id, iRet);
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : get_complete_compress_task_from_assign_table
 功能描述  : 从分配表中获取已经完成的任务
 输入参数  : vector<string>& RecordNumVector
             vector<int>& ErrorCodeVector
             vector<int>& CompressBeginTimeVector
             vector<int>& CompressEndTimeVector
             DBOper* pDbOper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年8月23日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

        /* 记录编号 */
        strRecordNum.clear();
        pDbOper->GetFieldValue("RecordNum", strRecordNum);
        osip_strncpy(compress_task->stYSPB.jlbh, (char*)strRecordNum.c_str(), MAX_TSU_TASK_LEN);

        /* 文件名称 */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("FileName", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.wjmc, (char*)tmp_svalue.c_str(), 128);

        /* 文件后缀 */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("FileSuffixName", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.kzm, (char*)tmp_svalue.c_str(), 32);

        /* 文件大小 */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("FileSize", tmp_ivalue);
        compress_task->stYSPB.wjdx = tmp_ivalue;

        /* 上传单位 */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("UploadUnit", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.scdw, (char*)tmp_svalue.c_str(), 128);

        /* 上传时间 */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("UploadTime", tmp_ivalue);
        compress_task->stYSPB.scsj = tmp_ivalue;

        /* 存储路径 */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("StoragePath", tmp_svalue);
        osip_strncpy(compress_task->stYSPB.cclj, (char*)tmp_svalue.c_str(), 128);

        /* 分配标识 */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("AssignFlag", tmp_ivalue);
        compress_task->iAssignFlag = tmp_ivalue;

        /* 平台IP地址 */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("PlatformIP", tmp_svalue);
        osip_strncpy(compress_task->strPlatformIP, (char*)tmp_svalue.c_str(), MAX_IP_LEN);

        /* ZRV IP地址 */
        tmp_svalue.clear();
        pDbOper->GetFieldValue("ZRVDeviceIP", tmp_svalue);
        osip_strncpy(compress_task->strZRVDeviceIP, (char*)tmp_svalue.c_str(), MAX_IP_LEN);

        /* 任务状态 */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("TaskStatus", tmp_ivalue);
        compress_task->iTaskStatus = tmp_ivalue;

        /* 任务结果 */
        tmp_ivalue = 0;
        pDbOper->GetFieldValue("TaskResult", tmp_ivalue);
        compress_task->iTaskResult = tmp_ivalue;

        /* 错误码 */
        iErrorCode = 0;
        pDbOper->GetFieldValue("ErrorCode", iErrorCode);

        /* 压缩创建时间 */
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

        /* 压缩开始时间 */
        iZRVCompressEndTime = 0;
        pDbOper->GetFieldValue("ZRVCompressBeginTime", iZRVCompressBeginTime);

        /* 压缩结束时间 */
        iZRVCompressEndTime = 0;
        pDbOper->GetFieldValue("ZRVCompressEndTime", iZRVCompressEndTime);

        compress_task->wait_answer_expire = 0;
        compress_task->resend_count = 0;

        /* 添加到队列 */
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
 函 数 名  : DeleteCompressTaskFromDBForStart
 功能描述  : 程序启动的时候，删除掉数据库中超时或者完成的任务
 输入参数  : DBOper* pDbOper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年8月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

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

    /* 读取到内存中 */
    RecordNumVector.clear();
    ErrorCodeVector.clear();
    CompressBeginTimeVector.clear();
    CompressEndTimeVector.clear();

    get_complete_compress_task_from_assign_table(RecordNumVector, ErrorCodeVector, CompressBeginTimeVector, CompressEndTimeVector, pDbOper);

    /* 更新到数据库 */
    if (RecordNumVector.size() > 0)
    {
        for (index = 0; index < (int)RecordNumVector.size(); index++)
        {
            /* 增加到数据库 */
            iRet = AddCompressTaskToDB((char*)RecordNumVector[index].c_str(), 0, 0, ErrorCodeVector[index], CompressBeginTimeVector[index], CompressEndTimeVector[index], 0, NULL, pDbOper);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "DeleteCompressTaskFromDBForStart() AddCompressTaskToDB: RecordNum=%s, iRet=%d\r\n", (char*)RecordNumVector[index].c_str(), iRet);

            /* 从内存中删除掉 */
            iRet = compress_task_remove((char*)RecordNumVector[index].c_str());
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "DeleteCompressTaskFromDBForStart() compress_task_remove: RecordNum=%s, iRet=%d\r\n", (char*)RecordNumVector[index].c_str(), iRet);
        }
    }

    RecordNumVector.clear();
    ErrorCodeVector.clear();
    CompressBeginTimeVector.clear();
    CompressEndTimeVector.clear();

    /* 删除掉分配库中的数据 */
    strDeleteSQL.clear();
    strDeleteSQL = "DELETE FROM ZRVCompressTaskAssignInfo WHERE TaskStatus=2 OR TaskStatus=3";

    iRet = pDbOper->DB_Delete(strDeleteSQL.c_str(), 1);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "DeleteCompressTaskFromDBForStart() :DB_Delete:iRet=%d \r\n", iRet);

    return iRet;
}

/*****************************************************************************
 函 数 名  : ShowCallTask
 功能描述  : 显示在线任务
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月29日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

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
