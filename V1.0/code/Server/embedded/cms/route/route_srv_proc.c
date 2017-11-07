
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
#include <string.h>
#include <errno.h>
#endif

#include "common/gblfunc_proc.inc"
#include "common/gblconfig_proc.inc"
#include "common/log_proc.inc"
#include "platformms/BoardInit.h"

#include "route/route_srv_proc.inc"

#include "user/user_srv_proc.inc"

#include "device/device_info_mgn.inc"
#include "device/device_srv_proc.inc"

#include "service/call_func_proc.inc"
#include "service/preset_proc.inc"
#include "service/compress_task_proc.inc"

#include "resource/resource_info_mgn.inc"

#include "record/record_srv_proc.inc"
#include "jwpt_interface/interface_operate.inc"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern gbl_conf_t* pGblconf;              /* 全局配置信息 */
extern BOARD_NET_ATTR  g_BoardNetConfig;
extern unsigned int g_transfer_xml_sn;    /* 全局的转发XML的SN */
extern int g_LocalMediaTransferFlag;      /* 下级媒体流是否经过本地TSU转发,默认转发 */
extern int g_RouteMediaTransferFlag;      /* 上级第三方平台是否有媒体转发功能,默认有 */

extern int db_GBLogicDeviceInfo_reload_mark; /* 逻辑设备数据库更新标识:0:不需要更新，1:需要更新数据库 */
extern int db_GBDeviceInfo_reload_mark;      /* 标准物理设备数据库更新标识:0:不需要更新，1:需要更新数据库 */
extern int db_GroupInfo_reload_mark;         /* 分组信息数据库更新标识:0:不需要更新，1:需要更新数据库 */
extern int db_GroupMapInfo_reload_mark;      /* 分组关系信息数据库更新标识:0:不需要更新，1:需要更新数据库 */

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
route_srv_msg_queue g_RouteSrvMsgQueue;  /* 互联路由业务消息队列 */
#ifdef MULTI_THR
osip_mutex_t* g_RouteSrvMsgQueueLock = NULL;
#endif

route_srv_msg_queue g_RouteMessageSrvMsgQueue;  /* 互联路由Message业务消息队列 */
#ifdef MULTI_THR
osip_mutex_t* g_RouteMessageSrvMsgQueueLock = NULL;
#endif

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#define MAXSIZE                    (8 * 4096)

#define SWAPL(x)    ((((x) & 0x000000ff) << 24) | \
             (((x) & 0x0000ff00) <<  8) | \
             (((x) & 0x00ff0000) >>  8) | \
             (((x) & 0xff000000) >> 24))

#if DECS("互联路由业务消息队列")
/*****************************************************************************
 函 数 名  : route_srv_msg_init
 功能描述  : 互联路由业务消息结构初始化
 输入参数  : route_srv_msg_t ** route_srv_msg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_srv_msg_init(route_srv_msg_t** route_srv_msg)
{
    *route_srv_msg = (route_srv_msg_t*)osip_malloc(sizeof(route_srv_msg_t));

    if (*route_srv_msg == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_msg_init() exit---: *route_srv_msg Smalloc Error \r\n");
        return -1;
    }

    (*route_srv_msg)->msg_type = MSG_TYPE_NULL;
    (*route_srv_msg)->pRouteInfo = NULL;
    (*route_srv_msg)->caller_id[0] = '\0';
    (*route_srv_msg)->callee_id[0] = '\0';
    (*route_srv_msg)->response_code = 0;
    (*route_srv_msg)->reasonphrase[0] = '\0';
    (*route_srv_msg)->ua_dialog_index = -1;
    (*route_srv_msg)->msg_body[0] = '\0';
    (*route_srv_msg)->msg_body_len = 0;
    (*route_srv_msg)->cr_pos = -1;

    return 0;
}

/*****************************************************************************
 函 数 名  : route_srv_msg_free
 功能描述  : 互联路由业务消息结构释放
 输入参数  : route_srv_msg_t * route_srv_msg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void route_srv_msg_free(route_srv_msg_t* route_srv_msg)
{
    if (route_srv_msg == NULL)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_msg_free() exit---: Param Error \r\n");
        return;
    }

    route_srv_msg->msg_type = MSG_TYPE_NULL;
    route_srv_msg->pRouteInfo = NULL;

    memset(route_srv_msg->caller_id, 0, MAX_ID_LEN + 4);
    memset(route_srv_msg->callee_id, 0, MAX_ID_LEN + 4);

    route_srv_msg->response_code = 0;

    memset(route_srv_msg->reasonphrase, 0, MAX_128CHAR_STRING_LEN + 4);

    route_srv_msg->ua_dialog_index = -1;

    memset(route_srv_msg->msg_body, 0, MAX_MSG_BODY_STRING_LEN + 4);

    route_srv_msg->msg_body_len = 0;
    route_srv_msg->cr_pos = -1;

    osip_free(route_srv_msg);
    route_srv_msg = NULL;

    return;
}

/*****************************************************************************
 函 数 名  : route_srv_msg_list_init
 功能描述  : 互联路由业务消息队列初始化
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
int route_srv_msg_list_init()
{
    g_RouteSrvMsgQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_RouteSrvMsgQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_RouteSrvMsgQueueLock)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_msg_list_init() exit---: Route Service Message List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : route_srv_msg_list_free
 功能描述  : 互联路由业务消息队列释放
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
void route_srv_msg_list_free()
{
    route_srv_msg_t* pRouteSrvMsg = NULL;

    while (!g_RouteSrvMsgQueue.empty())
    {
        pRouteSrvMsg = (route_srv_msg_t*) g_RouteSrvMsgQueue.front();
        g_RouteSrvMsgQueue.pop_front();

        if (NULL != pRouteSrvMsg)
        {
            route_srv_msg_free(pRouteSrvMsg);
            pRouteSrvMsg = NULL;
        }
    }

    g_RouteSrvMsgQueue.clear();

#ifdef MULTI_THR

    if (NULL != g_RouteSrvMsgQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_RouteSrvMsgQueueLock);
        g_RouteSrvMsgQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 函 数 名  : route_srv_msg_list_clean
 功能描述  : 互联路由业务消息队列清除
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
void route_srv_msg_list_clean()
{
    route_srv_msg_t* pRouteSrvMsg = NULL;

    while (!g_RouteSrvMsgQueue.empty())
    {
        pRouteSrvMsg = (route_srv_msg_t*) g_RouteSrvMsgQueue.front();
        g_RouteSrvMsgQueue.pop_front();

        if (NULL != pRouteSrvMsg)
        {
            route_srv_msg_free(pRouteSrvMsg);
            pRouteSrvMsg = NULL;
        }
    }

    g_RouteSrvMsgQueue.clear();

    return;
}

/*****************************************************************************
 函 数 名  : route_srv_msg_add
 功能描述  : 添加互联路由业务消息到队列中
 输入参数  : route_info_t* pRouteInfo
             msg_type_t msg_type
             char* caller_id
             char* callee_id
             int response_code
             char* reasonphrase
             int ua_dialog_index
             char* msg_body
             int msg_body_len
             int cr_pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_srv_msg_add(route_info_t* pRouteInfo, msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, char* reasonphrase, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos)
{
    route_srv_msg_t* pRouteSrvMsg = NULL;
    int iRet = 0;

    if (caller_id == NULL || pRouteInfo == NULL || callee_id == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_srv_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = route_srv_msg_init(&pRouteSrvMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_srv_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    pRouteSrvMsg->msg_type = msg_type;
    pRouteSrvMsg->pRouteInfo = pRouteInfo;

    if (NULL != caller_id)
    {
        osip_strncpy(pRouteSrvMsg->caller_id, caller_id, MAX_ID_LEN);
    }

    if (NULL != callee_id)
    {
        osip_strncpy(pRouteSrvMsg->callee_id, callee_id, MAX_ID_LEN);
    }

    pRouteSrvMsg->response_code = response_code;

    if (NULL != reasonphrase)
    {
        osip_strncpy(pRouteSrvMsg->reasonphrase, reasonphrase, MAX_128CHAR_STRING_LEN);
    }

    pRouteSrvMsg->ua_dialog_index = ua_dialog_index;

    if (NULL != msg_body)
    {
        osip_strncpy(pRouteSrvMsg->msg_body, msg_body, MAX_MSG_BODY_STRING_LEN);
    }

    pRouteSrvMsg->msg_body_len = msg_body_len;
    pRouteSrvMsg->cr_pos = cr_pos;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_RouteSrvMsgQueueLock);
#endif

    g_RouteSrvMsgQueue.push_back(pRouteSrvMsg);

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_RouteSrvMsgQueueLock);
#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : scan_route_srv_msg_list
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
void scan_route_srv_msg_list(DBOper* pRoute_Srv_dboper)
{
    int iRet = 0;
    route_srv_msg_t* pRouteSrvMsg = NULL;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_RouteSrvMsgQueueLock);
#endif

    while (!g_RouteSrvMsgQueue.empty())
    {
        pRouteSrvMsg = (route_srv_msg_t*) g_RouteSrvMsgQueue.front();
        g_RouteSrvMsgQueue.pop_front();

        if (NULL != pRouteSrvMsg)
        {
            break;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_RouteSrvMsgQueueLock);
#endif

    if (NULL != pRouteSrvMsg)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "scan_route_srv_msg_list() \
        \r\n In Param: \
        \r\n msg_type=%d \
        \r\n caller_id=%s \
        \r\n callee_id=%s \
        \r\n response_code=%d \
        \r\n ua_dialog_index=%d \
        \r\n msg_body_len=%d \
        \r\n ", pRouteSrvMsg->msg_type, pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->response_code, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->msg_body_len);

        iRet = route_srv_msg_proc(pRouteSrvMsg, pRoute_Srv_dboper);
        route_srv_msg_free(pRouteSrvMsg);
        pRouteSrvMsg = NULL;
    }

    return;
}

/*****************************************************************************
 函 数 名  : route_message_srv_msg_list_init
 功能描述  : 互联路由Message业务消息队列初始化
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
int route_message_srv_msg_list_init()
{
    g_RouteMessageSrvMsgQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_RouteMessageSrvMsgQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_RouteMessageSrvMsgQueueLock)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_message_srv_msg_list_init() exit---: Route Service Message List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : route_message_srv_msg_list_free
 功能描述  : 互联路由Message业务消息队列释放
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
void route_message_srv_msg_list_free()
{
    route_srv_msg_t* pRouteSrvMsg = NULL;

    while (!g_RouteMessageSrvMsgQueue.empty())
    {
        pRouteSrvMsg = (route_srv_msg_t*) g_RouteMessageSrvMsgQueue.front();
        g_RouteMessageSrvMsgQueue.pop_front();

        if (NULL != pRouteSrvMsg)
        {
            route_srv_msg_free(pRouteSrvMsg);
            pRouteSrvMsg = NULL;
        }
    }

    g_RouteMessageSrvMsgQueue.clear();

#ifdef MULTI_THR

    if (NULL != g_RouteMessageSrvMsgQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_RouteMessageSrvMsgQueueLock);
        g_RouteMessageSrvMsgQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 函 数 名  : route_message_srv_msg_list_clean
 功能描述  : 互联路由Message业务消息队列清除
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
void route_message_srv_msg_list_clean()
{
    route_srv_msg_t* pRouteSrvMsg = NULL;

    while (!g_RouteMessageSrvMsgQueue.empty())
    {
        pRouteSrvMsg = (route_srv_msg_t*) g_RouteMessageSrvMsgQueue.front();
        g_RouteMessageSrvMsgQueue.pop_front();

        if (NULL != pRouteSrvMsg)
        {
            route_srv_msg_free(pRouteSrvMsg);
            pRouteSrvMsg = NULL;
        }
    }

    g_RouteMessageSrvMsgQueue.clear();

    return;
}

/*****************************************************************************
 函 数 名  : route_message_srv_msg_add
 功能描述  : 添加互联路Message由业务消息到队列中
 输入参数  : route_info_t* pRouteInfo
             msg_type_t msg_type
             char* caller_id
             char* callee_id
             int response_code
             int ua_dialog_index
             char* msg_body
             int msg_body_len
             int cr_pos
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_message_srv_msg_add(route_info_t* pRouteInfo, msg_type_t msg_type, char* caller_id, char* callee_id, int response_code, int ua_dialog_index, char* msg_body, int msg_body_len, int cr_pos)
{
    route_srv_msg_t* pRouteSrvMsg = NULL;
    int iRet = 0;

    if (caller_id == NULL || pRouteInfo == NULL || callee_id == NULL)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_message_srv_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = route_srv_msg_init(&pRouteSrvMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_message_srv_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    pRouteSrvMsg->msg_type = msg_type;
    pRouteSrvMsg->pRouteInfo = pRouteInfo;

    if (NULL != caller_id)
    {
        osip_strncpy(pRouteSrvMsg->caller_id, caller_id, MAX_ID_LEN);
    }

    if (NULL != callee_id)
    {
        osip_strncpy(pRouteSrvMsg->callee_id, callee_id, MAX_ID_LEN);
    }

    pRouteSrvMsg->response_code = response_code;
    pRouteSrvMsg->ua_dialog_index = ua_dialog_index;

    if (NULL != msg_body)
    {
        osip_strncpy(pRouteSrvMsg->msg_body, msg_body, MAX_MSG_BODY_STRING_LEN);
    }

    pRouteSrvMsg->msg_body_len = msg_body_len;
    pRouteSrvMsg->cr_pos = cr_pos;

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_RouteMessageSrvMsgQueueLock);
#endif

    g_RouteMessageSrvMsgQueue.push_back(pRouteSrvMsg);

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_RouteMessageSrvMsgQueueLock);
#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : scan_route_message_srv_msg_list
 功能描述  : 扫描多级互联Message消息队列
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
void scan_route_message_srv_msg_list(DBOper* pRoute_Srv_dboper)
{
    int iRet = 0;
    route_srv_msg_t* pRouteSrvMsg = NULL;

#ifdef MULTI_THR

    if (NULL != g_RouteMessageSrvMsgQueueLock)
    {
        CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_RouteMessageSrvMsgQueueLock);
    }

#endif

    while (!g_RouteMessageSrvMsgQueue.empty())
    {
        pRouteSrvMsg = (route_srv_msg_t*) g_RouteMessageSrvMsgQueue.front();
        g_RouteMessageSrvMsgQueue.pop_front();

        if (NULL != pRouteSrvMsg)
        {
            break;
        }
    }

#ifdef MULTI_THR
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_RouteMessageSrvMsgQueueLock);
#endif

    if (NULL != pRouteSrvMsg)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "scan_route_message_srv_msg_list() \
        \r\n In Param: \
        \r\n msg_type=%d \
        \r\n caller_id=%s \
        \r\n callee_id=%s \
        \r\n response_code=%d \
        \r\n ua_dialog_index=%d \
        \r\n msg_body_len=%d \
        \r\n ", pRouteSrvMsg->msg_type, pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->response_code, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->msg_body_len);

        iRet = route_srv_msg_proc(pRouteSrvMsg, pRoute_Srv_dboper);
        route_srv_msg_free(pRouteSrvMsg);
        pRouteSrvMsg = NULL;
    }

    return;
}
#endif

/*****************************************************************************
 函 数 名  : route_srv_msg_proc
 功能描述  : 多级互联业务消息处理
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
int route_srv_msg_proc(route_srv_msg_t* pRouteSrvMsg, DBOper* pRoute_Srv_dboper)
{
    int i = 0;

    if (NULL == pRouteSrvMsg)
    {
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_srv_msg_proc() msg_type=%d \r\n ", pRouteSrvMsg->msg_type);

    switch (pRouteSrvMsg->msg_type)
    {
        case MSG_INVITE:
            i = route_invite_msg_proc(pRouteSrvMsg->pRouteInfo, pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->msg_body, pRouteSrvMsg->msg_body_len);
            break;

        case MSG_INVITE_RESPONSE:
            i = route_invite_response_msg_proc(pRouteSrvMsg->cr_pos, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->response_code, pRouteSrvMsg->reasonphrase, pRouteSrvMsg->msg_body, pRouteSrvMsg->msg_body_len);
            break;

        case MSG_CANCEL:
            i = route_cancel_msg_proc(pRouteSrvMsg->cr_pos, pRouteSrvMsg->ua_dialog_index);
            break;

        case MSG_ACK:
            i = route_ack_msg_proc(pRouteSrvMsg->cr_pos, pRouteSrvMsg->ua_dialog_index);
            break;

        case MSG_BYE:
            i = route_bye_msg_proc(pRouteSrvMsg->cr_pos, pRouteSrvMsg->ua_dialog_index);
            break;

        case MSG_BYE_RESPONSE:
            i = route_bye_response_msg_proc(pRouteSrvMsg->cr_pos, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->response_code);
            break;

        case MSG_INFO:
            i = route_info_msg_proc(pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->msg_body, pRouteSrvMsg->msg_body_len);
            break;

        case MSG_MESSAGE:
            i = route_message_msg_proc(pRouteSrvMsg->pRouteInfo, pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->msg_body, pRouteSrvMsg->msg_body_len, pRoute_Srv_dboper);
            break;

        case MSG_NOTIFY:
            i = route_notify_msg_proc(pRouteSrvMsg->pRouteInfo, pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->msg_body, pRouteSrvMsg->msg_body_len, pRoute_Srv_dboper);
            break;

        case MSG_SUBSCRIBE:
            //i = route_subscribe_msg_proc(pRouteSrvMsg->pRouteInfo, pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->response_code, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->msg_body, pRouteSrvMsg->msg_body_len, pRoute_Srv_dboper);
            i = route_subscribe_within_dialog_msg_proc(pRouteSrvMsg->pRouteInfo, pRouteSrvMsg->caller_id, pRouteSrvMsg->callee_id, pRouteSrvMsg->ua_dialog_index, pRouteSrvMsg->response_code, pRouteSrvMsg->msg_body, pRouteSrvMsg->msg_body_len, pRoute_Srv_dboper);
            break;

        default:
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_srv_msg_proc() exit---: Not Support Message Type:%d \r\n", pRouteSrvMsg->msg_type);
            return -1;
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_invite_msg_proc
 功能描述  : 上级互联CMS发送过来的INVITE消息处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
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
int route_invite_msg_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, int ua_dialog_index, char* msg_body, int msg_body_len)
{
    int i = 0;
    sdp_message_t* pClientSDP = NULL;
    sdp_param_t stClientSDPParam;
    sdp_extend_param_t stClientSDPExParam;
    GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo = NULL;
    char strErrorCode[32] = {0};

    if (NULL == pRouteInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_ROUTE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Route Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == callee_id))
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_PARAM_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Param Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request: superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    if (NULL == msg_body || msg_body_len == 0)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_MSG_BODY_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Message SDP Body Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: Get Message SDP Body Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"获取请求方SDP消息体失败");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Access SDP message body from the requester failed.");
        return -1;
    }

    /* 1、查找逻辑设备信息 */
    pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Callee GBlogicDevice Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: Get Callee GBlogicDevice Info Error:callee_id=%s \r\n", callee_id);
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"获取逻辑设备信息失败");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Access logical device information failed");
        return -1;
    }

    /* 2、获取来源的客户端sdp信息，根据其中的s字段，判断业务类型 */
    i = sdp_message_init(&pClientSDP);

    if (0 != i)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MSG_INIT_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"SDP Message Init Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: SDP Message Init Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"SDP消息初始化失败");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"SDP message initialization failed");
        return -1;
    }

    i = sdp_message_parse(pClientSDP, msg_body);

    if (0 != i)
    {
        sdp_message_free(pClientSDP);
        pClientSDP = NULL;

        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MSG_PARSE_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"SDP Message Parse Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: SDP Message Parse Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"SDP消息解析失败");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"SDP message parsing failed");
        return -1;
    }

    /* 3、解析sdp中的信息 */
    memset(&stClientSDPParam, 0, sizeof(sdp_param_t));
    memset(&stClientSDPExParam, 0, sizeof(sdp_extend_param_t));

    i = SIP_GetSDPInfoEx(pClientSDP, &stClientSDPParam, &stClientSDPExParam);

    if (i != 0)
    {
        sdp_message_free(pClientSDP);
        pClientSDP = NULL;

        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_GET_VIDEO_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Client SDP Video Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: Get Client SDP Video Info Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"获取SDP消息中的信息失败");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Access the information in the SDP message failed");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 请求方携带的视频参数:audio_port=%d, audio_code_type=%d, video_port=%d, video_code_type=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stClientSDPParam.audio_port, stClientSDPParam.audio_code_type, stClientSDPParam.video_port, stClientSDPParam.video_code_type);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request: superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, the requester to carry video parameters:audio_port=%d, audio_code_type=%d, video_port=%d, video_code_type=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stClientSDPParam.audio_port, stClientSDPParam.audio_code_type, stClientSDPParam.video_port, stClientSDPParam.video_code_type);

    /* 判断请求类型，是视频请求还是音频请求 */
    if (stClientSDPParam.audio_port > 0
        && stClientSDPParam.video_port <= 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 用户请求的是音频对讲业务", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request: superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, user requests are audio speaker business", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

        i = route_invite_audio_msg_proc(pRouteInfo, pClientSDP, &stClientSDPParam, pCalleeGBLogicDeviceInfo, caller_id, callee_id, ua_dialog_index);

        if (i != 0)
        {
            sdp_message_free(pClientSDP);
            pClientSDP = NULL;
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: Audio Invite Proc Error \r\n");
            return -1;
        }
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 用户请求的是实时视频业务", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request: superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, user requests are real-time video business", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

        i = route_invite_video_msg_proc(pRouteInfo, pClientSDP, &stClientSDPParam, pCalleeGBLogicDeviceInfo, caller_id, callee_id, ua_dialog_index);

        if (i != 0)
        {
            sdp_message_free(pClientSDP);
            pClientSDP = NULL;
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_msg_proc() exit---: Video Invite Proc Error \r\n");
            return -1;
        }
    }

    sdp_message_free(pClientSDP);
    pClientSDP = NULL;

    return i;
}

/*****************************************************************************
 函 数 名  : route_invite_video_msg_proc
 功能描述  : 上级互联CMS发送过来的INVITE视频消息处理
 输入参数  : route_info_t* pRouteInfo
             sdp_message_t* pClientSDP
             sdp_param_t* pClientSDPParam
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
             char* caller_id
             char* callee_id
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
int route_invite_video_msg_proc(route_info_t* pRouteInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index)
{
    int i = 0;
    char strErrorCode[32] = {0};

    if (NULL == pRouteInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_ROUTE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Route Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDP)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_MSG_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_msg_proc() exit---: Client SDP Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDPParam)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_PARAM_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Param Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_msg_proc() exit---: Client SDP Param Info Error \r\n");
        return -1;
    }

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Callee GBLogic Device Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_msg_proc() exit---: Callee GBLogic Device Info Error \r\n");
        return -1;
    }

    /* 根据逻辑设备所属域进行判断，决定消息走向 */
    if (1 == pCalleeGBLogicDeviceInfo->other_realm)
    {
        i = route_invite_route_video_msg_proc(pRouteInfo, pClientSDP, pClientSDPParam, pCalleeGBLogicDeviceInfo, caller_id, callee_id, ua_dialog_index);
    }
    else
    {
        i = route_invite_sub_video_msg_proc(pRouteInfo, pClientSDP, pClientSDPParam, pCalleeGBLogicDeviceInfo, caller_id, callee_id, ua_dialog_index);
    }

    return i;
}


/*****************************************************************************
 函 数 名  : route_invite_sub_video_msg_proc
 功能描述  : 上级互联CMS发送过来的INVITE视频消息处理
 输入参数  : route_info_t* pRouteInfo
             sdp_message_t* pClientSDP
             sdp_param_t* pClientSDPParam
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
             char* caller_id
             char* callee_id
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
int route_invite_sub_video_msg_proc(route_info_t* pRouteInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index)
{
    int i = 0;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    int record_info_pos = -1;
    record_info_t* pRecordInfo = NULL;

    GBDevice_info_t* pCalleeGBDeviceInfo = NULL;
    GBDevice_info_t* pCalleeCmsGBDeviceInfo = NULL;

    //int iAuth = 0;
    //char* realm = NULL;
    //osip_authorization_t* pAuthorization = NULL;

    char* sdp_url = NULL;
    char* sdp_ssrc = NULL;
    char* o_name = NULL;
    char* o_sess_id = NULL;
    char* o_sess_version = NULL;
    char* time_r_repeat = NULL;
    //char strSDPUrl[128] = {0};
    //int iSessionExpires = 0;
    call_type_t eCallType = CALL_TYPE_NULL;
    int stream_type = 0;
    int record_type = 0;
    transfer_protocol_type_t trans_type = TRANSFER_PROTOCOL_NULL;

    char strStartTime[64] = {0};
    char strEndTime[64] = {0};
    char strPlayBackURL[64] = {0};
    int iPlaybackTimeGap = 0;
    char strErrorCode[32] = {0};
    int record_cr_index = -1;

    if (NULL == pRouteInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_ROUTE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Route Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDP)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_MSG_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Client SDP Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDPParam)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_PARAM_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Param Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Client SDP Param Info Error \r\n");
        return -1;
    }

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Callee GBLogic Device Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Callee GBLogic Device Info Error \r\n");
        return -1;
    }

    /* 1、获取流类型，查找对应的物理设备 */
    if (pClientSDPParam->stream_type <= 0)
    {
        stream_type = EV9000_STREAM_TYPE_MASTER;
    }
    else
    {
        stream_type = pClientSDPParam->stream_type;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() Stream Type=%d \r\n", stream_type);

    if (stream_type == EV9000_STREAM_TYPE_SLAVE && pCalleeGBLogicDeviceInfo->stream_count == 1)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_NOT_SUPPORT_MULTI_STREAM_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Callee GBDevice Not Support Multi stream");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Callee GBDevice Not Support Multi stream \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"逻辑设备信息不支持多码流");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Logic device information does not support multi stream");
        return -1;
    }

    /* 2、获取传输类型 */
    if (pClientSDPParam->trans_type <= 0)
    {
        trans_type = TRANSFER_PROTOCOL_UDP;
    }
    else if (pClientSDPParam->trans_type == 2)
    {
        trans_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        trans_type = TRANSFER_PROTOCOL_UDP;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() Transfer Protocol Type=%d \r\n", trans_type);

    /* 查找目的物理设备信息 */
    pCalleeGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, stream_type);

    if (NULL == pCalleeGBDeviceInfo)
    {
        /* 如果是智能分析流，可能是下级平台的点位，这个时候需要再查找一下主流设备 */
        if (EV9000_STREAM_TYPE_INTELLIGENCE == stream_type)
        {
            pCalleeCmsGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (NULL != pCalleeCmsGBDeviceInfo
                && EV9000_DEVICETYPE_SIPSERVER == pCalleeCmsGBDeviceInfo->device_type)
            {
                pCalleeGBDeviceInfo = pCalleeCmsGBDeviceInfo;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() CalleeCmsGBDevice: id=%s, ip=%s \r\n", pCalleeCmsGBDeviceInfo->device_id, pCalleeCmsGBDeviceInfo->login_ip);
            }
        }
        else if (EV9000_STREAM_TYPE_SLAVE == stream_type) /* 如果是辅流，再找一下主流设备 */
        {
            pCalleeGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);
        }
    }

    if (NULL == pCalleeGBDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_DEVICE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Callee GBDevice Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Get Callee GBDevice Info Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 媒体流类型=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"获取逻辑设备对应的物理设备信息失败", stream_type);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, media stream type=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Physical device obtain the corresponding logical device information failed", stream_type);
        return -1;
    }

    /* 如果是第三方平台，去掉扩展的SDP信息 */
    if (EV9000_DEVICETYPE_SIPSERVER == pCalleeGBDeviceInfo->device_type
        && 1 == pCalleeGBDeviceInfo->three_party_flag)
    {
        DelSDPMediaAttributeByName(pClientSDP, (char*)"recordtype");
        DelSDPMediaAttributeByName(pClientSDP, (char*)"streamtype");

        /* 修改名称 */
        o_name = sdp_message_o_username_get(pClientSDP);

        if (NULL != o_name && 0 != sstrcmp(o_name, local_cms_id_get()))
        {
            osip_free(o_name);
            o_name = NULL;

            o_name = osip_getcopy(local_cms_id_get());
            sdp_message_o_username_set(pClientSDP, o_name);
        }
        else if (NULL == o_name)
        {
            o_name = osip_getcopy(local_cms_id_get());
            sdp_message_o_username_set(pClientSDP, o_name);
        }

        o_sess_id = sdp_message_o_sess_id_get(pClientSDP);

        if (NULL != o_sess_id && 0 != sstrcmp(o_sess_id, (char*)"0"))
        {
            osip_free(o_sess_id);
            o_sess_id = NULL;

            o_sess_id = osip_getcopy((char*)"0");
            sdp_message_o_sess_id_set(pClientSDP, o_sess_id);
        }
        else if (NULL == o_sess_id)
        {
            o_sess_id = osip_getcopy(local_cms_id_get());
            sdp_message_o_sess_id_set(pClientSDP, o_sess_id);
        }

        o_sess_version = sdp_message_o_sess_version_get(pClientSDP);

        if (NULL != o_sess_version && 0 != sstrcmp(o_sess_version, (char*)"0"))
        {
            osip_free(o_sess_version);
            o_sess_version = NULL;

            o_sess_version = osip_getcopy((char*)"0");
            sdp_message_o_sess_version_set(pClientSDP, o_sess_version);
        }
        else if (NULL == o_sess_version)
        {
            o_sess_version = osip_getcopy(local_cms_id_get());
            sdp_message_o_sess_version_set(pClientSDP, o_sess_version);
        }

        time_r_repeat = sdp_message_r_repeat_get(pClientSDP, 0, 0);

        if (NULL != time_r_repeat)
        {
            DelSDPTimeRepeatInfo(pClientSDP);
        }
    }

    if (0 == strncmp(pClientSDPParam->s_name, (char*)"Playback", 8))
    {
        eCallType = CALL_TYPE_RECORD_PLAY;

        sdp_url = sdp_message_u_uri_get(pClientSDP);

        if (NULL == sdp_url)
        {
            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }
        else
        {
            osip_free(sdp_url);
            sdp_url = NULL;

            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }

#if 1
        sdp_ssrc = sdp_message_y_ssrc_get(pClientSDP);

        if (NULL == sdp_ssrc)
        {
            sdp_ssrc = osip_getcopy((char*)"1100000001");
            sdp_message_y_ssrc_set(pClientSDP, sdp_ssrc);
        }

#endif

        /* 确定录像类型 */
        if (pClientSDPParam->record_type <= 0)
        {
            record_type = EV9000_RECORD_TYPE_NORMAL;
        }
        else
        {
            record_type = pClientSDPParam->record_type;
        }

        i = format_time(pClientSDPParam->start_time, strStartTime);
        i = format_time(pClientSDPParam->end_time, strEndTime);

#if 0

        /* 如果是前端的NVR或者DVR录像，则修改播放时间 */
        if (EV9000_DEVICETYPE_DVR == pCalleeGBDeviceInfo->device_type || EV9000_DEVICETYPE_NVR == pCalleeGBDeviceInfo->device_type)
        {
            if (1 == pCalleeGBLogicDeviceInfo->record_type)
            {
                i = ModifySDPRecordPlayTime(pClientSDP);
                iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
            }
        }
        else if (EV9000_DEVICETYPE_SIPSERVER == pCalleeGBDeviceInfo->device_type && 1 == pCalleeGBDeviceInfo->three_party_flag) /* 第三方平台 */
        {
            i = ModifySDPRecordPlayTime(pClientSDP);
            iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
        }

#endif
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的历史视频回放请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 录像类型=%d, 媒体流类型=%d, 传输方式=%d, 回放开始时间=%s, 结束时间=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, record_type, stream_type, trans_type, strStartTime, strEndTime);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Historical video playback request from superior CMS:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, media stream type=%d, transmission method=%d, playback start time=%s, end time=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type, strStartTime, strEndTime);
    }
    else if (0 == strncmp(pClientSDPParam->s_name, (char*)"Play", 4))
    {
        eCallType = CALL_TYPE_REALTIME;

#if 1
        sdp_ssrc = sdp_message_y_ssrc_get(pClientSDP);

        if (NULL == sdp_ssrc)
        {
            sdp_ssrc = osip_getcopy((char*)"0100000001");
            sdp_message_y_ssrc_set(pClientSDP, sdp_ssrc);
        }

#endif

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, 上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 媒体流类型=%d, 传输方式=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Real-time monitoring request from superior CMS, superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, media stream type=%d, transmission method=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type);
    }
    else if (0 == strncmp(pClientSDPParam->s_name, (char*)"Download", 8))
    {
        eCallType = CALL_TYPE_DOWNLOAD;

        sdp_url = sdp_message_u_uri_get(pClientSDP);

        if (NULL == sdp_url)
        {
            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }
        else
        {
            osip_free(sdp_url);
            sdp_url = NULL;

            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }

#if 1
        sdp_ssrc = sdp_message_y_ssrc_get(pClientSDP);

        if (NULL == sdp_ssrc)
        {
            sdp_ssrc = osip_getcopy((char*)"1100000001");
            sdp_message_y_ssrc_set(pClientSDP, sdp_ssrc);
        }

#endif

        /* 确定录像类型 */
        if (pClientSDPParam->record_type <= 0)
        {
            record_type = EV9000_RECORD_TYPE_NORMAL;
        }
        else
        {
            record_type = pClientSDPParam->record_type;
        }

        i = format_time(pClientSDPParam->start_time, strStartTime);
        i = format_time(pClientSDPParam->end_time, strEndTime);

#if 0

        /* 如果是前端的NVR或者DVR录像，则修改播放时间 */
        if (EV9000_DEVICETYPE_DVR == pCalleeGBDeviceInfo->device_type || EV9000_DEVICETYPE_NVR == pCalleeGBDeviceInfo->device_type)
        {
            if (1 == pCalleeGBLogicDeviceInfo->record_type)
            {
                i = ModifySDPRecordPlayTime(pClientSDP);
                iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
            }
        }
        else if (EV9000_DEVICETYPE_SIPSERVER == pCalleeGBDeviceInfo->device_type && 1 == pCalleeGBDeviceInfo->three_party_flag) /* 第三方平台 */
        {
            i = ModifySDPRecordPlayTime(pClientSDP);
            iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
        }

#endif
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的历史视频文件下载请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 录像类型=%d, 媒体流类型=%d, 传输方式=%d, 回放开始时间=%s, 结束时间=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type, strStartTime, strEndTime);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Historical video file download request from superior CMS:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, media stream type=%d, transmission method=%d, playback start time=%s, end time=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type, strStartTime, strEndTime);
    }
    else
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_NOT_SUPPORT_S_TYPE_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 488, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 488, (char*)"SDP S Type Not Support");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: SDP S Type Not Support \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, S名称=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"不支持的S名称类型", pClientSDPParam->s_name);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, S name=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"do not support S name sype", pClientSDPParam->s_name);
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() Call Type=%d \r\n", eCallType);

    /* 实时视频查看需要判断设备在线状态 */
    if (eCallType == CALL_TYPE_REALTIME)
    {
        /* 看物理设备是否在线 */
        if (0 == pCalleeGBDeviceInfo->reg_status)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_OFFLINE_ERROR);
            SIP_AnswerToInvite(ua_dialog_index, 480, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Device Not Online:device_id=%s \r\n", pCalleeGBDeviceInfo->device_id);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 物理设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"前端物理设备不在线", pCalleeGBDeviceInfo->device_id);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, physical deviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end physical device not online", pCalleeGBDeviceInfo->device_id);
            return -1;
        }

        /* 看逻辑设备点位状态, 如果是下级CMS的点，则不需要判断 */
        if (EV9000_DEVICETYPE_SIPSERVER != pCalleeGBDeviceInfo->device_type && 0 == pCalleeGBLogicDeviceInfo->status)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_OFFLINE_ERROR);
            SIP_AnswerToInvite(ua_dialog_index, 480, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: GBLogic Device Not Online \r\n");
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"前端逻辑设备不在线");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end logic device is not online");
            return -1;
        }

#if 0

        if (2 == pCalleeGBLogicDeviceInfo->status)
        {
            SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_invite_msg_proc() exit---: GBLogic Device No Stream \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"前端逻辑设备没有码流");
            return -1;
        }

#endif

        if (3 == pCalleeGBLogicDeviceInfo->status)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_UNREACHED_ERROR);
            SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: GBLogic Device NetWork UnReached \r\n");
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"前端逻辑设备网络不可达");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end logic device network unaccessable");
            return -1;
        }

        /* 第三方平台, 查看原有是否有业务，如果有，则关闭掉 */
        if (pRouteInfo->three_party_flag && 1 == g_RouteMediaTransferFlag)
        {
            i = StopRouteService(pRouteInfo, pCalleeGBLogicDeviceInfo->device_id, stream_type);

            if (i == -2)
            {
                /* 没有任务 */
            }
            else if (i < 0 && i != -2)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_invite_sub_video_msg_proc() StopRouteService Error:DeviceID=%s, stream_type=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, stream_type);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的实时视频请求, 关闭掉点位原有的实时视频业务:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 码流类型=%d, i=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, i);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Real-time monitoring request from superior CMS, close old service:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, stream type=%s, i=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, i);

                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_invite_sub_video_msg_proc() StopRouteService OK:DeviceID=%s, stream_type=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, stream_type);
            }
        }
    }

    /* 3、申请呼叫资源 */
    cr_pos = call_record_add();
    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() call_record_add:cr_pos=%d \r\n", cr_pos);

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_GET_IDLE_CR_DATA_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Call Record Data Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Get Call Record Data Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"申请呼叫资源失败");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"apply to call resource failed");
        return -1;
    }

    pCrData->call_type = eCallType;

    /* 4、添加 主被叫信息到呼叫资源信息 */
    /* 主叫端信息 */
    osip_strncpy(pCrData->caller_id, caller_id, MAX_ID_LEN);
    osip_strncpy(pCrData->caller_ip, pRouteInfo->server_ip, MAX_IP_LEN);
    pCrData->caller_port = pRouteInfo->server_port;
    pCrData->caller_ua_index = ua_dialog_index;
    osip_strncpy(pCrData->caller_server_ip_ethname, pRouteInfo->strRegLocalEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->caller_server_ip, pRouteInfo->strRegLocalIP, MAX_IP_LEN);
    pCrData->caller_server_port = pRouteInfo->iRegLocalPort;
    pCrData->caller_transfer_type = trans_type;
    pCrData->iPlaybackTimeGap = iPlaybackTimeGap;

    /* 主叫端的SDP信息 */
    osip_strncpy(pCrData->caller_sdp_ip, pClientSDPParam->sdp_ip, MAX_IP_LEN);
    pCrData->caller_sdp_port = pClientSDPParam->video_port;

#if 0
    printf("\r\n\r\n ************************************************* \r\n");
    printf(" route_invite_sub_video_msg_proc() pCrData->caller_id=%s \r\n", pCrData->caller_id);
    printf(" route_invite_sub_video_msg_proc() pCrData->caller_ip=%s \r\n", pCrData->caller_ip);
    printf(" route_invite_sub_video_msg_proc() pCrData->caller_port=%d \r\n", pCrData->caller_port);
    printf(" route_invite_sub_video_msg_proc() pCrData->caller_sdp_ip=%s \r\n", pCrData->caller_sdp_ip);
    printf(" route_invite_sub_video_msg_proc() pCrData->caller_sdp_port=%d \r\n", pCrData->caller_sdp_port);
    printf(" route_invite_sub_video_msg_proc() pCrData->caller_ua_index=%d \r\n", pCrData->caller_ua_index);
    printf(" ************************************************* \r\n\r\n ");
#endif

    /* 被叫端信息 */
    osip_strncpy(pCrData->callee_id, callee_id, MAX_ID_LEN);
    osip_strncpy(pCrData->callee_ip, pCalleeGBDeviceInfo->login_ip, MAX_IP_LEN);
    pCrData->callee_port = pCalleeGBDeviceInfo->login_port;
    osip_strncpy(pCrData->callee_server_ip_ethname, pCalleeGBDeviceInfo->strRegServerEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->callee_server_ip, pCalleeGBDeviceInfo->strRegServerIP, MAX_IP_LEN);
    pCrData->callee_server_port = pCalleeGBDeviceInfo->iRegServerPort;
    pCrData->callee_framerate = pCalleeGBLogicDeviceInfo->frame_count;
    pCrData->callee_stream_type = stream_type;
    pCrData->callee_gb_device_type = pCalleeGBDeviceInfo->device_type;

    if (1 == pCalleeGBDeviceInfo->trans_protocol)
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_UDP; /* 默认UDP */
    }

#if 0
    printf("\r\n\r\n ************************************************* \r\n");
    printf(" route_invite_sub_video_msg_proc() pCrData->callee_id=%s \r\n", pCrData->callee_id);
    printf(" route_invite_sub_video_msg_proc() pCrData->callee_ip=%s \r\n", pCrData->callee_ip);
    printf(" route_invite_sub_video_msg_proc() pCrData->callee_port=%d \r\n", pCrData->callee_port);
    printf(" ************************************************* \r\n\r\n ");
#endif

    /* 如果是录像回放或者下载, 判断录像是在前端还是在本地,如果录在本地，则不需要转到前端处理 */
    if (eCallType == CALL_TYPE_RECORD_PLAY || eCallType == CALL_TYPE_DOWNLOAD)
    {
        /* 查看该点位是否配置了录像 */
        if (EV9000_RECORD_TYPE_NORMAL == record_type
            || EV9000_RECORD_TYPE_ALARM == record_type)
        {
            record_info_pos = record_info_find_by_stream_type(pCalleeGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_MASTER);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_MASTER, record_info_pos);
        }
        else if (EV9000_RECORD_TYPE_BACKUP == record_type)
        {
            record_info_pos = record_info_find_by_stream_type(pCalleeGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_SLAVE);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_SLAVE, record_info_pos);
        }
        else if (EV9000_RECORD_TYPE_INTELLIGENCE == record_type)
        {
            record_info_pos = record_info_find_by_stream_type(pCalleeGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_INTELLIGENCE);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_INTELLIGENCE, record_info_pos);
        }
        else
        {
            record_info_pos = record_info_find_by_stream_type(pCalleeGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_MASTER);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_MASTER, record_info_pos);
        }

        if (record_info_pos >= 0)
        {
            pRecordInfo = record_info_get(record_info_pos);

            if (NULL != pRecordInfo)
            {
                if (1 == pRecordInfo->record_enable)
                {
                    i = user_video_sevice_current_cms_proc(pCrData, pClientSDP, eCallType, pClientSDPParam->start_time, pClientSDPParam->end_time, pClientSDPParam->play_time, 0, record_type);

                    if (0 != i)
                    {
                        memset(strErrorCode, 0, 32);
                        snprintf(strErrorCode, 32, "%d", i);
                        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
                        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Current CMS Proc Error");
                        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                        i = call_record_remove(cr_pos);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
                        }
                    }

                    return i;
                }
            }
        }
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() CalleeGBLogicDeviceInfo Record Type=%d, Status=%d \r\n", pCalleeGBLogicDeviceInfo->record_type, pCalleeGBLogicDeviceInfo->status);

        if (0 == pCalleeGBLogicDeviceInfo->record_type && 2 != pCalleeGBLogicDeviceInfo->status)/* 本地录像并且不是前端没有码流的状态情况下检查录像信息，因为前端没有码流情况下不影响实时视频流程 */
        {
            /* 检查该点位是否录像了 */
            record_info_pos = record_info_find_by_stream_type(pCalleeGBLogicDeviceInfo->id, stream_type);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, stream_type, record_info_pos);

            if (record_info_pos >= 0)
            {
                pRecordInfo = record_info_get(record_info_pos);

                if (NULL != pRecordInfo)
                {
                    if (1 == pRecordInfo->record_enable)
                    {
                        if (pRecordInfo->record_cr_index < 0 && pRecordInfo->record_status != RECORD_STATUS_NO_TSU)
                        {
                            record_cr_index = StartDeviceRecord(pRecordInfo);

                            if (record_cr_index >= 0)
                            {
                                pRecordInfo->record_retry_interval = 5;
                                pRecordInfo->record_try_count = 0;
                                pRecordInfo->iTSUPauseStatus = 0;
                                pRecordInfo->iTSUResumeStatus = 0;
                                pRecordInfo->iTSUAlarmRecordStatus = 0;

                                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
                                SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的实时视频请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 等待逻辑设备录像业务流程结束", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
                                return 0;
                            }
                            else
                            {
                                pRecordInfo->tsu_index = -1;
                                pRecordInfo->iTSUPauseStatus = 0;
                                pRecordInfo->iTSUResumeStatus = 0;
                                pRecordInfo->iTSUAlarmRecordStatus = 0;

                                if (-2 == record_cr_index)
                                {
                                    pRecordInfo->record_status = RECORD_STATUS_OFFLINE;
                                }
                                else if (-3 == record_cr_index)
                                {
                                    pRecordInfo->record_status = RECORD_STATUS_NOSTREAM;
                                }
                                else if (-5 == record_cr_index)
                                {
                                    pRecordInfo->record_status = RECORD_STATUS_NETWORK_ERROR;
                                }
                                else if (-4 == record_cr_index)
                                {
                                    pRecordInfo->record_status = RECORD_STATUS_NO_TSU;
                                }
                                else if (-6 == record_cr_index)
                                {
                                    pRecordInfo->record_status = RECORD_STATUS_NOT_SUPPORT_MULTI_STREAM;
                                }
                                else
                                {
                                    pRecordInfo->record_status = RECORD_STATUS_INIT;
                                }

                                pRecordInfo->record_start_time = 0;

                                pRecordInfo->record_try_count++;

                                if (pRecordInfo->record_try_count >= 3)
                                {
                                    pRecordInfo->record_try_count = 0;
                                    pRecordInfo->record_retry_interval = 5;
                                }

                                memset(strErrorCode, 0, 32);
                                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_RECORD_NOT_START_ERROR);
                                SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);

                                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                                i = call_record_remove(cr_pos);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
                                }

                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() exit---: Callee GBDevice Not Start Record \r\n");
                                SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"逻辑设备配置了录像，但是启动录像失败了");
                                EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"video is configured for the logic device, but not started");
                                return -1;
                            }
                        }
                        else if (pRecordInfo->record_cr_index >= 0 && pRecordInfo->record_status != RECORD_STATUS_COMPLETE)
                        {
                            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
                            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的实时视频请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 等待逻辑设备录像业务流程结束", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
                            return 0;
                        }
                    }
                }
            }
        }
    }

    /* 5、根据设备类型作不同的处理 */
    if (pCalleeGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER)
    {
        i = user_video_sevice_sub_cms_proc(pCrData, pClientSDP, eCallType);

        if (EV9000_CMS_ERR_INVITE_CALLEE_RECORD_NOT_COMPLETE_ERROR == i)
        {
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的实时视频请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 等待逻辑设备录像业务流程结束", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
            return 0;
        }
        else if (EV9000_CMS_ERR_INVITE_CALLEE_VIDEO_NOT_COMPLETE_ERROR == i)
        {
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的实时视频请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 等待逻辑设备视频业务流程结束", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
            return 0;
        }
        else if (0 != i)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", i);
            SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Sub CMS Proc Error");
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
        }
    }
    else
    {
        i = user_video_sevice_current_cms_proc(pCrData, pClientSDP, eCallType, pClientSDPParam->start_time, pClientSDPParam->end_time, pClientSDPParam->play_time, pCalleeGBLogicDeviceInfo->record_type, record_type);

        if (EV9000_CMS_ERR_INVITE_CALLEE_RECORD_NOT_COMPLETE_ERROR == i)
        {
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的实时视频请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 等待逻辑设备录像业务流程结束", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
            return 0;
        }
        else if (EV9000_CMS_ERR_INVITE_CALLEE_VIDEO_NOT_COMPLETE_ERROR == i)
        {
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的实时视频请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 等待逻辑设备视频业务流程结束", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
            return 0;
        }
        else if (0 != i)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", i);
            SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Current CMS Proc Error");
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_sub_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_sub_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_invite_route_video_msg_proc
 功能描述  : 上级互联CMS发送过来的INVITE视频消息处理
 输入参数  : route_info_t* pRouteInfo
             sdp_message_t* pClientSDP
             sdp_param_t* pClientSDPParam
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
             char* caller_id
             char* callee_id
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
int route_invite_route_video_msg_proc(route_info_t* pRouteInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index)
{
    int i = 0;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    int iCalleeRoutePos = 0;
    route_info_t* pCalleeRouteInfo = NULL;

    char* sdp_url = NULL;
    char* sdp_ssrc = NULL;
    char* o_name = NULL;
    char* o_sess_id = NULL;
    char* o_sess_version = NULL;
    char* time_r_repeat = NULL;
    call_type_t eCallType = CALL_TYPE_NULL;
    int stream_type = 0;
    int record_type = 0;
    transfer_protocol_type_t trans_type = TRANSFER_PROTOCOL_NULL;

    char strStartTime[64] = {0};
    char strEndTime[64] = {0};
    char strPlayBackURL[64] = {0};
    int iPlaybackTimeGap = 0;
    char strErrorCode[32] = {0};

    if (NULL == pRouteInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_ROUTE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Route Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDP)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_MSG_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Client SDP Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDPParam)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_PARAM_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Param Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Client SDP Param Info Error \r\n");
        return -1;
    }

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Callee GBLogic Device Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Callee GBLogic Device Info Error \r\n");
        return -1;
    }

    /* 1、获取流类型，查找对应的物理设备 */
    if (pClientSDPParam->stream_type <= 0)
    {
        stream_type = EV9000_STREAM_TYPE_MASTER;
    }
    else
    {
        stream_type = pClientSDPParam->stream_type;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() Stream Type=%d \r\n", stream_type);

    if (stream_type == EV9000_STREAM_TYPE_SLAVE && pCalleeGBLogicDeviceInfo->stream_count == 1)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_NOT_SUPPORT_MULTI_STREAM_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Callee GBDevice Not Support Multi stream");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Callee GBDevice Not Support Multi stream \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"逻辑设备信息不支持多码流");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Logic device information does not support multi stream");
        return -1;
    }

    /* 2、获取传输类型 */
    if (pClientSDPParam->trans_type <= 0)
    {
        trans_type = TRANSFER_PROTOCOL_UDP;
    }
    else if (pClientSDPParam->trans_type == 2)
    {
        trans_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        trans_type = TRANSFER_PROTOCOL_UDP;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() Transfer Protocol Type=%d \r\n", trans_type);

    /* 查找目的上级路由信息 */
    iCalleeRoutePos = route_info_find(pCalleeGBLogicDeviceInfo->cms_id);

    if (iCalleeRoutePos < 0)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_ROUTE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Find Callee Route Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Find Callee Route Info Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 媒体流类型=%d", caller_id,  pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"查找逻辑设备对应的上级路由信息失败", stream_type);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, reason=%s, Media stream type=%d", caller_id,  pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Find the route information which corresponding to logical device failed.", stream_type);
        return -1;
    }

    pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

    if (NULL == pCalleeRouteInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_ROUTE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Callee Route Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Get Callee Route Info Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 媒体流类型=%d", caller_id,  pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"获取逻辑设备对应的上级路由信息失败", stream_type);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, reason=%s, Media stream type=%d", caller_id,  pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Get the route information which corresponding to logical device failed.", stream_type);
        return -1;
    }

    /* 如果是第三方平台，去掉扩展的SDP信息 */
    if (1 == pCalleeRouteInfo->three_party_flag)
    {
        DelSDPMediaAttributeByName(pClientSDP, (char*)"recordtype");
        DelSDPMediaAttributeByName(pClientSDP, (char*)"streamtype");

        /* 修改名称 */
        o_name = sdp_message_o_username_get(pClientSDP);

        if (NULL != o_name && 0 != sstrcmp(o_name, local_cms_id_get()))
        {
            osip_free(o_name);
            o_name = NULL;

            o_name = osip_getcopy(local_cms_id_get());
            sdp_message_o_username_set(pClientSDP, o_name);
        }
        else if (NULL == o_name)
        {
            o_name = osip_getcopy(local_cms_id_get());
            sdp_message_o_username_set(pClientSDP, o_name);
        }

        o_sess_id = sdp_message_o_sess_id_get(pClientSDP);

        if (NULL != o_sess_id && 0 != sstrcmp(o_sess_id, (char*)"0"))
        {
            osip_free(o_sess_id);
            o_sess_id = NULL;

            o_sess_id = osip_getcopy((char*)"0");
            sdp_message_o_sess_id_set(pClientSDP, o_sess_id);
        }
        else if (NULL == o_sess_id)
        {
            o_sess_id = osip_getcopy(local_cms_id_get());
            sdp_message_o_sess_id_set(pClientSDP, o_sess_id);
        }

        o_sess_version = sdp_message_o_sess_version_get(pClientSDP);

        if (NULL != o_sess_version && 0 != sstrcmp(o_sess_version, (char*)"0"))
        {
            osip_free(o_sess_version);
            o_sess_version = NULL;

            o_sess_version = osip_getcopy((char*)"0");
            sdp_message_o_sess_version_set(pClientSDP, o_sess_version);
        }
        else if (NULL == o_sess_version)
        {
            o_sess_version = osip_getcopy(local_cms_id_get());
            sdp_message_o_sess_version_set(pClientSDP, o_sess_version);
        }

        time_r_repeat = sdp_message_r_repeat_get(pClientSDP, 0, 0);

        if (NULL != time_r_repeat)
        {
            DelSDPTimeRepeatInfo(pClientSDP);
        }
    }

    if (0 == strncmp(pClientSDPParam->s_name, (char*)"Playback", 8))
    {
        eCallType = CALL_TYPE_RECORD_PLAY;

        sdp_url = sdp_message_u_uri_get(pClientSDP);

        if (NULL == sdp_url)
        {
            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }
        else
        {
            osip_free(sdp_url);
            sdp_url = NULL;

            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }

#if 1
        sdp_ssrc = sdp_message_y_ssrc_get(pClientSDP);

        if (NULL == sdp_ssrc)
        {
            sdp_ssrc = osip_getcopy((char*)"1100000001");
            sdp_message_y_ssrc_set(pClientSDP, sdp_ssrc);
        }

#endif

        /* 确定录像类型 */
        if (pClientSDPParam->record_type <= 0)
        {
            record_type = EV9000_RECORD_TYPE_NORMAL;
        }
        else
        {
            record_type = pClientSDPParam->record_type;
        }

        i = format_time(pClientSDPParam->start_time, strStartTime);
        i = format_time(pClientSDPParam->end_time, strEndTime);

#if 0

        /* 如果是前端的NVR或者DVR录像，则修改播放时间 */
        if (EV9000_DEVICETYPE_DVR == pCalleeGBDeviceInfo->device_type || EV9000_DEVICETYPE_NVR == pCalleeGBDeviceInfo->device_type)
        {
            if (1 == pCalleeGBLogicDeviceInfo->record_type)
            {
                i = ModifySDPRecordPlayTime(pClientSDP);
                iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
            }
        }
        else if (EV9000_DEVICETYPE_SIPSERVER == pCalleeGBDeviceInfo->device_type && 1 == pCalleeGBDeviceInfo->three_party_flag) /* 第三方平台 */
        {
            i = ModifySDPRecordPlayTime(pClientSDP);
            iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
        }

#endif
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的历史视频回放请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 录像类型=%d, 媒体流类型=%d, 传输方式=%d, 回放开始时间=%s, 结束时间=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, record_type, stream_type, trans_type, strStartTime, strEndTime);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Historical video playback request from superior CMS:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, media stream type=%d, transmission method=%d, playback start time=%s, end time=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type, strStartTime, strEndTime);
    }
    else if (0 == strncmp(pClientSDPParam->s_name, (char*)"Play", 4))
    {
        eCallType = CALL_TYPE_REALTIME;

#if 1
        sdp_ssrc = sdp_message_y_ssrc_get(pClientSDP);

        if (NULL == sdp_ssrc)
        {
            sdp_ssrc = osip_getcopy((char*)"0100000001");
            sdp_message_y_ssrc_set(pClientSDP, sdp_ssrc);
        }

#endif

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, 上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 媒体流类型=%d, 传输方式=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Real-time monitoring request from superior CMS, superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, media stream type=%d, transmission method=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type);
    }
    else if (0 == strncmp(pClientSDPParam->s_name, (char*)"Download", 8))
    {
        eCallType = CALL_TYPE_DOWNLOAD;

        sdp_url = sdp_message_u_uri_get(pClientSDP);

        if (NULL == sdp_url)
        {
            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }
        else
        {
            osip_free(sdp_url);
            sdp_url = NULL;

            snprintf(strPlayBackURL, 64, "%s:%s", callee_id, (char*)"application");
            sdp_url = osip_getcopy(strPlayBackURL);
            sdp_message_u_uri_set(pClientSDP, sdp_url);
        }

#if 1
        sdp_ssrc = sdp_message_y_ssrc_get(pClientSDP);

        if (NULL == sdp_ssrc)
        {
            sdp_ssrc = osip_getcopy((char*)"1100000001");
            sdp_message_y_ssrc_set(pClientSDP, sdp_ssrc);
        }

#endif

        /* 确定录像类型 */
        if (pClientSDPParam->record_type <= 0)
        {
            record_type = EV9000_RECORD_TYPE_NORMAL;
        }
        else
        {
            record_type = pClientSDPParam->record_type;
        }

        i = format_time(pClientSDPParam->start_time, strStartTime);
        i = format_time(pClientSDPParam->end_time, strEndTime);

#if 0

        /* 如果是前端的NVR或者DVR录像，则修改播放时间 */
        if (EV9000_DEVICETYPE_DVR == pCalleeGBDeviceInfo->device_type || EV9000_DEVICETYPE_NVR == pCalleeGBDeviceInfo->device_type)
        {
            if (1 == pCalleeGBLogicDeviceInfo->record_type)
            {
                i = ModifySDPRecordPlayTime(pClientSDP);
                iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
            }
        }
        else if (EV9000_DEVICETYPE_SIPSERVER == pCalleeGBDeviceInfo->device_type && 1 == pCalleeGBDeviceInfo->three_party_flag) /* 第三方平台 */
        {
            i = ModifySDPRecordPlayTime(pClientSDP);
            iPlaybackTimeGap = pClientSDPParam->play_time - pClientSDPParam->start_time;
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() PlaybackTimeGap=%d \r\n", iPlaybackTimeGap);
        }

#endif
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的历史视频文件下载请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 录像类型=%d, 媒体流类型=%d, 传输方式=%d, 回放开始时间=%s, 结束时间=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, record_type, stream_type, trans_type, strStartTime, strEndTime);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Historical video file download request from superior CMS:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, media stream type=%d, transmission method=%d, playback start time=%s, end time=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, trans_type, strStartTime, strEndTime);
    }
    else
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_NOT_SUPPORT_S_TYPE_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 488, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 488, (char*)"SDP S Type Not Support");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: SDP S Type Not Support \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, S名称=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"不支持的S名称类型", pClientSDPParam->s_name);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, S name=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"do not support S name sype", pClientSDPParam->s_name);
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() Call Type=%d \r\n", eCallType);

    /* 实时视频查看需要判断设备在线状态 */
    if (eCallType == CALL_TYPE_REALTIME)
    {
        /* 看物理设备是否在线 */
        if (0 == pCalleeRouteInfo->reg_status)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_OFFLINE_ERROR);
            SIP_AnswerToInvite(ua_dialog_index, 480, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Device Not Online:device_id=%s \r\n", pCalleeRouteInfo->server_id);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 物理设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"前端物理设备不在线", pCalleeRouteInfo->server_id);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, physical deviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end physical device not online", pCalleeRouteInfo->server_id);
            return -1;
        }

#if 0

        /* 看逻辑设备点位状态 */
        if (0 == pCalleeGBLogicDeviceInfo->status)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_OFFLINE_ERROR);
            SIP_AnswerToInvite(ua_dialog_index, 480, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: GBLogic Device Not Online \r\n");
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"前端逻辑设备不在线");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end logic device is not online");
            return -1;
        }

        if (2 == pCalleeGBLogicDeviceInfo->status)
        {
            SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_invite_msg_proc() exit---: GBLogic Device No Stream \r\n");
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"前端逻辑设备没有码流");
            return -1;
        }

#endif

        if (3 == pCalleeGBLogicDeviceInfo->status)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_UNREACHED_ERROR);
            SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
            //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: GBLogic Device NetWork UnReached \r\n");
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"前端逻辑设备网络不可达");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end logic device network unaccessable");
            return -1;
        }

        /* 第三方平台, 查看原有是否有业务，如果有，则关闭掉 */
        if (pRouteInfo->three_party_flag && 1 == g_RouteMediaTransferFlag)
        {
            i = StopRouteService(pRouteInfo, pCalleeGBLogicDeviceInfo->device_id, stream_type);

            if (i == -2)
            {
                /* 没有任务 */
            }
            else if (i < 0 && i != -2)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_invite_route_video_msg_proc() StopRouteService Error:DeviceID=%s, stream_type=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, stream_type);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的实时视频请求, 关闭掉点位原有的实时视频业务:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 码流类型=%d, i=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, i);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Real-time monitoring request from superior CMS, close old service:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, stream type=%s, i=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, stream_type, i);

                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_invite_route_video_msg_proc() StopRouteService OK:DeviceID=%s, stream_type=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, stream_type);
            }
        }
    }

    /* 3、申请呼叫资源 */
    cr_pos = call_record_add();
    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() call_record_add:cr_pos=%d \r\n", cr_pos);

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_GET_IDLE_CR_DATA_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Call Record Data Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() exit---: Get Call Record Data Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"申请呼叫资源失败");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"apply to call resource failed");
        return -1;
    }

    pCrData->call_type = eCallType;

    /* 4、添加 主被叫信息到呼叫资源信息 */
    /* 主叫端信息 */
    osip_strncpy(pCrData->caller_id, caller_id, MAX_ID_LEN);
    osip_strncpy(pCrData->caller_ip, pRouteInfo->server_ip, MAX_IP_LEN);
    pCrData->caller_port = pRouteInfo->server_port;
    pCrData->caller_ua_index = ua_dialog_index;
    osip_strncpy(pCrData->caller_server_ip_ethname, pRouteInfo->strRegLocalEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->caller_server_ip, pRouteInfo->strRegLocalIP, MAX_IP_LEN);
    pCrData->caller_server_port = pRouteInfo->iRegLocalPort;
    pCrData->caller_transfer_type = trans_type;
    pCrData->iPlaybackTimeGap = iPlaybackTimeGap;

    /* 主叫端的SDP信息 */
    osip_strncpy(pCrData->caller_sdp_ip, pClientSDPParam->sdp_ip, MAX_IP_LEN);
    pCrData->caller_sdp_port = pClientSDPParam->video_port;

#if 0
    printf("\r\n\r\n ************************************************* \r\n");
    printf(" route_invite_route_video_msg_proc() pCrData->caller_id=%s \r\n", pCrData->caller_id);
    printf(" route_invite_route_video_msg_proc() pCrData->caller_ip=%s \r\n", pCrData->caller_ip);
    printf(" route_invite_route_video_msg_proc() pCrData->caller_port=%d \r\n", pCrData->caller_port);
    printf(" route_invite_route_video_msg_proc() pCrData->caller_sdp_ip=%s \r\n", pCrData->caller_sdp_ip);
    printf(" route_invite_route_video_msg_proc() pCrData->caller_sdp_port=%d \r\n", pCrData->caller_sdp_port);
    printf(" route_invite_route_video_msg_proc() pCrData->caller_ua_index=%d \r\n", pCrData->caller_ua_index);
    printf(" ************************************************* \r\n\r\n ");
#endif

    /* 被叫端信息 */
    osip_strncpy(pCrData->callee_id, callee_id, MAX_ID_LEN);
    osip_strncpy(pCrData->callee_ip, pCalleeRouteInfo->server_ip, MAX_IP_LEN);
    pCrData->callee_port = pCalleeRouteInfo->server_port;
    osip_strncpy(pCrData->callee_server_ip_ethname, pCalleeRouteInfo->strRegLocalEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->callee_server_ip, pCalleeRouteInfo->strRegLocalIP, MAX_IP_LEN);
    pCrData->callee_server_port = pCalleeRouteInfo->iRegLocalPort;
    pCrData->callee_framerate = pCalleeGBLogicDeviceInfo->frame_count;
    pCrData->callee_stream_type = stream_type;
    pCrData->callee_gb_device_type = EV9000_DEVICETYPE_SIPSERVER;

    if (1 == pCalleeRouteInfo->trans_protocol)
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_TCP;
    }
    else
    {
        pCrData->callee_transfer_type = TRANSFER_PROTOCOL_UDP; /* 默认UDP */
    }

#if 0
    printf("\r\n\r\n ************************************************* \r\n");
    printf(" route_invite_route_video_msg_proc() pCrData->callee_id=%s \r\n", pCrData->callee_id);
    printf(" route_invite_route_video_msg_proc() pCrData->callee_ip=%s \r\n", pCrData->callee_ip);
    printf(" route_invite_route_video_msg_proc() pCrData->callee_port=%d \r\n", pCrData->callee_port);
    printf(" ************************************************* \r\n\r\n ");
#endif

    i = user_video_sevice_sub_cms_proc(pCrData, pClientSDP, eCallType);

    if (EV9000_CMS_ERR_INVITE_CALLEE_RECORD_NOT_COMPLETE_ERROR == i)
    {
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的实时视频请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 等待逻辑设备录像业务流程结束", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        return 0;
    }
    else if (EV9000_CMS_ERR_INVITE_CALLEE_VIDEO_NOT_COMPLETE_ERROR == i)
    {
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_ANSWER);
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的实时视频请求:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 等待逻辑设备视频业务流程结束", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        return 0;
    }
    else if (0 != i)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", i);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Route CMS Proc Error");
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_route_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_route_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_invite_audio_msg_proc
 功能描述  : 上级互联CMS发送过来的INVITE音频对讲消息处理
 输入参数  : route_info_t* pRouteInfo
             sdp_message_t* pClientSDP
             sdp_param_t* pClientSDPParam
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
             char* caller_id
             char* callee_id
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
int route_invite_audio_msg_proc(route_info_t* pRouteInfo, sdp_message_t* pClientSDP, sdp_param_t* pClientSDPParam, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo, char* caller_id, char* callee_id, int ua_dialog_index)
{
    int i = 0;
    int cr_pos = -1;
    cr_t* pCrData = NULL;

    GBDevice_info_t* pCalleeGBDeviceInfo = NULL;

    //int iAuth = 0;
    //char* realm = NULL;
    //osip_authorization_t* pAuthorization = NULL;
    //char* sdp_ssrc = NULL;
    //char strSDPUrl[128] = {0};
    //int iSessionExpires = 0;
    call_type_t eCallType = CALL_TYPE_NULL;
    char strErrorCode[32] = {0};

    if (NULL == pRouteInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_ROUTE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Route Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDP)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_MSG_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Client SDP Info Error \r\n");
        return -1;
    }

    if (NULL == pClientSDPParam)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_SDP_PARAM_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Client SDP Param Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Client SDP Param Info Error \r\n");
        return -1;
    }

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Callee GBLogic Device Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Callee GBLogic Device Info Error \r\n");
        return -1;
    }

    /* 1、查找对应的物理设备 */
    pCalleeGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

    if (NULL == pCalleeGBDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_DEVICE_INFO_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Callee GBDevice Info Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Get Callee GBDevice Info Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时音频对讲请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"获取逻辑设备对应的物理设备信息失败");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Access logic device corresponded physical device info failed");
        return -1;
    }

    /* 目前语音对讲只支持实时对讲 */
    if (0 == strncmp(pClientSDPParam->s_name, "Play", 4))
    {
        eCallType = CALL_TYPE_AUDIO;
    }
    else
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_NOT_SUPPORT_S_TYPE_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 488, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 488, (char*)"SDP S Type Not Support");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: SDP S Type Not Support \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时音频对讲请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, S名称=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"不支持的S名称类型", pClientSDPParam->s_name);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, S name=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Do not support S name type", pClientSDPParam->s_name);
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_audio_msg_proc() Call Type=%d \r\n", eCallType);

    /* 2.看物理设备是否在线 */
    if (0 == pCalleeGBDeviceInfo->reg_status)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_OFFLINE_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 480, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Device Not Online:device_id=%s \r\n", pCalleeGBDeviceInfo->device_id);
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时音频对讲请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 物理设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"前端物理设备不在线", pCalleeGBDeviceInfo->device_id);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, physical deviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end physical device not online", pCalleeGBDeviceInfo->device_id);
        return -1;
    }

    /* 看逻辑设备点位状态, 如果是下级CMS的点，则不需要判断 */
    if (EV9000_DEVICETYPE_SIPSERVER != pCalleeGBDeviceInfo->device_type && 0 == pCalleeGBLogicDeviceInfo->status)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_OFFLINE_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 480, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: GBLogic Device Not Online \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时音频对讲请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"前端逻辑设备不在线");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end logic device is not online");
        return -1;
    }

    if (3 == pCalleeGBLogicDeviceInfo->status)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_LOGIC_DEVICE_UNREACHED_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 480, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: GBLogic Device NetWork UnReached \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时音频对讲请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"前端逻辑设备网络不可达");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Front-end logic device network unaccessable");
        return -1;
    }

    /* 3、申请呼叫资源 */
    cr_pos = call_record_add();
    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_audio_msg_proc() call_record_add:cr_pos=%d \r\n", cr_pos);

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_GET_IDLE_CR_DATA_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Get Call Record Data Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Get Call Record Data Error \r\n");
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时音频对讲请求失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"申请呼叫资源失败");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"Apply to call resource failed");
        return -1;
    }

    pCrData->call_type = eCallType;

    /* 4、添加 主被叫信息到呼叫资源信息 */
    /* 主叫端信息 */
    osip_strncpy(pCrData->caller_id, caller_id, MAX_ID_LEN);
    osip_strncpy(pCrData->caller_ip, pRouteInfo->server_ip, MAX_IP_LEN);
    pCrData->caller_port = pRouteInfo->server_port;
    pCrData->caller_ua_index = ua_dialog_index;
    osip_strncpy(pCrData->caller_server_ip_ethname, pRouteInfo->strRegLocalEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->caller_server_ip, pRouteInfo->strRegLocalIP, MAX_IP_LEN);
    pCrData->caller_server_port = pRouteInfo->iRegLocalPort;
    pCrData->caller_transfer_type = TRANSFER_PROTOCOL_UDP; /* 赋默认值 */

    /* 主叫端的SDP信息 */
    osip_strncpy(pCrData->caller_sdp_ip, pClientSDPParam->sdp_ip, MAX_IP_LEN);
    pCrData->caller_sdp_port = pClientSDPParam->audio_port;

#if 0
    printf("\r\n\r\n ************************************************* \r\n");
    printf(" route_invite_audio_msg_proc() pCrData->caller_id=%s \r\n", pCrData->caller_id);
    printf(" route_invite_audio_msg_proc() pCrData->caller_ip=%s \r\n", pCrData->caller_ip);
    printf(" route_invite_audio_msg_proc() pCrData->caller_port=%d \r\n", pCrData->caller_port);
    printf(" route_invite_audio_msg_proc() pCrData->caller_sdp_ip=%s \r\n", pCrData->caller_sdp_ip);
    printf(" route_invite_audio_msg_proc() pCrData->caller_sdp_port=%d \r\n", pCrData->caller_sdp_port);
    printf(" route_invite_audio_msg_proc() pCrData->caller_ua_index=%d \r\n", pCrData->caller_ua_index);
    printf(" ************************************************* \r\n\r\n ");
#endif

    /* 被叫端信息 */
    osip_strncpy(pCrData->callee_id, callee_id, MAX_ID_LEN);
    osip_strncpy(pCrData->callee_ip, pCalleeGBDeviceInfo->login_ip, MAX_IP_LEN);
    pCrData->callee_port = pCalleeGBDeviceInfo->login_port;
    osip_strncpy(pCrData->callee_server_ip_ethname, pCalleeGBDeviceInfo->strRegServerEthName, MAX_IP_LEN);
    osip_strncpy(pCrData->callee_server_ip, pCalleeGBDeviceInfo->strRegServerIP, MAX_IP_LEN);
    pCrData->callee_server_port = pCalleeGBDeviceInfo->iRegServerPort;
    pCrData->callee_framerate = pCalleeGBLogicDeviceInfo->frame_count;
    pCrData->callee_stream_type = EV9000_STREAM_TYPE_MASTER;
    pCrData->callee_transfer_type = TRANSFER_PROTOCOL_UDP; /* 赋默认值 */
    pCrData->callee_gb_device_type = pCalleeGBDeviceInfo->device_type;

#if 0
    printf("\r\n\r\n ************************************************* \r\n");
    printf(" route_invite_audio_msg_proc() pCrData->callee_id=%s \r\n", pCrData->callee_id);
    printf(" route_invite_audio_msg_proc() pCrData->callee_ip=%s \r\n", pCrData->callee_ip);
    printf(" route_invite_audio_msg_proc() pCrData->callee_port=%d \r\n", pCrData->callee_port);
    printf(" ************************************************* \r\n\r\n ");
#endif

    /* 5、转发消息到前端 */
    i = user_transfer_invite_to_dest_for_audio_proc(pCrData, pClientSDP);

    if (0 != i)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_TRANSFER_MSG_TO_DEST_ERROR);
        SIP_AnswerToInvite(ua_dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(ua_dialog_index, 503, (char*)"Transfer Invite Message Proc Error");
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() exit---: Transfer Invite Message Proc Error \r\n");
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_audio_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_invite_response_msg_proc
 功能描述  : 上级互联CMS发送过来的INVITE 回应消息处理
 输入参数  : int cr_pos
             int ua_dialog_index
             int response_code
             char* reasonphrase
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
int route_invite_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code, char* reasonphrase, char* msg_body, int msg_body_len)
{
    int i = 0;
    cr_t* pCrData = NULL;
    sdp_message_t* pRemoteSDP = NULL;
    sdp_param_t stRemoteSDPParam;
    sdp_extend_param_t stRemoteSDPExParam;
    GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo = NULL;
    char strErrorCode[32] = {0};

    if (cr_pos < 0)
    {
        if (200 == response_code)
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* invite的响应一般由被叫发送过来*/
    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        if (200 == response_code)
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Get Call Record Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc():caller_id=%s, caller_ip=%s, callee_id=%s, callee_ip=%s \r\n", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, INVITE回应消息处理:上级CMS ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Real-time monitoring request from superior CMS, INVITE response message process:superior CMS ID=%s, IPaddress=%s, logic device ID=%s, IP address=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

    /* 查找逻辑设备信息 */
    pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(pCrData->callee_id);

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"获取前端逻辑设备信息失败");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access front-end logical device information failed");

        if (200 == response_code)
        {
            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get Callee GBlogicDevice Info Error");
        }

        i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_INVITE_CALLEE_LOGIC_DEVICE_INFO_ERROR);

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Get Callee GBlogicDevice Info Error \r\n");
        return -1;
    }

    /* 根据响应码作不同的处理 */
    if (200 == response_code)
    {
        if (NULL == msg_body || msg_body_len == 0)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"获对端的SDP信息失败");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access opposite end SDP info failed");

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLEE_MSG_BODY_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_INVITE_CALLEE_MSG_BODY_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Get Remote Message SDP Body Error \r\n");
            return -1;
        }

        /* 获取200消息中的被叫的sdp信息 */
        i = sdp_message_init(&pRemoteSDP);

        if (0 != i)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"SDP初始化失败");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"SDP Initialization failed ");

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MSG_INIT_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_SDP_MSG_INIT_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Remote SDP Message Init Error \r\n");
            return -1;
        }

        /* 解析SDP信息 */
        i = sdp_message_parse(pRemoteSDP, msg_body);

        if (0 != i)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"SDP信息解析失败");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"SDP info analysis failed");

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MSG_PARSE_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_SDP_MSG_PARSE_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            sdp_message_free(pRemoteSDP);
            pRemoteSDP = NULL;

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Remote SDP Message Parse Error \r\n");
            return -1;
        }

        /* 获取协商的SDP信息 */
        memset(&stRemoteSDPParam, 0, sizeof(sdp_param_t));
        memset(&stRemoteSDPExParam, 0, sizeof(sdp_extend_param_t));

        i = SIP_GetSDPInfoEx(pRemoteSDP, &stRemoteSDPParam, &stRemoteSDPExParam);

        if (0 != i)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"获取SDP中的信息失败");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access info in SDP failed");

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_GET_VIDEO_INFO_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);

            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_SDP_GET_VIDEO_INFO_ERROR);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            sdp_message_free(pRemoteSDP);
            pRemoteSDP = NULL;

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Get Remote SDP Video Info Error \r\n");
            return -1;
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() addr=%s,port=%d,code=%d,flag=%d \r\n", stRemoteSDPParam.sdp_ip, stRemoteSDPParam.video_port, stRemoteSDPParam.video_code_type, stRemoteSDPParam.media_direction);

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

            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_response_msg_proc() callee_onvif_url=%s \r\n", pCrData->callee_onvif_url);

            /* 被叫侧的协议类型改为RTSP */
            //pCrData->callee_transfer_type = TRANSFER_PROTOCOL_RTSP;
        }

        /* 添加被叫的SDP 信息 */
        osip_strncpy(pCrData->callee_sdp_ip, stRemoteSDPParam.sdp_ip, MAX_IP_LEN);

#if 0
        printf("\r\n\r\n ************************************************* \r\n");
        printf(" route_invite_response_msg_proc() pCrData->callee_sdp_ip=%s \r\n", pCrData->callee_sdp_ip);
        printf(" route_invite_response_msg_proc() pCrData->callee_sdp_port=%d \r\n", pCrData->callee_sdp_port);
        printf(" route_invite_response_msg_proc() pCrData->tsu_code=%d \r\n", pCrData->tsu_code);
        printf(" ************************************************* \r\n\r\n ");
#endif

        if (CALL_TYPE_AUDIO == pCrData->call_type)
        {
            pCrData->callee_sdp_port = stRemoteSDPParam.audio_port;
            pCrData->tsu_code = stRemoteSDPParam.audio_code_type;

            i = route_invite_audio_response_msg_proc(cr_pos, ua_dialog_index, pCrData, pRemoteSDP, pCalleeGBLogicDeviceInfo);
        }
        else
        {
            pCrData->callee_sdp_port = stRemoteSDPParam.video_port;
            pCrData->tsu_code = stRemoteSDPParam.video_code_type;

            i = route_invite_video_response_msg_proc(cr_pos, ua_dialog_index, pCrData, pRemoteSDP, pCalleeGBLogicDeviceInfo);
        }

        if (i != 0)
        {
            i = return_error_for_wait_answer_call_record(pCrData, i);

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
            }

            sdp_message_free(pRemoteSDP);
            pRemoteSDP = NULL;

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() exit---: Accept Invite Error \r\n");
            return -1;
        }

        /* 通知等待的呼叫任务，接受请求，通知TSU转发码流 */
        i = resumed_wait_answer_call_record1(pCrData);

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, INVITE回应消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, respond the INVITE message processing success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);
        sdp_message_free(pRemoteSDP);
        pRemoteSDP = NULL;
    }
    else
    {
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 错误码=%d, reasonphrase=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"非200的错误响应消息", response_code, reasonphrase);
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, error code=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Non 200 fault response message", response_code);

        if (EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type)/* 可能是下级CMS返回的有具体错误原因 */
        {
            if (NULL != reasonphrase && reasonphrase[0] != '\0')
            {
                SIP_AnswerToInvite(pCrData->caller_ua_index, response_code, reasonphrase);
                i = return_error_for_wait_answer_call_record(pCrData, response_code);
            }
            else
            {
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_FRONT_RETURN_ERROR);
                SIP_AnswerToInvite(pCrData->caller_ua_index, response_code, strErrorCode);
                i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_INVITE_FRONT_RETURN_ERROR);
            }
        }
        else
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_FRONT_RETURN_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, response_code, strErrorCode);
            i = return_error_for_wait_answer_call_record(pCrData, EV9000_CMS_ERR_INVITE_FRONT_RETURN_ERROR);
        }
    }

    /* 4、如果是错误回应，移除呼叫信息 */
    if (response_code >= 400)
    {
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : route_invite_video_response_msg_proc
 功能描述  : 上级互联CMS发送过来的INVITE 音频回应消息处理
 输入参数  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             sdp_message_t* pRemoteSDP
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_invite_video_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;

    /* 根据逻辑设备所属域进行判断，决定消息走向 */
    if (1 == pCalleeGBLogicDeviceInfo->other_realm)
    {
        i = route_invite_video_route_response_msg_proc(cr_pos, ua_dialog_index, pCrData, pRemoteSDP, pCalleeGBLogicDeviceInfo);
    }
    else
    {
        i = route_invite_video_sub_response_msg_proc(cr_pos, ua_dialog_index, pCrData, pRemoteSDP, pCalleeGBLogicDeviceInfo);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_invite_video_sub_response_msg_proc
 功能描述  : 上级互联CMS发送过来的INVITE 音频回应消息处理
 输入参数  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             sdp_message_t* pRemoteSDP
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_invite_video_sub_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;
    int send_port = 0;
    char* sdp_tsu_ip = NULL;
    char strErrorCode[32] = {0};

    if (!g_LocalMediaTransferFlag
        && EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type)
    {
        /* 如果是跨级CMS的点位并且不需要本级TSU转发码流，直接转发消息 */
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, INVITE回应消息处理:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 媒体流不经过本级转发, 直接转发消息到媒体流请求方", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, respond the INVITE message processing: the higher the CMS, ID = % s = % s IP address and port number = % d, logical device ID = % s, stream without forward at the corresponding level, the forward message directly to the requesting party media flow", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);
    }
    else
    {
        if (EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type
            && (pCrData->call_type == CALL_TYPE_RECORD_PLAY || pCrData->call_type == CALL_TYPE_DOWNLOAD)) /* 下级CMS 过来的 */
        {
            //发送端口号从新获取
            /* 获取TSU 发送端口号 */
            send_port = get_send_port_by_tsu_resource(pCrData->tsu_ip);

            if (send_port <= 0)
            {
                SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"获取TSU的发送端口号失败", pCrData->tsu_ip);
                EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access TSU send port number failed", pCrData->tsu_ip);

                /* 回应消息给被叫 */
                i = SIP_SendAck(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                i = SIP_SendBye(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                /* 回应消息给主叫 */
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR);
                SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
                //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get TSU Send Port rror");

                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Get TSU Send Port Error:tsu_ip=%s \r\n", pCrData->tsu_ip);
                return EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR;
            }

            pCrData->tsu_send_port = send_port;

            /* 通知TSU开始转发码流 */
            i = notify_tsu_add_transfer_for_replay_task(pCrData, 0, pCrData->callee_stream_type);
        }
        else if (pCalleeGBLogicDeviceInfo->record_type == 1 && (pCrData->call_type == CALL_TYPE_RECORD_PLAY || pCrData->call_type == CALL_TYPE_DOWNLOAD)) /* 调看前端录像 */
        {
            //发送端口号从新获取
            /* 获取TSU 发送端口号 */
            send_port = get_send_port_by_tsu_resource(pCrData->tsu_ip);

            if (send_port <= 0)
            {
                SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"获取TSU的发送端口号失败", pCrData->tsu_ip);
                EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access TSU send port number failed", pCrData->tsu_ip);

                /* 回应消息给被叫 */
                i = SIP_SendAck(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                i = SIP_SendBye(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                /* 回应消息给主叫 */
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR);
                SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
                //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get TSU Send Port rror");

                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Get TSU Send Port Error:tsu_ip=%s \r\n", pCrData->tsu_ip);
                return EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR;
            }

            pCrData->tsu_send_port = send_port;

            /* 通知TSU开始转发码流 */
            i = notify_tsu_add_transfer_for_replay_task(pCrData, 0, pCrData->callee_stream_type);
        }
        else
        {
            if (TRANSFER_PROTOCOL_TCP == pCrData->caller_transfer_type) /* TCP的情况下，直接调用TSU转发接口，返回值是TSU的发送端口号 */
            {
                /* 通知TSU开始转发码流 */
                i = notify_tsu_add_transfer_task(pCrData, pCrData->callee_service_type, pCrData->callee_stream_type);

                if (i > 0)
                {
                    pCrData->tsu_send_port = i;
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_video_sub_response_msg_proc() tsu_send_port=%d \r\n", pCrData->tsu_send_port);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() tsu_send_port=%d \r\n", pCrData->tsu_send_port);
                }
            }
            else
            {
                //发送端口号从新获取
                /* 获取TSU 发送端口号 */
                send_port = get_send_port_by_tsu_resource(pCrData->tsu_ip);

                if (send_port <= 0)
                {
                    SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"获取TSU的发送端口号失败", pCrData->tsu_ip);
                    EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access TSU send port number failed", pCrData->tsu_ip);

                    /* 回应消息给被叫 */
                    i = SIP_SendAck(ua_dialog_index);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                    i = SIP_SendBye(ua_dialog_index);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                    /* 回应消息给主叫 */
                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR);
                    SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
                    //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get TSU Send Port rror");

                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Get TSU Send Port Error:tsu_ip=%s \r\n", pCrData->tsu_ip);
                    return EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR;
                }

                pCrData->tsu_send_port = send_port;

                /* 通知TSU开始转发码流 */
                i = notify_tsu_add_transfer_task(pCrData, 0, pCrData->callee_stream_type);
            }
        }

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: notify_tsu_add_transfer_task Error: TSU IP=%s, i=%d \r\n", pCrData->tsu_ip, i);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, TSU IP=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"通知TSU添加转发任务失败", pCrData->tsu_ip, i);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, TSU IP=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Notify TSU to add forwarding task failed.", pCrData->tsu_ip, i);

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            i = SIP_SendBye(pCrData->caller_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);

            return EV9000_CMS_ERR_TSU_NOTIFY_ADD_TRANSFER_ERROR;
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, INVITE回应消息处理:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 通知TSU添加转发任务成功, TSU IP=%s, task_id=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id, i);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, respond the INVITE message processing: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, notify the TSU add forward task successfully, TSU IP=%s, task_id=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id, i);
        }

        /* 组建本地SDP信息*/
        sdp_tsu_ip = get_cr_sdp_tsu_ip(pCrData, pCrData->caller_server_ip_ethname);

        if (NULL == sdp_tsu_ip)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Get Caller TSU SDP IP Error:callee_server_ip_ethname=%s\r\n", pCrData->caller_server_ip_ethname);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, callee_server_ip_ethname=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"获取主叫侧的SDP消息中的TSU的IP地址失败", pCrData->caller_server_ip_ethname, i);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed: superior CMS ID=%s, IPaddress=%s, port number=%d, logic deviceID=%s, cause=%s, callee_server_ip_ethname=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access TSU IP address from caller side SDP.", pCrData->caller_server_ip_ethname, i);

            /* 通知TSU停止转发码流 */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_CALLER_TSU_IP_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP S Name Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Get sdp_tsu_ip Error \r\n");

            return EV9000_CMS_ERR_TSU_GET_CALLER_TSU_IP_ERROR;
        }

        /* 修改SDP中的S Name */
        i = ModifySDPSName(pRemoteSDP, pCrData->call_type);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"修改SDP中S Name失败");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Edit S Name in SDP failed");

            /* 通知TSU停止转发码流 */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MODIFY_S_NAME_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP S Name Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Modify SDP S Name Error \r\n");
            return EV9000_CMS_ERR_SDP_MODIFY_S_NAME_ERROR;
        }

        /* 修改SDP中的ip地址和端口号*/
        i = ModifySDPIPAndPort(pRemoteSDP, sdp_tsu_ip, pCrData->tsu_send_port);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"修改SDP中的IP和端口号失败");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Edit IP and port number in SDP failed");

            /* 通知TSU停止转发码流 */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MODIFY_IP_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP IP And Addr Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Modify SDP IP And Addr Error \r\n");
            return EV9000_CMS_ERR_SDP_MODIFY_IP_ERROR;
        }

        /* 修改SDP中的协议传输类型 */
        if (pCrData->caller_transfer_type != pCrData->callee_transfer_type)
        {
            if (TRANSFER_PROTOCOL_TCP == pCrData->caller_transfer_type)
            {
                i = ModifySDPTransProtocol(pRemoteSDP, 1);
            }
            else
            {
                i = ModifySDPTransProtocol(pRemoteSDP, 0);
            }

            if (i != 0)
            {
                SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"修改SDP中的协议传输类型失败");
                EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Edit protocal transimission type in SDP failed");

                /* 通知TSU停止转发码流 */
                i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

                /* 回应消息给被叫 */
                i = SIP_SendAck(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                i = SIP_SendBye(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                /* 回应消息给主叫 */
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MODIFY_PROTOCOL_ERROR);
                SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
                //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP Protocol Error");

                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Modify SDP Protocol Error \r\n");
                return EV9000_CMS_ERR_SDP_MODIFY_PROTOCOL_ERROR;
            }
        }

        /* 去除SDP中的ONVIF URL，级联之间不能用RTSP */
        if (NULL != pRemoteSDP->u_uri)
        {
            osip_free(pRemoteSDP->u_uri);
            pRemoteSDP->u_uri = NULL;
        }
    }

    /* 接收主叫方的呼叫*/
    i = SIP_AcceptInvite(pCrData->caller_ua_index, pRemoteSDP);

    if (i != 0)
    {
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"接收客户端的INVITE消息失败");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Receive INVITE message from client failed");

        if (!g_LocalMediaTransferFlag
            && EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type)
        {
            /* 如果是跨级CMS的点位并且不需要本级TSU转发码流，直接转发消息 */
        }
        else
        {
            /* 通知TSU停止转发码流 */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);
        }

        /* 回应消息给被叫 */
        i = SIP_SendAck(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

        i = SIP_SendBye(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

        /* 回应消息给主叫 */
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_ACCEPT_ERROR);
        SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
        //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Accept Invite Error");

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_sub_response_msg_proc() exit---: Accept Invite Error \r\n");
        return EV9000_CMS_ERR_INVITE_ACCEPT_ERROR;
    }

    if (!g_LocalMediaTransferFlag
        && EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type)
    {
        /* 如果是跨级CMS的点位并且不需要本级TSU转发码流，直接转发消息 */
        i = SIP_SendAck(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);
    }
    else
    {
        if (EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type
            && (pCrData->call_type == CALL_TYPE_RECORD_PLAY || pCrData->call_type == CALL_TYPE_DOWNLOAD)) /* 下级CMS 过来的 */
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);
        }
        else if (pCalleeGBLogicDeviceInfo->record_type == 1 && (pCrData->call_type == CALL_TYPE_RECORD_PLAY || pCrData->call_type == CALL_TYPE_DOWNLOAD)) /* 调看前端录像 */
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);
        }
        else
        {
            /* 加入待发送ACK 消息队列 */
            if (ua_dialog_index == pCrData->caller_ua_index)
            {
                i = ack_send_use(cr_pos, ua_dialog_index, -1);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() ack_send_use:cr_pos=%d, caller_ua_index=%d, i=%d \r\n", cr_pos, ua_dialog_index, i);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, INVITE回应消息处理, 添加等待TSU通知任务创建结果消息处理结果事件, 等待发送ACK消息:cr_pos=%d, caller_ua_index=%d, iRet=%d", cr_pos, ua_dialog_index, i);
            }
            else if (ua_dialog_index == pCrData->callee_ua_index)
            {
                i = ack_send_use(cr_pos, -1, ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_sub_response_msg_proc() ack_send_use:cr_pos=%d, callee_ua_index=%d, i=%d \r\n", cr_pos, ua_dialog_index, i);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, INVITE回应消息处理, 添加等待TSU通知任务创建结果消息处理结果事件, 等待发送ACK消息:cr_pos=%d, callee_ua_index=%d, iRet=%d", cr_pos, ua_dialog_index, i);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : route_invite_video_route_response_msg_proc
 功能描述  : 上级互联CMS发送过来的INVITE 音频回应消息处理
 输入参数  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             sdp_message_t* pRemoteSDP
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_invite_video_route_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;
    int send_port = 0;
    char* sdp_tsu_ip = NULL;
    char strErrorCode[32] = {0};

    if (!g_LocalMediaTransferFlag)
    {
        /* 如果是跨级CMS的点位并且不需要本级TSU转发码流，直接转发消息 */
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, INVITE回应消息处理:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 媒体流不经过本级转发, 直接转发消息到媒体流请求方", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, respond the INVITE message processing: the higher the CMS, ID = % s = % s IP address and port number = % d, logical device ID = % s, stream without forward at the corresponding level, the forward message directly to the requesting party media flow", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);
    }
    else
    {
        if (pCrData->call_type == CALL_TYPE_RECORD_PLAY || pCrData->call_type == CALL_TYPE_DOWNLOAD) /* 调看前端录像 */
        {
            //发送端口号从新获取
            /* 获取TSU 发送端口号 */
            send_port = get_send_port_by_tsu_resource(pCrData->tsu_ip);

            if (send_port <= 0)
            {
                SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"获取TSU的发送端口号失败", pCrData->tsu_ip);
                EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access TSU send port number failed", pCrData->tsu_ip);

                /* 回应消息给被叫 */
                i = SIP_SendAck(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                i = SIP_SendBye(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                /* 回应消息给主叫 */
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR);
                SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
                //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get TSU Send Port rror");

                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Get TSU Send Port Error:tsu_ip=%s \r\n", pCrData->tsu_ip);
                return EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR;
            }

            pCrData->tsu_send_port = send_port;

            /* 通知TSU开始转发码流 */
            i = notify_tsu_add_transfer_for_replay_task(pCrData, 0, pCrData->callee_stream_type);
        }
        else
        {
            if (TRANSFER_PROTOCOL_TCP == pCrData->caller_transfer_type) /* TCP的情况下，直接调用TSU转发接口，返回值是TSU的发送端口号 */
            {
                /* 通知TSU开始转发码流 */
                i = notify_tsu_add_transfer_task(pCrData, pCrData->callee_service_type, pCrData->callee_stream_type);

                if (i > 0)
                {
                    pCrData->tsu_send_port = i;
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_invite_video_route_response_msg_proc() tsu_send_port=%d \r\n", pCrData->tsu_send_port);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() tsu_send_port=%d \r\n", pCrData->tsu_send_port);
                }
            }
            else
            {
                //发送端口号从新获取
                /* 获取TSU 发送端口号 */
                send_port = get_send_port_by_tsu_resource(pCrData->tsu_ip);

                if (send_port <= 0)
                {
                    SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"获取TSU的发送端口号失败", pCrData->tsu_ip);
                    EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access TSU send port number failed", pCrData->tsu_ip);

                    /* 回应消息给被叫 */
                    i = SIP_SendAck(ua_dialog_index);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                    i = SIP_SendBye(ua_dialog_index);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                    /* 回应消息给主叫 */
                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR);
                    SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
                    //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get TSU Send Port rror");

                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Get TSU Send Port Error:tsu_ip=%s \r\n", pCrData->tsu_ip);
                    return EV9000_CMS_ERR_TSU_GET_SEND_PORT_ERROR;
                }

                pCrData->tsu_send_port = send_port;

                /* 通知TSU开始转发码流 */
                i = notify_tsu_add_transfer_task(pCrData, 0, pCrData->callee_stream_type);
            }
        }

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: notify_tsu_add_transfer_task Error: TSU IP=%s, i=%d \r\n", pCrData->tsu_ip, i);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, TSU IP=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"通知TSU添加转发任务失败", pCrData->tsu_ip, i);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, TSU IP=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Notify TSU to add forwarding task failed.", pCrData->tsu_ip, i);

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            i = SIP_SendBye(pCrData->caller_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);

            return EV9000_CMS_ERR_TSU_NOTIFY_ADD_TRANSFER_ERROR;
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, INVITE回应消息处理:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 通知TSU添加转发任务成功, TSU IP=%s, task_id=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id, i);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, respond the INVITE message processing: the higher the CMS, ID = % s = % s IP address and port number = % d, logical device ID = % s, notify the TSU add forward mission success, TSU IP = % s, task_id = % s, iRet = %d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, pCrData->tsu_ip, pCrData->task_id, i);
        }

        /* 组建本地SDP信息*/
        sdp_tsu_ip = get_cr_sdp_tsu_ip(pCrData, pCrData->caller_server_ip_ethname);

        if (NULL == sdp_tsu_ip)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Get Caller TSU SDP IP Error:callee_server_ip_ethname=%s\r\n", pCrData->caller_server_ip_ethname);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, callee_server_ip_ethname=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"获取主叫侧的SDP消息中的TSU的IP地址失败", pCrData->caller_server_ip_ethname, i);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed: superior CMS ID=%s, IPaddress=%s, port number=%d, logic deviceID=%s, cause=%s, callee_server_ip_ethname=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access TSU IP address from caller side SDP.", pCrData->caller_server_ip_ethname, i);

            /* 通知TSU停止转发码流 */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_CALLER_TSU_IP_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP S Name Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Get sdp_tsu_ip Error \r\n");
            return EV9000_CMS_ERR_TSU_GET_CALLER_TSU_IP_ERROR;
        }

        /* 修改SDP中的S Name */
        i = ModifySDPSName(pRemoteSDP, pCrData->call_type);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"修改SDP中S Name失败");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Edit S Name in SDP failed");

            /* 通知TSU停止转发码流 */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MODIFY_S_NAME_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP S Name Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Modify SDP S Name Error \r\n");
            return EV9000_CMS_ERR_SDP_MODIFY_S_NAME_ERROR;
        }

        /* 修改SDP中的ip地址和端口号*/
        i = ModifySDPIPAndPort(pRemoteSDP, sdp_tsu_ip, pCrData->tsu_send_port);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"修改SDP中的IP和端口号失败");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Edit IP and port number in SDP failed");

            /* 通知TSU停止转发码流 */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MODIFY_IP_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP IP And Addr Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Modify SDP IP And Addr Error \r\n");
            return EV9000_CMS_ERR_SDP_MODIFY_IP_ERROR;
        }

        /* 修改SDP中的协议传输类型 */
        if (pCrData->caller_transfer_type != pCrData->callee_transfer_type)
        {
            if (TRANSFER_PROTOCOL_TCP == pCrData->caller_transfer_type)
            {
                i = ModifySDPTransProtocol(pRemoteSDP, 1);
            }
            else
            {
                i = ModifySDPTransProtocol(pRemoteSDP, 0);
            }

            if (i != 0)
            {
                SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"修改SDP中的协议传输类型失败");
                EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Edit protocal transimission type in SDP failed");

                /* 通知TSU停止转发码流 */
                i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

                /* 回应消息给被叫 */
                i = SIP_SendAck(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                i = SIP_SendBye(ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

                /* 回应消息给主叫 */
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MODIFY_PROTOCOL_ERROR);
                SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
                //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP Protocol Error");

                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Modify SDP Protocol Error \r\n");
                return EV9000_CMS_ERR_SDP_MODIFY_PROTOCOL_ERROR;
            }
        }

        /* 去除SDP中的ONVIF URL，级联之间不能用RTSP */
        if (NULL != pRemoteSDP->u_uri)
        {
            osip_free(pRemoteSDP->u_uri);
            pRemoteSDP->u_uri = NULL;
        }
    }

    /* 接收主叫方的呼叫*/
    i = SIP_AcceptInvite(pCrData->caller_ua_index, pRemoteSDP);

    if (i != 0)
    {
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时视频请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"接收客户端的INVITE消息失败");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real-time monitoring request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Receive INVITE message from client failed");

        if (!g_LocalMediaTransferFlag)
        {
            /* 如果是跨级CMS的点位并且不需要本级TSU转发码流，直接转发消息 */
        }
        else
        {
            /* 通知TSU停止转发码流 */
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);
        }

        /* 回应消息给被叫 */
        i = SIP_SendAck(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

        i = SIP_SendBye(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

        /* 回应消息给主叫 */
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_ACCEPT_ERROR);
        SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
        //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Accept Invite Error");

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_video_route_response_msg_proc() exit---: Accept Invite Error \r\n");
        return EV9000_CMS_ERR_INVITE_ACCEPT_ERROR;
    }

    if (!g_LocalMediaTransferFlag)
    {
        /* 如果是跨级CMS的点位并且不需要本级TSU转发码流，直接转发消息 */
        i = SIP_SendAck(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);
    }
    else
    {
        if (pCrData->call_type == CALL_TYPE_RECORD_PLAY || pCrData->call_type == CALL_TYPE_DOWNLOAD) /* 下级CMS 过来的 */
        {
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);
        }
        else
        {
            /* 加入待发送ACK 消息队列 */
            if (ua_dialog_index == pCrData->caller_ua_index)
            {
                i = ack_send_use(cr_pos, ua_dialog_index, -1);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() ack_send_use:cr_pos=%d, caller_ua_index=%d, i=%d \r\n", cr_pos, ua_dialog_index, i);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, INVITE回应消息处理, 添加等待TSU通知任务创建结果消息处理结果事件, 等待发送ACK消息:cr_pos=%d, caller_ua_index=%d, iRet=%d", cr_pos, ua_dialog_index, i);
            }
            else if (ua_dialog_index == pCrData->callee_ua_index)
            {
                i = ack_send_use(cr_pos, -1, ua_dialog_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_video_route_response_msg_proc() ack_send_use:cr_pos=%d, callee_ua_index=%d, i=%d \r\n", cr_pos, ua_dialog_index, i);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, INVITE回应消息处理, 添加等待TSU通知任务创建结果消息处理结果事件, 等待发送ACK消息:cr_pos=%d, callee_ua_index=%d, iRet=%d", cr_pos, ua_dialog_index, i);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : route_invite_audio_response_msg_proc
 功能描述  : 上级互联CMS发送过来的INVITE 音频回应消息处理
 输入参数  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             sdp_message_t* pRemoteSDP
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_invite_audio_response_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, sdp_message_t* pRemoteSDP, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;
    int recv_port = 0;
    char* sdp_tsu_ip = NULL;
    char strErrorCode[32] = {0};

    if (!g_LocalMediaTransferFlag
        && EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type)
    {
        /* 如果是跨级CMS的点位并且不需要本级TSU转发码流，直接转发消息 */
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时音频对讲请求, INVITE回应消息处理:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 媒体流不经过本级转发, 直接转发消息到媒体流请求方", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time audio speaker request, respond the INVITE message processing: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, stream without forward at the corresponding level, the forward message directly to the media stream requesting party", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id);

    }
    else
    {
        /* 获取TSU 音频接收端口号 */
        recv_port = get_tsu_audio_recv_port(pCrData->tsu_ip);

        if (recv_port <= 0)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时音频对讲请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"获取TSU的接收端口号失败", pCrData->tsu_ip);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, tsu_ip=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access receive port number of TSU failed.", pCrData->tsu_ip);

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_RECV_PORT_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get TSU Send Port rror");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_response_msg_proc() exit---: Get TSU Send Port Error:tsu_ip=%s \r\n", pCrData->tsu_ip);
            return EV9000_CMS_ERR_TSU_GET_RECV_PORT_ERROR;
        }

        pCrData->tsu_recv_port = recv_port;

        /* 组建本地SDP信息*/
        sdp_tsu_ip = get_cr_sdp_tsu_ip(pCrData, pCrData->caller_server_ip_ethname);

        if (NULL == sdp_tsu_ip)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时音频对讲请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, callee_server_ip_ethname=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"获取主叫侧的SDP消息中的TSU的IP地址失败", pCrData->caller_server_ip_ethname, i);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, callee_server_ip_ethname=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Access IP address from caller side SDP message", pCrData->caller_server_ip_ethname, i);

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_CALLER_TSU_IP_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Get Caller TSU SDP IP Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_response_msg_proc() exit---: Get Caller TSU SDP IP Error \r\n");
            return EV9000_CMS_ERR_TSU_GET_CALLER_TSU_IP_ERROR;
        }

        /* 修改SDP中的ip地址和端口号*/
        i = ModifySDPIPAndPort(pRemoteSDP, sdp_tsu_ip, pCrData->tsu_recv_port);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时音频对讲请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"修改SDP中的IP和端口号失败");
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Edit IP and port number in SDP failed");

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SDP_MODIFY_IP_ERROR);
            SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
            //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Modify SDP Info Error");

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_response_msg_proc() exit---: Modify SDP Info Error \r\n");
            return EV9000_CMS_ERR_SDP_MODIFY_IP_ERROR;
        }
    }

    /* 接收主叫方的呼叫*/
    i = SIP_AcceptInvite(pCrData->caller_ua_index, pRemoteSDP);

    if (i != 0)
    {
        SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时音频对讲请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"接收客户端的INVITE消息失败");
        EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Receive INVITE message from client failed");

        /* 回应消息给被叫 */
        i = SIP_SendAck(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

        i = SIP_SendBye(ua_dialog_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

        /* 回应消息给主叫 */
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_ACCEPT_ERROR);
        SIP_AnswerToInvite(pCrData->caller_ua_index, 503, strErrorCode);
        //SIP_AnswerToInvite(pCrData->caller_ua_index, 503, (char*)"Accept Invite Error");

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_response_msg_proc() exit---: Accept Invite Error \r\n");
        return EV9000_CMS_ERR_INVITE_ACCEPT_ERROR;
    }

    if (!g_LocalMediaTransferFlag
        && EV9000_DEVICETYPE_SIPSERVER == pCrData->callee_gb_device_type)
    {
        /* 如果是跨级CMS的点位并且不需要本级TSU转发码流，直接转发消息 */
    }
    else
    {
        /* 通知TSU开始转发音频流 */
        i = notify_tsu_add_audio_transfer_task(pCrData);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_invite_audio_response_msg_proc() exit---: notify_tsu_add_audio_transfer_task Error: TSU IP=%s, i=%d \r\n", pCrData->tsu_ip, i);
            SystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的实时音频对讲请求, INVITE回应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, TSU IP=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"通知TSU添加转发任务失败", pCrData->tsu_ip, i);
            EnSystemLog(EV9000_CMS_VIDEO_REQUEST_ERROR, EV9000_LOG_LEVEL_ERROR, "Real time audio intercom request from superior CMS, INVITE response message process failed:superior CMS ID=%s, IPaddress=%s, port number=%d, logic device ID=%s, cause=%s, TSU IP=%s, iRet=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->caller_port, pCrData->callee_id, (char*)"Notify TSU to add forwarding task failed", pCrData->tsu_ip, i);

            /* 回应消息给被叫 */
            i = SIP_SendAck(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            i = SIP_SendBye(ua_dialog_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendBye:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

            /* 回应消息给主叫 */
            i = SIP_SendBye(pCrData->caller_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendBye:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);

            return EV9000_CMS_ERR_TSU_NOTIFY_ADD_TRANSFER_ERROR;
        }
    }

    i = SIP_SendAck(ua_dialog_index);
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_invite_audio_response_msg_proc() SIP_SendAck:ua_dialog_index=%d, i=%d \r\n", ua_dialog_index, i);

    return 0;
}

/*****************************************************************************
 函 数 名  : route_cancel_msg_proc
 功能描述  : 上级互联CMS发送过来的Cancel消息处理
 输入参数  : int cr_pos
             int ua_dialog_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月29日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_cancel_msg_proc(int cr_pos, int ua_dialog_index)
{
    int i = 0;
    cr_t* pCrData = NULL;

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_cancel_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* Cancel消息由客户端在发送Invite之后没有收到最终应答前发出 */
    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_cancel_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, 收到CANCEL取消消息处理, 实时视频关闭:上级CMS ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Real-time video from superior CMS, receive the user cancel message processing:User ID=%s, User IP=%s,Logic Device ID=%s, IP Address=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

    /* 2、如果是实时视频业务，服务器需要将Cancel 消息转发给被叫方 */
    if (pCrData->callee_ua_index >= 0)
    {
        i = SIP_SendCancel(pCrData->callee_ua_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_cancel_msg_proc() SIP_SendCancel:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
    }

    /* 3、移除呼叫记录信息 */
    i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
    i = call_record_remove(cr_pos);

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_cancel_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_cancel_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_ack_msg_proc
 功能描述  : 上级互联CMS发送过来的ACK消息处理
 输入参数  : int cr_pos
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
int route_ack_msg_proc(int cr_pos, int ua_dialog_index)
{
    int i = 0;
    cr_t* pCrData = NULL;

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_ack_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* Ack消息由上级CMS 收到200后发出 */
    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_ack_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, 收到ACK消息处理:上级CMS ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, receives an ACK message processing: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

    /* 2、如果是实时视频业务，服务器需要将ack消息转发给被叫方 */
    /*if (pCrData->callee_ua_index >= 0) //收到前端响应的时候直接回复了ACK,所以这个地方收到客户端的ACK不需要再转发给前端
    {
        i = SIP_SendAck(pCrData->callee_ua_index);
    }*/


    return i;
}

/*****************************************************************************
 函 数 名  : route_bye_msg_proc
 功能描述  : 上级互联CMS发送过来的BYE 消息处理
 输入参数  : int cr_pos
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
int route_bye_msg_proc(int cr_pos, int ua_dialog_index)
{
    int i = 0;
    cr_t* pCrData = NULL;
    GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo = NULL;

    if (cr_pos < 0)
    {
        SIP_AnswerToBye(ua_dialog_index, 403, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        SIP_AnswerToBye(ua_dialog_index, 481, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频, 收到BYE关闭消息处理:上级CMS ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Real-time video from superior CMS, receive close message process from superior CMS:user ID=%s, user IP address=%s, logic deviceID=%s, IPaddress=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

    /* 查找逻辑设备信息 */
    pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(pCrData->callee_id);

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        SIP_AnswerToBye(ua_dialog_index, 503, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_msg_proc() exit---: Get Callee GBLogic Device Info Error:callee_id=%s \r\n", pCrData->callee_id);
        return -1;
    }

    if (CALL_TYPE_AUDIO == pCrData->call_type)
    {
        i = route_bye_audio_msg_proc(cr_pos, ua_dialog_index, pCrData, pCalleeGBLogicDeviceInfo);
    }
    else
    {
        i = route_bye_video_msg_proc(cr_pos, ua_dialog_index, pCrData, pCalleeGBLogicDeviceInfo);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_bye_video_msg_proc
 功能描述  : 上级互联CMS发送过来的BYE 视频消息处理
 输入参数  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_bye_video_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;

    /* 根据逻辑设备所属域进行判断，决定消息走向 */
    if (1 == pCalleeGBLogicDeviceInfo->other_realm)
    {
        i = route_bye_route_video_msg_proc(cr_pos, ua_dialog_index, pCrData, pCalleeGBLogicDeviceInfo);
    }
    else
    {
        i = route_bye_sub_video_msg_proc(cr_pos, ua_dialog_index, pCrData, pCalleeGBLogicDeviceInfo);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_bye_sub_video_msg_proc
 功能描述  : 上级互联CMS发送过来的BYE 视频消息处理
 输入参数  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             GBDevice_info_t* pCalleeGBDeviceInfo
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_bye_sub_video_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;
    int other_cr_pos = -1;
    cr_t* pOtherCrData = NULL;
    GBDevice_info_t* pCalleeGBDeviceInfo = NULL;
    GBDevice_info_t* pCalleeCmsGBDeviceInfo = NULL;

    if (NULL == pCrData)
    {
        SIP_AnswerToBye(ua_dialog_index, 481, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_sub_video_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        SIP_AnswerToBye(ua_dialog_index, 503, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_sub_video_msg_proc() exit---: Get Callee GBLogic Device Info Error:callee_id=%s \r\n", pCrData->callee_id);
        return -1;
    }

    /* 查找对应的物理设备 */
    pCalleeGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, pCrData->callee_stream_type);

    if (NULL == pCalleeGBDeviceInfo)
    {
        /* 如果是智能分析流，可能是下级平台的点位，这个时候需要再查找一下主流设备 */
        if (EV9000_STREAM_TYPE_INTELLIGENCE == pCrData->callee_stream_type)
        {
            pCalleeCmsGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (NULL != pCalleeCmsGBDeviceInfo
                && EV9000_DEVICETYPE_SIPSERVER == pCalleeCmsGBDeviceInfo->device_type)
            {
                pCalleeGBDeviceInfo = pCalleeCmsGBDeviceInfo;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_sub_video_msg_proc() CalleeCmsGBDevice: id=%s, ip=%s \r\n", pCalleeCmsGBDeviceInfo->device_id, pCalleeCmsGBDeviceInfo->login_ip);
            }
        }
    }

    if (NULL == pCalleeGBDeviceInfo)
    {
        SIP_AnswerToBye(ua_dialog_index, 503, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_sub_video_msg_proc() exit---: Get Callee GBDevice Info Error \r\n");
        return -1;
    }

    /* Bye 消息可能是主叫发送的也可能是被叫发送的*/
    if (pCrData->callee_ua_index == ua_dialog_index)    /* 源端发送的Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, 收到前端BYE消息处理, 实时视频关闭:上级CMS ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, receive front-end BYE message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* 直接发送200 响应消息给被叫侧 */
        SIP_AnswerToBye(ua_dialog_index, 200, NULL);

        /* 通知TSU停止接收码流*/
        if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
            || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
        {
            /* 可能TSU中的缓存还没有放完，这个时候不能发送Bye 给客户端 */
            i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_sub_video_msg_proc() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* 如果是下级录像查看或者前端录像的情况下，需要转发Bye上级或者客户端 */
            if (pCalleeGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER) /* 下级CMS 的录像 */
            {
                /* 发送Bye 给主叫侧 */
                i = SIP_SendBye(pCrData->caller_ua_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
            }
            else if (pCalleeGBLogicDeviceInfo->record_type == 1) /* 调看前端录像 */
            {
                /* 发送Bye 给主叫侧 */
                i = SIP_SendBye(pCrData->caller_ua_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
            }
        }
        else
        {
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_sub_video_msg_proc() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* 发送Bye 给所有其他业务主叫侧用户*/
            i = send_bye_to_all_other_caller_by_callee_id_and_streamtype(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() send_bye_to_all_other_caller_by_callee_id_and_streamtype:callee_id=%s, callee_stream_type=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, i);

            /* 发送Bye 给主叫侧 */
            i = SIP_SendBye(pCrData->caller_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
        }

        /* 移除呼叫记录信息 */
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_sub_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_sub_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        return 0;
    }
    else if (pCrData->caller_ua_index == ua_dialog_index) /* 主叫发送的Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, 收到请求方BYE消息处理, 实时视频关闭:上级CMS ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, received the requester BYE message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* 通知TSU停止接收码流*/
        if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
            || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
        {
            i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_sub_video_msg_proc() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* 如果是下级录像查看或者前端录像的情况下，需要转发Bye到下级或者前端 */
            if (pCalleeGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER) /* 下级CMS 的录像 */
            {
                /*发送Bye 给被叫侧 */
                i = SIP_SendBye(pCrData->callee_ua_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
            }
            else if (pCalleeGBLogicDeviceInfo->record_type == 1) /* 调看前端录像 */
            {
                /*发送Bye 给被叫侧 */
                i = SIP_SendBye(pCrData->callee_ua_index);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
            }
        }
        else
        {
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_sub_video_msg_proc() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* 看是否有前端连接 */
            if (pCrData->callee_ua_index >= 0)
            {
                /* 查看是否有其他客户端业务 */
                other_cr_pos = is_GBLogic_device_has_other_service(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() is_GBLogic_device_has_other_service:other_cr_pos=%d \r\n", other_cr_pos);

                if (other_cr_pos < 0) /* 没有其他业务 */
                {
                    /*发送Bye 给被叫侧 */
                    i = SIP_SendBye(pCrData->callee_ua_index);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
                }
                else
                {
                    pOtherCrData = call_record_get(other_cr_pos);

                    if (NULL != pOtherCrData)
                    {
                        pOtherCrData->callee_ua_index = pCrData->callee_ua_index; /* 将前端的会话句柄拷贝到下个业务 */
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_sub_video_msg_proc() callee_ua_index=%d copy from %d to %d \r\n", pOtherCrData->callee_ua_index, cr_pos, other_cr_pos);
                    }
                }
            }
        }

        /* 直接发送200 响应消息主叫侧 */
        SIP_AnswerToBye(ua_dialog_index, 200, NULL);

        /* 移除呼叫记录信息 */
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_sub_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_sub_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        return 0;
    }

    /* 移除呼叫记录信息 */
    i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
    i = call_record_remove(cr_pos);

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_sub_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_sub_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    SIP_AnswerToBye(ua_dialog_index, 481, NULL);
    return -1;
}

/*****************************************************************************
 函 数 名  : route_bye_route_video_msg_proc
 功能描述  : 上级互联CMS发送过来的BYE 视频消息处理
 输入参数  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_bye_route_video_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;
    int other_cr_pos = -1;
    cr_t* pOtherCrData = NULL;

    if (NULL == pCrData)
    {
        SIP_AnswerToBye(ua_dialog_index, 481, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_route_video_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        SIP_AnswerToBye(ua_dialog_index, 503, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_route_video_msg_proc() exit---: Get Callee GBLogic Device Info Error:callee_id=%s \r\n", pCrData->callee_id);
        return -1;
    }

    /* Bye 消息可能是主叫发送的也可能是被叫发送的*/
    if (pCrData->callee_ua_index == ua_dialog_index)    /* 源端发送的Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, 收到前端BYE消息处理, 实时视频关闭:上级CMS ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, receive front-end BYE message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* 直接发送200 响应消息给被叫侧 */
        SIP_AnswerToBye(ua_dialog_index, 200, NULL);

        /* 通知TSU停止接收码流*/
        if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
            || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
        {
            /* 可能TSU中的缓存还没有放完，这个时候不能发送Bye 给客户端 */
            i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_route_video_msg_proc() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* 发送Bye 给主叫侧 */
            i = SIP_SendBye(pCrData->caller_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
        }
        else
        {
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_route_video_msg_proc() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* 发送Bye 给所有其他业务主叫侧用户*/
            i = send_bye_to_all_other_caller_by_callee_id_and_streamtype(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() send_bye_to_all_other_caller_by_callee_id_and_streamtype:callee_id=%s, callee_stream_type=%d, i=%d \r\n", pCrData->callee_id, pCrData->callee_stream_type, i);

            /* 发送Bye 给主叫侧 */
            i = SIP_SendBye(pCrData->caller_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);
        }

        /* 移除呼叫记录信息 */
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_route_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_route_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        return 0;
    }
    else if (pCrData->caller_ua_index == ua_dialog_index) /* 主叫发送的Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, 收到请求方BYE消息处理, 实时视频关闭:上级CMS ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, received the requester BYE message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* 通知TSU停止接收码流*/
        if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
            || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
        {
            i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_route_video_msg_proc() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /*发送Bye 给被叫侧 */
            i = SIP_SendBye(pCrData->callee_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
        }
        else
        {
            i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_route_video_msg_proc() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* 看是否有前端连接 */
            if (pCrData->callee_ua_index >= 0)
            {
                /* 查看是否有其他客户端业务 */
                other_cr_pos = is_GBLogic_device_has_other_service(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() is_GBLogic_device_has_other_service:other_cr_pos=%d \r\n", other_cr_pos);

                if (other_cr_pos < 0) /* 没有其他业务 */
                {
                    /*发送Bye 给被叫侧 */
                    i = SIP_SendBye(pCrData->callee_ua_index);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
                }
                else
                {
                    pOtherCrData = call_record_get(other_cr_pos);

                    if (NULL != pOtherCrData)
                    {
                        pOtherCrData->callee_ua_index = pCrData->callee_ua_index; /* 将前端的会话句柄拷贝到下个业务 */
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_route_video_msg_proc() callee_ua_index=%d copy from %d to %d \r\n", pOtherCrData->callee_ua_index, cr_pos, other_cr_pos);
                    }
                }
            }
        }

        /* 直接发送200 响应消息主叫侧 */
        SIP_AnswerToBye(ua_dialog_index, 200, NULL);

        /* 移除呼叫记录信息 */
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_route_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_route_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        return 0;
    }

    /* 移除呼叫记录信息 */
    i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
    i = call_record_remove(cr_pos);

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_route_video_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_route_video_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    SIP_AnswerToBye(ua_dialog_index, 481, NULL);
    return -1;
}

/*****************************************************************************
 函 数 名  : route_bye_audio_msg_proc
 功能描述  : 上级互联CMS发送过来的BYE 音频对讲消息处理
 输入参数  : int cr_pos
             int ua_dialog_index
             cr_t* pCrData
             GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_bye_audio_msg_proc(int cr_pos, int ua_dialog_index, cr_t* pCrData, GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo)
{
    int i = 0;

    if (NULL == pCrData)
    {
        SIP_AnswerToBye(ua_dialog_index, 481, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_audio_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        SIP_AnswerToBye(ua_dialog_index, 503, NULL);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_audio_msg_proc() exit---: Get Callee GBLogic Device Info Error:callee_id=%s \r\n", pCrData->callee_id);
        return -1;
    }

    /* Bye 消息可能是主叫发送的也可能是被叫发送的*/
    if (pCrData->callee_ua_index == ua_dialog_index)    /* 源端发送的Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时音频对讲请求, 收到前端BYE消息处理, 实时视频关闭:上级CMS ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time audio speaker request, receive front-end BYE message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* 直接发送200 响应消息给被叫侧 */
        SIP_AnswerToBye(ua_dialog_index, 200, NULL);

        /* 通知TSU停止转发音频流*/
        i = notify_tsu_delete_audio_transfer_task(pCrData->tsu_ip, pCrData->caller_sdp_ip, pCrData->caller_sdp_port);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_audio_msg_proc() notify_tsu_delete_audio_transfer_task Error:tsu_ip=%s, receive_ip=%s, receive_port=%d, i=%d \r\n", pCrData->tsu_ip, pCrData->caller_sdp_ip, pCrData->caller_sdp_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_audio_msg_proc() notify_tsu_delete_audio_transfer_task OK:tsu_ip=%s, receive_ip=%s, receive_port=%d, i=%d \r\n", pCrData->tsu_ip, pCrData->caller_sdp_ip, pCrData->caller_sdp_port, i);
        }

        /* 发送Bye 给主叫侧 */
        i = SIP_SendBye(pCrData->caller_ua_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_audio_msg_proc() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);

        /* 移除呼叫记录信息 */
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_audio_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_audio_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        return 0;
    }
    else if (pCrData->caller_ua_index == ua_dialog_index) /* 主叫发送的Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时音频对讲请求, 收到请求方BYE消息处理, 实时视频关闭:上级CMS ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time audio speaker request, received the requester BYE message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* 直接发送200 响应消息给主叫侧 */
        SIP_AnswerToBye(ua_dialog_index, 200, NULL);

        /* 通知TSU停止转发音频流*/
        i = notify_tsu_delete_audio_transfer_task(pCrData->tsu_ip, pCrData->caller_sdp_ip, pCrData->caller_sdp_port);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_bye_audio_msg_proc() notify_tsu_delete_audio_transfer_task Error:tsu_ip=%s, receive_ip=%s, receive_port=%d, i=%d \r\n", pCrData->tsu_ip, pCrData->caller_sdp_ip, pCrData->caller_sdp_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_audio_msg_proc() notify_tsu_delete_audio_transfer_task OK:tsu_ip=%s, receive_ip=%s, receive_port=%d, i=%d \r\n", pCrData->tsu_ip, pCrData->caller_sdp_ip, pCrData->caller_sdp_port, i);
        }

        /*发送Bye 给被叫侧 */
        i = SIP_SendBye(pCrData->callee_ua_index);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_bye_audio_msg_proc() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);

        /* 移除呼叫记录信息 */
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_audio_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_audio_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
        }

        return 0;
    }

    /* 移除呼叫记录信息 */
    i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
    i = call_record_remove(cr_pos);

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_audio_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_audio_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    SIP_AnswerToBye(ua_dialog_index, 481, NULL);
    return -1;
}

/*****************************************************************************
 函 数 名  : route_bye_response_msg_proc
 功能描述  : 上级互联CMS发送过来的BYE 回应消息处理
 输入参数  : int cr_pos
             int ua_dialog_index
             int response_code
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_bye_response_msg_proc(int cr_pos, int ua_dialog_index, int response_code)
{
    int i = 0;
    cr_t* pCrData = NULL;

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_bye_response_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_response_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求 收到BYE响应消息处理:上级CMS ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request received BYE response message handling: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

    /* Bye 响应消息可能是主叫发送的也可能是被叫发送的*/
    if (pCrData->callee_ua_index == ua_dialog_index)    /* 被叫发送的Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, 收到请求方BYE响应消息处理, 实时视频关闭:上级CMS ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, received the requester BYE response message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* 如果是实时视频业务，服务器需要将Bye响应消息转给主叫侧 */
        if (pCrData->caller_ua_index >= 0)
        {
            SIP_AnswerToBye(pCrData->caller_ua_index, response_code, NULL);
        }
    }
    else if (pCrData->caller_ua_index == ua_dialog_index) /* 主叫发送的Bye */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的实时视频请求, 收到前端BYE消息处理, 实时视频关闭:上级CMS ID=%s, IP地址=%s, 逻辑设备ID=%s, IP地址=%s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS real-time video request, receive front-end BYE message processing, real-time video closed: the higher the CMS, ID = % s IP address = % s, logical device ID = % s, IP address = % s", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

        /* 如果是实时视频业务，服务器需要将Bye响应消息转给被叫侧 */
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
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_bye_response_msg_proc() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_bye_response_msg_proc() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    return -1;
}

#if 1
/*****************************************************************************
 函 数 名  : route_info_msg_proc
 功能描述  : 上级互联CMS发送过来的Info 消息处理
 输入参数  : char* caller_id
             char* callee_id
             int dialog_index
             char* msg_body
             int msg_body_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_info_msg_proc(char* caller_id, char* callee_id, int dialog_index, char* msg_body, int msg_body_len)
{
    int i = 0;
    int cr_pos = -1;
    mansrtsp_t* rtsp = NULL;
    cr_t* pCrData = NULL;
    GBDevice_info_t* pCalleeGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo = NULL;
    float iScale = 0.0;
    int iLen = 0;
    int iNtp = 0;
    char tmpNtp[32] = {0};

    int iCalleeRoutePos = 0;
    route_info_t* pCalleeRouteInfo = NULL;

    if ((NULL == caller_id) || (NULL == callee_id) || (NULL == msg_body) || dialog_index < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    //处理过程

    /* 获取呼叫记录 */
    cr_pos = call_record_find_by_caller_index(dialog_index);

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Find Call Record Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS历史视频回放控制处理:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, cr_pos=%d", caller_id, pCrData->caller_ip, callee_id, cr_pos);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS history video playback control process: the requester, ID = % s IP address = % s, logical device ID = % s, cr_pos = % d", caller_id, pCrData->caller_ip, callee_id, cr_pos);

    /* 查找逻辑设备信息 */
    pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_msg_proc() exit---: Get Callee GBlogicDevice Info Error \r\n");
        return -1;
    }

    /* 根据逻辑设备所属域进行判断，决定消息走向 */
    if (1 == pCalleeGBLogicDeviceInfo->other_realm)
    {
        /* 查找上级路由信息 */
        iCalleeRoutePos = route_info_find(pCalleeGBLogicDeviceInfo->cms_id);

        if (iCalleeRoutePos >= 0)
        {
            pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

            if (NULL != pCalleeRouteInfo)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS历史视频回放控制处理, 转发到上级CMS:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, cr_pos=%d, 转发上级CMS ID=%s, IP地址=%s, 端口号=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, cr_pos, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS video playback control processing history, forwarded to the superior CMS: requester, ID = % s IP address = % s, logical device ID = % s, cr_pos = % d, forwarding the superior CMS, ID = % s = % s IP address, port number = % d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, cr_pos, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);

                /* 转发消息出去 */
                i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                }
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() route_info_get Error:CalleeRoutePos=%d \r\n", iCalleeRoutePos);
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() route_info_get Error:cms_id=%s \r\n", pCalleeGBLogicDeviceInfo->cms_id);
        }
    }
    else
    {
        /* 查找对应的物理设备 */
        pCalleeGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, pCrData->callee_stream_type);

        if (NULL != pCalleeGBDeviceInfo)
        {
            if (pCalleeGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
                || pCalleeGBLogicDeviceInfo->record_type == 1) /* 下级CMS 获取前端录像*/
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS历史视频回放控制处理, 转发到前端设备:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, cr_pos=%d, 转发前端设备ID=%s, IP地址=%s, 端口号=%d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, cr_pos, pCalleeGBDeviceInfo->device_id, pCalleeGBDeviceInfo->login_ip, pCalleeGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS history video playback control processing, forwarded to the front-end equipment: the requester, ID = % s IP address = % s, logical device ID = % s, cr_pos = % d, forwarding the front-end device ID = % s, = % s IP address, port number = % d", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, cr_pos, pCalleeGBDeviceInfo->device_id, pCalleeGBDeviceInfo->login_ip, pCalleeGBDeviceInfo->login_port);

                /* 转发消息出去 */
                i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                }
            }
            else
            {
                //根据RTSP中的信息，传输给TSU
                if (0 == strncmp(msg_body, "PLAY", 4))
                {
                    i = notify_tsu_start_replay(pCrData->tsu_ip, pCrData->task_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_start_replay Error:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_start_replay OK:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    }

                    /* 解析MANSRTSP 消息 */
                    i = mansrtsp_init(&rtsp);

                    if (i != 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Init Error \r\n");
                        return -1;
                    }

                    i = mansrtsp_parse(rtsp, msg_body);

                    if (i != 0)
                    {
                        mansrtsp_free(rtsp);
                        osip_free(rtsp);
                        rtsp = NULL;

                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Parse Error \r\n");
                        return -1;
                    }

                    /* 快放慢放 */
                    if (NULL != rtsp->scale && NULL != rtsp->scale->number)
                    {
                        iScale = atof(rtsp->scale->number);

                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() Scale Number=%f \r\n", iScale);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_info_msg_proc() No Scale Value \r\n");
                    }

                    /* 拖放 */
                    if (NULL != rtsp->range && NULL != rtsp->range->start)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() NPT Start=%s \r\n", rtsp->range->start);

                        if (0 != sstrcmp(rtsp->range->start, "now")
                            && 0 != sstrcmp(rtsp->range->start, "196")
                            && 0 != sstrcmp(rtsp->range->start, "196-")
                            && 0 != sstrcmp(rtsp->range->start, "0-")) /* 老版本客户端里面暂停后的继续命令里面携带的是196，防止从头开始播放 */
                        {
                            iLen = strlen(rtsp->range->start);

                            if (iLen > 0)
                            {
                                osip_strncpy(tmpNtp, rtsp->range->start, iLen - 1); /* 去掉最后的"-" */
                                iNtp = osip_atoi(tmpNtp);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Get NPT Error \r\n");
                            }
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Range Start Now \r\n");
                        }
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_info_msg_proc() No NPT Value \r\n");
                    }

                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS历史视频回放控制处理, PLAY命令处理:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, cr_pos=%d, Scale=%f, NPT=%d", caller_id, pCrData->caller_ip, callee_id, cr_pos, iScale, iNtp);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS history video playback control processing , PLAY command processing: the requester, ID = % s IP address = % s, logical device ID = % s, cr_pos = % d, Scale = % f, NPT = % d", caller_id, pCrData->caller_ip, callee_id, cr_pos, iScale, iNtp);

                    if (iScale > 0 || iNtp > 0)
                    {
                        if (iScale > 0 && iScale != 1.0)
                        {
                            pCrData->iScale = iScale;
                        }

                        if (iScale > 0)
                        {
                            i = notify_set_replay_speed(pCrData->tsu_ip, pCrData->task_id, iScale);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_set_replay_speed Error:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_set_replay_speed OK:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                            }
                        }

                        if (iNtp > 0)
                        {
                            i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                            }
                        }
                    }

                    mansrtsp_free(rtsp);
                    osip_free(rtsp);
                    rtsp = NULL;

                    return i;
                }
                else if (0 == strncmp(msg_body, "PAUSE", 5))
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS历史视频回放控制处理, PAUSE命令处理:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, cr_pos=%d", caller_id, pCrData->caller_ip, callee_id, cr_pos);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS history video playback control processing , PAUSE command processing: the requester, ID = % s IP address = % s, logical device ID = % s, cr_pos = % d", caller_id, pCrData->caller_ip, callee_id, cr_pos);

                    i = notify_tsu_pause_replay(pCrData->tsu_ip, pCrData->task_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_pause_replay Error:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_pause_replay OK:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    }

                    return i;
                }
                else if (0 == strncmp(msg_body, "TEARDOWN", 8))
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS历史视频回放控制处理, TEARDOWN命令处理:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, cr_pos=%d", caller_id, pCrData->caller_ip, callee_id, cr_pos);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS historical video playback control processing , TEARDOWN command processing: the requester, ID = % s IP address = % s, logical device ID = % s, cr_pos = % d", caller_id, pCrData->caller_ip, callee_id, cr_pos);

                    i = notify_tsu_stop_replay(pCrData->tsu_ip, pCrData->task_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_stop_replay Error:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_stop_replay OK:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
                    }

                    return i;
                }
                else if (0 == strncmp(msg_body, "SEEK", 4))
                {
                    /* 解析MANSRTSP 消息 */
                    i = mansrtsp_init(&rtsp);

                    if (i != 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Init Error \r\n");
                        return -1;
                    }

                    i = mansrtsp_parse(rtsp, msg_body);

                    if (i != 0)
                    {
                        mansrtsp_free(rtsp);
                        osip_free(rtsp);
                        rtsp = NULL;

                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Parse Error \r\n");
                        return -1;
                    }

                    if (NULL != rtsp->range && NULL != rtsp->range->start)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() NPT Start=%s \r\n", rtsp->range->start);

                        if (0 != sstrcmp(rtsp->range->start, "now"))
                        {
                            iLen = strlen(rtsp->range->start);

                            if (iLen > 0)
                            {
                                osip_strncpy(tmpNtp, rtsp->range->start, iLen - 1); /* 去掉最后的"-" */
                                iNtp = osip_atoi(tmpNtp);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Get NPT Error \r\n");
                            }

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS历史视频回放控制处理, SEEK命令处理:请求方ID=%s, IP地址=%s, 逻辑设备ID=%s, cr_pos=%d, NPT=%d", caller_id, pCrData->caller_ip, callee_id, cr_pos, iNtp);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS historical video playback control processing, The SEEK command processing: the requester, ID = % s IP address = % s, logical device ID = % s, cr_pos = % d, NPT = % d", caller_id, pCrData->caller_ip, callee_id, cr_pos, iNtp);

                            if (iNtp > 0)
                            {
                                i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, osip_atoi(rtsp->range->start), i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, osip_atoi(rtsp->range->start), i);
                                }
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() NPT Value Error \r\n");
                            }
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Range Start Now \r\n");
                        }
                    }

                    mansrtsp_free(rtsp);
                    osip_free(rtsp);
                    rtsp = NULL;

                    return i;
                }
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() GBDevice_info_get_by_stream_type Error:device_id=%s, callee_stream_type=%d \r\n", pCalleeGBLogicDeviceInfo->device_id, pCrData->callee_stream_type);
        }
    }

    return -1;
}
#endif

#if 0
/*****************************************************************************
 函 数 名  : route_info_msg_proc
 功能描述  : 上级互联CMS发送过来的Info 消息处理
 输入参数  : char* caller_id
             char* callee_id
             int dialog_index
             char* msg_body
             int msg_body_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_info_msg_proc(char* caller_id, char* callee_id, int dialog_index, char* msg_body, int msg_body_len)
{
    int i = 0;
    int cr_pos = -1;
    mansrtsp_t* rtsp = NULL;
    cr_t* pCrData = NULL;
    GBDevice_info_t* pCalleeGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pCalleeGBLogicDeviceInfo = NULL;
    float iScale = 0.0;
    int iLen = 0;
    int iNtp = 0;
    char tmpBuf[128] = {0};
    char tmpNtp[32] = {0};

    int iCalleeRoutePos = 0;
    route_info_t* pCalleeRouteInfo = NULL;

    if ((NULL == caller_id) || (NULL == callee_id) || (NULL == msg_body) || dialog_index < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    //处理过程

    /* 获取呼叫记录 */
    cr_pos = call_record_find_by_caller_index(dialog_index);

    //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() call_record_find_by_caller_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Find Call Record Error:dialog_index=%d \r\n", dialog_index);
        return -1;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Get Call Record Error:cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc():caller_id=%s, caller_ip=%s, callee_id=%s, callee_ip=%s \r\n", pCrData->caller_id, pCrData->caller_ip, pCrData->callee_id, pCrData->callee_ip);

#if 0

    /* 查找逻辑设备信息 */
    pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL == pCalleeGBLogicDeviceInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_msg_proc() exit---: Get Callee GBlogicDevice Info Error \r\n");
        return -1;
    }

    /* 查找对应的物理设备 */
    pCalleeGBDeviceInfo = pCalleeGBLogicDeviceInfo->ptGBDeviceInfo;

    if (NULL == pCalleeGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_info_msg_proc() exit---: Get Callee GBDevice Info Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() CalleeGBDeviceInfo device_type=%d \r\n", pCalleeGBDeviceInfo->device_type);

    if (pCalleeGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER || pCalleeGBLogicDeviceInfo->record_type == 1) /* 下级CMS 获取前端录像*/
    {
        /* 转发消息出去 */
        SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

        return 0;
    }
    else
#endif
    {
        // TODO:根据RTSP中的信息，传输给TSU
        if (0 == strncmp(msg_body, "PLAY", 4))
        {
            i = notify_tsu_start_replay(pCrData->tsu_ip, pCrData->task_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_start_replay Error:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_start_replay OK:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            /* 解析MANSRTSP 消息 */
            i = mansrtsp_init(&rtsp);

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Init Error \r\n");
                return -1;
            }

            i = mansrtsp_parse(rtsp, msg_body);

            if (i != 0)
            {
                mansrtsp_free(rtsp);
                osip_free(rtsp);
                rtsp = NULL;

                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Parse Error \r\n");
                return -1;
            }

            /* 快放慢放 */
            if (NULL != rtsp->scale && NULL != rtsp->scale->number)
            {
                iScale = atof(rtsp->scale->number);

                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() Scale Number=%f \r\n", iScale);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_info_msg_proc() No Scale Value \r\n");
            }

            /* 拖放 */
            if (NULL != rtsp->range && NULL != rtsp->range->start)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() NPT Start=%s \r\n", rtsp->range->start);

                if (0 != sstrcmp(rtsp->range->start, "now")
                    && 0 != sstrcmp(rtsp->range->start, "196")
                    && 0 != sstrcmp(rtsp->range->start, "196-")
                    && 0 != sstrcmp(rtsp->range->start, "0-")) /* 老版本客户端里面暂停后的继续命令里面携带的是196，防止从头开始播放 */
                {
                    iLen = strlen(rtsp->range->start);

                    if (iLen > 0)
                    {
                        osip_strncpy(tmpNtp, rtsp->range->start, iLen - 1); /* 去掉最后的"-" */
                        iNtp = osip_atoi(tmpNtp);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Get NPT Error \r\n");
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Range Start Now \r\n");
                }
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_info_msg_proc() No NPT Value \r\n");
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() Scale=%f, NPT=%d \r\n", iScale, iNtp);

            if (iScale > 0 || iNtp > 0)
            {
                if (iScale > 0 && iScale != 1.0)
                {
                    pCrData->iScale = iScale;
                }

                /* 查找逻辑设备信息 */
                pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

                if (NULL != pCalleeGBLogicDeviceInfo)
                {
                    /* 根据逻辑设备所属域进行判断，决定消息走向 */
                    if (1 == pCalleeGBLogicDeviceInfo->other_realm)
                    {
                        /* 查找上级路由信息 */
                        iCalleeRoutePos = route_info_find(pCalleeGBLogicDeviceInfo->cms_id);

                        if (iCalleeRoutePos >= 0)
                        {
                            pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

                            if (NULL != pCalleeRouteInfo)
                            {
                                if (iNtp > 0)
                                {
                                    i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }

                                    /* 拖放之前先发送一下恢复命令，大华NVR在暂停状态下，不响应拖放命令 */
                                    if (iScale > 0 && iScale != 1.0)
                                    {
                                        snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nScale: %f\r\nRange: npt=now-\r\n", rtsp->cseq->number, pCrData->iScale);
                                    }
                                    else
                                    {
                                        snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nRange: npt=now-\r\n", rtsp->cseq->number);
                                    }

                                    i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, tmpBuf, strlen(tmpBuf));

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }

                                    /* 修改拖放时间 */
                                    if (pCrData->iPlaybackTimeGap > 0)
                                    {
                                        if (iScale > 0 && iScale != 1.0)
                                        {
                                            snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nScale: %f\r\nRange: npt=%d-\r\n", rtsp->cseq->number, pCrData->iScale, iNtp - pCrData->iPlaybackTimeGap);
                                        }
                                        else
                                        {
                                            snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nRange: npt=%d-\r\n", rtsp->cseq->number, iNtp - pCrData->iPlaybackTimeGap);
                                        }

                                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() SIP_SendInfoWithinDialog:Scale=%f, NPT=%d \r\n", iScale, iNtp - pCrData->iPlaybackTimeGap);

                                        i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, tmpBuf, strlen(tmpBuf));

                                        if (0 != i)
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                        else
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                    }
                                    else
                                    {
                                        /* 转发消息出去 */
                                        i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                                        if (0 != i)
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                        else
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                    }
                                }
                                else
                                {
                                    /* 转发消息出去 */
                                    i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }
                                }
                            }
                            else
                            {
                                if (iScale > 0)
                                {
                                    i = notify_set_replay_speed(pCrData->tsu_ip, pCrData->task_id, iScale);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_set_replay_speed Error:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_set_replay_speed OK:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                    }
                                }

                                if (iNtp > 0)
                                {
                                    i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (iScale > 0)
                            {
                                i = notify_set_replay_speed(pCrData->tsu_ip, pCrData->task_id, iScale);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_set_replay_speed Error:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_set_replay_speed OK:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                }
                            }

                            if (iNtp > 0)
                            {
                                i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                }
                            }
                        }
                    }
                    else
                    {
                        /* 查找对应的物理设备 */
                        pCalleeGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, pCrData->callee_stream_type);

                        if (NULL != pCalleeGBDeviceInfo)
                        {
                            if (pCalleeGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER || pCalleeGBLogicDeviceInfo->record_type == 1) /* 下级CMS或者前端录像*/
                            {
                                if (iNtp > 0)
                                {
                                    i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }

                                    /* 拖放之前先发送一下恢复命令，大华NVR在暂停状态下，不响应拖放命令 */
                                    if (iScale > 0 && iScale != 1.0)
                                    {
                                        snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nScale: %f\r\nRange: npt=now-\r\n", rtsp->cseq->number, pCrData->iScale);
                                    }
                                    else
                                    {
                                        snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nRange: npt=now-\r\n", rtsp->cseq->number);
                                    }

                                    i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, tmpBuf, strlen(tmpBuf));

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }

                                    /* 修改拖放时间 */
                                    if (pCrData->iPlaybackTimeGap > 0)
                                    {
                                        if (iScale > 0 && iScale != 1.0)
                                        {
                                            snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nScale: %f\r\nRange: npt=%d-\r\n", rtsp->cseq->number, pCrData->iScale, iNtp - pCrData->iPlaybackTimeGap);
                                        }
                                        else
                                        {
                                            snprintf(tmpBuf, 128, (char*)"PLAY RTSP/1.0\r\nCSeq: %s\r\nRange: npt=%d-\r\n", rtsp->cseq->number, iNtp - pCrData->iPlaybackTimeGap);
                                        }

                                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() SIP_SendInfoWithinDialog:Scale=%f, NPT=%d \r\n", iScale, iNtp - pCrData->iPlaybackTimeGap);

                                        i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, tmpBuf, strlen(tmpBuf));

                                        if (0 != i)
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                        else
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                    }
                                    else
                                    {
                                        /* 转发消息出去 */
                                        i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                                        if (0 != i)
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                        else
                                        {
                                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                        }
                                    }
                                }
                                else
                                {
                                    /* 转发消息出去 */
                                    i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                                    }
                                }
                            }
                            else
                            {
                                if (iScale > 0)
                                {
                                    i = notify_set_replay_speed(pCrData->tsu_ip, pCrData->task_id, iScale);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_set_replay_speed Error:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_set_replay_speed OK:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                    }
                                }

                                if (iNtp > 0)
                                {
                                    i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                                    if (0 != i)
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }
                                    else
                                    {
                                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (iScale > 0)
                            {
                                i = notify_set_replay_speed(pCrData->tsu_ip, pCrData->task_id, iScale);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_set_replay_speed Error:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_set_replay_speed OK:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                                }
                            }

                            if (iNtp > 0)
                            {
                                i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                                }
                            }
                        }
                    }
                }
                else
                {
                    if (iScale > 0)
                    {
                        i = notify_set_replay_speed(pCrData->tsu_ip, pCrData->task_id, iScale);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_set_replay_speed Error:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_set_replay_speed OK:tsu_ip=%s, task_id=%s, Scale=%f, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iScale, i);
                        }
                    }

                    if (iNtp > 0)
                    {
                        i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, iNtp, i);
                        }
                    }
                }
            }

            return i;
        }
        else if (0 == strncmp(msg_body, "PAUSE", 5))
        {
            i = notify_tsu_pause_replay(pCrData->tsu_ip, pCrData->task_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_pause_replay Error:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_pause_replay OK:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            return i;
        }
        else if (0 == strncmp(msg_body, "TEARDOWN", 8))
        {
            i = notify_tsu_stop_replay(pCrData->tsu_ip, pCrData->task_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_stop_replay Error:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_stop_replay OK:tsu_ip=%s, task_id=%s, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
            }

            return i;
        }
        else if (0 == strncmp(msg_body, "SEEK", 4))
        {
            /* Seek 命令需要同时发送给前端 */
            /* 查找逻辑设备信息 */
            pCalleeGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

            if (NULL != pCalleeGBLogicDeviceInfo)
            {
                /* 根据逻辑设备所属域进行判断，决定消息走向 */
                if (1 == pCalleeGBLogicDeviceInfo->other_realm)
                {
                    /* 查找上级路由信息 */
                    iCalleeRoutePos = route_info_find(pCalleeGBLogicDeviceInfo->cms_id);

                    if (iCalleeRoutePos >= 0)
                    {
                        pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

                        if (NULL != pCalleeRouteInfo)
                        {
                            /* 转发消息出去 */
                            i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                            }
                        }
                    }
                }
                else
                {
                    /* 查找对应的物理设备 */
                    pCalleeGBDeviceInfo = GBDevice_info_get_by_stream_type(pCalleeGBLogicDeviceInfo, pCrData->callee_stream_type);

                    if (NULL != pCalleeGBDeviceInfo)
                    {
                        if (pCalleeGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER || pCalleeGBLogicDeviceInfo->record_type == 1) /* 下级CMS 获取前端录像*/
                        {
                            /* 转发消息出去 */
                            i = SIP_SendInfoWithinDialog(pCrData->callee_ua_index, msg_body, msg_body_len);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() SIP_SendInfoWithinDialog Error:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() SIP_SendInfoWithinDialog OK:callee_ua_index=%d, callee_id=%s, callip=%s, iRet=%d \r\n", pCrData->callee_ua_index, pCrData->callee_id, pCrData->callee_ip, i);
                            }
                        }
                    }
                }
            }

            /* 解析MANSRTSP 消息 */
            i = mansrtsp_init(&rtsp);

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Init Error \r\n");
                return -1;
            }

            i = mansrtsp_parse(rtsp, msg_body);

            if (i != 0)
            {
                mansrtsp_free(rtsp);
                osip_free(rtsp);
                rtsp = NULL;

                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() exit---: Mansrtsp Parse Error \r\n");
                return -1;
            }

            if (NULL != rtsp->range && NULL != rtsp->range->start)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_info_msg_proc() NPT Start=%s \r\n", rtsp->range->start);

                if (0 != sstrcmp(rtsp->range->start, "now"))
                {
                    iLen = strlen(rtsp->range->start);

                    if (iLen > 0)
                    {
                        osip_strncpy(tmpNtp, rtsp->range->start, iLen - 1); /* 去掉最后的"-" */
                        iNtp = osip_atoi(tmpNtp);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Get NPT Error \r\n");
                    }

                    if (iNtp > 0)
                    {
                        i = notify_tsu_seek_replay(pCrData->tsu_ip, pCrData->task_id, iNtp);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() notify_tsu_seek_replay Error:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, osip_atoi(rtsp->range->start), i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_info_msg_proc() notify_tsu_seek_replay OK:tsu_ip=%s, task_id=%s, start=%d, iRet=%d \r\n", pCrData->tsu_ip, pCrData->task_id, osip_atoi(rtsp->range->start), i);
                        }
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() NPT Value Error \r\n");
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_info_msg_proc() Range Start Now \r\n");
                }
            }

            mansrtsp_free(rtsp);
            osip_free(rtsp);
            rtsp = NULL;

            return i;
        }
    }

    return -1;
}
#endif

/*****************************************************************************
 函 数 名  : route_message_msg_proc
 功能描述  :上级互联CMS发送过来的Message消息处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_message_msg_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, char* msg_body, int msg_body_len, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    xml_type_t xml_type = XML_TYPE_NULL;
    CPacket inPacket;
    vector<string> NodeName_Vector;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_message_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == callee_id) || (NULL == msg_body))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_message_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG,  "route_message_msg_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    //解析XML
    iRet = inPacket.BuiltTree(msg_body, msg_body_len);//生成DOM树结构.

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_message_msg_proc() exit---: XML Build Tree Error \r\nmsg=%s \r\n", msg_body);
        return iRet;
    }

    NodeName_Vector.clear();
    DOMDocument* pDOMDocument = inPacket.GetDOMDocument();
    DOMElement* pDOMElement = pDOMDocument->get_root();
    pDOMElement->ClearNodeNumber();

    if (pDOMElement->GetNodeName(NodeName_Vector) <= 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_message_msg_proc() exit---: Get Node Name Error \r\n");
        return -1;
    }

    /* 解析出xml的消息类型 */
    xml_type = get_xml_type_from_xml_body(NodeName_Vector, inPacket);
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_message_msg_proc() get_xml_type_from_xml_body:xml_type=%d \r\n", xml_type);

    switch (xml_type)
    {
        case XML_CONTROL_DEVICECONTROL : /* 设备控制 */
            i = route_device_control_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_CONTROL_DEVICECONFIG : /* 设备配置 */
            i = route_device_config_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_CONTROL_SETDEVICEVIDEOPARAM :    /* 设置前端图像参数 */
            i = route_set_device_video_param_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_CONTROL_REQUESTIFRAMDATA :        /* 请求I 帧 */
            i = route_request_ifram_data_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_CONTROL_AUTOZOOMIN :              /* 点击放大*/
            i = route_control_autozoomin_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_CONTROL_SETDEVICEXYPARAM :
            i = route_set_device_xy_param_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_CONTROL_EXECUTEPRESET : /* 执行预置位 */
            i = route_execute_preset_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_QUERY_CATALOG :
            i = route_query_catalog_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_QUERY_DEVICEINFO :
            i = route_query_device_info_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_QUERY_DEVICESTATUS :
            i = route_query_device_status_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_QUERY_DEVICECONFIG : /* 设备配置查询 */
            i = route_query_device_config_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_QUERY_RECORDINFO :
            i = route_query_record_info_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_QUERY_GETPRESET :                    /* 获取预置位 */
            i = route_query_preset_info_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_QUERY_DEVICEGROUP :                /* 逻辑设备分组信息 */
            i = route_query_device_group_config_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_QUERY_DEVICEMAPGROUP :             /* 逻辑设备分组映射关系 */
            i = route_query_device_map_group_config_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_QUERY_DEVICEVIDEOPARAM :        /* 获取前端图像参数*/
            i = route_query_device_video_param_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_QUERY_DEVICEPRESET :            /* 获取前端预置位*/
            i = route_query_device_preset_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_QUERY_GETDBIP :                 /* 获取数据库IP地址 */
            i = route_get_db_ip_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_QUERY_TOPOLOGYPHYDEVICE :       /* 获取拓扑物理设备配置表 */
            i = route_query_topology_phydevice_config_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_RESPONSE_QUERY_GETPRESET:
        case XML_RESPONSE_GETDEVICEPRESET:       /* 获取前端预置位回应  */
            i = route_preset_info_response_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_NOTIFY_ALARM :
            i = route_notify_alarm_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_NOTIFY_KEEPLIVE :
            i = route_notify_keep_alive_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_NOTIFY_TV_STATUS :
            i = route_notify_tv_status_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_NOTIFY_CMS_RESTART :
            i = route_notify_cms_restart_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_NOTIFY_CATALOG:
            i = route_notify_catalog_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_NOTIFY_STATUS :
            i = route_notify_status_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_RESPONSE_DEVICECONTROL :
            i = route_device_control_response_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_RESPONSE_ALARM :
            i = route_notify_alarm_response_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_RESPONSE_CATALOG :
            i = route_query_catalog_response_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_RESPONSE_DEVICEINFO :
            i = route_device_info_response_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_RESPONSE_DEVICESTATUS:
            i = route_device_status_response_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        case XML_RESPONSE_RECORDINFO:
            i = route_record_info_response_proc(pRouteInfo, caller_id, callee_id, inPacket);
            break;

        default:
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_message_msg_proc() exit---: Not Support Message Type:%d \r\n", xml_type);
            return -1;
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_notify_msg_proc
 功能描述  : 互连路由Notify消息处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             char* msg_body
             int msg_body_len
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月9日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_notify_msg_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, char* msg_body, int msg_body_len, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    xml_type_t xml_type = XML_TYPE_NULL;
    CPacket inPacket;
    vector<string> NodeName_Vector;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "device_notify_msg_proc() exit---: GBDevice Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == callee_id) || (NULL == msg_body))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG,  "device_notify_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG,  "device_notify_msg_proc() exit---: Device Srv DB Oper Error \r\n");
        return -1;
    }

    //解析XML
    iRet = inPacket.BuiltTree(msg_body, msg_body_len);//生成DOM树结构.

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "device_notify_msg_proc() exit---: XML Build Tree Error \r\nmsg=%s \r\n", msg_body);
        return iRet;
    }

    NodeName_Vector.clear();
    DOMDocument* pDOMDocument = inPacket.GetDOMDocument();
    DOMElement* pDOMElement = pDOMDocument->get_root();
    pDOMElement->ClearNodeNumber();

    if (pDOMElement->GetNodeName(NodeName_Vector) <= 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "device_notify_msg_proc() exit---: Get XML Node Name Error \r\n");
        return -1;
    }

    /* 解析出xml的消息类型 */
    xml_type = get_xml_type_from_xml_body(NodeName_Vector, inPacket);
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "device_notify_msg_proc() get_xml_type_from_xml_body:xml_type=%d \r\n", xml_type);

    switch (xml_type)
    {
        case XML_NOTIFY_CATALOG:
            i = route_notify_catalog_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        case XML_NOTIFY_STATUS :
            i = route_notify_status_proc(pRouteInfo, caller_id, callee_id, inPacket, pRoute_Srv_dboper);
            break;

        default:
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "device_notify_msg_proc() exit---: Not Support Message Type:%d \r\n", xml_type);
            return -1;
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_subscribe_msg_proc
 功能描述  : 上级互联路由过来的订阅消息处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             int event_id
             int subscribe_expires
             char* msg_body
             int msg_body_len
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月10日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_subscribe_msg_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, int event_id, int subscribe_expires, char* msg_body, int msg_body_len, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    xml_type_t xml_type = XML_TYPE_NULL;
    CPacket inPacket;
    vector<string> NodeName_Vector;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == callee_id) || (NULL == msg_body))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG,  "route_subscribe_msg_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    //解析XML
    iRet = inPacket.BuiltTree(msg_body, msg_body_len);//生成DOM树结构.

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_subscribe_msg_proc() exit---: XML Build Tree Error \r\nmsg=%s \r\n", msg_body);
        return iRet;
    }

    NodeName_Vector.clear();
    DOMDocument* pDOMDocument = inPacket.GetDOMDocument();
    DOMElement* pDOMElement = pDOMDocument->get_root();
    pDOMElement->ClearNodeNumber();

    if (pDOMElement->GetNodeName(NodeName_Vector) <= 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_subscribe_msg_proc() exit---: Get Node Name Error \r\n");
        return -1;
    }

    /* 解析出xml的消息类型 */
    xml_type = get_xml_type_from_xml_body(NodeName_Vector, inPacket);
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_subscribe_msg_proc() get_xml_type_from_xml_body:xml_type=%d \r\n", xml_type);

    switch (xml_type)
    {
        case XML_QUERY_CATALOG :
            i = route_subscribe_query_catalog_proc(pRouteInfo, caller_id, callee_id, event_id, subscribe_expires, inPacket, pRoute_Srv_dboper);
            break;

        default:
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_subscribe_msg_proc() exit---: Not Support Message Type:%d \r\n", xml_type);
            return -1;
    }

    return i;
}

int route_subscribe_within_dialog_msg_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, int dialog_index, int subscribe_expires, char* msg_body, int msg_body_len, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int iRet = 0;
    xml_type_t xml_type = XML_TYPE_NULL;
    CPacket inPacket;
    vector<string> NodeName_Vector;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_within_dialog_msg_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == callee_id) || (NULL == msg_body))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_within_dialog_msg_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG,  "route_subscribe_within_dialog_msg_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    //解析XML
    iRet = inPacket.BuiltTree(msg_body, msg_body_len);//生成DOM树结构.

    if (iRet < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_subscribe_within_dialog_msg_proc() exit---: XML Build Tree Error \r\nmsg=%s \r\n", msg_body);
        return iRet;
    }

    NodeName_Vector.clear();
    DOMDocument* pDOMDocument = inPacket.GetDOMDocument();
    DOMElement* pDOMElement = pDOMDocument->get_root();
    pDOMElement->ClearNodeNumber();

    if (pDOMElement->GetNodeName(NodeName_Vector) <= 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_subscribe_within_dialog_msg_proc() exit---: Get Node Name Error \r\n");
        return -1;
    }

    /* 解析出xml的消息类型 */
    xml_type = get_xml_type_from_xml_body(NodeName_Vector, inPacket);
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_subscribe_within_dialog_msg_proc() get_xml_type_from_xml_body:xml_type=%d \r\n", xml_type);

    switch (xml_type)
    {
        case XML_QUERY_CATALOG :
            i = route_subscribe_witin_dialog_query_catalog_proc(pRouteInfo, caller_id, callee_id, dialog_index, subscribe_expires, inPacket, pRoute_Srv_dboper);
            break;

        default:
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_subscribe_within_dialog_msg_proc() exit---: Not Support Message Type:%d \r\n", xml_type);
            return -1;
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_device_control_proc
 功能描述  : 上级互联CMS发送过来的设备控制消息处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_device_control_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int iLen = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strPTZCmd[64] = {0};      /* 球机/云台控制命令 */
    char strRecordCmd[32] = {0};   /* 录像控制命令 */
    char strGuardCmd[32] = {0};    /* 报警布防/撤防命令 */
    char strLockCmd[32] = {0};     /* 点位锁定命令 */
    char strTeleBootCmd[32] = {0}; /* 远程启动控制命令 */
    char strAlarmCmd[32] = {0};    /* 报警复位命令 */

    time_t utc_time;
    struct tm local_time = { 0 };
    char str_date[12] = {0};
    char str_time[12] = {0};
    char strTime[32] = {0};
    char strReason[128] = {0};     /* 原因 */

    CPacket outPacket;
    DOMElement* AccNode = NULL;

    int iCalleeRoutePos = 0;
    route_info_t* pCalleeRouteInfo = NULL;

    char strTransferSN[32] = {0};
    DOMElement* AccSnNode = NULL;
    unsigned int old_xml_sn = 0;
    unsigned int transfer_xml_sn = 0;

    unsigned char szPtzCmd[PTZCMD_28181_LEN + 1] = {0};

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* 设备控制的命令直接转发给前段设备，不做处理
          控制流程见9.3.2

          命令包括如下字段:
          <!-- 命令类型：设备控制（必选） -->
          <element name="CmdType" fixed ="DeviceControl" />
          <!-- 命令序列号（必选） -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- 目标设备编码（必选） -->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- 球机/云台控制命令（可选，控制码应符合附录L中的规定) -->
          <element name=" PTZCmd " type="tg:PTZType" />
          <!-- 远程启动控制命令（可选） -->
          <element name="TeleBoot" minOccurs= "0">
          <restriction base="string">
          <enumeration value="Boot"/>
          </restriction>
          </element>
          <!-- 录像控制命令（可选） -->
          <element name=" RecordCmd " type="tg:recordType" minOccurs= "0"/>
          <!-- 报警布防/撤防命令（可选） -->
          <element name=" GuardCmd " type="tg:guardType" minOccurs= "0"/>
          <!-- 报警复位命令（可选） -->
          <element name="AlarmCmd" minOccurs= "0">
          <restriction base="string">
          <enumeration value="ResetAlarm"/>
          </restriction>
          </element>
          <!-- 扩展信息，可多项 -->
          <element name= "Info" minOccurs= "0" maxOccurs="unbounded">
          <restriction base= "string">
          <maxLength value= "1024" />
          </restriction>
          </element>
      */

    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"PTZCmd", strPTZCmd);
    inPacket.GetElementValue((char*)"RecordCmd", strRecordCmd);
    inPacket.GetElementValue((char*)"GuardCmd", strGuardCmd);
    inPacket.GetElementValue((char*)"LockCmd", strLockCmd);
    inPacket.GetElementValue((char*)"TeleBoot", strTeleBootCmd);
    inPacket.GetElementValue((char*)"AlarmCmd", strAlarmCmd);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_device_control_proc() \
    \r\n XML Para: \
    \r\n SN=%s, DeviceID=%s, PTZCmd=%s, RecordCmd=%s, GuardCmd=%s, LockCmd=%s, TeleBoot=%s, AlarmCmd=%s \r\n", strSN, strDeviceID, strPTZCmd, strRecordCmd, strGuardCmd, strLockCmd, strTeleBootCmd, strAlarmCmd);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设备控制命令处理:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 命令:PTZCmd=%s, RecordCmd=%s, GuardCmd=%s, LockCmd=%s, TeleBoot=%s, AlarmCmd=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, strPTZCmd, strRecordCmd, strGuardCmd, strLockCmd, strTeleBootCmd, strAlarmCmd);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS equipment control command processing, superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, command: PTZCmd = % s, RecordCmd = % s, GuardCmd = % s, LockCmd = % s, TeleBoot = % s, AlarmCmd = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, strPTZCmd, strRecordCmd, strGuardCmd, strLockCmd, strTeleBootCmd, strAlarmCmd);

    /* 重新启动 */
    if (strTeleBootCmd[0] != '\0')
    {
        if (0 == sstrcmp(strTeleBootCmd, (char*)"Boot"))
        {
            if (0 == sstrcmp(strDeviceID, local_cms_id_get()))
            {
                /* 发送去注册 */
                i = SIP_SendUnRegister(pRouteInfo->reg_info_index);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendUnRegister Error:reg_info_index=%d, iRet=%d \r\n", pRouteInfo->reg_info_index, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendUnRegister OK:reg_info_index=%d, iRet=%d \r\n", pRouteInfo->reg_info_index, i);
                }

                osip_usleep(5000000);

                BoardReboot();

                return i;
            }
        }
    }

    /* 点位锁定 */
    if (strLockCmd[0] != '\0')
    {
        i = RouteLockDeviceProc(strLockCmd, strDeviceID, pRouteInfo);
    }

    /* 手动录像 */
    if (strRecordCmd[0] != '\0')
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

        if (NULL != pGBLogicDeviceInfo)
        {
            pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type)
            {
                if (0 == sstrcmp(strRecordCmd, (char*)"Record")) /* 开始手动录像 */
                {
                    if (0 == pGBLogicDeviceInfo->record_type) /* 本地录像 */
                    {
                        /* 启动录像 */
                        i = add_record_info_by_message_cmd(pGBLogicDeviceInfo->id, pRoute_Srv_dboper);

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 手动启动录像失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"添加录像任务失败");
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "The equipment control command of superior CMS processing, manual start video failure: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, reason = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"添加录像任务失败");
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() add_record_info_by_message_cmd Error \r\n");
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设备控制命令处理, 手动启动录像成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The equipment control command of superior CMS processing, manual start video success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() add_record_info_by_message_cmd OK \r\n");
                        }

                        /* 组建XML信息 */
                        outPacket.SetRootTag("Response");
                        AccNode = outPacket.CreateElement((char*)"CmdType");
                        outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

                        AccNode = outPacket.CreateElement((char*)"SN");
                        outPacket.SetElementValue(AccNode, strSN);

                        AccNode = outPacket.CreateElement((char*)"DeviceID");
                        outPacket.SetElementValue(AccNode, (char*)strDeviceID);

                        AccNode = outPacket.CreateElement((char*)"Result");
                        outPacket.SetElementValue(AccNode, (char*)"OK");

                        /* 发送响应消息给上级CMS */
                        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                        if (i != 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }

                        return i;
                    }
                }
                else if (0 == sstrcmp(strRecordCmd, (char*)"StopRecord")) /* 停止手动录像 */
                {
                    if (0 == pGBLogicDeviceInfo->record_type) /* 本地录像 */
                    {
                        /* 停止录像 */
                        i = del_record_info_by_message_cmd(pGBLogicDeviceInfo->id, pRoute_Srv_dboper);

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 手动停止录像失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"删除录像任务失败");
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "The equipment control command of superior CMS processing, Manual stop video failure: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, reason = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"删除录像任务失败");
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() del_record_info_by_message_cmd Error \r\n");
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设备控制命令处理, 手动停止录像成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The equipment control command of superior CMS processing, Manual stop video success: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() del_record_info_by_message_cmd OK \r\n");
                        }

                        /* 组建XML信息 */
                        outPacket.SetRootTag("Response");
                        AccNode = outPacket.CreateElement((char*)"CmdType");
                        outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

                        AccNode = outPacket.CreateElement((char*)"SN");
                        outPacket.SetElementValue(AccNode, strSN);

                        AccNode = outPacket.CreateElement((char*)"DeviceID");
                        outPacket.SetElementValue(AccNode, (char*)strDeviceID);

                        AccNode = outPacket.CreateElement((char*)"Result");
                        outPacket.SetElementValue(AccNode, (char*)"OK");

                        /* 发送响应消息给上级CMS */
                        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                        if (i != 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }

                        return i;
                    }
                }
                else
                {
                    SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 手动录像失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=不支持的录像命令:RecordCmd=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strRecordCmd);
                    EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, Manual recording failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=do not support video command:RecordCmd=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strRecordCmd);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() Record Cmd Error: RecordCmd=%s \r\n", strRecordCmd);
                }
            }
        }
        else
        {
            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 手动录像失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=查找逻辑设备失败:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, Manual recording failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause= search for logic device failed:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() exit---: Find GB LogicDevice Info Error: DeviceID=%s \r\n", strDeviceID);
        }
    }

    /* 布防、撤防 */
    if (strGuardCmd[0] != '\0')
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

        if (NULL != pGBLogicDeviceInfo)
        {
            if (0 == sstrcmp(strGuardCmd, (char*)"SetGuard")) /* 布防*/
            {
                pGBLogicDeviceInfo->guard_type = 1;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_device_control_proc() GBLogicDevice=%s, GuardStatus=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->guard_type);
            }
            else if (0 == sstrcmp(strGuardCmd, (char*)"ResetGuard")) /*  撤防 */
            {
                pGBLogicDeviceInfo->guard_type = 0;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_device_control_proc() GBLogicDevice=%s, GuardStatus=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->guard_type);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_proc() Guard Cmd Error: GuardCmd=%s \r\n", strGuardCmd);
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_proc() exit---: Find GB LogicDevice Info Error: DeviceID=%s \r\n", strDeviceID);
        }

        /* 组建XML信息 */
        outPacket.SetRootTag("Response");
        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, (char*)strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        /* 发送响应消息给上级CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
    }

    /* 报警复位 */
    if (strAlarmCmd[0] != '\0')
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

        if (NULL != pGBLogicDeviceInfo)
        {
            if (0 == sstrcmp(strAlarmCmd, (char*)"ResetAlarm")) /* 复位告警 */
            {
                pGBLogicDeviceInfo->guard_type = 0;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_device_control_proc() GBLogicDevice=%s, GuardStatus=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->guard_type);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_proc() Alarm Cmd Error: AlarmCmd=%s \r\n", strAlarmCmd);
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_proc() exit---: Find GB LogicDevice Info Error: DeviceID=%s \r\n", strDeviceID);
        }

        /* 组建XML信息 */
        outPacket.SetRootTag("Response");
        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, (char*)strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        /* 发送响应消息给上级CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
    }

    /* PTZ云台控制命令 */
    if (strPTZCmd[0] != '\0')
    {
        /* 如果是控球命令，需要检查逻辑点位是否被锁定 */
        pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

        if (NULL != pGBLogicDeviceInfo)
        {
            if (LOCK_STATUS_USER_LOCK == pGBLogicDeviceInfo->lock_status)
            {
                if (NULL != pGBLogicDeviceInfo->pLockUserInfo)
                {
                    /* 上级控球不受本级用户锁定控制 */
                }
                else /* 原有用户锁定失效 */
                {
                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = NULL;
                }
            }
            else if (LOCK_STATUS_ROUTE_LOCK == pGBLogicDeviceInfo->lock_status)
            {
                if (NULL != pGBLogicDeviceInfo->pLockRouteInfo)
                {
                    if (0 == pGBLogicDeviceInfo->pLockRouteInfo->reg_status)
                    {
                        pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                        pGBLogicDeviceInfo->pLockUserInfo = NULL;
                        pGBLogicDeviceInfo->pLockRouteInfo = NULL;
                    }
                    else
                    {
                        if (pGBLogicDeviceInfo->pLockRouteInfo == pRouteInfo)
                        {
                            /* 更新锁定时间 */
                            i = device_auto_unlock_update(pGBLogicDeviceInfo->id);
                        }
                        else
                        {
                            /* 发送报警数据给客户端用户 */
                            outPacket.SetRootTag("Notify");

                            AccNode = outPacket.CreateElement((char*)"CmdType");
                            outPacket.SetElementValue(AccNode, (char*)"Alarm");

                            AccNode = outPacket.CreateElement((char*)"SN");
                            outPacket.SetElementValue(AccNode, strSN);

                            AccNode = outPacket.CreateElement((char*)"DeviceID");
                            outPacket.SetElementValue(AccNode, strDeviceID);

                            AccNode = outPacket.CreateElement((char*)"AlarmPriority");
                            outPacket.SetElementValue(AccNode, (char*)"4");

                            AccNode = outPacket.CreateElement((char*)"AlarmMethod");
                            outPacket.SetElementValue(AccNode, (char*)"5");

                            AccNode = outPacket.CreateElement((char*)"AlarmTime");
                            utc_time = time(NULL);
                            localtime_r(&utc_time, &local_time);
                            strftime(str_date, sizeof(str_date), "%Y-%m-%d", &local_time);
                            strftime(str_time, sizeof(str_time), "%H:%M:%S", &local_time);
                            snprintf(strTime, 32, "%sT%s", str_date, str_time);
                            outPacket.SetElementValue(AccNode, strTime);

                            AccNode = outPacket.CreateElement((char*)"AlarmDescription");
                            snprintf(strReason, 128, "逻辑设备已经被上级锁定:锁定上级CMD ID=%s, 锁定上级CMS IP地址=%s, 锁定上级CMS端口号=%d", pGBLogicDeviceInfo->pLockRouteInfo->server_id, pGBLogicDeviceInfo->pLockRouteInfo->server_ip, pGBLogicDeviceInfo->pLockRouteInfo->server_port);
                            outPacket.SetElementValue(AccNode, strReason);

                            /* 发送响应消息给上级CMS */
                            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                            if (i != 0)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }

                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=逻辑设备已经被上级锁定:锁定上级CMD ID=%s, 锁定上级CMS IP地址=%s, 锁定上级CMS端口号=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBLogicDeviceInfo->pLockRouteInfo->server_id, pGBLogicDeviceInfo->pLockRouteInfo->server_ip, pGBLogicDeviceInfo->pLockRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:CMS ID=%s, IP=%s, Port=%d, Logic Device ID=%s,  reason=Logical device has been locked", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() Record Cmd Error: GBLogicDevice Locked: ID=%s \r\n", strDeviceID);
                            return -1;
                        }
                    }
                }
                else /* 原有上级平台锁定失效 */
                {
                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = NULL;
                }
            }
        }
    }

    /* 转发消息到前端 */
    pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

    if (NULL != pGBLogicDeviceInfo)
    {
        /* 其他域的点位 */
        if (1 == pGBLogicDeviceInfo->other_realm)
        {
            /* 查找上级路由信息 */
            iCalleeRoutePos = route_info_find(pGBLogicDeviceInfo->cms_id);

            if (iCalleeRoutePos < 0)
            {
                SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的上级路由信息:cms_id=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBLogicDeviceInfo->cms_id);
                EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause= corresponding route info not found:cms_id=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBLogicDeviceInfo->cms_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() exit---: Get LogicDevice's Route Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }

            pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

            if (NULL == pCalleeRouteInfo)
            {
                SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的上级路由信息:cms_id=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBLogicDeviceInfo->cms_id);
                EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause= corresponding route info not found:cms_id=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBLogicDeviceInfo->cms_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() exit---: Get LogicDevice's Route Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }

            if (strPTZCmd[0] != '\0') /* 可能是设置预置位操作的，前端有返回 */
            {
                iLen = String2Bytes((unsigned char*)strPTZCmd, szPtzCmd, PTZCMD_28181_LEN);

                if (szPtzCmd[3] == 0x81 || szPtzCmd[3] == 0x83)
                {
                    /* 获取老的SN节点 */
                    AccSnNode = inPacket.SearchElement((char*)"SN");

                    if (NULL != AccSnNode)
                    {
                        g_transfer_xml_sn++;
                        snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                        inPacket.SetElementValue(AccSnNode, strTransferSN);
                    }

                    i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pCalleeRouteInfo->strRegLocalIP, pCalleeRouteInfo->iRegLocalPort, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到上级CMS失败", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to superior CMS failed", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设备控制命令处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);

                        old_xml_sn = strtoul(strSN, NULL, 10);
                        transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                        i = transfer_xml_msg_add(XML_CONTROL_DEVICECONTROL, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_device_control_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_CONTROL_DEVICECONTROL, old_xml_sn, transfer_xml_sn, strDeviceID, i);
                    }
                }
                else
                {
                    i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pCalleeRouteInfo->strRegLocalIP, pCalleeRouteInfo->iRegLocalPort, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到上级CMS失败", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to superior CMS failed", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设备控制命令处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                    }
                }
            }
            else
            {
                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pCalleeRouteInfo->strRegLocalIP, pCalleeRouteInfo->iRegLocalPort, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到上级CMS失败", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to superior CMS failed", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设备控制命令处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
                }
            }
        }
        else
        {
            pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (NULL != pGBDeviceInfo)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() GBDeviceInfo device_type=%d\r\n", pGBDeviceInfo->device_type);

                if (strPTZCmd[0] != '\0')
                {
                    //如果是预置位命令，判断是否需要更新数据库,下级cms情况下不需要判断
                    if (EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type)
                    {
                        i = preset_record(inPacket, pGBLogicDeviceInfo->id, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() Preset Record Error:device_id=%s \r\n", pGBDeviceInfo->device_id);

                            /* 组建XML信息 */
                            outPacket.SetRootTag("Response");
                            AccNode = outPacket.CreateElement((char*)"CmdType");
                            outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

                            AccNode = outPacket.CreateElement((char*)"SN");
                            outPacket.SetElementValue(AccNode, strSN);

                            AccNode = outPacket.CreateElement((char*)"DeviceID");
                            outPacket.SetElementValue(AccNode, (char*)strDeviceID);

                            AccNode = outPacket.CreateElement((char*)"Result");
                            outPacket.SetElementValue(AccNode, (char*)"ERROR");

                            /* 发送响应消息给上级CMS */
                            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                            if (i != 0)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }

                            return -1;
                        }
                        else
                        {
                            /* 组建XML信息 */
                            outPacket.SetRootTag("Response");
                            AccNode = outPacket.CreateElement((char*)"CmdType");
                            outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

                            AccNode = outPacket.CreateElement((char*)"SN");
                            outPacket.SetElementValue(AccNode, strSN);

                            AccNode = outPacket.CreateElement((char*)"DeviceID");
                            outPacket.SetElementValue(AccNode, (char*)strDeviceID);

                            AccNode = outPacket.CreateElement((char*)"Result");
                            outPacket.SetElementValue(AccNode, (char*)"OK");

                            /* 发送响应消息给上级CMS */
                            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                            if (i != 0)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }
                        }
                    }
                    else if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
                             && 1 == pGBDeviceInfo->three_party_flag) /* 第三方平台的下级点位 */
                    {
                        i = preset_record(inPacket, pGBLogicDeviceInfo->id, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() Preset Record Error:device_id=%s \r\n", pGBDeviceInfo->device_id);

                            /* 组建XML信息 */
                            outPacket.SetRootTag("Response");
                            AccNode = outPacket.CreateElement((char*)"CmdType");
                            outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

                            AccNode = outPacket.CreateElement((char*)"SN");
                            outPacket.SetElementValue(AccNode, strSN);

                            AccNode = outPacket.CreateElement((char*)"DeviceID");
                            outPacket.SetElementValue(AccNode, (char*)strDeviceID);

                            AccNode = outPacket.CreateElement((char*)"Result");
                            outPacket.SetElementValue(AccNode, (char*)"ERROR");

                            /* 发送响应消息给上级CMS */
                            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                            if (i != 0)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }

                            return -1;
                        }
                        else
                        {
                            /* 组建XML信息 */
                            outPacket.SetRootTag("Response");
                            AccNode = outPacket.CreateElement((char*)"CmdType");
                            outPacket.SetElementValue(AccNode, (char*)"DeviceControl");

                            AccNode = outPacket.CreateElement((char*)"SN");
                            outPacket.SetElementValue(AccNode, strSN);

                            AccNode = outPacket.CreateElement((char*)"DeviceID");
                            outPacket.SetElementValue(AccNode, (char*)strDeviceID);

                            AccNode = outPacket.CreateElement((char*)"Result");
                            outPacket.SetElementValue(AccNode, (char*)"OK");

                            /* 发送响应消息给上级CMS */
                            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

                            if (i != 0)
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            }
                        }
                    }
                }

                if (strLockCmd[0] != '\0') /* 如果是锁定命令，只发给下级平台 */
                {
                    if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type)
                    {
                        i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设备控制命令处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }
                }
                else if (strRecordCmd[0] != '\0') /* 如果是手动录像命令，前端有返回 */
                {
                    /* 获取老的SN节点 */
                    AccSnNode = inPacket.SearchElement((char*)"SN");

                    if (NULL != AccSnNode)
                    {
                        g_transfer_xml_sn++;
                        snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                        inPacket.SetElementValue(AccSnNode, strTransferSN);
                    }

                    i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设备控制命令处理成功,转发消息到前端物理设备:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS success, forwarding messages to the front end physical device: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forward front physical device ID = % s, IP address = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                        old_xml_sn = strtoul(strSN, NULL, 10);
                        transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                        i = transfer_xml_msg_add(XML_CONTROL_DEVICECONTROL, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_device_control_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_CONTROL_DEVICECONTROL, old_xml_sn, transfer_xml_sn, strDeviceID, i);
                    }
                }
                else if (strPTZCmd[0] != '\0') /* 可能是设置预置位操作的，前端有返回 */
                {
                    if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
                        && 0 == pGBDeviceInfo->three_party_flag) /* 下级点位 */
                    {
                        iLen = String2Bytes((unsigned char*)strPTZCmd, szPtzCmd, PTZCMD_28181_LEN);

                        if (szPtzCmd[3] == 0x81 || szPtzCmd[3] == 0x83)
                        {
                            /* 获取老的SN节点 */
                            AccSnNode = inPacket.SearchElement((char*)"SN");

                            if (NULL != AccSnNode)
                            {
                                g_transfer_xml_sn++;
                                snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                                inPacket.SetElementValue(AccSnNode, strTransferSN);
                            }

                            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                            if (i != 0)
                            {
                                SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设备控制命令处理成功,转发消息到前端物理设备:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS success, forwarding messages to the front end physical device: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forward front physical device ID = % s, IP address = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                                old_xml_sn = strtoul(strSN, NULL, 10);
                                transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                                i = transfer_xml_msg_add(XML_CONTROL_DEVICECONTROL, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_device_control_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_CONTROL_DEVICECONTROL, old_xml_sn, transfer_xml_sn, strDeviceID, i);
                            }
                        }
                        else
                        {
                            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                            if (i != 0)
                            {
                                SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设备控制命令处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS success:the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front-end, physical device ID = % s = % s IP address, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            }
                        }
                    }
                    else
                    {
                        i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设备控制命令处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS success:the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front-end, physical device ID = % s = % s IP address, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }
                }
                else
                {
                    i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设备控制命令处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS success:the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front-end, physical device ID = % s = % s IP address, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_control_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }
                }
            }
            else
            {
                SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
                EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause= corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
    }
    else
    {
        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
        return -1;
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_device_config_proc
 功能描述  : 上级互联CMS发送过来的设备配置命令处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月15日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_device_config_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_config_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_config_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_config_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的前端设备配置命令处理:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control configuration process from superior CMS success:the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s,", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* 网络设备配置

          设备配置命令直接转发给前段设备，不做处理
          命令包括如下字段:
          <!-- 命令类型：设备信息查询（必选） -->
          <element name="CmdType" fixed ="DeviceConfig" />
          <!-- 命令序列号（必选） -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- 目标设备的设备编码（必选） -->
          <element name="DeviceID" type="tg:deviceIDType" />
      */

    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_device_config_proc() \
        \r\n XML Para: \
        \r\n SN=%s, DeviceID=%s \r\n", strSN, strDeviceID);

    /* 查看被叫是否是本CMS ID */
    if (0 == strncmp(callee_id, local_cms_id_get(), 20))
    {
        // TODO:设置本级CMS的级联参数

        /* 回复响应,组建消息 */
        outPacket.SetRootTag("Response");
        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"DeviceConfig");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        /* 发送响应消息 给上级CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备配置失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"发送应答消息失败");
            EnSystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Device configuration from superior CMS :Requester ID=%s, IP address=%s, port number=%d,cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Failed to send reply message");
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_status_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设备配置成功:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device configuration from superior CMS failure:Requester ID=%s, IP address=%s, port number=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_status_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
    }
    else
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

        if (NULL != pGBLogicDeviceInfo)
        {
            pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (NULL != pGBDeviceInfo)
            {
                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的前端设备配置命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Front-end device configuration command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_config_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的前端设备配置命令处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The front of the equipment configuration command from superior CMS processing success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_config_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的前端设备配置命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Front-end device configuration command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_config_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
        else
        {
            pGBDeviceInfo = GBDevice_info_find(callee_id);

            if (NULL != pGBDeviceInfo)
            {
                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的前端设备配置命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Front-end device configuration command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical device ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_config_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的前端设备配置命令处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The front of the equipment configuration command from superior CMS processing success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_device_config_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的前端设备配置命令处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_SET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Front-end device configuration command process from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_config_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_query_device_video_param_proc
 功能描述  : 上级互联CMS发送过来的获取前端图像参数
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_query_device_video_param_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    char strSN[32] = {0};
    char strTransferSN[32] = {0};
    char strDeviceID[32] = {0};
    DOMElement* AccSnNode = NULL;
    unsigned int old_xml_sn = 0;
    unsigned int transfer_xml_sn = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_video_param_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_video_param_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_video_param_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取前端图像参数:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access front-end image parameter from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* 获取前端图像参数的命令直接转发给前段设备，不做处理

          命令包括如下字段:

      <?xml version="1.0"?>
      <Query>
      <CmdType>GetDeviceVideoParam</CmdType>
      <SN>43</SN>
      <DeviceID>64010000001110000001</DeviceID>
      </Query>
      */
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    if (strSN[0] == '\0')
    {
        SystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取前端图像参数失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=获取XML消息中的SN失败", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        EnSystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=access SN from XML message failed", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_video_param_proc() exit---: Get XML SN Error\r\n");
        return -1;
    }

    if (strDeviceID[0] == '\0')
    {
        SystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取前端图像参数失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=获取XML消息中的DeviceID失败", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        EnSystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=access device ID from XML message failed", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_video_param_proc() exit---: Get XML DeviceID Error\r\n");
        return -1;
    }

    pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo)
        {
            /* 获取老的SN节点 */
            AccSnNode = inPacket.SearchElement((char*)"SN");

            if (NULL != AccSnNode)
            {
                g_transfer_xml_sn++;
                snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                inPacket.SetElementValue(AccSnNode, strTransferSN);
            }

            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取前端图像参数失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_video_param_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取前端图像参数成功,转发消息到前端物理设备:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access front-end image parameter from superior CMS  success, forwarding messages to the front end physical device: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forwarding the front-end, physical device ID = % s = % s IP address, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_video_param_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                old_xml_sn = strtoul(strSN, NULL, 10);
                transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                i = transfer_xml_msg_add(XML_QUERY_DEVICEVIDEOPARAM, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_device_video_param_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_DEVICEVIDEOPARAM, old_xml_sn, transfer_xml_sn, strDeviceID, i);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取前端图像参数失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_video_param_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }
    else
    {
        pGBDeviceInfo = GBDevice_info_find(callee_id);

        if (NULL != pGBDeviceInfo)
        {
            /* 获取老的SN节点 */
            AccSnNode = inPacket.SearchElement((char*)"SN");

            if (NULL != AccSnNode)
            {
                g_transfer_xml_sn++;
                snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                inPacket.SetElementValue(AccSnNode, strTransferSN);
            }

            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取前端图像参数失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_video_param_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取前端图像参数成功,转发消息到前端物理设备:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access front-end image parameter from superior CMS success, forwarding messages to the front end physical device: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forwarding the front-end, physical device ID = % s = % s IP address, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_video_param_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                old_xml_sn = strtoul(strSN, NULL, 10);
                transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                i = transfer_xml_msg_add(XML_QUERY_DEVICEVIDEOPARAM, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_device_video_param_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_DEVICEVIDEOPARAM, old_xml_sn, transfer_xml_sn, strDeviceID, i);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取前端图像参数失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_GET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_video_param_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_set_device_video_param_proc
 功能描述  : 上级互联CMS发送过来的设置前端图像参数
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月28日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_set_device_video_param_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_set_device_video_param_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_set_device_video_param_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_set_device_video_param_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设置前端图像参数:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access front-end image parameter from superior CMS: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* 设置前端图像参数的命令直接转发给前段设备，不做处理

          命令包括如下字段:

      <?xml version="1.0"?>
      <Control>
      <CmdType>SetDeviceVideoParam</CmdType>
      <SN>43</SN>
      <DeviceID>64010000001110000001</DeviceID>
      <Brightness>12</ Brightness >        // 亮度
      <Saturation>22</ Saturation >         // 饱和度
      <Contrast>2</ Contrast>                 // 对比度
      <ColourDegree>2</ ColourDegree >  // 色度
      </Control>
      */

    pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设置前端图像参数失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_video_param_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设置前端图像参数成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Set front-end image parameter from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_set_device_video_param_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设置前端图像参数失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_video_param_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }
    else
    {
        pGBDeviceInfo = GBDevice_info_find(callee_id);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设置前端图像参数失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_video_param_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设置前端图像参数成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Set front-end image parameter from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_set_device_video_param_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设置前端图像参数失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_SET_VIDEO_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set front-end image parameter from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_video_param_proc() exit---: Find GB Device Info Error:DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_request_ifram_data_proc
 功能描述  : 上级互联CMS发送过来的请求I帧
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月28日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_request_ifram_data_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_request_ifram_data_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_request_ifram_data_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_request_ifram_data_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的请求I帧:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Request I frame from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* 请求I 帧的命令直接转发给前段设备，不做处理

          命令包括如下字段:

      <?xml version="1.0"?>
      < Control >
      <CmdType>RequestIFrameData</CmdType>
      <SN>43</SN>
      <DeviceID>64010000001110000001</DeviceID>
      </ Control >

      */

    pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo)
        {
            if (EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type
                || (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type && 0 == pGBDeviceInfo->three_party_flag))
            {
                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的请求I帧失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Request I frame from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_request_ifram_data_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的请求I帧成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Request I frame from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_request_ifram_data_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
            }
        }
        else
        {
            SystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的请求I帧失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Request I frame from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_request_ifram_data_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }
    else
    {
        pGBDeviceInfo = GBDevice_info_find(callee_id);

        if (NULL != pGBDeviceInfo)
        {
            if (EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type
                || (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type && 0 == pGBDeviceInfo->three_party_flag))
            {
                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的请求I帧失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Request I frame from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_request_ifram_data_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的请求I帧成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Request I frame from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_request_ifram_data_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
            }
        }
        else
        {
            SystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的请求I帧失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_REQUEST_IFRAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Request I frame from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_request_ifram_data_proc() exit---: Find GB Device Info Error:DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_control_autozoomin_proc
 功能描述  : 上级互联CMS发送过来的点击放大
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月28日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_control_autozoomin_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_control_autozoomin_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_control_autozoomin_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_control_autozoomin_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的点击放大消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Click to enlarge from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* 点击放大的命令直接转发给前段设备，不做处理

          命令包括如下字段:

      <?xml version="1.0"?>
      <Control>
      <CmdType> AutoZoomIn</CmdType>
      <SN>43</SN>
      <DeviceID>64010000001110000001</DeviceID>
      <DisplayFrameWide>12</ DisplayFrameWide>      //显示画面宽
      <DisplayFrameHigh>12</ DisplayFrameHigh >      //显示画面高
      <DestMatrixTopLeftX>1.2</ DestMatrixTopLeftX >  //目标矩形左上角位置：
      <DestMatrixTopLeftY>1.2</ DestMatrixTopLeftY >  //目标矩形左上角位置：
      <DestMatrixWide>22</ DestMatrixWide >             //目标矩形宽
      <DestMatrixHigh>13</ DestMatrixHigh >              //目标矩形高
      </ Control >

      */

    pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的点击放大消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "Click to enlarge from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_control_autozoomin_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的点击放大消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Click to enlarge from superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_control_autozoomin_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的点击放大消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "Click to enlarge from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_control_autozoomin_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }
    else
    {
        pGBDeviceInfo = GBDevice_info_find(callee_id);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的点击放大消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "Click to enlarge from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_control_autozoomin_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的点击放大消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Click to enlarge from superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_control_autozoomin_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的点击放大消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_CONTROL_AUTOZOMMIN_ERROR, EV9000_LOG_LEVEL_ERROR, "Click to enlarge from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_control_autozoomin_proc() exit---: Find GB Device Info Error:DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_set_device_xy_param_proc
 功能描述  : 上级互联CMS发送过来的设置点位的经纬度消息处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月31日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_set_device_xy_param_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strLongitude[64] = {0};
    char strLatitude[64] = {0};
    char strMapLayer[MAX_128CHAR_STRING_LEN + 4] = {0};
    double longitude = 0.0;
    double latitude = 0.0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_set_device_xy_param_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_set_device_xy_param_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_set_device_xy_param_proc() exit---: Route Srv dboper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_set_device_xy_param_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设置点位的经纬度消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Set point latitude and longitude from superior CMS :the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* 设置前端经纬度的命令直接设置到数据库

          命令包括如下字段:

      <?xml version="1.0"?>
          <Control>
          <CmdType>SetDeviceXYParam</CmdType>
          <SN>1234</SN>
          <DeviceID>逻辑设备ID</DeviceID>
          <Longitude>经度</Longitude>
          <Latitude>纬度</Latitude>
          </Control>
      */

    /* 取得 数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"Longitude", strLongitude);
    inPacket.GetElementValue((char*)"Latitude", strLatitude);
    inPacket.GetElementValue((char*)"MapLayer", strMapLayer);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_set_device_xy_param_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n DeviceID=%s \
    \r\n Longitude=%s \
    \r\n Latitude=%s \
    \r\n MapLayer=%s \r\n", strSN, strDeviceID, strLongitude, strLatitude, strMapLayer);

    pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

    if (NULL != pGBLogicDeviceInfo)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo)
        {
            if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type) /* 如果是下级的点位，并且是同级的时候 ，设置下级点位的经纬度信息 */
            {
                if (pGBDeviceInfo->link_type == 1)
                {
                    i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设置点位的经纬度消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 下级 CMS ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"点位属于同级CMS中下级CMS, 转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set point latitude and longitude from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, subordinate CMS ID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"point belongs to subordinate CMS among parallel CMS, forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_xy_param_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设置点位的经纬度消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 下级 CMS ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Set point latitude and longitude from superior CMS  success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, the lower the CMS, ID = % s IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_set_device_xy_param_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }

                    return 0;
                }
            }

            /* 经度 */
            if (strLongitude[0] != '\0')
            {
                longitude = strtod(strLongitude, (char**) NULL);
            }

            /* 纬度 */
            if (strLatitude[0] != '\0')
            {
                latitude = strtod(strLatitude, (char**) NULL);
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_set_device_xy_param_proc() Longitude=%.16lf, Latitude=%.16lf \r\n", longitude, latitude);

            if (pGBLogicDeviceInfo->longitude != longitude
                || pGBLogicDeviceInfo->latitude != latitude
                || 0 != sstrcmp(pGBLogicDeviceInfo->map_layer, strMapLayer)) /* 有变化就更新 */
            {
                pGBLogicDeviceInfo->longitude = longitude;
                pGBLogicDeviceInfo->latitude = latitude;

                if (0 != sstrcmp(pGBLogicDeviceInfo->map_layer, strMapLayer))
                {
                    memset(pGBLogicDeviceInfo->map_layer, 0, MAX_128CHAR_STRING_LEN + 4);
                    osip_strncpy(pGBLogicDeviceInfo->map_layer, strMapLayer, MAX_128CHAR_STRING_LEN);
                }

                /* 更新到数据库 */
                i = UpdateGBLogicDeviceXYParam2DB(pGBLogicDeviceInfo->device_id, longitude, latitude, strMapLayer, pRoute_Srv_dboper);

                if (i < 0)
                {
                    SystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设置点位的经纬度消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 经度=%.16lf, 纬度=%.16lf, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, longitude, latitude, (char*)"更新到数据库失败");
                    EnSystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set point latitude and longitude from superior CMS failed:requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, longitude=%.16lf, latitude=%.16lf, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, longitude, latitude, (char*)"update to database failed");
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_xy_param_proc() UpdateGBLogicDeviceXYParam2DB Error \r\n");
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设置点位的经纬度消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 经度=%.16lf, 纬度=%.16lf", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, longitude, latitude);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Set point latitude and longitude from superior CMS  success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, the lower the CMS, ID = % s IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_set_device_xy_param_proc() UpdateGBLogicDeviceXYParam2DB OK \r\n");
                }
            }
        }
        else
        {
            SystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设置点位的经纬度消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:GBLogicDeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
            EnSystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set point latitude and longitude from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:GBLogicDeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_xy_param_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", strDeviceID);
            return -1;
        }
    }
    else
    {
        SystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设置点位的经纬度消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的逻辑设备信息:GBLogicDeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
        EnSystemLog(EV9000_CMS_SET_XY_PARAM_ERROR, EV9000_LOG_LEVEL_ERROR, "Set point latitude and longitude from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding logic device info not found:GBLogicDeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_set_device_xy_param_proc() exit---: Find GBLogicDevice Info Error:GBLogicDeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_execute_preset_proc
 功能描述  : 上级CMS过来的执行预置位操作
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年1月28日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_execute_preset_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strPresetID[64] = {0};
    int iPresetID = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_execute_preset_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_execute_preset_proc() exit---: Param Error \r\n");
        return -1;
    }

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_execute_preset_proc() exit---: Route Srv dboper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_execute_preset_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的执行预置位消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The execution of the preset message of superior CMS: superior, CMS ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    /* 取得 数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"PresetID", strPresetID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_execute_preset_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n DeviceID=%s \
    \r\n PresetID=%s \r\n", strSN, strDeviceID, strPresetID);

    iPresetID = osip_atoi(strPresetID);

    i = ExecuteDevicePresetByPresetID(iPresetID, pRoute_Srv_dboper);

    if (i != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的执行预置位消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, PresetID=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, iPresetID);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "The execution of the preset message of superior CMS processing failure: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, PresetID=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, iPresetID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_execute_preset_proc() ExecuteDevicePresetByPresetID Error:PresetID=%d\r\n", iPresetID);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的执行预置位消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, PresetID=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, iPresetID);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "The execution of the preset message of superior CMS processing success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, PresetID = %", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, iPresetID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_execute_preset_proc() ExecuteDevicePresetByPresetID OK:PresetID=%d\r\n", iPresetID);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_query_device_preset_proc
 功能描述  : 上级互联CMS发送过来的获取前端预置位
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月28日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_query_device_preset_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_preset_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_preset_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_preset_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取前端预置位消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access front-end preset from superior CMS::Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* 获取前端预置位的命令直接转发给前段设备，不做处理

          命令包括如下字段:

      <Query>
      <CmdType>GetDevicePreset</CmdType>
      <SN>43</SN>
      <DeviceID>64010000001110000001</DeviceID>
      </Query>

      */

    pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取前端预置位消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end preset from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_preset_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取前端预置位消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access front-end preset from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_preset_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取前端预置位消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end preset from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_preset_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }
    else
    {
        pGBDeviceInfo = GBDevice_info_find(callee_id);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取前端预置位消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end preset from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_preset_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取前端预置位消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access front-end preset from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_preset_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取前端预置位失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Access front-end preset from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_preset_proc() exit---: Find GB Device Info Error:DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_get_db_ip_proc
 功能描述  : 上级互联CMS发送过来的获取本级CMS数据库IP地址
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月10日 星期一
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_get_db_ip_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int iRet = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strEthName1[MAX_IP_LEN] = {0};
    char strEthName2[MAX_IP_LEN] = {0};
    char* local_ip = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_db_ip_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_db_ip_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_get_db_ip_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取数据库IP地址消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access database IP address from superior CMS :Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_get_db_ip_proc() \
    \r\n XML Para: \
    \r\n SN=%s, DeviceID=%s \r\n", strSN, strDeviceID);

    /* 回复响应,组建消息 */
    CPacket outPacket;
    DOMElement* AccNode = NULL;

    outPacket.SetRootTag("Response");

    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"GetDBIP");

    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, strSN);

    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, strDeviceID);

    AccNode = outPacket.CreateElement((char*)"DBIP");

    if (0 == sstrcmp(pGblconf->db_server_ip, (char*)"127.0.0.1"))
    {
        local_ip = local_ip_get(pRouteInfo->strRegLocalEthName);

        if (NULL != local_ip)
        {
            outPacket.SetElementValue(AccNode, local_ip);
        }
        else
        {
            outPacket.SetElementValue(AccNode, (char*)"");
        }
    }
    else if (pGblconf->db_server_ip[0] == '\0')
    {
        local_ip = local_ip_get(pRouteInfo->strRegLocalEthName);

        if (NULL != local_ip)
        {
            outPacket.SetElementValue(AccNode, local_ip);
        }
        else
        {
            outPacket.SetElementValue(AccNode, (char*)"");
        }
    }
    else
    {
        outPacket.SetElementValue(AccNode, pGblconf->db_server_ip);
    }

    iRet = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

    if (iRet != 0)
    {
        SystemLog(EV9000_CMS_GET_DBIP_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取数据库IP地址消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=发送应答消息失败", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_GET_DBIP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access database IP address from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=send reply message failed", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_get_db_ip_proc() SIP_SendMessage Error:callee_id=%s, caller_ip=%s, caller_port=%d \r\n", callee_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取数据库IP地址消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access database IP address from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_get_db_ip_proc() SIP_SendMessage OK:callee_id=%s, caller_ip=%s, caller_port=%d \r\n", callee_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : route_notify_tv_status_proc
 功能描述  : 上级互联CMS发送过来的通知电视墙状态
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月28日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_notify_tv_status_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_tv_status_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_tv_status_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_tv_status_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的通知电视墙状态消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Notify TV wall status from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* 获取前端图像参数的命令直接转发给前段设备，不做处理

          命令包括如下字段:

      <?xml version="1.0"?>
      <Notify>
      <CmdType>TVStatus</ CmdType>
      <SN>1234</SN>
      <DecoderChannelID>解码器逻辑通道编码</DecoderChannelID>
      <Status></Status>        //  max  normal
      </Notify>

      */

    pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_NOTIFY_DEC_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的通知电视墙状态消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_NOTIFY_DEC_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Notify TV wall status from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_tv_status_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的通知电视墙状态消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Notify TV wall status from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front end physical device ID = % s, IP = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_tv_status_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else
        {
            SystemLog(EV9000_CMS_NOTIFY_DEC_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的通知电视墙状态消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            EnSystemLog(EV9000_CMS_NOTIFY_DEC_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Notify TV wall status from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_tv_status_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
            return -1;
        }
    }
    else
    {
        SystemLog(EV9000_CMS_NOTIFY_DEC_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的通知电视墙状态消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的逻辑设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
        EnSystemLog(EV9000_CMS_NOTIFY_DEC_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Notify TV wall status from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=orresponding logic device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_tv_status_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
        return -1;
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_query_catalog_proc
 功能描述  : 上级互联CMS发送过来的查询设备目录
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_query_catalog_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    char strTransferSN[32] = {0};
    DOMElement* AccSnNode = NULL;
    unsigned int old_xml_sn = 0;
    unsigned int transfer_xml_sn = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_catalog_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_catalog_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_catalog_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取逻辑设备信息消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* 网络设备信息查询
          控制流程见9.5.2
          设备目录查询的命令直接转发给前段设备，不做处理

          命令包括如下字段:
          <!-- 命令类型：设备目录查询（必选） -->
          <element name="CmdType" fixed ="Catalog" />
          <!-- 命令序列号（必选） -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- 目标设备的设备编码（必选） -->
          <element name="DeviceID" type="tg:deviceIDType" />
      */
    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_proc() \
            \r\n XML Para: \
            \r\n SN=%s, DeviceID=%s\r\n", strSN, strDeviceID);

    /* 查看被叫是否是本CMS ID */
    if (0 == strncmp(callee_id, local_cms_id_get(), 20))
    {
        if (0 == sstrcmp(callee_id, strDeviceID)) /* 如果查询的设备ID为本CMS ID，表示获取本设备列表*/
        {
            i = RouteGetGBDeviceListAndSendCataLogToCMS(pRouteInfo, caller_id, strDeviceID, strSN, pRoute_Srv_dboper);
        }
        else /* 获取具体的某一个设备信息 */
        {
            pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

            if (NULL != pGBLogicDeviceInfo)
            {
                pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

                if (NULL != pGBDeviceInfo)
                {
                    AccSnNode = inPacket.SearchElement((char*)"SN");

                    if (NULL != AccSnNode)
                    {
                        g_transfer_xml_sn++;
                        snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                        inPacket.SetElementValue(AccSnNode, strTransferSN);
                        //inPacket.SetTextContent();
                    }

                    i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备信息消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取逻辑设备信息消息处理成功,转发消息到前端物理设备:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS success,Forwards messages to the front end physical device: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, front-end, physical device ID = % s = % s IP address, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_catalog_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                        old_xml_sn = strtoul(strSN, NULL, 10);
                        transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                        i = transfer_xml_msg_add(XML_QUERY_CATALOG, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_catalog_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_CATALOG, old_xml_sn, transfer_xml_sn, strDeviceID, i);

                    }
                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备信息消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, reason = didn't find the corresponding physical device information: DeviceID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_proc() exit---: Find GBDevice Info Error  \r\n");
                return -1;
            }
        }
    }
    else
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

        if (NULL != pGBLogicDeviceInfo)
        {
            pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (NULL != pGBDeviceInfo)
            {
                AccSnNode = inPacket.SearchElement((char*)"SN");

                if (NULL != AccSnNode)
                {
                    g_transfer_xml_sn++;
                    snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                    inPacket.SetElementValue(AccSnNode, strTransferSN);
                    //inPacket.SetTextContent();
                }

                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备信息消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取逻辑设备信息消息处理成功,转发消息到前端物理设备:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS success,Forwarding messages to the front end physical device: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forwarding the front-end, physical device ID = % s = % s IP address, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_catalog_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    old_xml_sn = strtoul(strSN, NULL, 10);
                    transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                    i = transfer_xml_msg_add(XML_QUERY_CATALOG, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_catalog_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_CATALOG, old_xml_sn, transfer_xml_sn, strDeviceID, i);

                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备信息消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failure:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, reason = didn't find the corresponding physical device information: DeviceID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
        else
        {
            pGBDeviceInfo = GBDevice_info_find(callee_id);

            if (NULL != pGBDeviceInfo)
            {
                AccSnNode = inPacket.SearchElement((char*)"SN");

                if (NULL != AccSnNode)
                {
                    g_transfer_xml_sn++;
                    snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                    inPacket.SetElementValue(AccSnNode, strTransferSN);
                    //inPacket.SetTextContent();
                }

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备信息消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取逻辑设备信息消息处理成功,转发消息到前端物理设备:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS success, forwarding the message to the front end physical device:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forward front physical device ID = % s, IP address = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_catalog_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    old_xml_sn = strtoul(strSN, NULL, 10);
                    transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                    i = transfer_xml_msg_add(XML_QUERY_CATALOG, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_catalog_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_CATALOG, old_xml_sn, transfer_xml_sn, strDeviceID, i);

                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备信息消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, reason = didn't find the corresponding physical device information: DeviceID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取逻辑设备信息消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS  success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_catalog_proc() Exit---\r\n");

    return i;
}

/*****************************************************************************
 函 数 名  : route_query_device_info_proc
 功能描述  : 上级互联CMS发送过来的查询设备信息处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_query_device_info_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    char strTransferSN[32] = {0};
    DOMElement* AccSnNode = NULL;
    unsigned int old_xml_sn = 0;
    unsigned int transfer_xml_sn = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_info_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_info_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_info_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询设备信息消息处理:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS:the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* 网络设备信息查询
          控制流程见9.5.2
          设备信息查询的命令直接转发给前段设备，不做处理

          命令包括如下字段:
          <!-- 命令类型：设备信息查询（必选） -->
          <element name="CmdType" fixed ="DeviceInfo" />
          <!-- 命令序列号（必选） -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- 目标设备的设备编码（必选） -->
          <element name="DeviceID" type="tg:deviceIDType" />
      */
    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_device_info_proc() \
            \r\n XML Para: \
            \r\n SN=%s, DeviceID=%s \r\n", strSN, strDeviceID);

    /* 查看被叫是否是本CMS ID */
    if (0 == strncmp(callee_id, local_cms_id_get(), 20))
    {
        i = SendDeviceInfoMessageToRouteCMS(caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, strSN, strDeviceID, pRouteInfo->three_party_flag, pRouteInfo->trans_protocol);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备信息消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"发送应答消息失败");
            EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"send reply message failed");
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_info_proc() SendDeviceInfoMessageToRouteCMS Error\r\n");
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询设备信息消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_info_proc() SendDeviceInfoMessageToRouteCMS OK\r\n");
        }
    }
    else
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

        if (NULL != pGBLogicDeviceInfo)
        {
            pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (NULL != pGBDeviceInfo)
            {
                AccSnNode = inPacket.SearchElement((char*)"SN");

                if (NULL != AccSnNode)
                {
                    g_transfer_xml_sn++;
                    snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                    inPacket.SetElementValue(AccSnNode, strTransferSN);
                    //inPacket.SetTextContent();
                }

                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备信息消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_info_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询设备信息消息处理成功,转发消息到前端物理设备:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS success, forwarding the message to the front end physical device:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_info_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    old_xml_sn = strtoul(strSN, NULL, 10);
                    transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                    i = transfer_xml_msg_add(XML_QUERY_DEVICEINFO, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_device_info_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_DEVICEINFO, old_xml_sn, transfer_xml_sn, strDeviceID, i);

                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备信息消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_info_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
        else
        {
            pGBDeviceInfo = GBDevice_info_find(callee_id);

            if (NULL != pGBDeviceInfo)
            {
                AccSnNode = inPacket.SearchElement((char*)"SN");

                if (NULL != AccSnNode)
                {
                    g_transfer_xml_sn++;
                    snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                    inPacket.SetElementValue(AccSnNode, strTransferSN);
                    //inPacket.SetTextContent();
                }

                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备信息消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_info_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询设备信息消息处理成功,转发消息到前端物理设备:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device info from superior CMS success, forwarding the message to the front end physical device:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_info_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    old_xml_sn = strtoul(strSN, NULL, 10);
                    transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                    i = transfer_xml_msg_add(XML_QUERY_DEVICEINFO, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_device_info_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_DEVICEINFO, old_xml_sn, transfer_xml_sn, strDeviceID, i);

                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备信息消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_info_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_query_device_status_proc
 功能描述  : 上级互联CMS发送过来的查询设备状态处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_query_device_status_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    string strStatus = "";

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_status_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_status_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_status_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询设备状态消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device status from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);


    /* 网络设备信息查询
          控制流程见9.5.2
          设备状态查询的命令直接转发给前段设备，不做处理

          命令包括如下字段:
          <!-- 命令类型：设备状态查询（必选） -->
          <element name="CmdType" fixed ="DeviceStatus" />
          <!-- 命令序列号（必选） -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- 目标设备的设备编码（必选） -->
          <element name="DeviceID" type="tg:deviceIDType" />
      */

    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_device_status_proc() \
        \r\n XML Para: \
        \r\n SN=%s, DeviceID=%s \r\n", strSN, strDeviceID);

    /* 查看被叫是否是本CMS ID */
    if (0 == strncmp(callee_id, local_cms_id_get(), 20))
    {
        /* 回复响应,组建消息 */
        outPacket.SetRootTag("Response");
        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"DeviceStatus");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        AccNode = outPacket.CreateElement((char*)"Online");
        outPacket.SetElementValue(AccNode, (char*)"ONLINE");

        AccNode = outPacket.CreateElement((char*)"Status");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        /* 发送响应消息 给上级CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备状态消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"发送应答消息失败");
            EnSystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device status from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"send reply message failed");
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_status_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询设备状态消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device status from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_status_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
    }
    else
    {
        strStatus = "OFFLINE";

        pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);  /* 查找逻辑设备队列 */

        if (NULL != pGBLogicDeviceInfo)
        {
            if (pRouteInfo->three_party_flag) /* 第三方平台 */
            {
                if (1 == pGBLogicDeviceInfo->status)
                {
                    strStatus = "ON";
                }
                else if (2 == pGBLogicDeviceInfo->status)
                {
                    strStatus = "VLOST";
                }
            }
            else
            {
                if (1 == pGBLogicDeviceInfo->status)
                {
                    if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                    {
                        strStatus = "INTELLIGENT";
                    }
                    else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                    {
                        strStatus = "CLOSE";
                    }
                    else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                    {
                        strStatus = "APART";
                    }
                    else
                    {
                        strStatus = "ONLINE";
                    }
                }
                else if (2 == pGBLogicDeviceInfo->status)
                {
                    strStatus = "NOVIDEO";
                }
            }

            /* 回复响应,组建消息 */
            outPacket.SetRootTag("Response");
            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"DeviceStatus");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Result");
            outPacket.SetElementValue(AccNode, (char*)"OK");

            AccNode = outPacket.CreateElement((char*)"Online");
            outPacket.SetElementValue(AccNode, (char*)strStatus.c_str());

            AccNode = outPacket.CreateElement((char*)"Status");
            outPacket.SetElementValue(AccNode, (char*)"OK");

            /* 发送响应消息 给上级CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备状态消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"发送应答消息失败");
                EnSystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device status from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d,cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"send reply message failed");
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_status_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询设备状态消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device status from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_status_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
        else
        {
            pGBDeviceInfo = GBDevice_info_find(callee_id);

            if (NULL != pGBDeviceInfo)
            {
                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备状态消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device status from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_status_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询设备状态消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 前端物理设备ID=%s, IP=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device status from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_status_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备状态消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device status from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_status_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_query_device_config_proc
 功能描述  : 上级互联CMS发送过来的查询设备配置处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月15日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_query_device_config_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strConfigType[32] = {0};
    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    DOMElement* ListAccNode = NULL;
    DOMElement* ItemAccNode = NULL;
    DOMElement* BasicParamNode = NULL;
    char strServerPort[32] = {0};
    char strExpire[32] = {0};
    char strHeartBeatInterval[32] = {0};

    char strTransferSN[32] = {0};
    DOMElement* AccSnNode = NULL;
    unsigned int old_xml_sn = 0;
    unsigned int transfer_xml_sn = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_config_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_config_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_config_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询设备配置消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device config from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* 网络设备配置查询

          设备配置查询的命令直接转发给前段设备，不做处理
          命令包括如下字段:
          <!-- 命令类型：设备信息查询（必选） -->
          <element name="CmdType" fixed ="ConfigDownload" />
          <!-- 命令序列号（必选） -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- 目标设备的设备编码（必选） -->
          <element name="DeviceID" type="tg:deviceIDType" />
      */

    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"ConfigType", strConfigType);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_device_config_proc() \
        \r\n XML Para: \
        \r\n SN=%s, DeviceID=%s, ConfigType=%s \r\n", strSN, strDeviceID, strConfigType);

    if (strSN[0] == '\0')
    {
        SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备配置消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=获取XML消息中的SN失败", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=access SN from XML failed", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_config_proc() exit---: Get XML SN Error\r\n");
        return -1;
    }

    if (strDeviceID[0] == '\0')
    {
        SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备配置消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=获取XML消息中的DeviceID失败", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=access device ID SN from XML failed", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_config_proc() exit---: Get XML DeviceID Error\r\n");
        return -1;
    }

    /* 查看被叫是否是本CMS ID */
    if (0 == strncmp(callee_id, local_cms_id_get(), 20))
    {
        /* 回复响应,组建消息 */
        if (0 == sstrcmp(strConfigType, (char*)"VideoParamConfig"))
        {
            /* 回复响应,组建消息 */
            outPacket.SetRootTag("Response");
            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"ConfigDownload");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Result");
            outPacket.SetElementValue(AccNode, (char*)"OK");

            ListAccNode = outPacket.CreateElement((char*)"VideoParamConfig");
            outPacket.SetElementAttr(ListAccNode, (char*)"Num", (char*)"1");

            outPacket.SetCurrentElement(ListAccNode);
            ItemAccNode = outPacket.CreateElement((char*)"Item");
            outPacket.SetCurrentElement(ItemAccNode);

            AccNode = outPacket.CreateElement((char*)"StreamName");
            outPacket.SetElementValue(AccNode, (char*)"Stream1");

            AccNode = outPacket.CreateElement((char*)"VideoFormat");
            outPacket.SetElementValue(AccNode, (char*)"2");

            AccNode = outPacket.CreateElement((char*)"Resolution");
            outPacket.SetElementValue(AccNode, (char*)"0");

            AccNode = outPacket.CreateElement((char*)"FrameRate");
            outPacket.SetElementValue(AccNode, (char*)"25");

            AccNode = outPacket.CreateElement((char*)"BitRateType");
            outPacket.SetElementValue(AccNode, (char*)"1");

            AccNode = outPacket.CreateElement((char*)"VideoBitRate");
            outPacket.SetElementValue(AccNode, (char*)"2048");

            /* 发送响应消息 给上级CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备配置消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"发送应答消息失败");
                EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"send reply message failed");
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_status_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询设备配置消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device config from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_status_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
        else if (0 == sstrcmp(strConfigType, (char*)"BasicParam"))
        {
            outPacket.SetRootTag("Response");
            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"ConfigDownload");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Result");
            outPacket.SetElementValue(AccNode, (char*)"OK");

            BasicParamNode = outPacket.CreateElement((char*)"BasicParam");
            outPacket.SetCurrentElement(BasicParamNode);

            /* Name */
            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, local_cms_name_get());

            /* ID */
            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, local_cms_id_get());

            /* SIP Server ID */
            AccNode = outPacket.CreateElement((char*)"SIPServerID");
            outPacket.SetElementValue(AccNode, pRouteInfo->server_id);

            /* SIP Server IP */
            AccNode = outPacket.CreateElement((char*)"SIPServerIP");
            outPacket.SetElementValue(AccNode, pRouteInfo->server_ip);

            /* SIP Server Port */
            AccNode = outPacket.CreateElement((char*)"SIPServerPort");
            snprintf(strServerPort, 32, "%d", pRouteInfo->server_port);
            outPacket.SetElementValue(AccNode, strServerPort);

            /* DomainName */
            AccNode = outPacket.CreateElement((char*)"DomainName");
            outPacket.SetElementValue(AccNode, pRouteInfo->server_host);

            /* Expriration */
            AccNode = outPacket.CreateElement((char*)"Expriration");
            snprintf(strExpire, 32, "%d", pRouteInfo->min_expires);
            outPacket.SetElementValue(AccNode, strExpire);

            /* Password */
            AccNode = outPacket.CreateElement((char*)"Password");
            outPacket.SetElementValue(AccNode, pRouteInfo->register_password);

            /* HeartBeatInterval, 心跳间隔时间 */
            AccNode = outPacket.CreateElement((char*)"HeartBeatInterval");
            snprintf(strHeartBeatInterval, 32, "%d", local_keep_alive_interval_get());
            outPacket.SetElementValue(AccNode, strHeartBeatInterval);

            /* HeartBeatCount, 心跳超时次数 */
            AccNode = outPacket.CreateElement((char*)"HeartBeatCount");
            outPacket.SetElementValue(AccNode, (char*)"3");

            /* 发送响应消息 给上级CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备配置消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"发送应答消息失败");
                EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"send reply message failed");
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_status_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询设备配置消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device config from superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_status_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
    }
    else
    {
        pGBLogicDeviceInfo = GBLogicDevice_info_find(callee_id);

        if (NULL != pGBLogicDeviceInfo)
        {
            pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

            if (NULL != pGBDeviceInfo)
            {
                /* 获取老的SN节点 */
                AccSnNode = inPacket.SearchElement((char*)"SN");

                if (NULL != AccSnNode)
                {
                    g_transfer_xml_sn++;
                    snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                    inPacket.SetElementValue(AccSnNode, strTransferSN);
                }

                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备配置消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_config_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询设备配置消息处理成功,转发消息到前端物理设备:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device config from superior CMS success,Forwarding messages to the front end physical devices:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forward front physical device ID = % s, IP address = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_config_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    old_xml_sn = strtoul(strSN, NULL, 10);
                    transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                    i = transfer_xml_msg_add(XML_QUERY_DEVICECONFIG, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_device_config_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_DEVICECONFIG, old_xml_sn, transfer_xml_sn, strDeviceID, i);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备配置消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_config_proc() exit---: Get LogicDevice's GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
        else
        {
            pGBDeviceInfo = GBDevice_info_find(callee_id);

            if (NULL != pGBDeviceInfo)
            {
                /* 获取老的SN节点 */
                AccSnNode = inPacket.SearchElement((char*)"SN");

                if (NULL != AccSnNode)
                {
                    g_transfer_xml_sn++;
                    snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                    inPacket.SetElementValue(AccSnNode, strTransferSN);
                }

                i = SIP_SendMessage(NULL, local_cms_id_get(), callee_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备配置消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"转发消息到前端物理设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s, front-end physical deviceID=%s, IP=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, (char*)"forward message to frond-end physical device failed", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_config_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询设备配置消息处理成功,转发消息到前端物理设备:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 转发前端物理设备ID=%s, IP地址=%s, 端口=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for device config from superior CMS success,Forwarding messages to the front end physical devices:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, forward front physical device ID = % s, IP address = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_config_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    old_xml_sn = strtoul(strSN, NULL, 10);
                    transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                    i = transfer_xml_msg_add(XML_QUERY_DEVICECONFIG, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_device_config_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_DEVICECONFIG, old_xml_sn, transfer_xml_sn, strDeviceID, i);
                }
            }
            else
            {
                SystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询设备配置消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=没有找到对应的物理设备信息:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                EnSystemLog(EV9000_CMS_GET_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for device config from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=corresponding physical device info not found:DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id, callee_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_config_proc() exit---: Find GBDevice Info Error: DeviceID=%s \r\n", callee_id);
                return -1;
            }
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_query_record_info_proc
 功能描述  : 上级互联CMS发送过来的查询设备视频文件处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_query_record_info_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    /* 设备视音频文件检索
          控制流程见9.7.2
      */
    int i = 0;
    int iRet = 0;
    int index = 0;
    int tsu_index = -1;
    int record_count = 0;
    int query_count = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strRecordType[32] = {0};
    char strStartTime[32] = {0};
    char strEndTime[32] = {0};
    char strFilePath[32] = {0};
    char strAddress[32] = {0};
    char strSecrecy[32] = {0};
    char str3PartFlag[16] = {0};
    char strRecorderID[32] = {0};
    char strErrorCode[32] = {0};

    GBDevice_info_t* pGBDeviceInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    tsu_resource_info_t* pTsuResourceInfo = NULL;
    VideoRecordList stVideoRecordList;
    unsigned int iStartTime = 0;
    unsigned int iEndTime = 0;
    int record_type = 0;

    DOMElement* RecordListAccNode = NULL;
    //DOMElement* AccSnNode = NULL;
    char strTransferSN[32] = {0};
    unsigned int old_xml_sn = 0;
    unsigned int transfer_xml_sn = 0;

    int iCalleeRoutePos = 0;
    route_info_t* pCalleeRouteInfo = NULL;

    int record_info_pos = -1;
    record_info_t* pRecordInfo = NULL;

    int iRecordCRPos = -1;
    cr_t* pRecordCrData = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_record_info_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_record_info_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    /* 取得查询条件数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"StartTime", strStartTime);
    inPacket.GetElementValue((char*)"EndTime", strEndTime);
    inPacket.GetElementValue((char*)"FilePath", strFilePath);
    inPacket.GetElementValue((char*)"Address", strAddress);
    inPacket.GetElementValue((char*)"Secrecy", strSecrecy);
    inPacket.GetElementValue((char*)"Type", strRecordType);
    inPacket.GetElementValue((char*)"ThreeParty", str3PartFlag);
    inPacket.GetElementValue((char*)"RecorderID", strRecorderID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_record_info_proc() \
        \r\n XML Param: \
        \r\n SN=%s \
        \r\n DeviceID=%s \
        \r\n RecordType=%s \
        \r\n StartTime=%s \
        \r\n EndTime=%s \
        \r\n", strSN, strDeviceID, strRecordType, strStartTime, strEndTime);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询录像记录信息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for video record info from superior CMS :CMS ID=%s, IP address=%s, port number=%d, logic deviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);

    /* 1、获取录像点位信息 */
    pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

    if (NULL == pGBLogicDeviceInfo)
    {
        /* 回复响应,组建消息 */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Name");
        outPacket.SetElementValue(AccNode, (char*)" ");

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"-1");

        AccNode = outPacket.CreateElement((char*)"ErrCode");
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_FIND_LOGIC_DEVICE_INFO_ERROR);
        outPacket.SetElementValue(AccNode, strErrorCode);

        AccNode = outPacket.CreateElement((char*)"Reason");
        outPacket.SetElementValue(AccNode, (char*)"获取逻辑设备信息失败");

        RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
        outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

        /* 发送响应消息 给上级CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Get GBLogic Device Info Error \r\n");
        SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"获取逻辑设备信息失败");
        EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Access logical device information failed");
        return -1;
    }

    /* 看是否是其他域的点位 */
    if (1 == pGBLogicDeviceInfo->other_realm)
    {
        /* 查找上级路由信息 */
        iCalleeRoutePos = route_info_find(pGBLogicDeviceInfo->cms_id);

        if (iCalleeRoutePos < 0)
        {
            /* 回复响应,组建消息 */
            CPacket outPacket;
            DOMElement* AccNode = NULL;

            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, (char*)" ");

            AccNode = outPacket.CreateElement((char*)"SumNum");
            outPacket.SetElementValue(AccNode, (char*)"-1");

            AccNode = outPacket.CreateElement((char*)"ErrCode");
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_ROUTE_FIND_ROUTE_INFO_ERROR);
            outPacket.SetElementValue(AccNode, strErrorCode);

            AccNode = outPacket.CreateElement((char*)"Reason");
            outPacket.SetElementValue(AccNode, (char*)"获取上级路由信息失败");

            RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
            outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

            /* 发送响应消息 给上级CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Get GBLogic Device Info Error \r\n");
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"获取上级路由信息失败");
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Get Route information failed");
            return -1;
        }

        pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

        if (NULL == pCalleeRouteInfo)
        {
            /* 回复响应,组建消息 */
            CPacket outPacket;
            DOMElement* AccNode = NULL;

            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, (char*)" ");

            AccNode = outPacket.CreateElement((char*)"SumNum");
            outPacket.SetElementValue(AccNode, (char*)"-1");

            AccNode = outPacket.CreateElement((char*)"ErrCode");
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_ROUTE_GET_ROUTE_INFO_ERROR);
            outPacket.SetElementValue(AccNode, strErrorCode);

            AccNode = outPacket.CreateElement((char*)"Reason");
            outPacket.SetElementValue(AccNode, (char*)"获取上级路由信息失败");

            RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
            outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

            /* 发送响应消息 给上级CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Get GBLogic Device Info Error \r\n");
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"获取上级路由信息失败");
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Get route information failed");
            return -1;
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询录像记录信息是上级CMS中的点位:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 所在的上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS to query video information is the higher level in the CMS: superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s, the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);

        /* 从新组建消息 */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Query");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = outPacket.CreateElement((char*)"SN");
        g_transfer_xml_sn++;
        snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
        outPacket.SetElementValue(AccNode, strTransferSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"StartTime");
        outPacket.SetElementValue(AccNode, strStartTime);

        AccNode = outPacket.CreateElement((char*)"EndTime");
        outPacket.SetElementValue(AccNode, strEndTime);

        AccNode = outPacket.CreateElement((char*)"Type");

        if (strRecordType[0] != 0)
        {
            outPacket.SetElementValue(AccNode, strRecordType);
        }
        else
        {
            outPacket.SetElementValue(AccNode, (char*)"all");
        }

        /* 第三方平台查询录像的命令，如果转到上级去，上级平台是自己的话，需要加上第三方标识 */
        if (1 == pRouteInfo->three_party_flag && 0 == pCalleeRouteInfo->three_party_flag)
        {
            AccNode = outPacket.CreateElement((char*)"ThreeParty");
            outPacket.SetElementValue(AccNode, (char*)"YES");
        }

        AccNode = outPacket.CreateElement((char*)"RecorderID");
        outPacket.SetElementValue(AccNode, pCalleeRouteInfo->server_id);

        /* 将查询录像任务的消息转发出去 */
        i = SIP_SendMessage(NULL, local_cms_id_get(), pCalleeRouteInfo->server_id, pCalleeRouteInfo->strRegLocalIP, pCalleeRouteInfo->iRegLocalPort, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s, 转发上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"转发查询消息到上级CMS失败", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS , forward search info to superior CMS failed:superior CMS ID=%s, IP address=%s, port number=%d, superior CMS ID=%s, IPaddress =%s, port number=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询录像记录信息成功,转发查询消息到上级CMS:上级CMS ID=%s, IP地址=%s, 端口号=%d, 转发上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for video record info from superior success,Forwarding query message to the superior the CMS:Superior CMS, ID = % s = % s IP address, port number = % d, forwarding the superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:server_id=%s, server_ip=%s, server_port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);

            old_xml_sn = strtoul(strSN, NULL, 10);
            transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
            i = transfer_xml_msg_add(XML_QUERY_RECORDINFO, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_RECORDINFO, old_xml_sn, transfer_xml_sn, strDeviceID, i);
        }

        return i;
    }

    /* 确定录像类型 */
    if (strRecordType[0] != 0)
    {
        record_type = osip_atoi(strRecordType);
    }
    else
    {
        record_type = EV9000_RECORD_TYPE_NORMAL;
    }

    if (record_type <= 0)
    {
        record_type = EV9000_RECORD_TYPE_NORMAL;
    }

    /* 获取逻辑设备所属的物理设备 */
    if (EV9000_RECORD_TYPE_NORMAL == record_type
        || EV9000_RECORD_TYPE_BACKUP == record_type
        || EV9000_RECORD_TYPE_ALARM == record_type)
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);
    }
    else
    {
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_INTELLIGENCE);
    }

    if (NULL == pGBDeviceInfo)
    {
        /* 回复响应,组建消息 */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Name");
        outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"-1");

        AccNode = outPacket.CreateElement((char*)"ErrCode");
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_GET_DEVICE_INFO_ERROR);
        outPacket.SetElementValue(AccNode, strErrorCode);

        AccNode = outPacket.CreateElement((char*)"Reason");
        outPacket.SetElementValue(AccNode, (char*)"获取物理设备信息失败");

        RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
        outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

        /* 发送响应消息 给上级CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Get GBLogic Device Info Error \r\n");
        SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"获取物理设备信息失败");
        EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Access physical device info failed");
        return -1;
    }

    /* 查看该点位本地是否配置了录像，如果配置了，则在本地查找录像记录 */
    if (EV9000_RECORD_TYPE_NORMAL == record_type
        || EV9000_RECORD_TYPE_ALARM == record_type)
    {
        record_info_pos = record_info_find_by_stream_type(pGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_MASTER);
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_MASTER, record_info_pos);
    }
    else if (EV9000_RECORD_TYPE_BACKUP == record_type)
    {
        record_info_pos = record_info_find_by_stream_type(pGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_SLAVE);
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_SLAVE, record_info_pos);
    }
    else if (EV9000_RECORD_TYPE_INTELLIGENCE == record_type)
    {
        record_info_pos = record_info_find_by_stream_type(pGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_INTELLIGENCE);
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_INTELLIGENCE, record_info_pos);
    }
    else
    {
        record_info_pos = record_info_find_by_stream_type(pGBLogicDeviceInfo->id, EV9000_STREAM_TYPE_MASTER);
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() record_info_find_by_stream_type:device_id=%s, stream_type=%d, record_info_pos=%d \r\n", pGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_MASTER, record_info_pos);
    }

    if (record_info_pos >= 0)
    {
        pRecordInfo = record_info_get(record_info_pos);

        if (NULL != pRecordInfo)
        {
            if (1 == pRecordInfo->record_enable)
            {
                goto normal_record;
            }
        }
    }

    if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER) /* 下级cms 的录像  */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询录像记录信息是下级CMS中的点位:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 所在的下级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS of query video information is lower in the CMS point: the superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s, the lower the CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

        /* 从新组建消息 */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Query");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = outPacket.CreateElement((char*)"SN");
        g_transfer_xml_sn++;
        snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
        outPacket.SetElementValue(AccNode, strTransferSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"StartTime");
        outPacket.SetElementValue(AccNode, strStartTime);

        AccNode = outPacket.CreateElement((char*)"EndTime");
        outPacket.SetElementValue(AccNode, strEndTime);

        AccNode = outPacket.CreateElement((char*)"Type");

        if (1 == pGBDeviceInfo->three_party_flag)
        {
            outPacket.SetElementValue(AccNode, (char*)"all");
        }
        else
        {
            if (strRecordType[0] != 0)
            {
                outPacket.SetElementValue(AccNode, strRecordType);
            }
            else
            {
                outPacket.SetElementValue(AccNode, (char*)"all");
            }
        }

        /* 第三方平台查询录像的命令，如果转到下级去，下级是自己的平台的话，需要加上第三方标识 */
        if (1 == pRouteInfo->three_party_flag && 0 == pGBDeviceInfo->three_party_flag)
        {
            AccNode = outPacket.CreateElement((char*)"ThreeParty");
            outPacket.SetElementValue(AccNode, (char*)"YES");
        }

        AccNode = outPacket.CreateElement((char*)"RecorderID");
        outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

        /* 将查询录像任务的消息转发出去 */
        i = SIP_SendMessage(NULL, local_cms_id_get(), strDeviceID, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s, 转发下级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"转发查询消息到下级CMS失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS , forward search info to subordinate CMS failed:superior CMS ID=%s, IP address=%s, port number=%d, subordinate CMS ID=%s, IPaddress =%s, port number=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询录像记录信息成功,转发查询消息到下级CMS:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 转发下级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for video record info from superior CMS , forward search info to subordinate CMS sucessfully:Superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s, forwarding the lower CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

            old_xml_sn = strtoul(strSN, NULL, 10);
            transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
            i = transfer_xml_msg_add(XML_QUERY_RECORDINFO, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_RECORDINFO, old_xml_sn, transfer_xml_sn, strDeviceID, i);
        }

        return i;
    }
    else if (1 == pGBLogicDeviceInfo->record_type) /* 前端录像的情况下，直接转发给前端处理 */
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询录像记录信息的点位是在前端录像,上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 所在的前端设备ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for video record info from superior CMS is on the front end video, the higher the CMS, ID = % s = % s IP address and port number = % d, query logic device ID = % s, where the front-end device ID = % s, = % s IP address and port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

        /* 从新组建消息 */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Query");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = outPacket.CreateElement((char*)"SN");
        g_transfer_xml_sn++;
        snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
        outPacket.SetElementValue(AccNode, strTransferSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"StartTime");
        outPacket.SetElementValue(AccNode, strStartTime);

        AccNode = outPacket.CreateElement((char*)"EndTime");
        outPacket.SetElementValue(AccNode, strEndTime);

        AccNode = outPacket.CreateElement((char*)"Type");

        /* 如果是NVR或者DVR,则要改为all */
        if (EV9000_DEVICETYPE_IPC == pGBDeviceInfo->device_type
            || EV9000_DEVICETYPE_DVR == pGBDeviceInfo->device_type
            || EV9000_DEVICETYPE_NVR == pGBDeviceInfo->device_type)
        {
            outPacket.SetElementValue(AccNode, (char*)"all");
        }
        else
        {
            if (strRecordType[0] != 0)
            {
                outPacket.SetElementValue(AccNode, strRecordType);
            }
            else
            {
                outPacket.SetElementValue(AccNode, (char*)"all");
            }
        }

        /* 第三方平台查询录像的命令，如果转到前端媒体网关去，前端媒体网关是自己的话，需要加上第三方标识 */
        if (EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
        {
            if (1 == pRouteInfo->three_party_flag && 0 == pGBDeviceInfo->three_party_flag)
            {
                AccNode = outPacket.CreateElement((char*)"ThreeParty");
                outPacket.SetElementValue(AccNode, (char*)"YES");
            }
        }

        AccNode = outPacket.CreateElement((char*)"RecorderID");
        outPacket.SetElementValue(AccNode, pGBDeviceInfo->device_id);

        /* 将查询录像任务的消息转发出去 */
        i = SIP_SendMessage(NULL, local_cms_id_get(), strDeviceID, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s, 前端设备ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"转发查询消息到前端设备失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS , forward search info to front-end device failed:superior CMS ID=%s, IP address=%s, port number=%d, front-end numberID=%s, IP address=%s, port number=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询录像记录信息成功,转发查询消息到前端设备:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 前端设备ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for video record info from superior CMS , forward search info to front-end device successfully:Superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s, front-end device ID = % s, IP address = % s, port = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:device_id=%s, device_ip=%s, device_port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

            old_xml_sn = strtoul(strSN, NULL, 10);
            transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
            i = transfer_xml_msg_add(XML_QUERY_RECORDINFO, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_RECORDINFO, old_xml_sn, transfer_xml_sn, strDeviceID, i);
        }

        return i;
    }

normal_record:
    {
        /* 2、查找录像回放TSU 资源 */
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() get_idle_tsu_by_resource_balance_for_replay: Begin--- \r\n", tsu_index);
        tsu_index = get_idle_tsu_by_resource_balance_for_replay();
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() get_idle_tsu_by_resource_balance_for_replay: End--- tsu_index=%d \r\n", tsu_index);

        if (tsu_index < 0)
        {
            /* 回复响应,组建消息 */
            CPacket outPacket;
            DOMElement* AccNode = NULL;

            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

            AccNode = outPacket.CreateElement((char*)"SumNum");
            outPacket.SetElementValue(AccNode, (char*)"-1");

            AccNode = outPacket.CreateElement((char*)"ErrCode");
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_IDLE_TSU_INDEX_ERROR);
            outPacket.SetElementValue(AccNode, strErrorCode);

            AccNode = outPacket.CreateElement((char*)"Reason");
            outPacket.SetElementValue(AccNode, (char*)"获取可用的TSU索引失败");

            RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
            outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

            /* 发送响应消息 给上级CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            if (-2 == tsu_index)
            {
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"查找可用的录像回放TSU索引失败,TSU资源队列为空");
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"access available TSU index failed");
            }
            else if (-3 == tsu_index)
            {
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"查找可用的录像回放TSU索引失败,TSU资源信息错误");
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"access available TSU index failed");
            }
            else if (-4 == tsu_index)
            {
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"查找可用的录像回放TSU索引失败,TSU资源都没有启用");
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"access available TSU index failed");
            }
            else if (-5 == tsu_index)
            {
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"查找可用的录像回放TSU索引失败,TSU资源都不在线");
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"access available TSU index failed");
            }
            else if (-9 == tsu_index)
            {
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"查找可用的录像回放TSU索引失败,通过ICE获取所有的TSU资源状态失败");
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"access available TSU index failed");
            }
            else
            {
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"查找可用的录像回放TSU索引失败");
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"access available TSU index failed");
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Get Idle TSU Index Error \r\n");
            return -1;
        }

        pTsuResourceInfo = tsu_resource_info_get(tsu_index);

        if (NULL == pTsuResourceInfo)
        {
            /* 回复响应,组建消息 */
            CPacket outPacket;
            DOMElement* AccNode = NULL;

            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

            AccNode = outPacket.CreateElement((char*)"SumNum");
            outPacket.SetElementValue(AccNode, (char*)"-1");

            AccNode = outPacket.CreateElement((char*)"ErrCode");
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_GET_TSU_INFO_ERROR);
            outPacket.SetElementValue(AccNode, strErrorCode);

            AccNode = outPacket.CreateElement((char*)"Reason");
            outPacket.SetElementValue(AccNode, (char*)"获取可用的TSU信息失败");

            RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
            outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

            /* 发送响应消息 给上级CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Get TSU Resource Info Error \r\n");
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s, tsu_index=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"获取录像回放的TSU信息失败", tsu_index);
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IPaddress=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"access available TSU info failed");
            return -1;
        }

        /* 3、通知TSU, 查询录像列表 */
        iStartTime = analysis_time(strStartTime);
        iEndTime = analysis_time(strEndTime);

        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() iStartTime=%d,iEndTime=%d \r\n", iStartTime, iEndTime);

        if ((iStartTime <= 0) || (iEndTime <= 0))
        {
            /* 回复响应,组建消息 */
            CPacket outPacket;
            DOMElement* AccNode = NULL;

            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

            AccNode = outPacket.CreateElement((char*)"SumNum");
            outPacket.SetElementValue(AccNode, (char*)"-1");

            AccNode = outPacket.CreateElement((char*)"ErrCode");
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_RECORD_TIME_INFO_ERROR);
            outPacket.SetElementValue(AccNode, strErrorCode);

            AccNode = outPacket.CreateElement((char*)"Reason");
            outPacket.SetElementValue(AccNode, (char*)"录像时间信息不对");

            RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
            outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

            /* 发送响应消息 给上级CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Get Time Error \r\n");
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s,开始时间=%s, 结束时间=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"录像时间信息不对", strStartTime, strEndTime);
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IPaddress=%s, port number=%d, cause=%s,start time=%s, end time=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"record time incorrect", strStartTime, strEndTime);
            return -1;
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询录像记录信息, 开始通知TSU查询记录:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, TSU ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pTsuResourceInfo->tsu_device_id);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for video record info from superior CMS , start to notify TSU search record:CMS ID=%s, IPaddress=%s, port number=%d, TSU ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, pTsuResourceInfo->tsu_device_id);

        /* 查找该点位的录像任务，通知TSU更新结束时间 */
        if (EV9000_RECORD_TYPE_NORMAL == record_type
            || EV9000_RECORD_TYPE_ALARM == record_type)
        {
            iRecordCRPos = record_call_record_find_by_calleeid_and_streamtype(strDeviceID, EV9000_STREAM_TYPE_MASTER);
        }
        else if (EV9000_RECORD_TYPE_BACKUP == record_type)
        {
            iRecordCRPos = record_call_record_find_by_calleeid_and_streamtype(strDeviceID, EV9000_STREAM_TYPE_SLAVE);
        }
        else if (EV9000_RECORD_TYPE_INTELLIGENCE == record_type)
        {
            iRecordCRPos = record_call_record_find_by_calleeid_and_streamtype(strDeviceID, EV9000_STREAM_TYPE_INTELLIGENCE);
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc():record_call_record_find_by_calleeid_and_streamtype:callee_id=%s, RecordCRPos=%d\r\n", strDeviceID, iRecordCRPos);

        if (iRecordCRPos >= 0)
        {
            pRecordCrData = call_record_get(iRecordCRPos);

            if (NULL != pRecordCrData)
            {
                i = notify_tsu_update_mysql_record_stoptime(pRecordCrData->tsu_ip, pRecordCrData->task_id);
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc():notify_tsu_update_mysql_record_stoptime:tsu_ip=%s, task_id=%s, i=%d \r\n", pRecordCrData->tsu_ip, pRecordCrData->task_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: call_record_get Error: RecordCRPos=%d \r\n", iRecordCRPos);
            }
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: record_call_record_find_by_calleeid_and_streamtype Error: callee_id=%s \r\n", strDeviceID);
        }

        /* 通知TSU查询录像 */
        iRet = notify_tsu_query_replay_list(pTsuResourceInfo, strDeviceID, 1, record_type, iStartTime, iEndTime, stVideoRecordList);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() notify_tsu_query_replay_list Error:i=%d \r\n", iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() notify_tsu_query_replay_list OK:i=%d \r\n", iRet);
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询录像记录信息, TSU查询记录结束:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, TSU ID=%s, i=%d, 记录数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pTsuResourceInfo->tsu_device_id, i, stVideoRecordList.size());
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for video record info from superior CMS , TSU search record result:CMS ID=%s, IPaddress=%s, port number=%d, TSU ID=%s, i=%d, record number=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, pTsuResourceInfo->tsu_device_id, i, stVideoRecordList.size());

        if (iRet < 0)
        {
            /* 回复响应,组建消息 */
            CPacket outPacket;
            DOMElement* AccNode = NULL;

            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

            AccNode = outPacket.CreateElement((char*)"SumNum");
            outPacket.SetElementValue(AccNode, (char*)"-1");

            AccNode = outPacket.CreateElement((char*)"ErrCode");
            memset(strErrorCode, 0, 32);

            if (-1 == iRet)
            {
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_TSU_ICE_ERROR);
            }
            else
            {
                snprintf(strErrorCode, 32, "%d", iRet);
            }

            outPacket.SetElementValue(AccNode, strErrorCode);

            AccNode = outPacket.CreateElement((char*)"Reason");
            outPacket.SetElementValue(AccNode, (char*)"TSU查询录像记录返回失败,ICE异常");

            RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
            outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"-1");

            /* 发送响应消息 给上级CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            if (-1 == iRet)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Notify TSU Query Replay List Error \r\n");
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"TSU查询录像记录返回失败, ICE异常");
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"TSU search for video record return failed, ICE abnormal");
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: Notify TSU Query Replay List Error \r\n");
                SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s, iRet=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"TSU查询录像记录返回失败", iRet);
                EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s, iRet=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"TSUTSU search for video record return failed", iRet);
            }

            return -1;
        }

        record_count = stVideoRecordList.size();
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_record_info_proc() record_count=%d \r\n", record_count);

        if (record_count == 0)
        {
            /* 回复响应,组建消息 */
            CPacket outPacket;
            DOMElement* AccNode = NULL;

            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"RecordInfo");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"DeviceID");
            outPacket.SetElementValue(AccNode, strDeviceID);

            AccNode = outPacket.CreateElement((char*)"Name");
            outPacket.SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

            AccNode = outPacket.CreateElement((char*)"SumNum");
            outPacket.SetElementValue(AccNode, (char*)"0");

            RecordListAccNode = outPacket.CreateElement((char*)"RecordList");
            outPacket.SetElementAttr(RecordListAccNode, (char*)"Num", (char*)"0");

            /* 发送响应消息 给上级CMS */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的查询录像记录信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"没有查询到录像记录");
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_WARNING, "Search for video record info from superior CMS failed:CMS ID=%s, IP address=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Video record not found.");
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_record_info_proc() exit---: No Record Count \r\n");
            return i;
        }

        /* 4、循环查找容器，读取录像文件列表信息，加入xml中 */
        CPacket* pOutPacket = NULL;

        if ((str3PartFlag[0] != 0 && 0 == sstrcmp(str3PartFlag, (char*)"YES"))
            || (1 == pRouteInfo->three_party_flag)) /* 第三方平台,发送条数有限制 */
        {
            for (index = 0; index < record_count; index++)
            {
                /* 如果记录数大于10，则要分次发送 */
                query_count++;

                /* 创建XML头部 */
                i = CreateRecordInfoQueryResponseXMLHeadForRoute(&pOutPacket, query_count, record_count, strSN, strDeviceID, pGBLogicDeviceInfo->device_name, &RecordListAccNode);

                /* 加入Item 值 */
                i = AddRecordInfoToXMLItemForRoute(pOutPacket, RecordListAccNode, stVideoRecordList[index], strDeviceID, pGBLogicDeviceInfo->device_name);

                if ((query_count % MAX_ROUTE_RECORD_INFO_COUT_SEND == 0) || (query_count == record_count))
                {
                    if (NULL != pOutPacket)
                    {
                        /* 发送响应消息 给上级CMS */
                        i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息, 发送Message消息到上级CMS失败:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS video query information, Message sending messages to the superior CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询录像记录信息, 发送Message消息到上级CMS成功:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS video query information, Message sending messages to the superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }

                        delete pOutPacket;
                        pOutPacket = NULL;
                    }
                }
            }
        }
        else /* 如果是自己平台，则按照发送给用户的条数发送 */
        {
            for (index = 0; index < record_count; index++)
            {
                /* 如果记录数大于10，则要分次发送 */
                query_count++;

                /* 创建XML头部 */
                i = CreateRecordInfoQueryResponseXMLHead(&pOutPacket, query_count, record_count, strSN, strDeviceID, pGBLogicDeviceInfo->device_name, &RecordListAccNode);

                /* 加入Item 值 */
                i = AddRecordInfoToXMLItem(pOutPacket, RecordListAccNode, stVideoRecordList[index], strDeviceID, pGBLogicDeviceInfo->device_name);

                if ((query_count % MAX_USER_RECORD_INFO_COUT_SEND == 0) || (query_count == record_count))
                {
                    if (NULL != pOutPacket)
                    {
                        /* 发送响应消息 给上级CMS */
                        i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息, 发送Message消息到上级CMS失败:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS video query information, Message sending messages to the superior CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询录像记录信息, 发送Message消息到上级CMS成功:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS video query information, Message sending messages to the superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_record_info_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }

                        delete pOutPacket;
                        pOutPacket = NULL;
                    }
                }
            }
        }
    }

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询录像记录信息成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS video query information of success: the superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
    }
    else
    {
        SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询录像记录信息成功:上级ID=%s, IP地址=%s, 端口号=%d, 原因=%s, 查询的逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"发送SIP响应消息失败");
        EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS video query information failure: superior, ID = % s = % s IP address, port number = % d, reason = % s, query logic device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"SIP response message sent failure");
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : route_query_preset_info_proc
 功能描述  : 上级互联CMS发送过来的查询设备预置位信息处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年3月15日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_query_preset_info_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int record_count = 0; /* 记录数 */
    int send_count = 0;   /* 发送的次数 */
    int query_count = 0;  /* 查询数据统计 */

    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    int while_count = 0;
    char strTransferSN[32] = {0};
    DOMElement* AccSnNode = NULL;
    unsigned int old_xml_sn = 0;
    unsigned int transfer_xml_sn = 0;

    int iCalleeRoutePos = 0;
    route_info_t* pCalleeRouteInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_preset_info_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_preset_info_proc() exit---: para Error \r\n");
        return -1;
    }

    /* 取得查询条件数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询预置位信息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for preset info from superior CMS :CMS ID=%s, IP address=%s, port=%d, logic deviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);

    /* 1、获取录像点位信息 */
    pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

    if (NULL == pGBLogicDeviceInfo)
    {
        /* 回复响应,组建消息 */
        CPacket* pOutPacket = NULL;
        DOMElement* AccNode = NULL;
        DOMElement* ListAccNode = NULL;

        //DOMElement* ItemAccNode = NULL;
        if (!pOutPacket)
        {
            pOutPacket = new CPacket();
        }

        pOutPacket->SetRootTag("Response");
        AccNode = pOutPacket->CreateElement((char*)"CmdType");
        pOutPacket->SetElementValue(AccNode, (char*)"PresetConfig");

        AccNode = pOutPacket->CreateElement((char*)"SN");
        pOutPacket->SetElementValue(AccNode, strSN);

        AccNode = pOutPacket->CreateElement((char*)"DeviceID");
        pOutPacket->SetElementValue(AccNode, strDeviceID);

        AccNode = pOutPacket->CreateElement((char*)"SumNum");
        pOutPacket->SetElementValue(AccNode, (char*)"0");

        ListAccNode = pOutPacket->CreateElement((char*)"PresetConfigList");
        pOutPacket->SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

        /* 发送响应消息 给上级CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        if (pOutPacket)
        {
            delete pOutPacket;
        }

        pOutPacket = NULL;
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() exit---: Get GBLogic Device Info Error \r\n");
        SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询预置位信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"获取逻辑设备信息失败");
        EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for preset info from superior CMS failed:CMS ID=%s, IP address=%s, port=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Access logical device information failed");
        return -1;
    }

    /* 根据逻辑设备所属域进行判断，决定消息走向 */
    if (1 == pGBLogicDeviceInfo->other_realm)
    {
        /* 查找上级路由信息 */
        iCalleeRoutePos = route_info_find(pGBLogicDeviceInfo->cms_id);

        if (iCalleeRoutePos < 0)
        {
            /* 回复响应,组建消息 */
            CPacket* pOutPacket = NULL;
            DOMElement* AccNode = NULL;
            DOMElement* ListAccNode = NULL;

            //DOMElement* ItemAccNode = NULL;
            if (!pOutPacket)
            {
                pOutPacket = new CPacket();
            }

            pOutPacket->SetRootTag("Response");

            AccNode = pOutPacket->CreateElement((char*)"CmdType");
            pOutPacket->SetElementValue(AccNode, (char*)"PresetConfig");

            AccNode = pOutPacket->CreateElement((char*)"SN");
            pOutPacket->SetElementValue(AccNode, strSN);

            AccNode = pOutPacket->CreateElement((char*)"DeviceID");
            pOutPacket->SetElementValue(AccNode, strDeviceID);

            AccNode = pOutPacket->CreateElement((char*)"SumNum");
            pOutPacket->SetElementValue(AccNode, (char*)"0");

            ListAccNode = pOutPacket->CreateElement((char*)"PresetConfigList");
            pOutPacket->SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

            /* 发送响应消息 */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:CMS ID=%s, IP address=%s, port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:CMS ID=%s, IP address=%s, port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            if (pOutPacket)
            {
                delete pOutPacket;
            }

            pOutPacket = NULL;
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() exit---: Find Callee Route Info Error \r\n");
            SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询预置位信息失败:CMD ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"查找逻辑设备对应的上级路由信息失败");
            EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for preset info from superior CMS failed:CMS ID=%s, IP address=%s, port=%d, reason=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Find the route information which corresponding to logical device failed.");
            return -1;
        }

        pCalleeRouteInfo = route_info_get(iCalleeRoutePos);

        if (NULL == pCalleeRouteInfo)
        {
            /* 回复响应,组建消息 */
            CPacket* pOutPacket = NULL;
            DOMElement* AccNode = NULL;
            DOMElement* ListAccNode = NULL;

            //DOMElement* ItemAccNode = NULL;
            if (!pOutPacket)
            {
                pOutPacket = new CPacket();
            }

            pOutPacket->SetRootTag("Response");

            AccNode = pOutPacket->CreateElement((char*)"CmdType");
            pOutPacket->SetElementValue(AccNode, (char*)"PresetConfig");

            AccNode = pOutPacket->CreateElement((char*)"SN");
            pOutPacket->SetElementValue(AccNode, strSN);

            AccNode = pOutPacket->CreateElement((char*)"DeviceID");
            pOutPacket->SetElementValue(AccNode, strDeviceID);

            AccNode = pOutPacket->CreateElement((char*)"SumNum");
            pOutPacket->SetElementValue(AccNode, (char*)"0");

            ListAccNode = pOutPacket->CreateElement((char*)"PresetConfigList");
            pOutPacket->SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

            /* 发送响应消息 */
            i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:CMS ID=%s, IP address=%s, port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:CMS ID=%s, IP address=%s, port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }

            if (pOutPacket)
            {
                delete pOutPacket;
            }

            pOutPacket = NULL;
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() exit---: Get Callee Route Info Error \r\n");
            SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询预置位信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"获取逻辑设备对应的上级路由信息失败");
            EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for preset info from superior CMS failed:CMS ID=%s, IP address=%s, port=%d, reason=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Get the route information which corresponding to logical device failed.");
            return -1;
        }

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询预置位信息是上级CMS中的点位:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 所在的上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for preset info from superior CMS is the higher level in the CMS: superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s, the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);

        AccSnNode = inPacket.SearchElement((char*)"SN");

        if (NULL != AccSnNode)
        {
            g_transfer_xml_sn++;
            snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
            inPacket.SetElementValue(AccSnNode, strTransferSN);
            //inPacket.SetTextContent();
        }

        /* 将查询录像任务的消息转发出去 */
        i = SIP_SendMessage(NULL, local_cms_id_get(), pCalleeRouteInfo->server_id, pCalleeRouteInfo->strRegLocalIP, pCalleeRouteInfo->iRegLocalPort, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

        if (i != 0)
        {
            SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询预置位信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s, 转发上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"转发查询消息到上级CMS失败", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for preset info from superior CMS, forwards the query message to the superior CMS failure:CMS ID=%s, IP address=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:CMS ID=%s, IP address=%s, port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询预置位信息, 转发查询消息到上级CMS成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 转发上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for preset info from superior CMS, forwards the query message to the superior CMS success:Superior CMS, ID = % s = % s IP address, port number = % d, forward the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:CMS ID=%s, IP address=%s, port=%d \r\n", pCalleeRouteInfo->server_id, pCalleeRouteInfo->server_ip, pCalleeRouteInfo->server_port);

            old_xml_sn = strtoul(strSN, NULL, 10);
            transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
            i = transfer_xml_msg_add(XML_QUERY_GETPRESET, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_preset_info_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_GETPRESET, old_xml_sn, transfer_xml_sn, strDeviceID, i);
        }
    }
    else
    {
        /* 获取逻辑设备所属的物理设备 */
        pGBDeviceInfo = GBDevice_info_get_by_stream_type(pGBLogicDeviceInfo, EV9000_STREAM_TYPE_MASTER);

        if (NULL != pGBDeviceInfo
            && EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
            && 0 == pGBDeviceInfo->three_party_flag) /* 非第三方平台的下级cms 的点位 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询预置位信息是下级CMS中的点位:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 所在的下级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for preset info from superior CMS is the lower level in the CMS:Superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s, the lower the CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

            AccSnNode = inPacket.SearchElement((char*)"SN");

            if (NULL != AccSnNode)
            {
                g_transfer_xml_sn++;
                snprintf(strTransferSN, 32, "%u", g_transfer_xml_sn);
                inPacket.SetElementValue(AccSnNode, strTransferSN);
                //inPacket.SetTextContent();
            }

            /* 将查询录像任务的消息转发出去 */
            i = SIP_SendMessage(NULL, local_cms_id_get(), pGBDeviceInfo->device_id, pGBDeviceInfo->strRegServerIP, pGBDeviceInfo->iRegServerPort, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

            if (i != 0)
            {
                SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询预置位信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s, 转发下级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"转发查询消息到下级CMS失败", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for preset info from superior CMS ,forward search info to subordinate CMS failed:CMS ID=%s, IP address=%s, port=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:CMS ID=%s, IP address=%s, port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询预置位信息成功,转发查询消息到下级CMS:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 转发下级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Search for preset info from superior CMS success,forward search info to subordinate CMS:Superior CMS, ID = % s = % s IP address, port number = % d, query logic device ID = % s, forwarding the lower CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:CMS ID=%s, IP address=%s, port=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                old_xml_sn = strtoul(strSN, NULL, 10);
                transfer_xml_sn = strtoul(strTransferSN, NULL, 10);
                i = transfer_xml_msg_add(XML_QUERY_GETPRESET, old_xml_sn, transfer_xml_sn, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, strDeviceID);
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_query_preset_info_proc() transfer_xml_msg_add: Type=%d, old_xml_sn=%u, transfer_xml_sn=%u, DeviceID=%s, pos=%d \r\n", XML_QUERY_GETPRESET, old_xml_sn, transfer_xml_sn, strDeviceID, i);
            }
        }
        else
        {
            //query
            char szSql[100] = {0};
            char strSumNum[32] = {0};
            snprintf(szSql, 100, "select * from PresetConfig WHERE DeviceIndex = %u order by PresetID asc;", pGBLogicDeviceInfo->id);
            record_count  = pRoute_Srv_dboper->DB_Select(szSql, 1);
            snprintf(strSumNum, 32, "%d", record_count);

            if (record_count < 0)
            {
                /* 回复响应,组建消息 */
                CPacket* pOutPacket = NULL;
                DOMElement* AccNode = NULL;
                DOMElement* ListAccNode = NULL;

                //DOMElement* ItemAccNode = NULL;
                if (!pOutPacket)
                {
                    pOutPacket = new CPacket();
                }

                pOutPacket->SetRootTag("Response");

                AccNode = pOutPacket->CreateElement((char*)"CmdType");
                pOutPacket->SetElementValue(AccNode, (char*)"PresetConfig");

                AccNode = pOutPacket->CreateElement((char*)"SN");
                pOutPacket->SetElementValue(AccNode, strSN);

                AccNode = pOutPacket->CreateElement((char*)"DeviceID");
                pOutPacket->SetElementValue(AccNode, strDeviceID);

                AccNode = pOutPacket->CreateElement((char*)"SumNum");
                pOutPacket->SetElementValue(AccNode, (char*)"0");

                ListAccNode = pOutPacket->CreateElement((char*)"PresetConfigList");
                pOutPacket->SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

                /* 发送响应消息 给上级CMS */
                i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "发送Message消息到目的地失败:目的ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Sending messages to a destination failure: objective, ID = % s = % s IP address, port number = % d ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送Message消息到目的地成功:目的ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Sending messages to a destination success: objective, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }

                if (pOutPacket)
                {
                    delete pOutPacket;
                }

                pOutPacket = NULL;
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() Error DB Oper Error:strSQL=%s, record_count=%d \r\n", szSql, record_count);
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() ErrorMsg=%s\r\n", pRoute_Srv_dboper->GetLastDbErrorMsg());
                SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询预置位信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"查询数据库失败");
                EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Search for preset info from superior CMS failed:CMS ID=%s, IP address=%s, port=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Search in database failed");
                return -1;
            }
            else if (record_count == 0)
            {
                /* 回复响应,组建消息 */
                CPacket* pOutPacket = NULL;
                DOMElement* AccNode = NULL;
                DOMElement* ListAccNode = NULL;

                //DOMElement* ItemAccNode = NULL;
                if (!pOutPacket)
                {
                    pOutPacket = new CPacket();
                }

                pOutPacket->SetRootTag("Response");
                AccNode = pOutPacket->CreateElement((char*)"CmdType");
                pOutPacket->SetElementValue(AccNode, (char*)"PresetConfig");

                AccNode = pOutPacket->CreateElement((char*)"SN");
                pOutPacket->SetElementValue(AccNode, strSN);

                AccNode = pOutPacket->CreateElement((char*)"DeviceID");
                pOutPacket->SetElementValue(AccNode, strDeviceID);

                AccNode = pOutPacket->CreateElement((char*)"SumNum");
                pOutPacket->SetElementValue(AccNode, (char*)"0");

                ListAccNode = pOutPacket->CreateElement((char*)"PresetConfigList");
                pOutPacket->SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

                /* 发送响应消息 给上级CMS */
                i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "发送Message消息到目的地失败:目的ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Sending messages to a destination failure: objective, ID = % s = % s IP address, port number = % d ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送Message消息到目的地成功:目的ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Sending messages to a destination success: objective, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }

                if (pOutPacket)
                {
                    delete pOutPacket;
                }

                pOutPacket = NULL;
                DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_preset_info_proc() exit---: No Record Count \r\n");
                SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的查询预置位信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"没有查询到数据库记录");
                EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_WARNING, "Search for preset info from superior CMS failed:CMS ID=%s, IP address=%s, port=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Database record not found");
                return i;
            }

            CPacket* pOutPacket = NULL;
            DOMElement* ListAccNode = NULL;

            /* 循环查找数据库，将数据组成XML 发送给客户端 */
            do
            {
                int             nID = 0;                                      //记录编号
                unsigned int    nDeviceIndex = 0;                            //设备ID
                int             nPresetID = 0;                               //预置位编号
                string          strPresetName = "";                          //预置位名称
                int             nResved1 = 0;                                //保留1
                string          strResved2 = "";

                while_count++;

                if (while_count % 10000 == 0)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_preset_info_proc() While Count=%d \r\n", while_count);
                }

                query_count++;

                /* 创建XML头部 */
                i = CreatePresetConfigResponseXMLHead(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

                /* 索引 */
                pRoute_Srv_dboper->GetFieldValue("ID", nID);

                /* 点位索引 */
                pRoute_Srv_dboper->GetFieldValue("DeviceIndex", nDeviceIndex);

                /* 预置位ID */
                pRoute_Srv_dboper->GetFieldValue("PresetID", nPresetID);

                /* 预置位名称 */
                strPresetName.clear();
                pRoute_Srv_dboper->GetFieldValue("PresetName", strPresetName);

                /* 预留1 */
                pRoute_Srv_dboper->GetFieldValue("Resved1", nResved1);

                /* 预留2 */
                strResved2.clear();
                pRoute_Srv_dboper->GetFieldValue("ParentID", strResved2);

                /* 加入Item 值 */
                i = AddPresetConfigToXMLItem(pOutPacket, ListAccNode, nID, nDeviceIndex, nPresetID, strPresetName, nResved1, strResved2);

                if ((query_count % MAX_DEVICE_PRESET_COUT_SEND == 0) || (query_count == record_count))
                {
                    if (NULL != pOutPacket)
                    {
                        send_count++;
                        /* 发送出去 */
                        i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询预置位信息, 发送Message消息到上级CMS失败:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Query preset information from superior CMS, sending messages to the superior CMS failure: the higher the CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_preset_info_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询预置位信息, 发送Message消息到上级CMS成功:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Query preset information from superior CMS, sending messages to the superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_preset_info_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }

                        delete pOutPacket;
                        pOutPacket = NULL;
                    }
                }
            }
            while (pRoute_Srv_dboper->MoveNext() >= 0);

            if (i == 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询预置位信息成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 记录数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device group information success:front-end ID=%s, IP=%s, port=%d, record count=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
            }
            else
            {
                SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的查询预置位信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"发送SIP响应消息失败");
                EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logic device group information failure:front-end ID=%s, IP=%s, port=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"SIP response message sent failure");
            }
        }
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的查询预置位信息成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询的逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Query preset information from superior CMS, sending messages to the superior CMS success: the higher the CMS, ID = % s = % s IP address, port number = % d ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    return 0;
}

/*****************************************************************************
 函 数 名  : route_query_device_group_config_proc
 功能描述  : 上级互联CMS发送过来的获取逻辑设备分组配置表
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月12日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_query_device_group_config_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    /* 上级CMS 获取逻辑设备分组配置表
      */
    int i = 0;
    int record_count = 0; /* 记录数 */
    int send_count = 0;   /* 发送的次数 */
    int query_count = 0;  /* 查询数据统计 */

    char strSN[32] = {0};
    char strDeviceID[32] = {0};

    DOMElement* ListAccNode = NULL;

    string strSQL = "";
    int while_count = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_group_config_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_group_config_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取逻辑设备分组配置消息:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group config info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    /* 如果不是获取本CMS，则返回 */
    if (0 != strncmp(callee_id, local_cms_id_get(), 20))
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备分组配置消息处理失败:请求方=%s, IP地址=%s, 端口号=%d, 原因=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"查询的ID不是本CMS的ID", callee_id);
        EnSystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group config info from superior CMS failed:IP address=%s,IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_group_config_proc() exit---: DeviceID Not Belong To Mine CMSID:callee_id=%s \r\n", callee_id);
        return -1;
    }

    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_device_group_config_proc() \
        \r\n XML Para: \
        \r\n SN=%s, DeviceID=%s \r\n", strSN, strDeviceID);

    /* 如果查询的设备ID不是本CMS ID*/
    if (0 != sstrcmp(callee_id, strDeviceID))
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备分组配置消息处理失败:上级CMS ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"查询的ID不是本CMS的ID", strDeviceID);
        EnSystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group config info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_group_config_proc() exit---: DeviceID Not Belong To Mine CMSID:DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    /* 根据查询条件，查找数据库，找到相应的逻辑设备分组配置信息 */
    strSQL.clear();
    strSQL = "select * from LogicDeviceGroupConfig WHERE OtherRealm = 0 order by GroupID asc";
    record_count = pRoute_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_group_config_proc() record_count=%d \r\n", record_count);

    if (record_count <= 0)
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备分组配置消息处理:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询到的分组配置总数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取逻辑设备分组配置消息处理:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询到的分组配置总数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group config info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, total number of queries to the grouping configuration = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
    }

    if (record_count < 0)
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备分组配置消息处理, 查询数据库失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"查询数据库失败", pRoute_Srv_dboper->GetLastDbErrorMsg());
        EnSystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group config info from superior CMS search in database failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"searcch in database failed", pRoute_Srv_dboper->GetLastDbErrorMsg());
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_group_config_proc() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_group_config_proc() ErrorMsg=%s\r\n", pRoute_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }

    if (record_count == 0)
    {
        /* 回复响应,组建消息 */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"LogicDeviceGroupConfig");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"0");

        ListAccNode = outPacket.CreateElement((char*)"LogicDeviceGroupConfigList");
        outPacket.SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

        /* 发送响应消息给上级CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_group_config_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_group_config_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        SystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的获取逻辑设备分组配置消息处理, 查询数据库失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"未查询到数据库记录");
        EnSystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_WARNING, "Access device group config info from superior CMS search in database failed:Requester ID=%s, IP address=%s, port number=%d,cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Database record not found");
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_device_group_config_proc() exit---: No Record Count \r\n");
        return i;

    }

    CPacket* pOutPacket = NULL;

    /* 循环查找数据库，将数据组成XML 发送给上级CMS */
    do
    {
        int id = 0;
        string strName = "";
        string strGroupID = "";
        string strCMSID = "";
        int SortID = 0;
        string strParentID = "";

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_device_group_config_proc() While Count=%d \r\n", while_count);
        }

        query_count++;

        /* 创建XML头部 */
        i = CreateDeviceGroupConfigResponseXMLHead(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

        /* 索引 */
        pRoute_Srv_dboper->GetFieldValue("ID", id);

        /* 组编号 */
        strGroupID.clear();
        pRoute_Srv_dboper->GetFieldValue("GroupID", strGroupID);

        /* CMS编号 */
        /*
           strCMSID.clear();
           pRoute_Srv_dboper->GetFieldValue("CMSID", strCMSID);
           */

        /* 组名称 */
        strName.clear();
        pRoute_Srv_dboper->GetFieldValue("Name", strName);

        /* 同一父节点下组排序编号 */
        pRoute_Srv_dboper->GetFieldValue("SortID", SortID);

        /* 父节点编号 */
        strParentID.clear();
        pRoute_Srv_dboper->GetFieldValue("ParentID", strParentID);

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取逻辑设备分组配置消息处理, 查询到的分组信息:组编号=%s, 组名称=%s", strGroupID.c_str(), strName.c_str());
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group config info from superior CMS, Query to the grouping of information: the group number = % s, group name = % s", strGroupID.c_str(), strName.c_str());

        /* 加入Item 值 */
        i = AddDeviceGroupConfigToXMLItem(pOutPacket, ListAccNode, id, strGroupID, strCMSID, strName, SortID, strParentID);

        if ((query_count % MAX_DEVICE_GROUP_COUT_SEND == 0) || (query_count == record_count))
        {
            if (NULL != pOutPacket)
            {
                send_count++;
                /* 发送响应消息 给上级CMS */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备分组配置消息处理, 发送Message消息到上级CMS失败:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group config info from superior CMS, Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_group_config_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取逻辑设备分组配置消息处理, 发送Message消息到上级CMS成功:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group config info from superior CMS, Send Message Message to superiors CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_group_config_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }

                delete pOutPacket;
                pOutPacket = NULL;
            }
        }
    }
    while (pRoute_Srv_dboper->MoveNext() >= 0);

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取逻辑设备分组配置消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 记录数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group config info from superior CMS, Send Message Message to superiors CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备分组配置消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"发送SIP响应消息失败");
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group config info from superior CMS, Send Message Message to superiors CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_group_config_proc Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : route_query_device_map_group_config_proc
 功能描述  : 上级互联CMS发送过来的获取逻辑设备分组关系配置表
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月12日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_query_device_map_group_config_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    /* 上级CMS过来的获取逻辑设备分组关系配置表
      */
    int i = 0;
    int record_count = 0; /* 记录数 */
    int send_count = 0;   /* 发送的次数 */
    int query_count = 0;  /* 查询数据统计 */

    char strSN[32] = {0};
    char strDeviceID[32] = {0};

    DOMElement* ListAccNode = NULL;

    string strSQL = "";
    int while_count = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_map_group_config_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_device_map_group_config_proc() exit---: User Srv DB Oper Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取逻辑设备分组关系配置消息:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group relationship config info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    /* 如果不是获取本CMS，则返回 */
    if (0 != strncmp(callee_id, local_cms_id_get(), 20))
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备分组关系配置消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"查询的ID不是本CMS的ID", callee_id);
        EnSystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group relationship config info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_map_group_config_proc() exit---: DeviceID Not Belong To Mine CMSID \r\n");
        return -1;
    }

    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_device_map_group_config_proc() \
            \r\n XML Para: \
            \r\n SN=%s, DeviceID=%s \r\n", strSN, strDeviceID);

    /* 如果查询的设备ID不是本CMS ID*/
    if (0 != sstrcmp(callee_id, strDeviceID))
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备分组关系配置消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"查询的ID不是本CMS的ID", strDeviceID);
        EnSystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group relationship config info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_record_info_proc() exit---: DeviceID Not Belong To Mine CMSID \r\n");
        return -1;
    }

    /* 根据查询条件，查找数据库，找到相应的逻辑设备分组关系配置关系信息*/
    strSQL.clear();
    strSQL = "SELECT G.ID, G.GroupID, G.DeviceIndex, G.SortID FROM LogicDeviceMapGroupConfig AS G, GBLogicDeviceConfig AS GD WHERE G.DeviceIndex = GD.ID AND GD.OtherRealm=0";
    record_count = pRoute_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_map_group_config_proc() record_count=%d \r\n", record_count);

    if (record_count <= 0)
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备分组关系配置消息处理:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询到的分组关系配置总数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group relationship config info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, total number of queries to grouping relationship configuration = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取逻辑设备分组关系配置消息处理:上级CMS ID=%s, IP地址=%s, 端口号=%d, 查询到的分组关系配置总数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access device group relationship config info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, total number of queries to grouping relationship configuration = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
    }

    if (record_count < 0)
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备分组配置关系消息处理, 开始查询数据库失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"查询数据库失败", pRoute_Srv_dboper->GetLastDbErrorMsg());
        EnSystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access device group relationship config info from superior CMS start to search in database failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"searcch in database failed", pRoute_Srv_dboper->GetLastDbErrorMsg());
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_map_group_config_proc() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_map_group_config_proc() ErrorMsg=%s\r\n", pRoute_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }

    if (record_count == 0)
    {
        /* 回复响应,组建消息 */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"LogicDeviceMapGroupConfig");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"0");

        ListAccNode = outPacket.CreateElement((char*)"LogicDeviceMapGroupConfigList");
        outPacket.SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

        /* 发送响应消息 给上级CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_map_group_config_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_map_group_config_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的获取逻辑设备分组配置关系消息处理, 查询数据库失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"未查询到数据库记录");
        EnSystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_WARNING, "Access device group config info from superior CMS search in database failed:Requester ID=%s, IP address=%s, port number=%d,cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Database record not found");
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_device_map_group_config_proc() exit---: No Record Count \r\n");
        return i;
    }

    CPacket* pOutPacket = NULL;

    /* 循环查找数据库，将数据组成XML 发送给上级CMS */
    do
    {
        int id = 0;
        string strGroupID = "";
        unsigned int DeviceIndex = 0;
        string strCMSID = "";
        int SortID = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_device_map_group_config_proc() While Count=%d \r\n", while_count);
        }

        query_count++;

        /* 创建XML头部 */
        i = CreateDeviceMapGroupConfigResponseXMLHead(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

        /* 索引 */
        pRoute_Srv_dboper->GetFieldValue("ID", id);

        /* 点位组编号 */
        strGroupID.clear();
        pRoute_Srv_dboper->GetFieldValue("GroupID", strGroupID);

        /* 逻辑设备索引 */
        pRoute_Srv_dboper->GetFieldValue("DeviceIndex", DeviceIndex);

        /* CMS 编号 */
        /*
           strCMSID.clear();
           pRoute_Srv_dboper->GetFieldValue("CMSID", strCMSID);
           */

        /* 排序编号 */
        pRoute_Srv_dboper->GetFieldValue("SortID", SortID);

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取逻辑设备分组配置关系消息处理, 查询到的分组关系信息:组编号=%s, 逻辑设备索引=%u", strGroupID.c_str(), DeviceIndex);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logical device group configuration from superior CMS  processing,Query to the grouping of relationship information: group number = % s, logical device index = % u", strGroupID.c_str(), DeviceIndex);

        /* 加入Item 值 */
        i = AddDeviceMapGroupConfigToXMLItem(pOutPacket, ListAccNode, id, strGroupID, DeviceIndex, strCMSID, SortID);

        if ((query_count % MAX_DEVICE_MAP_GROUP_COUT_SEND == 0) || (query_count == record_count))
        {
            if (NULL != pOutPacket)
            {
                send_count++;
                /* 发送响应消息 给上级CMS */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备分组配置关系消息处理, 发送Message消息到上级CMS失败:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logical device group configuration from superior CMS  processing, Message sending messages to the superior CMS failure: the higher the CMS, ID = % s = % s IP address and port number = % d" , caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_device_group_config_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取逻辑设备分组配置关系消息处理, 发送Message消息到上级CMS成功:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logical device group configuration from superior CMS  processing, Message sending messages to the superior CMS success: the higher the CMS, ID = % s = % s IP address and port number = % d" , caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_group_config_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }

                delete pOutPacket;
                pOutPacket = NULL;
            }
        }
    }
    while (pRoute_Srv_dboper->MoveNext() >= 0);

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取逻辑设备分组关系配置消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 记录数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logical device group configuration from superior CMS  processing, Message sending messages to the superior CMS success: the higher the CMS, ID = % s = % s IP address and port number = % d" , caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        SystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取逻辑设备分组关系配置消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"发送SIP响应消息失败");
        EnSystemLog(EV9000_CMS_GET_DEIVCE_MAP_GROUP_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logical device group configuration from superior CMS  processing, Message sending messages to the superior CMS failure: the higher the CMS, ID = % s = % s IP address and port number = % d" , caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_device_map_group_config_proc Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : route_query_topology_phydevice_config_proc
 功能描述  : 上级互联CMS发送过来的获取拓扑物理设备配置表
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月27日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_query_topology_phydevice_config_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    /* 上级CMS 获取拓扑物理设备配置表
      */
    int i = 0;
    int iRet = 0;
    int record_count = 0; /* 记录数 */
    int send_count = 0;   /* 发送的次数 */
    int query_count = 0;  /* 查询数据统计 */
    char strDeviceType[16] = {0};

    char strSN[32] = {0};
    char strDeviceID[32] = {0};

    DOMElement* ListAccNode = NULL;

    string strSQL = "";
    int while_count = 0;
    char* local_ip = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_topology_phydevice_config_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_topology_phydevice_config_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取拓扑物理设备配置消息:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access topology physical device config info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    /* 如果不是获取本CMS，则返回 */
    if (0 != strncmp(callee_id, local_cms_id_get(), 20))
    {
        SystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取拓扑物理设备配置信息失败:请求方=%s, IP地址=%s, 端口号=%d, 原因=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"查询的ID不是本CMS的ID", callee_id);
        EnSystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access topology physical device config info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_topology_phydevice_config_proc() exit---: DeviceID Not Belong To Mine CMSID:callee_id=%s \r\n", callee_id);
        return -1;
    }

    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_topology_phydevice_config_proc() \
        \r\n XML Para: \
        \r\n SN=%s, DeviceID=%s \r\n", strSN, strDeviceID);

    /* 如果查询的设备ID不是本CMS ID*/
    if (0 != sstrcmp(callee_id, strDeviceID))
    {
        SystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取拓扑物理设备配置消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"查询的ID不是本CMS的ID", strDeviceID);
        EnSystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access topology physical device config info from superior CMS failed:Requester ID=%s, IP address=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_topology_phydevice_config_proc() exit---: DeviceID Not Belong To Mine CMSID:DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    /* 添加拓扑结构表信息 */
    snprintf(strDeviceType, 16, "%u", EV9000_DEVICETYPE_SIPSERVER);
    local_ip = local_ip_get(default_eth_name_get());
    iRet = AddTopologyPhyDeviceInfo2DB(local_cms_id_get(), local_cms_name_get(), strDeviceType, local_ip, (char*)"1", local_cms_id_get(), (char*)"0", pRoute_Srv_dboper);

    /* 根据查询条件，查找数据库，找到相应的逻辑设备分组配置信息 */
    strSQL.clear();
    strSQL = "select * from TopologyPhyDeviceConfig order by DeviceID asc";
    record_count = pRoute_Srv_dboper->DB_Select(strSQL.c_str(), 1);

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_topology_phydevice_config_proc() record_count=%d \r\n", record_count);

    if (record_count < 0)
    {
        SystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取拓扑物理设备配置消息处理, 查询数据库失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"查询数据库失败", pRoute_Srv_dboper->GetLastDbErrorMsg());
        EnSystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access topology physical device config info from superior CMS search in database failed:Requester ID=%s, IPaddress=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"searcch in database failed", pRoute_Srv_dboper->GetLastDbErrorMsg());
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_topology_phydevice_config_proc() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_topology_phydevice_config_proc() ErrorMsg=%s\r\n", pRoute_Srv_dboper->GetLastDbErrorMsg());
        return -1;
    }

    if (record_count == 0)
    {
        /* 回复响应,组建消息 */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"TopologyPhyDeviceConfig");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"OK");

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"0");

        ListAccNode = outPacket.CreateElement((char*)"TopologyPhyDeviceConfigList");
        outPacket.SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

        /* 发送响应消息 给上级CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_topology_phydevice_config_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_topology_phydevice_config_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        SystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的获取拓扑物理设备配置消息处理, 查询数据库失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"未查询到数据库记录");
        EnSystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_WARNING, "Access topology physical device config info from superior CMS search in database failed:Requester ID=%s, IPaddress=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Database record not found");
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_topology_phydevice_config_proc() exit---: No Record Count \r\n");
        return i;
    }

    CPacket* pOutPacket = NULL;

    /* 循环查找数据库，将数据组成XML 发送给上级CMS */
    do
    {
        string strItemDeviceID = "";
        string strDeviceName = "";
        int iDeviceType = 0;
        string strDeviceIP = "";
        int iStatus = 0;
        string strCMSID = "";
        int iLinkType = 0;

        while_count++;

        if (while_count % 10000 == 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_query_topology_phydevice_config_proc() While Count=%d \r\n", while_count);
        }

        query_count++;

        /* 创建XML头部 */
        i = CreateTopologyPhyDeviceConfigResponseXMLHead(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

        /* 设备ID */
        strItemDeviceID.clear();
        pRoute_Srv_dboper->GetFieldValue("DeviceID", strItemDeviceID);

        /* 设备名称 */
        strDeviceName.clear();
        pRoute_Srv_dboper->GetFieldValue("DeviceName", strDeviceName);

        /* 设备类型 */
        pRoute_Srv_dboper->GetFieldValue("DeviceType", iDeviceType);

        /* 设备IP */
        strDeviceIP.clear();
        pRoute_Srv_dboper->GetFieldValue("DeviceIP", strDeviceIP);

        /* 设备状态 */
        pRoute_Srv_dboper->GetFieldValue("Status", iStatus);

        /* 所属CMSID */
        strCMSID.clear();
        pRoute_Srv_dboper->GetFieldValue("CMSID", strCMSID);

        /* 是否同级 */
        pRoute_Srv_dboper->GetFieldValue("LinkType", iLinkType);

        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取拓扑物理设备配置消息处理, 查询到的分组信息:设备ID=%s, 设备IP=%s", strItemDeviceID.c_str(), strDeviceIP.c_str());
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, " Access topology physical device configuration from Superior CMS, query to the grouping of information: device ID = % s, equipment IP = % s ", strItemDeviceID.c_str(), strDeviceIP.c_str());

        /* 加入Item 值 */
        i = AddTopologyPhyDeviceConfigToXMLItem(pOutPacket, ListAccNode, strItemDeviceID, strDeviceName, iDeviceType, strDeviceIP, iStatus, strCMSID, iLinkType);

        if ((query_count % MAX_TOPOLOGY_DEVICE_CONFIG_COUT_SEND == 0) || (query_count == record_count))
        {
            if (NULL != pOutPacket)
            {
                send_count++;
                /* 发送响应消息 给上级CMS */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取拓扑物理设备配置消息处理, 发送Message消息到上级CMS失败:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access topology physical device configuration from Superior CMS, Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_topology_phydevice_config_proc() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取拓扑物理设备配置消息处理, 发送Message消息到上级CMS成功:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access topology physical device configuration from Superior CMS, Send Message Message to superiors CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_topology_phydevice_config_proc() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }

                delete pOutPacket;
                pOutPacket = NULL;
            }
        }
    }
    while (pRoute_Srv_dboper->MoveNext() >= 0);

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的获取拓扑物理设备配置消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 记录数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access topology physical device configuration from Superior CMS, Send Message Message to superiors CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        SystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的获取拓扑物理设备配置消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"发送SIP响应消息失败");
        EnSystemLog(EV9000_CMS_GET_TOP_DEVICE_CONFIG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access topology physical device configuration from Superior CMS, Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_topology_phydevice_config_proc Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : route_preset_info_response_proc
 功能描述  : 上级互联CMS过来的预置位查询结果返回
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月26日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_preset_info_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    /* 设备预置位信息, 可能是下级CMS返回的数据，需要转发给用户
      */
    int i = 0;
    int xml_pos = -1;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strSumNum[16] = {0};
    string strPresetConfigListNum = "";
    int iSumNum = 0;
    int iPresetConfigListNum = 0;
    unsigned int transfer_xml_sn = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_preset_info_response_proc() exit---: GBDevice Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_preset_info_response_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_preset_info_response_proc() caller_id=%s, callee_id=%s\r\n", caller_id, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS预置位信息查询响应消息:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS preset info search response message:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"SumNum", strSumNum);
    inPacket.GetElementAttr((char*)"PresetConfigList", (char*)"Num", strPresetConfigListNum);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_preset_info_response_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n DeviceID=%s \
    \r\n SumNum=%s \
    \r\n PresetConfigList Num=%s \r\n ", strSN, strDeviceID, strSumNum, (char*)strPresetConfigListNum.c_str());

    if (strSumNum[0] == '\0')
    {
        SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS预置位信息查询响应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=没有获取到前端上报的预置位总数", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS preset info search response message process failed:front-end device ID=%s, cause= did not get total preset number from front-end", caller_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_preset_info_response_proc() exit---: Get Sun Num Error \r\n");
        return -1;
    }

    iSumNum = osip_atoi(strSumNum);
    iPresetConfigListNum = osip_atoi((char*)strPresetConfigListNum.c_str());
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_preset_info_response_proc() SumNum=%d, RecordListNum=%d \r\n", iSumNum, iPresetConfigListNum);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS预置位信息查询响应消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 上报的预置位总数=%d, 本次上报的预置位条数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, iSumNum, iPresetConfigListNum);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS preset info search response message:front-end device ID=%s, total preset number reported=%d, number of presets reported this time=%d", caller_id, iSumNum, iPresetConfigListNum);

    /* 看看是否是用户查询或者上级查询的 */
    transfer_xml_sn = strtoul(strSN, NULL, 10);
    xml_pos = transfer_xml_msg_find(XML_QUERY_GETPRESET, strDeviceID, transfer_xml_sn);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_preset_info_response_proc() transfer_xml_msg_find:Type=%d, DeviceID=%s, transfer_xml_sn=%d, xml_pos=%d \r\n", XML_QUERY_GETPRESET, strDeviceID, transfer_xml_sn, xml_pos);

    if (xml_pos >= 0)
    {
        i = transfer_xml_message_to_dest(xml_pos, iSumNum, iPresetConfigListNum, inPacket);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS预置位信息查询响应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=根据XML的SN转发给目的地失败, xml_pos=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS preset info search response message process failed:front-end device ID=%s, cause=forward to destination accorrding to XML SN failed", caller_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_preset_info_response_proc() transfer_xml_message_to_dest Error:device_id=%s\r\n", caller_id);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "前端设备预置位信息查询响应消息处理成功:前端设备ID=%s, IP地址=%s, 端口号=%d,根据XML的SN转发给目的地, xml_pos=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "A front-end equipment preset information query response message processing success: the front-end device ID = % s, = % s IP address, port number = % d, according to the XML SN forwarded to destination, xml_pos = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_preset_info_response_proc() transfer_xml_message_to_dest OK:device_id=%s\r\n", caller_id);
        }
    }
    else
    {
        SystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS预置位信息查询响应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=根据XML的SN查找目的地失败, transfer_xml_sn=%d, strDeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, transfer_xml_sn, strDeviceID);
        EnSystemLog(EV9000_CMS_GET_PRESET_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS preset info search response message process failed:front-end device ID=%s, cause=find destination accorrding to XML SN failed", caller_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_preset_info_response_proc() transfer_xml_message_to_dest Error:device_id=%s\r\n", caller_id);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : route_notify_alarm_proc
 功能描述  : 上级互联CMS发送过来的报警事件通知信息处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_notify_alarm_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strAlarmPriority[32] = {0};
    char strAlarmTime[32] = {0};
    char strAlarmMethod[32] = {0};
    CPacket outPacket;
    DOMElement* AccNode = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_alarm_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_alarm_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_alarm_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s\r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* 报警事件通知和分发
          控制流程见9.4.2

          命令包括如下字段:
          <!-- 命令类型：报警通知（必选） -->
          <element name="CmdType" fixed = "Alarm" />
          <!-- 命令序列号（必选） -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- 报警设备编码（必选）-->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- 报警级别（必选），1为一级警情，2为二级警情，3为三级警情，4为四级警情-->
          <element name="AlarmPriority" type="string" />
          <!-- 报警方式（必选），取值1为电话报警，2为设备报警，3为短信报警，4为GPS报警，5为视频报警，6
          为设备故障报警，7其它报警-->
          <element name= "AlarmMethod" type= "string" />
          <!--报警时间（必选）-->
          <element name= "AlarmTime" type="dateTime" />
          <!-- 经纬度信息可选 -->
          <element name="Longitude" type="double" minOccurs= "0"/>
          <element name="Latitude" type="double" minOccurs= "0"/>
          <!-- 扩展信息，可多项 -->
          <element name= "Info" minOccurs= "0" maxOccurs="unbounded">
          <restriction base= "string">
          <maxLength value= "1024" />
          </restriction>
          </element>
      */

    /* 取得报警数据 */
    inPacket.GetElementValue((char*)"SN", strSN);/* 命令序列号 */
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);/* 报警设备编码 */
    inPacket.GetElementValue((char*)"AlarmPriority", strAlarmPriority);/* 报警级别:1为 一级警情，2为二级警情，3为三级警情，4为四级警情 */
    inPacket.GetElementValue((char*)"AlarmTime", strAlarmTime);/* 报警时间 */
    inPacket.GetElementValue((char*)"AlarmMethod", strAlarmMethod);/* 报警方式: 取值1为电话报警，2为设备报警，3为短信报警，4为GPS报警，5为视频报警，6为设备故障报警，7其它报警 */

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_notify_alarm_proc() \
        \r\n XML Para: \
        \r\n SN=%s \
        \r\n DeviceID=%s \
        \r\n AlarmPriority=%s \
        \r\n AlarmTime=%s \
        \r\n AlarmMethod=%s \r\n", strSN, strDeviceID, strAlarmPriority, strAlarmTime, strAlarmMethod);

    /* 回复响应 */
    outPacket.SetRootTag("Response");
    AccNode = outPacket.CreateElement((char*)"CmdType");
    outPacket.SetElementValue(AccNode, (char*)"Alarm");
    AccNode = outPacket.CreateElement((char*)"SN");
    outPacket.SetElementValue(AccNode, strSN);
    AccNode = outPacket.CreateElement((char*)"DeviceID");
    outPacket.SetElementValue(AccNode, strDeviceID);
    AccNode = outPacket.CreateElement((char*)"Result");
    outPacket.SetElementValue(AccNode, (char*)"OK");

    /* 回应消息 */
    i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());

    if (i != 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_alarm_proc() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_alarm_proc() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_notify_keep_alive_proc
 功能描述  : 上级互联CMS发送过来的保活信息处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_notify_keep_alive_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    /* 设备状态信息报送消息
          控制流程见9.6.2
          目前没有从路由过来的告警需要处理

          命令包括如下字段:
          <!-- 命令类型：设备状态信息报送（必选） -->
          < element name="CmdType" fixed ="Keepalive" />
          <!-- 命令序列号（必选） -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- 源设备的设备编码（必选） -->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- 是否正常工作（必选） -->
          <element name="Status" type="tg:resultType" />
     */

    return 0;
}

/*****************************************************************************
 函 数 名  : route_notify_cms_restart_proc
 功能描述  : 上级互联CMS发送过来的重启命令
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年7月5日 星期六
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_notify_cms_restart_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int iRet = 0;
    char strSN[32] = {0};
    char strCMSID[32] = {0};

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_cms_restart_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    /* 取得 数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"CMSID", strCMSID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_notify_cms_restart_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n CMSID=%s \r\n", strSN, strCMSID);

    iRet = StopAllServiceTaskByCallerIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

    if (0 != iRet)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_cms_restart_proc() StopAllServiceTaskByCallerIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_cms_restart_proc() StopAllServiceTaskByCallerIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
    }

    /* 还有向上级CMS的请求 */
    iRet = StopAllServiceTaskByCalleeIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

    if (0 != iRet)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_cms_restart_proc() StopAllServiceTaskByCalleeIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_notify_cms_restart_proc() StopAllServiceTaskByCalleeIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : route_notify_catalog_proc
 功能描述  : 上级过来的点位变化通知消息处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月9日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_notify_catalog_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int logic_device_pos = -1;
    char strSN[32] = {0};
    char strDeviceIndex[64] = {0};
    char strDeviceID[32] = {0};
    char strEvent[32] = {0};
    char strGBLogicDeviceID[32] = {0};
    char strSumNum[16] = {0};
    string strDeviceListNum = "";
    int iSumNum = 0;
    int iDeviceListNum = 0;
    int iItemNumCount = 0;
    char strName[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strEnable[64] = {0};
    char strAlarmDeviceSubType[64] = {0};
    char strCtrlEnable[64] = {0};
    char strMicEnable[64] = {0};
    char strFrameCount[64] = {0};
    char strStreamCount[64] = {0};
    char strAlarmLengthOfTime[64] = {0};
    char strManufacturer[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strModel[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strOwner[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strCivilCode[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strBlock[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strAddress[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strParental[16] = {0};
    char strParentID[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strSafetyWay[16] = {0};
    char strRegisterWay[16] = {0};
    char strCertNum[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strCertifiable[16] = {0};
    char strErrCode[16] = {0};
    char strEndTime[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strSecrecy[16] = {0};
    char strIPAddress[MAX_IP_LEN] = {0};
    char strPort[16] = {0};
    char strPassword[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strStatus[16] = {0};
    char strLongitude[64] = {0};
    char strLatitude[64] = {0};
    char strMapLayer[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strCMSID[MAX_ID_LEN + 4] = {0};
    char strAlarmPriority[32] = {0};/*2016.10.10 add for RCU*/
    char strGuard[32] = {0};/*2016.10.10 add for RCU*/
    char strValue[256] = {0};/*2016.10.10 add for RCU*/
    char strUnit[32] = {0};/*2016.10.10 add for RCU*/
    char strPTZType[64] = {0};
    GBLogicDevice_info_t* pOldGBLogicDeviceInfo = NULL;
    GBLogicDevice_info_t* pNewGBLogicDeviceInfo = NULL;
    DOMElement* ItemAccNode = NULL;
    char strDeviceType[4] = {0};
    int iPTZType = 0;
    char* pTmp = NULL;
    //char* tmp_civil_code;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_catalog_proc() exit---: GBDevice Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_catalog_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* 网络设备信息查询响应消息直接转发，不做处理
          控制流程见9.5.2

          命令包括如下字段:
          <!-- 命令类型：设备目录查询（必选） -->
          <element name="CmdType" fixed ="Catalog" />
          <!-- 命令序列号（必选） -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- 目标设备的设备编码（必选） -->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- 设备目录项列表,num表示目录项个数 -->
          <element name="DeviceList">
          <attribute name="Num" type="integer"/>
          <choice minOccurs= "0" maxOccurs= " unbounded " >
          <element name="Item" type="tg:itemType"/>
          </choice>
          </element>
          <!-- 扩展信息，可多项 -->
          <element name= "Info" minOccurs= "0" maxOccurs="unbounded">
          <restriction base= "string">
          <maxLength value= "1024" />
          </restriction>
          </element>
      */

    /* 逻辑设备信息一般是设备注册成功之后CMS主动发起的查询 */
    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"SumNum", strSumNum);
    inPacket.GetElementAttr((char*)"DeviceList", (char*)"Num", strDeviceListNum);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_notify_catalog_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n DeviceID=%s \
    \r\n SumNum=%s \
    \r\n DeviceList Num=%s \r\n ", strSN, strDeviceID, strSumNum, (char*)strDeviceListNum.c_str());

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS推送点位消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS push point info :Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);

    if (strSumNum[0] == '\0')
    {
        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS推送点位消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=没有获取到前端上报的逻辑通道目录总数", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS push point info process failed:Superior CMS ID=%s, IP address=%s, port number=%d, cause=did not find totoal logic channel number reported by front-end.", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: Get Sun Num Error \r\n");
        return -1;
    }

    if (0 != sstrcmp(strDeviceID, pRouteInfo->server_id))
    {
        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS推送点位消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=前端设备的XML里面的设备ID错误, XML设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS push point info process failed:Superior CMS ID=%s, IP address=%s, port number=%d, cause=device ID in XML of front-end device incorrect, XML deviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: DeviceID Error:DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    /* 将设备信息写入标准逻辑设备表 */
    iSumNum = osip_atoi(strSumNum);
    iDeviceListNum = osip_atoi((char*)strDeviceListNum.c_str());
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() SumNum=%d, DeviceListNum=%d \r\n", iSumNum, iDeviceListNum);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS推送点位消息处理:上级CMS ID=%s, IP地址=%s, 端口号=%d:上报的逻辑设备总数=%d, 本次上报的逻辑设备个数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, iSumNum, iDeviceListNum);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS push point info process failed:superior CMS ID=%s, IPaddress=%s, port number=%d:total number of logic device reported=%d, number of logic device reported this time=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, iSumNum, iDeviceListNum);

    /* 查看被叫是否是本CMS ID */
    if (0 == strncmp(callee_id, local_cms_id_get(), 20))
    {
        if (iSumNum > 0)
        {
            if (iDeviceListNum <= 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: DeviceListNum Error \r\n");
                return -1;
            }

            /* 获取所有的Item 数据 */
            ItemAccNode = inPacket.SearchElement((char*)"Item");

            if (!ItemAccNode)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: Get Item Node Error \r\n");
                return -1;
            }

            inPacket.SetCurrentElement(ItemAccNode);

            while (ItemAccNode)
            {
                iItemNumCount++;

                if (iItemNumCount > iDeviceListNum)
                {
                    break;
                }

                memset(strDeviceIndex, 0, 64);
                inPacket.GetElementValue((char*)"ID", strDeviceIndex);

                memset(strGBLogicDeviceID, 0, 32);
                inPacket.GetElementValue((char*)"DeviceID", strGBLogicDeviceID);

                memset(strEvent, 0, 32);
                inPacket.GetElementValue((char*)"Event", strEvent);

                memset(strName, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"Name", strName);

                memset(strEnable, 0, 64);
                inPacket.GetElementValue((char*)"Enable", strEnable);

                memset(strCtrlEnable, 0, 64);
                inPacket.GetElementValue((char*)"CtrlEnable", strCtrlEnable);

                memset(strAlarmDeviceSubType, 0, 64);
                inPacket.GetElementValue((char*)"ChlType", strAlarmDeviceSubType);

                memset(strMicEnable, 0, 64);
                inPacket.GetElementValue((char*)"MicEnable", strMicEnable);

                memset(strFrameCount, 0, 64);
                inPacket.GetElementValue((char*)"FrameCount", strFrameCount);

                memset(strStreamCount, 0, 64);
                inPacket.GetElementValue((char*)"StreamCount", strStreamCount);

                memset(strAlarmLengthOfTime, 0, 64);
                inPacket.GetElementValue((char*)"AlarmLengthOfTime", strAlarmLengthOfTime);

                memset(strManufacturer, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"Manufacturer", strManufacturer);

                memset(strModel, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"Model", strModel);

                memset(strOwner, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"Owner", strOwner);

                memset(strCivilCode, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"CivilCode", strCivilCode);

                memset(strBlock, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"Block", strBlock);

                memset(strAddress, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"Address", strAddress);

                memset(strParental, 0, 16);
                inPacket.GetElementValue((char*)"Parental", strParental);

                memset(strParentID, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"ParentID", strParentID);

                memset(strSafetyWay, 0, 16);
                inPacket.GetElementValue((char*)"SafetyWay", strSafetyWay);

                memset(strRegisterWay, 0, 16);
                inPacket.GetElementValue((char*)"RegisterWay", strRegisterWay);

                memset(strCertNum, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"CertNum", strCertNum);

                memset(strCertifiable, 0, 16);
                inPacket.GetElementValue((char*)"Certifiable", strCertifiable);

                memset(strErrCode, 0, 16);
                inPacket.GetElementValue((char*)"ErrCode", strErrCode);

                memset(strEndTime, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"EndTime", strEndTime);

                memset(strSecrecy, 0, 16);
                inPacket.GetElementValue((char*)"Secrecy", strSecrecy);

                memset(strIPAddress, 0, 16);
                inPacket.GetElementValue((char*)"IPAddress", strIPAddress);

                memset(strPort, 0, 16);
                inPacket.GetElementValue((char*)"Port", strPort);

                memset(strPassword, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"Password", strPassword);

                memset(strStatus, 0, 16);
                inPacket.GetElementValue((char*)"Status", strStatus);

                memset(strLongitude, 0, 64);
                inPacket.GetElementValue((char*)"Longitude", strLongitude);

                memset(strLatitude, 0, 64);
                inPacket.GetElementValue((char*)"Latitude", strLatitude);

                memset(strMapLayer, 0, MAX_128CHAR_STRING_LEN + 4);
                inPacket.GetElementValue((char*)"MapLayer", strMapLayer);

                memset(strCMSID, 0, 24);
                inPacket.GetElementValue((char*)"CMSID", strCMSID);

#if 1 /*2016.10.10 add for RCU*/
                memset(strValue, 0, 256);
                inPacket.GetElementValue((char*)"Value", strValue);

                memset(strUnit, 0, 32);
                inPacket.GetElementValue((char*)"Unit", strUnit);

                memset(strGuard, 0, 32);
                inPacket.GetElementValue((char*)"Guard", strGuard);

                memset(strAlarmPriority, 0, 32);
                inPacket.GetElementValue((char*)"AlarmPriority", strAlarmPriority);
#endif/*2016.10.10 add for RCU*/

                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_notify_catalog_proc() ItemCount=%d, GBLogicDeviceID=%s, Name=%s, Event=%s, Status=%s \r\n", iItemNumCount, strGBLogicDeviceID, strName, strEvent, strStatus);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS推送点位消息:上报的逻辑设备ID=%s, 逻辑点位名称=%s, 事件=%s, 状态=%s", strGBLogicDeviceID, strName, strEvent, strStatus);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS push point info:Reported logic device ID = % s, logic point name = % s, event = % s, state = % s", strGBLogicDeviceID, strName, strEvent, strStatus);

                if (strGBLogicDeviceID[0] == '\0')
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: GBLogic Device ID Error \r\n");
                    ItemAccNode = inPacket.SearchNextElement(true);
                    continue;
                }

                if (20 != strlen(strGBLogicDeviceID))
                {
                    SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS推送点位信息:上报的逻辑设备ID=%s, 逻辑点位名称=%s, 逻辑设备ID长度不合法, 逻辑设备ID长度=%d", strGBLogicDeviceID, strName, strlen(strGBLogicDeviceID));
                    EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Superior CMS push point info:Logic device ID reported=%s, logic point name=%s, logice device ID length is not valid, logic device ID length=%d", strGBLogicDeviceID, strName, strlen(strGBLogicDeviceID));
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: GBLogic Device ID=%s, Length=%d Error \r\n", strGBLogicDeviceID, strlen(strGBLogicDeviceID));
                    ItemAccNode = inPacket.SearchNextElement(true);
                    continue;
                }

                if (sstrcmp(strEvent, "ADD") == 0 || sstrcmp(strEvent, "UPDATE") == 0)
                {
                    /* 查找旧的逻辑设备,看旧的逻辑设备是否是本级的逻辑设备 */
                    pOldGBLogicDeviceInfo = GBLogicDevice_info_find(strGBLogicDeviceID);

                    if (NULL != pOldGBLogicDeviceInfo)
                    {
                        /* 如果下发的点位，在本级CMS中可以找到，并且也是非其他域的点位，则不处理,默认还是本域的点位 */
                        if (0 == pOldGBLogicDeviceInfo->other_realm)
                        {
                            SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS推送点位信息:上报的逻辑设备ID=%s, 逻辑点位名称=%s, 该逻辑设备是本级CMS的逻辑设备", strGBLogicDeviceID, strName);
                            EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Superior CMS push point info:Logic device ID reported=%s, logic point name=%s, this logic device is parallel CMS logic device", strGBLogicDeviceID, strName);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: GBLogic Device ID=%s, is Local Realm LogicDevice \r\n", strGBLogicDeviceID);
                            ItemAccNode = inPacket.SearchNextElement(true);
                            continue;
                        }
                    }

                    /* 将信息写入新的结构 */
                    i = GBLogicDevice_info_init(&pNewGBLogicDeviceInfo);

                    if (i != 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: GBLogicDevice Info Init Error \r\n");
                        ItemAccNode = inPacket.SearchNextElement(true);
                        continue;
                    }

                    /* 点位统一编号 */
                    if (strGBLogicDeviceID[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->device_id, strGBLogicDeviceID, MAX_ID_LEN);
                    }

                    /* 逻辑设备索引 */
                    if (strDeviceIndex[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->id = strtoul(strDeviceIndex, NULL, 10);
                    }
                    else
                    {
                        if ('\0' != pNewGBLogicDeviceInfo->device_id[0])
                        {
                            pNewGBLogicDeviceInfo->id = crc32((unsigned char*)pNewGBLogicDeviceInfo->device_id, MAX_ID_LEN);
                        }
                    }

                    /* 所属的CMS统一编号，如果是下级CMS上报的，则存在该数据，如果是具体物理设备，填本级CMSID */
                    if (strCMSID[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->cms_id, strCMSID, MAX_ID_LEN);
                    }
                    else
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->cms_id, local_cms_id_get(), MAX_ID_LEN);
                    }

                    /* 点位名称 */
                    if (strName[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->device_name, strName, MAX_128CHAR_STRING_LEN);
                    }

                    /* 设备类型 */
                    if ('\0' != pNewGBLogicDeviceInfo->device_id[0])
                    {
                        pTmp = &pNewGBLogicDeviceInfo->device_id[10];
                        osip_strncpy(strDeviceType, pTmp, 3);
                        pNewGBLogicDeviceInfo->device_type = osip_atoi(strDeviceType);
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->device_type = EV9000_DEVICETYPE_IPC;
                    }

                    /* 是否启用 */
                    if (strEnable[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->enable = osip_atoi(strEnable);
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->enable = 1;
                    }

                    /* 是否可控 */
                    if (strPTZType[0] != '\0') /* 国标新增扩展协议, 1-球机；2-半球；3-固定枪机； 4-遥控枪机 */
                    {
                        iPTZType = osip_atoi(strPTZType);

                        pNewGBLogicDeviceInfo->ctrl_enable = iPTZType;
                    }
                    else
                    {
                        if (strCtrlEnable[0] != '\0')
                        {
                            if (0 == sstrcmp((char*)"Disable", strCtrlEnable))
                            {
                                pNewGBLogicDeviceInfo->ctrl_enable = 0;
                            }
                            else if (0 == sstrcmp((char*)"Enable", strCtrlEnable))
                            {
                                pNewGBLogicDeviceInfo->ctrl_enable = 1;
                            }
                            else
                            {
                                pNewGBLogicDeviceInfo->ctrl_enable = 0;
                            }
                        }
                        else
                        {
                            pNewGBLogicDeviceInfo->ctrl_enable = 0;
                        }
                    }

                    /* 是否支持对讲 */
                    if (strMicEnable[0] != '\0')
                    {
                        if (0 == sstrcmp((char*)"Disable", strMicEnable))
                        {
                            pNewGBLogicDeviceInfo->mic_enable = 0;
                        }
                        else if (0 == sstrcmp((char*)"Enable", strMicEnable))
                        {
                            pNewGBLogicDeviceInfo->mic_enable = 1;
                        }
                        else
                        {
                            pNewGBLogicDeviceInfo->mic_enable = 0;
                        }
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->mic_enable = 0;
                    }

                    /* 帧率 */
                    if (strFrameCount[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->frame_count = osip_atoi(strFrameCount);
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->frame_count = 25;
                    }

                    /* 是否支持多码流 */
                    if (strStreamCount[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->stream_count = osip_atoi(strStreamCount);
                    }
                    else
                    {
                        if (pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_CAMERA
                            || pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_IPC
                            || pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_SCREEN
                            || pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_CODER)
                        {
                            pNewGBLogicDeviceInfo->stream_count = 1;
                        }
                        else
                        {
                            pNewGBLogicDeviceInfo->stream_count = 0;
                        }
                    }

                    /* 帧率 */
                    if (strAlarmLengthOfTime[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->alarm_duration = osip_atoi(strAlarmLengthOfTime);
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->alarm_duration = 0;
                    }

                    /* 是否属于其他域 */
                    pNewGBLogicDeviceInfo->other_realm = 1;

                    /* 设备生产商 */
                    if (strManufacturer[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->manufacturer, strManufacturer, MAX_128CHAR_STRING_LEN);
                    }

                    /* 设备型号 */
                    if (strModel[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->model, strModel, MAX_128CHAR_STRING_LEN);
                    }

                    /* 设备归属 */
                    if (strOwner[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->owner, strOwner, MAX_128CHAR_STRING_LEN);
                    }

                    /* 警区 */
                    if (strBlock[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->block, strBlock, MAX_128CHAR_STRING_LEN);
                    }

                    /* 安装地址 */
                    if (strAddress[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->address, strAddress, MAX_128CHAR_STRING_LEN);
                    }

                    /* 是否有子设备 */
                    if (strParental[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->parental = osip_atoi(strParental);
                    }

                    /* 父设备/区域/系统ID */
                    if (strParentID[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->parentID, strParentID, MAX_128CHAR_STRING_LEN);
                    }

                    /* 信令安全模式*/
                    if (strSafetyWay[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->safety_way = osip_atoi(strSafetyWay);
                    }

                    /* 注册方式 */
                    if (strRegisterWay[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->register_way = osip_atoi(strRegisterWay);
                    }

                    /* 证书序列号*/
                    if (strCertNum[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->cert_num, strCertNum, MAX_128CHAR_STRING_LEN);
                    }

                    /* 证书有效标识 */
                    if (strCertifiable[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->certifiable = osip_atoi(strCertifiable);
                    }

                    /* 无效原因码 */
                    if (strErrCode[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->error_code = osip_atoi(strErrCode);
                    }

                    /* 证书终止有效期*/
                    if (strEndTime[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->end_time, strEndTime, MAX_128CHAR_STRING_LEN);
                    }

                    /* 保密属性 */
                    if (strSecrecy[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->secrecy = osip_atoi(strSecrecy);
                    }

                    /* IP地址*/
                    if (strIPAddress[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->ip_address, strIPAddress, MAX_IP_LEN);
                    }
                    else
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->ip_address, pRouteInfo->server_ip, MAX_IP_LEN);
                    }

                    /* 端口号*/
                    if (strPort[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->port = osip_atoi(strPort);
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->port = pRouteInfo->server_port;
                    }

                    /* 密码*/
                    if (strPassword[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->password, strPassword, MAX_128CHAR_STRING_LEN);
                    }

                    /* 点位状态 */
                    if (strStatus[0] != '\0' && (0 == sstrcmp(strStatus, (char*)"ON") || 0 == sstrcmp(strStatus, (char*)"ONLINE")))
                    {
                        pNewGBLogicDeviceInfo->status = 1;
                    }
                    else if (strStatus[0] != '\0' && (0 == sstrcmp(strStatus, (char*)"OFF") || 0 == sstrcmp(strStatus, (char*)"OFFLINE")))
                    {
                        pNewGBLogicDeviceInfo->status = 0;
                        pNewGBLogicDeviceInfo->intelligent_status = INTELLIGENT_STATUS_NULL;
                        pNewGBLogicDeviceInfo->alarm_status = ALARM_STATUS_NULL;
                    }
                    else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"NOVIDEO"))
                    {
                        pNewGBLogicDeviceInfo->status = 2;
                    }
                    else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"VLOST"))
                    {
                        pNewGBLogicDeviceInfo->status = 2;
                    }
                    else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"INTELLIGENT"))
                    {
                        pNewGBLogicDeviceInfo->status = 1;
                        pNewGBLogicDeviceInfo->intelligent_status = INTELLIGENT_STATUS_ON;
                    }
                    else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"CLOSE"))
                    {
                        pNewGBLogicDeviceInfo->status = 1;
                        pNewGBLogicDeviceInfo->alarm_status = ALARM_STATUS_CLOSE;
                    }
                    else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"APART"))
                    {
                        pNewGBLogicDeviceInfo->status = 1;
                        pNewGBLogicDeviceInfo->alarm_status = ALARM_STATUS_APART;
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->status = 0;
                    }

                    /* 经度 */
                    if (strLongitude[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->longitude = strtod(strLongitude, (char**) NULL);
                    }

                    /* 纬度 */
                    if (strLatitude[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->latitude = strtod(strLatitude, (char**) NULL);
                    }

                    /* 所属图层 */
                    if (strMapLayer[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->map_layer, strMapLayer, MAX_128CHAR_STRING_LEN);
                    }

                    /* 报警设备子类型 */
                    if (strAlarmDeviceSubType[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->alarm_device_sub_type = osip_atoi(strAlarmDeviceSubType);
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->alarm_device_sub_type = 0;
                    }

#if 1 /*2016.10.10 add for RCU*/

                    /* Guard*/
                    if (strGuard[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->guard_type = osip_atoi(strGuard);
                    }

                    /* 报警级别 */
                    if (strAlarmPriority[0] != '\0')
                    {
                        pNewGBLogicDeviceInfo->AlarmPriority = osip_atoi(strAlarmPriority);
                    }

                    /* 单位 */
                    if (strUnit[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->Unit, strUnit, 32);
                    }

                    /* Value */
                    if (strValue[0] != '\0')
                    {
                        osip_strncpy(pNewGBLogicDeviceInfo->Value, strValue, 256);
                    }

#endif /*2016.10.10 add for RCU*/

                    /* 根据旧的逻辑设备判断 */
                    if (NULL != pOldGBLogicDeviceInfo)
                    {
                        i = GBLogicDeviceCatalogInfoProcForRoute(pNewGBLogicDeviceInfo, pOldGBLogicDeviceInfo, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() GBLogicDeviceCatalogInfoProcForRoute Error:i=%d \r\n", i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() GBLogicDeviceCatalogInfoProcForRoute OK:iRet=%d \r\n", i);
                        }

                        GBLogicDevice_info_free(pNewGBLogicDeviceInfo);
                        osip_free(pNewGBLogicDeviceInfo);
                        pNewGBLogicDeviceInfo = NULL;
                    }
                    else
                    {
                        /* 添加逻辑设备信息 */
                        logic_device_pos = GBLogicDevice_info_add(pNewGBLogicDeviceInfo);

                        //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() GBLogicDevice_info_add:logic_device_pos=%d \r\n", logic_device_pos);

                        if (logic_device_pos < 0)
                        {
                            GBLogicDevice_info_free(pNewGBLogicDeviceInfo);
                            osip_free(pNewGBLogicDeviceInfo);
                            pNewGBLogicDeviceInfo = NULL;
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: GBLogicDevice Info Add Error \r\n");
                            ItemAccNode = inPacket.SearchNextElement(true);
                            continue;
                        }

                        /* 发送添加消息给下级CMS  */
                        i = SendNotifyCatalogToSubCMS(pNewGBLogicDeviceInfo, 0, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() SendNotifyCatalogToSubCMS Error:iRet=%d \r\n", i);
                        }
                        else if (i > 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() SendNotifyCatalogToSubCMS OK:iRet=%d \r\n", i);
                        }

                        /* 更新数据库 */
                        i = AddGBLogicDeviceInfo2DBForRoute(strGBLogicDeviceID, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute ERROR:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute OK:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                        }
                    }
                }
                else if (sstrcmp(strEvent, "DEL") == 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级CMS推送点位删除消息:上报的逻辑设备ID=%s, 逻辑点位名称=%s", strGBLogicDeviceID, strName);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Superior CMS push point delete message:Logic device ID reported=%s, logic point name=%s", strGBLogicDeviceID, strName);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() : DeviceID=%s, DeviceName=%s, Notify Event=DEL \r\n", strGBLogicDeviceID, strName);

                    /* 查找旧的逻辑设备 */
                    pOldGBLogicDeviceInfo = GBLogicDevice_info_find(strGBLogicDeviceID);

                    if (NULL == pOldGBLogicDeviceInfo)
                    {
                        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS推送点位删除消息:上报的逻辑设备ID=%s, 逻辑点位名称=%s, 没有找到对应的逻辑设备信息", strGBLogicDeviceID, strName);
                        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS push point delete message:Logic device ID reported=%s, logic point name=%s, Corresponding logic device not found", strGBLogicDeviceID, strName);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: Find GBLogicDevice Info Error \r\n");
                        ItemAccNode = inPacket.SearchNextElement(true);
                        continue;
                    }

                    if (0 == pOldGBLogicDeviceInfo->enable)
                    {
                        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS推送点位删除消息:上报的逻辑设备ID=%s, 逻辑点位名称=%s, 上报的点位已经被禁用", strGBLogicDeviceID, strName);
                        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Superior CMS push point delete message:Logic device ID reported=%s, logic point name=%s, point reported is disabled", strGBLogicDeviceID, strName);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: Find GBLogicDevice Info Error \r\n");
                        ItemAccNode = inPacket.SearchNextElement(true);
                        continue;
                    }

                    pOldGBLogicDeviceInfo->enable = 0;
                    pOldGBLogicDeviceInfo->status = 0;

                    /* 发送设备状态消息给客户端 */
                    i = SendDeviceStatusToAllClientUser(pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, pRoute_Srv_dboper);

                    if (i < 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() SendDeviceStatusToAllClientUser Error:iRet=%d \r\n", i);
                    }
                    else if (i > 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() SendDeviceStatusToAllClientUser OK:iRet=%d \r\n", i);
                    }

                    /* 发送删除消息给下级CMS  */
                    i = SendNotifyCatalogToSubCMS(pOldGBLogicDeviceInfo, 1, pRoute_Srv_dboper);

                    if (i < 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() SendNotifyCatalogToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, i);
                    }
                    else if (i > 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() SendNotifyCatalogToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, i);
                    }

                    /* 发送告警信息到客户端 */
                    i = SendDeviceOffLineAlarmToAllClientUser(pOldGBLogicDeviceInfo->device_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() SendDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() SendDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                    }

                    /* 停止呼叫任务 */
                    i = StopAllServiceTaskByLogicDeviceID(pOldGBLogicDeviceInfo->device_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() StopAllServiceTaskByLogicDeviceID ERROR:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() StopAllServiceTaskByLogicDeviceID OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                    }

                    /* 停止音频对讲业务 */
                    i = StopAudioServiceTaskByLogicDeviceID(pOldGBLogicDeviceInfo->device_id);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() StopAudioServiceTaskByLogicDeviceID Error:DeviceID=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() StopAudioServiceTaskByLogicDeviceID OK:DeviceID=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                    }

                    /* 更新数据库 */
                    i = AddGBLogicDeviceInfo2DBForRoute(strGBLogicDeviceID, pRoute_Srv_dboper);

                    if (i < 0)
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute ERROR:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute OK:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                    }
                }
                else if (sstrcmp(strEvent, "ON") == 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS推送点位上线消息:上报的逻辑设备ID=%s, 逻辑点位名称=%s", strGBLogicDeviceID, strName);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS push point online message:Logic device ID reported=%s, logic point name=%s", strGBLogicDeviceID, strName);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() : DeviceID=%s, DeviceName=%s, Notify Event=OFF \r\n", strGBLogicDeviceID, strName);

                    /* 查找旧的逻辑设备 */
                    pOldGBLogicDeviceInfo = GBLogicDevice_info_find(strGBLogicDeviceID);

                    if (NULL == pOldGBLogicDeviceInfo)
                    {
                        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS推送点位上线消息:上报的逻辑设备ID=%s, 逻辑点位名称=%s, 没有找到对应的逻辑设备信息", strGBLogicDeviceID, strName);
                        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS push point online message:Logic device ID reported=%s, logic point name=%s, Corresponding logic device not found", strGBLogicDeviceID, strName);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: Find GBLogicDevice Info Error \r\n");
                        ItemAccNode = inPacket.SearchNextElement(true);
                        continue;
                    }

                    if (0 == pOldGBLogicDeviceInfo->enable)
                    {
                        pOldGBLogicDeviceInfo->enable = 1;
                        pOldGBLogicDeviceInfo->status = 1;
                    }

                    if (pOldGBLogicDeviceInfo->status == 0)
                    {
                        pOldGBLogicDeviceInfo->status = 1;

                        /* 发送设备状态消息给客户端 */
                        i = SendDeviceStatusToAllClientUser(pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() SendDeviceStatusToAllClientUser Error:iRet=%d \r\n", i);
                        }
                        else if (i > 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() SendDeviceStatusToAllClientUser OK:iRet=%d \r\n", i);
                        }

                        /* 发送设备状态消息给下级CMS  */
                        i = SendDeviceStatusToSubCMS(pOldGBLogicDeviceInfo, pOldGBLogicDeviceInfo->status, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, i);
                        }
                        else if (i > 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, i);
                        }

                        /* 更新数据库 */
                        i = AddGBLogicDeviceInfo2DBForRoute(strGBLogicDeviceID, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute ERROR:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute OK:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                        }
                    }
                }
                else if (sstrcmp(strEvent, "OFF") == 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级CMS推送点位下线消息:上报的逻辑设备ID=%s, 逻辑点位名称=%s", strGBLogicDeviceID, strName);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Superior CMS push point offline message:Logic device ID reported=%s, logic point name=%s", strGBLogicDeviceID, strName);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() : DeviceID=%s, DeviceName=%s, Notify Event=OFF \r\n", strGBLogicDeviceID, strName);

                    /* 查找旧的逻辑设备 */
                    pOldGBLogicDeviceInfo = GBLogicDevice_info_find(strGBLogicDeviceID);

                    if (NULL == pOldGBLogicDeviceInfo)
                    {
                        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS推送点位下线消息:上报的逻辑设备ID=%s, 逻辑点位名称=%s, 没有找到对应的逻辑设备信息", strGBLogicDeviceID, strName);
                        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS push point offline message:Logic device ID reported=%s, logic point name=%s, Corresponding logic device not found", strGBLogicDeviceID, strName);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: Find GBLogicDevice Info Error \r\n");
                        ItemAccNode = inPacket.SearchNextElement(true);
                        continue;
                    }

                    if (0 == pOldGBLogicDeviceInfo->enable)
                    {
                        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS推送点位下线消息:上报的逻辑设备ID=%s, 逻辑点位名称=%s, 上报的点位已经被禁用", strGBLogicDeviceID, strName);
                        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Superior CMS push point offline message:Logic device ID reported=%s, logic point name=%s, point reported is disabled", strGBLogicDeviceID, strName);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() exit---: Find GBLogicDevice Info Error \r\n");
                        ItemAccNode = inPacket.SearchNextElement(true);
                        continue;
                    }

                    if (pOldGBLogicDeviceInfo->status == 1)
                    {
                        pOldGBLogicDeviceInfo->status = 0;

                        /* 发送设备状态消息给客户端 */
                        i = SendDeviceStatusToAllClientUser(pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_catalog_proc() SendDeviceStatusToAllClientUser Error:iRet=%d \r\n", i);
                        }
                        else if (i > 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_catalog_proc() SendDeviceStatusToAllClientUser OK:iRet=%d \r\n", i);
                        }

                        /* 发送设备状态消息给下级CMS  */
                        i = SendDeviceStatusToSubCMS(pOldGBLogicDeviceInfo, pOldGBLogicDeviceInfo->status, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, i);
                        }
                        else if (i > 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, pOldGBLogicDeviceInfo->status, i);
                        }

                        /* 发送告警信息到客户端 */
                        i = SendDeviceOffLineAlarmToAllClientUser(pOldGBLogicDeviceInfo->device_id);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() SendDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() SendDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                        }

                        /* 停止呼叫任务 */
                        i = StopAllServiceTaskByLogicDeviceID(pOldGBLogicDeviceInfo->device_id);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() StopAllServiceTaskByLogicDeviceID ERROR:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() StopAllServiceTaskByLogicDeviceID OK:device_id=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                        }

                        /* 停止音频对讲业务 */
                        i = StopAudioServiceTaskByLogicDeviceID(pOldGBLogicDeviceInfo->device_id);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() StopAudioServiceTaskByLogicDeviceID Error:DeviceID=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() StopAudioServiceTaskByLogicDeviceID OK:DeviceID=%s, iRet=%d \r\n", pOldGBLogicDeviceInfo->device_id, i);
                        }

                        /* 更新数据库 */
                        i = AddGBLogicDeviceInfo2DBForRoute(strGBLogicDeviceID, pRoute_Srv_dboper);

                        if (i < 0)
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute ERROR:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_catalog_proc() AddGBLogicDeviceInfo2DBForRoute OK:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                        }
                    }
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "route_notify_catalog_proc() Event Error:Event=%s \r\n", strEvent);
                }

                ItemAccNode = inPacket.SearchNextElement(true);
            }
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_notify_status_proc
 功能描述  : 收到上级过来的设备状态通知消息处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月12日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_notify_status_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strStatus[32] = {0};
    int iOldDeviceStatus = 0;
    intelligent_status_t iOldIntelligentDeviceStatus = INTELLIGENT_STATUS_NULL;
    alarm_status_t iOldAlarmDeviceStatus = ALARM_STATUS_NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_status_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_notify_status_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* 设备状态信息报送消息*/
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS通知设备状态消息:上级CMS ID=%, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS notify device status message:Superior CMS, ID = % = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    /* 查看被叫是否是本CMS ID */
    if (0 != strncmp(callee_id, local_cms_id_get(), 20))
    {
        SystemLog(EV9000_CMS_NOTIFY_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS通知设备状态消息处理失败:上级CMS ID=%, IP地址=%s, 端口号=%d, 原因=CMS ID不属于本CMS: CMS ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
        EnSystemLog(EV9000_CMS_NOTIFY_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS notify device status message process failed:superior CMS ID=%, cause=CMS ID does not belong to localCMS: CMS ID=%s", caller_id, callee_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_status_proc() exit---: Not Belong To Mine:callee_id=%s \r\n", callee_id);
        return -1;
    }

    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"Status", strStatus);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_notify_status_proc() \
    \r\n XML Para: \
    \r\n SN=%s, DeviceID=%s, Status=%s \r\n", strSN, strDeviceID, strStatus);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS通知设备状态消息处理:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 状态=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, strStatus);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS notify device status message process:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, state = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, strStatus);

    pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

    if (NULL == pGBLogicDeviceInfo)
    {
        SystemLog(EV9000_CMS_NOTIFY_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS通知设备状态消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=获取逻辑设备信息失败: DeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
        EnSystemLog(EV9000_CMS_NOTIFY_STATUS_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS notify device status message process failed:superior CMS ID=%s, cause=access logice device info failed: DeviceID=%s", caller_id, strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_notify_status_proc() exit---: Find GBLogic Device Info Error:DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    iOldDeviceStatus = pGBLogicDeviceInfo->status;
    iOldIntelligentDeviceStatus = pGBLogicDeviceInfo->intelligent_status;
    iOldAlarmDeviceStatus = pGBLogicDeviceInfo->alarm_status;

    if (0 == sstrcmp(strStatus, (char*)"ON") || sstrcmp(strStatus, (char*)"ONLINE") == 0)
    {
        pGBLogicDeviceInfo->status = 1;
    }
    else if (0 == sstrcmp(strStatus, (char*)"OFF") || sstrcmp(strStatus, (char*)"OFFLINE") == 0)
    {
        pGBLogicDeviceInfo->status = 0;
        pGBLogicDeviceInfo->intelligent_status = INTELLIGENT_STATUS_NULL;
        pGBLogicDeviceInfo->alarm_status = ALARM_STATUS_NULL;
    }
    else if (sstrcmp(strStatus, (char*)"NOVIDEO") == 0)
    {
        pGBLogicDeviceInfo->status = 2;
    }
    else if (sstrcmp(strStatus, (char*)"VLOST") == 0)
    {
        pGBLogicDeviceInfo->status = 2;
    }
    else if (sstrcmp(strStatus, (char*)"INTELLIGENT") == 0)
    {
        pGBLogicDeviceInfo->status = 1;
        pGBLogicDeviceInfo->intelligent_status = INTELLIGENT_STATUS_ON;
    }
    else if (sstrcmp(strStatus, (char*)"CLOSE") == 0)
    {
        pGBLogicDeviceInfo->status = 1;
        pGBLogicDeviceInfo->alarm_status = ALARM_STATUS_CLOSE;
    }
    else if (sstrcmp(strStatus, (char*)"APART") == 0)
    {
        pGBLogicDeviceInfo->status = 1;
        pGBLogicDeviceInfo->alarm_status = ALARM_STATUS_APART;
    }

    /* 查看状态是否有变化 */
    if (pGBLogicDeviceInfo->status != iOldDeviceStatus
        || iOldIntelligentDeviceStatus != pGBLogicDeviceInfo->intelligent_status
        || iOldAlarmDeviceStatus != pGBLogicDeviceInfo->alarm_status)
    {
        if (pGBLogicDeviceInfo->status != iOldDeviceStatus)
        {
            /* 更新数据库 */
            i = UpdateGBLogicDeviceRegStatus2DB(pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() UpdateGBLogicDeviceRegStatus2DB Error:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() UpdateGBLogicDeviceRegStatus2DB OK:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, i);
            }
        }

        /* 发送设备状态消息给客户端 */
        if (1 == pGBLogicDeviceInfo->status && INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
        {
            i = SendDeviceStatusToAllClientUser(pGBLogicDeviceInfo->device_id, 4, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToAllClientUser Error:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }

            /* 发送设备状态消息给下级CMS  */
            i = SendDeviceStatusToSubCMS(pGBLogicDeviceInfo, 4, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
        }
        else if (1 == pGBLogicDeviceInfo->status && ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
        {
            i = SendDeviceStatusToAllClientUser(pGBLogicDeviceInfo->device_id, 5, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToAllClientUser Error:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, 5, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, 5, i);
            }

            /* 发送设备状态消息给下级CMS  */
            i = SendDeviceStatusToSubCMS(pGBLogicDeviceInfo, 5, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
        }
        else if (1 == pGBLogicDeviceInfo->status && ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
        {
            i = SendDeviceStatusToAllClientUser(pGBLogicDeviceInfo->device_id, 6, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToAllClientUser Error:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, 6, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, 6, i);
            }

            /* 发送设备状态消息给下级CMS  */
            i = SendDeviceStatusToSubCMS(pGBLogicDeviceInfo, 6, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
        }
        else
        {
            i = SendDeviceStatusToAllClientUser(pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToAllClientUser Error:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToAllClientUser OK:device_id=%s, status=%d, i=%d \r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status, i);
            }

            /* 发送设备状态消息给下级CMS  */
            i = SendDeviceStatusToSubCMS(pGBLogicDeviceInfo, pGBLogicDeviceInfo->status, pRoute_Srv_dboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceStatusToSubCMS ERROR:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
            else if (i > 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceStatusToSubCMS OK:device_id=%s, status=%d, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, 4, i);
            }
        }

        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_notify_status_proc() UpdateGBLogicDeviceRegStatus2DB:device_id=%s,status=%d\r\n", pGBLogicDeviceInfo->device_id, pGBLogicDeviceInfo->status);
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级CMS通知设备状态消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 老状态=%d, 新状态=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, pGBLogicDeviceInfo->device_id, iOldDeviceStatus, pGBLogicDeviceInfo->status);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Superior CMS notify device status message :logice device ID=%s, old state=%d, new state=%d", pGBLogicDeviceInfo->device_id, iOldDeviceStatus, pGBLogicDeviceInfo->status);
    }

    if (iOldDeviceStatus == 1 && (pGBLogicDeviceInfo->status == 0 || pGBLogicDeviceInfo->status == 2))
    {
        if (iOldDeviceStatus == 1 && pGBLogicDeviceInfo->status == 0)
        {
            /* 发送告警信息到客户端 */
            i = SendDeviceOffLineAlarmToAllClientUser(pGBLogicDeviceInfo->device_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
            }
        }
        else if (iOldDeviceStatus == 1 && pGBLogicDeviceInfo->status == 2)
        {
            /* 发送告警信息到客户端 */
            i = SendDeviceNoStreamAlarmToAllClientUser(pGBLogicDeviceInfo->device_id);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendDeviceNoStreamAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendDeviceNoStreamAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
            }
        }

        i = StopAllServiceTaskByLogicDeviceID(pGBLogicDeviceInfo->device_id);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() StopAllServiceTaskByLogicDeviceID Error:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() StopAllServiceTaskByLogicDeviceID OK:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }

        /* 停止音频对讲业务 */
        i = StopAudioServiceTaskByLogicDeviceID(pGBLogicDeviceInfo->device_id);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() StopAudioServiceTaskByLogicDeviceID Error:DeviceID=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() StopAudioServiceTaskByLogicDeviceID OK:DeviceID=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }
    }
    else if (INTELLIGENT_STATUS_ON == iOldIntelligentDeviceStatus && INTELLIGENT_STATUS_NULL == pGBLogicDeviceInfo->intelligent_status)
    {
        /* 发送告警信息到客户端 */
        i = SendIntelligentDeviceOffLineAlarmToAllClientUser(pGBLogicDeviceInfo->device_id);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() SendIntelligentDeviceOffLineAlarmToAllClientUser Error:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() SendIntelligentDeviceOffLineAlarmToAllClientUser OK:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }

        i = StopAllServiceTaskByLogicDeviceIDAndStreamType(pGBLogicDeviceInfo->device_id, EV9000_STREAM_TYPE_INTELLIGENCE);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_notify_status_proc() StopAllServiceTaskByLogicDeviceIDAndStreamType Error:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_notify_status_proc() StopAllServiceTaskByLogicDeviceIDAndStreamType OK:device_id=%s, iRet=%d \r\n", pGBLogicDeviceInfo->device_id, i);
        }
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "前端设备通知设备状态消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Front-end equipment inform status message processing success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    return 0;
}

/*****************************************************************************
 函 数 名  : route_device_control_response_proc
 功能描述  : 上级互联CMS发送过来的设备控制响应处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_device_control_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    int i = 0;
    int xml_pos = -1;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    unsigned int transfer_xml_sn = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_response_proc() exit---: GBDevice Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_device_control_response_proc() exit---: Param Error \r\n");
        return -1;
    }

    /* 设备控制的响应命令直接转发，不做处理
          控制流程见9.3.2

          命令包括如下字段:
          <!-- 命令类型：设备控制（必选） -->
          <element name="CmdType" fixed ="DeviceControl" />
          <!-- 命令序列号（必选） -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- 目标设备编码（必选） -->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- 执行结果标志（必选） -->
          <element name="Result" type="tg:resultType" />
      */

    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_device_control_response_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n DeviceID=%s \r\n ", strSN, strDeviceID);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS设备控制响应消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Front-end equipment control response message: front-end equipment, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);

    if (strSN[0] == '\0')
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_device_control_response_proc() exit---: Get XML SN Error\r\n");
        return -1;
    }

    if (strDeviceID[0] == '\0')
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "route_device_control_response_proc() exit---: Get XML DeviceID Error\r\n");
        return -1;
    }

    /* 看看是否是用户查询或者上级查询的 */
    transfer_xml_sn = strtoul(strSN, NULL, 10);
    xml_pos = transfer_xml_msg_find(XML_CONTROL_DEVICECONTROL, strDeviceID, transfer_xml_sn);

    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "route_device_control_response_proc() transfer_xml_msg_find:Type=%d, DeviceID=%s, transfer_xml_sn=%d, xml_pos=%d \r\n", XML_CONTROL_DEVICECONTROL, strDeviceID, transfer_xml_sn, xml_pos);

    if (xml_pos >= 0)
    {
        i = transfer_xml_message_to_dest2(xml_pos, inPacket);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS控制响应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=根据XML的SN转发给目的地失败, xml_pos=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "The front-end device control response message processing failed: the front-end device ID=%s, the reason = SN XML forwarding to the destination failed", strDeviceID);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "route_device_control_response_proc() transfer_xml_message_to_dest Error:device_id=%s\r\n", caller_id);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS控制响应消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d,根据XML的SN转发给目的地, xml_pos=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Front-end equipment control response message processing success: the front-end device ID = % s, = % s IP address, port number = % d, according to the XML SN forwarded to destination, xml_pos = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "route_device_control_response_proc() transfer_xml_message_to_dest OK:device_id=%s\r\n", caller_id);
        }
    }
    else
    {
        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS控制响应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=根据XML的SN查找目的地失败, transfer_xml_sn=%d, strDeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, transfer_xml_sn, strDeviceID);
        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "The front-end device control response message processing failed: the front-end device ID=%s, the reason = SN XML forwarding to the destination failed", strDeviceID);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "route_device_control_response_proc() transfer_xml_message_to_dest Error:device_id=%s\r\n", caller_id);
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_notify_alarm_response_proc
 功能描述  : 上级互联CMS发送过来的告警信息响应处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_notify_alarm_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    /* 报警响应消息暂时不需要处理

      命令包括如下字段:
      <!-- 命令类型：报警通知（必选） -->
      <element name="CmdType" fixed ="Alarm" />
      <!-- 命令序列号（必选） -->
      <element name="SN" type="integer" minInclusive value = "1" />
      <!-- 目标设备编码（必选） -->
      <element name="DeviceID" type="tg:deviceIDType" />
      <!-- 执行结果标志（必选） -->
      <element name="Result" type="tg:resultType" />
      */

    return 0;
}

/*****************************************************************************
 函 数 名  : route_query_catalog_response_proc
 功能描述  : 上级互联CMS发送过来的目录查询响应消息处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月21日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_query_catalog_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int logic_device_pos = -1;
    char strSN[32] = {0};
    char strDeviceIndex[64] = {0};
    char strDeviceID[32] = {0};
    char strGBLogicDeviceID[32] = {0};
    char strSumNum[16] = {0};
    string strDeviceListNum = "";
    int iSumNum = 0;
    int iDeviceListNum = 0;
    int iItemNumCount = 0;
    char strName[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strEnable[64] = {0};
    char strAlarmDeviceSubType[64] = {0};
    char strCtrlEnable[64] = {0};
    char strMicEnable[64] = {0};
    char strFrameCount[64] = {0};
    char strStreamCount[64] = {0};
    char strAlarmLengthOfTime[64] = {0};
    char strManufacturer[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strModel[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strOwner[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strCivilCode[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strBlock[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strAddress[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strParental[16] = {0};
    char strParentID[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strSafetyWay[16] = {0};
    char strRegisterWay[16] = {0};
    char strCertNum[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strCertifiable[16] = {0};
    char strErrCode[16] = {0};
    char strEndTime[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strSecrecy[16] = {0};
    char strIPAddress[MAX_IP_LEN] = {0};
    char strPort[16] = {0};
    char strPassword[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strStatus[16] = {0};
    char strLongitude[64] = {0};
    char strLatitude[64] = {0};
    char strMapLayer[MAX_128CHAR_STRING_LEN + 4] = {0};
    char strCMSID[MAX_ID_LEN + 4] = {0};
    char strAlarmPriority[32] = {0};/*2016.10.10 add for RCU*/
    char strGuard[32] = {0};/*2016.10.10 add for RCU*/
    char strValue[256] = {0};/*2016.10.10 add for RCU*/
    char strUnit[32] = {0};/*2016.10.10 add for RCU*/
    char strPTZType[64] = {0};
    GBLogicDevice_info_t* pOldGBLogicDeviceInfo = NULL;
    GBLogicDevice_info_t* pNewGBLogicDeviceInfo = NULL;
    DOMElement* ItemAccNode = NULL;
    char strDeviceType[4] = {0};
    int iPTZType = 0;
    char* pTmp = NULL;
    //char* tmp_civil_code;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_catalog_response_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_query_catalog_response_proc() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_catalog_response_proc() Enter---: caller_id=%s, callee_id=%s\r\n", caller_id, callee_id);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS目录查询响应消息:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS directory search response process :Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    /* 网络设备信息查询响应消息直接转发，不做处理
          控制流程见9.5.2

          命令包括如下字段:
          <!-- 命令类型：设备目录查询（必选） -->
          <element name="CmdType" fixed ="Catalog" />
          <!-- 命令序列号（必选） -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- 目标设备的设备编码（必选） -->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- 设备目录项列表,num表示目录项个数 -->
          <element name="DeviceList">
          <attribute name="Num" type="integer"/>
          <choice minOccurs= "0" maxOccurs= " unbounded " >
          <element name="Item" type="tg:itemType"/>
          </choice>
          </element>
          <!-- 扩展信息，可多项 -->
          <element name= "Info" minOccurs= "0" maxOccurs="unbounded">
          <restriction base= "string">
          <maxLength value= "1024" />
          </restriction>
          </element>
      */

    /* 逻辑设备信息一般是设备注册成功之后CMS主动发起的查询 */
    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"SumNum", strSumNum);
    inPacket.GetElementAttr((char*)"DeviceList", (char*)"Num", strDeviceListNum);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n DeviceID=%s \
    \r\n SumNum=%s \
    \r\n DeviceList Num=%s \r\n ", strSN, strDeviceID, strSumNum, (char*)strDeviceListNum.c_str());

    if (strSumNum[0] == '\0')
    {
        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS目录查询响应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=没有获取到前端上报的逻辑通道目录总数", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS directory search response process failed:front device ID=%s, cause=did not find totoal logic channel number reported by front-end.", caller_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: Get Sun Num Error \r\n");
        return -1;
    }

    if (0 != sstrcmp(strDeviceID, pRouteInfo->server_id))
    {
        SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS目录查询响应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=前端设备的XML里面的设备ID错误, XML设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
        EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS directory search response process failed:front device ID=%s, cause=device ID in XML of front-end device incorrect, XMLdeviceID=%s", caller_id, strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: DeviceID Error:DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    /* 将设备信息写入标准逻辑设备表 */
    iSumNum = osip_atoi(strSumNum);
    iDeviceListNum = osip_atoi((char*)strDeviceListNum.c_str());
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_query_catalog_response_proc() SumNum=%d, DeviceListNum=%d \r\n", iSumNum, iDeviceListNum);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS目录查询响应消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 上报的逻辑设备总数=%d, 本次上报的逻辑设备个数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, iSumNum, iDeviceListNum);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS directory search response :total number of logic device reported=%d, number of logic device reported this time=%d", iSumNum, iDeviceListNum);

    if (iSumNum > 0)
    {
        if (iDeviceListNum <= 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: DeviceListNum Error \r\n");
            return -1;
        }

        if (pRouteInfo->CataLogNumCount == 0) /* 第一次收到推送点位信息，将SN赋值 */
        {
            pRouteInfo->CataLogSN = osip_atoi(strSN);

            /* 先将该路由下面的逻辑通道都置为删除状态 */
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS目录查询响应消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 设置该CMS所属的所有通道删除标识, 推送总数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, iSumNum);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS push directory message: set all channel channel delete notification that this CMS belongs to, superior CMS ID=%s, IP=%s, total push number=%d", pRouteInfo->server_id, pRouteInfo->server_ip, iSumNum);
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() Proc Catalog Item Begin---:server_id=%s, SumNum=%d \r\n", pRouteInfo->server_id, iSumNum);
            i = SetGBLogicDeviceInfoDelFlagForRoute(pRouteInfo);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_query_catalog_response_proc() SetGBLogicDeviceInfoDelFlagForRoute:i=%d \r\n", i);

            /* 上报个数统计 */
            pRouteInfo->CataLogNumCount += iDeviceListNum;
            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() DeviceListNum=%d, RouteInfo:server_id=%s,CataLogNumCount=%d \r\n", iDeviceListNum, pRouteInfo->server_id, pRouteInfo->CataLogNumCount);
        }
        else
        {
            if (pRouteInfo->CataLogSN == (unsigned int)osip_atoi(strSN))
            {
                /* 上报个数统计 */
                pRouteInfo->CataLogNumCount += iDeviceListNum;
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() DeviceListNum=%d, RouteInfo:server_id=%s,CataLogNumCount=%d \r\n", iDeviceListNum, pRouteInfo->server_id, pRouteInfo->CataLogNumCount);
            }
        }

        /* 获取所有的Item 数据 */
        ItemAccNode = inPacket.SearchElement((char*)"Item");

        if (!ItemAccNode)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: Get Item Node Error \r\n");
            return -1;
        }

        inPacket.SetCurrentElement(ItemAccNode);

        while (ItemAccNode)
        {
            iItemNumCount++;

            if (iItemNumCount > iDeviceListNum)
            {
                break;
            }

            memset(strDeviceIndex, 0, 64);
            inPacket.GetElementValue((char*)"ID", strDeviceIndex);

            memset(strGBLogicDeviceID, 0, 32);
            inPacket.GetElementValue((char*)"DeviceID", strGBLogicDeviceID);

            memset(strName, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"Name", strName);

            memset(strEnable, 0, 64);
            inPacket.GetElementValue((char*)"Enable", strEnable);

            memset(strAlarmDeviceSubType, 0, 64);
            inPacket.GetElementValue((char*)"ChlType", strAlarmDeviceSubType);

            memset(strCtrlEnable, 0, 64);
            inPacket.GetElementValue((char*)"CtrlEnable", strCtrlEnable);

            memset(strMicEnable, 0, 64);
            inPacket.GetElementValue((char*)"MicEnable", strMicEnable);

            memset(strFrameCount, 0, 64);
            inPacket.GetElementValue((char*)"FrameCount", strFrameCount);

            memset(strStreamCount, 0, 64);
            inPacket.GetElementValue((char*)"StreamCount", strStreamCount);

            memset(strAlarmLengthOfTime, 0, 64);
            inPacket.GetElementValue((char*)"AlarmLengthOfTime", strAlarmLengthOfTime);

            memset(strManufacturer, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"Manufacturer", strManufacturer);

            memset(strModel, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"Model", strModel);

            memset(strOwner, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"Owner", strOwner);

            memset(strCivilCode, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"CivilCode", strCivilCode);

            memset(strBlock, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"Block", strBlock);

            memset(strAddress, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"Address", strAddress);

            memset(strParental, 0, 16);
            inPacket.GetElementValue((char*)"Parental", strParental);

            memset(strParentID, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"ParentID", strParentID);

            memset(strSafetyWay, 0, 16);
            inPacket.GetElementValue((char*)"SafetyWay", strSafetyWay);

            memset(strRegisterWay, 0, 16);
            inPacket.GetElementValue((char*)"RegisterWay", strRegisterWay);

            memset(strCertNum, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"CertNum", strCertNum);

            memset(strCertifiable, 0, 16);
            inPacket.GetElementValue((char*)"Certifiable", strCertifiable);

            memset(strErrCode, 0, 16);
            inPacket.GetElementValue((char*)"ErrCode", strErrCode);

            memset(strEndTime, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"EndTime", strEndTime);

            memset(strSecrecy, 0, 16);
            inPacket.GetElementValue((char*)"Secrecy", strSecrecy);

            memset(strIPAddress, 0, 16);
            inPacket.GetElementValue((char*)"IPAddress", strIPAddress);

            memset(strPort, 0, 16);
            inPacket.GetElementValue((char*)"Port", strPort);

            memset(strPassword, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"Password", strPassword);

            memset(strStatus, 0, 16);
            inPacket.GetElementValue((char*)"Status", strStatus);

            memset(strLongitude, 0, 64);
            inPacket.GetElementValue((char*)"Longitude", strLongitude);

            memset(strLatitude, 0, 64);
            inPacket.GetElementValue((char*)"Latitude", strLatitude);

            memset(strMapLayer, 0, MAX_128CHAR_STRING_LEN + 4);
            inPacket.GetElementValue((char*)"MapLayer", strMapLayer);

            memset(strCMSID, 0, 24);
            inPacket.GetElementValue((char*)"CMSID", strCMSID);

#if 1 /*2016.10.10 add for RCU*/
            memset(strValue, 0, 256);
            inPacket.GetElementValue((char*)"Value", strValue);

            memset(strUnit, 0, 32);
            inPacket.GetElementValue((char*)"Unit", strUnit);

            memset(strGuard, 0, 32);
            inPacket.GetElementValue((char*)"Guard", strGuard);

            memset(strAlarmPriority, 0, 32);
            inPacket.GetElementValue((char*)"AlarmPriority", strAlarmPriority);
#endif/*2016.10.10 add for RCU*/

            DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() ItemCount=%d, GBLogicDeviceID=%s, Name=%s, Status=%s \r\n", iItemNumCount, strGBLogicDeviceID, strName, strStatus);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS目录查询响应消息:上级:上级CMS ID=%s, IP地址=%s, 端口号=%d, 上报的逻辑设备ID=%s, 逻辑点位名称=%s, 状态=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strGBLogicDeviceID, strName, strStatus);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS directory query response message: superior: the superior CMS, ID = % s = % s IP address and port number = % d, report the logical device ID = % s, logical point name = % s, state = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strGBLogicDeviceID, strName, strStatus);

            if (strGBLogicDeviceID[0] == '\0')
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: GBLogic Device ID Error \r\n");
                ItemAccNode = inPacket.SearchNextElement(true);
                continue;
            }

            if (20 != strlen(strGBLogicDeviceID))
            {
                SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS目录查询响应消息:上级:上级CMS ID=%s, IP地址=%s, 端口号=%d, 上报的逻辑设备ID=%s, 逻辑点位名称=%s, 逻辑设备ID长度不合法, 逻辑设备ID长度=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strGBLogicDeviceID, strName, strlen(strGBLogicDeviceID));
                EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Superior CMS directory search response :Logic device ID reported=%s, logic point name=%s, logice device ID length is not valid, logic device ID length=%d", strGBLogicDeviceID, strName, strlen(strGBLogicDeviceID));
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: GBLogic Device ID=%s, Length=%d Error \r\n", strGBLogicDeviceID, strlen(strGBLogicDeviceID));
                ItemAccNode = inPacket.SearchNextElement(true);
                continue;
            }

            /* 查找旧的逻辑设备,看旧的逻辑设备是否是本级的逻辑设备 */
            pOldGBLogicDeviceInfo = GBLogicDevice_info_find(strGBLogicDeviceID);

            if (NULL != pOldGBLogicDeviceInfo)
            {
                if (1 == pOldGBLogicDeviceInfo->del_mark)
                {
                    /* 移除删除标识 */
                    pOldGBLogicDeviceInfo->del_mark = 0;
                }

                /* 如果下发的点位，在本级CMS中可以找到，并且也是非其他域的点位，则不处理,默认还是本域的点位 */
                if (0 == pOldGBLogicDeviceInfo->other_realm)
                {
                    SystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS目录查询响应消息:上级:上级CMS ID=%s, IP地址=%s, 端口号=%d, 上报的逻辑设备ID=%s, 逻辑点位名称=%s, 该逻辑设备是本级CMS的逻辑设备", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, strGBLogicDeviceID, strName);
                    EnSystemLog(EV9000_CMS_NOTIFY_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Superior CMS push point info:Logic device ID reported=%s, logic point name=%s, this logic device is parallel CMS logic device", strGBLogicDeviceID, strName);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: GBLogic Device ID=%s, is Local Realm LogicDevice \r\n", strGBLogicDeviceID);
                    ItemAccNode = inPacket.SearchNextElement(true);
                    continue;
                }
            }

            i = GBLogicDevice_info_init(&pNewGBLogicDeviceInfo);

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: GBLogicDevice Info Init Error \r\n");
                ItemAccNode = inPacket.SearchNextElement(true);
                continue;
            }

            /* 将信息写入新的结构 */
            /* 点位统一编号 */
            if (strGBLogicDeviceID[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->device_id, strGBLogicDeviceID, MAX_ID_LEN);
            }

            /* 逻辑设备索引 */
            if (strDeviceIndex[0] != '\0')
            {
                pNewGBLogicDeviceInfo->id = strtoul(strDeviceIndex, NULL, 10);
            }
            else
            {
                if ('\0' != pNewGBLogicDeviceInfo->device_id[0])
                {
                    pNewGBLogicDeviceInfo->id = crc32((unsigned char*)pNewGBLogicDeviceInfo->device_id, MAX_ID_LEN);
                }
            }

            /* 所属的CMS统一编号，如果是下级CMS上报的，则存在该数据，如果是具体物理设备，填本级CMSID */
            if (strCMSID[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->cms_id, strCMSID, MAX_ID_LEN);
            }
            else
            {
                osip_strncpy(pNewGBLogicDeviceInfo->cms_id, local_cms_id_get(), MAX_ID_LEN);
            }

            /* 点位名称 */
            if (strName[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->device_name, strName, MAX_128CHAR_STRING_LEN);
            }

            /* 设备类型 */
            if ('\0' != pNewGBLogicDeviceInfo->device_id[0])
            {
                pTmp = &pNewGBLogicDeviceInfo->device_id[10];
                osip_strncpy(strDeviceType, pTmp, 3);
                pNewGBLogicDeviceInfo->device_type = osip_atoi(strDeviceType);
            }
            else
            {
                pNewGBLogicDeviceInfo->device_type = EV9000_DEVICETYPE_IPC;
            }

            /* 是否启用 */
            if (strEnable[0] != '\0')
            {
                pNewGBLogicDeviceInfo->enable = osip_atoi(strEnable);
            }
            else
            {
                pNewGBLogicDeviceInfo->enable = 1;
            }

            /* 是否可控 */
            if (strPTZType[0] != '\0') /* 国标新增扩展协议, 1-球机；2-半球；3-固定枪机； 4-遥控枪机 */
            {
                iPTZType = osip_atoi(strPTZType);

                pNewGBLogicDeviceInfo->ctrl_enable = iPTZType;
            }
            else
            {
                if (strCtrlEnable[0] != '\0')
                {
                    if (0 == sstrcmp((char*)"Disable", strCtrlEnable))
                    {
                        pNewGBLogicDeviceInfo->ctrl_enable = 0;
                    }
                    else if (0 == sstrcmp((char*)"Enable", strCtrlEnable))
                    {
                        pNewGBLogicDeviceInfo->ctrl_enable = 1;
                    }
                    else
                    {
                        pNewGBLogicDeviceInfo->ctrl_enable = 0;
                    }
                }
                else
                {
                    pNewGBLogicDeviceInfo->ctrl_enable = 0;
                }
            }

            /* 是否支持对讲 */
            if (strMicEnable[0] != '\0')
            {
                if (0 == sstrcmp((char*)"Disable", strMicEnable))
                {
                    pNewGBLogicDeviceInfo->mic_enable = 0;
                }
                else if (0 == sstrcmp((char*)"Enable", strMicEnable))
                {
                    pNewGBLogicDeviceInfo->mic_enable = 1;
                }
                else
                {
                    pNewGBLogicDeviceInfo->mic_enable = 0;
                }
            }
            else
            {
                pNewGBLogicDeviceInfo->mic_enable = 0;
            }

            /* 帧率 */
            if (strFrameCount[0] != '\0')
            {
                pNewGBLogicDeviceInfo->frame_count = osip_atoi(strFrameCount);
            }
            else
            {
                pNewGBLogicDeviceInfo->frame_count = 25;
            }

            /* 是否支持多码流 */
            if (strStreamCount[0] != '\0')
            {
                pNewGBLogicDeviceInfo->stream_count = osip_atoi(strStreamCount);
            }
            else
            {
                if (pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_CAMERA
                    || pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_IPC
                    || pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_SCREEN
                    || pNewGBLogicDeviceInfo->device_type == EV9000_DEVICETYPE_CODER)
                {
                    pNewGBLogicDeviceInfo->stream_count = 1;
                }
                else
                {
                    pNewGBLogicDeviceInfo->stream_count = 0;
                }
            }

            /* 帧率 */
            if (strAlarmLengthOfTime[0] != '\0')
            {
                pNewGBLogicDeviceInfo->alarm_duration = osip_atoi(strAlarmLengthOfTime);
            }
            else
            {
                pNewGBLogicDeviceInfo->alarm_duration = 0;
            }

            /* 是否属于其他域 */
            pNewGBLogicDeviceInfo->other_realm = 1;

            /* 设备生产商 */
            if (strManufacturer[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->manufacturer, strManufacturer, MAX_128CHAR_STRING_LEN);
            }

            /* 设备型号 */
            if (strModel[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->model, strModel, MAX_128CHAR_STRING_LEN);
            }

            /* 设备归属 */
            if (strOwner[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->owner, strOwner, MAX_128CHAR_STRING_LEN);
            }

            /* 警区 */
            if (strBlock[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->block, strBlock, MAX_128CHAR_STRING_LEN);
            }

            /* 安装地址 */
            if (strAddress[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->address, strAddress, MAX_128CHAR_STRING_LEN);
            }

            /* 是否有子设备 */
            if (strParental[0] != '\0')
            {
                pNewGBLogicDeviceInfo->parental = osip_atoi(strParental);
            }

            /* 父设备/区域/系统ID */
            if (strParentID[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->parentID, strParentID, MAX_128CHAR_STRING_LEN);
            }

            /* 信令安全模式*/
            if (strSafetyWay[0] != '\0')
            {
                pNewGBLogicDeviceInfo->safety_way = osip_atoi(strSafetyWay);
            }

            /* 注册方式 */
            if (strRegisterWay[0] != '\0')
            {
                pNewGBLogicDeviceInfo->register_way = osip_atoi(strRegisterWay);
            }

            /* 证书序列号*/
            if (strCertNum[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->cert_num, strCertNum, MAX_128CHAR_STRING_LEN);
            }

            /* 证书有效标识 */
            if (strCertifiable[0] != '\0')
            {
                pNewGBLogicDeviceInfo->certifiable = osip_atoi(strCertifiable);
            }

            /* 无效原因码 */
            if (strErrCode[0] != '\0')
            {
                pNewGBLogicDeviceInfo->error_code = osip_atoi(strErrCode);
            }

            /* 证书终止有效期*/
            if (strEndTime[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->end_time, strEndTime, MAX_128CHAR_STRING_LEN);
            }

            /* 保密属性 */
            if (strSecrecy[0] != '\0')
            {
                pNewGBLogicDeviceInfo->secrecy = osip_atoi(strSecrecy);
            }

            /* IP地址*/
            if (strIPAddress[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->ip_address, strIPAddress, MAX_IP_LEN);
            }
            else
            {
                osip_strncpy(pNewGBLogicDeviceInfo->ip_address, pRouteInfo->server_ip, MAX_IP_LEN);
            }

            /* 端口号*/
            if (strPort[0] != '\0')
            {
                pNewGBLogicDeviceInfo->port = osip_atoi(strPort);
            }
            else
            {
                pNewGBLogicDeviceInfo->port = pRouteInfo->server_port;
            }

            /* 密码*/
            if (strPassword[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->password, strPassword, MAX_128CHAR_STRING_LEN);
            }

            /* 点位状态 */
            if (strStatus[0] != '\0' && (0 == sstrcmp(strStatus, (char*)"ON") || 0 == sstrcmp(strStatus, (char*)"ONLINE")))
            {
                pNewGBLogicDeviceInfo->status = 1;
            }
            else if (strStatus[0] != '\0' && (0 == sstrcmp(strStatus, (char*)"OFF") || 0 == sstrcmp(strStatus, (char*)"OFFLINE")))
            {
                pNewGBLogicDeviceInfo->status = 0;
                pNewGBLogicDeviceInfo->intelligent_status = INTELLIGENT_STATUS_NULL;
                pNewGBLogicDeviceInfo->alarm_status = ALARM_STATUS_NULL;
            }
            else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"NOVIDEO"))
            {
                pNewGBLogicDeviceInfo->status = 2;
            }
            else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"VLOST"))
            {
                pNewGBLogicDeviceInfo->status = 2;
            }
            else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"INTELLIGENT"))
            {
                pNewGBLogicDeviceInfo->status = 1;
                pNewGBLogicDeviceInfo->intelligent_status = INTELLIGENT_STATUS_ON;
            }
            else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"CLOSE"))
            {
                pNewGBLogicDeviceInfo->status = 1;
                pNewGBLogicDeviceInfo->alarm_status = ALARM_STATUS_CLOSE;
            }
            else if (strStatus[0] != '\0' && 0 == sstrcmp(strStatus, (char*)"APART"))
            {
                pNewGBLogicDeviceInfo->status = 1;
                pNewGBLogicDeviceInfo->alarm_status = ALARM_STATUS_APART;
            }
            else
            {
                pNewGBLogicDeviceInfo->status = 0;
            }

            /* 经度 */
            if (strLongitude[0] != '\0')
            {
                pNewGBLogicDeviceInfo->longitude = strtod(strLongitude, (char**) NULL);
            }

            /* 纬度 */
            if (strLatitude[0] != '\0')
            {
                pNewGBLogicDeviceInfo->latitude = strtod(strLatitude, (char**) NULL);
            }

            /* 所属图层 */
            if (strMapLayer[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->map_layer, strMapLayer, MAX_128CHAR_STRING_LEN);
            }

            /* 报警设备子类型 */
            if (strAlarmDeviceSubType[0] != '\0')
            {
                pNewGBLogicDeviceInfo->alarm_device_sub_type = osip_atoi(strAlarmDeviceSubType);
            }
            else
            {
                pNewGBLogicDeviceInfo->alarm_device_sub_type = 0;
            }

#if 1 /*2016.10.10 add for RCU*/

            /* Guard*/
            if (strGuard[0] != '\0')
            {
                pNewGBLogicDeviceInfo->guard_type = osip_atoi(strGuard);
            }

            /* 报警级别 */
            if (strAlarmPriority[0] != '\0')
            {
                pNewGBLogicDeviceInfo->AlarmPriority = osip_atoi(strAlarmPriority);
            }

            /* 单位 */
            if (strUnit[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->Unit, strUnit, 32);
            }

            /* Value */
            if (strValue[0] != '\0')
            {
                osip_strncpy(pNewGBLogicDeviceInfo->Value, strValue, 256);
            }

#endif /*2016.10.10 add for RCU*/

            /* 根据旧的逻辑设备是否存在判断 */
            if (NULL != pOldGBLogicDeviceInfo)
            {
                i = GBLogicDeviceCatalogInfoProcForRoute(pNewGBLogicDeviceInfo, pOldGBLogicDeviceInfo, pRoute_Srv_dboper);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_response_proc() GBLogicDeviceCatalogInfoProcForRoute Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_query_catalog_response_proc() GBLogicDeviceCatalogInfoProcForRoute OK:iRet=%d \r\n", i);
                }

                GBLogicDevice_info_free(pNewGBLogicDeviceInfo);
                osip_free(pNewGBLogicDeviceInfo);
                pNewGBLogicDeviceInfo = NULL;
            }
            else
            {
                /* 添加逻辑设备信息 */
                logic_device_pos = GBLogicDevice_info_add(pNewGBLogicDeviceInfo);

                //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() GBLogicDevice_info_add:logic_device_pos=%d \r\n", logic_device_pos);

                if (logic_device_pos < 0)
                {
                    GBLogicDevice_info_free(pNewGBLogicDeviceInfo);
                    osip_free(pNewGBLogicDeviceInfo);
                    pNewGBLogicDeviceInfo = NULL;
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() exit---: GBLogicDevice Info Add Error \r\n");
                    ItemAccNode = inPacket.SearchNextElement(true);
                    continue;
                }

                /* 发送添加消息给下级CMS  */
                i = SendNotifyCatalogToSubCMS(pNewGBLogicDeviceInfo, 0, pRoute_Srv_dboper);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_query_catalog_response_proc() SendNotifyCatalogToSubCMS Error:iRet=%d \r\n", i);
                }
                else if (i > 0)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_query_catalog_response_proc() SendNotifyCatalogToSubCMS OK:iRet=%d \r\n", i);
                }

                /* 更新数据库 */
                i = AddGBLogicDeviceInfo2DBForRoute(strGBLogicDeviceID, pRoute_Srv_dboper);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_query_catalog_response_proc() AddGBLogicDeviceInfo2DBForRoute ERROR:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_query_catalog_response_proc() AddGBLogicDeviceInfo2DBForRoute OK:device_id=%s, iRet=%d \r\n", strGBLogicDeviceID, i);
                }
            }

            ItemAccNode = inPacket.SearchNextElement(true);
        }

        if (pRouteInfo->CataLogSN == (unsigned int)osip_atoi(strSN))
        {
            /* 上报结束，清零 */
            if (pRouteInfo->CataLogNumCount >= iSumNum)
            {
                DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_query_catalog_response_proc() Proc Catalog Item End---:server_id=%s, CataLogNumCount=%d, SumNum=%d \r\n", pRouteInfo->server_id, pRouteInfo->CataLogNumCount, iSumNum);
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级CMS推送目录消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 开始根据删除标识删除该CMS下面的通道", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Superior CMS push directory message:start to delete channel belong to this CMS accorrding to delete notification, superior CMS ID=%s, IP=%s", pRouteInfo->server_id, pRouteInfo->server_ip);

                pRouteInfo->CataLogNumCount = 0;

                /* 根据删除标识，设置逻辑设备禁用标识 */
                i = SetGBLogicDeviceInfoEnableFlagByDelMarkForRoute(pRouteInfo, pRoute_Srv_dboper);
            }
        }
    }

    return i;
}

/*****************************************************************************
 函 数 名  : route_device_info_response_proc
 功能描述  : 上级互联CMS发送过来的设备设备信息查询响应处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_device_info_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
#if 0

    int i = 0;
    int device_pos = -1;
    GBDevice_info_t* pGBDeviceInfo = NULL;

    /* 网络设备信息查询响应消息直接转发，不做处理
          控制流程见9.5.2

          命令包括如下字段:
          <!-- 命令类型：设备信息查询（必选） -->
          <element name="CmdType" fixed ="DeviceInfo" />
          <!-- 命令序列号（必选） -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- 目标设备的设备编码（必选） -->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- 查询结果（必选） -->
          <element name="Result" type="tg:resultType" />
          <!-- 设备生产商（可选） -->
          <element name ="Manufacturer" type="normalizedString" minOccurs= "0"/ >
          <!-- 设备型号（可选） -->
          <element name ="Model" type="string" minOccurs= "0"/>
          <!-- 设备固件版本（可选） -->
          <element name ="Firmware" type="string" minOccurs= "0"/>
          <!-- 视频输入通道数（可选） -->
          <element name ="Channel" type="integer" minInclusive value = "0" minOccurs= "0"/ >
          <!-- 扩展信息，可多项 -->
          <element name= "Info" minOccurs= "0" maxOccurs="unbounded">
          <restriction base= "string">
          <maxLength value= "1024" />
          </restriction>
          </element>
      */

    /* 根据callee_id 查找物理设备*/
    device_pos = GBDevice_info_find_by_GBLogicDevice(callee_id);

    if (device_pos >= 0)
    {
        pGBDeviceInfo = GBDevice_info_get(device_pos);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_ProxyBuildTargetAndSendMessage(caller_id, callee_id, pGBDeviceInfo->device_id, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());
        }
    }
    else
    {
        i = SIP_ProxyBuildTargetAndSendMessage(caller_id, callee_id, callee_id, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : route_device_status_response_proc
 功能描述  : 上级互联CMS发送过来的设备状态查询响应处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_device_status_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
#if 0

    int i = 0;
    int device_pos = -1;
    GBDevice_info_t* pGBDeviceInfo = NULL;

    /* 网络设备信息查询响应消息直接转发，不做处理
          控制流程见9.5.2

          命令包括如下字段:
          <!-- 命令类型：设备状态查询（必选） -->
          <element name="CmdType" fixed ="DeviceStatus" />
          <!-- 命令序列号（必选） -->
          <element name="SN" type="integer" minInclusive value = "1" />
          <!-- 目标设备的设备编码（必选） -->
          <element name="DeviceID" type="tg:deviceIDType" />
          <!-- 查询结果标志（必选） -->
          <element name="Result" type="tg:resultType" />
          <!-- 是否在线（必选） -->
          <element name="Online" >
          <restriction base= "string">
          <enumeration value= "ONLINE" />
          <enumeration value= "OFFLINE" />
          </restriction>
          </element>
          <!-- 是否正常工作（必选） -->
          <element name="Status" type="tg:relultType" />
          <!-- 不正常工作原因（可选） -->
          <element name="Reason" type="string" minOccurs= "0"/>
          <!-- 是否编码（可选） -->
          <element name="Encode" type="tg:statusType" minOccurs= "0"/>
          <!-- 是否录像（可选） -->
          <element name="Record" type=" tg:statusType" minOccurs= "0"/>
          <!-- 设备时间和日期（可选） -->
          <element name ="DeviceTime" type="dateTime" minOccurs= "0"/>
          <!-- 报警设备状态列表,num表示列表项个数（可选） -->
          <element name="Alarmstatus" minOccurs="0">
          <attribute name="Num" type="integer"/>
          <element name="Item" minOccurs="0" maxOccurs=" unbounded ">
          <simpleType>
          <sequence>
          <!-- 报警设备编码（必选） -->
          <element name="DeviceID" type=" tg:deviceIDType " minOccurs= "0"/>
          <!-- 报警设备状态（必选） -->
          <element name="Status" minOccurs= "0">
          <restriction base="string">
          <enumeration value="ONDUTY"/>
          <enumeration value="OFFDUTY"/>
          <enumeration value="ALARM"/>
          </restriction>
          </element>
          </sequence>
          </simpleType>
          </element>
          </element>
          <!-- 扩展信息，可多项 -->
          <element name= "Info" minOccurs= "0" maxOccurs="unbounded">
          <restriction base= "string">
          <maxLength value= "1024" />
          </restriction>
          </element>
      */

    /* 根据callee_id 查找物理设备*/
    device_pos = GBDevice_info_find_by_GBLogicDevice(callee_id);

    if (device_pos >= 0)
    {
        pGBDeviceInfo = GBDevice_info_get(device_pos);

        if (NULL != pGBDeviceInfo)
        {
            i = SIP_ProxyBuildTargetAndSendMessage(caller_id, callee_id, pGBDeviceInfo->device_id, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());
        }
    }
    else
    {
        i = SIP_ProxyBuildTargetAndSendMessage(caller_id, callee_id, callee_id, (char*)inPacket.GetXml(NULL).c_str(), inPacket.GetXml(NULL).length());
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : route_record_info_response_proc
 功能描述  : 上级互联CMS发送过来的录像信息查询响应处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             CPacket& inPacket
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月20日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int route_record_info_response_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, CPacket& inPacket)
{
    /* 设备视音频文件检索, 转发给用户
      */
    int i = 0;
    int xml_pos = -1;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};
    char strSumNum[16] = {0};
    string strRecordListNum = "";
    int iSumNum = 0;
    int iRecordListNum = 0;
    unsigned int transfer_xml_sn = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_record_info_response_proc() exit---: GBDevice Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_record_info_response_proc() exit---: Param Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS录像信息查询响应消息:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS video information query response message:front-end device ID=%s", caller_id);

    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);
    inPacket.GetElementValue((char*)"SumNum", strSumNum);
    inPacket.GetElementAttr((char*)"RecordList", (char*)"Num", strRecordListNum);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_record_info_response_proc() \
    \r\n XML Para: \
    \r\n SN=%s \
    \r\n DeviceID=%s \
    \r\n SumNum=%s \
    \r\n RecordList Num=%s \r\n ", strSN, strDeviceID, strSumNum, (char*)strRecordListNum.c_str());

    if (strSumNum[0] == '\0')
    {
        SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS录像信息查询响应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=没有获取到前端上报的录像记录总数", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS video information query response message process failed:front-end deviceID=%s, cause=did not get total number of video reported", caller_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR,  "route_record_info_response_proc() exit---: Get Sun Num Error \r\n");
        return -1;
    }

    /* 将设备信息写入标准逻辑设备表 */
    iSumNum = osip_atoi(strSumNum);
    iRecordListNum = osip_atoi((char*)strRecordListNum.c_str());
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE,  "route_record_info_response_proc() SumNum=%d, RecordListNum=%d \r\n", iSumNum, iRecordListNum);
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS录像信息查询响应消息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 上报的录像总数=%d, 本次上报的录像条数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, iSumNum, iRecordListNum);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS video information query response message:front-end device ID=%s, total video number reported=%d, video number reported this time=%d", caller_id, iSumNum, iRecordListNum);

    /* 看看是否是用户查询或者上级查询的 */
    transfer_xml_sn = strtoul(strSN, NULL, 10);
    xml_pos = transfer_xml_msg_find(XML_QUERY_RECORDINFO, strDeviceID, transfer_xml_sn);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "route_record_info_response_proc() transfer_xml_msg_find:Type=%d, DeviceID=%s, transfer_xml_sn=%d, xml_pos=%d \r\n", XML_QUERY_RECORDINFO, strDeviceID, transfer_xml_sn, xml_pos);

    if (xml_pos >= 0)
    {
        i = transfer_xml_message_to_dest(xml_pos, iSumNum, iRecordListNum, inPacket);

        if (i != 0)
        {
            SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS录像信息查询响应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=根据XML的SN转发给目的地失败, xml_pos=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS video information query response message process failed:front-end device ID=%s:front-end device ID=%s, cause=forward to destination failed accorrding to XML SN", caller_id);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_record_info_response_proc() transfer_xml_message_to_dest Error:device_id=%s\r\n", caller_id);
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS录像信息查询响应消息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d,根据XML的SN转发给目的地, xml_pos=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS video information query response message process success:Superior CMS, ID = % s = % s IP address, port number = % d, according to the XML SN forwarded to destination, xml_pos = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, xml_pos);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "route_record_info_response_proc() transfer_xml_message_to_dest OK:device_id=%s\r\n", caller_id);
        }
    }
    else
    {
        SystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS录像信息查询响应消息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=根据XML的SN查找目的地失败, transfer_xml_sn=%d, strDeviceID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, transfer_xml_sn, strDeviceID);
        EnSystemLog(EV9000_CMS_QUERY_RECORD_INFO_ERROR, EV9000_LOG_LEVEL_ERROR, "Superior CMS video information query response message process failed:front-end device ID=%s:front-end device ID=%s, cause=find to destination failed accorrding to XML SN", caller_id);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_record_info_response_proc() transfer_xml_message_to_dest Error:device_id=%s\r\n", caller_id);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : route_subscribe_query_catalog_proc
 功能描述  : 上级互联路由过来的域间目录订阅消息的处理
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* callee_id
             int event_id
             int subscribe_expires
             CPacket& inPacket
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年6月10日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_subscribe_query_catalog_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, int event_id, int subscribe_expires, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_query_catalog_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_query_catalog_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的目录订阅信息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Directory subscription information from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_subscribe_query_catalog_proc() \
            \r\n XML Para: \
            \r\n SN=%s, DeviceID=%s\r\n", strSN, strDeviceID);

    /* 查看被叫是否是本CMS ID */
    if (0 != strncmp(strDeviceID, local_cms_id_get(), 20))
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的目录订阅信息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"查询的ID不是本CMS的ID", strDeviceID);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Directory subscription information from superior CMS process failed:Requester ID=%s, IPaddress=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_subscribe_query_catalog_proc() exit---: Device ID Not Belong To Mine CMSID:DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    if (0 == pRouteInfo->catalog_subscribe_flag)
    {
        /* 更新状态 */
        pRouteInfo->catalog_subscribe_flag = 1;
        pRouteInfo->catalog_subscribe_expires = subscribe_expires;
        pRouteInfo->catalog_subscribe_expires_count = subscribe_expires;

        if (event_id > 0)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的目录订阅信息:上级CMS ID=%s, IP地址=%s, 端口号=%d, CMS启动之后收到的初始订阅", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            pRouteInfo->catalog_subscribe_event_id = event_id;
        }
        else
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的目录订阅信息:上级CMS ID=%s, IP地址=%s, 端口号=%d, CMS启动之后收到的初始订阅, 没有携带Event ID字段", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            pRouteInfo->catalog_subscribe_event_id = 99999;
        }

        /* 发送掉线状态点位给上级 */
        i = SendAllOfflineDeviceStatusTo3PartyRouteCMS(pRouteInfo);
    }
    else
    {
        pRouteInfo->catalog_subscribe_expires = subscribe_expires;
        pRouteInfo->catalog_subscribe_expires_count = subscribe_expires;

        if (event_id > 0)
        {
            if (pRouteInfo->catalog_subscribe_event_id != event_id)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的目录订阅信息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 收到上级CMS启动之后的初始订阅", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                pRouteInfo->catalog_subscribe_event_id = event_id;

                /* 发送掉线状态点位给上级 */
                i = SendAllOfflineDeviceStatusTo3PartyRouteCMS(pRouteInfo);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的目录订阅信息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 收到上级CMS的刷新订阅消息", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
        else /* 上级平台可能有时候没有发送这个字段时候，每次都发送 */
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的目录订阅信息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 收到上级CMS的刷新订阅消息, 没有携带Event ID字段", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            pRouteInfo->catalog_subscribe_event_id = 99999;

            /* 发送掉线状态点位给上级 */
            i = SendAllOfflineDeviceStatusTo3PartyRouteCMS(pRouteInfo);
        }
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的目录订阅信息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 发送所有离线点位状态消息给上级平台", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Directory subscription information from superior CMS process success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    return i;
}

int route_subscribe_witin_dialog_query_catalog_proc(route_info_t* pRouteInfo, char* caller_id, char* callee_id, int dialog_index, int subscribe_expires, CPacket& inPacket, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    char strSN[32] = {0};
    char strDeviceID[32] = {0};

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_witin_dialog_query_catalog_proc() exit---: Route Info Error \r\n");
        return -1;
    }

    if (NULL == caller_id || NULL == callee_id || NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_subscribe_witin_dialog_query_catalog_proc() exit---: Route Srv DB Oper Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的目录订阅信息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Directory subscription information from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, callee_id);

    /* 取得数据*/
    inPacket.GetElementValue((char*)"SN", strSN);
    inPacket.GetElementValue((char*)"DeviceID", strDeviceID);

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO,  "route_subscribe_witin_dialog_query_catalog_proc() \
            \r\n XML Para: \
            \r\n SN=%s, DeviceID=%s\r\n", strSN, strDeviceID);

    /* 查看被叫是否是本CMS ID */
    if (0 != strncmp(strDeviceID, local_cms_id_get(), 20))
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的目录订阅信息处理失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"查询的ID不是本CMS的ID", strDeviceID);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Directory subscription information from superior CMS process failed:Requester ID=%s, IPaddress=%s, port number=%d, cause=%s:%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"ID searched does not belong to this CMS", strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_subscribe_witin_dialog_query_catalog_proc() exit---: Device ID Not Belong To Mine CMSID:DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    if (subscribe_expires > 0)
    {
        if (0 == pRouteInfo->catalog_subscribe_flag)
        {
            /* 更新状态 */
            pRouteInfo->catalog_subscribe_flag = 1;
            pRouteInfo->catalog_subscribe_expires = subscribe_expires;
            pRouteInfo->catalog_subscribe_expires_count = subscribe_expires;
            pRouteInfo->catalog_subscribe_dialog_index = dialog_index;

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的目录订阅信息:上级CMS ID=%s, IP地址=%s, 端口号=%d, CMS启动之后收到的初始订阅", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

        }
        else
        {
            pRouteInfo->catalog_subscribe_expires = subscribe_expires;
            pRouteInfo->catalog_subscribe_expires_count = subscribe_expires;
            pRouteInfo->catalog_subscribe_dialog_index = dialog_index;

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的目录订阅信息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 收到上级CMS的刷新订阅消息", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        /* 发送掉线状态点位给上级 */
        i = SendAllOfflineDeviceStatusTo3PartyRouteCMS(pRouteInfo);
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的目录订阅信息处理成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 发送所有离线点位状态消息给上级平台", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Directory subscription information from superior CMS process success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    return i;
}

/*****************************************************************************
 函 数 名  : route_get_service_id_response_proc
 功能描述  : 获取服务器ID的响应处理
 输入参数  : char* caller_id
             char* caller_ip
             int caller_port
             char* callee_id
             char* local_ip
             int local_port
             char* pcSN
             char* pcServerID
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月13日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int route_get_service_id_response_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* local_ip, int local_port, char* pcSN, char* pcServerID, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int route_pos = -1;
    route_info_t* pRouteInfo = NULL;
    string strSQL = "";
    char strID[32] = {0};

    if (NULL == pRoute_Srv_dboper)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_service_id_response_proc() exit---: Route Srv dboper Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == caller_ip) || (caller_port <= 0))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_service_id_response_proc() exit---: Caller Info Error \r\n");
        return -1;
    }

    if ((NULL == callee_id) || (NULL == local_ip) || (local_port <= 0))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_service_id_response_proc() exit---: Callee Info Error \r\n");
        return -1;
    }

    if ((NULL == pcServerID) || (pcServerID[0] == '\0'))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_service_id_response_proc() exit---: Server ID Error \r\n");
        return -1;
    }

    /* 查找路由信息 */
    route_pos = route_info_find_by_host_and_port(caller_ip, caller_port);

    if (route_pos < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_service_id_response_proc() exit---: route_info_find_by_host_and_port Error:server IP=%s, server port=%d \r\n", caller_ip, caller_port);
        return -1;
    }

    pRouteInfo = route_info_get(route_pos);

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "route_get_service_id_response_proc() exit---: route_info_get Error:route_pos=%d \r\n", route_pos);
        return -1;
    }

    /* 更新到内存 */
    memset(pRouteInfo->server_id, 0, MAX_ID_LEN);
    osip_strncpy(pRouteInfo->server_id, pcServerID, MAX_ID_LEN);

    /* 更新数据库 */
    strSQL.clear();
    strSQL = "UPDATE RouteNetConfig SET ServerID = '";
    strSQL += pcServerID;
    strSQL += "' WHERE ID = ";
    snprintf(strID, 16, "%d", pRouteInfo->id);
    strSQL += strID;

    i = pRoute_Srv_dboper->DB_Update(strSQL.c_str(), 1);

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_get_service_id_response_proc() DB Oper Error:strSQL=%s, iRet=%d \r\n", strSQL.c_str(), i);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "route_get_service_id_response_proc() ErrorMsg=%s\r\n", pRoute_Srv_dboper->GetLastDbErrorMsg());
    }

    return i;
}

/*****************************************************************************
 函 数 名  : RouteGetGBDeviceListAndSendCataLogToCMS
 功能描述  : 上级互联CMS发送过来的获取设备信息并将其发送给上级CMS
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* strDeviceID
             char* strSN
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月10日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int RouteGetGBDeviceListAndSendCataLogToCMS(route_info_t* pRouteInfo, char* caller_id, char* strDeviceID, char* strSN, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    static int iWaitCount = 0;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendCataLogToCMS() exit---: Route Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == strDeviceID) || (NULL == strSN) || (NULL == pRoute_Srv_dboper))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendCataLogToCMS() exit---: Param Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS获取设逻辑备信息:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device information from superior CMS: superior CMS, ID = % s = % s IP address, port number = % d ", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    if (1 == pRouteInfo->catlog_get_status)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级CMS获取设逻辑备信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=上次获取Catlog还没有结束", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        return 0;
    }
    else
    {
        /* 检查数据库是否正在更新 */
        pRouteInfo->catlog_get_status = 1;

        while (checkIfHasDBRefresh() && iWaitCount < 12)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级CMS获取设逻辑备信息,数据库正在更新,等待数据库更新完成:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            osip_usleep(5000000);

            iWaitCount++;
        }

        if (iWaitCount >= 12)
        {
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级CMS获取设逻辑备信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=等待更新数据库完成超时,请稍后再获取", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            iWaitCount = 0;
            pRouteInfo->catlog_get_status = 0;
            return 0;
        }

        iWaitCount = 0;

        if (1 == pRouteInfo->three_party_flag) /* 需要发送行政区域分组信息 */
        {
            i = RouteGetGBDeviceListAndSendCataLogTo3PartyCMS(pRouteInfo, caller_id, strDeviceID, strSN, pRoute_Srv_dboper);
        }
        else
        {
            i = RouteGetGBDeviceListAndSendCataLogToOwnerCMS(pRouteInfo, caller_id, strDeviceID, strSN, pRoute_Srv_dboper);
        }

        pRouteInfo->catlog_get_status = 0;
    }

    return i;
}

/*****************************************************************************
 函 数 名  : RouteGetGBDeviceListAndSendCataLogToOwnerCMS
 功能描述  : 上级互联CMS发送过来的获取设备信息并将其发送给上级CMS
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* strDeviceID
             char* strSN
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月6日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int RouteGetGBDeviceListAndSendCataLogToOwnerCMS(route_info_t* pRouteInfo, char* caller_id, char* strDeviceID, char* strSN, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int index = 0;
    int record_count = 0; /* 记录数 */
    int send_count = 0;   /* 发送的次数 */
    int query_count = 0;  /* 查询数据统计 */
    DOMElement* ListAccNode = NULL;

    string strSQL = "";
    vector<string> DeviceIDVector;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() exit---: Route Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == strDeviceID) || (NULL == strSN) || (NULL == pRoute_Srv_dboper))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() exit---: Param Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS获取设逻辑备信息, 上级为本地平台:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior CMS access logic device information :Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    /* 上级cms查询的设备列表是本级cms中所有设备列表
     */
    DeviceIDVector.clear();

    /* 添加所有的逻辑设备 */
    i = AddAllGBLogicDeviceIDToVectorForRoute(DeviceIDVector, pRouteInfo->id, pRouteInfo->three_party_flag, pRouteInfo->link_type, pRoute_Srv_dboper);

    /* 4、获取容器中的设备个数 */
    record_count = DeviceIDVector.size();

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() record_count=%d \r\n", record_count);

    /* 5、如果记录数为0 */
    if (record_count == 0)
    {
        /* 回复响应,组建消息 */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"Catalog");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"0");

        ListAccNode = outPacket.CreateElement((char*)"DeviceList");
        outPacket.SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

        /* 转发消息给上级CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS获取逻辑设备信息信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"未查询到数据库记录");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Access logic device info from superior CMS message failed:Requester ID=%s, IPaddress=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Database record not found");
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() exit---: No Record Count \r\n");
        return i;
    }

    /* 6、循环查找容器，读取用户的设备信息，加入xml中 */
    CPacket* pOutPacket = NULL;

    for (index = 0; index < record_count; index++)
    {
        //DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "RouteGetGBDeviceListAndSendCataLogToCMS() DeviceIndex=%u \r\n", device_index);

        /* 如果记录数大于4，则要分次发送 */
        query_count++;

        /* 创建XML头部 */
        i = CreateGBLogicDeviceCatalogResponseXMLHeadForRoute(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

        /* 加入Item 值 */
        i = AddLogicDeviceInfoToXMLItemForRoute(pOutPacket, ListAccNode, (char*)DeviceIDVector[index].c_str(), pRouteInfo->three_party_flag, pRoute_Srv_dboper);

        if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 0) || (query_count == record_count))
        {
            if (NULL != pOutPacket)
            {
                send_count++;

                /* 转发消息给上级CMS */
                i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS获取逻辑设备信息信息, 发送Message消息到上级CMS失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 发送次数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, send_count);
                    EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logic device info from superior CMS message, Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS获取逻辑设备信息信息, 发送Message消息到上级CMS成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 发送次数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, send_count);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device info from superior CMS message, Send Message Message to superiors CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                }

                delete pOutPacket;
                pOutPacket = NULL;
            }
        }
    }

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS获取逻辑设备信息成功:上级CMS ID=%s, IP地址=%s, 端口号=%d,逻辑设备数目=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device info from superior CMS message, Send Message Message to superiors CMS success:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }
    else
    {
        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS获取逻辑设备信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"发送SIP响应消息失败");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logic device info from superior CMS message, Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendCataLogToOwnerCMS Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : RouteGetGBDeviceListAndSendCataLogTo3PartyCMS
 功能描述  : 第三方上级互联CMS发送过来的获取设备信息并将其发送给上级CMS
 输入参数  : route_info_t* pRouteInfo
             char* caller_id
             char* strDeviceID
             char* strSN
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月6日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int RouteGetGBDeviceListAndSendCataLogTo3PartyCMS(route_info_t* pRouteInfo, char* caller_id, char* strDeviceID, char* strSN, DBOper* pRoute_Srv_dboper)
{
    int i = 0;
    int index = 0;
    int group_record_count = 0; /* 记录数 */
    int device_record_count = 0; /* 记录数 */
    int record_count = 0; /* 记录数 */
    int send_count = 0;   /* 发送的次数 */
    int query_count = 0;  /* 查询数据统计 */
    DOMElement* ListAccNode = NULL;
    primary_group_t* pPrimaryGroup = NULL;
    int tcp_socket = -1;

    string strSQL = "";
    vector<string> GroupIDVector;
    vector<string> DeviceIDVector;

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() exit---: Route Info Error \r\n");
        return -1;
    }

    if ((NULL == caller_id) || (NULL == strDeviceID) || (NULL == strSN) || (NULL == pRoute_Srv_dboper))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() exit---: Param Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS获取设逻辑备信息,上级为第三方平台:上级CMS ID=%s, IP地址=%s, 端口号=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device info from superior CMS message:Superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

    /* 获取分组行政区域分组列表 */
    GroupIDVector.clear();

    i = AddGblLogicDeviceGroupToVectorForRoute(GroupIDVector);

    /* 获取容器中的设备个数 */
    group_record_count = GroupIDVector.size();

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() group_record_count=%d \r\n", group_record_count);

    /* 获取逻辑设备列表 */
    DeviceIDVector.clear();

    i = AddAllGBLogicDeviceIDToVectorForRoute(DeviceIDVector, pRouteInfo->id, pRouteInfo->three_party_flag, pRouteInfo->link_type, pRoute_Srv_dboper);

    /* 获取容器中的设备个数 */
    device_record_count = DeviceIDVector.size();

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() device_record_count=%d \r\n", device_record_count);

    /* 4、获取总数 */
    record_count = group_record_count + device_record_count;

    DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() record_count=%d \r\n", record_count);

    /* 5、如果记录数为0 */
    if (record_count == 0)
    {
        /* 回复响应,组建消息 */
        CPacket outPacket;
        DOMElement* AccNode = NULL;

        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"Catalog");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"DeviceID");
        outPacket.SetElementValue(AccNode, strDeviceID);

        AccNode = outPacket.CreateElement((char*)"SumNum");
        outPacket.SetElementValue(AccNode, (char*)"0");

        ListAccNode = outPacket.CreateElement((char*)"DeviceList");
        outPacket.SetElementAttr(ListAccNode, (char*)"Num", (char*)"0");

        /* 转发消息给上级CMS */
        i = SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() SIP_SendMessage Error:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() SIP_SendMessage OK:caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
        }

        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "上级CMS获取逻辑设备信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"未查询到数据库记录");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_WARNING, "Access logic device info from superior CMS message failed:Requester ID=%s, IPaddress=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Database record not found");
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() exit---: No Record Count \r\n");
        return i;
    }

    /* 6、循环查找容器，读取用户的设备信息，加入xml中 */
    CPacket* pOutPacket = NULL;

    /* 先采用TCP连接 */
    tcp_socket = CMS_CreateSIPTCPConnect(pRouteInfo->server_ip, pRouteInfo->server_port);

    if (tcp_socket >= 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS获取逻辑设备信息, 连接SIP TCP成功,将通过SIP TCP发送目录Catalog消息到第三方上级CMS:上级CMS ID=%s, IP地址=%s, 端口号=%d, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket);

        for (index = 0; index < record_count; index++)
        {
            /* 如果记录数大于4，则要分次发送 */
            query_count++;

            /* 创建XML头部 */
            i = CreateGBLogicDeviceCatalogResponseXMLHeadForRoute(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

            if (index < group_record_count)
            {
                pPrimaryGroup = get_primary_group((char*)GroupIDVector[index].c_str());

                if (NULL != pPrimaryGroup)
                {
                    /* 加入Item 值 */
                    i = AddLogicDeviceGroupInfoToXMLItemForRoute(pOutPacket, ListAccNode, pPrimaryGroup->group_code, pPrimaryGroup->group_name, pPrimaryGroup->parent_code);
                }
                else
                {
                    /* 加入Item 值 */
                    i = AddLogicDeviceGroupInfoToXMLItemForRoute(pOutPacket, ListAccNode, (char*)"", (char*)"", NULL);
                }
            }
            else if (index >= group_record_count)
            {
                /* 加入Item 值 */
                i = AddLogicDeviceInfoToXMLItemForRoute(pOutPacket, ListAccNode, (char*)DeviceIDVector[index - group_record_count].c_str(), pRouteInfo->three_party_flag, pRoute_Srv_dboper);
            }

            if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 0) || (query_count == record_count))
            {
                if (NULL != pOutPacket)
                {
                    send_count++;

                    /* 转发消息给上级CMS */
                    i |= SIP_SendMessage_By_TCP(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length(), tcp_socket);

                    if (i != 0)
                    {
                        if (i == -2)
                        {
                            SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS获取逻辑设备信息信息, 通过TCP发送Message消息到上级CMS失败,TCP连接异常关闭:上级CMS ID=%s, IP地址=%s, 端口号=%d, tcp_socket=%d, 发送次数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket, send_count);
                            EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logic device info from superior CMS,Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() SIP_SendMessage_By_TCP Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            break;
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS获取逻辑设备信息信息, 通过TCP发送Message消息到上级CMS失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, tcp_socket=%d, 发送次数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket, send_count);
                            EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logic device info from superior CMS,Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() SIP_SendMessage_By_TCP Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        }
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS获取逻辑设备信息信息, 通过TCP发送Message消息到上级CMS成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, tcp_socket=%d, 发送次数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket, send_count);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device info from superior CMS,Send Message Message to superiors CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    }

                    delete pOutPacket;
                    pOutPacket = NULL;
                }
            }
        }

        CMS_CloseSIPTCPConnect(tcp_socket);
    }
    else
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级CMS获取逻辑设备信息, 连接SIP TCP失败,将通过SIP UDP发送目录Catalog消息到第三方上级CMS:上级CMS ID=%s, IP地址=%s, 端口号=%d, tcp_socket=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, tcp_socket);

        for (index = 0; index < record_count; index++)
        {
            /* 如果记录数大于4，则要分次发送 */
            query_count++;

            /* 创建XML头部 */
            i = CreateGBLogicDeviceCatalogResponseXMLHeadForRoute(&pOutPacket, query_count, record_count, strSN, strDeviceID, &ListAccNode);

            if (index < group_record_count)
            {
                pPrimaryGroup = get_primary_group((char*)GroupIDVector[index].c_str());

                if (NULL != pPrimaryGroup)
                {
                    /* 加入Item 值 */
                    i = AddLogicDeviceGroupInfoToXMLItemForRoute(pOutPacket, ListAccNode, pPrimaryGroup->group_code, pPrimaryGroup->group_name, pPrimaryGroup->parent_code);
                }
                else
                {
                    /* 加入Item 值 */
                    i = AddLogicDeviceGroupInfoToXMLItemForRoute(pOutPacket, ListAccNode, (char*)"", (char*)"", NULL);
                }
            }
            else if (index >= group_record_count)
            {
                /* 加入Item 值 */
                i = AddLogicDeviceInfoToXMLItemForRoute(pOutPacket, ListAccNode, (char*)DeviceIDVector[index - group_record_count].c_str(), pRouteInfo->three_party_flag, pRoute_Srv_dboper);
            }

            if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 0) || (query_count == record_count))
            {
                if (NULL != pOutPacket)
                {
                    send_count++;

                    /* 转发消息给上级CMS */
                    i |= SIP_SendMessage(NULL, local_cms_id_get(), caller_id, pRouteInfo->strRegLocalIP, pRouteInfo->iRegLocalPort, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)pOutPacket->GetXml(NULL).c_str(), pOutPacket->GetXml(NULL).length());

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS获取逻辑设备信息信息, 发送Message消息到上级CMS失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 发送次数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, send_count);
                        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logic device info from superior CMS,Send Message Message to superiors CMS failure: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() SIP_SendMessage Error:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS获取逻辑设备信息信息, 发送Message消息到上级CMS成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 发送次数=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, send_count);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device info from superior CMS,Send Message Message to superiors CMS success: the superior CMS, ID = % s = % s IP address, port number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS() SIP_SendMessage OK:dest_id=%s, dest_ip=%s, dest_port=%d \r\n", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    }

                    delete pOutPacket;
                    pOutPacket = NULL;
                }
            }
        }
    }

    if (i == 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS获取逻辑设备信息成功:上级CMS ID=%s, IP地址=%s, 端口号=%d,逻辑设备数目=%d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Access logic device info from superior CMS:Superior CMS, ID = % s = % s IP address, port number = % d, logical device number = % d", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, record_count);
    }
    else
    {
        SystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS获取逻辑设备信息失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 原因=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"发送SIP响应消息失败");
        EnSystemLog(EV9000_CMS_GET_CATALOG_ERROR, EV9000_LOG_LEVEL_ERROR, "Access logic device info from superior CMS failed:Requester ID=%s, IPaddress=%s, port number=%d, cause=%s", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port, (char*)"Send SIP response message failed.");
    }

    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteGetGBDeviceListAndSendCataLogTo3PartyCMS Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : AddLogicDeviceInfoToXMLItemForRoute
 功能描述  : 添加逻辑设备信息到XML的Item
 输入参数  : CPacket* pOutPacket
             DOMElement* ListAccNode
             char* device_id
             int iThreePartyFlag
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月27日 星期六
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int AddLogicDeviceInfoToXMLItemForRoute(CPacket* pOutPacket, DOMElement* ListAccNode, char* device_id, int iThreePartyFlag, DBOper* pRoute_Srv_dboper)
{
    int iRet = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    //GBLogicDevice_info_t* pDBGBLogicDeviceInfo = NULL;
    DOMElement* ItemAccNode = NULL;
    DOMElement* AccNode = NULL;
    DOMElement* ItemInfoNode = NULL;

    char strID[64] = {0};
    char strParental[16] = {0};
    char strSafetyWay[16] = {0};
    char strRegisterWay[16] = {0};
    char strCertifiable[16] = {0};
    char strErrCode[16] = {0};
    char strSecrecy[16] = {0};
    char strPort[16] = {0};
    char strLongitude[64] = {0};
    char strLatitude[64] = {0};
    char strAlarmDeviceSubType[64] = {0};
    char strStreamCount[16] = {0};
    char strFrameCount[16] = {0};
    char strAlarmLengthOfTime[16] = {0};
    char strPTZType[16] = {0};

    if (NULL == pOutPacket || NULL == ListAccNode || NULL == device_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "AddLogicDeviceInfoToXMLItemForRoute() exit---: Param Error \r\n");
        return -1;
    }

    /* 填写XML数据*/
    pOutPacket->SetCurrentElement(ListAccNode);
    ItemAccNode = pOutPacket->CreateElement((char*)"Item");
    pOutPacket->SetCurrentElement(ItemAccNode);

    /* 根据Index 获取逻辑设备信息，可能是只配置了物理设备，还没有上线，数据库和内存中都没有的*/
    pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        if (!iThreePartyFlag) /* 非第三方平台 */
        {
            /* 设备索引 */
            AccNode = pOutPacket->CreateElement((char*)"ID");
            snprintf(strID, 64, "%u", pGBLogicDeviceInfo->id);
            pOutPacket->SetElementValue(AccNode, strID);
        }

        /* 设备统一编号 */
        AccNode = pOutPacket->CreateElement((char*)"DeviceID");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

        /* 点位名称 */
        AccNode = pOutPacket->CreateElement((char*)"Name");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

        if (!iThreePartyFlag) /* 非第三方平台 */
        {
            /* 是否启用*/
            AccNode = pOutPacket->CreateElement((char*)"Enable");

            if (0 == pGBLogicDeviceInfo->enable)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"0");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"1");
            }

            /* 是否可控 */
            AccNode = pOutPacket->CreateElement((char*)"CtrlEnable");

            if (1 == pGBLogicDeviceInfo->ctrl_enable)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Enable");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Disable");
            }

            /* 是否支持对讲 */
            AccNode = pOutPacket->CreateElement((char*)"MicEnable");

            if (0 == pGBLogicDeviceInfo->mic_enable)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Disable");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Enable");
            }

            /* 帧率 */
            AccNode = pOutPacket->CreateElement((char*)"FrameCount");
            snprintf(strFrameCount, 16, "%d", pGBLogicDeviceInfo->frame_count);
            pOutPacket->SetElementValue(AccNode, strFrameCount);

            /* 是否支持多码流 */
            AccNode = pOutPacket->CreateElement((char*)"StreamCount");
            snprintf(strStreamCount, 16, "%d", pGBLogicDeviceInfo->stream_count);
            pOutPacket->SetElementValue(AccNode, strStreamCount);

            /* 告警时长 */
            AccNode = pOutPacket->CreateElement((char*)"AlarmLengthOfTime");
            snprintf(strAlarmLengthOfTime, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
            pOutPacket->SetElementValue(AccNode, strAlarmLengthOfTime);
        }

        /* 设备生产商 */
        AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->manufacturer);

        /* 设备型号 */
        AccNode = pOutPacket->CreateElement((char*)"Model");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->model);

        /* 设备归属 */
        AccNode = pOutPacket->CreateElement((char*)"Owner");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->owner);

        /* 行政区域 */
        AccNode = pOutPacket->CreateElement((char*)"CivilCode");

        if ('\0' == pGBLogicDeviceInfo->civil_code[0])
        {
            pOutPacket->SetElementValue(AccNode, local_civil_code_get());
        }
        else
        {
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->civil_code);
        }

        /* 警区 */
        AccNode = pOutPacket->CreateElement((char*)"Block");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->block);

        /* 安装地址 */
        AccNode = pOutPacket->CreateElement((char*)"Address");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->address);

        /* 是否有子设备 */
        AccNode = pOutPacket->CreateElement((char*)"Parental");
        snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
        pOutPacket->SetElementValue(AccNode, strParental);

        /* 父设备/区域/系统ID, 和其他平台对接的时候，统一使用本级CMS ID */
        AccNode = pOutPacket->CreateElement((char*)"ParentID");

        if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
        {
            pOutPacket->SetElementValue(AccNode, local_cms_id_get());
        }
        else
        {
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->virtualParentID);
        }

        /* 信令安全模式*/
        AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
        snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
        pOutPacket->SetElementValue(AccNode, strSafetyWay);

        /* 注册方式 */
        AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
        snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
        pOutPacket->SetElementValue(AccNode, strRegisterWay);

        /* 证书序列号*/
        AccNode = pOutPacket->CreateElement((char*)"CertNum");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->cert_num);

        /* 证书有效标识 */
        AccNode = pOutPacket->CreateElement((char*)"Certifiable");
        snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
        pOutPacket->SetElementValue(AccNode, strCertifiable);

        /* 无效原因码 */
        AccNode = pOutPacket->CreateElement((char*)"ErrCode");
        snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
        pOutPacket->SetElementValue(AccNode, strErrCode);

        /* 证书终止有效期*/
        AccNode = pOutPacket->CreateElement((char*)"EndTime");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->end_time);

        /* 保密属性 */
        AccNode = pOutPacket->CreateElement((char*)"Secrecy");
        snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
        pOutPacket->SetElementValue(AccNode, strSecrecy);

        /* IP地址*/
        AccNode = pOutPacket->CreateElement((char*)"IPAddress");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->ip_address);

        /* 端口号 */
        AccNode = pOutPacket->CreateElement((char*)"Port");
        snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
        pOutPacket->SetElementValue(AccNode, strPort);

        /* 密码*/
        AccNode = pOutPacket->CreateElement((char*)"Password");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->password);

        /* 点位状态 */
        AccNode = pOutPacket->CreateElement((char*)"Status");

        if (iThreePartyFlag) /* 第三方平台 */
        {
            if (1 == pGBLogicDeviceInfo->status)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"ON");
            }
            else if (2 == pGBLogicDeviceInfo->status)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"VLOST");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"OFF");
            }
        }
        else
        {
            if (1 == pGBLogicDeviceInfo->status)
            {
                if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"INTELLIGENT");
                }
                else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"CLOSE");
                }
                else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"APART");
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"ON");
                }
            }
            else if (2 == pGBLogicDeviceInfo->status)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"NOVIDEO");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"OFF");
            }
        }

        /* 经度 */
        AccNode = pOutPacket->CreateElement((char*)"Longitude");
        snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
        pOutPacket->SetElementValue(AccNode, strLongitude);

        /* 纬度 */
        AccNode = pOutPacket->CreateElement((char*)"Latitude");
        snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
        pOutPacket->SetElementValue(AccNode, strLatitude);

        if (!iThreePartyFlag) /* 非第三方平台 */
        {
            /* 所属图层 */
            AccNode = pOutPacket->CreateElement((char*)"MapLayer");
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->map_layer);

            /* 报警设备子类型 */
            AccNode = pOutPacket->CreateElement((char*)"ChlType");
            snprintf(strAlarmDeviceSubType, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
            pOutPacket->SetElementValue(AccNode, strAlarmDeviceSubType);

            /* 所属的CMS ID */
            AccNode = pOutPacket->CreateElement((char*)"CMSID");
            pOutPacket->SetElementValue(AccNode, local_cms_id_get());

            /* 扩展的Info字段 */
            pOutPacket->SetCurrentElement(ItemAccNode);
            ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
            pOutPacket->SetCurrentElement(ItemInfoNode);

            /* 是否可控 */
            AccNode = pOutPacket->CreateElement((char*)"PTZType");

            if (pGBLogicDeviceInfo->ctrl_enable <= 0)
            {
                snprintf(strPTZType, 16, "%u", 3);
            }
            else
            {
                snprintf(strPTZType, 16, "%u", pGBLogicDeviceInfo->ctrl_enable);
            }

            pOutPacket->SetElementValue(AccNode, strPTZType);
        }
    }
    else
    {
        iRet = load_db_data_to_LogicDevice_info_list_by_device_id(pRoute_Srv_dboper, device_id);

        if (iRet == 0)
        {
            pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

            if (NULL != pGBLogicDeviceInfo)
            {
                if (!iThreePartyFlag) /* 非第三方平台 */
                {
                    /* 设备索引 */
                    AccNode = pOutPacket->CreateElement((char*)"ID");
                    snprintf(strID, 64, "%u", pGBLogicDeviceInfo->id);
                    pOutPacket->SetElementValue(AccNode, strID);
                }

                /* 设备统一编号 */
                AccNode = pOutPacket->CreateElement((char*)"DeviceID");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

                /* 点位名称 */
                AccNode = pOutPacket->CreateElement((char*)"Name");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

                if (!iThreePartyFlag) /* 非第三方平台 */
                {
                    /* 是否启用*/
                    AccNode = pOutPacket->CreateElement((char*)"Enable");

                    if (0 == pGBLogicDeviceInfo->enable)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"0");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"1");
                    }

                    /* 是否可控 */
                    AccNode = pOutPacket->CreateElement((char*)"CtrlEnable");

                    if (1 == pGBLogicDeviceInfo->ctrl_enable)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Enable");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Disable");
                    }

                    /* 是否支持对讲 */
                    AccNode = pOutPacket->CreateElement((char*)"MicEnable");

                    if (0 == pGBLogicDeviceInfo->mic_enable)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Disable");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Enable");
                    }

                    /* 帧率 */
                    AccNode = pOutPacket->CreateElement((char*)"FrameCount");
                    snprintf(strFrameCount, 16, "%d", pGBLogicDeviceInfo->frame_count);
                    pOutPacket->SetElementValue(AccNode, strFrameCount);

                    /* 是否支持多码流 */
                    AccNode = pOutPacket->CreateElement((char*)"StreamCount");
                    snprintf(strStreamCount, 16, "%d", pGBLogicDeviceInfo->stream_count);
                    pOutPacket->SetElementValue(AccNode, strStreamCount);

                    /* 告警时长 */
                    AccNode = pOutPacket->CreateElement((char*)"AlarmLengthOfTime");
                    snprintf(strAlarmLengthOfTime, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
                    pOutPacket->SetElementValue(AccNode, strAlarmLengthOfTime);
                }

                /* 设备生产商 */
                AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->manufacturer);

                /* 设备型号 */
                AccNode = pOutPacket->CreateElement((char*)"Model");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->model);

                /* 设备归属 */
                AccNode = pOutPacket->CreateElement((char*)"Owner");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->owner);

                /* 行政区域 */
                AccNode = pOutPacket->CreateElement((char*)"CivilCode");

                if ('\0' == pGBLogicDeviceInfo->civil_code[0])
                {
                    pOutPacket->SetElementValue(AccNode, local_civil_code_get());
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->civil_code);
                }

                /* 警区 */
                AccNode = pOutPacket->CreateElement((char*)"Block");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->block);

                /* 安装地址 */
                AccNode = pOutPacket->CreateElement((char*)"Address");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->address);

                /* 是否有子设备 */
                AccNode = pOutPacket->CreateElement((char*)"Parental");
                snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
                pOutPacket->SetElementValue(AccNode, strParental);

                /* 父设备/区域/系统ID */
                AccNode = pOutPacket->CreateElement((char*)"ParentID");

                if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
                {
                    pOutPacket->SetElementValue(AccNode, local_cms_id_get());
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->virtualParentID);
                }

                /* 信令安全模式*/
                AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
                snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
                pOutPacket->SetElementValue(AccNode, strSafetyWay);

                /* 注册方式 */
                AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
                snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
                pOutPacket->SetElementValue(AccNode, strRegisterWay);

                /* 证书序列号*/
                AccNode = pOutPacket->CreateElement((char*)"CertNum");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->cert_num);

                /* 证书有效标识 */
                AccNode = pOutPacket->CreateElement((char*)"Certifiable");
                snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
                pOutPacket->SetElementValue(AccNode, strCertifiable);

                /* 无效原因码 */
                AccNode = pOutPacket->CreateElement((char*)"ErrCode");
                snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
                pOutPacket->SetElementValue(AccNode, strErrCode);

                /* 证书终止有效期*/
                AccNode = pOutPacket->CreateElement((char*)"EndTime");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->end_time);

                /* 保密属性 */
                AccNode = pOutPacket->CreateElement((char*)"Secrecy");
                snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
                pOutPacket->SetElementValue(AccNode, strSecrecy);

                /* IP地址*/
                AccNode = pOutPacket->CreateElement((char*)"IPAddress");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->ip_address);

                /* 端口号 */
                AccNode = pOutPacket->CreateElement((char*)"Port");
                snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
                pOutPacket->SetElementValue(AccNode, strPort);

                /* 密码*/
                AccNode = pOutPacket->CreateElement((char*)"Password");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->password);

                /* 点位状态 */
                AccNode = pOutPacket->CreateElement((char*)"Status");

                if (iThreePartyFlag) /* 第三方平台 */
                {
                    if (1 == pGBLogicDeviceInfo->status)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"ON");
                    }
                    else if (2 == pGBLogicDeviceInfo->status)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"VLOST");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"OFF");
                    }
                }
                else
                {
                    if (1 == pGBLogicDeviceInfo->status)
                    {
                        if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"INTELLIGENT");
                        }
                        else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"CLOSE");
                        }
                        else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"APART");
                        }
                        else
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"ON");
                        }
                    }
                    else if (2 == pGBLogicDeviceInfo->status)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"NOVIDEO");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"OFF");
                    }
                }

                /* 经度 */
                AccNode = pOutPacket->CreateElement((char*)"Longitude");
                snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
                pOutPacket->SetElementValue(AccNode, strLongitude);

                /* 纬度 */
                AccNode = pOutPacket->CreateElement((char*)"Latitude");
                snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
                pOutPacket->SetElementValue(AccNode, strLatitude);

                if (!iThreePartyFlag) /* 非第三方平台 */
                {
                    /* 所属图层 */
                    AccNode = pOutPacket->CreateElement((char*)"MapLayer");
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->map_layer);

                    /* 报警设备子类型 */
                    AccNode = pOutPacket->CreateElement((char*)"ChlType");
                    snprintf(strAlarmDeviceSubType, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
                    pOutPacket->SetElementValue(AccNode, strAlarmDeviceSubType);

                    /* 所属的CMS ID */
                    AccNode = pOutPacket->CreateElement((char*)"CMSID");
                    pOutPacket->SetElementValue(AccNode, local_cms_id_get());

                    /* 扩展的Info字段 */
                    pOutPacket->SetCurrentElement(ItemAccNode);
                    ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
                    pOutPacket->SetCurrentElement(ItemInfoNode);

                    /* 是否可控 */
                    AccNode = pOutPacket->CreateElement((char*)"PTZType");

                    if (pGBLogicDeviceInfo->ctrl_enable <= 0)
                    {
                        snprintf(strPTZType, 16, "%u", 3);
                    }
                    else
                    {
                        snprintf(strPTZType, 16, "%u", pGBLogicDeviceInfo->ctrl_enable);
                    }

                    pOutPacket->SetElementValue(AccNode, strPTZType);
                }
            }
            else
            {
                if (!iThreePartyFlag) /* 非第三方平台 */
                {
                    /* 设备索引 */
                    AccNode = pOutPacket->CreateElement((char*)"ID");
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* 设备统一编号 */
                AccNode = pOutPacket->CreateElement((char*)"DeviceID");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 点位名称 */
                AccNode = pOutPacket->CreateElement((char*)"Name");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 设备生产商 */
                AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 设备型号 */
                AccNode = pOutPacket->CreateElement((char*)"Model");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 设备归属 */
                AccNode = pOutPacket->CreateElement((char*)"Owner");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 行政区域 */
                AccNode = pOutPacket->CreateElement((char*)"CivilCode");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 警区 */
                AccNode = pOutPacket->CreateElement((char*)"Block");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 安装地址 */
                AccNode = pOutPacket->CreateElement((char*)"Address");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 是否有子设备 */
                AccNode = pOutPacket->CreateElement((char*)"Parental");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 父设备/区域/系统ID */
                AccNode = pOutPacket->CreateElement((char*)"ParentID");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 信令安全模式*/
                AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 注册方式 */
                AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 证书序列号*/
                AccNode = pOutPacket->CreateElement((char*)"CertNum");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 证书有效标识 */
                AccNode = pOutPacket->CreateElement((char*)"Certifiable");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 无效原因码 */
                AccNode = pOutPacket->CreateElement((char*)"ErrCode");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 证书终止有效期*/
                AccNode = pOutPacket->CreateElement((char*)"EndTime");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 保密属性 */
                AccNode = pOutPacket->CreateElement((char*)"Secrecy");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* IP地址*/
                AccNode = pOutPacket->CreateElement((char*)"IPAddress");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 端口号 */
                AccNode = pOutPacket->CreateElement((char*)"Port");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 密码*/
                AccNode = pOutPacket->CreateElement((char*)"Password");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 点位状态 */
                AccNode = pOutPacket->CreateElement((char*)"Status");
                pOutPacket->SetElementValue(AccNode, (char*)"OFF");

                /* 经度 */
                AccNode = pOutPacket->CreateElement((char*)"Longitude");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 纬度 */
                AccNode = pOutPacket->CreateElement((char*)"Latitude");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                if (!iThreePartyFlag) /* 非第三方平台 */
                {
                    /* 所属的CMS ID */
                    AccNode = pOutPacket->CreateElement((char*)"CMSID");
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* 扩展的Info字段 */
                pOutPacket->SetCurrentElement(ItemAccNode);
                ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
                pOutPacket->SetCurrentElement(ItemInfoNode);

                /* 是否可控 */
                AccNode = pOutPacket->CreateElement((char*)"PTZType");
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }
        }
        else
        {
            if (!iThreePartyFlag) /* 非第三方平台 */
            {
                /* 设备索引 */
                AccNode = pOutPacket->CreateElement((char*)"ID");
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* 设备统一编号 */
            AccNode = pOutPacket->CreateElement((char*)"DeviceID");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 点位名称 */
            AccNode = pOutPacket->CreateElement((char*)"Name");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 设备生产商 */
            AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 设备型号 */
            AccNode = pOutPacket->CreateElement((char*)"Model");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 设备归属 */
            AccNode = pOutPacket->CreateElement((char*)"Owner");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 行政区域 */
            AccNode = pOutPacket->CreateElement((char*)"CivilCode");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 警区 */
            AccNode = pOutPacket->CreateElement((char*)"Block");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 安装地址 */
            AccNode = pOutPacket->CreateElement((char*)"Address");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 是否有子设备 */
            AccNode = pOutPacket->CreateElement((char*)"Parental");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 父设备/区域/系统ID */
            AccNode = pOutPacket->CreateElement((char*)"ParentID");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 信令安全模式*/
            AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 注册方式 */
            AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 证书序列号*/
            AccNode = pOutPacket->CreateElement((char*)"CertNum");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 证书有效标识 */
            AccNode = pOutPacket->CreateElement((char*)"Certifiable");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 无效原因码 */
            AccNode = pOutPacket->CreateElement((char*)"ErrCode");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 证书终止有效期*/
            AccNode = pOutPacket->CreateElement((char*)"EndTime");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 保密属性 */
            AccNode = pOutPacket->CreateElement((char*)"Secrecy");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* IP地址*/
            AccNode = pOutPacket->CreateElement((char*)"IPAddress");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 端口号 */
            AccNode = pOutPacket->CreateElement((char*)"Port");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 密码*/
            AccNode = pOutPacket->CreateElement((char*)"Password");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 点位状态 */
            AccNode = pOutPacket->CreateElement((char*)"Status");
            pOutPacket->SetElementValue(AccNode, (char*)"OFF");

            /* 经度 */
            AccNode = pOutPacket->CreateElement((char*)"Longitude");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 纬度 */
            AccNode = pOutPacket->CreateElement((char*)"Latitude");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            if (!iThreePartyFlag) /* 非第三方平台 */
            {
                /* 所属的CMS ID */
                AccNode = pOutPacket->CreateElement((char*)"CMSID");
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* 扩展的Info字段 */
            pOutPacket->SetCurrentElement(ItemAccNode);
            ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
            pOutPacket->SetCurrentElement(ItemInfoNode);

            /* 是否可控 */
            AccNode = pOutPacket->CreateElement((char*)"PTZType");
            pOutPacket->SetElementValue(AccNode, (char*)"");
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : AddLogicDeviceInfoToXMLItemForRouteNotify
 功能描述  : 添加逻辑设备信息到XML的Item
 输入参数  : CPacket* pOutPacket
             DOMElement* ListAccNode
             char* device_id
             int iThreePartyFlag
             DBOper* pRoute_Srv_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月27日 星期六
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int AddLogicDeviceInfoToXMLItemForRouteNotify(CPacket* pOutPacket, DOMElement* ListAccNode, char* device_id, int iThreePartyFlag, DBOper* pRoute_Srv_dboper)
{
    int iRet = 0;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;
    //GBLogicDevice_info_t* pDBGBLogicDeviceInfo = NULL;
    DOMElement* ItemAccNode = NULL;
    DOMElement* AccNode = NULL;
    DOMElement* ItemInfoNode = NULL;

    char strID[64] = {0};
    char strParental[16] = {0};
    char strSafetyWay[16] = {0};
    char strRegisterWay[16] = {0};
    char strCertifiable[16] = {0};
    char strErrCode[16] = {0};
    char strSecrecy[16] = {0};
    char strPort[16] = {0};
    char strLongitude[64] = {0};
    char strLatitude[64] = {0};
    char strAlarmDeviceSubType[64] = {0};
    char strStreamCount[16] = {0};
    char strFrameCount[16] = {0};
    char strAlarmLengthOfTime[16] = {0};
    char strPTZType[16] = {0};

    if (NULL == pOutPacket || NULL == ListAccNode || NULL == device_id)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "AddLogicDeviceInfoToXMLItemForRouteNotify() exit---: Param Error \r\n");
        return -1;
    }

    /* 填写XML数据*/
    pOutPacket->SetCurrentElement(ListAccNode);
    ItemAccNode = pOutPacket->CreateElement((char*)"Item");
    pOutPacket->SetCurrentElement(ItemAccNode);

    /* 根据Index 获取逻辑设备信息，可能是只配置了物理设备，还没有上线，数据库和内存中都没有的*/
    pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

    if (NULL != pGBLogicDeviceInfo)
    {
        /* 事件类型 */
        AccNode = pOutPacket->CreateElement((char*)"Event");
        pOutPacket->SetElementValue(AccNode, (char*)"ADD");

        if (!iThreePartyFlag) /* 非第三方平台 */
        {
            /* 设备索引 */
            AccNode = pOutPacket->CreateElement((char*)"ID");
            snprintf(strID, 64, "%u", pGBLogicDeviceInfo->id);
            pOutPacket->SetElementValue(AccNode, strID);
        }

        /* 设备统一编号 */
        AccNode = pOutPacket->CreateElement((char*)"DeviceID");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

        /* 点位名称 */
        AccNode = pOutPacket->CreateElement((char*)"Name");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

        if (!iThreePartyFlag) /* 非第三方平台 */
        {
            /* 是否启用*/
            AccNode = pOutPacket->CreateElement((char*)"Enable");

            if (0 == pGBLogicDeviceInfo->enable)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"0");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"1");
            }

            /* 是否可控 */
            AccNode = pOutPacket->CreateElement((char*)"CtrlEnable");

            if (1 == pGBLogicDeviceInfo->ctrl_enable)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Enable");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Disable");
            }

            /* 是否支持对讲 */
            AccNode = pOutPacket->CreateElement((char*)"MicEnable");

            if (0 == pGBLogicDeviceInfo->mic_enable)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Disable");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"Enable");
            }

            /* 帧率 */
            AccNode = pOutPacket->CreateElement((char*)"FrameCount");
            snprintf(strFrameCount, 16, "%d", pGBLogicDeviceInfo->frame_count);
            pOutPacket->SetElementValue(AccNode, strFrameCount);

            /* 是否支持多码流 */
            AccNode = pOutPacket->CreateElement((char*)"StreamCount");
            snprintf(strStreamCount, 16, "%d", pGBLogicDeviceInfo->stream_count);
            pOutPacket->SetElementValue(AccNode, strStreamCount);

            /* 告警时长 */
            AccNode = pOutPacket->CreateElement((char*)"AlarmLengthOfTime");
            snprintf(strAlarmLengthOfTime, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
            pOutPacket->SetElementValue(AccNode, strAlarmLengthOfTime);
        }

        /* 设备生产商 */
        AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->manufacturer);

        /* 设备型号 */
        AccNode = pOutPacket->CreateElement((char*)"Model");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->model);

        /* 设备归属 */
        AccNode = pOutPacket->CreateElement((char*)"Owner");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->owner);

        /* 行政区域 */
        AccNode = pOutPacket->CreateElement((char*)"CivilCode");

        if ('\0' == pGBLogicDeviceInfo->civil_code[0])
        {
            pOutPacket->SetElementValue(AccNode, local_civil_code_get());
        }
        else
        {
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->civil_code);
        }

        /* 警区 */
        AccNode = pOutPacket->CreateElement((char*)"Block");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->block);

        /* 安装地址 */
        AccNode = pOutPacket->CreateElement((char*)"Address");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->address);

        /* 是否有子设备 */
        AccNode = pOutPacket->CreateElement((char*)"Parental");
        snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
        pOutPacket->SetElementValue(AccNode, strParental);

        /* 父设备/区域/系统ID, 和其他平台对接的时候，统一使用本级CMS ID */
        AccNode = pOutPacket->CreateElement((char*)"ParentID");

        if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
        {
            pOutPacket->SetElementValue(AccNode, local_cms_id_get());
        }
        else
        {
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->virtualParentID);
        }

        /* 信令安全模式*/
        AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
        snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
        pOutPacket->SetElementValue(AccNode, strSafetyWay);

        /* 注册方式 */
        AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
        snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
        pOutPacket->SetElementValue(AccNode, strRegisterWay);

        /* 证书序列号*/
        AccNode = pOutPacket->CreateElement((char*)"CertNum");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->cert_num);

        /* 证书有效标识 */
        AccNode = pOutPacket->CreateElement((char*)"Certifiable");
        snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
        pOutPacket->SetElementValue(AccNode, strCertifiable);

        /* 无效原因码 */
        AccNode = pOutPacket->CreateElement((char*)"ErrCode");
        snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
        pOutPacket->SetElementValue(AccNode, strErrCode);

        /* 证书终止有效期*/
        AccNode = pOutPacket->CreateElement((char*)"EndTime");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->end_time);

        /* 保密属性 */
        AccNode = pOutPacket->CreateElement((char*)"Secrecy");
        snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
        pOutPacket->SetElementValue(AccNode, strSecrecy);

        /* IP地址*/
        AccNode = pOutPacket->CreateElement((char*)"IPAddress");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->ip_address);

        /* 端口号 */
        AccNode = pOutPacket->CreateElement((char*)"Port");
        snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
        pOutPacket->SetElementValue(AccNode, strPort);

        /* 密码*/
        AccNode = pOutPacket->CreateElement((char*)"Password");
        pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->password);

        /* 点位状态 */
        AccNode = pOutPacket->CreateElement((char*)"Status");

        if (iThreePartyFlag) /* 第三方平台 */
        {
            if (1 == pGBLogicDeviceInfo->status)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"ON");
            }
            else if (2 == pGBLogicDeviceInfo->status)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"VLOST");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"OFF");
            }
        }
        else
        {
            if (1 == pGBLogicDeviceInfo->status)
            {
                if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"INTELLIGENT");
                }
                else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"CLOSE");
                }
                else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"APART");
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, (char*)"ON");
                }
            }
            else if (2 == pGBLogicDeviceInfo->status)
            {
                pOutPacket->SetElementValue(AccNode, (char*)"NOVIDEO");
            }
            else
            {
                pOutPacket->SetElementValue(AccNode, (char*)"OFF");
            }
        }

        /* 经度 */
        AccNode = pOutPacket->CreateElement((char*)"Longitude");
        snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
        pOutPacket->SetElementValue(AccNode, strLongitude);

        /* 纬度 */
        AccNode = pOutPacket->CreateElement((char*)"Latitude");
        snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
        pOutPacket->SetElementValue(AccNode, strLatitude);

        if (!iThreePartyFlag) /* 非第三方平台 */
        {
            /* 所属图层 */
            AccNode = pOutPacket->CreateElement((char*)"MapLayer");
            pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->map_layer);

            /* 报警设备子类型 */
            AccNode = pOutPacket->CreateElement((char*)"ChlType");
            snprintf(strAlarmDeviceSubType, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
            pOutPacket->SetElementValue(AccNode, strAlarmDeviceSubType);

            /* 所属的CMS ID */
            AccNode = pOutPacket->CreateElement((char*)"CMSID");
            pOutPacket->SetElementValue(AccNode, local_cms_id_get());
        }

        /* 扩展的Info字段 */
        pOutPacket->SetCurrentElement(ItemAccNode);
        ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
        pOutPacket->SetCurrentElement(ItemInfoNode);

        /* 是否可控 */
        AccNode = pOutPacket->CreateElement((char*)"PTZType");

        if (pGBLogicDeviceInfo->ctrl_enable <= 0)
        {
            snprintf(strPTZType, 16, "%u", 3);
        }
        else
        {
            snprintf(strPTZType, 16, "%u", pGBLogicDeviceInfo->ctrl_enable);
        }

        pOutPacket->SetElementValue(AccNode, strPTZType);
    }
    else
    {
        iRet = load_db_data_to_LogicDevice_info_list_by_device_id(pRoute_Srv_dboper, device_id);

        if (iRet == 0)
        {
            pGBLogicDeviceInfo = GBLogicDevice_info_find(device_id);

            if (NULL != pGBLogicDeviceInfo)
            {
                if (!iThreePartyFlag) /* 非第三方平台 */
                {
                    /* 设备索引 */
                    AccNode = pOutPacket->CreateElement((char*)"ID");
                    snprintf(strID, 64, "%u", pGBLogicDeviceInfo->id);
                    pOutPacket->SetElementValue(AccNode, strID);
                }

                /* 设备统一编号 */
                AccNode = pOutPacket->CreateElement((char*)"DeviceID");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_id);

                /* 点位名称 */
                AccNode = pOutPacket->CreateElement((char*)"Name");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->device_name);

                if (!iThreePartyFlag) /* 非第三方平台 */
                {
                    /* 是否启用*/
                    AccNode = pOutPacket->CreateElement((char*)"Enable");

                    if (0 == pGBLogicDeviceInfo->enable)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"0");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"1");
                    }

                    /* 是否可控 */
                    AccNode = pOutPacket->CreateElement((char*)"CtrlEnable");

                    if (1 == pGBLogicDeviceInfo->ctrl_enable)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Enable");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Disable");
                    }

                    /* 是否支持对讲 */
                    AccNode = pOutPacket->CreateElement((char*)"MicEnable");

                    if (0 == pGBLogicDeviceInfo->mic_enable)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Disable");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"Enable");
                    }

                    /* 帧率 */
                    AccNode = pOutPacket->CreateElement((char*)"FrameCount");
                    snprintf(strFrameCount, 16, "%d", pGBLogicDeviceInfo->frame_count);
                    pOutPacket->SetElementValue(AccNode, strFrameCount);

                    /* 是否支持多码流 */
                    AccNode = pOutPacket->CreateElement((char*)"StreamCount");
                    snprintf(strStreamCount, 16, "%d", pGBLogicDeviceInfo->stream_count);
                    pOutPacket->SetElementValue(AccNode, strStreamCount);

                    /* 告警时长 */
                    AccNode = pOutPacket->CreateElement((char*)"AlarmLengthOfTime");
                    snprintf(strAlarmLengthOfTime, 16, "%d", pGBLogicDeviceInfo->alarm_duration);
                    pOutPacket->SetElementValue(AccNode, strAlarmLengthOfTime);
                }

                /* 设备生产商 */
                AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->manufacturer);

                /* 设备型号 */
                AccNode = pOutPacket->CreateElement((char*)"Model");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->model);

                /* 设备归属 */
                AccNode = pOutPacket->CreateElement((char*)"Owner");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->owner);

                /* 行政区域 */
                AccNode = pOutPacket->CreateElement((char*)"CivilCode");

                if ('\0' == pGBLogicDeviceInfo->civil_code[0])
                {
                    pOutPacket->SetElementValue(AccNode, local_civil_code_get());
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->civil_code);
                }

                /* 警区 */
                AccNode = pOutPacket->CreateElement((char*)"Block");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->block);

                /* 安装地址 */
                AccNode = pOutPacket->CreateElement((char*)"Address");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->address);

                /* 是否有子设备 */
                AccNode = pOutPacket->CreateElement((char*)"Parental");
                snprintf(strParental, 16, "%d", pGBLogicDeviceInfo->parental);
                pOutPacket->SetElementValue(AccNode, strParental);

                /* 父设备/区域/系统ID */
                AccNode = pOutPacket->CreateElement((char*)"ParentID");

                if ('\0' == pGBLogicDeviceInfo->virtualParentID[0])
                {
                    pOutPacket->SetElementValue(AccNode, local_cms_id_get());
                }
                else
                {
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->virtualParentID);
                }

                /* 信令安全模式*/
                AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
                snprintf(strSafetyWay, 16, "%d", pGBLogicDeviceInfo->safety_way);
                pOutPacket->SetElementValue(AccNode, strSafetyWay);

                /* 注册方式 */
                AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
                snprintf(strRegisterWay, 16, "%d", pGBLogicDeviceInfo->register_way);
                pOutPacket->SetElementValue(AccNode, strRegisterWay);

                /* 证书序列号*/
                AccNode = pOutPacket->CreateElement((char*)"CertNum");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->cert_num);

                /* 证书有效标识 */
                AccNode = pOutPacket->CreateElement((char*)"Certifiable");
                snprintf(strCertifiable, 16, "%d", pGBLogicDeviceInfo->certifiable);
                pOutPacket->SetElementValue(AccNode, strCertifiable);

                /* 无效原因码 */
                AccNode = pOutPacket->CreateElement((char*)"ErrCode");
                snprintf(strErrCode, 16, "%d", pGBLogicDeviceInfo->error_code);
                pOutPacket->SetElementValue(AccNode, strErrCode);

                /* 证书终止有效期*/
                AccNode = pOutPacket->CreateElement((char*)"EndTime");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->end_time);

                /* 保密属性 */
                AccNode = pOutPacket->CreateElement((char*)"Secrecy");
                snprintf(strSecrecy, 16, "%d", pGBLogicDeviceInfo->secrecy);
                pOutPacket->SetElementValue(AccNode, strSecrecy);

                /* IP地址*/
                AccNode = pOutPacket->CreateElement((char*)"IPAddress");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->ip_address);

                /* 端口号 */
                AccNode = pOutPacket->CreateElement((char*)"Port");
                snprintf(strPort, 16, "%d", pGBLogicDeviceInfo->port);
                pOutPacket->SetElementValue(AccNode, strPort);

                /* 密码*/
                AccNode = pOutPacket->CreateElement((char*)"Password");
                pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->password);

                /* 点位状态 */
                AccNode = pOutPacket->CreateElement((char*)"Status");

                if (iThreePartyFlag) /* 第三方平台 */
                {
                    if (1 == pGBLogicDeviceInfo->status)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"ON");
                    }
                    else if (2 == pGBLogicDeviceInfo->status)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"VLOST");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"OFF");
                    }
                }
                else
                {
                    if (1 == pGBLogicDeviceInfo->status)
                    {
                        if (INTELLIGENT_STATUS_ON == pGBLogicDeviceInfo->intelligent_status)
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"INTELLIGENT");
                        }
                        else if (ALARM_STATUS_CLOSE == pGBLogicDeviceInfo->alarm_status)
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"CLOSE");
                        }
                        else if (ALARM_STATUS_APART == pGBLogicDeviceInfo->alarm_status)
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"APART");
                        }
                        else
                        {
                            pOutPacket->SetElementValue(AccNode, (char*)"ON");
                        }
                    }
                    else if (2 == pGBLogicDeviceInfo->status)
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"NOVIDEO");
                    }
                    else
                    {
                        pOutPacket->SetElementValue(AccNode, (char*)"OFF");
                    }
                }

                /* 经度 */
                AccNode = pOutPacket->CreateElement((char*)"Longitude");
                snprintf(strLongitude, 64, "%.16lf", pGBLogicDeviceInfo->longitude);
                pOutPacket->SetElementValue(AccNode, strLongitude);

                /* 纬度 */
                AccNode = pOutPacket->CreateElement((char*)"Latitude");
                snprintf(strLatitude, 64, "%.16lf", pGBLogicDeviceInfo->latitude);
                pOutPacket->SetElementValue(AccNode, strLatitude);

                if (!iThreePartyFlag) /* 非第三方平台 */
                {
                    /* 所属图层 */
                    AccNode = pOutPacket->CreateElement((char*)"MapLayer");
                    pOutPacket->SetElementValue(AccNode, pGBLogicDeviceInfo->map_layer);

                    /* 报警设备子类型 */
                    AccNode = pOutPacket->CreateElement((char*)"ChlType");
                    snprintf(strAlarmDeviceSubType, 64, "%u", pGBLogicDeviceInfo->alarm_device_sub_type);
                    pOutPacket->SetElementValue(AccNode, strAlarmDeviceSubType);

                    /* 所属的CMS ID */
                    AccNode = pOutPacket->CreateElement((char*)"CMSID");
                    pOutPacket->SetElementValue(AccNode, local_cms_id_get());
                }

                /* 扩展的Info字段 */
                pOutPacket->SetCurrentElement(ItemAccNode);
                ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
                pOutPacket->SetCurrentElement(ItemInfoNode);

                /* 是否可控 */
                AccNode = pOutPacket->CreateElement((char*)"PTZType");

                if (pGBLogicDeviceInfo->ctrl_enable <= 0)
                {
                    snprintf(strPTZType, 16, "%u", 3);
                }
                else
                {
                    snprintf(strPTZType, 16, "%u", pGBLogicDeviceInfo->ctrl_enable);
                }

                pOutPacket->SetElementValue(AccNode, strPTZType);
            }
            else
            {
                if (!iThreePartyFlag) /* 非第三方平台 */
                {
                    /* 设备索引 */
                    AccNode = pOutPacket->CreateElement((char*)"ID");
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* 设备统一编号 */
                AccNode = pOutPacket->CreateElement((char*)"DeviceID");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 点位名称 */
                AccNode = pOutPacket->CreateElement((char*)"Name");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 设备生产商 */
                AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 设备型号 */
                AccNode = pOutPacket->CreateElement((char*)"Model");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 设备归属 */
                AccNode = pOutPacket->CreateElement((char*)"Owner");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 行政区域 */
                AccNode = pOutPacket->CreateElement((char*)"CivilCode");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 警区 */
                AccNode = pOutPacket->CreateElement((char*)"Block");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 安装地址 */
                AccNode = pOutPacket->CreateElement((char*)"Address");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 是否有子设备 */
                AccNode = pOutPacket->CreateElement((char*)"Parental");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 父设备/区域/系统ID */
                AccNode = pOutPacket->CreateElement((char*)"ParentID");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 信令安全模式*/
                AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 注册方式 */
                AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 证书序列号*/
                AccNode = pOutPacket->CreateElement((char*)"CertNum");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 证书有效标识 */
                AccNode = pOutPacket->CreateElement((char*)"Certifiable");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 无效原因码 */
                AccNode = pOutPacket->CreateElement((char*)"ErrCode");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 证书终止有效期*/
                AccNode = pOutPacket->CreateElement((char*)"EndTime");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 保密属性 */
                AccNode = pOutPacket->CreateElement((char*)"Secrecy");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* IP地址*/
                AccNode = pOutPacket->CreateElement((char*)"IPAddress");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 端口号 */
                AccNode = pOutPacket->CreateElement((char*)"Port");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 密码*/
                AccNode = pOutPacket->CreateElement((char*)"Password");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 点位状态 */
                AccNode = pOutPacket->CreateElement((char*)"Status");
                pOutPacket->SetElementValue(AccNode, (char*)"OFF");

                /* 经度 */
                AccNode = pOutPacket->CreateElement((char*)"Longitude");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                /* 纬度 */
                AccNode = pOutPacket->CreateElement((char*)"Latitude");
                pOutPacket->SetElementValue(AccNode, (char*)"");

                if (!iThreePartyFlag) /* 非第三方平台 */
                {
                    /* 所属的CMS ID */
                    AccNode = pOutPacket->CreateElement((char*)"CMSID");
                    pOutPacket->SetElementValue(AccNode, (char*)"");
                }

                /* 扩展的Info字段 */
                pOutPacket->SetCurrentElement(ItemAccNode);
                ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
                pOutPacket->SetCurrentElement(ItemInfoNode);

                /* 是否可控 */
                AccNode = pOutPacket->CreateElement((char*)"PTZType");
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }
        }
        else
        {
            if (!iThreePartyFlag) /* 非第三方平台 */
            {
                /* 设备索引 */
                AccNode = pOutPacket->CreateElement((char*)"ID");
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* 设备统一编号 */
            AccNode = pOutPacket->CreateElement((char*)"DeviceID");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 点位名称 */
            AccNode = pOutPacket->CreateElement((char*)"Name");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 设备生产商 */
            AccNode = pOutPacket->CreateElement((char*)"Manufacturer");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 设备型号 */
            AccNode = pOutPacket->CreateElement((char*)"Model");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 设备归属 */
            AccNode = pOutPacket->CreateElement((char*)"Owner");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 行政区域 */
            AccNode = pOutPacket->CreateElement((char*)"CivilCode");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 警区 */
            AccNode = pOutPacket->CreateElement((char*)"Block");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 安装地址 */
            AccNode = pOutPacket->CreateElement((char*)"Address");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 是否有子设备 */
            AccNode = pOutPacket->CreateElement((char*)"Parental");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 父设备/区域/系统ID */
            AccNode = pOutPacket->CreateElement((char*)"ParentID");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 信令安全模式*/
            AccNode = pOutPacket->CreateElement((char*)"SafetyWay");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 注册方式 */
            AccNode = pOutPacket->CreateElement((char*)"RegisterWay");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 证书序列号*/
            AccNode = pOutPacket->CreateElement((char*)"CertNum");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 证书有效标识 */
            AccNode = pOutPacket->CreateElement((char*)"Certifiable");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 无效原因码 */
            AccNode = pOutPacket->CreateElement((char*)"ErrCode");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 证书终止有效期*/
            AccNode = pOutPacket->CreateElement((char*)"EndTime");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 保密属性 */
            AccNode = pOutPacket->CreateElement((char*)"Secrecy");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* IP地址*/
            AccNode = pOutPacket->CreateElement((char*)"IPAddress");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 端口号 */
            AccNode = pOutPacket->CreateElement((char*)"Port");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 密码*/
            AccNode = pOutPacket->CreateElement((char*)"Password");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 点位状态 */
            AccNode = pOutPacket->CreateElement((char*)"Status");
            pOutPacket->SetElementValue(AccNode, (char*)"OFF");

            /* 经度 */
            AccNode = pOutPacket->CreateElement((char*)"Longitude");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            /* 纬度 */
            AccNode = pOutPacket->CreateElement((char*)"Latitude");
            pOutPacket->SetElementValue(AccNode, (char*)"");

            if (!iThreePartyFlag) /* 非第三方平台 */
            {
                /* 所属的CMS ID */
                AccNode = pOutPacket->CreateElement((char*)"CMSID");
                pOutPacket->SetElementValue(AccNode, (char*)"");
            }

            /* 扩展的Info字段 */
            pOutPacket->SetCurrentElement(ItemAccNode);
            ItemInfoNode = pOutPacket->CreateElement((char*)"Info");
            pOutPacket->SetCurrentElement(ItemInfoNode);

            /* 是否可控 */
            AccNode = pOutPacket->CreateElement((char*)"PTZType");
            pOutPacket->SetElementValue(AccNode, (char*)"");
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : AddLogicDeviceGroupInfoToXMLItemForRoute
 功能描述  : 添加逻辑设备分组信息到Catalog XML中
 输入参数  : CPacket* pOutPacket
             DOMElement* ListAccNode
             char* group_code
             char* group_name
             char* parent_code
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月6日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddLogicDeviceGroupInfoToXMLItemForRoute(CPacket* pOutPacket, DOMElement* ListAccNode, char* group_code, char* group_name, char* parent_code)
{
    DOMElement* ItemAccNode = NULL;
    DOMElement* AccNode = NULL;

    if (NULL == pOutPacket || NULL == ListAccNode || NULL == group_code || NULL == group_name)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "AddLogicDeviceGroupInfoToXMLItemForRoute() exit---: Param Error \r\n");
        return -1;
    }

    /* 填写XML数据*/
    pOutPacket->SetCurrentElement(ListAccNode);
    ItemAccNode = pOutPacket->CreateElement((char*)"Item");
    pOutPacket->SetCurrentElement(ItemAccNode);

    /* 设备统一编号 */
    AccNode = pOutPacket->CreateElement((char*)"DeviceID");
    pOutPacket->SetElementValue(AccNode, group_code);

    /* 点位名称 */
    AccNode = pOutPacket->CreateElement((char*)"Name");
    pOutPacket->SetElementValue(AccNode, group_name);

    /* 父设备/区域/系统ID, 和其他平台对接的时候，统一使用本级CMS ID */
    if (NULL != parent_code && parent_code[0] != '\0')
    {
        AccNode = pOutPacket->CreateElement((char*)"ParentID");
        pOutPacket->SetElementValue(AccNode, parent_code);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : CreateGBLogicDeviceCatalogResponseXMLHead
 功能描述  : 创建上级CMS过来的获取逻辑设备列表回应消息XML头部
 输入参数  : CPacket** pOutPacket
             int query_count
             int record_count
             char* strSN
             char* strDeviceID
             DOMElement** ListAccNode
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月27日 星期六
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int CreateGBLogicDeviceCatalogResponseXMLHeadForRoute(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, DOMElement** ListAccNode)
{
    DOMElement* AccNode = NULL;

    char strSumNum[32] = {0};
    char strRecordCount[32] = {0};

    snprintf(strSumNum, 32, "%d", record_count);

    /* 添加发送的xml头部 */
    if (query_count == 1)
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateGBLogicDeviceCatalogResponseXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        if (record_count <= MAX_ROUTE_CATALOG_COUT_SEND)
        {
            snprintf(strRecordCount, 32, "%d", record_count);
        }
        else
        {
            snprintf(strRecordCount, 32, "%d", MAX_ROUTE_CATALOG_COUT_SEND);
        }

        (*pOutPacket)->SetRootTag("Response");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"Catalog");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");
        (*pOutPacket)->SetElementValue(AccNode, strSN);

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");
        (*pOutPacket)->SetElementValue(AccNode, strDeviceID);

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"DeviceList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }
    else if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 1) && (record_count - query_count >= MAX_ROUTE_CATALOG_COUT_SEND))
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateGBLogicDeviceCatalogResponseXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        snprintf(strRecordCount, 32, "%d", MAX_ROUTE_CATALOG_COUT_SEND);
        (*pOutPacket)->SetRootTag("Response");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"Catalog");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");
        (*pOutPacket)->SetElementValue(AccNode, strSN);

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");
        (*pOutPacket)->SetElementValue(AccNode, strDeviceID);

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"DeviceList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }
    else if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 1) && (record_count - query_count < MAX_ROUTE_CATALOG_COUT_SEND))
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateGBLogicDeviceCatalogResponseXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        snprintf(strRecordCount, 32, "%d", record_count - query_count + 1);
        (*pOutPacket)->SetRootTag("Response");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"Catalog");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");
        (*pOutPacket)->SetElementValue(AccNode, strSN);

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");
        (*pOutPacket)->SetElementValue(AccNode, strDeviceID);

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"DeviceList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : CreateGBLogicDeviceCatalogNotifyXMLHeadForRoute
 功能描述  : 创建逻辑设备列表通知消息XML头部
 输入参数  : CPacket** pOutPacket
             int query_count
             int record_count
             char* strSN
             char* strDeviceID
             DOMElement** ListAccNode
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年7月27日 星期六
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int CreateGBLogicDeviceCatalogNotifyXMLHeadForRoute(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, DOMElement** ListAccNode)
{
    DOMElement* AccNode = NULL;

    char strSumNum[32] = {0};
    char strRecordCount[32] = {0};

    snprintf(strSumNum, 32, "%d", record_count);

    /* 添加发送的xml头部 */
    if (query_count == 1)
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateGBLogicDeviceCatalogNotifyXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        if (record_count <= MAX_ROUTE_CATALOG_COUT_SEND)
        {
            snprintf(strRecordCount, 32, "%d", record_count);
        }
        else
        {
            snprintf(strRecordCount, 32, "%d", MAX_ROUTE_CATALOG_COUT_SEND);
        }

        (*pOutPacket)->SetRootTag("Notify");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"Catalog");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");
        (*pOutPacket)->SetElementValue(AccNode, strSN);

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");
        (*pOutPacket)->SetElementValue(AccNode, strDeviceID);

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"DeviceList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }
    else if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 1) && (record_count - query_count >= MAX_ROUTE_CATALOG_COUT_SEND))
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateGBLogicDeviceCatalogNotifyXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        snprintf(strRecordCount, 32, "%d", MAX_ROUTE_CATALOG_COUT_SEND);
        (*pOutPacket)->SetRootTag("Notify");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"Catalog");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");
        (*pOutPacket)->SetElementValue(AccNode, strSN);

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");
        (*pOutPacket)->SetElementValue(AccNode, strDeviceID);

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"DeviceList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }
    else if ((query_count % MAX_ROUTE_CATALOG_COUT_SEND == 1) && (record_count - query_count < MAX_ROUTE_CATALOG_COUT_SEND))
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateGBLogicDeviceCatalogNotifyXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        snprintf(strRecordCount, 32, "%d", record_count - query_count + 1);
        (*pOutPacket)->SetRootTag("Notify");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"Catalog");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");
        (*pOutPacket)->SetElementValue(AccNode, strSN);

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");
        (*pOutPacket)->SetElementValue(AccNode, strDeviceID);

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"DeviceList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : CreateRecordInfoQueryResponseXMLHeadForRoute
 功能描述  : 创建获取录像文件查询回应XML头部
 输入参数  : CPacket** pOutPacket
             int query_count
             int record_count
             char* strSN
             char* strDeviceID
             char* strDeviceName
             DOMElement** ListAccNode
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月6日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int CreateRecordInfoQueryResponseXMLHeadForRoute(CPacket** pOutPacket, int query_count, int record_count, char* strSN, char* strDeviceID, char* strDeviceName, DOMElement** ListAccNode)
{
    DOMElement* AccNode = NULL;

    char strSumNum[32] = {0};
    char strRecordCount[32] = {0};

    snprintf(strSumNum, 32, "%d", record_count);

    /* 添加发送的xml头部 */
    if (query_count == 1)
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateRecordInfoQueryResponseXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        if (record_count <= MAX_ROUTE_RECORD_INFO_COUT_SEND)
        {
            snprintf(strRecordCount, 32, "%d", record_count);
        }
        else
        {
            snprintf(strRecordCount, 32, "%d", MAX_ROUTE_RECORD_INFO_COUT_SEND);
        }

        (*pOutPacket)->SetRootTag("Response");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");

        if (NULL != strSN)
        {
            (*pOutPacket)->SetElementValue(AccNode, strSN);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)"");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");

        if (NULL != strDeviceID)
        {
            (*pOutPacket)->SetElementValue(AccNode, strDeviceID);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)"");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"Name");

        if (NULL != strDeviceName)
        {
            (*pOutPacket)->SetElementValue(AccNode, strDeviceName);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)" ");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"RecordList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);
    }
    else if ((query_count % MAX_ROUTE_RECORD_INFO_COUT_SEND == 1) && (record_count - query_count >= MAX_ROUTE_RECORD_INFO_COUT_SEND))
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateRecordInfoQueryResponseXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        snprintf(strRecordCount, 32, "%d", MAX_ROUTE_RECORD_INFO_COUT_SEND);
        (*pOutPacket)->SetRootTag("Response");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");

        if (NULL != strSN)
        {
            (*pOutPacket)->SetElementValue(AccNode, strSN);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)"");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");

        if (NULL != strDeviceID)
        {
            (*pOutPacket)->SetElementValue(AccNode, strDeviceID);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)"");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"Name");

        if (NULL != strDeviceName)
        {
            (*pOutPacket)->SetElementValue(AccNode, strDeviceName);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)" ");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"RecordList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);

    }
    else if ((query_count % MAX_ROUTE_RECORD_INFO_COUT_SEND == 1) && (record_count - query_count < MAX_ROUTE_RECORD_INFO_COUT_SEND))
    {
        *pOutPacket = new CPacket();

        if (NULL == *pOutPacket)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "CreateRecordInfoQueryResponseXMLHeadForRoute() exit---: Create XML Packet Error \r\n");
            return -1;
        }

        snprintf(strRecordCount, 32, "%d", record_count - query_count + 1);
        (*pOutPacket)->SetRootTag("Response");

        AccNode = (*pOutPacket)->CreateElement((char*)"CmdType");
        (*pOutPacket)->SetElementValue(AccNode, (char*)"RecordInfo");

        AccNode = (*pOutPacket)->CreateElement((char*)"SN");

        if (NULL != strSN)
        {
            (*pOutPacket)->SetElementValue(AccNode, strSN);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)"");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"DeviceID");

        if (NULL != strDeviceID)
        {
            (*pOutPacket)->SetElementValue(AccNode, strDeviceID);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)"");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"Name");

        if (NULL != strDeviceName)
        {
            (*pOutPacket)->SetElementValue(AccNode, strDeviceName);
        }
        else
        {
            (*pOutPacket)->SetElementValue(AccNode, (char*)" ");
        }

        AccNode = (*pOutPacket)->CreateElement((char*)"SumNum");
        (*pOutPacket)->SetElementValue(AccNode, strSumNum);

        (*ListAccNode) = (*pOutPacket)->CreateElement((char*)"RecordList");
        (*pOutPacket)->SetElementAttr((*ListAccNode), (char*)"Num", strRecordCount);

    }

    return 0;
}

/*****************************************************************************
 函 数 名  : AddRecordInfoToXMLItemForRoute
 功能描述  : 互联路由添加录像文件信息到XML的Item
 输入参数  : CPacket* pOutPacket
             DOMElement* ListAccNode
             VideoRecord& stVideoRecord
             char* strDeviceID
             char* strDeviceName
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月15日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int AddRecordInfoToXMLItemForRoute(CPacket* pOutPacket, DOMElement* ListAccNode, VideoRecord& stVideoRecord, char* strDeviceID, char* strDeviceName)
{
    DOMElement* ItemAccNode = NULL;
    DOMElement* AccNode = NULL;

    //char strStorageIndex[32] = {0};
    char strStartTime[32] = {0};
    char strEndTime[32] = {0};
    char str_date[12] = {0};
    char str_time[12] = {0};
    //char strSize[32] = {0};
    time_t sStartTime;
    time_t sStopTime;
    struct tm local_start_time = { 0 };
    struct tm local_stop_time = { 0 };

    if (NULL == pOutPacket || NULL == ListAccNode)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "AddRecordInfoToXMLItemForRoute() exit---: Param Error \r\n");
        return -1;
    }

    /* 填写XML数据*/
    pOutPacket->SetCurrentElement(ListAccNode);
    ItemAccNode = pOutPacket->CreateElement((char*)"Item");
    pOutPacket->SetCurrentElement(ItemAccNode);

    AccNode = pOutPacket->CreateElement((char*)"DeviceID");

    if (NULL != strDeviceID)
    {
        pOutPacket->SetElementValue(AccNode, strDeviceID);
    }
    else
    {
        pOutPacket->SetElementValue(AccNode, (char*)"");
    }

    AccNode = pOutPacket->CreateElement((char*)"Name");

    if (NULL != strDeviceName)
    {
        pOutPacket->SetElementValue(AccNode, strDeviceName);
    }
    else
    {
        pOutPacket->SetElementValue(AccNode, (char*)" ");
    }

#if 0
    AccNode = pOutPacket->CreateElement((char*)"StorageIndex");
    snprintf(strStorageIndex, 32, "%d", stVideoRecord.StorageIndex);
    pOutPacket->SetElementValue(AccNode, strStorageIndex);
#endif

    AccNode = pOutPacket->CreateElement((char*)"StartTime");
    sStartTime = stVideoRecord.StartTime;
    localtime_r(&sStartTime, &local_start_time);
    memset(str_date, 0, 12);
    memset(str_time, 0, 12);
    memset(strStartTime, 0, 32);
    strftime(str_date, sizeof(str_date), "%Y-%m-%d", &local_start_time);
    strftime(str_time, sizeof(str_time), "%H:%M:%S", &local_start_time);
    snprintf(strStartTime, 32, "%sT%s", str_date, str_time);
    //DEBUG_TRACE(MODULE_USER, LOG_INFO,"AddDeviceGroupConfigToXMLItem() strStartTime=%s \r\n", strStartTime);
    pOutPacket->SetElementValue(AccNode, strStartTime);

    AccNode = pOutPacket->CreateElement((char*)"EndTime");
    sStopTime = stVideoRecord.StopTime;
    localtime_r(&sStopTime, &local_stop_time);
    memset(str_date, 0, 12);
    memset(str_time, 0, 12);
    memset(strEndTime, 0, 32);
    strftime(str_date, sizeof(str_date), "%Y-%m-%d", &local_stop_time);
    strftime(str_time, sizeof(str_time), "%H:%M:%S", &local_stop_time);
    snprintf(strEndTime, 32, "%sT%s", str_date, str_time);
    //DEBUG_TRACE(MODULE_USER, LOG_INFO,"AddDeviceGroupConfigToXMLItem() strEndTime=%s \r\n", strEndTime);
    pOutPacket->SetElementValue(AccNode, strEndTime);

#if 0
    AccNode = pOutPacket->CreateElement((char*)"Size");
    snprintf(strSize, 32, "%d", stVideoRecord.Size);
    pOutPacket->SetElementValue(AccNode, strSize);

    AccNode = pOutPacket->CreateElement((char*)"StorageIP");

    if (stVideoRecord.StorageIP.length() > 0)
    {
        pOutPacket->SetElementValue(AccNode, (char*)stVideoRecord.StorageIP.c_str());
    }
    else
    {
        pOutPacket->SetElementValue(AccNode, (char*)"");
    }

    AccNode = pOutPacket->CreateElement((char*)"FilePath");

    if (stVideoRecord.StoragePath.length() > 0)
    {
        pOutPacket->SetElementValue(AccNode, (char*)stVideoRecord.StoragePath.c_str());
    }
    else
    {
        pOutPacket->SetElementValue(AccNode, (char*)"");
    }

    AccNode = pOutPacket->CreateElement((char*)"Address");
    pOutPacket->SetElementValue(AccNode, (char*)"Address 1");
#endif

    AccNode = pOutPacket->CreateElement((char*)"Secrecy");
    pOutPacket->SetElementValue(AccNode, (char*)"0");

    AccNode = pOutPacket->CreateElement((char*)"Type");
    pOutPacket->SetElementValue(AccNode, (char*)"time");

    return 0;
}

/*****************************************************************************
 函 数 名  : checkIfHasDBRefresh
 功能描述  : 检测数据库是否正在更新
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年2月20日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int checkIfHasDBRefresh()
{
    if (db_GBLogicDeviceInfo_reload_mark
        || db_GBDeviceInfo_reload_mark
        || db_GroupInfo_reload_mark
        || db_GroupMapInfo_reload_mark)
    {
        return 1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : RouteLockDeviceProc
 功能描述  : 上级平台锁定操作处理
 输入参数  : char* strLockCmd
             char* strDeviceID
             route_info_t* pRouteInfo
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年4月7日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int RouteLockDeviceProc(char* strLockCmd, char* strDeviceID, route_info_t* pRouteInfo)
{
    int i = 0;
    user_info_t* pOldUserInfo = NULL;
    GBLogicDevice_info_t* pGBLogicDeviceInfo = NULL;

    if (NULL == strLockCmd || NULL == strDeviceID || NULL == pRouteInfo)
    {
        return -1;
    }

    pGBLogicDeviceInfo = GBLogicDevice_info_find(strDeviceID);

    if (NULL != pGBLogicDeviceInfo)
    {
        if (0 == sstrcmp(strLockCmd, (char*)"Lock")) /* 锁定 */
        {
            if (LOCK_STATUS_OFF == pGBLogicDeviceInfo->lock_status) /* 没有锁定 */
            {
                pGBLogicDeviceInfo->lock_status = LOCK_STATUS_ROUTE_LOCK;
                pGBLogicDeviceInfo->pLockUserInfo = NULL;
                pGBLogicDeviceInfo->pLockRouteInfo = pRouteInfo;

                i = device_auto_unlock_use(pGBLogicDeviceInfo->id);

                if (i != 0)
                {
                    SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 锁定点位失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"添加到自动解锁队列失败");
                    EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, lock point failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Adding to the list of auto unlock failed.");
                    DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_use Error \r\n");
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设备控制命令处理, 锁定点位成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS, lock point success:Superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_use OK \r\n");
                }
            }
            else if (LOCK_STATUS_USER_LOCK == pGBLogicDeviceInfo->lock_status) /* 已经被用户锁定 */
            {
                if (NULL != pGBLogicDeviceInfo->pLockUserInfo) /* 原有用户锁定存在 */
                {
                    /* 上级平台直接抢占本级用户锁定权限 */
                    pOldUserInfo = pGBLogicDeviceInfo->pLockUserInfo;

                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_ROUTE_LOCK;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = pRouteInfo;

                    i = device_auto_unlock_use(pGBLogicDeviceInfo->id);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 锁定点位失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"添加到自动解锁队列失败");
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, lock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Adding to the list of auto unlock failed.");
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_use Error \r\n");
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的设备控制命令处理, 抢占锁定原有锁定点位:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原有锁定用户ID=%s, 原有锁定用户IP地址=%s, 原有锁定用户端口号=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pOldUserInfo->user_id, pOldUserInfo->login_ip, pOldUserInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device control command process from superior CMS,Preemption lock  original locking point: original superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, original lock user ID = % s, original lock users IP address = % s, locking client old slogan = % d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pOldUserInfo->user_id, pOldUserInfo->login_ip, pOldUserInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_use OK \r\n");
                    }
                }
                else /* 原有用户锁定无效 */
                {
                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_ROUTE_LOCK;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = pRouteInfo;

                    i = device_auto_unlock_use(pGBLogicDeviceInfo->id);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 锁定点位失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"添加到自动解锁队列失败");
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, lock point failed:Requester ID=%s, IP address=%s, port number=%d, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Adding to the list of auto unlock failed.");
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_use Error \r\n");
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的设备控制命令处理, 原有锁定用户失效, 锁定点位成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device control command process from superior CMS, original lock users failure ,Locking point success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_use OK \r\n");
                    }
                }
            }
            else if (LOCK_STATUS_ROUTE_LOCK == pGBLogicDeviceInfo->lock_status) /* 已经被上级锁定 */
            {
                if (NULL != pGBLogicDeviceInfo->pLockRouteInfo) /* 原有上级锁定存在 */
                {
                    /* 判断上级是否有效 */
                    if (0 == pGBLogicDeviceInfo->pLockRouteInfo->reg_status) /* 原有上级锁定无效 */
                    {
                        /* 更改为新的上级锁定 */
                        pGBLogicDeviceInfo->lock_status = LOCK_STATUS_ROUTE_LOCK;
                        pGBLogicDeviceInfo->pLockUserInfo = NULL;
                        pGBLogicDeviceInfo->pLockRouteInfo = pRouteInfo;

                        i = device_auto_unlock_use(pGBLogicDeviceInfo->id);

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 锁定点位失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"添加到自动解锁队列失败");
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, lock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Adding to the list of auto unlock failed.");
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_use Error \r\n");
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的设备控制命令处理, 原有上级锁定信息失效，更改为用户锁定点位成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device control command process from superior CMS, Original superior lock information failure, change the lock for the user level success: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_use OK \r\n");
                        }
                    }
                    else /* 有效 */
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 锁定点位失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 锁定的上级CMS ID=%s, 锁定的上级CMS IP地址=%s, 锁定的上级CMS 端口号=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"点位已经被上级平台锁定", pGBLogicDeviceInfo->pLockRouteInfo->server_id, pGBLogicDeviceInfo->pLockRouteInfo->server_ip, pGBLogicDeviceInfo->pLockRouteInfo->server_port);
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, lock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Device Has locked by route CMS");
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_use Error \r\n");
                    }
                }
                else /* 原有上级锁定无效 */
                {
                    /* 更改为上级锁定 */
                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_ROUTE_LOCK;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = pRouteInfo;

                    i = device_auto_unlock_use(pGBLogicDeviceInfo->id);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 锁定点位失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"添加到自动解锁队列失败");
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, lock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Adding to the list of auto unlock failed.");
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_use Error \r\n");
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的设备控制命令处理, 原有上级锁定信息失效，重新锁定点位成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, " Device control command process from superior CMS, the original superior locking information failure, Locking point success again: locking point superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_use OK \r\n");
                    }
                }
            }
        }
        else if (0 == sstrcmp(strLockCmd, (char*)"UnLock")) /* 解除锁定 */
        {
            if (LOCK_STATUS_USER_LOCK == pGBLogicDeviceInfo->lock_status) /* 已经被用户锁定 */
            {
                if (NULL != pGBLogicDeviceInfo->pLockUserInfo) /* 判断用户锁定是否有效 */
                {
                    /* 上级平台直接解锁本级用户锁定权限 */
                    pOldUserInfo = pGBLogicDeviceInfo->pLockUserInfo;

                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = NULL;

                    i = device_auto_unlock_remove(pGBLogicDeviceInfo->id);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 解除锁定点位失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"从自动解锁队列移除失败");
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, unlock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Remove from the list of auto unlock failed.");
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_remove Error \r\n");
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的设备控制命令处理, 解除原有用户锁定的点位成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原有锁定的用户ID=%s, 锁定的用户IP地址=%s, 锁定的用户端口号=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pOldUserInfo->user_id, pOldUserInfo->login_ip, pOldUserInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device control command process from superior CMS, Remove the original user locking point success: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, the original lock user ID = % s, lock the user IP address = % s, locking the client number ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pOldUserInfo->user_id, pOldUserInfo->login_ip, pOldUserInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_remove OK \r\n");
                    }
                }
                else /* 原有用户锁定无效 */
                {
                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = NULL;

                    i = device_auto_unlock_remove(pGBLogicDeviceInfo->id);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 解除锁定点位失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"从自动解锁队列移除失败");
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, unlock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Remove from the list of auto unlock failed.");
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_remove Error \r\n");
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的设备控制命令处理, 原有锁定用户已经无效, 解除锁定点位成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device control command process from superior CMS, Remove the original user locking point success: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, the original lock user ID = % s, lock the user IP address = % s, locking the client number ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pOldUserInfo->user_id, pOldUserInfo->login_ip, pOldUserInfo->login_port);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_remove OK \r\n");
                    }
                }
            }
            else if (LOCK_STATUS_ROUTE_LOCK == pGBLogicDeviceInfo->lock_status)
            {
                if (NULL != pGBLogicDeviceInfo->pLockRouteInfo) /* 已经被上级锁定 */
                {
                    /* 判断是否锁定的是否就是该上级Route */
                    if (pGBLogicDeviceInfo->pLockRouteInfo == pRouteInfo)
                    {
                        pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                        pGBLogicDeviceInfo->pLockUserInfo = NULL;
                        pGBLogicDeviceInfo->pLockRouteInfo = NULL;

                        i = device_auto_unlock_remove(pGBLogicDeviceInfo->id);

                        if (i != 0)
                        {
                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 解除锁定点位失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"从自动解锁队列移除失败");
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, unlock point failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Remove from the list of auto unlock failed.");
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_remove Error \r\n");
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级CMS过来的设备控制命令处理, 解除锁定点位成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device control command process from superior CMS, Remove the original user locking point success: the superior CMS, ID = % s = % s IP address, port number = % d, logical device ID = % s, the original lock user ID = % s, lock the user IP address = % s, locking the client number ", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, pOldUserInfo->user_id, pOldUserInfo->login_ip, pOldUserInfo->login_port);
                            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_remove OK \r\n");
                        }
                    }
                    else /* 其他上级锁定 */
                    {
                        /* 判断原有上级是否有效 */
                        if (0 == pGBLogicDeviceInfo->pLockRouteInfo->reg_status) /* 原有上级锁定无效 */
                        {
                            pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                            pGBLogicDeviceInfo->pLockUserInfo = NULL;
                            pGBLogicDeviceInfo->pLockRouteInfo = NULL;

                            i = device_auto_unlock_remove(pGBLogicDeviceInfo->id);

                            if (i != 0)
                            {
                                SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 解除锁定点位失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"从自动解锁队列移除失败");
                                EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, unlock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Remove from the list of auto unlock failed.");
                                DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_remove Error \r\n");
                            }
                            else
                            {
                                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的设备控制命令处理, 原有锁定上级平台已经无效, 解除锁定点位成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device control command process from superior CMS, Locking the superior platform is invalid, unlocked original point success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = %s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_remove OK \r\n");
                            }
                        }
                        else /* 有效 */
                        {
                            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 解除锁定点位失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s, 锁定的上级CMS ID=%s, 锁定的上级CMS IP地址=%s, 锁定的上级CMS 端口号=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"点位已经被其他上级CMS锁定", pGBLogicDeviceInfo->pLockRouteInfo->server_id, pGBLogicDeviceInfo->pLockRouteInfo->server_ip, pGBLogicDeviceInfo->pLockRouteInfo->server_port);
                            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, lock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Device Has locked by high CMS");
                            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_use Error \r\n");
                        }
                    }
                }
                else /* 原有上级锁定无效 */
                {
                    pGBLogicDeviceInfo->lock_status = LOCK_STATUS_OFF;
                    pGBLogicDeviceInfo->pLockUserInfo = NULL;
                    pGBLogicDeviceInfo->pLockRouteInfo = NULL;

                    i = device_auto_unlock_remove(pGBLogicDeviceInfo->id);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 解除锁定点位失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"从自动解锁队列移除失败");
                        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, unlock point failed:Requester ID=%s, IP address=%s, port number=%d, Logic Device ID=%s, reason=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, (char*)"Remove from the list of auto unlock failed.");
                        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() device_auto_unlock_remove Error \r\n");
                    }
                    else
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的设备控制命令处理, 原有锁定用户已经无效, 解除锁定点位成功:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device control command process from superior CMS, Unlocked point success: the higher the CMS, ID = % s = % s IP address, port number = % d, logical device ID = %s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
                        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "RouteLockDeviceProc() device_auto_unlock_remove OK \r\n");
                    }
                }
            }
            else if (LOCK_STATUS_OFF == pGBLogicDeviceInfo->lock_status) /* 没有锁定 */
            {

            }
        }
        else
        {
            SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 锁定点位失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因=不支持的锁定命令:LockCmd=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, strLockCmd);
            EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, Lock point failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=unsupportable lock command:LockCmd=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, strLockCmd);
            DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() Lock Cmd Error: LockCmd=%s \r\n", strLockCmd);
            return -1;
        }
    }
    else
    {
        SystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "上级CMS过来的设备控制命令处理, 锁定点位失败:上级CMS ID=%s, IP地址=%s, 端口号=%d, 逻辑设备ID=%s, 原因:DeviceID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID);
        EnSystemLog(EV9000_CMS_DEVICE_CONTROL_ERROR, EV9000_LOG_LEVEL_ERROR, "Device control command process from superior CMS, Lock point failed:Requester ID=%s, IP address=%s, port number=%d, logic device ID=%s, cause=find logical device failed:DeviceID=%s", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, strDeviceID, strDeviceID);
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "RouteLockDeviceProc() exit---: Find GB LogicDevice Info Error: DeviceID=%s \r\n", strDeviceID);
        return -1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : StopRouteService
 功能描述  : 停止呼叫路由任务
 输入参数  : route_info_t* pRouteInfo
             char * device_id
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
int StopRouteService(route_info_t* pRouteInfo, char * device_id, int stream_type)
{
    int i = 0;
    int cr_pos = -1;
    int other_cr_pos = -1;
    cr_t* pOtherCrData = NULL;
    cr_t* pCrData = NULL;

    if ((NULL == pRouteInfo) || (NULL == device_id))
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_DEBUG, "StopRouteService() exit---: Param Error \r\n");
        return -1;
    }

    /* 1、根据DEC 端的Dialog Index 查找呼叫记录信息 */
    cr_pos = call_record_find_by_callerinfo_and_calleeid(pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_id, stream_type);
    //DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "StopDecService() call_record_find_by_callerid_and_calleeid:cr_pos=%d \r\n", cr_pos);

    if (cr_pos < 0)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "StopRouteService() exit---: Not Find Call Record Info:server_id=%s, server_ip=%s, server_port=%d, device_id=%s, stream_type=%d, cr_pos=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, device_id, stream_type, cr_pos);
        return -2;
    }

    pCrData = call_record_get(cr_pos);

    if (NULL == pCrData)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "StopRouteService() exit---: Get Call Record Error \r\n");
        return -1;
    }

    /* 3、通知TSU停止接收码流*/
    if ((CALL_TYPE_RECORD_PLAY == pCrData->call_type)
        || (CALL_TYPE_DOWNLOAD == pCrData->call_type))
    {
        i = notify_tsu_delete_replay_task(pCrData->tsu_ip, pCrData->task_id);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "StopRouteService() notify_tsu_delete_replay_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "StopRouteService() notify_tsu_delete_replay_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }
    }
    else
    {
        i = notify_tsu_delete_transfer_task(pCrData->tsu_ip, pCrData->task_id);

        if (i < 0)
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_WARN, "StopRouteService() notify_tsu_delete_transfer_task Error:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "StopRouteService() notify_tsu_delete_transfer_task OK:tsu_ip=%s, task_id=%s, i=%d \r\n", pCrData->tsu_ip, pCrData->task_id, i);
        }
    }

    /* 4、发送Bye 给主叫侧 */
    i = SIP_SendBye(pCrData->caller_ua_index);
    DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "StopRouteService() SIP_SendBye To Caller:caller_ua_index=%d, i=%d \r\n", pCrData->caller_ua_index, i);

    /* 看是否有前端连接 */
    if (pCrData->callee_ua_index >= 0)
    {
        /* 查看是否有其他客户端业务 */
        other_cr_pos = is_GBLogic_device_has_other_service(pCrData->callee_id, pCrData->callee_stream_type, cr_pos);
        DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "StopRouteService() is_GBLogic_device_has_other_service:other_cr_pos=%d \r\n", other_cr_pos);

        if (other_cr_pos < 0) /* 没有其他业务 */
        {
            /*发送Bye 给被叫侧 */
            i = SIP_SendBye(pCrData->callee_ua_index);
            DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "StopRouteService() SIP_SendBye To Callee:callee_ua_index=%d, i=%d \r\n", pCrData->callee_ua_index, i);
        }
        else
        {
            pOtherCrData = call_record_get(other_cr_pos);

            if (NULL != pOtherCrData)
            {
                pOtherCrData->callee_ua_index = pCrData->callee_ua_index; /* 将前端的会话句柄拷贝到下个业务 */
                DEBUG_TRACE(MODULE_ROUTE, LOG_TRACE, "StopRouteService() callee_ua_index=%d copy from %d to %d \r\n", pOtherCrData->callee_ua_index, cr_pos, other_cr_pos);
            }
        }
    }

    /* 5、移除呼叫记录信息 */
    i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
    i = call_record_remove(cr_pos);

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_ERROR, "StopRouteService() call_record_remove Error:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_ROUTE, LOG_INFO, "StopRouteService() call_record_remove OK:cr_pos=%d, i=%d \r\n", cr_pos, i);
    }

    return cr_pos;
}

#if DECS("通过WebServer接口获取压缩任务处理")
/*****************************************************************************
 函 数 名  : get_compress_task_from_webservice_proc
 功能描述  : 通过WebServer接口获取压缩任务
 输入参数  : char* platform_ip
             int iTaskBeginTime
             int iTaskEndTime
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
int get_compress_task_from_webservice_proc(char* platform_ip, int iTaskBeginTime, int iTaskEndTime, DBOper* ptDBoper)
{
    int iRet = 0;

    if (NULL == platform_ip || platform_ip[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "get_compress_task_from_webservice_proc() exit---: platform_ip Error \r\n");
        return -1;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "从上级平台获取压缩任务信息,平台IP=%s: 开始---", platform_ip);

    /* 3、通过http协议获取VMS下面的real device 信息 */
    iRet = get_compress_task_by_http(platform_ip, iTaskBeginTime, iTaskEndTime, ptDBoper);

    if (0 != iRet)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "从上级平台获取压缩任务信息,失败");
        return iRet;
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "从上级平台获取压缩任务信息: 结束---");
    return 0;
}

/*****************************************************************************
 函 数 名  : get_compress_task_by_http
 功能描述  : 获取压缩任务
 输入参数  : char* platform_ip
             int iTaskBeginTime
             int iTaskEndTime
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
int get_compress_task_by_http(char* platform_ip, int iTaskBeginTime, int iTaskEndTime, DBOper* ptDBoper)
{
    int iRet = 0;
    int task_count = 0;
    string strXMLBuffer = "";
    CPacket inPacket;

    DOMElement* ItemAccNode = NULL;
    char strReturnFiles[1024] = {0};
    jly_yspb_t stYSPB;/* 对应的数据库结构字段 */

    char strTaskBeginTime[64] = {0};
    char strTaskEndTime[64] = {0};

    if (NULL == platform_ip || platform_ip[0] == '\0')
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "get_compress_task_by_http() exit---: platform_ip Error \r\n");
        return -1;
    }

    iRet = format_time(iTaskBeginTime, strTaskBeginTime);
    iRet = format_time(iTaskEndTime, strTaskEndTime);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "通过WebService接口获取压缩任务信息开始:platform_ip=%s, 本次获取任务的开始时间=%s, 结束时间=%s", platform_ip, strTaskBeginTime, strTaskEndTime);

    strXMLBuffer.clear();
    iRet = interface_queryObjectInfo(iTaskBeginTime, iTaskEndTime, strXMLBuffer);
    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "get_compress_task_by_http: interface_queryObjectInfo Response XML msg=\r\n%s \r\n", (char*)strXMLBuffer.c_str());

    if (iRet != 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "通过WebService接口获取压缩任务信息失败,原因:调用WebService查询接口失败,platform_ip=%s", platform_ip);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "get_compress_task_by_http() exit---: interface_queryObjectInfo Error\r\n");
        return iRet;
    }

    //解析XML
    iRet = inPacket.BuiltTree((char*)strXMLBuffer.c_str(), strXMLBuffer.length());//生成DOM树结构.

    if (iRet < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "通过WebService接口获取压缩任务信息失败,原因:解析Http响应报文对应的XML消息体失败,platform_ip=%s, XMLBuffer=%s", platform_ip, (char*)strXMLBuffer.c_str());
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "get_compress_task_by_http() exit---: XML Build Tree Error msg=\r\n%s \r\n", (char*)strXMLBuffer.c_str());
        return FILE_COMPRESS_GET_XML_ERROR;
    }

    /* 获取所有的Item 数据 */
    ItemAccNode = inPacket.SearchElement((char*)"rows");

    if (!ItemAccNode)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "通过WebService接口获取压缩任务信息失败,原因:解析Http响应报文对应的XML消息体中的rows失败,platform_ip=%s, XMLBuffer=%s", platform_ip, (char*)strXMLBuffer.c_str());
        return FILE_COMPRESS_GET_XML_ERROR;
    }

    /* 循环读取数据，插入新的数据 */
    inPacket.SetCurrentElement(ItemAccNode);

    while (ItemAccNode)
    {
        memset(strReturnFiles, 0, 1024);
        inPacket.GetElementValue((char*)"rows", strReturnFiles);

        /* 解析出XML字段 */
        memset(&stYSPB, 0, sizeof(jly_yspb_t));
        analysis_return_fileds(strReturnFiles, &stYSPB);

        task_count++;
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "通过WebService接口获取压缩任务信息, 任务信息:任务数=%d, 记录编号=%s, 文件名称=%s, 后缀名称=%s, 文件大小=%d, 上传单位=%s, 上传时间=%d, 存储路径=%s", task_count, stYSPB.jlbh, stYSPB.wjmc, stYSPB.kzm, stYSPB.wjdx, stYSPB.scdw, stYSPB.scsj, stYSPB.cclj);

        /* 添加压缩任务 */
        iRet = AddCompressTask(platform_ip, &stYSPB, ptDBoper);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "get_compress_task_by_http() exit---: AddCompressTask Error:platform_ip=%s, iRet=%d \r\n", platform_ip, iRet);
            continue;
        }

        ItemAccNode = inPacket.SearchNextElement(true);
    }

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_IMPORTANT, "通过WebService接口获取压缩任务信息结束:platform_ip=%s, 本次获取任务的开始时间=%s, 结束时间=%s, 本次获取的任务总数=%d", platform_ip, strTaskBeginTime, strTaskEndTime, task_count);

    return 0;
}

/*****************************************************************************
 函 数 名  : analysis_return_fileds
 功能描述  : 解析返回字段
 输入参数  : char* strTime
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月6日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int analysis_return_fileds(char* strReturnFiles, jly_yspb_t* pstYSPB)
{
    char* tmp = NULL;
    char strFirstTmp[512] = {0};                     /* 前面的字符串 */
    char strLastTmp[512] = {0};                      /* 剩下的字符串 */
    char strLast[512] = {0};                         /* 剩下的字符串 */

    char jlbh[64 + 4];                               /*1、记录编号(jlbh)，字符串 */
    char wjmc[128 + 4];                              /*2、文件名称(wjmc)，字符串*/
    char kzm[32 + 4];                                /*3、文件后缀名称(kzm)，字符串*/
    int wjdx;                                        /*4、文件大小(wjdx)，整形*/
    char scdw[128 + 4];                              /*5、上传单位(scdw)，字符串*/
    char strscsj[128 + 4];                           /*6、上传时间字符串(scsj)，格式：20170527120000*/
    int scsj;                                        /*6、上传时间(scsj)，格式：20170527120000*/
    char cclj[128 + 4];                              /*7、存储路径(cclj)，字符串*/

    if (NULL == strReturnFiles || NULL == pstYSPB)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "analysis_return_fileds() exit---: Param Error \r\n");
        return -1;
    }

    /* 1、解析记录编号 */
    tmp = strchr(strReturnFiles, '$');

    if (tmp != NULL)
    {
        memset(strFirstTmp, 0, 512);
        memset(strLastTmp, 0, 512);
        osip_strncpy(strFirstTmp, strReturnFiles, tmp - strReturnFiles);
        osip_strncpy(strLastTmp, tmp + 3, strReturnFiles + strlen(strReturnFiles) - tmp - 3);

        memset(jlbh, 0, 64 + 4);
        osip_strncpy(jlbh, strFirstTmp, 64);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "analysis_return_fileds() strFirstTmp=%s, jlbh=%s, strLastTmp=%s \r\n", strFirstTmp, jlbh, strLastTmp);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "analysis_return_fileds() exit---: Analysis jlbh Error \r\n");
        return -1;
    }

    /* 2、解析文件名称 */
    osip_strncpy(strLast, strLastTmp, strlen(strLastTmp));
    tmp = strchr(strLast, '$');

    if (tmp != NULL)
    {
        memset(strFirstTmp, 0, 512);
        memset(strLastTmp, 0, 512);
        osip_strncpy(strFirstTmp, strLast, tmp - strLast);
        osip_strncpy(strLastTmp, tmp + 3, strLast + strlen(strLast) - tmp - 3);

        memset(wjmc, 0, 128 + 4);
        osip_strncpy(wjmc, strFirstTmp, 128);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "analysis_return_fileds() strFirstTmp=%s, wjmc=%s, strLastTmp=%s \r\n", strFirstTmp, wjmc, strLastTmp);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "analysis_return_fileds() exit---: Analysis wjmc Error \r\n");
        return -1;
    }

    /* 3、解析文件后缀名 */
    osip_strncpy(strLast, strLastTmp, strlen(strLastTmp));
    tmp = strchr(strLast, '$');

    if (tmp != NULL)
    {
        memset(strFirstTmp, 0, 512);
        memset(strLastTmp, 0, 512);
        osip_strncpy(strFirstTmp, strLast, tmp - strLast);
        osip_strncpy(strLastTmp, tmp + 3, strLast + strlen(strLast) - tmp - 3);

        memset(kzm, 0, 32 + 4);
        osip_strncpy(kzm, strFirstTmp, 32);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "analysis_return_fileds() strFirstTmp=%s, kzm=%s, strLastTmp=%s \r\n", strFirstTmp, kzm, strLastTmp);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "analysis_return_fileds() exit---: Analysis kzm Error \r\n");
        return -1;
    }

    /* 4、解析文件大小 */
    osip_strncpy(strLast, strLastTmp, strlen(strLastTmp));
    tmp = strchr(strLast, '$');

    if (tmp != NULL)
    {
        memset(strFirstTmp, 0, 512);
        memset(strLastTmp, 0, 512);
        osip_strncpy(strFirstTmp, strLast, tmp - strLast);
        osip_strncpy(strLastTmp, tmp + 3, strLast + strlen(strLast) - tmp - 3);

        wjdx = osip_atoi(strFirstTmp);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "analysis_return_fileds() strFirstTmp=%s, wjdx=%d, strLastTmp=%s \r\n", strFirstTmp, wjdx, strLastTmp);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "analysis_return_fileds() exit---: Analysis kzm Error \r\n");
        return -1;
    }

    /* 5、解析上传单位 */
    osip_strncpy(strLast, strLastTmp, strlen(strLastTmp));
    tmp = strchr(strLast, '$');

    if (tmp != NULL)
    {
        memset(strFirstTmp, 0, 512);
        memset(strLastTmp, 0, 512);
        osip_strncpy(strFirstTmp, strLast, tmp - strLast);
        osip_strncpy(strLastTmp, tmp + 3, strLast + strlen(strLast) - tmp - 3);

        memset(scdw, 0, 128 + 4);
        osip_strncpy(scdw, strFirstTmp, 128);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "analysis_return_fileds() strFirstTmp=%s, scdw=%s, strLastTmp=%s \r\n", strFirstTmp, scdw, strLastTmp);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "analysis_return_fileds() exit---: Analysis scdw Error \r\n");
        return -1;
    }

    /* 6、解析上传时间和存储路径 */
    osip_strncpy(strLast, strLastTmp, strlen(strLastTmp));
    tmp = strchr(strLast, '$');

    if (tmp != NULL)
    {
        memset(strFirstTmp, 0, 512);
        memset(strLastTmp, 0, 512);
        osip_strncpy(strFirstTmp, strLast, tmp - strLast);
        osip_strncpy(strLastTmp, tmp + 3, strLast + strlen(strLast) - tmp - 3);

        memset(strscsj, 0, 128 + 4);
        memset(cclj, 0, 128 + 4);
        osip_strncpy(strscsj, strFirstTmp, 128);
        osip_strncpy(cclj, strLastTmp, 128);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "analysis_return_fileds() strFirstTmp=%s, strscsj=%s, cclj=%s, strLastTmp=%s \r\n", strFirstTmp, strscsj, cclj, strLastTmp);
    }
    else
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "analysis_return_fileds() exit---: Analysis scdw Error \r\n");
        return -1;
    }

    /* 解析时间 */
    scsj = analysis_time_for_compress(strscsj);

    osip_strncpy(pstYSPB->jlbh, jlbh, 64);
    osip_strncpy(pstYSPB->wjmc, wjmc, 128);
    osip_strncpy(pstYSPB->kzm, kzm, 32);
    pstYSPB->wjdx = wjdx;
    osip_strncpy(pstYSPB->scdw, scdw, 128);
    pstYSPB->scsj = scsj;
    osip_strncpy(pstYSPB->cclj, cclj, 128);

    return 0;
}

/*****************************************************************************
 函 数 名  : analysis_time_for_compress
 功能描述  : 时间解析
 输入参数  : char* strTime
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月6日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int analysis_time_for_compress(char* strTime)
{
    char pcYear[5] = {0};
    char pcMonth[3] = {0};
    char pcDay[3] = {0};
    char pcHour[3] = {0};
    char pcMinute[3] = {0};
    char pcSecond[3] = {0};

    struct tm stm;
    time_t timep;

    if (NULL == strTime)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "analysis_time_for_compress() exit---: Param Error \r\n");
        return 0;
    }

    /* 格式：20170527120000 */
    osip_strncpy(pcYear, strTime, 4);
    osip_strncpy(pcMonth, &strTime[4], 2);
    osip_strncpy(pcDay, &strTime[6], 2);
    osip_strncpy(pcHour, &strTime[8], 2);
    osip_strncpy(pcMinute, &strTime[10], 2);
    osip_strncpy(pcSecond, &strTime[12], 2);

    /* 转换成time_t */
    stm.tm_year = osip_atoi(pcYear) - 1900;
    stm.tm_mon = osip_atoi(pcMonth) - 1;
    stm.tm_mday = osip_atoi(pcDay);

    stm.tm_hour = osip_atoi(pcHour);
    stm.tm_min = osip_atoi(pcMinute);
    stm.tm_sec = osip_atoi(pcSecond);

    timep = mktime(&stm);

    return timep;
}

#endif

