
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

#include "common/gblfunc_proc.inc"
#include "common/gblconfig_proc.inc"
#include "common/log_proc.inc"

#include "device/device_info_mgn.inc"
#include "resource/resource_info_mgn.inc"

#include "record/record_srv_proc.inc"
#include "record/record_info_mgn.inc"

#include "service/call_func_proc.inc"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern CR_Data_MAP g_CallRecordMap;             /* 呼叫链接数据队列 */
extern record_info_list_t* g_RecordInfoList;    /* 录像信息队列 */
extern int current_record_pos;                  /* 当前录像位置 */

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/
record_srv_msg_queue g_RecordSrvMsgQueue; /* 录像业务消息队列 */
#ifdef MULTI_THR
osip_mutex_t* g_RecordSrvMsgQueueLock = NULL;
#endif

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#if DECS("录像业务消息队列")
/*****************************************************************************
 函 数 名  : record_srv_msg_init
 功能描述  : 录像业务消息结构初始化
 输入参数  : record_srv_msg_t ** record_srv_msg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月27日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int record_srv_msg_init(record_srv_msg_t** record_srv_msg)
{
    *record_srv_msg = (record_srv_msg_t*)osip_malloc(sizeof(record_srv_msg_t));

    if (*record_srv_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_srv_msg_init() exit---: *record_srv_msg Smalloc Error \r\n");
        return -1;
    }

    (*record_srv_msg)->msg_type = MSG_TYPE_NULL;
    (*record_srv_msg)->caller_id[0] = '\0';
    (*record_srv_msg)->callee_id[0] = '\0';
    (*record_srv_msg)->response_code = 0;
    (*record_srv_msg)->reasonphrase[0] = '\0';
    (*record_srv_msg)->ua_dialog_index = -1;
    (*record_srv_msg)->msg_body[0] = '\0';
    (*record_srv_msg)->msg_body_len = 0;
    (*record_srv_msg)->cr_pos = -1;

    return 0;
}

/*****************************************************************************
 函 数 名  : record_srv_msg_free
 功能描述  : 录像业务消息结构释放
 输入参数  : record_srv_msg_t * record_srv_msg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月27日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void record_srv_msg_free(record_srv_msg_t* record_srv_msg)
{
    if (record_srv_msg == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_srv_msg_free() exit---: Param Error \r\n");
        return;
    }

    record_srv_msg->msg_type = MSG_TYPE_NULL;

    memset(record_srv_msg->caller_id, 0, MAX_ID_LEN + 4);
    memset(record_srv_msg->callee_id, 0, MAX_ID_LEN + 4);

    record_srv_msg->response_code = 0;

    memset(record_srv_msg->reasonphrase, 0, MAX_128CHAR_STRING_LEN + 4);

    record_srv_msg->ua_dialog_index = -1;

    memset(record_srv_msg->msg_body, 0, MAX_MSG_BODY_STRING_LEN + 4);

    record_srv_msg->msg_body_len = 0;

    osip_free(record_srv_msg);
    record_srv_msg = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : record_srv_msg_list_init
 功能描述  : 录像业务消息队列初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月27日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int record_srv_msg_list_init()
{
    g_RecordSrvMsgQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_RecordSrvMsgQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_RecordSrvMsgQueueLock)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_srv_msg_list_init() exit---: Record Srv Msg List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : record_srv_msg_list_free
 功能描述  : 录像业务消息队列释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月27日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void record_srv_msg_list_free()
{
    record_srv_msg_t* pRecordSrvMsg = NULL;

    while (!g_RecordSrvMsgQueue.empty())
    {
        pRecordSrvMsg = (record_srv_msg_t*) g_RecordSrvMsgQueue.front();
        g_RecordSrvMsgQueue.pop_front();

        if (NULL != pRecordSrvMsg)
        {
            record_srv_msg_free(pRecordSrvMsg);
            pRecordSrvMsg = NULL;
        }
    }

    g_RecordSrvMsgQueue.clear();

#ifdef MULTI_THR

    if (NULL != g_RecordSrvMsgQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_RecordSrvMsgQueueLock);
        g_RecordSrvMsgQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 函 数 名  : record_srv_msg_list_clean
 功能描述  : 录像业务消息队列清除
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
void record_srv_msg_list_clean()
{
    record_srv_msg_t* pRecordSrvMsg = NULL;

    while (!g_RecordSrvMsgQueue.empty())
    {
        pRecordSrvMsg = (record_srv_msg_t*) g_RecordSrvMsgQueue.front();
        g_RecordSrvMsgQueue.pop_front();

        if (NULL != pRecordSrvMsg)
        {
            record_srv_msg_free(pRecordSrvMsg);
            pRecordSrvMsg = NULL;
        }
    }

    g_RecordSrvMsgQueue.clear();

    return;
}

/*****************************************************************************
 函 数 名  : record_srv_msg_add
 功能描述  : 添加录像业务消息到队列中
 输入参数  : msg_type_t msg_type
             char* caller_id
             char* callee_id
             char* call_id
             int response_code
             char* reasonphrase
             int ua_dialog_index
             char* msg_body
             int msg_body_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月27日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int record_srv_msg_add(msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, char* reasonphrase, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos)
{
    record_srv_msg_t* pRecordSrvMsg = NULL;
    int iRet = 0;

    if (caller_id == NULL || callee_id == NULL)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "record_srv_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = record_srv_msg_init(&pRecordSrvMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_srv_msg_add() exit---: Record Srv Msg Init Error \r\n");
        return -1;
    }

    pRecordSrvMsg->msg_type = msg_type;

    if (NULL != caller_id)
    {
        osip_strncpy(pRecordSrvMsg->caller_id, caller_id, MAX_ID_LEN);
    }

    if (NULL != callee_id)
    {
        osip_strncpy(pRecordSrvMsg->callee_id, callee_id, MAX_ID_LEN);
    }

    pRecordSrvMsg->response_code = response_code;

    if (NULL != reasonphrase)
    {
        osip_strncpy(pRecordSrvMsg->reasonphrase, reasonphrase, MAX_128CHAR_STRING_LEN);
    }

    pRecordSrvMsg->ua_dialog_index = ua_dialog_index;

    if (NULL != msg_body)
    {
        osip_strncpy(pRecordSrvMsg->msg_body, msg_body, MAX_MSG_BODY_STRING_LEN);
    }

    pRecordSrvMsg->msg_body_len = msg_body_len;
    pRecordSrvMsg->cr_pos = cr_pos;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_RecordSrvMsgQueueLock);
#endif

    g_RecordSrvMsgQueue.push_back(pRecordSrvMsg);

#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_RecordSrvMsgQueueLock);
#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : scan_record_srv_msg_list
 功能描述  : 扫描多级互联消息队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void scan_record_srv_msg_list()
{
    int iRet = 0;
    record_srv_msg_t* pRecordSrvMsg = NULL;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_RecordSrvMsgQueueLock);
#endif

    while (!g_RecordSrvMsgQueue.empty())
    {
        pRecordSrvMsg = (record_srv_msg_t*) g_RecordSrvMsgQueue.front();
        g_RecordSrvMsgQueue.pop_front();

        if (NULL != pRecordSrvMsg)
        {
            break;
        }
    }

    if (NULL != pRecordSrvMsg)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_INFO,  "scan_record_srv_msg_list() \
        \r\n In Param: \
        \r\n msg_type=%d \
        \r\n caller_id=%s \
        \r\n callee_id=%s \
        \r\n response_code=%d \
        \r\n ua_dialog_index=%d \
        \r\n msg_body_len=%d \
        \r\n ", pRecordSrvMsg->msg_type, pRecordSrvMsg->caller_id, pRecordSrvMsg->callee_id, pRecordSrvMsg->response_code, pRecordSrvMsg->ua_dialog_index, pRecordSrvMsg->msg_body_len);

        iRet = record_srv_msg_proc(pRecordSrvMsg);
        DEBUG_TRACE(MODULE_RECORD, LOG_INFO,  "scan_record_srv_msg_list() record_srv_msg_proc end:iRet=%d\r\n", iRet);

        record_srv_msg_free(pRecordSrvMsg);
        pRecordSrvMsg = NULL;
    }

#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_RecordSrvMsgQueueLock);
#endif

    return;
}
#endif

/*****************************************************************************
 函 数 名  : record_srv_msg_proc
 功能描述  : 录像业务消息处理
 输入参数  : msg_type_t msg_type
                            char* caller_id
                            char* callee_id
                            char* call_id
                            int response_code
                            int ua_dialog_index
                            char* msg_body
                            int msg_body_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int record_srv_msg_proc(record_srv_msg_t* pRecordSrvMsg)
{
    int i = 0;

    if (NULL == pRecordSrvMsg)
    {
        return -1;
    }

    //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_srv_msg_proc() msg_type=%d  \r\n", msg_type);

    switch (pRecordSrvMsg->msg_type)
    {
        case  MSG_INVITE_RESPONSE:
            i = record_invite_response_msg_proc(pRecordSrvMsg->cr_pos, pRecordSrvMsg->ua_dialog_index, pRecordSrvMsg->response_code, pRecordSrvMsg->msg_body, pRecordSrvMsg->msg_body_len);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_srv_msg_proc() record_invite_response_msg_proc:i=%d \r\n", i);
            printf("record_srv_msg_proc() record_invite_response_msg_proc:i=%d \r\n", i);
            break;

        case  MSG_BYE:
            i = record_bye_msg_proc(pRecordSrvMsg->cr_pos, pRecordSrvMsg->ua_dialog_index);
            //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_srv_msg_proc() record_bye_msg_proc:i=%d \r\n", i);
            break;

        case  MSG_BYE_RESPONSE:
            i = record_bye_response_msg_proc(pRecordSrvMsg->cr_pos, pRecordSrvMsg->ua_dialog_index, pRecordSrvMsg->response_code);
            //DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_srv_msg_proc() record_bye_response_msg_proc:i=%d \r\n", i);
            break;

        default:
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_srv_msg_proc() exit---: Not Support Message Type:%d \r\n", pRecordSrvMsg->msg_type);
            return -1;
    }

    return i;
}

/*****************************************************************************
 函 数 名  : record_invite_response_msg_proc
 功能描述  : 录像INVITE 回应消息处理
 输入参数  : char* caller_id
                            char* callee_id
                            char* call_id
                            int ua_dialog_index
                            char* msg_body
                            int msg_body_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int record_invite_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code, char* msg_body, int msg_body_len)
{
    int i = 0;
    int iRet = 0 ;
    int record_info_index = -1;
    sdp_message_t* pRemoteSDP = NULL;
    sdp_param_t stRemoteSDPParam;
    sdp_extend_param_t stRemoteSDPExParam;
    record_info_t* pRecordInfo = NULL;
    cr_t* pCrData = NULL;
    string record_id = "";
    GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo = NULL;

    //printf_system_time();
    printf("record_invite_response_msg_proc() Enter--- :cr_pos=%d, ua_dialog_index=%d, response_code=%d \r\n", cr_pos, ua_dialog_index, response_code);

    if (cr_pos < 0)
    {
        if (200 == response_code)
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    pCrData = call_record_get(cr_pos);
    //printf_system_time();
    printf("record_invite_response_msg_proc() call_record_get:cr_pos=%d\r\n", cr_pos);

    if (pCrData == NULL)
    {
        if (200 == response_code)
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() exit---: Get Call Record Data Info Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "录像业务, INVITE回应消息处理:逻辑设备ID=%s, 逻辑设备IP地址=%s, callee_ua_index=%d, ua_dialog_index=%d, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, pCrData->callee_ua_index, ua_dialog_index, cr_pos);

    /*  获取录像信息表 */
    record_info_index = record_info_find_by_cr_index_for_response(cr_pos);  /*  录像队列锁 */
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "录像业务, INVITE回应消息处理:根据呼叫资源索引查找对应的录像策略信息, 逻辑设备ID=%s, 逻辑设备IP地址=%s, cr_pos=%d, record_info_index=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos, record_info_index);
    //printf_system_time();
    printf("record_invite_response_msg_proc() record_info_find_by_cr_index_for_response:cr_pos=%d, record_info_index=%d\r\n", cr_pos, record_info_index);

    if (record_info_index < 0)
    {
        if (-2 == record_info_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "录像业务, INVITE回应消息处理失败:逻辑设备ID=%s, 原因=%s", pCrData->callee_id, (char*)"查找逻辑设备录像资源失败:对应的录像策略信息被删除");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"Search video resource of logic device failed");
        }
        else if (-3 == record_info_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "录像业务, INVITE回应消息处理失败:逻辑设备ID=%s, 原因=%s", pCrData->callee_id, (char*)"查找逻辑设备录像资源失败:对应的录像策略信息没有启用");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"Search video resource of logic device failed");
        }
        else if (-4 == record_info_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "录像业务, INVITE回应消息处理失败:逻辑设备ID=%s, 原因=%s", pCrData->callee_id, (char*)"查找逻辑设备录像资源失败:对应的录像策略信息中TSU索引为空");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"Search video resource of logic device failed");
        }
        else if (-5 == record_info_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "录像业务, INVITE回应消息处理失败:逻辑设备ID=%s, 原因=%s", pCrData->callee_id, (char*)"查找逻辑设备录像资源失败:对应的录像策略信息中状态不处于等待处理响应");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"Search video resource of logic device failed");
        }
        else
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "录像业务, INVITE回应消息处理失败:逻辑设备ID=%s, 原因=%s", pCrData->callee_id, (char*)"查找逻辑设备录像资源失败");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"Search video resource of logic device failed");
        }

        /* 回应消息给被叫 */
        if (200 == response_code)
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);
        }

        i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);
        //printf_system_time();
        printf("record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

        i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_RECORD_FIND_RECORD_INFO_ERROR);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() exit---: record_info_find_by_cr_index_for_response Error \r\n");
        return -1;
    }

    pRecordInfo = record_info_get(record_info_index);
    //printf_system_time();
    printf("record_invite_response_msg_proc() record_info_get:record_info_index=%d\r\n", record_info_index);

    if (NULL == pRecordInfo)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "录像业务, INVITE回应消息处理失败:逻辑设备ID=%s, 原因=%s", pCrData->callee_id, (char*)"获取逻辑设备录像资源失败");

        /* 回应消息给被叫 */
        if (200 == response_code)
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);
        }

        i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

        i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_RECORD_GET_RECORD_INFO_ERROR);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() exit---: record_info_get Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "录像业务, INVITE回应消息处理:录像策略索引=%u, 逻辑设备ID=%s, 逻辑设备IP地址=%s, record_cr_index=%d", pRecordInfo->uID, pCrData->callee_id, pCrData->callee_ip, pRecordInfo->record_cr_index);

    if (200 == response_code)
    {
        if (NULL == msg_body || msg_body_len == 0)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "录像业务, INVITE回应消息处理失败:逻辑设备ID=%s, 原因=%s", pCrData->callee_id, (char*)"200回应消息体里面没有SDP信息");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"200 response mesaage body do not have SDP message");

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_SDP_GET_VIDEO_INFO_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() NULL == msg_body || msg_body_len == 0  \r\n");
            return -1;
        }

        /* 获取200消息中的被叫的sdp信息 */
        i = sdp_message_init(&pRemoteSDP);

        if (0 != i)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "录像业务, INVITE回应消息处理失败:逻辑设备ID=%s, 原因=%s", pCrData->callee_id, (char*)"SDP信息初始化失败");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"SDP info initialization failed");

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_SDP_MSG_INIT_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() exit---: Remote SDP Message Init Error \r\n");
            return -1;
        }

        i = sdp_message_parse(pRemoteSDP, msg_body); /*parse body */

        if (0 != i)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "录像业务, INVITE回应消息处理失败:逻辑设备ID=%s, 原因=%s", pCrData->callee_id, (char*)"解析200回应消息体里面SDP信息失败");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"Analysis SDP message in 200 response message body failed");

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() RemoveDeviceRecordInfo:cr_pos=%d \r\n", cr_pos);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_SDP_MSG_PARSE_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            sdp_message_free(pRemoteSDP);
            pRemoteSDP = NULL;

            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() exit---: Remote SDP Message Parse Error \r\n");
            return -1;
        }

        /* 获取协商的SDP信息 */
        memset(&stRemoteSDPParam, 0, sizeof(sdp_param_t));
        memset(&stRemoteSDPExParam, 0, sizeof(sdp_extend_param_t));

        i = SIP_GetSDPInfoEx(pRemoteSDP, &stRemoteSDPParam, &stRemoteSDPExParam);
        //printf_system_time();
        printf("record_invite_response_msg_proc() SIP_GetSDPInfoEx:i=%d\r\n", i);

        if (0 != i)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "录像业务, INVITE回应消息处理失败:逻辑设备ID=%s, 原因=%s", pCrData->callee_id, (char*)"获取SDP中的信息失败");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s", pCrData->callee_id, (char*)"Access info in SDP failed");

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_SDP_GENERAL_MSG_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            sdp_message_free(pRemoteSDP);
            pRemoteSDP = NULL;

            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() exit---: Get SDP Video Info Error \r\n");
            return -1;
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() addr=%s,port=%d,code=%d,flag=%d \r\n", stRemoteSDPParam.sdp_ip, stRemoteSDPParam.video_port, stRemoteSDPParam.video_code_type, stRemoteSDPParam.media_direction);
        //printf_system_time();
        printf("record_invite_response_msg_proc() addr=%s,port=%d,code=%d,flag=%d \r\n", stRemoteSDPParam.sdp_ip, stRemoteSDPParam.video_port, stRemoteSDPParam.video_code_type, stRemoteSDPParam.media_direction);

        /* 判断前端设备的ONVIF URL */
        if (stRemoteSDPExParam.onvif_url[0] != '\0')
        {
            if (strlen(stRemoteSDPExParam.onvif_url) < 255)
            {
                osip_strncpy(pCrData->callee_onvif_url, stRemoteSDPExParam.onvif_url, strlen(stRemoteSDPExParam.onvif_url));
            }
            else
            {
                osip_strncpy(pCrData->callee_onvif_url, stRemoteSDPExParam.onvif_url, 255);
            }

            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() callee_onvif_url=%s \r\n", pCrData->callee_onvif_url);

            /* 被叫侧的协议类型改为RTSP */
            //pCrData->callee_transfer_type = TRANSFER_PROTOCOL_RTSP;
        }

        /* 添加被叫的SDP 信息 */
        osip_strncpy(pCrData->callee_sdp_ip, stRemoteSDPParam.sdp_ip, MAX_IP_LEN);

        pCrData->callee_sdp_port = stRemoteSDPParam.video_port;
        pCrData->tsu_code = stRemoteSDPParam.video_code_type;

        /* 通知TSU开始录像*/
        iRet = notify_tsu_add_record_task(pCrData, 1, pRecordInfo->record_type, pRecordInfo->record_days, pRecordInfo->bandwidth);
        //printf_system_time();
        printf("record_invite_response_msg_proc() notify_tsu_add_record_task:i=%d\r\n", i);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() notify_tsu_add_record_task Error: TSU IP=%s, iRet=%d \r\n", pCrData->tsu_ip, iRet);
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "录像业务, INVITE回应消息处理失败:逻辑设备ID=%s, 原因=%s, TSU IP=%s, iRet=%d", pCrData->callee_id, (char*)"通知TSU添加录像任务失败", pCrData->tsu_ip, iRet);
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s, TSU IP=%s, iRet=%d", pCrData->callee_id, (char*)"Notify TSU to add video task failed", pCrData->tsu_ip, iRet);

            if (iRet == -2) /* TSU 录像路数达到上限 */
            {
                i = SetTSUStatus(pCrData->tsu_resource_index, 2);
                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SetTSUStatus:tsu_resource_index=%d, i=%d \r\n", pCrData->tsu_resource_index, i);
            }
            else if (iRet == -3) /* TSU 没有获取到索引，再设置一下 */
            {
                i = set_tsu_index_id2(pCrData->tsu_resource_index);
                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() set_tsu_index_id2:tsu_resource_index=%d, i=%d \r\n", pCrData->tsu_resource_index, i);
            }

            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_TSU_NOTIFY_ADD_TRANSFER_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            if (iRet == -2) /* 将录像信息的状态设置为没有空闲的TSU */
            {
                pRecordInfo->record_status = RECORD_STATUS_NO_TSU;
            }

            sdp_message_free(pRemoteSDP);
            pRemoteSDP = NULL;
            return -1;
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "录像业务, INVITE回应消息处理:逻辑设备ID=%s, 通知TSU添加录像任务成功, TSU IP=%s, task_id=%s, iRet=%d", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id, iRet);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Video business, INVITE response message processing: logical device ID = % s, notify the TSU add video mission success, TSU IP = % s, task_id = % s, iRet = % d", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id, iRet);
        }

        /* 如果不是全周录像，则要发送暂停命令给TSU */
        if (1 != pRecordInfo->TimeOfAllWeek)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "录像业务, INVITE回应消息处理: 该录像是时间段录像, 通知TSU暂停录像: 点位ID=%s, TSU IP地址=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "INVITE response message process: the video is time slot record video, notify TSU to pause video: point ID=%s, TSU IP address=%s, task_id=%s", pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id);

            i = notify_tsu_pause_record(pCrData->tsu_ip, pCrData->task_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() notify_tsu_pause_record Error: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
                pRecordInfo->iTSUPauseStatus = 0;
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() notify_tsu_pause_record OK: tsu_ip=%s, task_id=%s, i=%d", pCrData->tsu_ip, pCrData->task_id, i);
                pRecordInfo->iTSUPauseStatus = 1;
            }

            /* 去除策略中的标志位 */
            i = RemoveRecordTimeSchedMark(pRecordInfo->uID);
        }
        else
        {
            pRecordInfo->iTSUResumeStatus = 1;
        }

        /* 加入待发送ACK 消息队列 */
        if (ua_dialog_index == pCrData->callee_ua_index)
        {
            i = ack_send_use(cr_pos, -1, ua_dialog_index);
            printf("record_invite_response_msg_proc() ack_send_use:cr_pos=%d, callee_ua_index=%d \r\n", cr_pos, ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() ack_send_use:cr_pos=%d, callee_ua_index=%d \r\n", cr_pos, ua_dialog_index);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "录像业务, INVITE回应消息处理, 添加等待TSU通知任务创建结果消息处理结果事件, 等待发送ACK消息:cr_pos=%d, callee_ua_index=%d, iRet=%d", cr_pos, ua_dialog_index, i);
        }
        else
        {
            //i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() not ack_send_use:cr_pos=%d, caller_ua_index=%d, callee_ua_index=%d, ua_dialog_index=%d \r\n", cr_pos, pCrData->caller_ua_index, pCrData->callee_ua_index, ua_dialog_index);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "录像业务, INVITE回应消息处理失败, 原因=CrData中没有找到收到的ua_dialog_index:cr_pos=%d, callee_ua_index=%d, ua_dialog_index=%d", cr_pos, pCrData->callee_ua_index, ua_dialog_index);

            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendAck:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 通知TSU停止接收码流*/
            i = notify_tsu_delete_record_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "record_invite_response_msg_proc() notify_tsu_delete_record_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() notify_tsu_delete_record_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

            /* 设置TSU 状态 */
            i = SetTSUStatus(pCrData->tsu_resource_index, 1);
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() SetTSUStatus:tsu_resource_index=%d, i=%d \r\n", pCrData->tsu_resource_index, i);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_RECORD_FIND_RECORD_INFO_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            return -1;
        }

        pRecordInfo->record_status = RECORD_STATUS_COMPLETE;
        sdp_message_free(pRemoteSDP);
        pRemoteSDP = NULL;

        /* 通知等待的呼叫任务，接受请求，通知TSU转发码流 */
        i = resumed_wait_answer_call_record2(pCrData, pRecordInfo->record_type);

        DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() record_info_index=%d, device_index=%u, stream_type=%d, record_status=%d \r\n", record_info_index, pRecordInfo->stream_type, pRecordInfo->record_status);
        //printf_system_time();
        printf("record_invite_response_msg_proc() record_info_index=%d, device_index=%u, stream_type=%d, record_status=%d \r\n", record_info_index, pRecordInfo->stream_type, pRecordInfo->record_status);

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "录像业务, INVITE回应消息处理成功:逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);
    }
    else
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "录像业务, INVITE回应消息处理失败:逻辑设备ID=%s, 原因=%s, 错误码=%d", pCrData->callee_id, (char*)"非200的错误响应消息", response_code);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "INVITE response message process failed: logic deviceID=%s, cause=%s, error code=%d", pCrData->callee_id, (char*)"non 200 fault response message息", response_code);

        i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_invite_response_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

        i = return_error_for_wait_answer_call_record(pCrData, response_code);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        /* 如果是408的错误，可能网络不通，这个时候将点位设置为掉线 */
        if (408 == response_code)
        {
            pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(pCrData->callee_id);

            if (NULL != pCalleeGBLogicDeviceInfo)
            {
                pCalleeGBLogicDeviceInfo->status = 3;
                SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "逻辑设备ID=%s, 名称=%s, 网络暂时不可达", pCalleeGBLogicDeviceInfo->device_id, pCalleeGBLogicDeviceInfo->device_name);
                EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Logical device ID=%s, name =%s, the network is temporarily not reachable", pCalleeGBLogicDeviceInfo->device_id, pCalleeGBLogicDeviceInfo->device_name);

                DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "record_invite_response_msg_proc() device_id=%s, NetWork Unreached \r\n", pCalleeGBLogicDeviceInfo->device_id);
            }
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_invite_response_msg_proc() response_code=%d \r\n", response_code);
        return -1;
    }

    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_invite_response_msg_proc() End--- :callee_id=%s, callee_ip=%s \r\n", pCrData->callee_id, pCrData->callee_ip);
    //printf_system_time();
    printf("record_invite_response_msg_proc() Exit--- :callee_id=%s, callee_ip=%s \r\n", pCrData->callee_id, pCrData->callee_ip);
    return 0;
}

/*****************************************************************************
 函 数 名  : record_bye_msg_proc
 功能描述  : 录像BYE 消息处理
 输入参数  : char* caller_id
                            char* callee_id
                            char* call_id
                            int ua_dialog_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int record_bye_msg_proc(int cr_pos, int ua_dialog_index)
{
    int i = 0;
    cr_t* pCrData = NULL;

    /* 查找呼叫记录信息 */
    if (cr_pos < 0)
    {
        SIP_AnswerToBye(ua_dialog_index, 481, NULL);
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_bye_msg_proc() exit---: Find Record Srv Error \r\n");
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        SIP_AnswerToBye(ua_dialog_index, 481, NULL);
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_bye_msg_proc() exit---: Get Record Srv Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_bye_msg_proc():callee_id=%s, callee_ip=%s \r\n", pCrData->callee_id, pCrData->callee_ip);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "录像业务, 收到前端BYE消息处理, 录像视频关闭:逻辑设备ID=%s, IP地址=%s", pCrData->callee_id, pCrData->callee_ip);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Video service, received a front-end BYE message processing, video video off: logic device ID=%s, IP address =%s", pCrData->callee_id, pCrData->callee_ip);

    /* 回应200 */
    SIP_AnswerToBye(ua_dialog_index, 200, NULL);

    /* 通知TSU停止接收码流*/
    i = notify_tsu_delete_record_task(pCrData->tsu_ip, pCrData->task_id);

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "record_bye_msg_proc() notify_tsu_delete_record_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_bye_msg_proc() notify_tsu_delete_record_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
    }

    /* 发送Bye 给其他正在使用录像业务的主叫侧用户*/
    i = send_bye_to_all_other_caller_by_callee_id_and_streamtype(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_bye_msg_proc() send_bye_to_all_other_caller_by_callee_id_and_streamtype:callee_id=%s, callee_stream_type=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, i);

    i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_bye_msg_proc() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

    /* 设置TSU 状态 */
    i = SetTSUStatus(pCrData->tsu_resource_index, 1);
    DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "record_bye_msg_proc() SetTSUStatus:tsu_resource_index=%d, i=%d \r\n", pCrData->tsu_resource_index, i);

    i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
    i = call_record_remove(cr_pos);

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_bye_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_bye_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : record_bye_response_msg_proc
 功能描述  : 录像BYE 回应消息处理
 输入参数  : char* caller_id
                            char* callee_id
                            char* call_id
                            int ua_dialog_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int record_bye_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code)
{
    int i = 0;
    cr_t* pCrData = NULL;

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_bye_response_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_bye_response_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_bye_response_msg_proc():callee_id=%s, callee_ip=%s \r\n", pCrData->callee_id, pCrData->callee_ip);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "录像业务, 收到BYE响应消息处理, 录像视频关闭:逻辑设备ID=%s, IP地址=%s, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, cr_pos);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Record business, received the BYE response message processing, video video closed: logical device ID = % s, IP address = % s, cr_pos = % d", pCrData->callee_id, pCrData->callee_ip, cr_pos);

    if (pCrData->callee_ua_index == ua_dialog_index)    /* 主叫发送的Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "录像业务, 收到主叫BYE响应消息处理, 录像视频关闭:逻辑设备ID=%s, cr_pos=%d", pCrData->callee_id, cr_pos);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Record business, received calling BYE response message processing, video video closed: logical device ID = %s, cr_pos=%d", pCrData->callee_id, cr_pos);

        if (pCrData->caller_ua_index >= 0)
        {
            SIP_AnswerToBye(pCrData->caller_ua_index, response_code, NULL);
        }
    }
    else if (pCrData->caller_ua_index == ua_dialog_index) /* 被叫发送的Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "录像业务, 收到前端叫BYE响应消息处理, 录像视频关闭:逻辑设备ID=%s, cr_pos=%d", pCrData->callee_id, cr_pos);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Record business, receive front-end call BYE response message processing, video video closed: logical device ID = % s, cr_pos = %d", pCrData->callee_id, cr_pos);

        if (pCrData->callee_ua_index >= 0)
        {
            SIP_AnswerToBye(pCrData->callee_ua_index, response_code, NULL);
        }
    }

    /* 移除呼叫记录信息 */
    i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
    i = call_record_remove(cr_pos);

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "record_bye_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "record_bye_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : StartDeviceRecord
 功能描述  : 启动设备录像任务
 输入参数  : int device_index
                            int storagelife
                            int bandwidth
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月29日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int StartDeviceRecord(record_info_t* pProcRecordInfo)
{
    int i = 0;
    int tsu_index = -1;
    int index = 0;
    int cr_pos = -1;
    int record_info_index = -1;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    char* device_ip = NULL;
    int device_port = 0;
    char* sdp_ssrc = NULL;
    sdp_message_t* local_sdp = NULL;
    sdp_param_t sdp_param;
    int recv_port = 0;
    tsu_resource_info_t* pTsuResourceInfo = NULL;
    char* tsu_ip = NULL;
    cr_t* pCrData = NULL;
    int has_record_pos = -1;
    record_info_t* pHasRecordInfo = NULL;
    record_info_t* pRecordInfo = NULL;
    char* sdp_tsu_ip = NULL;

    if (NULL == pProcRecordInfo)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "StartDeviceRecord() exit---: Param Error \r\n");
        return -1;
    }

    printf("\r\nStartDeviceRecord() Start---: device_index=%u \r\n", pProcRecordInfo->device_index);

    /* 根据码流查看是否已经有前端接入的录像流 */
    has_record_pos = has_record_info_find_by_stream_type(pProcRecordInfo->device_index, pProcRecordInfo->stream_type, pProcRecordInfo->record_type);

    if (has_record_pos >= 0) /* 同一种码流类型，不同的录像类型，需要重新定义策略，不能再从前端取流 */
    {
        pHasRecordInfo = record_info_get(has_record_pos);

        if (NULL != pHasRecordInfo)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "启动的录像流类型已经在录像:逻辑设备Index=%u, 码流类型=%d, 录像类型=%d, 已录像的录像类型=%d", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Start the record stream type has been in the video: logical device Index = % u, code flow type = % d, video type = % d, have video video type = %d", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() Device Has Recording Proc:device_index=%u,stream_type=%d,record_type=%d, has_record_type=%d  \r\n", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);

            if (RECORD_STATUS_INIT == pHasRecordInfo->record_status)
            {
                SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动的录像流类型已经在录像，录像没有成功:逻辑设备Index=%u, 码流类型=%d, 录像类型=%d, 已录像的录像类型=%d", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);
                EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Video streaming has been started. The video was not successful: the logic device Index=%u, stream type =%d, video type =%d, recorded video type =%d", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);

                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: Device Has Recording Proc:device_index=%u,stream_type=%d,record_type=%d, has_record_type=%d  \r\n", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);
                return -1;
            }
            else if (RECORD_STATUS_PROC == pHasRecordInfo->record_status)
            {
                SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动的录像流类型已经在录像，录像流程还没结束:逻辑设备Index=%u, 码流类型=%d, 录像类型=%d, 已录像的录像类型=%d", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);
                EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Video streaming has been started, The video is not over yet: the logic device Index=%u,  stream type =%d,  video type =%d, recorded video type =%d", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);

                DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: Device Has Recording Proc:device_index=%u,stream_type=%d,record_type=%d, has_record_type=%d  \r\n", pHasRecordInfo->device_index, pHasRecordInfo->stream_type, pHasRecordInfo->record_type, pHasRecordInfo->record_type);
                return -1;
            }
            else
            {
                pProcRecordInfo->record_cr_index = pHasRecordInfo->record_cr_index;
                pProcRecordInfo->record_status = pHasRecordInfo->record_status;
                pProcRecordInfo->tsu_index = pHasRecordInfo->tsu_index;

                SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_WARNING, "启动的录像流类型已经在录像:逻辑设备Index=%u, 码流类型=%d, 录像类型=%d, 已录像的录像类型=%d", pProcRecordInfo->device_index, pProcRecordInfo->stream_type, pProcRecordInfo->record_type, pHasRecordInfo->record_type);
                EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_WARNING, "Video streaming has been launched in the video: logic device Index=%u, stream type =%d, video type =%d, recorded video type=%d", pProcRecordInfo->device_index, pProcRecordInfo->stream_type, pProcRecordInfo->record_type, pHasRecordInfo->record_type);

                DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "StartDeviceRecord() exit---: Device Has Recording:device_index=%u,stream_type=%d,record_type=%d, has_record_type=%d  \r\n", pProcRecordInfo->device_index, pProcRecordInfo->stream_type, pProcRecordInfo->record_type, pHasRecordInfo->record_type);
                return pProcRecordInfo->record_cr_index;
            }
        }
    }

    printf("\r\nStartDeviceRecord() has_record_info_find_by_stream_type End--- \r\n");

    /* 查找逻辑设备信息 */
    pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(pProcRecordInfo->device_index);   /* 逻辑设备锁 */

    if (NULL == pGBLogicDeviceInfo)
    {
        if (pProcRecordInfo->record_try_count >= 3)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备Index=%d，原因=%s", pProcRecordInfo->device_index, (char*)"获取逻辑设备的逻辑设备信息失败");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pProcRecordInfo->device_index, (char*)"Access logic device info of logic device failed.");
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: GBLogicDevice_info_find_by_device_index Error:device_index=%u \r\n", pProcRecordInfo->device_index);
        return -1;
    }

    printf("\r\nStartDeviceRecord() GBLogicDevice_info_find_by_device_index End--- \r\n");

#if 0

    /* 判断逻辑设备的录像类型 */
    if (1 == pGBLogicDeviceInfo->record_type)
    {
        if (pProcRecordInfo->record_try_count >= 3)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备Index=%d，原因=%s", pProcRecordInfo->device_index, (char*)"逻辑设备配置为前端录像");
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "StartDeviceRecord() exit---: GBLogicDevice Not Config Record In CMS:device_index=%d \r\n", pProcRecordInfo->device_index);
        return -1;
    }

#endif

    //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() device_id=%s \r\n", pGBLogicDeviceInfo->device_id);
    //printf("StartDeviceRecord() device_id=%s \r\n", pGBLogicDeviceInfo->device_id);
    //LogRunTraceToFile("开始启动录像:逻辑设备ID=%s\r\n", pGBLogicDeviceInfo->device_id);

    if (0 == pGBLogicDeviceInfo->status)
    {
        if (pProcRecordInfo->record_try_count >= 3)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"逻辑设备不在线");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Logic device is not online.");
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: GBlogic Device Not Online \r\n");
        return -2;
    }
    else if (2 == pGBLogicDeviceInfo->status)
    {
        if (pProcRecordInfo->record_try_count >= 3)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"逻辑设备没有视频流");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Logic device do not have video stream.");
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: GBlogic Device No Stream \r\n");
        return -3;
    }
    else if (3 == pGBLogicDeviceInfo->status)
    {
        if (pProcRecordInfo->record_try_count >= 3)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"逻辑设备网络不可达");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Logic device network is not accessble");
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: GBlogic Device Unreached \r\n");
        return -5;
    }

    if (pProcRecordInfo->stream_type == EV9000_STREAM_TYPE_SLAVE && pGBLogicDeviceInfo->stream_count == 1)
    {
        if (pProcRecordInfo->record_try_count >= 3)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"逻辑设备不支持多码流");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Logic device do not support mult stream");
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: GBlogic Device Not Support Muilt Stream \r\n");
        return -6;
    }

    /* 获取物理设备 */
    pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, pProcRecordInfo->stream_type);

    if (NULL == pGBDeviceInfo)
    {
        if (pProcRecordInfo->record_try_count >= 3)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s, 媒体流类型=%d", pGBLogicDeviceInfo->device_id, (char*)"获取逻辑设备物理设备错误", pProcRecordInfo->stream_type);
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s, media straeam type=%d", pGBLogicDeviceInfo->device_id, (char*)"Access logic device physical device failed", pProcRecordInfo->stream_type);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: Get Device Port Error \r\n");
        return -1;
    }

    printf("\r\nStartDeviceRecord() GBDevice_info_get_by_stream_type End--- \r\n");

    device_ip = pGBDeviceInfo->login_ip;
    device_port = pGBDeviceInfo->login_port;

    /* 申请呼叫资源*/
    cr_pos = call_record_add();
    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_add:device_id=%s, cr_pos=%d \r\n", pGBLogicDeviceInfo->device_id, cr_pos);

    if (cr_pos < 0)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"获取可用的呼叫资源失败");
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available call resource failed");

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: Call Record Add Error \r\n");
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (pCrData == NULL)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"获取可用的呼叫资源失败");
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available call resource failure");

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: Get Transfer Info Error \r\n");
        return -1;
    }

    printf("\r\nStartDeviceRecord() call_record_add End--- \r\n");

    record_info_index = record_info_find_by_cr_index(cr_pos);  /*  查找这个cr_pos是否被占用了，如果占用了，释放掉 */

    if (record_info_index >= 0)
    {
        pRecordInfo = record_info_get(record_info_index);

        if (NULL != pRecordInfo)
        {
            pRecordInfo->tsu_index = -1;
            pRecordInfo->record_cr_index = -1;
            pRecordInfo->record_status = RECORD_STATUS_INIT;
            pRecordInfo->record_start_time = 0;
            pRecordInfo->record_try_count = 0;
            pRecordInfo->record_retry_interval = 5;
            pRecordInfo->iTSUPauseStatus = 0;
            pRecordInfo->iTSUResumeStatus = 0;
            pRecordInfo->iTSUAlarmRecordStatus = 0;
            //printf("StartDeviceRecord() get_idle_tsu_by_resource_balance_for_record: RecordInfo: device_index=%u, record_type=%d, stream_type=%d tsu_index reset \r\n", pRecordInfo->device_index, pRecordInfo->record_type, pRecordInfo->stream_type);
        }
    }

    printf("\r\nStartDeviceRecord() record_info_find_by_cr_index End--- \r\n");

    /* 如果原来有业务，先停止掉 */
    if (pProcRecordInfo->record_cr_index >= 0)
    {
        i = StopDeviceRecord(pProcRecordInfo->record_cr_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() StopDeviceRecord: record_cr_index=%d, i=%d \r\n", pProcRecordInfo->record_cr_index, i);
    }

    pCrData->call_type = CALL_TYPE_RECORD;

    if (pProcRecordInfo->assign_record)
    {
        tsu_index = tsu_resource_info_find_by_index(pProcRecordInfo->assign_tsu_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() tsu_resource_info_find_by_index: assign_tsu_index=%d, tsu_index=%d \r\n", pProcRecordInfo->assign_tsu_index, tsu_index);
    }
    else
    {
        /* 获取空闲的TSU资源 */
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "StartDeviceRecord() get_idle_tsu_by_resource_balance_for_record: Begin--- \r\n", tsu_index);
        tsu_index = get_idle_tsu_by_resource_balance_for_record(pProcRecordInfo->record_days, pProcRecordInfo->bandwidth);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "StartDeviceRecord() get_idle_tsu_by_resource_balance_for_record: End--- tsu_index=%d \r\n", tsu_index);
    }

    if (tsu_index < 0)
    {
        if (-2 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"查找空闲的录像TSU资源索引失败,TSU资源队列为空");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-3 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"查找空闲的录像TSU资源索引失败,TSU资源信息错误");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-4 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"查找空闲的录像TSU资源索引失败,TSU资源都没有启用");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-5 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"查找空闲的录像TSU资源索引失败,TSU资源都不在线");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-6 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"查找空闲的录像TSU资源索引失败,TSU资源都用于指定录像");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-7 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"查找空闲的录像TSU资源索引失败,TSU资源都已经达到上限");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-8 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"查找空闲的录像TSU资源索引失败,TSU资源都没有挂载磁阵");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-9 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"查找空闲的录像TSU资源索引失败,通过ICE获取所有的TSU资源状态失败");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else if (-10 == tsu_index)
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"查找空闲的录像TSU资源索引失败,TSU资源都没有用于指定录像");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }
        else
        {
            SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"查找空闲的录像TSU资源索引失败");
            EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: Find Idle TSU Error \r\n");
        return -4;
    }

    printf("\r\nStartDeviceRecord() get_idle_tsu_by_resource_balance_for_record End--- \r\n");

    pProcRecordInfo->tsu_index = tsu_index;

    pTsuResourceInfo = tsu_resource_info_get(tsu_index);

    if (NULL == pTsuResourceInfo)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s, tsu_index=%d", pGBLogicDeviceInfo->device_id, (char*)"获取录像TSU资源失败", tsu_index);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource failed");

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: tsu_resource_info_get Error \r\n");
        return -4;
    }

    printf("\r\nStartDeviceRecord() tsu_resource_info_get End--- \r\n");

    if (0 == pTsuResourceInfo->iStatus)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s, TSU ID=%s", pGBLogicDeviceInfo->device_id, (char*)"TSU不在线", pTsuResourceInfo->tsu_device_id);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s, TSU ID=%s", pGBLogicDeviceInfo->device_id, (char*)"TSU not online", pTsuResourceInfo->tsu_device_id);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: TSU Status Error 0 \r\n");
        return -4;
    }

    if (2 == pTsuResourceInfo->iStatus)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s, TSU ID=%s", pGBLogicDeviceInfo->device_id, (char*)"TSU资源达到上限", pTsuResourceInfo->tsu_device_id);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s, TSU ID=%s", pGBLogicDeviceInfo->device_id, (char*)"TSU resource reach upper limit", pTsuResourceInfo->tsu_device_id);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: TSU Status Error 2 \r\n");
        return -4;
    }

    if (pProcRecordInfo->assign_record && pTsuResourceInfo->iTsuType != 1)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"指定录像点位的指定TSU没有设置为指定录像TSU");
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"The specified TSU is not set to the specified video TSU");

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: TSU assign_record \r\n");
        return -4;
    }

    /* 获取和TSU通信的IP地址 */
    tsu_ip = get_tsu_ip(pTsuResourceInfo, default_eth_name_get());

    if (NULL == tsu_ip)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"获取TSU IP地址失败");
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Access TSU IP address failed");

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: get_tsu_ip Error \r\n");
        return -4;
    }

    printf("\r\nStartDeviceRecord() get_tsu_ip End--- \r\n");

    /* 获取TSU 接收端口号 */
    recv_port = get_recv_port_by_tsu_resource(tsu_ip);

    if (recv_port <= 0)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s, tsu_ip=%s", pGBLogicDeviceInfo->device_id, (char*)"获取可用的TSU资接收端口失败", tsu_ip);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s, tsu_ip=%s", pGBLogicDeviceInfo->device_id, (char*)"Access to available TSU resource receive port failed", tsu_ip);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: get_recv_port_by_tsu_resource Error \r\n");
        return -4;
    }

    printf("\r\nStartDeviceRecord() get_recv_port_by_tsu_resource End--- \r\n");

    /* 添加转发信息 */
    /* TSU的转发资源 */
    osip_strncpy(pCrData->tsu_device_id, pTsuResourceInfo->tsu_device_id, MAX_ID_LEN);
    pCrData->tsu_resource_index = tsu_index;
    pCrData->tsu_recv_port = recv_port;
    osip_strncpy(pCrData->tsu_ip, tsu_ip, MAX_IP_LEN);

    i = TSUResourceIPAddrListClone(pTsuResourceInfo->pTSUIPAddrList, pCrData);

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: TSU IP Addr List Clone Error \r\n");
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"添加本地TSU IP地址失败");
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"add local TSU IP address failed");

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: TSU IP Addr List Clone Error \r\n");
        return -1;
    }

    printf("\r\nStartDeviceRecord() TSUResourceIPAddrListClone End--- \r\n");

    /* 主叫测 */
    osip_strncpy(pCrData->caller_id, local_cms_id_get(), MAX_ID_LEN);
    osip_strncpy(pCrData->caller_ip, pGBDeviceInfo->strRegServerIP, MAX_IP_LEN);
    pCrData->caller_port = pGBDeviceInfo->iRegServerPort;
    osip_strncpy(pCrData->caller_server_ip_ethname, pGBDeviceInfo->strRegServerEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->caller_server_ip, pGBDeviceInfo->strRegServerIP, MAX_IP_LEN);
    pCrData->caller_server_port = pGBDeviceInfo->iRegServerPort;

    if (1 == pGBDeviceInfo->trans_protocol)
    {
        pCrData->caller_transfer_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        pCrData->caller_transfer_type = TRANSFER_PROTOCOL_UDP; /* 默认UDP */
    }

    /* 主叫侧SDP 信息 */
    sdp_tsu_ip = get_cr_sdp_tsu_ip(pCrData, pCrData->caller_server_ip_ethname);

    if (NULL == sdp_tsu_ip)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s, caller_server_ip_ethname=%s", pGBLogicDeviceInfo->device_id, (char*)"获取主叫侧的SDP信息中的TSU的IP地址失败", pCrData->caller_server_ip_ethname);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s, caller_server_ip_ethname=%s", pGBLogicDeviceInfo->device_id, (char*)"Get the calling side TSU IP address in SDP information failed", pCrData->caller_server_ip_ethname);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: Get Caller TSU SDP IP Error:caller_server_ip_ethname=%s \r\n", pCrData->caller_server_ip_ethname);
        return -1;
    }

    printf("\r\nStartDeviceRecord() get_cr_sdp_tsu_ip caller_server_ip_ethname End--- \r\n");

    osip_strncpy(pCrData->caller_sdp_ip, sdp_tsu_ip, MAX_IP_LEN);
    pCrData->caller_sdp_port = recv_port;

    /* 被叫侧 */
    osip_strncpy(pCrData->callee_id, pGBLogicDeviceInfo->device_id, MAX_ID_LEN);
    osip_strncpy(pCrData->callee_ip, device_ip, MAX_IP_LEN);
    pCrData->callee_port = device_port;
    osip_strncpy(pCrData->callee_server_ip_ethname, pGBDeviceInfo->strRegServerEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->callee_server_ip, pGBDeviceInfo->strRegServerIP, MAX_IP_LEN);
    pCrData->callee_server_port = pGBDeviceInfo->iRegServerPort;
    pCrData->callee_framerate = pGBLogicDeviceInfo->frame_count;
    pCrData->callee_stream_type = pProcRecordInfo->stream_type;
    pCrData->callee_gb_device_type = pGBDeviceInfo->device_type;

    if (1 == pGBDeviceInfo->trans_protocol)
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_UDP; /* 默认UDP */
    }

    /* 组建本地SDP信息*/
    memset(&sdp_param, 0, sizeof(sdp_param_t));
    osip_strncpy(sdp_param.o_username, local_cms_id_get(), 32);
    osip_strncpy(sdp_param.s_name, (char*)"Play", 32);

    sdp_tsu_ip = get_cr_sdp_tsu_ip(pCrData, pCrData->callee_server_ip_ethname);

    if (NULL == sdp_tsu_ip)
    {
        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s, callee_server_ip_ethname=%s", pGBLogicDeviceInfo->device_id, (char*)"获取被叫侧的SDP信息中的TSU的IP地址失败", pCrData->callee_server_ip_ethname);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s, callee_server_ip_ethname=%s", pGBLogicDeviceInfo->device_id, (char*)"Get the called side TSU IP address in SDP information failed", pCrData->callee_server_ip_ethname);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: Get Callee TSU SDP IP Error:callee_server_ip_ethname=%s \r\n", pCrData->callee_server_ip_ethname);
        return -1;
    }

    printf("\r\nStartDeviceRecord() get_cr_sdp_tsu_ip callee_server_ip_ethname End--- \r\n");

    osip_strncpy(sdp_param.sdp_ip, sdp_tsu_ip, MAX_IP_LEN);
    sdp_param.video_port = pCrData->tsu_recv_port;
    sdp_param.video_code_type = -1;
    sdp_param.media_direction = MEDIA_DIRECTION_TYPE_RECVONLY;
    sdp_param.stream_type = pCrData->callee_stream_type;

    /* 传输方式赋值 */
    if (TRANSFER_PROTOCOL_TCP == pCrData->callee_transfer_type)
    {
        sdp_param.trans_type = 2;
    }
    else
    {
        sdp_param.trans_type = 1;
    }

    i = SIP_BuildSDPInfoEx(&local_sdp, &sdp_param, NULL);

    if (0 != i)
    {
        sdp_message_free(local_sdp);
        local_sdp = NULL;

        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s", pGBLogicDeviceInfo->device_id, (char*)"构建本地SDP信息失败");
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Create local SDP info failed");

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() exit---: SDP Build Offer Error:tsu_port=%d \r\n", pCrData->tsu_recv_port);
        return -1;
    }

    printf("\r\nStartDeviceRecord() SIP_BuildSDPInfoEx End--- \r\n");

#if 1
    sdp_ssrc = sdp_message_y_ssrc_get(local_sdp);

    if (NULL == sdp_ssrc)
    {
        sdp_ssrc = osip_getcopy((char*)"0100000001");
        sdp_message_y_ssrc_set(local_sdp, sdp_ssrc);
    }

#endif

    printf("\r\nStartDeviceRecord() sdp_message_y_ssrc_get End--- \r\n");

    index = SIP_SendInvite(local_cms_id_get(), pGBLogicDeviceInfo->device_id, pCrData->callee_server_ip, pCrData->callee_server_port, device_ip, device_port, NULL, NULL, local_sdp);
    DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() SIP_SendInvite:device_id=%s, index=%d \r\n", pGBLogicDeviceInfo->device_id, index);

    if (index < 0)
    {
        sdp_message_free(local_sdp);
        local_sdp = NULL;

        SystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "启动录像失败:逻辑设备ID=%s，原因=%s, ua_index=%d", pGBLogicDeviceInfo->device_id, (char*)"发送SIP Invite消息失败", index);
        EnSystemLog(EV9000_CMS_START_RECORD_ERROR, EV9000_LOG_LEVEL_ERROR, "Start video recording failed: logic device ID =%s, cause=%s", pGBLogicDeviceInfo->device_id, (char*)"Send SIP Invite message failed");

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StartDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        printf("\r\nStartDeviceRecord() exit---: SIP_SendInvite Error:index=%d \r\n", index);
        return -1;
    }

    printf("\r\nStartDeviceRecord() SIP_SendInvite End--- \r\n");

    /* 添加其他信息到录像业务记录*/
    pCrData->callee_ua_index = index;

    /* 添加录像信息到TSU上 */
    i = AddRecordInfoToTSU(pTsuResourceInfo, pProcRecordInfo);

    printf("\r\nStartDeviceRecord() AddRecordInfoToTSU End--- \r\n");

    pProcRecordInfo->record_status = RECORD_STATUS_PROC;
    pProcRecordInfo->record_start_time = time(NULL);
    pProcRecordInfo->record_cr_index = cr_pos;

    sdp_message_free(local_sdp);
    local_sdp = NULL;

    //DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StartDeviceRecord() device_id=%s, device_index=%u, stream_type=%d, record_status=%d, cr_pos=%d, caller_ua_index=%d, callee_ua_index=%d \r\n", pGBLogicDeviceInfo->device_id, pProcRecordInfo->device_index, pProcRecordInfo->stream_type, pProcRecordInfo->record_status, pProcRecordInfo->record_cr_index, pCrData->caller_ua_index, pCrData->callee_ua_index);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "启动录像成功:逻辑设备ID=%s, 逻辑设备名称=%s, 逻辑设备IP地址=%s, 录像类型=%d, 码流类型=%d, 所选的TSU IP=%s, callee_ua_index=%d, cr_pos=%d, 录像策略索引=%u", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, device_ip, pProcRecordInfo->record_type, pProcRecordInfo->stream_type, tsu_ip, index, cr_pos, pProcRecordInfo->uID);

    printf("\r\nStartDeviceRecord() exit OK --- \r\n");
    return cr_pos;
}

/*****************************************************************************
 函 数 名  : StopDeviceRecord
 功能描述  : 停止设备录像任务
 输入参数  : int record_cr_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月29日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int StopDeviceRecord(int record_cr_index)
{
    int i = 0;
    cr_t* pCrData = NULL;

    pCrData = call_record_get(record_cr_index);

    if (NULL != pCrData)
    {
        if (pCrData->call_type != CALL_TYPE_RECORD)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StopDeviceRecord() exit---: call_type Error:record_cr_index=%d, call_type=%d \r\n", record_cr_index, pCrData->call_type);
            return -1;
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "停止录像:逻辑设备ID=%s", pCrData->callee_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Stop video: logical device ID = % s", pCrData->callee_id);
        DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StopDeviceRecord() :device_id=%s, device_ip=%s, record_cr_index=%d \r\n", pCrData->callee_id, pCrData->callee_ip, record_cr_index);
        printf("StopDeviceRecord() :device_id=%s, device_ip=%s, record_cr_index=%d \r\n", pCrData->callee_id, pCrData->callee_ip, record_cr_index);

        i = SIP_SendBye(pCrData->callee_ua_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "StopDeviceRecord() SIP_SendBye:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);

        /* 通知TSU停止接收码流*/
        i = notify_tsu_delete_record_task(pCrData->tsu_ip, pCrData->task_id);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_WARN, "StopDeviceRecord() notify_tsu_delete_record_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "StopDeviceRecord() notify_tsu_delete_record_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }

        /* 发送Bye 给其他正在使用录像业务的主叫侧用户*/
        i = send_bye_to_all_other_caller_by_callee_id_and_streamtype(pCrData->callee_id, pCrData->callee_stream_type, record_cr_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "StopDeviceRecord() send_bye_to_all_other_caller_by_callee_id_and_streamtype:callee_id=%s, callee_stream_type=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, i);
        printf("StopDeviceRecord() send_bye_to_all_other_caller_by_callee_id_and_streamtype:callee_id=%s, callee_stream_type=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, i);

        i = RemoveDeviceRecordInfo(pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "StopDeviceRecord() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);
        printf("StopDeviceRecord() :callee_id=%s, callee_stream_type=%d, tsu_resource_index=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, pCrData->tsu_resource_index, i);

        /* 设置TSU 状态 */
        i = SetTSUStatus(pCrData->tsu_resource_index, 1);
        DEBUG_TRACE(MODULE_RECORD, LOG_TRACE, "StopDeviceRecord() SetTSUStatus:tsu_resource_index=%d, i=%d \r\n", pCrData->tsu_resource_index, i);

        i = call_record_set_call_status(record_cr_index, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(record_cr_index);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_ERROR, "StopDeviceRecord() call_record_remove Error:cr_pos=%d, i=%d \r\n", record_cr_index, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_RECORD, LOG_INFO, "StopDeviceRecord() call_record_remove OK:cr_pos=%d, i=%d \r\n", record_cr_index, i);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllRecordTask
 功能描述  : 所有录像任务
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
int StopAllRecordTask(int sock)
{
    int i = 0;
    record_info_t* pRecordInfo = NULL;
    needtoproc_recordinfo_queue needtoproc;
    char rbuf[128] = {0};

    if ((NULL == g_RecordInfoList) || (NULL == g_RecordInfoList->pRecordInfoList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "scan_record_info_list() exit---: Param Error \r\n");
        return -1;
    }

    needtoproc.clear();

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        return 0;
    }

    for (i = 0; i < osip_list_size(g_RecordInfoList->pRecordInfoList); i++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, i);

        if (NULL == pRecordInfo)
        {
            continue;
        }

        if (pRecordInfo->record_cr_index >= 0) /* 已经录像的 */
        {
            needtoproc.push_back(pRecordInfo);
        }
        else if (pRecordInfo->record_status != RECORD_STATUS_INIT)
        {
            needtoproc.push_back(pRecordInfo);
        }
    }

    RECORD_INFO_SMUTEX_UNLOCK();

    /* 处理需要停止录像的 */
    while (!needtoproc.empty())
    {
        pRecordInfo = (record_info_t*) needtoproc.front();
        needtoproc.pop_front();

        if (NULL != pRecordInfo)
        {
            i = StopDeviceRecord(pRecordInfo->record_cr_index);

            if (sock > 0)
            {
                memset(rbuf, 0, 128);

                if (i == 0)
                {
                    snprintf(rbuf, 128, "\r释放录像任务成功:录像任务索引=%d\r\n", pRecordInfo->record_cr_index);
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
                else
                {
                    snprintf(rbuf, 128, "\r释放录像任务失败:录像任务索引=%d\r\n", pRecordInfo->record_cr_index);
                    send(sock, rbuf, sizeof(rbuf) - 1, 0);
                }
            }

            pRecordInfo->tsu_index = -1;
            pRecordInfo->record_cr_index = -1;
            pRecordInfo->record_status = RECORD_STATUS_INIT;
            pRecordInfo->record_start_time = 0;
            pRecordInfo->record_try_count = 0;
            pRecordInfo->record_retry_interval = 5;
            pRecordInfo->iTSUPauseStatus = 0;
            pRecordInfo->iTSUResumeStatus = 0;
            pRecordInfo->iTSUAlarmRecordStatus = 0;
        }
    }

    needtoproc.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopAllProcRecordTask
 功能描述  : 释放掉没有录像成功的录像任务
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年8月25日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int StopAllProcRecordTask()
{
    int i = 0;
    record_info_t* pRecordInfo = NULL;
    needtoproc_recordinfo_queue needtoproc;
    time_t time_now;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if ((NULL == g_RecordInfoList) || (NULL == g_RecordInfoList->pRecordInfoList))
    {
        //DEBUG_TRACE(MODULE_RECORD, LOG_DEBUG, "scan_record_info_list() exit---: Param Error \r\n");
        return -1;
    }

    needtoproc.clear();

    RECORD_INFO_SMUTEX_LOCK();

    if (osip_list_size(g_RecordInfoList->pRecordInfoList) <= 0)
    {
        RECORD_INFO_SMUTEX_UNLOCK();
        return 0;
    }

    for (i = 0; i < osip_list_size(g_RecordInfoList->pRecordInfoList); i++)
    {
        pRecordInfo = (record_info_t*)osip_list_get(g_RecordInfoList->pRecordInfoList, i);

        if (NULL == pRecordInfo)
        {
            continue;
        }

        if (pRecordInfo->record_cr_index >= 0 && pRecordInfo->record_status == RECORD_STATUS_PROC)
        {
            /* 看看启动时间 */
            time_now = time(NULL);

            if (pRecordInfo->record_start_time > 0 && time_now > pRecordInfo->record_start_time && time_now - pRecordInfo->record_start_time >= 600)
            {
                needtoproc.push_back(pRecordInfo);
                printf("StopAllProcRecordTask() needtoproc:device_index=%u \r\n", pRecordInfo->device_index);
            }
        }
    }

    RECORD_INFO_SMUTEX_UNLOCK();

    /* 处理需要停止录像的 */
    while (!needtoproc.empty())
    {
        pRecordInfo = (record_info_t*) needtoproc.front();
        needtoproc.pop_front();

        if (NULL != pRecordInfo)
        {
            i = StopDeviceRecord(pRecordInfo->record_cr_index);
            printf("StopAllProcRecordTask() StopDeviceRecord:record_cr_index=%d \r\n", pRecordInfo->record_cr_index);

            pRecordInfo->tsu_index = -1;
            pRecordInfo->record_cr_index = -1;
            pRecordInfo->record_status = RECORD_STATUS_INIT;
            pRecordInfo->record_start_time = 0;
            pRecordInfo->record_try_count = 0;
            pRecordInfo->record_retry_interval = 5;
            pRecordInfo->iTSUPauseStatus = 0;
            pRecordInfo->iTSUResumeStatus = 0;
            pRecordInfo->iTSUAlarmRecordStatus = 0;

            /* 查找逻辑设备信息 */
            pGBLogicDeviceInfo = GBLogicDevice_info_find_by_device_index(pRecordInfo->device_index);

            if (NULL != pGBLogicDeviceInfo)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "释放掉没有录像成功的录像任务, 录像点位ID=%s, 点位名称=%s, 录像类型=%d, 码流类型=%d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Video, video released no video success point ID = %s, point name = % s, video type = % d, code flow type = % d", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->device_name, pRecordInfo->record_type, pRecordInfo->stream_type);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "释放掉没有录像成功的录像任务, 录像点位索引=%u, 录像类型=%d, 码流类型=%d", pRecordInfo->device_index, pRecordInfo->record_type, pRecordInfo->stream_type);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Video, video released no video success point index = % u, video type = % d, code flow type = % d ", pRecordInfo->device_index, pRecordInfo->record_type, pRecordInfo->stream_type);
            }
        }
    }

    needtoproc.clear();

    return 0;
}

/*****************************************************************************
 函 数 名  : StopRecordTask
 功能描述  : 释放录像任务
 输入参数  : int sock
             int record_cr_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月31日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int StopRecordTask(int sock, int record_cr_index)
{
    int i = 0;
    int record_info_index = -1;
    char rbuf[128] = {0};
    record_info_t* pRecordInfo = NULL;

    i = StopDeviceRecord(record_cr_index);

    if (sock > 0)
    {
        memset(rbuf, 0, 128);

        if (i == 0)
        {
            snprintf(rbuf, 128, "\r释放录像任务成功:录像任务索引=%d\r\n$", record_cr_index);
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
        else
        {
            snprintf(rbuf, 128, "\r释放录像任务失败:录像任务索引=%d\r\n$", record_cr_index);
            send(sock, rbuf, sizeof(rbuf) - 1, 0);
        }
    }

    /* 防止上面释放失败 */
    record_info_index = record_info_find_by_cr_index(record_cr_index);

    if (record_info_index >= 0)
    {
        pRecordInfo = record_info_get(record_info_index);

        if (NULL != pRecordInfo)
        {
            pRecordInfo->tsu_index = -1;
            pRecordInfo->record_cr_index = -1;
            pRecordInfo->record_status = RECORD_STATUS_INIT;
            pRecordInfo->record_start_time = 0;
            pRecordInfo->record_try_count = 0;
            pRecordInfo->record_retry_interval = 5;
            pRecordInfo->iTSUPauseStatus = 0;
            pRecordInfo->iTSUResumeStatus = 0;
            pRecordInfo->iTSUAlarmRecordStatus = 0;
        }
    }

    return 0;
}
