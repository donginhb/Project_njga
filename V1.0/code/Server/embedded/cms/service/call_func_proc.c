
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

#include "service/call_func_proc.inc"

#include "resource/resource_info_mgn.inc"

#include "user/user_info_mgn.inc"
#include "user/user_srv_proc.inc"

#include "device/device_info_mgn.inc"

#include "route/route_info_mgn.inc"
#include "record/record_srv_proc.inc"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern GBLogicDevice_Info_MAP g_GBLogicDeviceInfoMap; /* 标准逻辑设备信息队列 */
extern int g_IsPay;                                   /* 是否付费，默认1 */
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
unsigned long long iCrDataLockCount = 0;
unsigned long long iCrDataUnLockCount = 0;

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
CR_Data_MAP g_CallRecordMap; /* 呼叫链接数据队列 */
#ifdef MULTI_THR
osip_mutex_t* g_CallRecordMapLock = NULL;
#endif

ack_send_list_t* g_AckSendList = NULL;
transfer_xml_msg_list_t* g_TransferXMLMsgList = NULL; /* 服务器转发的 XML Message 消息队列*/

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/
#define MAX_CR_CONNECTS 5000  /* 最大呼叫路数 */

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#if DECS("呼叫链接数据队列")
/*****************************************************************************
 函 数 名  : call_record_init
 功能描述  : 呼叫链接结构初始化
 输入参数  : cr_t** call_record
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月19日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int call_record_init(cr_t** call_record)
{
    *call_record = (cr_t*)osip_malloc(sizeof(cr_t));

    if (*call_record == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_init() exit---: *call_record Smalloc Error \r\n");
        return -1;
    }

    (*call_record)->iUsed = 0;
    (*call_record)->call_type = CALL_TYPE_NULL;

    (*call_record)->caller_id[0] = '\0';
    (*call_record)->caller_ip[0] = '\0';
    (*call_record)->caller_port = 0;
    (*call_record)->caller_sdp_ip[0] = '\0';
    (*call_record)->caller_sdp_port = 0;
    (*call_record)->caller_ua_index = -1;
    (*call_record)->caller_server_ip_ethname[0] = '\0';
    (*call_record)->caller_server_ip[0] = '\0';
    (*call_record)->caller_server_port = 0;
    (*call_record)->caller_transfer_type = TRANSFER_PROTOCOL_NULL;

    (*call_record)->callee_id[0] = '\0';
    (*call_record)->callee_ip[0] = '\0';
    (*call_record)->callee_port = 0;
    (*call_record)->callee_sdp_ip[0] = '\0';
    (*call_record)->callee_sdp_port = 0;
    (*call_record)->callee_ua_index = -1;
    (*call_record)->callee_server_ip_ethname[0] = '\0';
    (*call_record)->callee_server_ip[0] = '\0';
    (*call_record)->callee_server_port = 0;
    (*call_record)->callee_transfer_type = TRANSFER_PROTOCOL_NULL;

    (*call_record)->callee_framerate = 25;
    (*call_record)->callee_service_type = 0;
    (*call_record)->callee_record_type = EV9000_RECORD_TYPE_NORMAL;
    (*call_record)->callee_stream_type = EV9000_STREAM_TYPE_MASTER;
    (*call_record)->callee_onvif_url[0] = '\0';
    (*call_record)->callee_gb_device_type = 0;

    (*call_record)->iScale = 1.0;
    (*call_record)->iPlaybackTimeGap = 0;
    (*call_record)->tsu_device_id[0] = '\0';
    (*call_record)->tsu_resource_index = -1;
    (*call_record)->tsu_ip[0] = '\0';

    (*call_record)->tsu_code = 0;
    (*call_record)->tsu_recv_port = 0;
    (*call_record)->tsu_send_port = 0;
    (*call_record)->task_id[0] = '\0';

    (*call_record)->tsu_session_expire = 0;
    (*call_record)->wait_answer_expire = 0;
    (*call_record)->call_status = CALL_STATUS_NULL;

    /* TSU IP地址队列初始化 */
    memset(&(*call_record)->TSUVideoIP, 0, sizeof(ip_pair_t));
    memset(&(*call_record)->TSUDeviceIP, 0, sizeof(ip_pair_t));

    return 0;
}

/*****************************************************************************
 函 数 名  : call_record_free
 功能描述  : 呼叫链接结构释放
 输入参数  : cr_t * call_record
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void call_record_free(cr_t* call_record)
{
    if (call_record == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_free() exit---: Param Error \r\n");
        return;
    }

    call_record->iUsed = 0;
    call_record->call_type = CALL_TYPE_NULL;

    memset(call_record->caller_id, 0, MAX_ID_LEN + 4);
    memset(call_record->caller_ip, 0, MAX_IP_LEN);
    call_record->caller_port = 0;
    memset(call_record->caller_sdp_ip, 0, MAX_IP_LEN);
    call_record->caller_sdp_port = 0;
    call_record->caller_ua_index = -1;
    memset(call_record->caller_server_ip_ethname, 0, MAX_IP_LEN);
    call_record->caller_server_ip[0] = '\0';
    call_record->caller_server_port = 0;
    call_record->caller_transfer_type = TRANSFER_PROTOCOL_NULL;

    memset(call_record->callee_id, 0, MAX_ID_LEN + 4);
    memset(call_record->callee_ip, 0, MAX_IP_LEN);
    call_record->callee_port = 0;
    memset(call_record->callee_sdp_ip, 0, MAX_IP_LEN);
    call_record->callee_sdp_port = 0;
    call_record->callee_ua_index = -1;
    memset(call_record->callee_server_ip_ethname, 0, MAX_IP_LEN);
    call_record->callee_server_ip[0] = '\0';
    call_record->callee_server_port = 0;
    call_record->callee_transfer_type = TRANSFER_PROTOCOL_NULL;

    call_record->callee_framerate = 25;
    call_record->callee_service_type = 0;
    call_record->callee_record_type = EV9000_RECORD_TYPE_NORMAL;
    call_record->callee_stream_type = EV9000_STREAM_TYPE_MASTER;
    memset(call_record->callee_onvif_url, 0, 256);
    call_record->callee_gb_device_type = 0;

    call_record->iScale = 1.0;
    call_record->iPlaybackTimeGap = 0;
    memset(call_record->tsu_device_id, 0, MAX_ID_LEN + 4);
    call_record->tsu_resource_index = -1;
    memset(call_record->tsu_ip, 0, MAX_IP_LEN);

    call_record->tsu_code = 0;
    call_record->tsu_recv_port = 0;
    call_record->tsu_send_port = 0;
    memset(call_record->task_id, 0, MAX_TSU_TASK_LEN + 4);
    call_record->tsu_session_expire = 0;
    call_record->wait_answer_expire = 0;
    call_record->call_status = CALL_STATUS_NULL;

    memset(&call_record->TSUVideoIP, 0, sizeof(ip_pair_t));
    memset(&call_record->TSUDeviceIP, 0, sizeof(ip_pair_t));

    return;
}

/*****************************************************************************
 函 数 名  : call_record_list_init
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
int call_record_list_init()
{
    int i = 0;
    int pos = 0;

    g_CallRecordMap.clear();

    for (pos = 0; pos < MAX_CR_CONNECTS; pos++)
    {
        cr_t* pCrData = NULL;

        i = call_record_init(&pCrData);

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_list_init() exit---: Call Record Init Error \r\n");
            return -1;
        }

        g_CallRecordMap[pos] = pCrData;
    }

#ifdef MULTI_THR
    g_CallRecordMapLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_CallRecordMapLock)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_list_init() exit---: Call Record Map Lock Init Error \r\n");
        return -1;
    }

#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : call_record_list_free
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
void call_record_list_free()
{
    CR_Data_Iterator Itr;
    cr_t* pCrData = NULL;

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if (NULL != pCrData)
        {
            call_record_free(pCrData);
            osip_free(pCrData);
            pCrData = NULL;
        }
    }

    g_CallRecordMap.clear();

#ifdef MULTI_THR

    if (NULL != g_CallRecordMapLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_CallRecordMapLock);
        g_CallRecordMapLock = NULL;
    }

#endif
    return;
}

/*****************************************************************************
 函 数 名  : call_record_list_lock
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
int call_record_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CallRecordMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_CallRecordMapLock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : call_record_list_unlock
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
int call_record_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CallRecordMapLock == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_CallRecordMapLock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_call_record_list_lock
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
int debug_call_record_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CallRecordMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "debug_call_record_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_CallRecordMapLock, file, line, func);

    iCrDataLockCount++;

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_call_record_list_lock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iCrDataLockCount != iCrDataUnLockCount + 1)
        {
            //printf("\r\n**********%s:%d:%s:debug_call_record_list_lock:iRet=%d, iCrDataLockCount=%lld**********\r\n", file, line, func, iRet, iCrDataLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_call_record_list_lock:iRet=%d, iCrDataLockCount=%lld", file, line, func, iRet, iCrDataLockCount);
        }
    }

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_call_record_list_unlock
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
int debug_call_record_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_CallRecordMapLock == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "debug_call_record_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iCrDataUnLockCount++;

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_CallRecordMapLock, file, line, func);

    if (0 != iRet)
    {
        printf("\r\n**********%s:%d:%s:debug_call_record_list_unlock:iRet=%d**********\r\n", file, line, func, iRet);
        /* fprintf(stdout, "\r\n%s:%d:%s:osip_debug_mutex_lock:iRet=%d\r\n", file, line, func, iRet); */
        /* fflush(stdout); */
    }
    else
    {
        if (iCrDataLockCount != iCrDataUnLockCount)
        {
            //printf("\r\n**********%s:%d:%s:debug_call_record_list_unlock:iRet=%d, iCrDataUnLockCount=%lld**********\r\n", file, line, func, iRet, iCrDataUnLockCount);
        }
        else
        {
            //printf("\r\n%s:%d:%s:debug_call_record_list_unlock:iRet=%d, iCrDataUnLockCount=%lld", file, line, func, iRet, iCrDataUnLockCount);
        }
    }

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : call_record_add
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
int call_record_add()
{
    CR_Data_Iterator Itr;
    cr_t* pCrData = NULL;

    CR_SMUTEX_LOCK();

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if (0 == pCrData->iUsed)
        {
            /* 找到空闲的位置 */
            pCrData->iUsed = 1;
            pCrData->call_status = CALL_STATUS_NULL;

            memset(&pCrData->TSUVideoIP, 0, sizeof(ip_pair_t));
            memset(&pCrData->TSUDeviceIP, 0, sizeof(ip_pair_t));

            CR_SMUTEX_UNLOCK();
            //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "call_record_add() exit---: pos=%d \r\n", Itr->first);
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : call_record_remove
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
int call_record_remove(int pos)
{
    cr_t* pCrData = NULL;

    CR_SMUTEX_LOCK();

    if (pos < 0 || (pos >= (int)g_CallRecordMap.size()))
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_remove() exit---: Param Error \r\n");
        return -1;
    }

    pCrData = g_CallRecordMap[pos];

    if (0 == pCrData->iUsed)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_remove() exit---: CR Data UnUsed:pos=%d \r\n", pos);
        return -1;
    }

    if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "call_record_remove() exit---: call_status=%d, pos=%d \r\n", pCrData->call_status, pos);
        return -1;
    }

    //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_remove() pos=%d \r\n", pos);
    call_record_free(pCrData);
    CR_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 函 数 名  : call_record_set_call_status
 功能描述  : 设置呼叫资源的释放状态
 输入参数  : int pos
             call_status_t call_status
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年1月5日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int call_record_set_call_status(int pos, call_status_t call_status)
{
    cr_t* pCrData = NULL;

    CR_SMUTEX_LOCK();

    if (pos < 0 || (pos >= (int)g_CallRecordMap.size()))
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_set_call_status() exit---: Param Error \r\n");
        return -1;
    }

    pCrData = g_CallRecordMap[pos];

    pCrData->call_status = call_status;

    CR_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 函 数 名  : call_record_find_by_callerid_and_calleeid
 功能描述  : 根据caller id和callee id 查找呼叫链接
 输入参数  : char* caller_id
                            char* callee_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月31日 星期六
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int call_record_find_by_callerid_and_calleeid(char* caller_id, char* callee_id)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (NULL == caller_id || NULL == callee_id)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_callerid_and_calleeid() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_callerid_and_calleeid() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_AUDIO == pCrData->call_type)
            || ('\0' == pCrData->caller_id[0])
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if ((0 == sstrcmp(pCrData->caller_id, caller_id))
            && (0 == sstrcmp(pCrData->callee_id, callee_id)))
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : call_record_find_by_callerinfo
 功能描述  : 根据主叫侧的SDP信息查找呼叫任务
 输入参数  : char* caller_id
             char* caller_sdp_ip
             int caller_sdp_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月11日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int call_record_find_by_callerinfo(char* caller_id, char* caller_sdp_ip, int caller_sdp_port)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (NULL == caller_id || NULL == caller_sdp_ip || caller_sdp_port <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_callerinfo() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_callerid_and_calleeid() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_AUDIO == pCrData->call_type)
            || ('\0' == pCrData->caller_id[0])
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if ((0 == sstrcmp(pCrData->caller_id, caller_id))
            && (0 == sstrcmp(pCrData->caller_sdp_ip, caller_sdp_ip))
            && pCrData->caller_sdp_port == caller_sdp_port)
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : call_record_find_by_callerinfo_and_calleeid
 功能描述  : 根据主叫信息和被叫的ID，流类型查找任务
 输入参数  : char* caller_id
             char* caller_ip
             int caller_port
             char* callee_id
             int stream_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月8日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int call_record_find_by_callerinfo_and_calleeid(char* caller_id, char* caller_ip, int caller_port, char* callee_id, int stream_type)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (NULL == caller_id || NULL == caller_ip || caller_port <= 0 || NULL == callee_id)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_callerinfo() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_callerid_and_calleeid() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_AUDIO == pCrData->call_type)
            || ('\0' == pCrData->caller_id[0])
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if ((0 == sstrcmp(pCrData->caller_id, caller_id))
            && (0 == sstrcmp(pCrData->caller_ip, caller_ip))
            && pCrData->caller_port == caller_port
            && (0 == sstrcmp(pCrData->callee_id, callee_id))
            && pCrData->callee_stream_type == stream_type)
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : call_record_find_by_caller_id
 功能描述  : 根据caller id查找呼叫链接
 输入参数  : char* caller_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月26日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int call_record_find_by_caller_id(char* caller_id)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (NULL == caller_id)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_caller_id() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_caller_id() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_AUDIO == pCrData->call_type)
            || ('\0' == pCrData->caller_id[0])
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if (0 == sstrcmp(pCrData->caller_id, caller_id))
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : call_record_find_by_caller_index
 功能描述  : 查找呼叫链接
 输入参数  : int caller_ua_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月16日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int call_record_find_by_caller_index(int caller_ua_index)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (caller_ua_index < 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_caller_index() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_caller_index() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (pCrData->caller_ua_index < 0))
        {
            continue;
        }

        if (pCrData->caller_ua_index == caller_ua_index)
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : call_record_find_by_callee_index
 功能描述  : 查找呼叫链接
 输入参数  : int caller_ua_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月16日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int call_record_find_by_callee_index(int callee_ua_index)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (callee_ua_index < 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_callee_index() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_callee_index() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (pCrData->callee_ua_index < 0))
        {
            continue;
        }

        if (pCrData->callee_ua_index == callee_ua_index)
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : call_record_find_by_task_id
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
int call_record_find_by_task_id(char* task_id)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (NULL == task_id)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_task_id() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_tsu_and_transfer() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || ('\0' == pCrData->task_id[0]))
        {
            continue;
        }

        if (0 == sstrcmp(pCrData->task_id, task_id))
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : audio_call_record_find_by_send_info
 功能描述  : 根据主叫侧的发送IP和端口号查找音频任务信息
 输入参数  : char* caller_ip
             int caller_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月9日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int audio_call_record_find_by_send_info(char* caller_ip, int caller_port)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (NULL == caller_ip || caller_port <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "audio_call_record_find_by_send_info() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_tsu_and_transfer() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_AUDIO != pCrData->call_type))
        {
            continue;
        }

        if (0 == sstrcmp(pCrData->caller_sdp_ip, caller_ip)
            && pCrData->caller_sdp_port == caller_port)
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : dc_call_record_find_by_caller_info
 功能描述  : 根据DC侧的信息查找DC业务
 输入参数  : char* caller_id
             char* caller_ip
             int caller_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月15日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int dc_call_record_find_by_caller_info(char* caller_id, char* caller_ip, int caller_port)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (NULL == caller_id)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "dc_call_record_find_by_caller_info() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_caller_id() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_DC != pCrData->call_type)
            || ('\0' == pCrData->caller_id[0]))
        {
            continue;
        }

        if (0 == sstrcmp(pCrData->caller_id, caller_id)
            && 0 == sstrcmp(pCrData->caller_ip, caller_ip)
            && pCrData->caller_port == caller_port)
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : record_call_record_find_by_calleeid_and_streamtype
 功能描述  : 根据设备ID和留类型查询录像任务
 输入参数  : char* callee_id
             int stream_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月4日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int record_call_record_find_by_calleeid_and_streamtype(char* callee_id, int stream_type)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (NULL == callee_id || stream_type < 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "record_call_record_find_by_calleeid_and_streamtype() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_callerid_and_calleeid() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_AUDIO == pCrData->call_type)
            || ('\0' == pCrData->caller_id[0])
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if (CALL_TYPE_RECORD != pCrData->call_type)
        {
            continue;
        }

        if (stream_type == pCrData->callee_stream_type
            && (0 == sstrcmp(pCrData->callee_id, callee_id)))
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : is_GBLogic_device_has_other_service
 功能描述  : 根据源端的ID关闭主叫侧业务
 输入参数  : char* callee_id
             int cr_pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月13日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int send_bye_to_all_other_caller_by_callee_id(char* callee_id, int cr_pos)
{
    int iRet = 0;
    int index = -1;
    int other_cr_pos = -1;
    cr_t* pOtherCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if (NULL == callee_id || cr_pos < 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "send_bye_to_all_other_caller_by_callee_id() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "is_GBLogic_device_has_other_service() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pOtherCrData = Itr->second;

        if ((NULL == pOtherCrData) || (0 == pOtherCrData->iUsed))
        {
            continue;
        }

        if (CALL_STATUS_NULL != pOtherCrData->call_status)
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pOtherCrData->call_type)
            || (CALL_TYPE_RECORD_PLAY == pOtherCrData->call_type)
            || (CALL_TYPE_DOWNLOAD == pOtherCrData->call_type)
            || (CALL_TYPE_AUDIO == pOtherCrData->call_type)
            || ('\0' == pOtherCrData->callee_id[0]))
        {
            continue;
        }

        if ((0 == sstrcmp(pOtherCrData->callee_id, callee_id))
            && (cr_pos != Itr->first))
        {
            pOtherCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            other_cr_pos = CRIndexVector[index];

            pOtherCrData = call_record_get(other_cr_pos);

            if (NULL == pOtherCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pOtherCrData->call_status)
            {
                continue;
            }

            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Receive front-end BYE message, trigger off closing real-time video processing using the logic device, real- time monitoring OFF:user side ID=%s, user side IP address =%s, user side port number =%d, logic device ID=%s, cr_pos=%d", pOtherCrData->caller_id, pOtherCrData->caller_ip, pOtherCrData->caller_port, pOtherCrData->callee_id, other_cr_pos);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "收到前端BYE消息, 触发关闭其他用到该逻辑设备的实时视频处理, 实时视频关闭:用户侧ID=%s, 用户侧IP地址=%s, 用户侧端口号=%d, 逻辑设备ID=%s, cr_pos=%d", pOtherCrData->caller_id, pOtherCrData->caller_ip, pOtherCrData->caller_port, pOtherCrData->callee_id, other_cr_pos);

            if (CALL_TYPE_RECORD == pOtherCrData->call_type)
            {
                /* 移除录像信息 */
                iRet = RemoveDeviceRecordInfo(pOtherCrData->callee_id, pOtherCrData->callee_stream_type, pOtherCrData->tsu_resource_index);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "send_bye_to_all_other_caller_by_callee_id() RemoveDeviceRecordInfo Error:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pOtherCrData->callee_id, pOtherCrData->callee_stream_type, pOtherCrData->tsu_resource_index);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "send_bye_to_all_other_caller_by_callee_id() RemoveDeviceRecordInfo OK:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pOtherCrData->callee_id, pOtherCrData->callee_stream_type, pOtherCrData->tsu_resource_index);
                }
            }

            /* 停止业务 */
            iRet = StopCallService(pOtherCrData);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "send_bye_to_all_other_caller_by_callee_id() StopCallService Error:cr_pos=%d \r\n", other_cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "send_bye_to_all_other_caller_by_callee_id() StopCallService OK:cr_pos=%d \r\n", other_cr_pos);
            }

            /* 移除业务数据 */
            iRet = call_record_remove(other_cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "send_bye_to_all_other_caller_by_callee_id() call_record_remove Error:cr_pos=%d \r\n", other_cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "send_bye_to_all_other_caller_by_callee_id() call_record_remove OK:cr_pos=%d \r\n", other_cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return iRet;
}

/*****************************************************************************
 函 数 名  : send_bye_to_all_other_caller_by_callee_id_and_streamtype
 功能描述  : 根据源端的ID和流类型关闭主叫侧业务
 输入参数  : char* callee_id
             int stream_type
             int cr_pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月12日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int send_bye_to_all_other_caller_by_callee_id_and_streamtype(char* callee_id, int stream_type, int cr_pos)
{
    int iRet = 0;
    int index = -1;
    int other_cr_pos = -1;
    cr_t* pOtherCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if (NULL == callee_id || cr_pos < 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "send_bye_to_all_other_caller_by_callee_id_and_streamtype() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "send_bye_to_all_other_caller_by_callee_id_and_streamtype() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pOtherCrData = Itr->second;

        if ((NULL == pOtherCrData) || (0 == pOtherCrData->iUsed))
        {
            continue;
        }

        if (CALL_STATUS_NULL != pOtherCrData->call_status)
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pOtherCrData->call_type)
            || (CALL_TYPE_RECORD_PLAY == pOtherCrData->call_type)
            || (CALL_TYPE_DOWNLOAD == pOtherCrData->call_type)
            || (CALL_TYPE_AUDIO == pOtherCrData->call_type)
            || ('\0' == pOtherCrData->callee_id[0]))
        {
            continue;
        }

        if ((0 == sstrcmp(pOtherCrData->callee_id, callee_id))
            && (pOtherCrData->callee_stream_type == stream_type)
            && (cr_pos != Itr->first))
        {
            pOtherCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            other_cr_pos = CRIndexVector[index];

            pOtherCrData = call_record_get(other_cr_pos);

            if (NULL == pOtherCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pOtherCrData->call_status)
            {
                continue;
            }

            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Receive front-end BYE message, trigger off closing real-time video processing using the logic devic, real- time monitoring OFF:user side ID=%s, user side IP address =%s, user side port number=%d, logic device ID=%s, stream type=%d, cr_pos=%d", pOtherCrData->caller_id, pOtherCrData->caller_ip, pOtherCrData->caller_port, pOtherCrData->callee_id, pOtherCrData->callee_stream_type, other_cr_pos);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "收到前端BYE消息, 触发关闭其他用到该逻辑设备的实时视频处理, 实时视频关闭:用户侧ID=%s, 用户侧IP地址=%s, 用户侧端口号=%d, 逻辑设备ID=%s, 流类型=%d, cr_pos=%d", pOtherCrData->caller_id, pOtherCrData->caller_ip, pOtherCrData->caller_port, pOtherCrData->callee_id, pOtherCrData->callee_stream_type, other_cr_pos);

            if (CALL_TYPE_RECORD == pOtherCrData->call_type)
            {
                /* 移除录像信息 */
                iRet = RemoveDeviceRecordInfo(pOtherCrData->callee_id, pOtherCrData->callee_stream_type, pOtherCrData->tsu_resource_index);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "send_bye_to_all_other_caller_by_callee_id() RemoveDeviceRecordInfo Error:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pOtherCrData->callee_id, pOtherCrData->callee_stream_type, pOtherCrData->tsu_resource_index);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "send_bye_to_all_other_caller_by_callee_id() RemoveDeviceRecordInfo OK:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pOtherCrData->callee_id, pOtherCrData->callee_stream_type, pOtherCrData->tsu_resource_index);
                }
            }

            /* 停止业务 */
            iRet = StopCallService(pOtherCrData);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "send_bye_to_all_other_caller_by_callee_id() StopCallService Error:cr_pos=%d \r\n", other_cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "send_bye_to_all_other_caller_by_callee_id() StopCallService OK:cr_pos=%d \r\n", other_cr_pos);
            }

            /* 移除业务数据 */
            iRet = call_record_remove(other_cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "send_bye_to_all_other_caller_by_callee_id() call_record_remove Error:cr_pos=%d \r\n", other_cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "send_bye_to_all_other_caller_by_callee_id() call_record_remove OK:cr_pos=%d \r\n", other_cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return iRet;
}

/*****************************************************************************
 函 数 名  : is_GBLogic_device_has_other_service
 功能描述  : 查找逻辑设备的是否有其他 业务
 输入参数  : char* device_id
             int stream_type
             int cr_pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月13日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int is_GBLogic_device_has_other_service(char* device_id, int stream_type, int cr_pos)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (NULL == device_id || cr_pos < 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "is_GBLogic_device_has_other_service() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "is_GBLogic_device_has_other_service() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_AUDIO == pCrData->call_type)
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
            || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
        {
            continue;
        }

        if ('\0' == pCrData->tsu_ip[0]) /* 没有经过本级TSU转发的,这个时候需要发送给下级关闭 */
        {
            continue;
        }

        if (pCrData->callee_ua_index >= 0) /* 需要找码流发送方没有协议栈句柄的 */
        {
            continue;
        }

        if ((0 == sstrcmp(pCrData->callee_id, device_id)
             && (pCrData->callee_stream_type == stream_type))
            && (cr_pos != Itr->first))
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : find_GBLogic_device_has_video_service
 功能描述  : 查找逻辑设备是否已经有视频业务
 输入参数  : char* device_id
             int stream_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年11月27日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int find_GBLogic_device_has_video_service(char* device_id, int stream_type)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (NULL == device_id || stream_type <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "is_GBLogic_device_has_other_service() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "is_GBLogic_device_has_other_service() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_AUDIO == pCrData->call_type)
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
            || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
        {
            continue;
        }

        if ((0 == sstrcmp(pCrData->callee_id, device_id))
            && (pCrData->callee_ua_index >= 0)
            && (pCrData->callee_stream_type == stream_type))
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : find_GBLogic_device_has_record_service
 功能描述  : 查找逻辑设备是否有录像业务
 输入参数  : char* device_id
             int stream_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年9月2日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int find_GBLogic_device_has_record_service(char* device_id, int stream_type)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (NULL == device_id || stream_type <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "find_GBLogic_device_has_record_service() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "is_GBLogic_device_has_other_service() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_AUDIO == pCrData->call_type)
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if (CALL_TYPE_RECORD != pCrData->call_type)
        {
            continue;
        }

        if ((0 == sstrcmp(pCrData->callee_id, device_id))
            && (pCrData->callee_ua_index >= 0)
            && (pCrData->callee_stream_type == stream_type))
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : find_GBLogic_device_has_realplay_service
 功能描述  : 查找逻辑设备是否有实时视频业务
 输入参数  : char* device_id
             int stream_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年9月2日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int find_GBLogic_device_has_realplay_service(char* device_id, int stream_type)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (NULL == device_id || stream_type <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "find_GBLogic_device_has_realplay_service() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "is_GBLogic_device_has_other_service() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_AUDIO == pCrData->call_type)
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if ((CALL_TYPE_REALTIME != pCrData->call_type) && (CALL_TYPE_DC != pCrData->call_type))
        {
            continue;
        }

        if ((0 == sstrcmp(pCrData->callee_id, device_id))
            && (pCrData->callee_ua_index >= 0)
            && (pCrData->callee_stream_type == stream_type))
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : find_GBLogic_device_has_realplay_service2
 功能描述  : 查找逻辑设备是否有实时视频业务，过滤掉没有经过本级TSU转发的实时
             视频流
 输入参数  : char* device_id
             int stream_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年7月4日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int find_GBLogic_device_has_realplay_service2(char* device_id, int stream_type)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (NULL == device_id || stream_type <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "find_GBLogic_device_has_realplay_service() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "is_GBLogic_device_has_other_service() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_AUDIO == pCrData->call_type)
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if ((CALL_TYPE_REALTIME != pCrData->call_type) && (CALL_TYPE_DC != pCrData->call_type))
        {
            continue;
        }

        /* 可能存在没有经过TSU转发的视频流，过滤掉 */
        if (pCrData->tsu_ip[0] == '\0' || pCrData->tsu_code <= 0 || pCrData->tsu_recv_port <= 0)
        {
            continue;
        }

        if ((0 == sstrcmp(pCrData->callee_id, device_id))
            && (pCrData->callee_ua_index >= 0)
            && (pCrData->callee_stream_type == stream_type))
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : find_recordinfo_has_other_cr_data
 功能描述  : 查找录像信息是否有其他呼叫资源
 输入参数  : char* device_id
             int stream_type
             int record_cr_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年3月2日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int find_recordinfo_has_other_cr_data(char* device_id, int stream_type, int record_cr_index)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (NULL == device_id || stream_type <= 0 || record_cr_index < 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "is_GBLogic_device_has_other_service() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "is_GBLogic_device_has_other_service() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if (CALL_TYPE_RECORD != pCrData->call_type)
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if (Itr->first == record_cr_index)
        {
            continue;
        }

        if ((0 == sstrcmp(pCrData->callee_id, device_id))
            && (pCrData->callee_ua_index >= 0)
            && (pCrData->callee_stream_type == stream_type))
        {
            CR_SMUTEX_UNLOCK();
            return Itr->first;
        }
    }

    CR_SMUTEX_UNLOCK();
    return -1;
}


/*****************************************************************************
 函 数 名  : call_record_get
 功能描述  : 获取呼叫链接数据
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月16日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
cr_t* call_record_get(int pos)
{
    cr_t* pCrData = NULL;

    CR_SMUTEX_LOCK();

    if (pos < 0 || pos >= (int)g_CallRecordMap.size())
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_get() exit---: Param Error \r\n");
        return NULL;
    }

    pCrData = g_CallRecordMap[pos];

    if (0 == pCrData->iUsed)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "call_record_get() exit---: Call Record UnUsed:pos=%d \r\n", pos);
        return NULL;
    }

    CR_SMUTEX_UNLOCK();
    return pCrData;
}

/*****************************************************************************
 函 数 名  : set_call_record_tsu_expire
 功能描述  : 设置呼叫任务的TSU保活时间
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月26日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int set_call_record_tsu_expire(int pos)
{
    cr_t* pCrData = NULL;

    CR_SMUTEX_LOCK();

    if (pos < 0 || pos >= (int)g_CallRecordMap.size())
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "set_call_record_tsu_expire() exit---: Param Error \r\n");
        return -1;
    }

    pCrData = g_CallRecordMap[pos];

    if (0 == pCrData->iUsed)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "set_call_record_tsu_expire() exit---: Call Record UnUsed:pos=%d \r\n", pos);
        return -1;
    }

    pCrData->tsu_session_expire = 0;

    CR_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 函 数 名  : scan_call_record_list
 功能描述  : 扫描任务列表
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月26日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int scan_call_record_list()
{
    int i = 0;
    int iRet = 0;
    int index = -1;
    int cr_pos = -1;
    int other_cr_pos = -1;
    cr_t* pCrData = NULL;
    cr_t* pOtherCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;
    vector<int> AudioCRIndexVector;

    CRIndexVector.clear();
    AudioCRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "scan_call_record_list() exit---: Call Record Srv Map NULL \r\n");
        return 0;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if (CALL_TYPE_NULL == pCrData->call_type)
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if ('\0' == pCrData->tsu_ip[0]) /* 没有经过TSU转发的过滤掉 */
        {
            continue;
        }

        pCrData->tsu_session_expire++;

        if (pCrData->tsu_session_expire >= DEFAULT_TSU_SESSION_EXPIRE)
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;

            if (CALL_TYPE_AUDIO == pCrData->call_type)
            {
                AudioCRIndexVector.push_back(Itr->first);
            }
            else
            {
                CRIndexVector.push_back(Itr->first);
            }
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_call_record_list() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_call_record_list() Enter------------: cr_pos=%d \r\n", cr_pos);

            if (CALL_TYPE_RECORD == pCrData->call_type)
            {
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Turn off video tasks that are not listed on the TSU: logic device ID=%s, IP address=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "TSU会话保活超时, 关闭TSU上面没有的录像任务: 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);

                /* 移除录像信息 */
                iRet = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_call_record_list() RemoveDeviceRecordInfo Error:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_call_record_list() RemoveDeviceRecordInfo OK:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
            }
            else
            {
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Turn off real-time video tasks that are not listed on the TSU: userID=%s, user IP address=%s, logic device ID=%s, IP address=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "TSU会话保活超时, 关闭TSU上面没有的实时视频业务:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
            }

            /* 通知TSU停止接收码流*/
            if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
            {
                i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "scan_call_record_list() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_call_record_list() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }
            else if (CALL_TYPE_RECORD == pCrData->call_type)
            {
                i = notify_tsu_delete_record_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "scan_call_record_list() notify_tsu_delete_record_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_call_record_list() notify_tsu_delete_record_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }

                /* 设置TSU 状态 */
                i = SetTSUStatus(pCrData->tsu_resource_index, 1);
            }
            else
            {
                i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "scan_call_record_list() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_call_record_list() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }

            /* 发送Bye给主叫侧 */
            if (pCrData->caller_ua_index >= 0)
            {
                /*发送Bye 给主叫侧 */
                i = SIP_SendBye(pCrData->caller_ua_index);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_call_record_list() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
            }

            /* 看是否有前端连接 */
            if (pCrData->callee_ua_index >= 0)
            {
                /* 查看是否有其他客户端业务 */
                other_cr_pos = is_GBLogic_device_has_other_service(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_call_record_list() is_GBLogic_device_has_other_service:other_cr_pos=%d \r\n", other_cr_pos);

                if (other_cr_pos < 0) /* 没有其他业务 */
                {
                    /*发送Bye 给被叫侧 */
                    i = SIP_SendBye(pCrData->callee_ua_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_call_record_list() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
                }
                else
                {
                    pOtherCrData = call_record_get(other_cr_pos);

                    if (NULL != pOtherCrData)
                    {
                        pOtherCrData->callee_ua_index = pCrData->callee_ua_index; /* 将前端的会话句柄拷贝到下个业务 */
                        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_call_record_list() callee_ua_index=%d copy from %d to %d \r\n", pOtherCrData->callee_ua_index, cr_pos, other_cr_pos);
                    }
                }
            }

            /* 移除业务数据 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_call_record_list() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_call_record_list() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    /* 音频转发任务处理 */
    if (AudioCRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_call_record_list() AudioCRIndexVector.size()=%d \r\n", (int)AudioCRIndexVector.size());

        for (index = 0; index < (int)AudioCRIndexVector.size(); index++)
        {
            cr_pos = AudioCRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_call_record_list() Auido Enter------------: cr_pos=%d \r\n", cr_pos);

            iRet = StopAudioCallService(pCrData);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_call_record_list() StopAudioCallService Error:cr_pos=%d \r\n", other_cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_call_record_list() StopAudioCallService OK:cr_pos=%d \r\n", other_cr_pos);
            }

            /* 移除业务数据 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_call_record_list() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_call_record_list() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    AudioCRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : scan_call_record_list_for_wait_expire
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
int scan_call_record_list_for_wait_expire()
{
    int i = 0;
    int iRet = 0;
    int index = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "scan_call_record_list_for_wait_expire() exit---: Call Record Srv Map NULL \r\n");
        return 0;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if (CALL_TYPE_NULL == pCrData->call_type)
        {
            continue;
        }

        if (CALL_STATUS_WAIT_ANSWER != pCrData->call_status)
        {
            continue;
        }

        pCrData->wait_answer_expire++;

        if (pCrData->wait_answer_expire >= 15)
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_call_record_list_for_wait_expire() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_call_record_list_for_wait_expire() Enter------------: cr_pos=%d \r\n", cr_pos);

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "呼叫任务等待响应超时, 关闭实时视频业务:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);

            /* 发送Bye给主叫侧 */
            if (pCrData->caller_ua_index >= 0)
            {
                /*发送Bye 给主叫侧 */
                i = SIP_SendBye(pCrData->caller_ua_index);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_call_record_list_for_wait_expire() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
            }

            /* 移除业务数据 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "scan_call_record_list_for_wait_expire() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_call_record_list_for_wait_expire() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : check_sipua_is_in_call_record_list
 功能描述  : 检测SIP UA是否在会话中
 输入参数  : int sipua_index
             vector<int>& CRSIPUAIndexVector
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int check_sipua_is_in_call_record_list(int sipua_index, vector<int>& CRSIPUAIndexVector)
{
    vector<int>::iterator result = std::find(CRSIPUAIndexVector.begin(), CRSIPUAIndexVector.end(), sipua_index);

    if (result == CRSIPUAIndexVector.end())  //没找到
    {
        return 0;
    }

    return 1;
}

/*****************************************************************************
 函 数 名  : get_all_sipua_in_call_record_list
 功能描述  : 获取所有再用胡呼叫任务的SIP UA Index
 输入参数  : vector<int>& SIPUAIndexVector
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年3月24日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int get_all_sipua_in_call_record_list(vector<int>& SIPUAIndexVector)
{
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    SIPUAIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        return 0;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if (CALL_TYPE_NULL == pCrData->call_type)
        {
            continue;
        }

        if (pCrData->caller_ua_index >= 0)
        {
            SIPUAIndexVector.push_back(pCrData->caller_ua_index);
        }

        if (pCrData->callee_ua_index >= 0)
        {
            SIPUAIndexVector.push_back(pCrData->callee_ua_index);
        }
    }

    CR_SMUTEX_UNLOCK();

    return 0;
}

int get_all_sipua_in_gb_device_list(vector<int>& SIPUAIndexVector)
{
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBDevice_Info_Iterator Itr;

    GBDEVICE_SMUTEX_LOCK();

    if (g_GBDeviceInfoMap.size() <= 0)
    {
        GBDEVICE_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_GBDeviceInfoMap.begin(); Itr != g_GBDeviceInfoMap.end(); Itr++)
    {
        pGBDeviceInfo = Itr->second;

        if (NULL == pGBDeviceInfo)
        {
            continue;
        }

        if (pGBDeviceInfo->catalog_subscribe_flag <= 0)
        {
            continue;
        }

        if (pGBDeviceInfo->catalog_subscribe_dialog_index >= 0)
        {
            SIPUAIndexVector.push_back(pGBDeviceInfo->catalog_subscribe_dialog_index);
        }
    }

    GBDEVICE_SMUTEX_UNLOCK();
    return 0;
}

int get_all_sipua_in_route_info_list(vector<int>& SIPUAIndexVector)
{
    int pos = -1;
    route_info_t* pRouteInfo = NULL;

    ROUTE_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RouteInfoList->pRouteInfoList) <= 0)
    {
        ROUTE_INFO_SMUTEX_UNLOCK();
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_RouteInfoList->pRouteInfoList); pos++)
    {
        pRouteInfo = (route_info_t*)osip_list_get(g_RouteInfoList->pRouteInfoList, pos);

        if (NULL == pRouteInfo)
        {
            continue;
        }

        if (pRouteInfo->catalog_subscribe_flag <= 0)
        {
            continue;
        }

        if (pRouteInfo->catalog_subscribe_dialog_index >= 0)
        {
            SIPUAIndexVector.push_back(pRouteInfo->catalog_subscribe_dialog_index);
        }
    }

    ROUTE_INFO_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 函 数 名  : scan_call_record_list_for_sipua
 功能描述  : 扫描所有的呼叫任务，查找在使用的SIPUA信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月21日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int scan_call_record_list_for_sipua()
{
    int iRet = 0;
    int index = -1;
    int sipua_index = -1;
    vector<int> StackSIPIndexVector;   /* 协议栈中使用的SIP UA */
    vector<int> CRSIPUAIndexVector;    /* 呼叫资源中使用的SIP UA */
    vector<int> SIPReleaseIndexVector; /* 需要释放的SIP UA */

    StackSIPIndexVector.clear();
    CRSIPUAIndexVector.clear();

    SIPReleaseIndexVector.clear();

    /* 获取到SIP协议中在用的所有SIP UA的索引 */
    iRet = SIP_GetAllUsedSIPUAIndex(StackSIPIndexVector);

    if ((int)StackSIPIndexVector.size() > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "扫描SIP会话句柄: 获取到协议栈中所有的在用的SIP UA句柄总数=%d", (int)StackSIPIndexVector.size());

        for (index = 0; index < (int)StackSIPIndexVector.size(); index++)
        {
            sipua_index = StackSIPIndexVector[index];
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_call_record_list_for_sipua() StackSIPIndexVector:index=%d, sipua_index=%d \r\n", index, sipua_index);
        }
    }

    /* 获取到呼叫资源中在用的所有SIP UA的索引 */
    iRet = get_all_sipua_in_call_record_list(CRSIPUAIndexVector);

    if ((int)CRSIPUAIndexVector.size() > 0)
    {
        for (index = 0; index < (int)CRSIPUAIndexVector.size(); index++)
        {
            sipua_index = CRSIPUAIndexVector[index];
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_call_record_list_for_sipua() get_all_sipua_in_call_record_list:index=%d, sipua_index=%d \r\n", index, sipua_index);
        }
    }

    /* 获取物理设备中在用的所有SIP UA的索引 */
    iRet = get_all_sipua_in_gb_device_list(CRSIPUAIndexVector);

    if ((int)CRSIPUAIndexVector.size() > 0)
    {
        for (index = 0; index < (int)CRSIPUAIndexVector.size(); index++)
        {
            sipua_index = CRSIPUAIndexVector[index];
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_call_record_list_for_sipua() get_all_sipua_in_gb_device_list:index=%d, sipua_index=%d \r\n", index, sipua_index);
        }
    }

    /* 获取上级路由中在用的所有SIP UA的索引 */
    iRet = get_all_sipua_in_route_info_list(CRSIPUAIndexVector);

    if ((int)CRSIPUAIndexVector.size() > 0)
    {
        for (index = 0; index < (int)CRSIPUAIndexVector.size(); index++)
        {
            sipua_index = CRSIPUAIndexVector[index];
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_call_record_list_for_sipua() get_all_sipua_in_route_info_list:index=%d, sipua_index=%d \r\n", index, sipua_index);
        }
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "扫描SIP会话句柄: 获取到呼叫资源中所有的在用的SIP UA句柄总数=%d", (int)CRSIPUAIndexVector.size());

    /* 检测在用的所有SIP UA的索引是否在call record的使用中 */
    if ((int)StackSIPIndexVector.size() > 0)
    {
        for (index = 0; index < (int)StackSIPIndexVector.size(); index++)
        {
            sipua_index = StackSIPIndexVector[index];

            if (!check_sipua_is_in_call_record_list(sipua_index, CRSIPUAIndexVector))
            {
                SIPReleaseIndexVector.push_back(sipua_index);
            }
        }
    }

    StackSIPIndexVector.clear();

    if ((int)SIPReleaseIndexVector.size() > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "扫描SIP会话句柄: 获取到业务没有使用, 但是协议栈还存在的SIP UA句柄总数=%d", (int)SIPReleaseIndexVector.size());

        for (index = 0; index < (int)SIPReleaseIndexVector.size(); index++)
        {
            sipua_index = SIPReleaseIndexVector[index];

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "扫描SIP会话句柄: 获取到业务没有使用, 但是协议栈还存在的SIP UA句柄=%d, 通知协议栈关闭", sipua_index);

            SIP_ReleaseUnUsedSIPUA(sipua_index);
        }
    }

    SIPReleaseIndexVector.clear();

    return 0;
}
#endif

/*****************************************************************************
 函 数 名  : ModifySDPIPAndPort
 功能描述  : 修改SDP中的IP和端口信息
 输入参数  : sdp_t* sdp
                            char* ip_addr
                            int port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月3日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int ModifySDPIPAndPort(sdp_message_t* sdp, char* ip_addr, int port)
{
    char* port_media = NULL;
    char* addr_sess = NULL;
    char* addr_media = NULL;
    int media_stream_no = 0;
    sdp_connection_t* sdp_conn = NULL;
    sdp_media_t* sdp_med = NULL;

    if ((NULL == sdp) || (NULL == ip_addr) || (port <= 0))
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "ModifySDPIPAndPort() exit---: Param Error \r\n");
        return -1;
    }

    if (ip_addr[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "ModifySDPIPAndPort() exit---: IP Addr Error \r\n");
        return -1;
    }

    if (NULL != sdp->o_addr)
    {
        osip_free(sdp->o_addr);
        sdp->o_addr = NULL;
        sdp->o_addr = osip_getcopy(ip_addr);
    }

    /*
    * first, check presence of a 'c=' item on session level
    */
    if (sdp->c_connection != NULL)
    {
        addr_sess = sdp_message_c_addr_get(sdp, -1, 0);

        if (addr_sess == NULL)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ModifySDPIPAndPort() exit---: Get C Addr Error \r\n");
            return -1;
        }

        if (strncmp(addr_sess, "0.0.0.0", 8) != 0)
        {
            osip_free(addr_sess);
            addr_sess = NULL;
            sdp->c_connection->c_addr = osip_getcopy(ip_addr);
        }
    }

    /*
    * loop through all media descritions
    */
    media_stream_no = 0;

    while (!sdp_message_endof_media(sdp, media_stream_no))
    {
        sdp_med = NULL;
        sdp_med  = (sdp_media_t*)osip_list_get(&sdp->m_medias, media_stream_no);

        if (sdp_med == NULL)
        {
            break;
        }

        port_media = sdp_med->m_port;

        if (port_media == NULL)
        {
            break;
        }

        addr_media = NULL;
        sdp_conn = NULL;
        sdp_conn  = sdp_message_connection_get(sdp, media_stream_no, 0);

        if (sdp_conn != NULL)
        {
            addr_media = sdp_conn->c_addr;
        }

        if (addr_media != NULL)
        {
            if (strncmp(addr_media, "0.0.0.0", 8) != 0)
            {
                osip_free(addr_media);
                addr_media = NULL;
                sdp_conn->c_addr = osip_getcopy(ip_addr);

                if (NULL == sdp_conn->c_addr)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ModifySDPIPAndPort() exit---: C Addr Error \r\n");
                    return -1;
                }
            }
        }

        if (strncmp(port_media, "0", 2) != 0)
        {
            //sfree(sdp_med->m_number_of_port);
            osip_free(port_media);
            port_media = NULL;
            sdp_med->m_port = (char*)osip_malloc(6);

            if (NULL == sdp_med->m_port)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ModifySDPIPAndPort() exit---: m port Error \r\n");
                return -1;
            }

            snprintf(sdp_med->m_port, 6, "%i", port);
        }

        media_stream_no++;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : ModifySDPTransProtocol
 功能描述  : 更改SDP中的传输协议
 输入参数  : sdp_message_t* sdp
             int protocol
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月25日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ModifySDPTransProtocol(sdp_message_t* sdp, int protocol)
{
    int media_stream_no = 0;
    sdp_media_t* sdp_med = NULL;

    if ((NULL == sdp) || (protocol < 0))
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "ModifySDPTransProtocol() exit---: Param Error \r\n");
        return -1;
    }

    /*
    * loop through all media descritions
    */
    media_stream_no = 0;

    while (!sdp_message_endof_media(sdp, media_stream_no))
    {
        sdp_med = NULL;
        sdp_med  = (sdp_media_t*)osip_list_get(&sdp->m_medias, media_stream_no);

        if (sdp_med == NULL)
        {
            break;
        }

        if (NULL == sdp_med->m_proto)
        {
            if (1 == protocol)
            {
                sdp_med->m_proto = osip_getcopy((char*)"TCP");
            }
            else
            {
                sdp_med->m_proto = osip_getcopy((char*)"RTP/AVP");
            }
        }
        else
        {
            osip_free(sdp_med->m_proto);
            sdp_med->m_proto = NULL;

            if (1 == protocol)
            {
                sdp_med->m_proto = osip_getcopy((char*)"TCP");
            }
            else
            {
                sdp_med->m_proto = osip_getcopy((char*)"RTP/AVP");
            }
        }

        media_stream_no++;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : ModifySDPSName
 功能描述  : 修改SDP中的S Name名称
 输入参数  : sdp_message_t* sdp
             call_type_t call_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月27日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ModifySDPSName(sdp_message_t* sdp, call_type_t call_type)
{
    string strSName = "";

    if ((NULL == sdp) || (CALL_TYPE_NULL == call_type))
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "ModifySDPSName() exit---: Param Error \r\n");
        return -1;
    }

    if (CALL_TYPE_RECORD_PLAY == call_type)
    {
        strSName = "Playback";
    }
    else if (CALL_TYPE_DOWNLOAD == call_type)
    {
        strSName = "Download";
    }
    else
    {
        strSName = "Play";
    }

    if (NULL != sdp->s_name
        && 0 != sstrcmp(sdp->s_name, (char *)strSName.c_str()))
    {
        osip_free(sdp->s_name);
        sdp->s_name = NULL;
        sdp->s_name = osip_getcopy((char *)strSName.c_str());
    }
    else if (NULL == sdp->s_name)
    {
        sdp->s_name = osip_getcopy((char *)strSName.c_str());
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : ModifySDPRecordPlayTime
 功能描述  : 修改SDP中的开始播放时间
 输入参数  : sdp_message_t* sdp
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月25日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ModifySDPRecordPlayTime(sdp_message_t* sdp)
{
    char* play_time = NULL;
    sdp_time_descr_t* td = NULL;

    if (NULL == sdp)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "ModifySDPRecordPlayTime() exit---: Param Error \r\n");
        return -1;
    }

    td = (sdp_time_descr_t*) osip_list_get(&sdp->t_descrs, 0);

    if (td == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "ModifySDPRecordPlayTime() exit---: Get Time Descr Error \r\n");
        return -1;
    }

    play_time = (char*) osip_list_get(&td->r_repeats, 0); /* 新增的播放时间信息 */

    if (play_time == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "ModifySDPRecordPlayTime() exit---: Get Play Time Error \r\n");
        return -1;
    }


    if (NULL != td->t_start_time)
    {
        osip_free(td->t_start_time);
        td->t_start_time = NULL;
    }

    td->t_start_time = osip_getcopy(play_time);

    return 0;
}

/*****************************************************************************
 函 数 名  : DelSDPMediaAttributeByName
 功能描述  : 根据sdp的属性名称删除属性
 输入参数  : sdp_message_t* sdp
             char* attribute_name
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月18日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DelSDPMediaAttributeByName(sdp_message_t* sdp, char* attribute_name)
{
    int iRet = 0;
    int pos_media = -1;
    int found = 0;
    char* tmp = NULL;

    if (sdp == NULL || NULL == attribute_name)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "DelSDPMediaAttributeByName() exit---: Param Error \r\n");
        return -1;
    }

    found = 0;
    pos_media = 0;

    while (!sdp_message_endof_media(sdp, pos_media))  /* is have media */
    {
        /* media type */
        tmp = sdp_message_m_media_get(sdp, pos_media);

        if (0 == sstrcmp(tmp, "video"))
        {
            found = 1;
            break;
        }

        pos_media++;
    }

    if (!found)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "DelSDPMediaAttributeByName() exit---: Not Found \r\n");
        return -1;
    }

    iRet = sdp_message_a_attribute_del(sdp, pos_media, attribute_name);

    return iRet;
}

/*****************************************************************************
 函 数 名  : DelSDPTimeRepeatInfo
 功能描述  : 删除SDP中的时间R信息
 输入参数  : sdp_message_t* sdp
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月31日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DelSDPTimeRepeatInfo(sdp_message_t* sdp)
{
    sdp_time_descr_t* td = NULL;

    if (sdp == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "DelSDPMediaAttributeByName() exit---: Param Error \r\n");
        return -1;
    }

    td = (sdp_time_descr_t*) osip_list_get(&sdp->t_descrs, 0);

    if (td == NULL)
    {
        return -1;
    }

    osip_list_ofchar_free(&td->r_repeats);

    return 0;
}

/*****************************************************************************
 函 数 名  : cr_tsu_ip_get_by_ethname
 功能描述  : 获取呼叫信息中的IP地址
 输入参数  : cr_t* pCrData
             char* eth_name
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年8月18日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
char* cr_tsu_ip_get_by_ethname(cr_t* pCrData, char* eth_name)
{
    int i = 0;

    if (NULL == eth_name || pCrData == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "cr_tsu_ip_get_by_ethname() exit---: Param Error:eth_name=%s \r\n", eth_name);
        return NULL;
    }

    if (eth_name[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cr_tsu_ip_get_by_ethname() exit---: Param Error:eth_name NULL \r\n");
        return NULL;
    }

    if (0 == sstrcmp(eth_name, pCrData->TSUVideoIP.eth_name))
    {
        return pCrData->TSUVideoIP.local_ip;
    }
    else if (0 == sstrcmp(eth_name, pCrData->TSUDeviceIP.eth_name))
    {
        return pCrData->TSUDeviceIP.local_ip;
    }
    else     /* 如果没找到，找个默认的视频网IP地址 */
    {
        return pCrData->TSUVideoIP.local_ip;
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : cr_tsu_ip_get_by_type
 功能描述  : 根据ip地址类型获取tsu的ip地址
 输入参数  : cr_t* pCrData
             ip_addr_type_t ip_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月9日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
char* cr_tsu_ip_get_by_type(cr_t* pCrData, ip_addr_type_t ip_type)
{
    int i = 0;
    ip_pair_t* pIPaddr = NULL;

    if (IP_ADDR_NULL == ip_type || pCrData == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "cr_tsu_ip_get_by_type() exit---: Param Error:ip_type=%d \r\n", ip_type);
        return NULL;
    }

    if (ip_type == pCrData->TSUVideoIP.ip_type)
    {
        return pCrData->TSUVideoIP.local_ip;
    }
    else if (ip_type == pCrData->TSUDeviceIP.ip_type)
    {
        return pCrData->TSUDeviceIP.local_ip;
    }
    else     /* 如果没找到，找个默认的视频网IP地址 */
    {
        return pCrData->TSUVideoIP.local_ip;
    }

    return NULL;
}

/*****************************************************************************
 函 数 名  : get_cr_sdp_tsu_ip
 功能描述  : 获取呼叫资源中的用于SDP的TSU的 IP地址
 输入参数  : cr_t* pCrData
             char* eth_name
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月12日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
char* get_cr_sdp_tsu_ip(cr_t* pCrData, char* eth_name)
{
    char* sdp_tsu_ip = NULL;
    ip_addr_type_t ip_type = IP_ADDR_NULL;

    /* 根据网口找IP地址 */
    sdp_tsu_ip = cr_tsu_ip_get_by_ethname(pCrData, eth_name);

    if (NULL == sdp_tsu_ip)
    {
        /* 再根据地址类型找一次IP地址 */
        ip_type = get_local_ip_type(eth_name);

        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "get_cr_sdp_tsu_ip(): ip_type=%d \r\n", ip_type);

        sdp_tsu_ip = cr_tsu_ip_get_by_type(pCrData, ip_type);

        if (NULL == sdp_tsu_ip || sdp_tsu_ip[0] == '\0')
        {
            return NULL;
        }
    }

    return sdp_tsu_ip;
}

/*****************************************************************************
 函 数 名  : StopCallService
 功能描述  : 停止呼叫任务
 输入参数  : int cr_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月29日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int StopCallService(cr_t* pCrData)
{
    int i = 0;

    if (pCrData == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopCallService() exit---: Get Call Record Data Error \r\n");
        return -1;
    }

    if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopCallService() exit---: call_status Error: call_status=%d \r\n", pCrData->call_status);
        return -1;
    }

    if (pCrData->caller_ua_index >= 0)
    {
        /* 发送Bye 给主叫 */
        i = SIP_SendBye(pCrData->caller_ua_index);
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopCallService() SIP_SendBye To Caller:caller_id=%s, caller_ua_index=%d\r\n", pCrData->caller_id, pCrData->caller_ua_index);
    }

    if (pCrData->callee_ua_index >= 0)
    {
        /* 发送Bye 给被叫 */
        i = SIP_SendBye(pCrData->callee_ua_index);
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopCallService() SIP_SendBye To Callee:callee_id=%s, callee_ua_index=%d\r\n", pCrData->callee_id, pCrData->callee_ua_index);
    }

    /* 通知TSU停止接收码流*/
    if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
        || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
    {
        i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopCallService() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopCallService() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }
    }
    else if (CALL_TYPE_RECORD == pCrData->call_type)
    {
        i = notify_tsu_delete_record_task(pCrData->tsu_ip, pCrData->task_id);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopCallService() notify_tsu_delete_record_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopCallService() notify_tsu_delete_record_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }

        /* 设置TSU 状态 */
        i = SetTSUStatus(pCrData->tsu_resource_index, 1);
    }
    else
    {
        i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopCallService() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopCallService() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAudioCallService
 功能描述  : 停止音频对讲业务
 输入参数  : cr_t* pCrData
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月6日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StopAudioCallService(cr_t* pCrData)
{
    int i = 0;

    if (pCrData == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAudioCallService() exit---: Get Call Record Data Error \r\n");
        return -1;
    }

    if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAudioCallService() exit---: call_status Error: call_status=%d \r\n", pCrData->call_status);
        return -1;
    }

    if (pCrData->caller_ua_index >= 0)
    {
        /* 发送Bye 给主叫 */
        i = SIP_SendBye(pCrData->caller_ua_index);
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAudioCallService() SIP_SendBye To Caller:caller_id=%s, caller_ua_index=%d\r\n", pCrData->caller_id, pCrData->caller_ua_index);
    }

    if (pCrData->callee_ua_index >= 0)
    {
        /* 发送Bye 给被叫 */
        i = SIP_SendBye(pCrData->callee_ua_index);
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAudioCallService() SIP_SendBye To Callee:callee_id=%s, callee_ua_index=%d\r\n", pCrData->callee_id, pCrData->callee_ua_index);
    }

    /* 通知TSU停止转发码流*/
    i = notify_tsu_delete_audio_transfer_task(pCrData->tsu_ip, pCrData->caller_sdp_ip, pCrData->caller_sdp_port);

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAudioCallService() notify_tsu_delete_audio_transfer_task Error:tsu_ip=%s, receive_ip=%s, receive_port=%d, i=%d \r\n", pCrData->tsu_ip, pCrData->caller_sdp_ip, pCrData->caller_sdp_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAudioCallService() notify_tsu_delete_audio_transfer_task OK:tsu_ip=%s, receive_ip=%s, receive_port=%d, i=%d \r\n", pCrData->tsu_ip, pCrData->caller_sdp_ip, pCrData->caller_sdp_port, i);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllServiceTaskByTSUID
 功能描述  : 根据TSU索引停止该TSU上面的所有呼叫任务
 输入参数  : char* tsu_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月24日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int StopAllServiceTaskByTSUID(char* tsu_id)
{
    int i = 0;
    int iRet = 0;
    int index = -1;
    int cr_pos = -1;
    int other_cr_pos = -1;
    cr_t* pCrData = NULL;
    cr_t* pOtherCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if (NULL == tsu_id)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByTSUID() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByTSUID() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed) || ('\0' == pCrData->tsu_device_id[0]))
        {
            continue;
        }

        if (CALL_TYPE_NULL == pCrData->call_type)
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if (0 == sstrcmp(pCrData->tsu_device_id, tsu_id))
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByTSUID() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTSUID() Enter------------: cr_pos=%d \r\n", cr_pos);

            if (CALL_TYPE_RECORD == pCrData->call_type)
            {
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "TSU go offline or restart, trigger off closing video monitoring:logic device ID=%s, IP address=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "TSU掉线或重启, 触发关闭其上面的录像业务:逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);

                /* 移除录像信息 */
                iRet = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByTSUID() RemoveDeviceRecordInfo Error:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTSUID() RemoveDeviceRecordInfo OK:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
            }
            else
            {
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "TSU go offline or restart, trigger off closing real-time monitoring:user ID=%s, user IPaddress=%s, logic deviceID=%s, IPaddress=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "TSU掉线或重启, 触发关闭其上面的实时视频业务:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
            }

            /* 通知TSU停止接收码流*/
            if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
            {
                i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByTSUID() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTSUID() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }
            else if (CALL_TYPE_RECORD == pCrData->call_type)
            {
                i = notify_tsu_delete_record_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByTSUID() notify_tsu_delete_record_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTSUID() notify_tsu_delete_record_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }

                /* 设置TSU 状态 */
                i = SetTSUStatus(pCrData->tsu_resource_index, 1);
            }
            else
            {
                i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByTSUID() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTSUID() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }

            /* 发送Bye给主叫侧 */
            if (pCrData->caller_ua_index >= 0)
            {
                /*发送Bye 给主叫侧 */
                i = SIP_SendBye(pCrData->caller_ua_index);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTSUID() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
            }

            /* 看是否有前端连接 */
            if (pCrData->callee_ua_index >= 0)
            {
                /* 查看是否有其他客户端业务 */
                other_cr_pos = is_GBLogic_device_has_other_service(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTSUID() is_GBLogic_device_has_other_service:other_cr_pos=%d \r\n", other_cr_pos);

                if (other_cr_pos < 0) /* 没有其他业务 */
                {
                    /*发送Bye 给被叫侧 */
                    i = SIP_SendBye(pCrData->callee_ua_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTSUID() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
                }
                else
                {
                    pOtherCrData = call_record_get(other_cr_pos);

                    if (NULL != pOtherCrData)
                    {
                        pOtherCrData->callee_ua_index = pCrData->callee_ua_index; /* 将前端的会话句柄拷贝到下个业务 */
                        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTSUID() callee_ua_index=%d copy from %d to %d \r\n", pOtherCrData->callee_ua_index, cr_pos, other_cr_pos);
                    }
                }
            }

            /* 移除业务数据 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByTSUID() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByTSUID() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllServiceTaskByTaskID
 功能描述  : 根据任务ID关闭任务
 输入参数  : char* task_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年1月8日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StopAllServiceTaskByTaskID(char* task_id)
{
    int i = 0;
    int iRet = 0;
    int index = -1;
    int cr_pos = -1;
    int other_cr_pos = -1;
    cr_t* pCrData = NULL;
    cr_t* pOtherCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if (NULL == task_id)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByTaskID() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByTaskID() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed) || ('\0' == pCrData->tsu_device_id[0]))
        {
            continue;
        }

        if (CALL_TYPE_NULL == pCrData->call_type)
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if (0 == sstrcmp(pCrData->task_id, task_id))
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByTaskID() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTaskID() Enter------------: cr_pos=%d \r\n", cr_pos);

            if (CALL_TYPE_RECORD == pCrData->call_type)
            {
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "TSU Notify Task List, trigger off closing video monitoring:logic device ID=%s, IP address=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "TSU通知码流TCP结束, 触发关闭其上面的录像业务:逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);

                /* 移除录像信息 */
                iRet = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByTaskID() RemoveDeviceRecordInfo Error:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTaskID() RemoveDeviceRecordInfo OK:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
            }
            else
            {
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "TSU Notify Task List, trigger off closing real-time monitoring:user ID=%s, user IPaddress=%s, logic deviceID=%s, IPaddress=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "TSU通知码流TCP结束, 触发关闭其上面的实时视频业务:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
            }

            /* 通知TSU停止接收码流*/
            if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
            {
                i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByTaskID() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTaskID() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }
            else if (CALL_TYPE_RECORD == pCrData->call_type)
            {
                i = notify_tsu_delete_record_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByTaskID() notify_tsu_delete_record_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTaskID() notify_tsu_delete_record_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }

                /* 设置TSU 状态 */
                i = SetTSUStatus(pCrData->tsu_resource_index, 1);
            }
            else
            {
                i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByTaskID() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTaskID() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }

            /* 发送Bye给主叫侧 */
            if (pCrData->caller_ua_index >= 0)
            {
                /*发送Bye 给主叫侧 */
                i = SIP_SendBye(pCrData->caller_ua_index);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTaskID() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
            }

            /* 看是否有前端连接 */
            if (pCrData->callee_ua_index >= 0)
            {
                /* 查看是否有其他客户端业务 */
                other_cr_pos = is_GBLogic_device_has_other_service(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTaskID() is_GBLogic_device_has_other_service:other_cr_pos=%d \r\n", other_cr_pos);

                if (other_cr_pos < 0) /* 没有其他业务 */
                {
                    /*发送Bye 给被叫侧 */
                    i = SIP_SendBye(pCrData->callee_ua_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTaskID() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
                }
                else
                {
                    pOtherCrData = call_record_get(other_cr_pos);

                    if (NULL != pOtherCrData)
                    {
                        pOtherCrData->callee_ua_index = pCrData->callee_ua_index; /* 将前端的会话句柄拷贝到下个业务 */
                        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByTaskID() callee_ua_index=%d copy from %d to %d \r\n", pOtherCrData->callee_ua_index, cr_pos, other_cr_pos);
                    }
                }
            }

            /* 移除业务数据 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByTaskID() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByTaskID() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAudioServiceTaskByTSUID
 功能描述  : 根据TSU ID停止音频业务
 输入参数  : char* tsu_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月9日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StopAudioServiceTaskByTSUID(char* tsu_id)
{
    int iRet = 0;
    int index = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if (NULL == tsu_id)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAudioServiceTaskByTSUID() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByTSUID() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed) || ('\0' == pCrData->tsu_device_id[0]))
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if (CALL_TYPE_NULL == pCrData->call_type
            || CALL_TYPE_AUDIO != pCrData->call_type)
        {
            continue;
        }

        if (0 == sstrcmp(pCrData->tsu_device_id, tsu_id))
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAudioServiceTaskByTSUID() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAudioServiceTaskByTSUID() Enter------------: cr_pos=%d \r\n", cr_pos);

            iRet = StopAudioCallService(pCrData);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAudioServiceTaskByTSUID() StopAudioCallService Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAudioServiceTaskByTSUID() StopAudioCallService OK:cr_pos=%d \r\n", cr_pos);
            }

            /* 移除业务数据 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByTSUID() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByTSUID() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopRecordServiceTaskByTSUID
 功能描述  : 根据TSU ID关闭TSU上面的录像任务
 输入参数  : char* tsu_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年4月11日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StopRecordServiceTaskByTSUID(char* tsu_id)
{
    int i = 0;
    int iRet = 0;
    int index = -1;
    int cr_pos = -1;
    int other_cr_pos = -1;
    cr_t* pCrData = NULL;
    cr_t* pOtherCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if (NULL == tsu_id)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopRecordServiceTaskByTSUID() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopRecordServiceTaskByTSUID() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed) || ('\0' == pCrData->tsu_device_id[0]))
        {
            continue;
        }

        if (CALL_TYPE_NULL == pCrData->call_type
            || CALL_TYPE_RECORD != pCrData->call_type)
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if (0 == sstrcmp(pCrData->tsu_device_id, tsu_id))
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopRecordServiceTaskByTSUID() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopRecordServiceTaskByTSUID() Enter------------: cr_pos=%d \r\n", cr_pos);

            if (CALL_TYPE_RECORD == pCrData->call_type)
            {
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "TSU type change, trigger off closing video monitoring:logic deviceID=%s, IP address=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "TSU类型发生变化, 触发关闭其上面的录像业务:逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);

                /* 移除录像信息 */
                iRet = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopRecordServiceTaskByTSUID() RemoveDeviceRecordInfo Error:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopRecordServiceTaskByTSUID() RemoveDeviceRecordInfo OK:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
            }

            /* 通知TSU停止接收码流*/
            if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
            {
                i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopRecordServiceTaskByTSUID() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopRecordServiceTaskByTSUID() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }
            else if (CALL_TYPE_RECORD == pCrData->call_type)
            {
                i = notify_tsu_delete_record_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopRecordServiceTaskByTSUID() notify_tsu_delete_record_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopRecordServiceTaskByTSUID() notify_tsu_delete_record_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }

                /* 设置TSU 状态 */
                i = SetTSUStatus(pCrData->tsu_resource_index, 1);
            }
            else
            {
                i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopRecordServiceTaskByTSUID() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopRecordServiceTaskByTSUID() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }

            /* 发送Bye给主叫侧 */
            if (pCrData->caller_ua_index >= 0)
            {
                /*发送Bye 给主叫侧 */
                i = SIP_SendBye(pCrData->caller_ua_index);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopRecordServiceTaskByTSUID() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
            }

            /* 看是否有前端连接 */
            if (pCrData->callee_ua_index >= 0)
            {
                /* 查看是否有其他客户端业务 */
                other_cr_pos = is_GBLogic_device_has_other_service(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopRecordServiceTaskByTSUID() is_GBLogic_device_has_other_service:other_cr_pos=%d \r\n", other_cr_pos);

                if (other_cr_pos < 0) /* 没有其他业务 */
                {
                    /*发送Bye 给被叫侧 */
                    i = SIP_SendBye(pCrData->callee_ua_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopRecordServiceTaskByTSUID() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
                }
                else
                {
                    pOtherCrData = call_record_get(other_cr_pos);

                    if (NULL != pOtherCrData)
                    {
                        pOtherCrData->callee_ua_index = pCrData->callee_ua_index; /* 将前端的会话句柄拷贝到下个业务 */
                        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopRecordServiceTaskByTSUID() callee_ua_index=%d copy from %d to %d \r\n", pOtherCrData->callee_ua_index, cr_pos, other_cr_pos);
                    }
                }
            }

            /* 移除业务数据 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopRecordServiceTaskByTSUID() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopRecordServiceTaskByTSUID() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllServiceTaskByLogicDeviceID
 功能描述  : 根据前端逻辑点位ID停止呼叫任务
 输入参数  : char* device_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年11月6日
    作    者   : 用户路由信息清理
    修改内容   : 新生成函数

*****************************************************************************/
int StopAllServiceTaskByLogicDeviceID(char* logic_device_id)
{
    int iRet = 0;
    int index = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if (NULL == logic_device_id)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByLogicDeviceID() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByTSUIndex() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
            || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
            || (CALL_TYPE_AUDIO == pCrData->call_type)
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if (0 == sstrcmp(pCrData->callee_id, logic_device_id))
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByLogicDeviceID() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByLogicDeviceID() Enter------------: cr_pos=%d \r\n", cr_pos);

            if (CALL_TYPE_RECORD == pCrData->call_type)
            {
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Video sender go offline or does not have stream, trigger off closing video monitoring:logic device ID=%s, IP address=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "视频发送方掉线或者没有码流, 触发关闭其上面的录像业务:逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);

                /* 移除录像信息 */
                iRet = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByLogicDeviceID() RemoveDeviceRecordInfo Error:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByLogicDeviceID() RemoveDeviceRecordInfo OK:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
            }
            else
            {
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video sender go offline or does not have stream, trigger off closing real-time monitoring:userID=%s, user IP address=%s, logic device ID=%s, IPaddress=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "视频发送方掉线或者没有码流, 触发关闭其上面的实时视频业务:请求方ID=%s, 请求方IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
            }

            /* 发送Bye 给所有其他业务主叫侧用户*/
            iRet = send_bye_to_all_other_caller_by_callee_id(pCrData->callee_id, cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByLogicDeviceID() send_bye_to_all_other_caller_by_callee_id Error:callee_id=%s, cr_pos=%d, i=%d \r\n", pCrData->callee_id, cr_pos, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByLogicDeviceID() send_bye_to_all_other_caller_by_callee_id OK:callee_id=%s, cr_pos=%d, i=%d \r\n", pCrData->callee_id, cr_pos, iRet);
            }

            /* 停止业务 */
            iRet = StopCallService(pCrData);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByLogicDeviceID() StopCallService Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByLogicDeviceID() StopCallService OK:cr_pos=%d \r\n", cr_pos);
            }

            /* 移除业务数据 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByLogicDeviceID() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByLogicDeviceID() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAudioServiceTaskByLogicDeviceID
 功能描述  : 根据逻辑设备ID停止音频对讲业务
 输入参数  : char* logic_device_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月6日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StopAudioServiceTaskByLogicDeviceID(char* logic_device_id)
{
    int iRet = 0;
    int index = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if (NULL == logic_device_id)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAudioServiceTaskByLogicDeviceID() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByTSUIndex() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_AUDIO != pCrData->call_type)
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if (0 == sstrcmp(pCrData->callee_id, logic_device_id))
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAudioServiceTaskByLogicDeviceID() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAudioServiceTaskByLogicDeviceID() Enter------------: cr_pos=%d \r\n", cr_pos);

            /* 停止业务 */
            iRet = StopAudioCallService(pCrData);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAudioServiceTaskByLogicDeviceID() StopAudioCallService Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAudioServiceTaskByLogicDeviceID() StopAudioCallService OK:cr_pos=%d \r\n", cr_pos);
            }

            /* 移除业务数据 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAudioServiceTaskByLogicDeviceID() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAudioServiceTaskByLogicDeviceID() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllServiceTaskByLogicDeviceIDAndStreamType
 功能描述  : 根据逻辑点位ID和流类型停止呼叫任务
 输入参数  : char* device_id
             int stream_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年9月28日 星期日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StopAllServiceTaskByLogicDeviceIDAndStreamType(char* device_id, int stream_type)
{
    int iRet = 0;
    int index = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if (NULL == device_id || stream_type <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByLogicDeviceIDAndStreamType() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByDeviceIDAndStreamType() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
            || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
            || (CALL_TYPE_AUDIO == pCrData->call_type)
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if (0 == sstrcmp(pCrData->callee_id, device_id) && pCrData->callee_stream_type == stream_type)
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByLogicDeviceIDAndStreamType() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByLogicDeviceIDAndStreamType() Enter------------: cr_pos=%d \r\n", cr_pos);

            if (CALL_TYPE_RECORD == pCrData->call_type)
            {
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Video sender go offline or does not have stream, trigger off closing video monitoring:logic device ID=%s, IP address=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "视频发送方掉线或者没有码流, 触发关闭其上面的录像业务:逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);

                /* 移除录像信息 */
                iRet = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByLogicDeviceIDAndStreamType() RemoveDeviceRecordInfo Error:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByLogicDeviceIDAndStreamType() RemoveDeviceRecordInfo OK:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
            }
            else
            {
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video sender go offline or does not have stream, trigger off closing real-time monitoring:user ID=%s, user IP address=%s, logic device ID=%s, IP address=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "视频发送方掉线或者没有码流, 触发关闭其上面的实时视频业务:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
            }

            /* 发送Bye 给所有其他业务主叫侧用户*/
            iRet = send_bye_to_all_other_caller_by_callee_id_and_streamtype(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByLogicDeviceIDAndStreamType() send_bye_to_all_other_caller_by_callee_id_and_streamtype Error:callee_id=%s, cr_pos=%d, i=%d \r\n", pCrData->callee_id, cr_pos, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByLogicDeviceIDAndStreamType() send_bye_to_all_other_caller_by_callee_id_and_streamtype OK:callee_id=%s, cr_pos=%d, i=%d \r\n", pCrData->callee_id, cr_pos, iRet);
            }

            /* 停止业务 */
            iRet = StopCallService(pCrData);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByLogicDeviceIDAndStreamType() StopCallService Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByLogicDeviceIDAndStreamType() StopCallService OK:cr_pos=%d \r\n", cr_pos);
            }

            /* 移除业务数据 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByLogicDeviceIDAndStreamType() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByLogicDeviceIDAndStreamType() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllServiceTaskByCallerUAIndex
 功能描述  : 根据主叫UA索引停止呼叫任务
 输入参数  : int caller_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月3日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int StopAllServiceTaskByCallerUAIndex(int caller_ua_index)
{
    int i = 0;
    int iRet = 0;
    int index = 0;
    int cr_pos = -1;
    int other_cr_pos = -1;
    cr_t* pCrData = NULL;
    cr_t* pOtherCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if (caller_ua_index < 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByCallerUAIndex() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByCallerUAIndex() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if (CALL_TYPE_NULL == pCrData->call_type || pCrData->caller_ua_index < 0)
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if (pCrData->caller_ua_index == caller_ua_index)
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByCallerUAIndex() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video requester interaction timeout, trigger off closing video tasks:userID=%s, user IP address=%s, logic device ID=%s, IP address=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "视频请求方会话超时, 触发关闭视频业务:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerUAIndex() Enter------------: cr_pos=%d \r\n", cr_pos);

            /* 通知TSU停止接收码流*/
            if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
            {
                i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByCallerUAIndex() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerUAIndex() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }
            else
            {
                i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByCallerUAIndex() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerUAIndex() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }

            /* 发送Bye给主叫侧 */
            if (pCrData->caller_ua_index >= 0)
            {
                /*发送Bye 给主叫侧 */
                i = SIP_SendBye(pCrData->caller_ua_index);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerUAIndex() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
            }

            /* 看是否有前端连接 */
            if (pCrData->callee_ua_index >= 0)
            {
                /* 查看是否有其他客户端业务 */
                other_cr_pos = is_GBLogic_device_has_other_service(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerUAIndex() is_GBLogic_device_has_other_service:other_cr_pos=%d \r\n", other_cr_pos);

                if (other_cr_pos < 0) /* 没有其他业务 */
                {
                    /*发送Bye 给被叫侧 */
                    i = SIP_SendBye(pCrData->callee_ua_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerUAIndex() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
                }
                else
                {
                    pOtherCrData = call_record_get(other_cr_pos);

                    if (NULL != pOtherCrData)
                    {
                        pOtherCrData->callee_ua_index = pCrData->callee_ua_index; /* 将前端的会话句柄拷贝到下个业务 */
                        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerUAIndex() callee_ua_index=%d copy from %d to %d \r\n", pOtherCrData->callee_ua_index, cr_pos, other_cr_pos);
                    }
                }
            }

            /* 移除呼叫记录信息 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByCallerUAIndex() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByCallerUAIndex() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllServiceTaskByCalleeUAIndex
 功能描述  : 根据被叫UA索引停止呼叫任务
 输入参数  : int callee_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月3日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int StopAllServiceTaskByCalleeUAIndex(int callee_ua_index)
{
    int iRet = 0;
    int index = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if (callee_ua_index < 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByCalleeUAIndex() exit---: Param Error \r\n");
        return -1;
    }

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByCalleeUAIndex() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if (CALL_TYPE_NULL == pCrData->call_type || pCrData->callee_ua_index < 0)
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if (pCrData->callee_ua_index == callee_ua_index)
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByCalleeUAIndex() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCalleeUAIndex() Enter------------: cr_pos=%d \r\n", cr_pos);

            if (CALL_TYPE_RECORD == pCrData->call_type)
            {
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Video sender interaction timeout, trigger off closing video monitoring: logic deviceID=%s, IP address=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "视频发送方会话超时, 触发关闭录像业务:逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);

                /* 移除录像信息 */
                iRet = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByCalleeUAIndex() RemoveDeviceRecordInfo Error:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCalleeUAIndex() RemoveDeviceRecordInfo OK:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
            }
            else
            {
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video sender interation timeout, trigger off closing video tasks:user ID=%s, user IP address=%s, logic device ID=%s, IP address=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "视频发送方会话超时, 触发关闭视频业务:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
            }

            /* 发送Bye 给所有其他业务主叫侧用户*/
            iRet = send_bye_to_all_other_caller_by_callee_id_and_streamtype(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByCalleeUAIndex() send_bye_to_all_other_caller_by_callee_id_and_streamtype Error:callee_id=%s, callee_stream_type=%d, cr_pos=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, cr_pos, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCalleeUAIndex() send_bye_to_all_other_caller_by_callee_id_and_streamtype OK:callee_id=%s, callee_stream_type=%d, cr_pos=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, cr_pos, iRet);
            }

            /* 停止业务 */
            iRet = StopCallService(pCrData);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByCalleeUAIndex() StopCallService Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCalleeUAIndex() StopCallService OK:cr_pos=%d \r\n", cr_pos);
            }

            /* 移除业务数据 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByCalleeUAIndex() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByCalleeUAIndex() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllServiceTaskByCallerID
 功能描述  : 根据主叫ID停止呼叫任务
 输入参数  : char* caller_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月25日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StopAllServiceTaskByCallerID(char* caller_id)
{
    int i = 0;
    int iRet = 0;
    int index = 0;
    int cr_pos = -1;
    int other_cr_pos = -1;
    cr_t* pCrData = NULL;
    cr_t* pOtherCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if (NULL == caller_id)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByCallerID() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByCallerID() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    /* 根据客户端信息 查找呼叫记录信息 */
    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || ('\0' == pCrData->caller_id[0])
            || ('\0' == pCrData->caller_ip[0])
            || (pCrData->caller_port <= 0))
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if (0 == sstrcmp(pCrData->caller_id, caller_id))
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByCallerID() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video requester log off or timeout, trigger off closing real-time monitoring:requester ID=%s, requester IP address=%s, logic device =%s, IP address=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "视频请求方注销登录或者超时, 触发关闭其实时视频业务:请求方ID=%s, 请求方IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerID() Enter------------: cr_pos=%d, caller_id=%s, callee_id=%s \r\n", cr_pos, pCrData->caller_id, pCrData->callee_id);

            /* 通知TSU停止接收码流*/
            if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
            {
                i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByCallerID() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerID() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }
            else
            {
                i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByCallerID() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerID() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }

            /* 发送Bye给主叫侧 */
            if (pCrData->caller_ua_index >= 0)
            {
                /*发送Bye 给主叫侧 */
                i = SIP_SendBye(pCrData->caller_ua_index);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerID() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
            }

            /* 看是否有前端连接 */
            if (pCrData->callee_ua_index >= 0)
            {
                /* 查看是否有其他客户端业务 */
                other_cr_pos = is_GBLogic_device_has_other_service(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerID() is_GBLogic_device_has_other_service:other_cr_pos=%d \r\n", other_cr_pos);

                if (other_cr_pos < 0) /* 没有其他业务 */
                {
                    /*发送Bye 给被叫侧 */
                    i = SIP_SendBye(pCrData->callee_ua_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerID() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
                }
                else
                {
                    pOtherCrData = call_record_get(other_cr_pos);

                    if (NULL != pOtherCrData)
                    {
                        pOtherCrData->callee_ua_index = pCrData->callee_ua_index; /* 将前端的会话句柄拷贝到下个业务 */
                        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerID() callee_ua_index=%d copy from %d to %d \r\n", pOtherCrData->callee_ua_index, cr_pos, other_cr_pos);
                    }
                }
            }

            /* 移除呼叫记录信息 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByCallerID() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByCallerID() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllServiceTaskByCallerIDForTVWall
 功能描述  : 根据电视墙通道关闭电视墙业务
 输入参数  : char* caller_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年2月25日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StopAllServiceTaskByCallerIDForTVWall(char* caller_id)
{
    int i = 0;
    int iRet = 0;
    int index = 0;
    int cr_pos = -1;
    int other_cr_pos = -1;
    cr_t* pCrData = NULL;
    cr_t* pOtherCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if (NULL == caller_id)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByCallerIDForTVWall() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByCallerIDForTVWall() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    /* 根据客户端信息 查找呼叫记录信息 */
    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || ('\0' == pCrData->caller_id[0])
            || ('\0' == pCrData->caller_ip[0])
            || (pCrData->caller_port <= 0))
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if (0 == sstrcmp(pCrData->caller_id, caller_id))
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByCallerIDForTVWall() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "TV Wall Service, trigger off closing real-time monitoring:requester ID=%s, requester IP address=%s, logic device =%s, IP address=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "电视墙业务, 关闭实时视频:请求方ID=%s, 请求方IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerIDForTVWall() Enter------------: cr_pos=%d, caller_id=%s, callee_id=%s \r\n", cr_pos, pCrData->caller_id, pCrData->callee_id);

            /* 通知TSU停止接收码流*/
            if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
            {
                i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByCallerIDForTVWall() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerIDForTVWall() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }
            else
            {
                i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByCallerIDForTVWall() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerIDForTVWall() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }

            /* 发送Bye给主叫侧 */
            if (pCrData->caller_ua_index >= 0)
            {
                /*发送Bye 给主叫侧 */
                i = SIP_SendBye(pCrData->caller_ua_index);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerIDForTVWall() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
            }

            /* 看是否有前端连接 */
            if (pCrData->callee_ua_index >= 0)
            {
                /* 查看是否有其他客户端业务 */
                other_cr_pos = is_GBLogic_device_has_other_service(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerIDForTVWall() is_GBLogic_device_has_other_service:other_cr_pos=%d \r\n", other_cr_pos);

                if (other_cr_pos < 0) /* 没有其他业务 */
                {
                    /*发送Bye 给被叫侧 */
                    i = SIP_SendBye(pCrData->callee_ua_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerIDForTVWall() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
                }
                else
                {
                    pOtherCrData = call_record_get(other_cr_pos);

                    if (NULL != pOtherCrData)
                    {
                        pOtherCrData->callee_ua_index = pCrData->callee_ua_index; /* 将前端的会话句柄拷贝到下个业务 */
                        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerIDForTVWall() callee_ua_index=%d copy from %d to %d \r\n", pOtherCrData->callee_ua_index, cr_pos, other_cr_pos);
                    }
                }
            }

            /* 移除呼叫记录信息 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByCallerIDForTVWall() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByCallerIDForTVWall() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllServiceTaskByCallerIPAndPort
 功能描述  : 根据主叫IP和端口停止呼叫任务
 输入参数  : char* caller_ip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月6日 星期日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StopAllServiceTaskByCallerIPAndPort(char* caller_ip, int caller_port)
{
    int i = 0;
    int iRet = 0;
    int index = 0;
    int cr_pos = -1;
    int other_cr_pos = -1;
    cr_t* pCrData = NULL;
    cr_t* pOtherCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if (NULL == caller_ip || caller_port <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByCallerIPAndPort() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByCallerIPAndPort() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    /* 根据客户端信息 查找呼叫记录信息 */
    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || ('\0' == pCrData->caller_id[0])
            || ('\0' == pCrData->caller_ip[0])
            || (pCrData->caller_port <= 0))
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if (0 == sstrcmp(pCrData->caller_ip, caller_ip) && pCrData->caller_port == caller_port)
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByCallerIPAndPort() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Video requester restart or registration timeout, trigger off closing real-time monitoring:calling side ID=%s, calling side IP address=%s, logic device ID=%s, IP address=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "视频请求方重启或者注册超时, 触发关闭其实时视频业务:主叫侧ID=%s, 主叫侧IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerIPAndPort() Enter------------: cr_pos=%d, caller_id=%s, callee_id=%s \r\n", cr_pos, pCrData->caller_id, pCrData->callee_id);

            /* 通知TSU停止接收码流*/
            if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
            {
                i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByCallerIPAndPort() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerIPAndPort() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }
            else
            {
                i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByCallerIPAndPort() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerIPAndPort() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }

            /* 发送Bye给主叫侧 */
            if (pCrData->caller_ua_index >= 0)
            {
                /*发送Bye 给主叫侧 */
                i = SIP_SendBye(pCrData->caller_ua_index);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerIPAndPort() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
            }

            /* 看是否有前端连接 */
            if (pCrData->callee_ua_index >= 0)
            {
                /* 查看是否有其他客户端业务 */
                other_cr_pos = is_GBLogic_device_has_other_service(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerIPAndPort() is_GBLogic_device_has_other_service:other_cr_pos=%d \r\n", other_cr_pos);

                if (other_cr_pos < 0) /* 没有其他业务 */
                {
                    /*发送Bye 给被叫侧 */
                    i = SIP_SendBye(pCrData->callee_ua_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerIPAndPort() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
                }
                else
                {
                    pOtherCrData = call_record_get(other_cr_pos);

                    if (NULL != pOtherCrData)
                    {
                        pOtherCrData->callee_ua_index = pCrData->callee_ua_index; /* 将前端的会话句柄拷贝到下个业务 */
                        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCallerIPAndPort() callee_ua_index=%d copy from %d to %d \r\n", pOtherCrData->callee_ua_index, cr_pos, other_cr_pos);
                    }
                }
            }

            /* 移除呼叫记录信息 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByCallerIPAndPort() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByCallerIPAndPort() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllServiceTaskByCalleeIPAndPort
 功能描述  :  根据被叫IP和端口号停止呼叫任务
 输入参数  : char* callee_ip
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月6日 星期日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StopAllServiceTaskByCalleeIPAndPort(char* callee_ip, int callee_port)
{
    //int i = 0;
    int iRet = 0;
    int index = 0;
    int cr_pos = -1;
    //int other_cr_pos = -1;
    cr_t* pCrData = NULL;
    //cr_t* pOtherCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if (NULL == callee_ip || callee_port <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByCalleeIPAndPort() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByCalleeIPAndPort() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    /* 根据客户端信息 查找呼叫记录信息 */
    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || ('\0' == pCrData->caller_id[0])
            || ('\0' == pCrData->caller_ip[0])
            || (pCrData->caller_port <= 0))
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if (0 == sstrcmp(pCrData->callee_ip, callee_ip) && pCrData->callee_port == callee_port)
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByCalleeIPAndPort() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Vidoe data stream sender restart or registration timeout, trigger off closing real-time monitoring:calling side ID=%s, calling side IP address=%s, logic device ID=%s, IP address=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "视频流发送方重启或者注册超时, 触发关闭其实时视频业务:主叫侧ID=%s, 主叫侧IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCalleeIPAndPort() Enter------------: cr_pos=%d, caller_id=%s, callee_id=%s \r\n", cr_pos, pCrData->caller_id, pCrData->callee_id);

            if (CALL_TYPE_RECORD == pCrData->call_type)
            {
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Vidoe data stream sender restart or registration timeout, trigger off closing video monitoring:logic device ID=%s, IP address=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "视频流发送方重启或者注册超时, 触发关闭录像业务:逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);

                /* 移除录像信息 */
                iRet = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByCalleeIPAndPort() RemoveDeviceRecordInfo Error:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCalleeIPAndPort() RemoveDeviceRecordInfo OK:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
                }
            }
            else
            {
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Vidoe data stream sender restart or registration timeout, trigger off closing video tasks:user ID=%s, user IP address=%s, logic device ID=%s, IP address =%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "视频流发送方重启或者注册超时, 触发关闭视频业务:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
            }

            /* 发送Bye 给所有其他业务主叫侧用户*/
            iRet = send_bye_to_all_other_caller_by_callee_id(pCrData->callee_id, cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByCalleeIPAndPort() send_bye_to_all_other_caller_by_callee_id Error:callee_id=%s, cr_pos=%d, i=%d \r\n", pCrData->callee_id, cr_pos, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCalleeIPAndPort() send_bye_to_all_other_caller_by_callee_id OK:callee_id=%s, cr_pos=%d, i=%d \r\n", pCrData->callee_id, cr_pos, iRet);
            }

            /* 停止业务 */
            iRet = StopCallService(pCrData);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByCalleeIPAndPort() StopCallService Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByCalleeIPAndPort() StopCallService OK:cr_pos=%d \r\n", cr_pos);
            }

            /* 移除呼叫记录信息 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByCalleeIPAndPort() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByCalleeIPAndPort() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllServiceTaskByUserInfo
 功能描述  : 查找客户端的业务信息并停止
 输入参数  : char* user_id
                            char* login_ip
                            int login_port
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月17日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int StopAllServiceTaskByUserInfo(char* user_id, char* login_ip, int login_port)
{
    int i = 0;
    int iRet = 0;
    int index = 0;
    int cr_pos = -1;
    int other_cr_pos = -1;
    cr_t* pCrData = NULL;
    cr_t* pOtherCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;

    if ((NULL == user_id) || (NULL == login_ip) || (login_port <= 0))
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByUserInfo() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "StopAllServiceTaskByUserInfo() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    /* 根据客户端信息 查找呼叫记录信息 */
    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || ('\0' == pCrData->caller_id[0])
            || ('\0' == pCrData->caller_ip[0])
            || (pCrData->caller_port <= 0))
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        if ((0 == sstrcmp(pCrData->caller_id, user_id))
            && (0 == sstrcmp(pCrData->caller_ip, login_ip))
            && (pCrData->caller_port == login_port))
        {
            pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

    if (CRIndexVector.size() > 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByUserInfo() CRIndexVector.size()=%d \r\n", (int)CRIndexVector.size());

        for (index = 0; index < (int)CRIndexVector.size(); index++)
        {
            cr_pos = CRIndexVector[index];

            pCrData = call_record_get(cr_pos);

            if (NULL == pCrData)
            {
                continue;
            }

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "User log off or timeout, trigger off closing real-time monitoring:user ID=%s, user IP address=%s, logic device ID=%s, IP address=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "用户注销登录或者超时, 触发关闭其实时视频业务:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);

            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByUserInfo() Enter------------: cr_pos=%d \r\n", cr_pos);

            /* 通知TSU停止接收码流*/
            if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
            {
                i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByUserInfo() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByUserInfo() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }
            else
            {
                i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopAllServiceTaskByUserInfo() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByUserInfo() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                }
            }

            /* 发送Bye给主叫侧 */
            if (pCrData->caller_ua_index >= 0)
            {
                /*发送Bye 给主叫侧 */
                i = SIP_SendBye(pCrData->caller_ua_index);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByUserInfo() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
            }

            /* 看是否有前端连接 */
            if (pCrData->callee_ua_index >= 0)
            {
                /* 查看是否有其他客户端业务 */
                other_cr_pos = is_GBLogic_device_has_other_service(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByUserInfo() is_GBLogic_device_has_other_service:other_cr_pos=%d \r\n", other_cr_pos);

                if (other_cr_pos < 0) /* 没有其他业务 */
                {
                    /*发送Bye 给被叫侧 */
                    i = SIP_SendBye(pCrData->callee_ua_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByUserInfo() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
                }
                else
                {
                    pOtherCrData = call_record_get(other_cr_pos);

                    if (NULL != pOtherCrData)
                    {
                        pOtherCrData->callee_ua_index = pCrData->callee_ua_index; /* 将前端的会话句柄拷贝到下个业务 */
                        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskByUserInfo() callee_ua_index=%d copy from %d to %d \r\n", pOtherCrData->callee_ua_index, cr_pos, other_cr_pos);
                    }
                }
            }

            /* 移除呼叫记录信息 */
            iRet = call_record_remove(cr_pos);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskByUserInfo() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopAllServiceTaskByUserInfo() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllServiceTask
 功能描述  : 停止所有呼叫任务
 输入参数  : int sock
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月24日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int StopAllServiceTask(int sock)
{
    int iRet = 0;
    int index = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;
    char rbuf[128] = {0};

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed) || ('\0' == pCrData->tsu_device_id[0]))
        {
            continue;
        }

        if (CALL_TYPE_NULL == pCrData->call_type)
        {
            continue;
        }

        if (CALL_STATUS_NULL != pCrData->call_status)
        {
            continue;
        }

        pCrData->call_status = CALL_STATUS_WAIT_RELEASE;
        CRIndexVector.push_back(Itr->first);
    }

    CR_SMUTEX_UNLOCK();

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

            if (CALL_STATUS_WAIT_RELEASE != pCrData->call_status)
            {
                continue;
            }

            if (CALL_TYPE_RECORD == pCrData->call_type)
            {
                /* 移除录像信息 */
                iRet = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
            }

            /* 停止业务 */
            iRet = StopCallService(pCrData);

            /* 移除业务数据 */
            iRet = call_record_remove(cr_pos);

            if (sock > 0)
            {
                memset(rbuf, 0, 128);

                if (0 == iRet)
                {
                    snprintf(rbuf, 128, "\r停止呼叫任务成功: 呼叫任务索引=%d\r\n$", cr_pos);
                    send(sock, rbuf, strlen(rbuf), 0);
                }
                else
                {
                    snprintf(rbuf, 128, "\r停止呼叫任务失败: 呼叫任务索引=%d\r\n$", cr_pos);
                    send(sock, rbuf, strlen(rbuf), 0);
                }
            }
        }
    }

    CRIndexVector.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllServiceTaskWhenExit
 功能描述  : 停止所有呼叫任务
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月24日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int StopAllServiceTaskWhenExit()
{
    int iRet = 0;
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;

    if (g_CallRecordMap.size() <= 0)
    {
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if (CALL_TYPE_NULL == pCrData->call_type)
        {
            continue;
        }

        pCrData->call_status = CALL_STATUS_WAIT_RELEASE;

        if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* 移除录像信息 */
            iRet = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskWhenExit() RemoveDeviceRecordInfo Error:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskWhenExit() RemoveDeviceRecordInfo OK:callee_id=%s,callee_stream_type=%d,tsu_resource_index=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
            }

        }

        /* 停止业务 */
        iRet = StopCallService(pCrData);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopAllServiceTaskWhenExit() StopCallService Error:cr_pos=%d \r\n", Itr->first);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskWhenExit() StopCallService OK:cr_pos=%d \r\n", Itr->first);
        }

        /* 移除业务数据 */
        call_record_free(pCrData);

        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopAllServiceTaskWhenExit() Call Record Index=%d \r\n", Itr->first);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : StopRecordPlayServiceByTaskID
 功能描述  : TSU上报任务播放完成,停止任务
 输入参数  : char* pcTsuID
                            int iTaskType
                            char* pcTaskID
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年11月13日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int StopRecordPlayServiceByTaskID(char* pcTsuID, int iTaskType, char* pcTaskID)
{
    int iRet = 0;
    int cr_pos = -1;
    cr_t* pCrData = NULL;

    //type = 1 //录像
    //type = 2 //转发
    //type = 3 //回放或者下载

    if (3 != iTaskType)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "StopRecordPlayServiceByTaskID() exit---: TSU Task Attribute Type Error \r\n");
        return -1;
    }

    if (NULL == pcTaskID)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopRecordPlayServiceByTaskID() exit---: TSU Task Attribute ID Error \r\n");
        return -1;
    }

    /* 根据TSU的索引和转发索引查找任务记录 */
    cr_pos = call_record_find_by_task_id(pcTaskID);

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopRecordPlayServiceByTaskID() exit---: Find Call Record Error:TaskID=%s \r\n", pcTaskID);
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopRecordPlayServiceByTaskID() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    iRet = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "TSU notifies that play-back task is over, trigger off closing play-back tasks:user ID=%s, user IP address=%s, logic ID=%s, IP address=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "TSU通知回放任务结束, 触发关闭回放视频业务:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);

    if (pCrData->caller_ua_index >= 0)
    {
        /* 发送Bye 给主叫 */
        iRet = SIP_SendBye(pCrData->caller_ua_index);
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopRecordPlayServiceByTaskID() SIP_SendBye To Caller:caller_ua_index=%d, iRet=%d \r\n", pCrData->caller_ua_index, iRet);
    }

    if (pCrData->callee_ua_index >= 0)
    {
        /* 发送Bye 给被叫 */
        iRet = SIP_SendBye(pCrData->callee_ua_index);
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "StopRecordPlayServiceByTaskID() SIP_SendBye To Callee:callee_ua_index=%d, iRet=%d \r\n", pCrData->caller_ua_index, iRet);
    }

    /* 移除业务数据 */
    iRet = call_record_remove(cr_pos);

    if (0 != iRet)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "StopRecordPlayServiceByTaskID() call_record_remove Error:cr_pos=%d \r\n", cr_pos);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "StopRecordPlayServiceByTaskID() call_record_remove OK:cr_pos=%d \r\n", cr_pos);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : SendInfoToCalleeByTaskID
 功能描述  : 根据TSU的任务ID发送Info消息给前端
 输入参数  : char* pcTsuID
                            int iMsgType
                            char* pcTaskID
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年5月5日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int SendInfoToCalleeByTaskID(char* pcTsuID, int iMsgType, char* pcTaskID)
{
    int iRet = 0;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    char tmpBuf[128] = {0};

    return 0;

    //iMsgType = 1 //暂停
    //iMsgType = 2 //恢复

    if (NULL == pcTaskID)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "SendInfoToCalleeByTaskID() exit---: TSU Task Attribute ID Error \r\n");
        return -1;
    }

    /* 根据TSU的索引和转发索引查找任务记录 */
    cr_pos = call_record_find_by_task_id(pcTaskID);

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SendInfoToCalleeByTaskID() exit---: Find Call Record Error:TaskID=%s \r\n", pcTaskID);
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SendInfoToCalleeByTaskID() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    if (pCrData->callee_ua_index < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SendInfoToCalleeByTaskID() exit---: Callee Ua Index Error:callee_ua_index=%d \r\n", pCrData->callee_ua_index);
        return -1;
    }

    if (1 == iMsgType)
    {
        if (pCrData->iScale != 1.0)
        {
            snprintf(tmpBuf, 128, (char*)"PAUSE RTSP/1.0\r\nCSeq: 5\r\nScale: %f\r\nPauseTime: now\r\n", pCrData->iScale);
        }
        else
        {
            snprintf(tmpBuf, 128, (char*)"PAUSE RTSP/1.0\r\nCSeq: 5\r\nPauseTime: now\r\n");
        }

        iRet = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, tmpBuf, strlen(tmpBuf));

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SendInfoToCalleeByTaskID() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iMsgType=%d, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, iMsgType, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "SendInfoToCalleeByTaskID() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iMsgType=%d, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, iMsgType, iRet);
        }
    }
    else if (2 == iMsgType)
    {
        if (pCrData->iScale != 1.0)
        {
            snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: 5\r\nScale: %f\r\nRange: npt=now-\r\n", pCrData->iScale);
        }
        else
        {
            snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: 5\r\nRange: npt=now-\r\n");
        }

        iRet = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, tmpBuf, strlen(tmpBuf));

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SendInfoToCalleeByTaskID() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iMsgType=%d, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, iMsgType, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "SendInfoToCalleeByTaskID() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iMsgType=%d, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, iMsgType, iRet);
        }
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "SendInfoToCalleeByTaskID() exit---: Message Type Error:iMsgType=%d \r\n", iMsgType);
        return -1;
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : StopCallTask
 功能描述  : 停止呼叫任务
 输入参数  : int sock
                            int cr_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月31日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int StopCallTask(int sock, int cr_index)
{
    int iRet = 0;
    char rbuf[128] = {0};
    cr_t* pCrData = NULL;

    if (cr_index < 0)
    {
        if (sock > 0)
        {
            memset(rbuf, 0, 128);
            snprintf(rbuf, 128, "\r输入的呼叫任务索引错误: 呼叫任务索引=%d\r\n$", cr_index);
            send(sock, rbuf, strlen(rbuf), 0);
        }

        return -1;
    }

    pCrData = call_record_get(cr_index);

    if (pCrData == NULL)
    {
        if (sock > 0)
        {
            memset(rbuf, 0, 128);
            snprintf(rbuf, 128, "\r要停止的呼叫任务不存在: 呼叫任务索引=%d\r\n$", cr_index);
            send(sock, rbuf, strlen(rbuf), 0);
        }

        return -1;
    }

    iRet = call_record_set_call_status(cr_index, CALL_STATUS_WAIT_RELEASE);

    if (CALL_TYPE_RECORD == pCrData->call_type)
    {
        /* 移除录像信息 */
        iRet = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
    }

    /* 停止业务 */
    iRet = StopCallService(pCrData);

    /* 移除业务数据 */
    iRet = call_record_remove(cr_index);

    if (sock > 0)
    {
        memset(rbuf, 0, 128);

        if (0 == iRet)
        {
            snprintf(rbuf, 128, "\r停止呼叫任务成功: 呼叫任务索引=%d\r\n$", cr_index);
            send(sock, rbuf, strlen(rbuf), 0);
        }
        else
        {
            snprintf(rbuf, 128, "\r停止呼叫任务失败: 呼叫任务索引=%d\r\n$", cr_index);
            send(sock, rbuf, strlen(rbuf), 0);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : resumed_wait_answer_call_record1
 功能描述  : 恢复等待的呼叫任务
 输入参数  : cr_t* pCalleeCrData
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年12月22日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int resumed_wait_answer_call_record1(cr_t* pCalleeCrData)
{
    int i = 0;
    int index = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;
    char strErrorCode[32] = {0};

    if (NULL == pCalleeCrData)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "resumed_wait_answer_call_record1() exit---: Param Error \r\n");
        return -1;
    }

    osip_usleep(500000); /* 延时500毫秒，防止TSU返回失败 */

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_callerid_and_calleeid() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_AUDIO == pCrData->call_type)
            || ('\0' == pCrData->caller_id[0])
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if (CALL_STATUS_WAIT_ANSWER != pCrData->call_status)
        {
            continue;
        }

        if (pCalleeCrData->callee_stream_type == pCrData->callee_stream_type
            && (0 == sstrcmp(pCrData->callee_id, pCalleeCrData->callee_id)))
        {
            pCrData->callee_service_type = 0;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

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

            if (CALL_STATUS_WAIT_ANSWER != pCrData->call_status)
            {
                continue;
            }

            i = user_answer_invite_for_realplay_proc(pCrData, pCalleeCrData, pCrData->callee_service_type, pCalleeCrData->callee_stream_type);

            if (0 != i)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "恢复用户实时视频业务失败, 关闭用户实时视频业务:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);

                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", i);
                SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);

                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "resumed_wait_answer_call_record1() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "resumed_wait_answer_call_record1() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
                }
            }
        }
    }

    CRIndexVector.clear();
    return 0;
}

/*****************************************************************************
 函 数 名  : resumed_wait_answer_call_record2
 功能描述  : 恢复等待的呼叫任务
 输入参数  : cr_t* pCalleeCrData
             int record_type
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年12月22日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int resumed_wait_answer_call_record2(cr_t* pCalleeCrData, int record_type)
{
    int i = 0;
    int index = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;
    char strErrorCode[32] = {0};

    if (NULL == pCalleeCrData)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "resumed_wait_answer_call_record2() exit---: Param Error \r\n");
        return -1;
    }

    osip_usleep(500000); /* 延时500毫秒，防止TSU返回失败 */

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_callerid_and_calleeid() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_AUDIO == pCrData->call_type)
            || ('\0' == pCrData->caller_id[0])
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if (CALL_STATUS_WAIT_ANSWER != pCrData->call_status)
        {
            continue;
        }

        if (pCalleeCrData->callee_stream_type == pCrData->callee_stream_type
            && (0 == sstrcmp(pCrData->callee_id, pCalleeCrData->callee_id)))
        {
            pCrData->callee_service_type = 1;
            pCrData->callee_record_type = record_type;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

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

            if (CALL_STATUS_WAIT_ANSWER != pCrData->call_status)
            {
                continue;
            }

            i = user_answer_invite_for_realplay_proc(pCrData, pCalleeCrData, pCrData->callee_service_type, pCalleeCrData->callee_stream_type);
            printf("resumed_wait_answer_call_record2() user_answer_invite_for_realplay_proc:i=%d \r\n", i);

            if (0 != i)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "恢复用户实时视频业务失败, 关闭用户实时视频业务:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos);

                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", i);
                SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);

                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "resumed_wait_answer_call_record2() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "resumed_wait_answer_call_record2() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
                }
            }
        }
    }

    CRIndexVector.clear();
    return 0;
}

/*****************************************************************************
 函 数 名  : return_error_for_wait_answer_call_record
 功能描述  : 返回错误给正在等待的呼叫任务
 输入参数  : cr_t* pCalleeCrData
             int iErroCode
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年12月22日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int return_error_for_wait_answer_call_record(cr_t* pCalleeCrData, int iErroCode)
{
    int i = 0;
    int index = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;
    vector<int> CRIndexVector;
    char strErrorCode[32] = {0};

    if (NULL == pCalleeCrData)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "resumed_wait_answer_call_record1() exit---: Param Error \r\n");
        return -1;
    }

    CRIndexVector.clear();

    CR_SMUTEX_LOCK();

    if (g_CallRecordMap.size() <= 0)
    {
        CR_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "call_record_find_by_callerid_and_calleeid() exit---: Call Record Srv Map NULL \r\n");
        return -1;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            continue;
        }

        if ((CALL_TYPE_NULL == pCrData->call_type)
            || (CALL_TYPE_AUDIO == pCrData->call_type)
            || ('\0' == pCrData->caller_id[0])
            || ('\0' == pCrData->callee_id[0]))
        {
            continue;
        }

        if (CALL_STATUS_WAIT_ANSWER != pCrData->call_status)
        {
            continue;
        }

        if (pCalleeCrData->callee_stream_type == pCrData->callee_stream_type
            && (0 == sstrcmp(pCrData->callee_id, pCalleeCrData->callee_id)))
        {
            pCrData->callee_service_type = 0;
            CRIndexVector.push_back(Itr->first);
        }
    }

    CR_SMUTEX_UNLOCK();

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

            if (CALL_STATUS_WAIT_ANSWER != pCrData->call_status)
            {
                continue;
            }

            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", iErroCode);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "用户实时视频业务失败, 返回错误给用户实时视频业务:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s, cr_pos=%d, ErroCode=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip, cr_pos, iErroCode);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "return_error_for_wait_answer_call_record() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "return_error_for_wait_answer_call_record() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
        }
    }

    CRIndexVector.clear();
    return 0;
}

int CrDateIPAddrListClone(const osip_list_t* src, osip_list_t* dst)
{
    int i = 0;

    if (NULL == src)
    {
        return -1;
    }

    CR_SMUTEX_LOCK();

    ip_addr_list_clone(src, dst);

    CR_SMUTEX_UNLOCK();
    return i;
}

#if DECS("电视墙业务")
/*****************************************************************************
 函 数 名  : start_connect_tv_proc
 功能描述  : 启动连接DC解码器
 输入参数  : char* device_id
             int stream_type
             char* dc_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月14日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int start_connect_tv_proc(char* device_id, int stream_type, char* dc_id, int dc_media_port)
{
    int i = 0;
    int cr_pos = -1;
    GBLogicDevice_info_t* pSourceGBLogicDeviceInfo = NULL;
    GBLogicDevice_info_t* pDCGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pDCGBDeviceInfo = NULL;

    int iSourceRoutePos = 0;
    route_info_t* pSourceRouteInfo = NULL;
    GBDevice_info_t* pSourceGBDeviceInfo = NULL;

    int device_stream_type = 0;

    if (NULL == device_id || NULL == dc_id)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "start_connect_tv_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (device_id[0] == '\0' || dc_id[0] == '\0')
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "start_connect_tv_proc() exit---: device_id or dc_id Error: device_id=%s, dc_id=%s \r\n", device_id, dc_id);
        return -1;
    }

    if ((device_id[0] == '0' && strlen(device_id) == 1) || (dc_id[0] == '0' && strlen(dc_id) == 1)) /* 随机生成ID第一位可能为0 */
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "start_connect_tv_proc() exit---: device_id or dc_id Error: device_id=%s, dc_id=%s \r\n", device_id, dc_id);
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "启动电视墙业务:逻辑设备ID=%s, 电视墙通道ID=%s", device_id, dc_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Launch the TV wall:logic device ID=%s, TV wall channel ID=%s", device_id, dc_id);

    if (!g_IsPay) /* 没有付费，返回失败 */
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "start_connect_tv_proc() exit---: Get Pay Info Error \r\n");
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause =%s", device_id, dc_id, (char*)"Failed to obtain authorization information.");
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", device_id, dc_id, (char*)"获取授权信息失败");
        return -1;
    }

    /* 1、查找源的逻辑设备信息 */
    pSourceGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

    if (NULL == pSourceGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() exit---: Get Source GBlogicDevice Info Error:device_id=%s \r\n", device_id);
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause =%s", device_id, dc_id, (char*)"Failed to find information on the front-end logic device.");
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", device_id, dc_id, (char*)"没有找到前端逻辑设备信息");
        return -1;
    }

    /* 查看源逻辑设备状态 */
    if (0 == pSourceGBLogicDeviceInfo->status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() exit---: Source GBlogic Device Not Online \r\n");
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", device_id, dc_id, (char*)"前端逻辑设备不在线");
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause =%s", device_id, dc_id, (char*)"Front-end logic device is offline.");
        return -1;
    }

#if 0
    else if (2 == pSourceGBLogicDeviceInfo->status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "start_connect_tv_proc() exit---: Source GBlogic Device No Stream \r\n");
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", device_id, dc_id, (char*)"前端逻辑设备没有码流");

        return -1;
    }

#endif
    else if (3 == pSourceGBLogicDeviceInfo->status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() exit---: Source GBlogic Device NetWork UnReached \r\n");
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause =%s", device_id, dc_id, (char*)"The logic device network is unaccessible.");
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", device_id, dc_id, (char*)"前端逻辑设备网络不可达");
        return -1;
    }

    /* 2、查找DC 逻辑设备信息 */
    pDCGBLogicDeviceInfo = GBLogicDevice_info_find(dc_id);

    if (NULL == pDCGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() exit---: Get DEC GBlogicDevice Info Error:dc_id=%s \r\n", dc_id);
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", device_id, dc_id, (char*)"没有找到电视墙通道逻辑设备信息");
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", device_id, dc_id, (char*)"Did not found TV wall channel physical device information.");
        return -1;
    }

    /* 查看电视墙逻辑设备状态 */
    if (0 == pDCGBLogicDeviceInfo->status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() exit---: DEC GBlogic Device Not Online \r\n");
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", device_id, dc_id, (char*)"电视墙逻辑通道不在线");
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic deviceID=%s, TV wall channel ID=%s, cause=%s", device_id, dc_id, (char*)"TV wall logic channel is not online.");
        return -1;
    }

    /* 查找对应的DC 物理设备 */
    pDCGBDeviceInfo = GBDevice_info_get_by_stream_type(pDCGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);;

    if (NULL == pDCGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() exit---: Get DEC GBDevice Info Error \r\n");
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", device_id, dc_id, (char*)"没有找到电视墙物理设备信息");
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic deviceID=%s, TV wall channel ID=%s, cause=%s", device_id, dc_id, (char*)"Did not found TV wall physical device information.");
        return -1;
    }

    /* 查看DC 物理设备 类型*/
    if (EV9000_DEVICETYPE_DECODER != pDCGBDeviceInfo->device_type)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() exit---: DEC GBDevice Type Error:device_type=%d \r\n", pDCGBDeviceInfo->device_type);
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", device_id, dc_id, (char*)"电视墙物理设备类型不匹配");
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", device_id, dc_id, (char*)"TV wall physical device is not match.");
        return -1;
    }

    /* 查看电视墙物理设备状态 */
    if (0 == pDCGBDeviceInfo->reg_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() exit---: DEC Device Not Online \r\n");
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", device_id, dc_id, (char*)"TV wall physical device is not online.");
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", device_id, dc_id, (char*)"电视墙物理设备不在线");
        return -1;
    }

    device_stream_type = stream_type;

    if (EV9000_STREAM_TYPE_SLAVE == stream_type && pSourceGBLogicDeviceInfo->stream_count == 1)
    {
        device_stream_type = EV9000_STREAM_TYPE_MASTER;

        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "start_connect_tv_proc() : Source GBDevice Support Multi stream, Change To Master Stream Type \r\n");
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务:逻辑设备ID=%s, 电视墙通道ID=%s, 媒体流类型=%d, 但是该点位不支持双码流，采用默认主流", device_id, dc_id, stream_type);
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Launch the TV wall:logic device ID=%s, TV wall channel ID=%s, Media stream type=%d, But this point does not support double stream, using default mainstream", device_id, dc_id, stream_type);
    }
    else if (EV9000_STREAM_TYPE_INTELLIGENCE == stream_type && INTELLIGENT_STATUS_NULL == pSourceGBLogicDeviceInfo->intelligent_status) /* 如果请求的是视频流，但是点位又没有进行分析，那么采用默认主流 */
    {
        device_stream_type = EV9000_STREAM_TYPE_MASTER;

        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "start_connect_tv_proc() : Source GBDevice Not Intelligence Now, Change To Master Stream Type \r\n");
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务:逻辑设备ID=%s, 电视墙通道ID=%s, 媒体流类型=%d, 但是该点位没有进行智能分析，采用默认主流", device_id, dc_id, stream_type);
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Launch the TV wall:logic device ID=%s, TV wall channel ID=%s, Media stream type=%d, But this point has not conducted intelligent video analysis, using default mainstream", device_id, dc_id, stream_type);
    }

    /* 根据逻辑设备的归属，确定请求方向 */
    if (1 == pSourceGBLogicDeviceInfo->other_realm)
    {
        /* 查找上级路由信息 */
        iSourceRoutePos = route_info_find(pSourceGBLogicDeviceInfo->cms_id);

        if (iSourceRoutePos < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() exit---: Get Source GBDevice Info Error:device_stream_type=%d \r\n", device_stream_type);
            SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s, 媒体流类型=%d", device_id, dc_id, (char*)"查找逻辑设备对应的上级路由信息失败", device_stream_type);
            EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s, Media stream type=%d", device_id, dc_id, (char*)"Find the route information which corresponding to source logical device failed.", device_stream_type);
            return -1;
        }

        pSourceRouteInfo = route_info_get(iSourceRoutePos);

        if (NULL == pSourceRouteInfo)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() exit---: Get Source GBDevice Info Error:device_stream_type=%d \r\n", device_stream_type);
            SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s, 媒体流类型=%d", device_id, dc_id, (char*)"获取逻辑设备对应的上级路由信息失败", device_stream_type);
            EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s, Media stream type=%d", device_id, dc_id, (char*)"Get the route information which corresponding to source logical device failed.", device_stream_type);
            return -1;
        }

        /* 查看上级路由状态 */
        if (0 == pSourceRouteInfo->reg_status)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() exit---: Source Device Not Online \r\n");
            SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", device_id, dc_id, (char*)"逻辑设备对应的上级路由不在线");
            EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", device_id, dc_id, (char*)"Route information which corresponding to source logical device is not online");
            return -1;
        }

        if (!g_DECMediaTransferFlag) /* 上级平台媒体流不经过本级TSU转发 */
        {
            if (dc_media_port <= 0)
            {
                /* 发送获取端口的消息 */
                i = SendQueryDECDeviceMediaPortMessage(pDCGBDeviceInfo, dc_id, device_id, device_stream_type);
            }
            else
            {
                /* 先查看解码器通道原来是否已经在播放，如果有视频在播放则先停止掉,因为解码器通道I同时只能解一路 */
                cr_pos = call_record_find_by_caller_id(dc_id);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "start_connect_tv_proc() call_record_find_by_caller_id:dc_id=%s, cr_pos=%d \r\n", dc_id, cr_pos);

                if (cr_pos >= 0)
                {
                    i = StopAllServiceTaskByCallerIDForTVWall(dc_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() StopAllServiceTaskByCallerID Error:dc_id=%s, i=%d \r\n", dc_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "start_connect_tv_proc() StopAllServiceTaskByCallerID OK:dc_id=%s, i=%d \r\n", dc_id, i);
                    }
                }

                i = start_connect_tv_route_proc(pSourceGBLogicDeviceInfo, pSourceRouteInfo, pDCGBLogicDeviceInfo, pDCGBDeviceInfo, device_stream_type, dc_media_port);
            }
        }
        else
        {
            /* 先查看解码器通道原来是否已经在播放，如果有视频在播放则先停止掉,因为解码器通道I同时只能解一路 */
            cr_pos = call_record_find_by_caller_id(dc_id);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "start_connect_tv_proc() call_record_find_by_caller_id:dc_id=%s, cr_pos=%d \r\n", dc_id, cr_pos);

            if (cr_pos >= 0)
            {
                i = StopAllServiceTaskByCallerIDForTVWall(dc_id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() StopAllServiceTaskByCallerID Error:dc_id=%s, i=%d \r\n", dc_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "start_connect_tv_proc() StopAllServiceTaskByCallerID OK:dc_id=%s, i=%d \r\n", dc_id, i);
                }
            }

            i = start_connect_tv_route_proc(pSourceGBLogicDeviceInfo, pSourceRouteInfo, pDCGBLogicDeviceInfo, pDCGBDeviceInfo, device_stream_type, dc_media_port);
        }
    }
    else
    {
        /* 查找对应的源物理设备 */
        pSourceGBDeviceInfo = GBDevice_info_get_by_stream_type(pSourceGBLogicDeviceInfo, device_stream_type);;

        if (NULL == pSourceGBDeviceInfo)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() exit---: Get Source GBDevice Info Error \r\n");
            SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", device_id, dc_id, (char*)"没有找到逻辑点位对应的物理设备信息");
            EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic deviceID=%s, TV wall channel ID=%s, cause=%s", device_id, dc_id, (char*)"Did not found Source physical device information.");
            return -1;
        }

        /* 查看源物理设备状态 */
        if (0 == pSourceGBDeviceInfo->reg_status)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_sub_proc() exit---: Source Device Not Online \r\n");
            SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", device_id, dc_id, (char*)"源物理设备不在线");
            EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", device_id, dc_id, (char*)"Source physical device is not online");
            return -1;
        }

        if (!g_DECMediaTransferFlag
            && EV9000_DEVICETYPE_SIPSERVER == pSourceGBDeviceInfo->device_type) /* 下级平台媒体流不经过本级TSU转发 */
        {
            if (dc_media_port <= 0)
            {
                /* 发送获取端口的消息 */
                i = SendQueryDECDeviceMediaPortMessage(pDCGBDeviceInfo, dc_id, device_id, device_stream_type);

                if (i != 0)
                {
                    return -1;
                }
                else
                {
                    return 999999;
                }
            }
            else
            {
                /* 先查看解码器通道原来是否已经在播放，如果有视频在播放则先停止掉,因为解码器通道I同时只能解一路 */
                cr_pos = call_record_find_by_caller_id(dc_id);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "start_connect_tv_proc() call_record_find_by_caller_id:dc_id=%s, cr_pos=%d \r\n", dc_id, cr_pos);

                if (cr_pos >= 0)
                {
                    i = StopAllServiceTaskByCallerIDForTVWall(dc_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() StopAllServiceTaskByCallerID Error:dc_id=%s, i=%d \r\n", dc_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "start_connect_tv_proc() StopAllServiceTaskByCallerID OK:dc_id=%s, i=%d \r\n", dc_id, i);
                    }
                }

                i = start_connect_tv_sub_proc(pSourceGBLogicDeviceInfo, pSourceGBDeviceInfo, pDCGBLogicDeviceInfo, pDCGBDeviceInfo, device_stream_type, dc_media_port);
            }
        }
        else
        {
            /* 先查看解码器通道原来是否已经在播放，如果有视频在播放则先停止掉,因为解码器通道I同时只能解一路 */
            cr_pos = call_record_find_by_caller_id(dc_id);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "start_connect_tv_proc() call_record_find_by_caller_id:dc_id=%s, cr_pos=%d \r\n", dc_id, cr_pos);

            if (cr_pos >= 0)
            {
                i = StopAllServiceTaskByCallerIDForTVWall(dc_id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_proc() StopAllServiceTaskByCallerID Error:dc_id=%s, i=%d \r\n", dc_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "start_connect_tv_proc() StopAllServiceTaskByCallerID OK:dc_id=%s, i=%d \r\n", dc_id, i);
                }
            }

            i = start_connect_tv_sub_proc(pSourceGBLogicDeviceInfo, pSourceGBDeviceInfo, pDCGBLogicDeviceInfo, pDCGBDeviceInfo, device_stream_type, dc_media_port);
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : start_connect_tv_sub_proc
 功能描述  : 启动连接DC解码器
 输入参数  : GBLogicDevice_info_t* pSourceGBLogicDeviceInfo
             int stream_type
             char* dc_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月14日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int start_connect_tv_sub_proc(GBLogicDevice_info_t* pSourceGBLogicDeviceInfo, GBDevice_info_t* pSourceGBDeviceInfo, GBLogicDevice_info_t* pDCGBLogicDeviceInfo, GBDevice_info_t* pDCGBDeviceInfo, int stream_type, int dc_media_port)
{
    int i = 0;
    int iRet = 0;
    int cr_pos = -1;
    cr_t* pCrData = NULL;

    int record_info_pos = -1;
    record_info_t* pRecordInfo = NULL;

    if (NULL == pSourceGBLogicDeviceInfo || NULL == pSourceGBDeviceInfo  || NULL == pDCGBLogicDeviceInfo || NULL == pDCGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "start_connect_tv_sub_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (!g_DECMediaTransferFlag
        && EV9000_DEVICETYPE_SIPSERVER == pSourceGBDeviceInfo->device_type && dc_media_port > 0) /* 下级平台媒体流不经过本级TSU转发 */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "电视墙业务:逻辑设备ID=%s, 电视墙通道ID=%s, 媒体流不经过本级转发, 不检查本级是否存在实时视频业务", pSourceGBLogicDeviceInfo->device_id, pDCGBLogicDeviceInfo->device_id);
    }
    else
    {
        /* 看是否已经启动录像 */
        if (0 == pSourceGBLogicDeviceInfo->record_type && 2 != pSourceGBLogicDeviceInfo->status)/* 本地录像并且不是前端没有码流的状态情况下检查录像信息，因为前端没有码流情况下不影响实时视频流程 */
        {
            /* 检查源点位是否录像了 */
            record_info_pos = record_info_find_by_stream_type(pSourceGBLogicDeviceInfo->id, stream_type);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "start_connect_tv_sub_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pSourceGBLogicDeviceInfo->device_id, stream_type, record_info_pos);

            if (record_info_pos >= 0)
            {
                pRecordInfo = record_info_get(record_info_pos);

                if (NULL != pRecordInfo)
                {
                    if (1 == pRecordInfo->record_enable)
                    {
                        if (pRecordInfo->record_cr_index < 0 && pRecordInfo->record_status != RECORD_STATUS_NO_TSU)
                        {
                            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_sub_proc() exit---: Source GBDevice Not Start Record \r\n");
                            SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pSourceGBLogicDeviceInfo->device_id, pDCGBLogicDeviceInfo->device_id, (char*)"逻辑设备配置了录像，但是还没有启动录像");
                            EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pSourceGBLogicDeviceInfo->device_id, pDCGBLogicDeviceInfo->device_id, (char*)"The logic device is configured for video, but it has not been started.");
                            return -1;
                        }
                        else if (pRecordInfo->record_cr_index >= 0 && pRecordInfo->record_status != RECORD_STATUS_COMPLETE)
                        {
                            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_sub_proc() exit---: Source GBDevice Record Not Complete \r\n");
                            SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pSourceGBLogicDeviceInfo->device_id, pDCGBLogicDeviceInfo->device_id, (char*)"逻辑设备录像流程没有结束");
                            EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pSourceGBLogicDeviceInfo->device_id, pDCGBLogicDeviceInfo->device_id, (char*)"Logical device video process is not over");
                            return -1;
                        }
                    }
                }
            }
        }
    }

    /* 申请呼叫资源 */
    cr_pos = call_record_add();
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "start_connect_tv_sub_proc() call_record_add:cr_pos=%d \r\n", cr_pos);

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_sub_proc() exit---: Get Call Record Data Error:cr_pos=%d \r\n", cr_pos);
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pSourceGBLogicDeviceInfo->device_id, pDCGBLogicDeviceInfo->device_id, (char*)"获取呼叫资源失败");
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pSourceGBLogicDeviceInfo->device_id, pDCGBLogicDeviceInfo->device_id, (char*)"Failed to get call resource.");
        return -1;
    }

    pCrData->call_type = CALL_TYPE_DC;

    /* 添加 主被叫信息到呼叫资源信息 */
    /* 主叫端信息 */
    osip_strncpy(pCrData->caller_id, pDCGBLogicDeviceInfo->device_id, MAX_ID_LEN);
    osip_strncpy(pCrData->caller_ip, pDCGBDeviceInfo->login_ip, MAX_IP_LEN);
    pCrData->caller_port = pDCGBDeviceInfo->login_port;

    if (!g_DECMediaTransferFlag
        && EV9000_DEVICETYPE_SIPSERVER == pSourceGBDeviceInfo->device_type && dc_media_port > 0) /* 下级平台媒体流不经过本级TSU转发 */
    {
        osip_strncpy(pCrData->caller_sdp_ip, pDCGBDeviceInfo->login_ip, MAX_IP_LEN);
        pCrData->caller_sdp_port = dc_media_port;
    }

    osip_strncpy(pCrData->caller_server_ip_ethname, pDCGBDeviceInfo->strRegServerEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->caller_server_ip, pDCGBDeviceInfo->strRegServerIP, MAX_IP_LEN);
    pCrData->caller_server_port = pDCGBDeviceInfo->iRegServerPort;

    if (1 == pDCGBDeviceInfo->trans_protocol)
    {
        pCrData->caller_transfer_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        pCrData->caller_transfer_type = TRANSFER_PROTOCOL_UDP; /* 默认UDP */
    }

    /* 被叫端信息 */
    osip_strncpy(pCrData->callee_id, pSourceGBLogicDeviceInfo->device_id, MAX_ID_LEN);
    osip_strncpy(pCrData->callee_ip, pSourceGBDeviceInfo->login_ip, MAX_IP_LEN);
    pCrData->callee_port = pSourceGBDeviceInfo->login_port;
    osip_strncpy(pCrData->callee_server_ip_ethname, pSourceGBDeviceInfo->strRegServerEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->callee_server_ip, pSourceGBDeviceInfo->strRegServerIP, MAX_IP_LEN);
    pCrData->callee_server_port = pSourceGBDeviceInfo->iRegServerPort;
    pCrData->callee_framerate = pSourceGBLogicDeviceInfo->frame_count;
    pCrData->callee_stream_type = stream_type;
    pCrData->callee_gb_device_type = pSourceGBDeviceInfo->device_type;

    if (1 == pSourceGBDeviceInfo->trans_protocol)
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_UDP; /* 默认UDP */
    }

    i = connect_tv_service_proc(pCrData);

    if (i < 0)
    {
        iRet = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        iRet = call_record_remove(cr_pos);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_sub_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "start_connect_tv_sub_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, iRet);
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : start_connect_tv_route_proc
 功能描述  : 启动连接DC解码器
 输入参数  : GBLogicDevice_info_t* pSourceGBLogicDeviceInfo
             int stream_type
             char* dc_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月14日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int start_connect_tv_route_proc(GBLogicDevice_info_t* pSourceGBLogicDeviceInfo, route_info_t* pSourceRouteInfo, GBLogicDevice_info_t* pDCGBLogicDeviceInfo, GBDevice_info_t* pDCGBDeviceInfo, int stream_type, int dc_media_port)
{
    int i = 0;
    int iRet = 0;
    int cr_pos = -1;
    cr_t* pCrData = NULL;

    if (NULL == pSourceGBLogicDeviceInfo || NULL == pSourceRouteInfo || NULL == pDCGBLogicDeviceInfo || NULL == pDCGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "start_connect_tv_route_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* 申请呼叫资源 */
    cr_pos = call_record_add();
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "start_connect_tv_route_proc() call_record_add:cr_pos=%d \r\n", cr_pos);

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_route_proc() exit---: Get Call Record Data Error:cr_pos=%d \r\n", cr_pos);
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pSourceGBLogicDeviceInfo->device_id, pDCGBLogicDeviceInfo->device_id, (char*)"获取呼叫资源失败");
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pSourceGBLogicDeviceInfo->device_id, pDCGBLogicDeviceInfo->device_id, (char*)"Failed to get call resource.");
        return -1;
    }

    pCrData->call_type = CALL_TYPE_DC;

    /* 添加 主被叫信息到呼叫资源信息 */
    /* 主叫端信息 */
    osip_strncpy(pCrData->caller_id, pDCGBLogicDeviceInfo->device_id, MAX_ID_LEN);
    osip_strncpy(pCrData->caller_ip, pDCGBDeviceInfo->login_ip, MAX_IP_LEN);
    pCrData->caller_port = pDCGBDeviceInfo->login_port;

    if (!g_DECMediaTransferFlag && dc_media_port > 0) /* 下级平台媒体流不经过本级TSU转发 */
    {
        osip_strncpy(pCrData->caller_sdp_ip, pDCGBDeviceInfo->login_ip, MAX_IP_LEN);
        pCrData->caller_sdp_port = dc_media_port;
    }

    osip_strncpy(pCrData->caller_server_ip_ethname, pDCGBDeviceInfo->strRegServerEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->caller_server_ip, pDCGBDeviceInfo->strRegServerIP, MAX_IP_LEN);
    pCrData->caller_server_port = pDCGBDeviceInfo->iRegServerPort;

    if (1 == pDCGBDeviceInfo->trans_protocol)
    {
        pCrData->caller_transfer_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        pCrData->caller_transfer_type = TRANSFER_PROTOCOL_UDP; /* 默认UDP */
    }

    /* 被叫端信息 */
    osip_strncpy(pCrData->callee_id, pSourceGBLogicDeviceInfo->device_id, MAX_ID_LEN);
    osip_strncpy(pCrData->callee_ip, pSourceRouteInfo->server_ip, MAX_IP_LEN);
    pCrData->callee_port = pSourceRouteInfo->server_port;
    osip_strncpy(pCrData->callee_server_ip_ethname, pSourceRouteInfo->strRegLocalEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->callee_server_ip, pSourceRouteInfo->strRegLocalIP, MAX_IP_LEN);
    pCrData->callee_server_port = pSourceRouteInfo->iRegLocalPort;
    pCrData->callee_framerate = pSourceGBLogicDeviceInfo->frame_count;
    pCrData->callee_stream_type = stream_type;
    pCrData->callee_gb_device_type = EV9000_DEVICETYPE_SIPSERVER;

    if (1 == pSourceRouteInfo->trans_protocol)
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_UDP; /* 默认UDP */
    }

    i = connect_tv_service_proc(pCrData);

    if (i < 0)
    {
        iRet = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        iRet = call_record_remove(cr_pos);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "start_connect_tv_route_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "start_connect_tv_route_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, iRet);
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : connect_tv_service_proc
 功能描述  : 连接解码器的处理
 输入参数  : cr_t* pCrData

 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月14日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int connect_tv_service_proc(cr_t* pCrData)
{
    int i = -1;
    int cr_pos = -1;
    int record_info_pos = -1;
    cr_t* pCalleeCrData = NULL;
    record_info_t* pRecordInfo = NULL;

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "connect_tv_service_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (!g_DECMediaTransferFlag
        && EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type && pCrData->caller_sdp_port > 0) /* 下级平台媒体流不经过本级TSU转发 */
    {
        /* 如果是跨级CMS的点位并且不需要本级TSU转发码流，直接转发消息 */
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "电视墙业务:逻辑设备ID=%s, 媒体流类型=%d, 电视墙通道ID=%s, 媒体流不经过本级转发", pCrData->callee_id, pCrData->callee_stream_type, pCrData->caller_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TV wall:Logical device ID = %s, media stream type = %d, TV wall, channel ID = %s, Media stream without forward at the corresponding level", pCrData->callee_id, pCrData->callee_stream_type, pCrData->caller_id);

        pCrData->callee_service_type = 0;

        i = cms_send_invite_to_callee_for_dc_proc(pCrData);
    }
    else
    {
        /* 查询是否有录像业务 */
        cr_pos = find_GBLogic_device_has_record_service(pCrData->callee_id, pCrData->callee_stream_type);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "connect_tv_service_proc() find_GBLogic_device_has_record_service:callee_id=%s, callee_stream_type=%d, cr_pos=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, cr_pos);

        if (cr_pos >= 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "电视墙业务:逻辑设备ID=%s, 媒体流类型=%d, 电视墙通道ID=%s, 已经存在录像业务", pCrData->callee_id, pCrData->callee_stream_type, pCrData->caller_id);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TV wall:Logical device ID = % s, media stream type = % d, TV wall, channel ID = % s existing video business", pCrData->callee_id, pCrData->callee_stream_type, pCrData->caller_id);

            pCalleeCrData = call_record_get(cr_pos);

            if (NULL == pCalleeCrData)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "connect_tv_service_proc() exit---: Get Callee Cr Data Error:cr_pos=%d \r\n", cr_pos);
                SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCrData->callee_id, pCrData->caller_id, (char*)"获取逻辑设备的已有呼叫资源失败");
                EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCrData->callee_id, pCrData->caller_id, (char*)"Access logic device's existing call resource failed ");
                return -1;
            }

            if (pCalleeCrData->callee_sdp_ip[0] == '\0'
                || pCalleeCrData->callee_sdp_port <= 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "connect_tv_service_proc() exit---: GBLogic Device Record Srv Not Complete:record_info_pos=%d \r\n", record_info_pos);
                SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCrData->callee_id, pCrData->caller_id, (char*)"前端逻辑点位的录像流程没有结束");
                EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCrData->callee_id, pCrData->caller_id, (char*)"The video process of front-end logic point is not over.");
                return -1;
            }

            record_info_pos = record_info_find_by_cr_index(cr_pos);

            if (record_info_pos < 0)
            {
                /* 可能存在另外错误的一个录像任务信息 */
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "connect_tv_service_proc() :call_record_remove, cr_pos=%d \r\n", cr_pos);

                /* 再次查找一遍是否有录像业务 */
                cr_pos = find_GBLogic_device_has_record_service(pCrData->callee_id, pCrData->callee_stream_type);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "connect_tv_service_proc() find_GBLogic_device_has_record_service:callee_id=%s, callee_stream_type=%d, cr_pos=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, cr_pos);

                if (cr_pos >= 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "电视墙业务:逻辑设备ID=%s, 媒体流类型=%d, 电视墙通道ID=%s, 已经存在录像业务", pCrData->callee_id, pCrData->callee_stream_type, pCrData->caller_id);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TV wall:Logical device ID = % s, media stream type = % d, TV wall, channel ID = % s existing video business", pCrData->callee_id, pCrData->callee_stream_type, pCrData->caller_id);

                    pCalleeCrData = call_record_get(cr_pos);

                    if (NULL == pCalleeCrData)
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "connect_tv_service_proc() exit---: Get Callee Cr Data Error:cr_pos=%d \r\n", cr_pos);
                        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCrData->callee_id, pCrData->caller_id, (char*)"获取逻辑设备的已有呼叫资源失败");
                        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCrData->callee_id, pCrData->caller_id, (char*)"Access logic device's existing call resource failed ");
                        return -1;
                    }

                    if (pCalleeCrData->callee_sdp_ip[0] == '\0'
                        || pCalleeCrData->callee_sdp_port <= 0)
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "connect_tv_service_proc() exit---: GBLogic Device Record Srv Not Complete:record_info_pos=%d \r\n", record_info_pos);
                        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCrData->callee_id, pCrData->caller_id, (char*)"前端逻辑点位的录像流程没有结束");
                        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCrData->callee_id, pCrData->caller_id, (char*)"The video process of front-end logic point is not over.");
                        return -1;
                    }

                    record_info_pos = record_info_find_by_cr_index(cr_pos);

                    if (record_info_pos < 0)
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "connect_tv_service_proc() exit---: Find Record Info Error:cr_pos=%d \r\n", cr_pos);
                        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCrData->callee_id, pCrData->caller_id, (char*)"查找录像信息失败");
                        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCrData->callee_id, pCrData->caller_id, (char*)"Search video information failed.");
                        return -1;
                    }
                }
            }

            pRecordInfo = record_info_get(record_info_pos);

            if (NULL == pRecordInfo)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "connect_tv_service_proc() exit---: Get Record Info Error:record_info_pos=%d \r\n", record_info_pos);
                SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCrData->callee_id, pCrData->caller_id, (char*)"获取录像信息失败");
                EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCrData->callee_id, pCrData->caller_id, (char*)"Access video information failed");
                return -1;
            }

            pCrData->callee_service_type = 1;
            pCrData->callee_record_type = pRecordInfo->record_type;
            i = cms_send_invite_to_caller_for_dc_proc(pCrData, pCalleeCrData);
        }
        else
        {
            /* 查询是否有实时视频业务 */
            cr_pos = find_GBLogic_device_has_realplay_service2(pCrData->callee_id, pCrData->callee_stream_type);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "connect_tv_service_proc() find_GBLogic_device_has_realplay_service:callee_id=%s, callee_stream_type=%d, cr_pos=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, cr_pos);

            if (cr_pos >= 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "电视墙业务:逻辑设备ID=%s, 媒体流类型=%d, 电视墙通道ID=%s, 已经存在实时视频业务", pCrData->callee_id, pCrData->callee_stream_type, pCrData->caller_id);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "TV wall:logic device ID=%s, Media stream type=%d, TV wall channel ID=%s, Real-time video service already exists", pCrData->callee_id, pCrData->callee_stream_type, pCrData->caller_id);

                pCalleeCrData = call_record_get(cr_pos);

                if (NULL == pCalleeCrData)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "connect_tv_service_proc() exit---: Get Callee Cr Data Error:cr_pos=%d \r\n", cr_pos);
                    SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCrData->callee_id, pCrData->caller_id, (char*)"获取逻辑设备的实时视频呼叫资源失败");
                    EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCrData->callee_id, pCrData->caller_id, (char*)"Access logic device's existing real-time video call resource failed");
                    return -1;
                }

                if (pCalleeCrData->callee_sdp_ip[0] == '\0'
                    || pCalleeCrData->callee_sdp_port <= 0)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "connect_tv_service_proc() exit---: Callee Video Proc Not Complete \r\n");
                    SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCrData->callee_id, pCrData->caller_id, (char*)"逻辑设备已有的视频流程没有结束");
                    EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCrData->callee_id, pCrData->caller_id, (char*)"The logical device existing video process is not over.");
                    return -1;
                }

                pCrData->callee_service_type = 0;
                i = cms_send_invite_to_caller_for_dc_proc(pCrData, pCalleeCrData);
            }
            else
            {
                pCrData->callee_service_type = 0;
                i = cms_send_invite_to_callee_for_dc_proc(pCrData);
            }
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : user_send_invite_to_caller_for_dc_proc
 功能描述  : 用户连接解码器，源设备有录像情况下的处理
 输入参数  : cr_t* pCurrentCrData
             cr_t* pCalleeCrData
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月14日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int cms_send_invite_to_caller_for_dc_proc(cr_t* pCurrentCrData, cr_t* pCalleeCrData)
{
    int i = 0;
    int iRet = 0;
    int ua_index = -1;
    int send_port = 0;

    //char* sdp_ssrc = NULL;
    sdp_message_t* pLocalSDP = NULL;
    sdp_param_t sdp_param;
    sdp_extend_param_t sdp_exparam;
    char* sdp_tsu_ip = NULL;

    if (NULL == pCurrentCrData || NULL == pCalleeCrData)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "cms_send_invite_to_caller_for_dc_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* 添加被叫端信息到业务记录*/
    osip_strncpy(pCurrentCrData->callee_sdp_ip, pCalleeCrData->callee_sdp_ip, MAX_IP_LEN);
    pCurrentCrData->callee_sdp_port = pCalleeCrData->callee_sdp_port;
    pCurrentCrData->callee_transfer_type = pCalleeCrData->callee_transfer_type; /* 重新赋值 */

    /* 添加TSU 信息到业务记录*/
    osip_strncpy(pCurrentCrData->tsu_device_id, pCalleeCrData->tsu_device_id, MAX_ID_LEN);
    pCurrentCrData->tsu_resource_index = pCalleeCrData->tsu_resource_index;
    osip_strncpy(pCurrentCrData->tsu_ip, pCalleeCrData->tsu_ip, MAX_IP_LEN);

    memcpy(&pCurrentCrData->TSUVideoIP, &pCalleeCrData->TSUVideoIP, sizeof(ip_pair_t));
    memcpy(&pCurrentCrData->TSUDeviceIP, &pCalleeCrData->TSUDeviceIP, sizeof(ip_pair_t));

    pCurrentCrData->tsu_code = pCalleeCrData->tsu_code;
    pCurrentCrData->tsu_recv_port = pCalleeCrData->tsu_recv_port;

    //发送端口号从新获取
    /* 获取TSU 发送端口号, 先判断一下主叫侧的链接类型，如果是TCP链接，先查看一下是否存在，如果存在，则使用原来的发送端口号 */
    if (TRANSFER_PROTOCOL_TCP == pCurrentCrData->caller_transfer_type)
    {
        //send_port = get_GBLogic_device_caller_tcp_send_port(pCurrentCrData->callee_id, pCurrentCrData->callee_stream_type);

        //if (send_port < 0)
        //{
        //send_port = get_send_port_by_tsu_resource(pCalleeCrData->tsu_ip);
        //}
        /* 不需要获取TSU的发送端口号, TSU的端口号在添加转发任务的时候返回 */
        send_port = 43111;
    }
    else
    {
        send_port = get_send_port_by_tsu_resource(pCalleeCrData->tsu_ip);
    }

    if (send_port <= 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cms_send_invite_to_caller_for_dc_proc() exit---: Get TSU Send Port Error:tsu_ip=%s \r\n", pCalleeCrData->tsu_ip);
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s, tsu_ip=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"获取TSU的发送端口号失败", pCalleeCrData->tsu_ip);
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s, tsu_ip=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Failed to get the TSU's send port number", pCalleeCrData->tsu_ip);
        return -1;
    }

    pCurrentCrData->tsu_send_port = send_port;

    /* 组建本地SDP信息*/
    memset(&sdp_param, 0, sizeof(sdp_param_t));
    osip_strncpy(sdp_param.o_username, local_cms_id_get(), 32);
    osip_strncpy(sdp_param.s_name, (char*)"Play", 32);

    sdp_tsu_ip = get_cr_sdp_tsu_ip(pCurrentCrData, pCurrentCrData->caller_server_ip_ethname);

    if (NULL == sdp_tsu_ip)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cms_send_invite_to_caller_for_dc_proc() exit---: Get Caller TSU SDP IP Error:caller_server_ip_ethname=%s \r\n", pCurrentCrData->caller_server_ip_ethname);
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s, caller_server_ip_ethname=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"获取主叫侧的SDP消息中的TSU的IP地址失败", pCurrentCrData->caller_server_ip_ethname);
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s, caller_server_ip_ethname=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Access TSU IP address from caller side SDP message failed", pCurrentCrData->caller_server_ip_ethname);
        return -1;
    }

    osip_strncpy(sdp_param.sdp_ip, sdp_tsu_ip, MAX_IP_LEN);
    sdp_param.video_port = pCurrentCrData->tsu_send_port;
    sdp_param.video_code_type = pCurrentCrData->tsu_code;
    sdp_param.media_direction = MEDIA_DIRECTION_TYPE_SENDONLY;

    /* 被叫侧使用的码流类型，也需要带给解码器 */
    sdp_param.stream_type = pCurrentCrData->callee_stream_type;

    /* 传输方式赋值 */
    if (TRANSFER_PROTOCOL_TCP == pCurrentCrData->caller_transfer_type)
    {
        sdp_param.trans_type = 2;
    }
    else
    {
        sdp_param.trans_type = 1;
    }

    if (pCalleeCrData->callee_onvif_url[0] != '\0')
    {
        memset(&sdp_exparam, 0, sizeof(sdp_extend_param_t));

        osip_strncpy(pCurrentCrData->callee_onvif_url, pCalleeCrData->callee_onvif_url, 255);
        osip_strncpy(sdp_exparam.onvif_url, pCurrentCrData->callee_onvif_url, 255);

        iRet = SIP_BuildSDPInfoEx(&pLocalSDP, &sdp_param, &sdp_exparam);
    }
    else
    {
        iRet = SIP_BuildSDPInfoEx(&pLocalSDP, &sdp_param, NULL);
    }

    if (0 != iRet)
    {
        sdp_message_free(pLocalSDP);
        pLocalSDP = NULL;

        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cms_send_invite_to_caller_for_dc_proc() exit---: SDP Build Offer Error:tsu_port=%d, tsu_code=%d \r\n", pCurrentCrData->tsu_send_port, pCurrentCrData->tsu_code);
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"组建本地SDP信息失败");
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Created local SDP message failed");
        return -1;
    }

#if 0
    sdp_ssrc = sdp_message_y_ssrc_get(pLocalSDP);

    if (NULL == sdp_ssrc)
    {
        sdp_ssrc = osip_getcopy((char*)"0");
        sdp_message_y_ssrc_set(pLocalSDP, sdp_ssrc);
    }

#endif

    /* 11、发送呼叫给DC */
    ua_index = SIP_SendInvite(pCurrentCrData->caller_id, pCurrentCrData->callee_id, pCurrentCrData->caller_server_ip, pCurrentCrData->caller_server_port, pCurrentCrData->caller_ip, pCurrentCrData->caller_port, NULL, NULL, pLocalSDP);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cms_send_invite_to_caller_for_dc_proc() SIP_SendInvite:index=%d \r\n", ua_index);

    if (ua_index < 0)
    {
        sdp_message_free(pLocalSDP);
        pLocalSDP = NULL;

        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cms_send_invite_to_caller_for_dc_proc() exit---: SIP Send Invite Error \r\n");
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s, ua_index=%d", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"发送INVIT消息给DC失败", ua_index);
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"send INVIT messages to DC failed");
        return -1;
    }

    pCurrentCrData->caller_ua_index = ua_index;

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "启动电视墙业务成功,发送Invite消息到电视墙DC:逻辑设备ID=%s, 电视墙通道ID=%s, 电视墙DC IP地址=%s, 端口号=%d, SIP会话句柄=%d", pCurrentCrData->callee_id, pCurrentCrData->caller_id, pCurrentCrData->caller_ip, pCurrentCrData->caller_port, ua_index);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "succeed to launch the TV wall, send the Invite message to TV WALL:logic device ID=%s, TV wall IP=%s, port ID=%d, SIP daliog hand=%d", pCurrentCrData->callee_id, pCurrentCrData->caller_id, pCurrentCrData->caller_ip, pCurrentCrData->caller_port, ua_index);

    sdp_message_free(pLocalSDP);
    pLocalSDP = NULL;

    return ua_index;
}

/*****************************************************************************
 函 数 名  : user_connect_tv_device_no_record_no_realplay_proc
 功能描述  : 用户连接解码器，源设备没有录像业务和实时视频业务情况下的处理
 输入参数  : cr_t* pCurrentCrData
             char* user_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月14日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int cms_send_invite_to_callee_for_dc_proc(cr_t* pCurrentCrData)
{
    int i = 0;
    int iRet = 0;
    int ua_index = -1;
    int tsu_index = -1;
    int recv_port = 0;
    char* tsu_ip = NULL;
    tsu_resource_info_t* pTsuResourceInfo = NULL;

    char* sdp_ssrc = NULL;
    sdp_message_t* pLocalSDP = NULL;
    sdp_param_t sdp_param;
    char* sdp_tsu_ip = NULL;

    if (NULL == pCurrentCrData)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "cms_send_invite_to_callee_for_dc_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (!g_DECMediaTransferFlag
        && EV9000_DEVICETYPE_SIPSERVER == pCurrentCrData->callee_gb_device_type && pCurrentCrData->caller_sdp_port > 0) /* 下级平台媒体流不经过本级TSU转发 */
    {
        pCurrentCrData->tsu_recv_port = pCurrentCrData->caller_sdp_port; /* 直接将解码器的接收端口赋值给TSU接收端口 */
        sdp_tsu_ip = pCurrentCrData->caller_sdp_ip; /* 直接取解码器的IP地址 */
    }
    else
    {
        /* 获取空闲的TSU资源 */
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "cms_send_invite_to_callee_for_dc_proc() get_idle_tsu_by_resource_balance_for_transfer: Begin--- \r\n", tsu_index);
        tsu_index = get_idle_tsu_by_resource_balance_for_transfer();
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "cms_send_invite_to_callee_for_dc_proc() get_idle_tsu_by_resource_balance_for_transfer: End--- tsu_index=%d \r\n", tsu_index);

        if (tsu_index < 0)
        {
            if (-2 == tsu_index)
            {
                SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"查找空闲的转发TSU资源索引失败,TSU资源队列为空");
                EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Access unused TSU resource failed");
            }
            else if (-3 == tsu_index)
            {
                SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"查找空闲的转发TSU资源索引失败,TSU资源信息错误");
                EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Access unused TSU resource failed");
            }
            else if (-4 == tsu_index)
            {
                SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"查找空闲的转发TSU资源索引失败,TSU资源都没有启用");
                EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Access unused TSU resource failed");
            }
            else if (-5 == tsu_index)
            {
                SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"查找空闲的转发TSU资源索引失败,TSU资源都不在线");
                EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Access unused TSU resource failed");
            }
            else if (-9 == tsu_index)
            {
                SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"查找空闲的转发TSU资源索引失败,通过ICE获取所有的TSU资源状态失败");
                EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Access unused TSU resource failed");
            }
            else
            {
                SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"查找空闲的转发TSU资源索引失败");
                EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Access unused TSU resource failed");
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cms_send_invite_to_callee_for_dc_proc() exit---: Find Idle TSU Error \r\n");
            return -1;
        }

        pTsuResourceInfo = tsu_resource_info_get(tsu_index);

        if (NULL == pTsuResourceInfo)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cms_send_invite_to_callee_for_dc_proc() exit---: Get Idle TSU Error:tsu_index=%d \r\n", tsu_index);
            SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s, tsu_index=%d", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"获取转发TSU资源失败", tsu_index);
            EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Access unused TSU resource failed.");
            return -1;
        }

        /* 获取和TSU通信的IP地址 */
        tsu_ip = get_tsu_ip(pTsuResourceInfo, default_eth_name_get());

        if (NULL == tsu_ip)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cms_send_invite_to_callee_for_dc_proc() exit---: Get TSU IP Error:eth_name=%s \r\n", default_eth_name_get());
            SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"获取TSU IP地址失败");
            EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Access TSU IP address failed");
            return -1;
        }

        /* 获取TSU 接收端口号 */
        recv_port = get_recv_port_by_tsu_resource(tsu_ip);

        if (recv_port <= 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cms_send_invite_to_callee_for_dc_proc() exit---: Get TSU Recv Port Error:tsu_ip=%s \r\n", tsu_ip);
            SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s, tsu_ip=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"获取TSU的接收端口号失败", tsu_ip);
            EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s, tsu_ip=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Access TSU receive port number failed", tsu_ip);
            return -1;
        }

        /* 添加TSU 信息到业务记录*/
        osip_strncpy(pCurrentCrData->tsu_device_id, pTsuResourceInfo->tsu_device_id, MAX_ID_LEN);
        pCurrentCrData->tsu_resource_index = tsu_index;
        pCurrentCrData->tsu_recv_port = recv_port;
        osip_strncpy(pCurrentCrData->tsu_ip, tsu_ip, MAX_IP_LEN);

        i = TSUResourceIPAddrListClone(pTsuResourceInfo->pTSUIPAddrList, pCurrentCrData);

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cms_send_invite_to_callee_for_dc_proc() exit---: TSU IP Addr List Clone Error \r\n");
            SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"添加本地TSU IP地址失败");
            EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Add local TSU IP address failed");
            return -1;
        }

        sdp_tsu_ip = get_cr_sdp_tsu_ip(pCurrentCrData, pCurrentCrData->callee_server_ip_ethname);

        if (NULL == sdp_tsu_ip)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cms_send_invite_to_callee_for_dc_proc() exit---: Get Callee TSU SDP IP Error:callee_server_ip_ethname=%s\r\n", pCurrentCrData->callee_server_ip_ethname);
            SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s, callee_server_ip_ethname=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"获取被叫侧的SDP消息中的TSU的IP地址失败", pCurrentCrData->callee_server_ip_ethname);
            EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s, callee_server_ip_ethname=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Access TSU IP address from callee side SDP message failed", pCurrentCrData->callee_server_ip_ethname);
            return -1;
        }
    }

    /* 组建本地SDP信息*/
    memset(&sdp_param, 0, sizeof(sdp_param_t));
    osip_strncpy(sdp_param.o_username, local_cms_id_get(), 32);
    osip_strncpy(sdp_param.s_name, (char*)"Play", 32);
    osip_strncpy(sdp_param.sdp_ip, sdp_tsu_ip, MAX_IP_LEN);
    sdp_param.video_port = pCurrentCrData->tsu_recv_port;
    sdp_param.video_code_type = -1;
    sdp_param.media_direction = MEDIA_DIRECTION_TYPE_RECVONLY;
    sdp_param.stream_type = pCurrentCrData->callee_stream_type;

    /* 传输方式赋值 */
    if (TRANSFER_PROTOCOL_TCP == pCurrentCrData->callee_transfer_type)
    {
        sdp_param.trans_type = 2;
    }
    else
    {
        sdp_param.trans_type = 1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cms_send_invite_to_callee_for_dc_proc() sdp_param:sdp_ip=%s,video_port=%d,video_code_type=%d,media_direction=%d,stream_type=%d,trans_type=%d \r\n", sdp_param.sdp_ip, sdp_param.video_port, sdp_param.video_code_type, sdp_param.media_direction, sdp_param.stream_type, sdp_param.trans_type);

    iRet = SIP_BuildSDPInfoEx(&pLocalSDP, &sdp_param, NULL);

    if (0 != iRet)
    {
        sdp_message_free(pLocalSDP);
        pLocalSDP = NULL;

        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cms_send_invite_to_callee_for_dc_proc() exit---: SDP Build Offer Error:tsu_port=%d \r\n", pCurrentCrData->tsu_recv_port);
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"组建本地SDP信息失败");
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_WARNING, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Create local SDP information failed");
        return -1;
    }

    sdp_ssrc = sdp_message_y_ssrc_get(pLocalSDP);

    if (NULL == sdp_ssrc)
    {
        sdp_ssrc = osip_getcopy((char*)"0100000001");
        sdp_message_y_ssrc_set(pLocalSDP, sdp_ssrc);
    }

    /* 发送呼叫给源逻辑设备 */
    ua_index = SIP_SendInvite(local_cms_id_get(), pCurrentCrData->callee_id, pCurrentCrData->callee_server_ip, pCurrentCrData->callee_server_port, pCurrentCrData->callee_ip, pCurrentCrData->callee_port, NULL, NULL, pLocalSDP);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cms_send_invite_to_callee_for_dc_proc() SIP_SendInvite:index=%d \r\n", ua_index);

    if (ua_index < 0)
    {
        sdp_message_free(pLocalSDP);
        pLocalSDP = NULL;

        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cms_send_invite_to_callee_for_dc_proc() exit---: SIP Send Invite Error \r\n");
        SystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "启动电视墙业务失败:逻辑设备ID=%s, 电视墙通道ID=%s, 原因=%s, ua_index=%d", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"发送INVIT消息给源端逻辑设备失败", ua_index);
        EnSystemLog(EV9000_CMS_CONNECT_TV_ERROR, EV9000_LOG_LEVEL_ERROR, "Failed to launch the TV wall:logic device ID=%s, TV wall channel ID=%s, cause=%s", pCurrentCrData->callee_id, pCurrentCrData->caller_id, (char*)"Send INVIT message to source end logic device failed.");
        return -1;
    }

    pCurrentCrData->callee_ua_index = ua_index;

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "启动电视墙业务成功,发送Invite消息到视频流发送方:逻辑设备ID=%s, 电视墙通道ID=%s, 视频流发送方IP地址=%s, 端口号=%d, SIP会话句柄=%d", pCurrentCrData->callee_id, pCurrentCrData->caller_id, pCurrentCrData->callee_ip, pCurrentCrData->callee_port, ua_index);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "succeed to launch the TV wall, send the Invite message to video stream sender:logic device ID=%s, TV wall channel ID=%s, video stream sender IP=%s, port ID=%d, SIP dialog index=%d", pCurrentCrData->callee_id, pCurrentCrData->caller_id, pCurrentCrData->callee_ip, pCurrentCrData->callee_port, ua_index);

    sdp_message_free(pLocalSDP);
    pLocalSDP = NULL;

    return ua_index;
}
#endif

#if DECS("Ack回应处理队列")
/*****************************************************************************
 函 数 名  : ack_send_init
 功能描述  : ACK回应队列节点结构初始化
 输入参数  : ack_send_t** node
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月16日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ack_send_init(ack_send_t** node)
{
    *node = (ack_send_t*)osip_malloc(sizeof(ack_send_t));

    if (*node == NULL)
    {
        return -1;
    }

    (*node)->cr_index = -1;
    (*node)->caller_ua_index = -1;
    (*node)->caller_time_count = -1;
    (*node)->callee_ua_index = -1;
    (*node)->callee_time_count = -1;

    return 0;
}

/*****************************************************************************
 函 数 名  : ack_send_free
 功能描述  : ACK回应队列节点结构释放
 输入参数  : ack_send_t* node
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月16日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void ack_send_free(ack_send_t* node)
{
    if (node == NULL)
    {
        return;
    }

    node->cr_index = -1;
    node->caller_ua_index = -1;
    node->caller_time_count = -1;
    node->callee_ua_index = -1;
    node->callee_time_count = -1;

    return;
}

/*****************************************************************************
 函 数 名  : ack_send_list_init
 功能描述  : ACK回应队列初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月16日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ack_send_list_init()
{
    g_AckSendList = (ack_send_list_t*)osip_malloc(sizeof(ack_send_list_t));

    if (g_AckSendList == NULL)
    {
        return -1;
    }

    /* init timer list*/
    g_AckSendList->ack_list = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_AckSendList->ack_list)
    {
        osip_free(g_AckSendList);
        g_AckSendList = NULL;
        return -1;
    }

    osip_list_init(g_AckSendList->ack_list);

#ifdef MULTI_THR
    /* init smutex */
    g_AckSendList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_AckSendList->lock)
    {
        osip_free(g_AckSendList->ack_list);
        g_AckSendList->ack_list = NULL;
        osip_free(g_AckSendList);
        g_AckSendList = NULL;
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : ack_send_list_clean
 功能描述  : ACK回应队列清理
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月16日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ack_send_list_clean()
{
    ack_send_t* ack_node = NULL;

    if (NULL == g_AckSendList)
    {
        return -1;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_AckSendList->lock);
#endif

    while (!osip_list_eol(g_AckSendList->ack_list, 0))
    {
        ack_node = (ack_send_t*)osip_list_get(g_AckSendList->ack_list, 0);

        if (NULL != ack_node)
        {
            osip_list_remove(g_AckSendList->ack_list, 0);
            ack_send_free(ack_node);
            osip_free(ack_node);
            ack_node = NULL;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AckSendList->lock);
#endif

    return 0;

}

/*****************************************************************************
 函 数 名  : ack_send_list_free
 功能描述  : ACK回应队列释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月16日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void ack_send_list_free()
{
    if (NULL != g_AckSendList)
    {
        ack_send_list_clean();
        osip_free(g_AckSendList->ack_list);
        g_AckSendList->ack_list = NULL;
#ifdef MULTI_THR
        osip_mutex_destroy((struct osip_mutex*)g_AckSendList->lock);
        g_AckSendList->lock = NULL;
#endif
    }

    return;
}

/*****************************************************************************
 函 数 名  : ack_send_find
 功能描述  : ACK回应队列查找
 输入参数  : int cr_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月16日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ack_send_find(int cr_index)
{
    int i = 0;
    ack_send_t* node = NULL;

    if (NULL == g_AckSendList || NULL == g_AckSendList->ack_list)
    {
        return -1;
    }

    if (cr_index < 0)
    {
        return -1;
    }

    i = 0;

    while (!osip_list_eol(g_AckSendList->ack_list, i))
    {
        node = (ack_send_t*)osip_list_get(g_AckSendList->ack_list, i);

        if (NULL == node)
        {
            i++;
            continue;
        }

        if (node->cr_index == cr_index)
        {
            return i;
        }

        i++;
    }

    return -1;
}

/*****************************************************************************
 函 数 名  : ack_send_use
 功能描述  : ACK回应队列添加
 输入参数  : int cr_index
             int caller_ua_index
             int callee_ua_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月16日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ack_send_use(int cr_index, int caller_ua_index, int callee_ua_index)
{
    int i = 0, pos = -1;
    ack_send_t* node = NULL;

    if (cr_index < 0 || NULL == g_AckSendList || NULL == g_AckSendList->ack_list || NULL == g_AckSendList->lock)
    {
        return -1;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_AckSendList->lock);
#endif

    pos = ack_send_find(cr_index);

    if (pos < 0)
    {
        i = ack_send_init(&node);

        if (i != 0 || NULL == node)
        {
#ifdef MULTI_THR
            CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AckSendList->lock);
#endif
            return -1;
        }

        node->cr_index = cr_index;

        if (caller_ua_index >= 0)
        {
            node->caller_ua_index = caller_ua_index;
            node->caller_time_count = 5;
        }

        if (callee_ua_index >= 0)
        {
            node->callee_ua_index = callee_ua_index;
            node->callee_time_count = 5;
        }

        i = osip_list_add(g_AckSendList->ack_list, node, -1);

        if (i < 0)
        {
            ack_send_free(node);
            osip_free(node);
            node = NULL;
#ifdef MULTI_THR
            CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AckSendList->lock);
#endif
            return -1;
        }
    }
    else
    {
        node = (ack_send_t*)osip_list_get(g_AckSendList->ack_list, pos);

        if (NULL == node)
        {
#ifdef MULTI_THR
            CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AckSendList->lock);
#endif
            return -1;
        }

        if (caller_ua_index >= 0)
        {
            node->caller_ua_index = caller_ua_index;
            node->caller_time_count = 3;
        }

        if (callee_ua_index >= 0)
        {
            node->callee_ua_index = callee_ua_index;
            node->callee_time_count = 3;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AckSendList->lock);
#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : ack_send_remove
 功能描述  : ACK回应队列移除
 输入参数  : int cr_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月16日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ack_send_remove(int cr_index)
{
    int pos = -1;
    ack_send_t* node = NULL;

    if (cr_index < 0 || NULL == g_AckSendList || NULL == g_AckSendList->ack_list || NULL == g_AckSendList->lock)
    {
        return -1;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_AckSendList->lock);
#endif

    pos = ack_send_find(cr_index);

    if (pos >= 0)
    {
        node = (ack_send_t*)osip_list_get(g_AckSendList->ack_list, pos);

        if (NULL != node)
        {
            osip_list_remove(g_AckSendList->ack_list, pos);
            ack_send_free(node);
            osip_free(node);
            node = NULL;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AckSendList->lock);
#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : ack_send_proc
 功能描述  : ACK回应处理
 输入参数  : int cr_index
             int iResult
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月16日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int ack_send_proc(int cr_index, int iResult)
{
    int i = 0, pos = -1;
    ack_send_t* node = NULL;
    int caller_ua_index = -1;
    int callee_ua_index = -1;

    if (cr_index < 0 || NULL == g_AckSendList || NULL == g_AckSendList->ack_list || NULL == g_AckSendList->lock)
    {
        return -1;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_AckSendList->lock);
#endif

    pos = ack_send_find(cr_index);

    if (pos >= 0)
    {
        node = (ack_send_t*)osip_list_get(g_AckSendList->ack_list, pos);

        if (NULL != node)
        {
            caller_ua_index = node->caller_ua_index;
            callee_ua_index = node->callee_ua_index;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AckSendList->lock);
#endif

    if (caller_ua_index >= 0)
    {
        i = SIP_SendAck(caller_ua_index);
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "ack_send_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", caller_ua_index, i);
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ACK消息发送处理, 发送ACK消息到主叫侧:caller_ua_index=%d, i=%d", caller_ua_index, i);

        if (0 != iResult)
        {
            i = SIP_SendBye(caller_ua_index);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "ack_send_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", caller_ua_index, i);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ACK消息发送处理, 返回结果不正确, 发送BYE消息到主叫侧:caller_ua_index=%d, i=%d", caller_ua_index, i);
        }
    }

    if (callee_ua_index >= 0)
    {
        i = SIP_SendAck(callee_ua_index);
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "ack_send_proc() SIP_SendAck:callee_ua_index=%d, i=%d \r\n", callee_ua_index, i);
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "ACK消息发送处理, 发送ACK消息到被叫侧:callee_ua_index=%d, i=%d", callee_ua_index, i);

        if (0 != iResult)
        {
            i = SIP_SendBye(callee_ua_index);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "ack_send_proc() SIP_SendBye:callee_ua_index=%d, i=%d \r\n", callee_ua_index, i);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "ACK消息发送处理, 返回结果不正确, 发送BYE消息到被叫侧:callee_ua_index=%d, i=%d", callee_ua_index, i);
        }
    }

    i = ack_send_remove(cr_index);

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "ack_send_proc() ack_send_remove:cr_index=%d, i=%d \r\n", cr_index, i);

    return i;
}

/*****************************************************************************
 函 数 名  : scan_ack_send_list
 功能描述  : ACK回应队列扫描
 输入参数  : DBOper* pAck_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月16日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void scan_ack_send_list(DBOper* pAck_dboper)
{
    int i = 0;
    int pos = 0;
    int index = 0;
    int cr_index = 0;
    ack_send_t* node = NULL;
    vector<int> needSendAckProcVector;

    if (NULL == g_AckSendList || NULL == g_AckSendList->ack_list || NULL == g_AckSendList->lock)
    {
        return;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_AckSendList->lock);
#endif

    if (osip_list_size(g_AckSendList->ack_list) <= 0)
    {
#ifdef MULTI_THR
        CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AckSendList->lock);
#endif
        return;
    }

    pos = 0;
    needSendAckProcVector.clear();

    while (!osip_list_eol(g_AckSendList->ack_list, pos))
    {
        node = (ack_send_t*)osip_list_get(g_AckSendList->ack_list, pos);

        if (NULL == node)
        {
            pos++;
            continue;
        }

        if (node->cr_index < 0)
        {
            pos++;
            continue;
        }

        if (node->caller_ua_index >= 0)
        {
            if (node->caller_time_count > 0)
            {
                node->caller_time_count--;
                pos++;
                continue;
            }
            else if (node->caller_time_count == 0)
            {
                needSendAckProcVector.push_back(node->cr_index);
                pos++;
                continue;
            }
        }

        if (node->callee_ua_index >= 0)
        {
            if (node->callee_time_count > 0)
            {
                node->callee_time_count--;
                pos++;
                continue;
            }
            else if (node->callee_time_count == 0)
            {
                needSendAckProcVector.push_back(node->cr_index);
                pos++;
                continue;
            }
        }

        pos++;
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_AckSendList->lock);
#endif

    /* 处理超时需要发送Ack 的 */
    if (needSendAckProcVector.size() > 0)
    {
        for (index = 0; index < (int)needSendAckProcVector.size(); index++)
        {
            cr_index = needSendAckProcVector[index];

            i |= ack_send_proc(cr_index, 1);

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "等待TSU通知任务创建结果消息处理结果超时, ACK超时处理:cr_index=%d, iRet=%d", cr_index, i);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_ack_send_list(): ack_send_proc:cr_pos=%d, iRet=%d \r\n", cr_index, i);
        }
    }

    needSendAckProcVector.clear();

    return;
}
#endif

#if DECS("服务器转发的XML Message消息")
/*****************************************************************************
 函 数 名  : transfer_xml_msg_init
 功能描述  : 转发的XML Message消息结构初始化
 输入参数  : user_xml_msg_t ** user_xml_msg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月29日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int transfer_xml_msg_init(transfer_xml_msg_t** transfer_xml_msg)
{
    *transfer_xml_msg = (transfer_xml_msg_t*)osip_malloc(sizeof(transfer_xml_msg_t));

    if (*transfer_xml_msg == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_init() exit---: *transfer_xml_msg Smalloc Error \r\n");
        return -1;
    }

    (*transfer_xml_msg)->msg_type = XML_TYPE_NULL;
    (*transfer_xml_msg)->old_xml_sn = 0;
    (*transfer_xml_msg)->transfer_xml_sn = 0;
    (*transfer_xml_msg)->device_id[0] = '\0';
    (*transfer_xml_msg)->recevice_time = 0;
    (*transfer_xml_msg)->source_id[0] = '\0';
    (*transfer_xml_msg)->source_ip[0] = '\0';
    (*transfer_xml_msg)->source_port = 0;
    (*transfer_xml_msg)->local_ip[0] = '\0';
    (*transfer_xml_msg)->local_port = 0;
    (*transfer_xml_msg)->iSumNum = 0;
    (*transfer_xml_msg)->iListNum = 0;

    return 0;
}

/*****************************************************************************
 函 数 名  : transfer_xml_msg_free
 功能描述  : 转发的XML Message消息结构释放
 输入参数  : user_xml_msg_t * user_xml_msg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月29日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void transfer_xml_msg_free(transfer_xml_msg_t* transfer_xml_msg)
{
    if (transfer_xml_msg == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_free() exit---: Param Error \r\n");
        return;
    }

    transfer_xml_msg->msg_type = XML_TYPE_NULL;
    transfer_xml_msg->old_xml_sn = 0;
    transfer_xml_msg->transfer_xml_sn = 0;
    memset(transfer_xml_msg->device_id, 0, MAX_ID_LEN + 4);
    transfer_xml_msg->recevice_time = 0;

    memset(transfer_xml_msg->source_id, 0, MAX_ID_LEN + 4);
    memset(transfer_xml_msg->source_ip, 0, MAX_IP_LEN);
    transfer_xml_msg->source_port = 0;

    memset(transfer_xml_msg->local_ip, 0, MAX_IP_LEN);
    transfer_xml_msg->local_port = 0;

    transfer_xml_msg->iSumNum = 0;
    transfer_xml_msg->iListNum = 0;

    osip_free(transfer_xml_msg);
    transfer_xml_msg = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : transfer_xml_msg_list_init
 功能描述  : 服务器转发XML Message消息队列初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月29日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int transfer_xml_msg_list_init()
{
    g_TransferXMLMsgList = (transfer_xml_msg_list_t*)osip_malloc(sizeof(transfer_xml_msg_list_t));

    if (g_TransferXMLMsgList == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_list_init() exit---: *user_response_msg_list Smalloc Error \r\n");
        return -1;
    }

    g_TransferXMLMsgList->pXMLMsgList = (osip_list_t*)osip_malloc(sizeof(osip_list_t));

    if (NULL == g_TransferXMLMsgList->pXMLMsgList)
    {
        osip_free(g_TransferXMLMsgList);
        g_TransferXMLMsgList = NULL;
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_list_init() exit---: User XML Message List Init Error \r\n");
        return -1;
    }

    osip_list_init(g_TransferXMLMsgList->pXMLMsgList);

#ifdef MULTI_THR
    /* init smutex */
    g_TransferXMLMsgList->lock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_TransferXMLMsgList->lock)
    {
        osip_free(g_TransferXMLMsgList->pXMLMsgList);
        g_TransferXMLMsgList->pXMLMsgList = NULL;
        osip_free(g_TransferXMLMsgList);
        g_TransferXMLMsgList = NULL;
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_list_init() exit---: User XML Message List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : transfer_xml_msg_list_free
 功能描述  : 服务器转发XML Message消息队列释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月29日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void transfer_xml_msg_list_free()
{
    if (NULL == g_TransferXMLMsgList)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_list_free() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_TransferXMLMsgList->pXMLMsgList)
    {
        osip_list_special_free(g_TransferXMLMsgList->pXMLMsgList, (void (*)(void*))&transfer_xml_msg_free);
        osip_free(g_TransferXMLMsgList->pXMLMsgList);
        g_TransferXMLMsgList->pXMLMsgList = NULL;
    }

#ifdef MULTI_THR

    if (NULL != g_TransferXMLMsgList->lock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_TransferXMLMsgList->lock);
        g_TransferXMLMsgList->lock = NULL;
    }

#endif
    osip_free(g_TransferXMLMsgList);
    g_TransferXMLMsgList = NULL;
    return;
}

/*****************************************************************************
 函 数 名  : transfer_xml_msg_list_clean
 功能描述  : 服务器转发XML Message消息队列清除
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年3月8日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void transfer_xml_msg_list_clean()
{
    if (NULL == g_TransferXMLMsgList)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_list_clean() exit---: Param Error \r\n");
        return;
    }

    if (NULL != g_TransferXMLMsgList->pXMLMsgList)
    {
        osip_list_special_free(g_TransferXMLMsgList->pXMLMsgList, (void (*)(void*))&transfer_xml_msg_free);
    }

    return;
}

/*****************************************************************************
 函 数 名  : transfer_xml_msg_list_lock
 功能描述  : 服务器转发XML Message消息队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月29日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int transfer_xml_msg_list_lock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_TransferXMLMsgList == NULL || g_TransferXMLMsgList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_lock((struct osip_mutex*)g_TransferXMLMsgList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : transfer_xml_msg_list_unlock
 功能描述  : 服务器转发XML Message消息解锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月29日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int transfer_xml_msg_list_unlock()
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_TransferXMLMsgList == NULL || g_TransferXMLMsgList->lock == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_mutex_unlock((struct osip_mutex*)g_TransferXMLMsgList->lock);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_transfer_xml_msg_list_lock
 功能描述  : 服务器转发XML Message消息队列锁定
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月29日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int debug_transfer_xml_msg_list_lock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_TransferXMLMsgList == NULL || g_TransferXMLMsgList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "debug_transfer_xml_msg_list_lock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_lock((struct osip_mutex*)g_TransferXMLMsgList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : debug_transfer_xml_msg_list_unlock
 功能描述  : 服务器转发XML Message消息解锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月29日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int debug_transfer_xml_msg_list_unlock(const char* file, int line, const char* func)
{
    int iRet = 0;

#ifdef MULTI_THR

    if (g_TransferXMLMsgList == NULL || g_TransferXMLMsgList->lock == NULL)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "debug_transfer_xml_msg_list_unlock() exit---: Param Error \r\n");
        return -1;
    }

    iRet = osip_debug_mutex_unlock((struct osip_mutex*)g_TransferXMLMsgList->lock, file, line, func);

#endif
    return iRet;
}

/*****************************************************************************
 函 数 名  : transfer_xml_msg_add
 功能描述  : 服务器转发XML Message消息到队列中
 输入参数  : xml_type_t msg_type
             unsigned int old_xml_sn
             unsigned int transfer_xml_sn
             char* source_id
             char* source_ip
             int source_port
             char* local_ip
             int local_port
             char* device_id
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月29日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int transfer_xml_msg_add(xml_type_t msg_type, unsigned int old_xml_sn, unsigned int transfer_xml_sn, char* source_id, char* source_ip, int source_port, char* local_ip, int local_port, char* device_id)
{
    transfer_xml_msg_t* pTransferXMLMsg = NULL;
    int iRet = 0;

    if (g_TransferXMLMsgList == NULL || g_TransferXMLMsgList->pXMLMsgList == NULL || old_xml_sn <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == source_id || source_id[0] == '\0'
        || NULL == source_ip || source_ip[0] == '\0' || source_port <= 0
        || NULL == device_id || device_id[0] == '\0')
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = transfer_xml_msg_init(&pTransferXMLMsg);

    if (iRet != 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "transfer_xml_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    pTransferXMLMsg->msg_type = msg_type;
    pTransferXMLMsg->old_xml_sn = old_xml_sn;

    if (transfer_xml_sn <= 0)
    {
        pTransferXMLMsg->transfer_xml_sn = old_xml_sn;
    }
    else
    {
        pTransferXMLMsg->transfer_xml_sn = transfer_xml_sn;
    }

    osip_strncpy(pTransferXMLMsg->device_id, device_id, MAX_ID_LEN);
    pTransferXMLMsg->recevice_time = time(NULL);

    osip_strncpy(pTransferXMLMsg->source_id, source_id, MAX_ID_LEN);
    osip_strncpy(pTransferXMLMsg->source_ip, source_ip, MAX_IP_LEN);
    pTransferXMLMsg->source_port = source_port;

    osip_strncpy(pTransferXMLMsg->local_ip, local_ip, MAX_IP_LEN);
    pTransferXMLMsg->local_port = local_port;

    TRANSFER_XML_SMUTEX_LOCK();

    iRet = osip_list_add(g_TransferXMLMsgList->pXMLMsgList, pTransferXMLMsg, -1); /* add to list tail */

    if (iRet < 0)
    {
        TRANSFER_XML_SMUTEX_UNLOCK();
        transfer_xml_msg_free(pTransferXMLMsg);
        pTransferXMLMsg = NULL;
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "transfer_xml_msg_add() exit---: List Add Error \r\n");
        return -1;
    }

    TRANSFER_XML_SMUTEX_UNLOCK();
    return iRet - 1;
}

/*****************************************************************************
 函 数 名  : transfer_xml_msg_remove
 功能描述  : 服务器转发XML Message消息
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月29日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int transfer_xml_msg_remove(int pos)
{
    transfer_xml_msg_t* pTransferXMLMsg = NULL;

    if (g_TransferXMLMsgList == NULL || g_TransferXMLMsgList->pXMLMsgList == NULL
        || pos < 0 || (pos >= osip_list_size(g_TransferXMLMsgList->pXMLMsgList)))
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_remove() exit---: Param Error \r\n");
        return -1;
    }

    TRANSFER_XML_SMUTEX_LOCK();

    pTransferXMLMsg = (transfer_xml_msg_t*)osip_list_get(g_TransferXMLMsgList->pXMLMsgList, pos);

    if (NULL == pTransferXMLMsg)
    {
        TRANSFER_XML_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "transfer_xml_msg_remove() exit---: Message NULL \r\n");
        return -1;
    }

    osip_list_remove(g_TransferXMLMsgList->pXMLMsgList, pos);
    transfer_xml_msg_free(pTransferXMLMsg);
    pTransferXMLMsg = NULL;
    TRANSFER_XML_SMUTEX_UNLOCK();
    return 0;
}

/*****************************************************************************
 函 数 名  : transfer_xml_msg_find
 功能描述  : 查找服务器转发XML Message消息
 输入参数  : xml_type_t msg_type
             char* device_id
             unsigned int transfer_xml_sn
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月16日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int transfer_xml_msg_find(xml_type_t msg_type, char* device_id, unsigned int transfer_xml_sn)
{
    int pos = -1;
    transfer_xml_msg_t* pTransferXMLMsg = NULL;

    if (NULL == g_TransferXMLMsgList || g_TransferXMLMsgList->pXMLMsgList == NULL || transfer_xml_sn <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_find() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == device_id || device_id[0] == '\0')
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_find() exit---: device id Error \r\n");
        return -1;
    }

    TRANSFER_XML_SMUTEX_LOCK();

    if (osip_list_size(g_TransferXMLMsgList->pXMLMsgList) <= 0)
    {
        TRANSFER_XML_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "transfer_xml_msg_find() exit---: User XML Message List NULL \r\n");
        return -1;
    }

    for (pos = 0; pos < osip_list_size(g_TransferXMLMsgList->pXMLMsgList); pos++)
    {
        pTransferXMLMsg = (transfer_xml_msg_t*)osip_list_get(g_TransferXMLMsgList->pXMLMsgList, pos);

        if ((NULL == pTransferXMLMsg) || (pTransferXMLMsg->transfer_xml_sn <= 0))
        {
            continue;
        }

        if ((msg_type == pTransferXMLMsg->msg_type)
            && (0 == sstrcmp(device_id, pTransferXMLMsg->device_id))
            && (pTransferXMLMsg->transfer_xml_sn == transfer_xml_sn))
        {
            TRANSFER_XML_SMUTEX_UNLOCK();
            return pos;
        }
    }

    TRANSFER_XML_SMUTEX_UNLOCK();
    return -1;
}

/*****************************************************************************
 函 数 名  : transfer_xml_msg_count_get
 功能描述  : 获取转发消源个数
 输入参数  : xml_type_t msg_type
             char* device_id
             unsigned int transfer_xml_sn
             int* msg_pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int transfer_xml_msg_count_get(xml_type_t msg_type, char* device_id, unsigned int transfer_xml_sn, int* msg_pos)
{
    int xml_count = 0;
    int pos = -1;
    transfer_xml_msg_t* pTransferXMLMsg = NULL;

    if (NULL == g_TransferXMLMsgList || g_TransferXMLMsgList->pXMLMsgList == NULL || transfer_xml_sn <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_count_get() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == device_id || device_id[0] == '\0')
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_count_get() exit---: device id Error \r\n");
        return -1;
    }

    TRANSFER_XML_SMUTEX_LOCK();

    if (osip_list_size(g_TransferXMLMsgList->pXMLMsgList) <= 0)
    {
        TRANSFER_XML_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "transfer_xml_msg_count_get() exit---: User XML Message List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_TransferXMLMsgList->pXMLMsgList); pos++)
    {
        pTransferXMLMsg = (transfer_xml_msg_t*)osip_list_get(g_TransferXMLMsgList->pXMLMsgList, pos);

        if ((NULL == pTransferXMLMsg) || (pTransferXMLMsg->transfer_xml_sn <= 0))
        {
            continue;
        }

        if ((msg_type == pTransferXMLMsg->msg_type)
            && (0 == sstrcmp(device_id, pTransferXMLMsg->device_id))
            && (pTransferXMLMsg->transfer_xml_sn == transfer_xml_sn))
        {
            xml_count++;
            *msg_pos = pos;
        }
    }

    TRANSFER_XML_SMUTEX_UNLOCK();
    return xml_count;
}

/*****************************************************************************
 函 数 名  : transfer_xml_msg_get
 功能描述  : 服务器转发 XML Message消息
 输入参数  : int pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月29日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
transfer_xml_msg_t* transfer_xml_msg_get(int pos)
{
    transfer_xml_msg_t* pTransferXMLMsg = NULL;

    if (g_TransferXMLMsgList == NULL || g_TransferXMLMsgList->pXMLMsgList == NULL
        || pos < 0 || pos >= osip_list_size(g_TransferXMLMsgList->pXMLMsgList))
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_msg_get() exit---: Param Error \r\n");
        return NULL;
    }

    TRANSFER_XML_SMUTEX_LOCK();

    pTransferXMLMsg = (transfer_xml_msg_t*)osip_list_get(g_TransferXMLMsgList->pXMLMsgList, pos);

    TRANSFER_XML_SMUTEX_UNLOCK();

    return pTransferXMLMsg;
}

/*****************************************************************************
 函 数 名  : transfer_message_by_xml_sn
 功能描述  : 根据XML消息的SN转发消息
 输入参数  : xml_type_t msg_type
             char* device_id
             int xml_sn
             int iSumNum
             int iListNum
             char* msg
             int msg_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月2日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int transfer_message_by_xml_sn(xml_type_t msg_type, char* device_id, unsigned int transfer_xml_sn, int iSumNum, int iListNum, CPacket& inPacket)
{
    int i = 0;
    int iRet = 0;
    int index = 0;
    int pos = -1;
    int xml_pos = -1;
    transfer_xml_msg_t* pTransferXMLMsg = NULL;
    vector<int> TransferXMLMsgVector;
    DOMElement* AccSnNode = NULL;
    char strOldSN[32] = {0};

    if (NULL == g_TransferXMLMsgList || g_TransferXMLMsgList->pXMLMsgList == NULL
        || transfer_xml_sn <= 0)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_message_by_xml_sn() exit---: Param Error \r\n");
        return -1;
    }

    TransferXMLMsgVector.clear();

    TRANSFER_XML_SMUTEX_LOCK();

    if (osip_list_size(g_TransferXMLMsgList->pXMLMsgList) <= 0)
    {
        TRANSFER_XML_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "transfer_message_by_xml_sn() exit---: User Response Message List NULL \r\n");
        return 0;
    }

    for (pos = 0; pos < osip_list_size(g_TransferXMLMsgList->pXMLMsgList); pos++)
    {
        pTransferXMLMsg = (transfer_xml_msg_t*)osip_list_get(g_TransferXMLMsgList->pXMLMsgList, pos);

        if ((NULL == pTransferXMLMsg) || (pTransferXMLMsg->transfer_xml_sn <= 0))
        {
            continue;
        }

        if ((msg_type == pTransferXMLMsg->msg_type)
            && (0 == sstrcmp(device_id, pTransferXMLMsg->device_id))
            && (pTransferXMLMsg->transfer_xml_sn == transfer_xml_sn))
        {
            TransferXMLMsgVector.push_back(pos);
        }
    }

    TRANSFER_XML_SMUTEX_UNLOCK();

    if (TransferXMLMsgVector.size() > 0)
    {
        for (index = 0; index < (int)TransferXMLMsgVector.size(); index++)
        {
            xml_pos = TransferXMLMsgVector[index];

            pTransferXMLMsg = transfer_xml_msg_get(xml_pos);

            if ((NULL == pTransferXMLMsg) || (pTransferXMLMsg->transfer_xml_sn <= 0))
            {
                continue;
            }

            AccSnNode = inPacket.SearchElement((char*)"SN");

            if (NULL != AccSnNode)
            {
                snprintf(strOldSN, 32, "%u", pTransferXMLMsg->old_xml_sn);
                inPacket.SetElementValue(AccSnNode, strOldSN);
                //inPacket.SetTextContent();
            }

            /* 发送消息 */
            i |= SIP_SendMessage(NULL, local_cms_id_get(), pTransferXMLMsg->source_id, pTransferXMLMsg->local_ip, pTransferXMLMsg->local_port, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "根据XML消息的SN转发Message消息到目的地失败:目的ID=%s, IP地址=%s, 端口号=%d", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "According to XML SN message fails to forward to the destination :dest_id=%s, dest_ip=%s, dest_port=%d", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "transfer_message_by_xml_sn() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "根据XML消息的SN转发Message消息到目的地成功:目的ID=%s, IP地址=%s, 端口号=%d", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "According to XML SN message succeeds to forward to the destination :dest_id=%s, dest_ip=%s, dest_port=%d", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "transfer_message_by_xml_sn() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
            }

            pTransferXMLMsg->iListNum += iListNum;
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "transfer_message_by_xml_sn() iSumNum=%d, iListNum=%d, iListNum=%d \r\n", iSumNum, iListNum, pTransferXMLMsg->iListNum);

            if (pTransferXMLMsg->iListNum >= iSumNum)
            {
                iRet = transfer_xml_msg_remove(xml_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "transfer_message_by_xml_sn() transfer_xml_msg_remove:xml_pos=%d, iRet=%d \r\n", xml_pos, iRet);
            }
        }
    }

    TransferXMLMsgVector.clear();

    return i;
}

/*****************************************************************************
 函 数 名  : transfer_xml_message_to_dest
 功能描述  : 转发XML消息到目的地
 输入参数  : int xml_pos
             int iSumNum
             int iListNum
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月16日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int transfer_xml_message_to_dest(int xml_pos, int iSumNum, int iListNum, CPacket& inPacket)
{
    int i = 0;
    transfer_xml_msg_t* pTransferXMLMsg = NULL;
    DOMElement* AccSnNode = NULL;
    char strOldSN[32] = {0};

    if (xml_pos < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_message_to_dest() exit---: Param Error \r\n");
        return -1;
    }

    pTransferXMLMsg = transfer_xml_msg_get(xml_pos);

    if (NULL == pTransferXMLMsg)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_message_to_dest() exit---: TransferXMLMsg Error \r\n");
        return -1;
    }

    AccSnNode = inPacket.SearchElement((char*)"SN");

    if (NULL != AccSnNode)
    {
        snprintf(strOldSN, 32, "%u", pTransferXMLMsg->old_xml_sn);
        inPacket.SetElementValue(AccSnNode, strOldSN);
    }

    /* 发送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pTransferXMLMsg->source_id, pTransferXMLMsg->local_ip, pTransferXMLMsg->local_port, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "根据XML消息的SN转发Message消息到目的地失败:目的ID=%s, IP地址=%s, 端口号=%d", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "According to XML SN message fails to forward to the destination :dest_id=%s, dest_ip=%s, dest_port=%d", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "transfer_xml_message_to_dest() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "根据XML消息的SN转发Message消息到目的地成功:目的ID=%s, IP地址=%s, 端口号=%d", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "According to XML SN message succeeds to forward to the destination :dest_id=%s, dest_ip=%s, dest_port=%d", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "transfer_xml_message_to_dest() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
    }

    pTransferXMLMsg->iListNum += iListNum;
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "transfer_xml_message_to_dest() iSumNum=%d, iListNum=%d, iListNum=%d \r\n", iSumNum, iListNum, pTransferXMLMsg->iListNum);

    if (pTransferXMLMsg->iListNum >= iSumNum)
    {
        i = transfer_xml_msg_remove(xml_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "transfer_xml_message_to_dest() transfer_xml_msg_remove:xml_pos=%d, iRet=%d \r\n", xml_pos, i);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : transfer_xml_message_to_dest2
 功能描述  : 转发XML消息到目的地
 输入参数  : int xml_pos
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年8月20日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int transfer_xml_message_to_dest2(int xml_pos, CPacket& inPacket)
{
    int i = 0;
    transfer_xml_msg_t* pTransferXMLMsg = NULL;
    DOMElement* AccSnNode = NULL;
    char strOldSN[32] = {0};

    if (xml_pos < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_message_to_dest2() exit---: Param Error \r\n");
        return -1;
    }

    pTransferXMLMsg = transfer_xml_msg_get(xml_pos);

    if (NULL == pTransferXMLMsg)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "transfer_xml_message_to_dest2() exit---: TransferXMLMsg Error \r\n");
        return -1;
    }

    AccSnNode = inPacket.SearchElement((char*)"SN");

    if (NULL != AccSnNode)
    {
        snprintf(strOldSN, 32, "%u", pTransferXMLMsg->old_xml_sn);
        inPacket.SetElementValue(AccSnNode, strOldSN);
    }

    /* 发送消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), pTransferXMLMsg->source_id, pTransferXMLMsg->local_ip, pTransferXMLMsg->local_port, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "根据XML消息的SN转发Message消息到目的地失败:目的ID=%s, IP地址=%s, 端口号=%d", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "According to XML SN message fails to forward to the destination :dest_id=%s, dest_ip=%s, dest_port=%d", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "transfer_xml_message_to_dest2() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "根据XML消息的SN转发Message消息到目的地成功:目的ID=%s, IP地址=%s, 端口号=%d", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "According to XML SN message succeeds to forward to the destination :dest_id=%s, dest_ip=%s, dest_port=%d", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "transfer_xml_message_to_dest2() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", pTransferXMLMsg->source_id, pTransferXMLMsg->source_ip, pTransferXMLMsg->source_port);
    }

    i = transfer_xml_msg_remove(xml_pos);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "transfer_xml_message_to_dest2() transfer_xml_msg_remove:xml_pos=%d, iRet=%d \r\n", xml_pos, i);

    return 0;
}

/*****************************************************************************
 函 数 名  : scan_transfer_message_list
 功能描述  : 扫描服务器转发消息队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月16日 星期五
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void scan_transfer_message_list()
{
    int iRet = 0;
    int index = 0;
    int pos = -1;
    int xml_pos = -1;
    time_t now = 0;
    transfer_xml_msg_t* pTransferXMLMsg = NULL;
    vector<int> TransferXMLMsgVector;

    if (NULL == g_TransferXMLMsgList || g_TransferXMLMsgList->pXMLMsgList == NULL)
    {
        //DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "scan_transfer_message_list() exit---: Param Error \r\n");
        return;
    }

    TransferXMLMsgVector.clear();

    TRANSFER_XML_SMUTEX_LOCK();

    if (osip_list_size(g_TransferXMLMsgList->pXMLMsgList) <= 0)
    {
        TRANSFER_XML_SMUTEX_UNLOCK();
        //DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "scan_transfer_message_list() exit---: User Response Message List NULL \r\n");
        return;
    }

    now = time(NULL);

    for (pos = 0; pos < osip_list_size(g_TransferXMLMsgList->pXMLMsgList); pos++)
    {
        pTransferXMLMsg = (transfer_xml_msg_t*)osip_list_get(g_TransferXMLMsgList->pXMLMsgList, pos);

        if (NULL == pTransferXMLMsg)
        {
            continue;
        }

        if (pTransferXMLMsg->transfer_xml_sn <= 0
            || (now > pTransferXMLMsg->recevice_time && now - pTransferXMLMsg->recevice_time >= 30))
        {
            TransferXMLMsgVector.push_back(pos);
        }
    }

    TRANSFER_XML_SMUTEX_UNLOCK();

    if (TransferXMLMsgVector.size() > 0)
    {
        for (index = 0; index < (int)TransferXMLMsgVector.size(); index++)
        {
            xml_pos = TransferXMLMsgVector[index];

            pTransferXMLMsg = transfer_xml_msg_get(xml_pos);

            if (NULL == pTransferXMLMsg)
            {
                continue;
            }

            iRet = transfer_xml_msg_remove(xml_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "scan_transfer_message_list() transfer_xml_msg_remove:xml_pos=%d, iRet=%d \r\n", xml_pos, iRet);
        }
    }

    TransferXMLMsgVector.clear();

    return;
}
#endif

/*****************************************************************************
 函 数 名  : ShowCallTask
 功能描述  : 显示在线任务
 输入参数  : int sock
             call_type_t call_type
             int streamp_type: 0: 所有，1:普通流 2:智能分析流
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月29日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void ShowCallTask(int sock, call_type_t call_type, int streamp_type)
{
    char strLine[] = "\r-------------------------------------------------------------------------------------------------------------------------------------------------------------------------\r\n";
    char strHead[] = "\rIndex CallerID             CallerIP        CallerPort  CalleeID             CalleeIP        CalleePort  CalleeStreamType TSU IP          TSUPort TSUCode TSUSessionExpire\r\n";
    cr_t* pCrData = NULL;
    CR_Data_Iterator Itr;
    char rbuf[256] = {0};
    char* pTmp = NULL;
    char strDeviceType[4] = {0};
    int iType = 0;

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
        send(sock, strHead, strlen(strHead), 0);
    }

    if (g_CallRecordMap.size() <= 0)
    {
        return;
    }

    for (Itr = g_CallRecordMap.begin(); Itr != g_CallRecordMap.end(); Itr++)
    {
        pCrData = Itr->second;

        if ((NULL == pCrData) || (0 == pCrData->iUsed))
        {
            //snprintf(rbuf, 128, "\rpos=%d %s\r\n", Itr->first, "Not Used");
            //send(sock, rbuf, strlen(rbuf), 0);
            continue;
        }

        if (CALL_TYPE_NULL == pCrData->call_type
            || '\0' == pCrData->caller_id[0]
            || '\0' == pCrData->caller_ip[0]
            || pCrData->caller_port <= 0
            || '\0' == pCrData->callee_id[0]
            || '\0' == pCrData->callee_ip[0]
            || pCrData->callee_port <= 0)
        {
            //snprintf(rbuf, 128, "\rpos=%d %s\r\n", Itr->first, "Call Type NULL");
            //send(sock, rbuf, strlen(rbuf), 0);
            continue;
        }

        if (CALL_TYPE_DIAGNOSIS == call_type)
        {
            if (CALL_TYPE_REALTIME != pCrData->call_type)
            {
                //snprintf(rbuf, 255, "\rpos=%d %s\r\n", Itr->first, "Call Type Not Equeal");
                //send(sock, rbuf, strlen(rbuf), 0);
                continue;
            }

            /* 获取主叫侧的类型 */
            pTmp = &pCrData->caller_id[10];
            osip_strncpy(strDeviceType, pTmp, 3);
            iType = osip_atoi(strDeviceType);

            if (EV9000_DEVICETYPE_VIDEODIAGNOSIS != iType)
            {
                continue;
            }
        }
        else if (CALL_TYPE_INTELLIGENT == call_type)
        {
            if (CALL_TYPE_REALTIME != pCrData->call_type)
            {
                //snprintf(rbuf, 255, "\rpos=%d %s\r\n", Itr->first, "Call Type Not Equeal");
                //send(sock, rbuf, strlen(rbuf), 0);
                continue;
            }

            /* 获取主叫侧的类型 */
            pTmp = &pCrData->caller_id[10];
            osip_strncpy(strDeviceType, pTmp, 3);
            iType = osip_atoi(strDeviceType);

            if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS != iType)
            {
                continue;
            }
        }
        else if (CALL_TYPE_NULL != call_type)
        {
            if (pCrData->call_type != call_type)
            {
                //snprintf(rbuf, 255, "\rpos=%d %s\r\n", Itr->first, "Call Type Not Equeal");
                //send(sock, rbuf, strlen(rbuf), 0);
                continue;
            }
        }

        if (1 == streamp_type) /* 显示普通流 */
        {
            if (EV9000_STREAM_TYPE_INTELLIGENCE == pCrData->callee_stream_type)
            {
                continue;
            }
        }
        else if (2 == streamp_type) /* 显示智能分析流 */
        {
            if (EV9000_STREAM_TYPE_INTELLIGENCE != pCrData->callee_stream_type)
            {
                continue;
            }
        }

        memset(rbuf, 0, 256);
        snprintf(rbuf, 255, "\r%-5d %-20s %-15s %-5d       %-20s %-15s %-5d       %-16d %-15s %-7d %-7d %-16d\r\n", Itr->first, pCrData->caller_id, pCrData->caller_ip, pCrData->caller_sdp_port, pCrData->callee_id, pCrData->callee_ip, pCrData->callee_sdp_port, pCrData->callee_stream_type, pCrData->tsu_ip, pCrData->tsu_recv_port, pCrData->tsu_code, pCrData->tsu_session_expire);

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
 函 数 名  : ShowCallTaskDetail
 功能描述  : 显示呼叫任务详细信息
 输入参数  : int sock
                            int cr_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月29日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void ShowCallTaskDetail(int sock, int cr_index)
{
    int i = 0;
    char strLine[] = "\r--------------------------------------------------------------\r\n";
    cr_t* pCrData = NULL;
    char rbuf[256] = {0};

    pCrData = call_record_get(cr_index);

    if (NULL == pCrData || pCrData->iUsed == 0)
    {
        snprintf(rbuf, 256, "\rCall Task Index=%d:%s\r\n", cr_index, "Not Used");
        send(sock, rbuf, strlen(rbuf), 0);
        return;
    }

    if (CALL_TYPE_NULL == pCrData->call_type)
    {
        snprintf(rbuf, 256, "\rCall Task Index=%d:%s\r\n", cr_index, "Call Type NULL");
        send(sock, rbuf, strlen(rbuf), 0);
        return;
    }

    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    /* 索引 */
    snprintf(rbuf, 256, "\rCall Record Index       :%d\r\n", cr_index);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* 类型 */
    if (CALL_TYPE_REALTIME == pCrData->call_type)
    {
        snprintf(rbuf, 256, "\rRecord Type             :%s\r\n", (char*)"Realtime Play Task");
    }
    else if (CALL_TYPE_RECORD == pCrData->call_type)
    {
        snprintf(rbuf, 256, "\rRecord Type             :%s\r\n", (char*)"Record Task");
    }
    else if (CALL_TYPE_DC == pCrData->call_type)
    {
        snprintf(rbuf, 256, "\rRecord Type             :%s\r\n", (char*)"DC Play Task");
    }
    else if (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
    {
        snprintf(rbuf, 256, "\rRecord Type             :%s\r\n", (char*)"Record Play Play Task");
    }
    else if (CALL_TYPE_DOWNLOAD == pCrData->call_type)
    {
        snprintf(rbuf, 256, "\rRecord Type             :%s\r\n", (char*)"Download File Task");
    }
    else if (CALL_TYPE_AUDIO == pCrData->call_type)
    {
        snprintf(rbuf, 256, "\rRecord Type             :%s\r\n", (char*)"Audio Send Task");
    }

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Call Status */
    if (CALL_STATUS_WAIT_ANSWER == pCrData->call_status)
    {
        snprintf(rbuf, 256, "\rCall Status             :%s\r\n", (char*)"CALL_STATUS_WAIT_ANSWER");
    }
    else if (CALL_STATUS_WAIT_RELEASE == pCrData->call_status)
    {
        snprintf(rbuf, 256, "\rCall Status             :%s\r\n", (char*)"CALL_STATUS_WAIT_RELEASE");
    }
    else if (CALL_STATUS_RELEASE_COMPLETE == pCrData->call_status)
    {
        snprintf(rbuf, 256, "\rCall Status             :%s\r\n", (char*)"CALL_STATUS_RELEASE_COMPLETE");
    }
    else if (CALL_STATUS_NULL == pCrData->call_status)
    {
        snprintf(rbuf, 256, "\rCall Status             :%s\r\n", (char*)"CALL_STATUS_NULL");
    }

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* 主叫信息 */
    snprintf(rbuf, 256, "%s", "\r>>>>>>>>>>>>>>>>>>>>>>>>:Caller Info\r\n");

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Caller ID */
    snprintf(rbuf, 256, "\rCaller ID               :%s\r\n", pCrData->caller_id);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Caller IP */
    snprintf(rbuf, 256, "\rCaller IP               :%s\r\n", pCrData->caller_ip);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Caller Port */
    snprintf(rbuf, 256, "\rCaller Port             :%d\r\n", pCrData->caller_port);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Caller SDP IP */
    snprintf(rbuf, 256, "\rCaller SDP IP           :%s\r\n", pCrData->caller_sdp_ip);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Caller SDP Port */
    snprintf(rbuf, 256, "\rCaller SDP Port         :%d\r\n", pCrData->caller_sdp_port);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Caller UA Index */
    snprintf(rbuf, 256, "\rCaller SIP UA Index     :%d\r\n", pCrData->caller_ua_index);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Caller Server IP Eth Name */
    snprintf(rbuf, 256, "\rCaller Server Eth Name  :%s\r\n", pCrData->caller_server_ip_ethname);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Caller Server IP */
    snprintf(rbuf, 256, "\rCaller Server IP        :%s\r\n", pCrData->caller_server_ip);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Caller Server Port */
    snprintf(rbuf, 256, "\rCaller Server Port      :%d\r\n", pCrData->caller_server_port);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Caller Transfer Protol */
    snprintf(rbuf, 256, "\rCaller Transfer Protocol:%d\r\n", pCrData->caller_transfer_type);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* 被叫信息 */
    snprintf(rbuf, 256, "%s", "\r>>>>>>>>>>>>>>>>>>>>>>>>:Callee Info\r\n");

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Callee ID */
    snprintf(rbuf, 256, "\rCallee ID               :%s\r\n", pCrData->callee_id);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Callee IP */
    snprintf(rbuf, 256, "\rCallee IP               :%s\r\n", pCrData->callee_ip);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Callee Port */
    snprintf(rbuf, 256, "\rCallee Port             :%d\r\n", pCrData->callee_port);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Callee SDP IP */
    snprintf(rbuf, 256, "\rCallee SDP IP           :%s\r\n", pCrData->callee_sdp_ip);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Callee SDP Port */
    snprintf(rbuf, 256, "\rCallee SDP Port         :%d\r\n", pCrData->callee_sdp_port);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Callee UA Index */
    snprintf(rbuf, 256, "\rCallee SIP UA Index     :%d\r\n", pCrData->callee_ua_index);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Callee Server IP Eth Name */
    snprintf(rbuf, 256, "\rCallee Server Eth Name  :%s\r\n", pCrData->callee_server_ip_ethname);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Callee Server IP */
    snprintf(rbuf, 256, "\rCallee Server IP        :%s\r\n", pCrData->callee_server_ip);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Callee Server Port */
    snprintf(rbuf, 256, "\rCallee Server Port      :%d\r\n", pCrData->callee_server_port);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Callee Transfer Protol */
    snprintf(rbuf, 256, "\rCallee Transfer Protocol:%d\r\n", pCrData->callee_transfer_type);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Callee Stream Type */
    snprintf(rbuf, 256, "\rCallee Stream Type      :%d\r\n", pCrData->callee_stream_type);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Callee Frame Rate */
    snprintf(rbuf, 256, "\rCallee Frame Rate       :%d\r\n", pCrData->callee_framerate);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Callee Onvif URL */
    snprintf(rbuf, 256, "\rCallee Onvif URL        :%s\r\n", pCrData->callee_onvif_url);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* TSU 信息 */
    snprintf(rbuf, 256, "%s", "\r>>>>>>>>>>>>>>>>>>>>>>>>:TSU Info\r\n");

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* TSU ID */
    snprintf(rbuf, 256, "\rTSU ID                  :%s\r\n", pCrData->tsu_device_id);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Default TSU IP */
    snprintf(rbuf, 256, "\rDefault TSU IP          :%s\r\n", pCrData->tsu_ip);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* TSU IP */
    snprintf(rbuf, 256, "\rTSU Video Eth Name      :%s, IP=%s \r\n", pCrData->TSUVideoIP.eth_name, pCrData->TSUVideoIP.local_ip);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    snprintf(rbuf, 256, "\rTSU Device Eth Name     :%s, IP=%s \r\n", pCrData->TSUDeviceIP.eth_name, pCrData->TSUVideoIP.local_ip);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* TSU Code */
    snprintf(rbuf, 256, "\rTSU Code Type           :%d\r\n", pCrData->tsu_code);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* TSU Recv Port */
    snprintf(rbuf, 256, "\rTSU Recv Port           :%d\r\n", pCrData->tsu_recv_port);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* TSU Send Port */
    snprintf(rbuf, 256, "\rTSU Send Port           :%d\r\n", pCrData->tsu_send_port);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* TSU Task ID */
    snprintf(rbuf, 256, "\rTSU Task ID             :%s\r\n", pCrData->task_id);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* TSU Session Expire */
    snprintf(rbuf, 256, "\rTSU Session Expire      :%d\r\n", pCrData->tsu_session_expire);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* Wait Answer Expire */
    snprintf(rbuf, 256, "\rWait Answer Expire      :%d\r\n", pCrData->wait_answer_expire);

    if (sock > 0)
    {
        send(sock, rbuf, strlen(rbuf), 0);
    }

    /* 结束行 */
    if (sock > 0)
    {
        send(sock, strLine, strlen(strLine), 0);
    }

    return;
}

