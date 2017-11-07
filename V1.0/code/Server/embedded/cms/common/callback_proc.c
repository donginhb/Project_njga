/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "libsip.h"

#include "common/gbldef.inc"
#include "common/gblfunc_proc.inc"
#include "common/gblconfig_proc.inc"
#include "common/callback_proc.inc"
#include "common/log_proc.inc"
#include "common/db_proc.h"

#include "user/user_info_mgn.inc"
#include "user/user_reg_proc.inc"
#include "user/user_srv_proc.inc"
#include "user/user_thread_proc.inc"

#include "device/device_info_mgn.inc"
#include "device/device_reg_proc.inc"
#include "device/device_thread_proc.inc"

#include "record/record_srv_proc.inc"

#include "route/route_info_mgn.inc"
#include "route/route_thread_proc.inc"

#include "service/call_func_proc.inc"
#include "service/alarm_proc.inc"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern gbl_conf_t* pGblconf;              /* 全局配置信息 */
extern DBOper g_DBOper;
extern int cms_run_status;  /* 0:没有运行,1:正常运行 */

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

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#if DECS("上层应用回调处理函数")
/*****************************************************************************
 函 数 名  : uas_register_received_proc
 功能描述  : 服务端收到注册消息的处理函数,
             仅服务端需要处理注册消息
 输入参数  : char* proxy_id,服务器ID
             register_id, 注册的id
             login_ip, 登录IP
             login_port, 登录port
             register_name 注册用户名
             reg_info_index,注册句柄索引
             expires,时间
             link_type, 联网类型
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uas_register_received_proc(char* proxy_id, char* register_id, char* login_ip, int login_port, char* register_name, int reg_info_index, int expires, int link_type)
{
    int i = 0;
    char* pTmp = NULL;
    char strDeviceType[4] = {0};
    int iType = 0;
    char strLocalMgwID[36] = {0}; /* 本地Mgw编码 */
    char strErrorCode[32] = {0};

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "uas_register_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    if (NULL == register_id || NULL == login_ip)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "uas_register_received_proc() exit---: Register ID NULL \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "uas_register_received_proc() \
    \r\n In Param: \
    \r\n proxy_id=%s \
    \r\n register_id=%s \
    \r\n login_ip=%s \
    \r\n login_port=%d \
    \r\n register_name=%s \
    \r\n reg_info_index=%d \
    \r\n expires=%d \
    \r\n link_type=%d \
    \r\n", proxy_id, register_id, login_ip, login_port, register_name, reg_info_index, expires, link_type);

    /* 注册消息主要是客户端和下面标准物理设备的注册 */
    /* 获取注册id中的设备类型，以便放入相应的消息队列 */
    pTmp = &register_id[10];
    osip_strncpy(strDeviceType, pTmp, 3);
    iType = osip_atoi(strDeviceType);

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uas_register_received_proc() DeviceType=%d \r\n", iType);

    if (NULL != proxy_id && '\0' != proxy_id[0] && 0 != sstrcmp(proxy_id, local_cms_id_get()))
    {
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "Device registration failed:Device ID=%s, Device IP address =%s, Port number=%d, Cause=%s, Registration server ID=%s, Local config CMS ID=%s", register_id, login_ip, login_port, (char*)"Registration server ID do not match with local config CMS ID", proxy_id, local_cms_id_get());
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "设备注册失败:设备ID=%s, 设备IP地址=%s, 端口号=%d, 原因=%s, 注册的服务器ID=%s, 本地配置的CMS ID=%s", register_id, login_ip, login_port, (char*)"设备注册服务器ID和本地配置的CMS ID不匹配", proxy_id, local_cms_id_get());

        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_SERVER_ID_NOT_MATCH_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Register Server ID Not Matching With Local CMSID");
        return -1;
    }

    /* 如果是本地的媒体网关，看下IP地址是否是本地IP地址 */
    if (EV9000_DEVICETYPE_MGWSERVER == iType)
    {
        if (pGblconf->center_code[0] != '\0' && pGblconf->trade_code != '\0')
        {
            snprintf(strLocalMgwID, 36, "%s%s%s", pGblconf->center_code, pGblconf->trade_code, (char*)"2091090000");

            if (0 == sstrcmp(register_id, strLocalMgwID))
            {
                if (!IsLocalHost(login_ip))
                {
                    EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "Device registration failed:Device ID=%s,  Device IP address=%s, Port number=%d, Cause=%s", register_id, login_ip, login_port, (char*)"Local media gateway registration IP address do not match with local IP address");
                    SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "设备注册失败:设备ID=%s, 设备IP地址=%s, 端口号=%d, 原因=%s", register_id, login_ip, login_port, (char*)"本地的媒体网关注册的IP地址和本地IP地址不匹配");

                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_SERVER_IP_NOT_MATCH_ERROR);
                    SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                    //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Register Login IP Not Matching With Local IP");
                    return -1;
                }
            }
        }

        /* 检测注册的IP的地址是否是备机的IP地址 */
        if (checkLoginIPIsSlaveIP(login_ip))
        {
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "Device registration failed:Device ID=%s,  Device IP address=%s, Port number=%d, Cause=%s", register_id, login_ip, login_port, (char*)"Media gateway registration IP address is belong to slave cms");
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "设备注册失败:设备ID=%s, 设备IP地址=%s, 端口号=%d, 原因=%s", register_id, login_ip, login_port, (char*)"媒体网关注册的IP地址是备机上面的");

            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_SERVER_IP_NOT_MATCH_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Register Login IP Not Matching With Local IP");
            return -1;
        }
    }

#if 0

    if (sys_show_code_flag_get()) /* 国标模式 */
    {
        if (iType == EV9000_DEVICETYPE_DECODER && !IsIDMatchLocalCMSID(register_id)) /* 解码器注册需要判断ID是否匹配，防止非国标解码器注册 */
        {
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, 设备IP地址=%s, 端口号=%d, 原因=%s, 本地CMS ID=%s", register_id, login_ip, login_port, (char*)"设备注册ID和本地CMS ID不匹配", local_cms_id_get());
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:Device ID=%s,  Device IP address=%s, Port number=%d, Cause=%s, Local CMS ID=%s", register_id, login_ip, login_port, (char*)"Local CMS ID  do not match with registration id.", local_cms_id_get());
            SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Register ID Not Matching With Local CMSID");

            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_ID_NOT_MATCH_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Register Login IP Not Matching With Local IP");
            return -1;
        }
    }

#endif

#if 0

    if (iType < 500 && iType != EV9000_DEVICETYPE_SIPSERVER && !IsIDMatchLocalCMSID(register_id))
    {
        (void)SystemFaultAlarm(0, register_id, (char*)"6", (char*)"0x01070201", "设备注册失败:设备ID=%s, 设备IP地址=%s, 原因=%s, 本地CMS ID=%s", register_id, login_ip, (char*)"设备注册ID和本地CMS ID不匹配", local_cms_id_get());
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, 设备IP地址=%s, 端口号=%d, 原因=%s, 本地CMS ID=%s", register_id, login_ip, login_port, (char*)"设备注册ID和本地CMS ID不匹配", local_cms_id_get());
        SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Register ID Not Matching With Local CMSID");
        return -1;
    }

    /* 检测注册刷新时间是否满足最小间隔时间 */
    if (expires > 0 && expires < MIN_REGISTER_EXPIRE)
    {
        SIP_UASAnswerToRegister4RegExpire(reg_info_index, MIN_REGISTER_EXPIRE);
        return -1;
    }

#endif

    if ((iType >= 300 && iType <= 399) /* 中心用户 */
        || (iType >= 400 && iType <= 499)) /* 终端用户 */
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uas_register_received_proc() user_reg_msg_add:register_id=%s, login_ip=%s, login_port=%d, Type=%d  \r\n", register_id, login_ip, login_port, iType);
        //printf_system_time();

        i = user_reg_msg_add(register_id, login_ip, login_port, register_name, expires, reg_info_index);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uas_register_received_proc() user_reg_msg_add Error:register_id=%s, login_ip=%s, login_port=%d, Type=%d  \r\n", register_id, login_ip, login_port, iType);
        }

        return i;
    }
    else if ((iType >= 111 && iType <= 130) /* 前端主设备 */
             || (iType >= 131 && iType <= 199)) /* 前端外围设备 */
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uas_register_received_proc() GBDevice_reg_msg_add:register_id=%s, login_ip=%s, login_port=%d, Type=%d  \r\n", register_id, login_ip, login_port, iType);
        //printf_system_time();

        i = GBDevice_reg_msg_add(register_id, iType, login_ip, login_port, register_name, expires, reg_info_index, link_type);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uas_register_received_proc() GBDevice_reg_msg_add Error:register_id=%s, login_ip=%s, login_port=%d, Type=%d  \r\n", register_id, login_ip, login_port, iType);
        }

        return i;
    }
    else if (iType >= 200 && iType <= 299) /* 平台设备:下级CMS  */
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uas_register_received_proc() GBDevice_reg_msg_add:register_id=%s, login_ip=%s, login_port=%d, Type=%d \r\n", register_id, login_ip, login_port, iType);

        i = GBDevice_reg_msg_add(register_id, iType, login_ip, login_port, register_name, expires, reg_info_index, link_type);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uas_register_received_proc() GBDevice_reg_msg_add Error:register_id=%s, login_ip=%s, login_port=%d, Type=%d  \r\n", register_id, login_ip, login_port, iType);
        }

        return i;
    }
    else if (iType >= 500 && iType <= 999) /* 扩展设备类型*/
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uas_register_received_proc() GBDevice_reg_msg_add:register_id=%s, login_ip=%s, login_port=%d, Type=%d  \r\n", register_id, login_ip, login_port, iType);
        //printf_system_time();

        i = GBDevice_reg_msg_add(register_id, iType, login_ip, login_port, register_name, expires, reg_info_index, link_type);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uas_register_received_proc() GBDevice_reg_msg_add Error:register_id=%s, login_ip=%s, login_port=%d, Type=%d  \r\n", register_id, login_ip, login_port, iType);
        }

        return i;
    }

    EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:Device ID:DeviceID=%s, Device IP address=%s, Cause=%s, Registration device type=%d", register_id, login_ip, (char*)"Registration device type is not supported", iType);
    SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, 设备IP地址=%s, 原因=%s, 注册的设备类型=%d", register_id, login_ip, (char*)"注册设备类型不支持", iType);
    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_DEVICE_TYPE_NOT_SUPPORT_ERROR);
    SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
    //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Register ID Type Not Support");
    return -1;
}

/*****************************************************************************
 函 数 名  : uas_register_received_timeout_proc
 功能描述  : 服务端没有收到客户端刷新注册消息超时处理
                            仅服务端需要处理
 输入参数  : char* proxy_id,服务器ID
             char* register_id
             char* login_ip
             int login_port
             int reg_info_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月17日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uas_register_received_timeout_proc(char* proxy_id, char* register_id, char* login_ip, int login_port, int reg_info_index)
{
    int i = 0;
    char* pTmp = NULL;
    char strDeviceType[4] = {0};
    int iType = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "uas_register_received_timeout_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    if (NULL == register_id)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "uas_register_received_timeout_proc() exit---: Register ID NULL \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "uas_register_received_timeout_proc() \
    \r\n In Param: \
    \r\n proxy_id=%s \
    \r\n register_id=%s \
    \r\n login_ip=%s \
    \r\n login_port=%d \
    \r\n reg_info_index=%d \
    \r\n", proxy_id, register_id, login_ip, login_port, reg_info_index);

    /* 获取注册id中的设备类型，以便放入相应的消息队列 */
    pTmp = &register_id[10];
    osip_strncpy(strDeviceType, pTmp, 3);
    iType = osip_atoi(strDeviceType);

    if ((iType >= 300 && iType <= 399) /* 中心用户 */
        || (iType >= 400 && iType <= 499)) /* 终端用户 */
    {
        i = user_reg_msg_add(register_id, login_ip, login_port, NULL, -1, reg_info_index);
    }
    else if ((iType >= 111 && iType <= 130) /* 前端主设备 */
             || (iType >= 131 && iType <= 199)) /* 前端外围设备 */
    {
        i = GBDevice_reg_msg_add(register_id, iType, login_ip, login_port, NULL, 0, reg_info_index, 0);
    }
    else if (iType >= 200 && iType <= 299) /* 平台设备:下级CMS  */
    {
        i = GBDevice_reg_msg_add(register_id, iType, login_ip, login_port, NULL, 0, reg_info_index, 0);
    }
    else if (iType >= 500 && iType <= 999) /* 扩展设备类型*/
    {
        i = GBDevice_reg_msg_add(register_id, iType, login_ip, login_port, NULL, 0, reg_info_index, 0);
    }
    else
    {
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration time out message failed:DeviceID=%s, Device IP address=%s, Cause=%s, Registration device type=%d", register_id, login_ip, (char*)"Registration device type is not supported", iType);
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册超时消息处理失败:设备ID=%s, 设备IP地址=%s, 原因=%s, 注册的设备类型=%d", register_id, login_ip, (char*)"注册设备类型不支持", iType);
        return -1;
    }

    return i;
}

/*****************************************************************************
 函 数 名  : register_response_received_proc
 功能描述  : 客户端发送注册消息收到注册回应消息的处理函数
 输入参数  : int reg_info_index
             int iExpires
             int status_code
             char* reasonphrase
             unsigned int iTime
             int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int uac_register_response_received_proc(int reg_info_index, int iExpires, int status_code, char* reasonphrase, unsigned int iTime, int user_data)
{
    int iRet = 0;
    int pos = -1;
    route_info_t* pRouteInfo = NULL;
    int route_tl_pos = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "uac_register_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "uac_register_response_received_proc() \
    \r\n In Param: \
    \r\n reg_info_index=%d \
    \r\n status_code=%d \
    \r\n", reg_info_index, status_code);

    /* 根据reg_info_index 查找route注册信息 */
    if (reg_info_index < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "uac_register_response_received_proc() exit---: Register Info Index Error \r\n");
        return -1;
    }

    pos = route_info_find_by_reg_index(reg_info_index);

    if (pos < 0)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() exit---: Find Route Info Error:reg_info_index=%d \r\n", reg_info_index);
        return -1;
    }

    pRouteInfo = route_info_get(pos);

    if (NULL == pRouteInfo)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() exit---: Get Route Info Error:reg_info_index=%d \r\n", reg_info_index);
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() RouteInfo:server_id=%s, server_ip=%s, server_port=%d, link_type=%d, reg_status=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, pRouteInfo->link_type, pRouteInfo->reg_status);

    if (200 == status_code)
    {
        if (iExpires > 0)
        {
            if (pRouteInfo->reg_status <= 0)
            {
                /* 创建上级平台业务处理线程 */
                route_tl_pos = route_srv_proc_thread_find(pRouteInfo);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "uac_register_response_received_proc() route_srv_proc_thread_find:route_tl_pos=%d \r\n", route_tl_pos);

                if (route_tl_pos < 0)
                {
                    //分配处理线程
                    iRet = route_srv_proc_thread_assign(pRouteInfo);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "uac_register_response_received_proc() route_srv_proc_thread_assign:iRet=%d \r\n", iRet);

                    if (iRet != 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "分配上级平台业务处理线程失败:上级平台ID=%s, IP地址=%s, 端口号=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    }
                }
                else
                {
                    /* 释放一下之前的业务处理线程 */
                    iRet = route_srv_proc_thread_recycle(pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

                    //分配处理线程
                    iRet = route_srv_proc_thread_assign(pRouteInfo);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "uac_register_response_received_proc() route_srv_proc_thread_assign:iRet=%d \r\n", iRet);

                    if (iRet != 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "分配上级平台业务处理线程失败:上级平台ID=%s, IP地址=%s, 端口号=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
                    }
                }
            }

            pRouteInfo->reg_status = 1;
            pRouteInfo->reg_info_index = reg_info_index;
            pRouteInfo->expires = iExpires;
            pRouteInfo->min_expires = iExpires;

            if (pGblconf->ntp_server_ip[0] == '\0') /* 有NTP的时候就不跟上级校时了 */
            {
                /* 如果上级的IP在同一台机器上面，比如手机的MMS，就不校时了 */
                if (!IsLocalHost(pRouteInfo->server_ip))
                {
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior platform national standard registration return time system school: Superior CMS ID=%s, IP address=%s, port number=%d, time=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iTime);
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级平台国标注册返回时间系统校时:上级CMS ID=%s, IP地址=%s, 端口号=%d, 时间=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iTime);
                    /* 设置系统时间 */
                    set_system_time(iTime);
                    system("hwclock -wu");

                    iRet = SendKeepAliveMessageToRouteCMS(pRouteInfo);

                    if (0 != iRet)
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() SendKeepAliveMessageToRouteCMS Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() SendKeepAliveMessageToRouteCMS OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                    }
                }
            }
        }
        else if (iExpires == 0)
        {
            pRouteInfo->reg_status = 0;
            pRouteInfo->reg_info_index = -1;
            pRouteInfo->expires = 0;
            pRouteInfo->min_expires = 0;

            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Superior platform register remove return: Superior CMS ID=%s, IP address=%s, port number=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "上级平台注销注册返回:上级CMS ID=%s, IP地址=%s, 端口号=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

            /* 移除上级锁定的点位信息 */
            iRet = RemoveGBLogicDeviceLockInfoByRouteInfo(pRouteInfo);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }

            iRet = StopAllServiceTaskByCallerIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() StopAllServiceTaskByCallerIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() StopAllServiceTaskByCallerIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }

            /* 还有向上级CMS的请求 */
            iRet = StopAllServiceTaskByCalleeIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() StopAllServiceTaskByCalleeIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() StopAllServiceTaskByCalleeIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }

            /* 回收业务处理线程 */
            iRet = route_srv_proc_thread_recycle(pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

            if (0 != iRet)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() route_srv_proc_thread_recycle Error:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uac_register_response_received_proc() route_srv_proc_thread_recycle OK:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
            }
        }
    }
    else if (status_code >= 300 && status_code < 400)
    {
        pRouteInfo->reg_status = 0;
        pRouteInfo->reg_info_index = -1;
        pRouteInfo->expires = 0;
        pRouteInfo->min_expires = 0;

        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Superior platform register failed: Superior CMS ID=%s, IP address=%s, port number=%d, status code=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, status_code);
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级平台注册返回失败:上级CMS ID=%s, IP地址=%s, 端口号=%d，错误码=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, status_code);

        /* 移除上级锁定的点位信息 */
        iRet = RemoveGBLogicDeviceLockInfoByRouteInfo(pRouteInfo);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }

        iRet = StopAllServiceTaskByCallerIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() StopAllServiceTaskByCallerIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() StopAllServiceTaskByCallerIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }

        /* 还有向上级CMS的请求 */
        iRet = StopAllServiceTaskByCalleeIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() StopAllServiceTaskByCalleeIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() StopAllServiceTaskByCalleeIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }

        /* 回收业务处理线程 */
        iRet = route_srv_proc_thread_recycle(pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() route_srv_proc_thread_recycle Error:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uac_register_response_received_proc() route_srv_proc_thread_recycle OK:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
    }
    else if (status_code >= 400 && status_code != 401 && status_code != 407 && status_code != 423)
    {
        pRouteInfo->reg_status = 0;
        pRouteInfo->reg_info_index = -1;
        pRouteInfo->expires = 0;
        pRouteInfo->min_expires = 0;

        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Superior platform register failed: Superior CMS ID=%s, IP address=%s, port number=%d, status code=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, status_code);
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级平台注册返回失败:上级CMS ID=%s, IP地址=%s, 端口号=%d，错误码=%d", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, status_code);

        /* 移除上级锁定的点位信息 */
        iRet = RemoveGBLogicDeviceLockInfoByRouteInfo(pRouteInfo);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }

        iRet = StopAllServiceTaskByCallerIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() StopAllServiceTaskByCallerIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() StopAllServiceTaskByCallerIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }

        /* 还有向上级CMS的请求 */
        iRet = StopAllServiceTaskByCalleeIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() StopAllServiceTaskByCalleeIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "uac_register_response_received_proc() StopAllServiceTaskByCalleeIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }

        /* 回收业务处理线程 */
        iRet = route_srv_proc_thread_recycle(pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

        if (0 != iRet)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "uac_register_response_received_proc() route_srv_proc_thread_recycle Error:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "uac_register_response_received_proc() route_srv_proc_thread_recycle OK:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : invite_received_proc
 功能描述  : 收到INVITE消息的处理函数
 输入参数  : char* caller_id
             char* callee_id
             char* call_id
             int dialog_index
             char* msg_body
             int msg_len
             int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int invite_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, char* msg_body, int msg_len, int user_data)
{
    int i = 0;
    int pos = -1;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;
    char* caller_ip = NULL;
    int caller_port = 0;
    char strErrorCode[32] = {0};

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "invite_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "invite_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n", call_id, caller_id, callee_id, dialog_index);

    /* 目前的INVITE消息来自于客户端以及上级CMS */

    /* 获取消息来源的ip和端口号 */
    caller_ip = SIP_GetDialogFromHost(dialog_index);

    if (NULL == caller_ip)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_IP_ERROR);
        SIP_AnswerToInvite(dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(dialog_index, 503, (char*)"Get Caller IP Error");
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_received_proc() exit---: Get Caller IP Error \r\n");
        return -1;
    }

    caller_port = SIP_GetDialogFromPort(dialog_index);

    if (caller_port <= 0)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_INVITE_CALLER_PORT_ERROR);
        SIP_AnswerToInvite(dialog_index, 503, strErrorCode);
        //SIP_AnswerToInvite(dialog_index, 503, (char*)"Get Caller Port Error");
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_received_proc() exit---: Get Caller Port Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

    /* 查找用户处理线程队列 */
    user_srv_thread = get_user_srv_proc_thread(caller_id, caller_ip, caller_port);

    if (NULL != user_srv_thread)
    {
        if (NULL != user_srv_thread->pUserSrvMsgQueue)
        {
            /* 添加到用户业务消息队列 */
            i = user_srv_msg_add(user_srv_thread, MSG_INVITE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_received_proc() user_srv_msg_add:i=%d \r\n", i);
            return 0;
        }
    }
    else
    {
#if 1
        /* 查找设备处理线程队列 */
        device_srv_thread = get_device_srv_proc_thread(caller_id, caller_ip, caller_port);

        if (NULL != device_srv_thread)
        {
            /* 添加到设备线程业务消息队列 */
            i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_INVITE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
            return 0;
        }
        else
#endif
        {
            /* 可能来自前端设备*/
            pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(caller_id, caller_ip, caller_port);

            if (NULL != pGBDeviceInfo)
            {
                /* 添加到设备业务消息队列 */
                i = device_srv_msg_add(pGBDeviceInfo, MSG_INVITE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                return 0;
            }
            else
            {
                pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* 添加到设备业务消息队列 */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_INVITE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    /* 如果不是来自客户端和前端设备,检查是否来自上级CMS */
#if 1
                    /* 查找上级平台处理线程队列 */
                    route_srv_thread = get_route_srv_proc_thread(caller_id, caller_ip, caller_port);

                    if (NULL != route_srv_thread)
                    {
                        /* 添加到上级平台线程业务消息队列 */
                        i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_INVITE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                        return 0;
                    }
                    else
#endif
                    {
                        pos = route_info_find_by_host_and_port(caller_ip, caller_port);

                        if (pos >= 0)
                        {
                            pRouteInfo = route_info_get(pos);

                            if (NULL != pRouteInfo)
                            {
                                /* 添加到互联路由业务消息队列 */
                                i = route_srv_msg_add(pRouteInfo, MSG_INVITE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_received_proc() route_srv_msg_add:i=%d \r\n", i);
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Invite Message Proc failed:caller_id=%s, callee_id=%s, dialog_index=%d, Cause=%s", caller_id, callee_id, dialog_index, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Invite消息处理失败:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, dialog_index=%d, 原因=%s", caller_id, caller_ip, caller_port, callee_id, dialog_index, (char*)"找不到对应的处理线程");

    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_NO_DEAL_THREAD_ERROR);
    SIP_AnswerToInvite(dialog_index, 503, strErrorCode);
    //SIP_AnswerToInvite(dialog_index, 503, NULL);
    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "invite_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : invite_response_received_proc
 功能描述  : 收到INVITE回应消息的处理函数
 输入参数  : char* caller_id
             char* callee_id
             char* call_id
             int dialog_index
             int status_code
             char* reasonphrase
             char* msg_body
             int msg_len
             int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int invite_response_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int status_code, char* reasonphrase, char* msg_body, int msg_len, int user_data)
{
    int i = 0;
    int pos = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;
    char* caller_ip = NULL;
    int caller_port = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "invite_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "invite_response_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n status_code=%d \
    \r\n reasonphrase=%s \
    \r\n", call_id, caller_id, callee_id, dialog_index, status_code, reasonphrase);

    /* INVITE响应消息来自被叫侧 */

    /* 1、根据callee index 查找呼叫记录信息 */
    cr_pos = call_record_find_by_callee_index(dialog_index);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() call_record_find_by_callee_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            if (200 == status_code)
            {
                i = SIP_SendBye(dialog_index);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
            }

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS主动请求的DC流程 */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* 添加到DC 业务消息队列 */
            i = device_srv_msg_add(NULL, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, status_code=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, status_code, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* 添加到录像业务消息队列 */
            i = record_srv_msg_add(MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "录像业务, 收到INVITE回应消息, 添加到录像消息处理队列:逻辑设备ID=%s, IP地址=%s, callee_ua_index=%d, dialog_index=%d, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, pCrData->callee_ua_index, dialog_index, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, status_code=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, status_code, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* 获取消息来源的ip和端口号 */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                if (200 == status_code)
                {
                    i = SIP_SendBye(dialog_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
                }

                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: Get Caller IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                if (200 == status_code)
                {
                    i = SIP_SendBye(dialog_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
                }

                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: Get Caller Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 1.1、查找用户处理线程队列 */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* 添加到用户业务消息队列 */
                    i = user_srv_msg_add(user_srv_thread, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 1.2 检测是否来自前端设备*/
#if 1
            /* 查找设备处理线程队列 */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* 添加到设备线程业务消息队列 */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* 添加到设备业务消息队列 */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* 添加到设备业务消息队列 */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 1.3、检查是否来自上级CMS */
#if 1
            /* 查找上级平台处理线程队列 */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* 添加到上级平台线程业务消息队列 */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* 添加到互联路由业务消息队列 */
                        i = route_srv_msg_add(pRouteInfo, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        if (200 == status_code)
        {
            i = SIP_SendBye(dialog_index);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    /* 2、根据caller index 查找呼叫记录信息 */
    cr_pos = call_record_find_by_caller_index(dialog_index);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() call_record_find_by_caller_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            if (200 == status_code)
            {
                i = SIP_SendBye(dialog_index);
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
            }

            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS主动请求的DC流程 */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* 添加到DC 业务消息队列 */
            i = device_srv_msg_add(NULL, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, status_code=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, status_code, i);
            return 0;
        }

        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* 添加到录像业务消息队列 */
            i = record_srv_msg_add(MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "录像业务, 收到INVITE回应消息, 添加到录像消息处理队列:逻辑设备ID=%s, IP地址=%s, caller_ua_index=%d, dialog_index=%d, cr_pos=%d", pCrData->callee_id, pCrData->callee_ip, pCrData->caller_ua_index, dialog_index, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, status_code=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, status_code, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* 获取消息来源的ip和端口号 */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                if (200 == status_code)
                {
                    i = SIP_SendBye(dialog_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
                }

                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: Get User IP Error \r\n");
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                if (200 == status_code)
                {
                    i = SIP_SendBye(dialog_index);
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
                }

                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 2.1、查找用户处理线程队列 */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* 添加到用户业务消息队列 */
                    i = user_srv_msg_add(user_srv_thread, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 2.2 检测是否来自前端设备*/
#if 1
            /* 查找设备处理线程队列 */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* 添加到设备线程业务消息队列 */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* 添加到设备业务消息队列 */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* 添加到设备业务消息队列 */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 2.3、检查是否来自上级CMS */
#if 1
            /* 查找上级平台处理线程队列 */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* 添加到上级平台线程业务消息队列 */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* 添加到互联路由业务消息队列 */
                        i = route_srv_msg_add(pRouteInfo, MSG_INVITE_RESPONSE, caller_id, callee_id, status_code, reasonphrase, dialog_index, msg_body, msg_len, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "invite_response_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        if (200 == status_code)
        {
            i = SIP_SendBye(dialog_index);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "invite_response_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    if (200 == status_code)
    {
        i = SIP_SendBye(dialog_index);
        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "invite_response_received_proc() SIP_SendBye:dialog_index=%d, i=%d \r\n", dialog_index, i);
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Invite Response Message Proc failed:caller_id=%s, callee_id=%s, dialog_index=%d, Cause=%s", caller_id, callee_id, dialog_index, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Invite响应消息处理失败:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, dialog_index=%d, 原因=%s", caller_id, caller_ip, caller_port, callee_id, dialog_index, (char*)"找不到对应的处理线程");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "invite_response_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : cancel_received_proc
 功能描述  : 收到Cancel消息的处理函数
 输入参数  : char* caller_id
             char* callee_id
             char* call_id
             int dialog_index
             int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月22日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int cancel_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int user_data)
{
    int i = 0;
    int pos = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;
    char* caller_ip = NULL;
    int caller_port = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "cancel_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "cancel_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n", call_id, caller_id, callee_id, dialog_index);

    /* Cancel 消息来自主叫侧 */

    /* 1、根据callee index 查找呼叫记录信息 */
    cr_pos = call_record_find_by_callee_index(dialog_index);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "cancel_received_proc() call_record_find_by_callee_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS主动请求的DC流程 */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* 添加到DC 业务消息队列 */
            i = device_srv_msg_add(NULL, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "cancel_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* 添加到录像业务消息队列 */
            i = record_srv_msg_add(MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "cancel_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* 获取消息来源的ip和端口号 */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 1.1、查找用户处理线程队列 */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* 添加到用户业务消息队列 */
                    i = user_srv_msg_add(user_srv_thread, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 1.2 检测是否来自前端设备*/
#if 1
            /* 查找设备处理线程队列 */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* 添加到设备线程业务消息队列 */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* 添加到设备业务消息队列 */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* 添加到设备业务消息队列 */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 1.3、检查是否来自上级CMS */
#if 1
            /* 查找上机平台处理线程队列 */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* 添加到上级平台线程业务消息队列 */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* 添加到互联路由业务消息队列 */
                        i = route_srv_msg_add(pRouteInfo, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    /* 2、根据caller index 查找呼叫记录信息 */
    cr_pos = call_record_find_by_caller_index(dialog_index);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "cancel_received_proc() call_record_find_by_caller_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS主动请求的DC流程 */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* 添加到DC 业务消息队列 */
            i = device_srv_msg_add(NULL, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "cancel_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* 添加到录像业务消息队列 */
            i = record_srv_msg_add(MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "cancel_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* 获取消息来源的ip和端口号 */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 2.1、查找用户处理线程队列 */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* 添加到用户业务消息队列 */
                    i = user_srv_msg_add(user_srv_thread, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 2.2 检测是否来自前端设备*/
#if 1
            /* 查找设备处理线程队列 */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* 添加到设备线程业务消息队列 */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* 添加到设备业务消息队列 */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* 添加到设备业务消息队列 */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 2.3、检查是否来自上级CMS */
#if 1
            /* 查找上机平台处理线程队列 */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* 添加到上级平台线程业务消息队列 */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* 添加到互联路由业务消息队列 */
                        i = route_srv_msg_add(pRouteInfo, MSG_CANCEL, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "cancel_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "cancel_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Cancel Message Proc failed:caller_id=%s, callee_id=%s, dialog_index=%d, Cause=%s", caller_id, callee_id, dialog_index, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "取消消息处理失败:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, dialog_index=%d, 原因=%s", caller_id, caller_ip, caller_port, callee_id, dialog_index, (char*)"找不到对应的处理线程");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "cancel_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : ack_received_proc
 功能描述  : 收到Ack消息的处理函数
 输入参数  : char* caller_id
             char* caller_host,
             int dialog_index
             int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月19日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int ack_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int user_data)
{
    return 0; /* 协议栈适配已经处理，不需要上层再处理 */

    int i = 0;
    int pos = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;
    char* caller_ip = NULL;
    int caller_port = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "ack_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "ack_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n", call_id, caller_id, callee_id, dialog_index);

    /* ACK消息来自主叫侧 */

    /* 1、根据callee index 查找呼叫记录信息 */
    cr_pos = call_record_find_by_callee_index(dialog_index);
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "ack_received_proc() call_record_find_by_callee_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS主动请求的DC流程 */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* 添加到DC 业务消息队列 */
            i = device_srv_msg_add(NULL, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "ack_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* 添加到录像业务消息队列 */
            i = record_srv_msg_add(MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "ack_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* 获取消息来源的ip和端口号 */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 1.1、查找用户处理线程队列 */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* 添加到用户业务消息队列 */
                    i = user_srv_msg_add(user_srv_thread, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 1.2 检测是否来自前端设备*/
#if 1
            /* 查找设备处理线程队列 */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* 添加到设备线程业务消息队列 */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* 添加到设备业务消息队列 */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* 添加到设备业务消息队列 */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 1.3、检查是否来自上级CMS */
#if 1
            /* 查找上机平台处理线程队列 */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* 添加到上级平台线程业务消息队列 */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* 添加到互联路由业务消息队列 */
                        i = route_srv_msg_add(pRouteInfo, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    /* 2、根据caller index 查找呼叫记录信息 */
    cr_pos = call_record_find_by_caller_index(dialog_index); /* 主叫发起的Bye 消息 */
    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "ack_received_proc() call_record_find_by_caller_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS主动请求的DC流程 */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* 添加到DC 业务消息队列 */
            i = device_srv_msg_add(NULL, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "ack_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* 添加到录像业务消息队列 */
            i = record_srv_msg_add(MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "ack_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* 获取消息来源的ip和端口号 */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 2.1、查找用户处理线程队列 */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* 添加到用户业务消息队列 */
                    i = user_srv_msg_add(user_srv_thread, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 2.2 检测是否来自前端设备*/
#if 1
            /* 查找设备处理线程队列 */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* 添加到设备线程业务消息队列 */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* 添加到设备业务消息队列 */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* 添加到设备业务消息队列 */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 2.3、检查是否来自上级CMS */
#if 1
            /* 查找上机平台处理线程队列 */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* 添加到上级平台线程业务消息队列 */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* 添加到互联路由业务消息队列 */
                        i = route_srv_msg_add(pRouteInfo, MSG_ACK, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ack_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ack_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Ack Message Proc failed:caller_id=%s, callee_id=%s, dialog_index=%d, Cause=%s", caller_id, callee_id, dialog_index, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Ack消息处理失败:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, dialog_index=%d, 原因=%s", caller_id, caller_ip, caller_port, callee_id, dialog_index, (char*)"找不到对应的处理线程");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "ack_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : bye_received_proc
 功能描述  : 收到Bye消息的处理函数
 输入参数  : char* caller_id
             char* caller_host,
             int dialog_index
             int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int bye_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int user_data)
{
    int i = 0;
    int pos = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;
    char* caller_ip = NULL;
    int caller_port = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "bye_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "bye_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n", call_id, caller_id, callee_id, dialog_index);

    /* Bye消息可能是主叫发送的也可能是被叫发送的 */

    /* 1、根据callee index 查找呼叫记录信息 */
    cr_pos = call_record_find_by_callee_index(dialog_index); /* 被叫发起的Bye 消息 */
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_received_proc() call_record_find_by_callee_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            SIP_AnswerToBye(dialog_index, 503, (char*)"Get Call Record Error");
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS主动请求的DC流程 */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* 添加到DC 业务消息队列 */
            i = device_srv_msg_add(NULL, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* 添加到录像业务消息队列 */
            i = record_srv_msg_add(MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* 获取消息来源的ip和端口号 */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                SIP_AnswerToBye(dialog_index, 503, (char*)"Get User IP Error");
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                SIP_AnswerToBye(dialog_index, 503, (char*)"Get User Port Error");
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 1.1、查找用户处理线程队列 */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* 添加到用户业务消息队列 */
                    i = user_srv_msg_add(user_srv_thread, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 1.2 检测是否来自前端设备*/
#if 1
            /* 查找设备处理线程队列 */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* 添加到设备线程业务消息队列 */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* 添加到设备业务消息队列 */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* 添加到设备业务消息队列 */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 1.3、检查是否来自上级CMS */
#if 1
            /* 查找上机平台处理线程队列 */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* 添加到上级平台线程业务消息队列 */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* 添加到互联路由业务消息队列 */
                        i = route_srv_msg_add(pRouteInfo, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        SIP_AnswerToBye(dialog_index, 481, NULL);
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    /* 2、根据caller index 查找呼叫记录信息 */
    cr_pos = call_record_find_by_caller_index(dialog_index); /* 主叫发起的Bye 消息 */
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_received_proc() call_record_find_by_caller_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            SIP_AnswerToBye(dialog_index, 503, (char*)"Get Call Record Error");
            i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
            i = call_record_remove(cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS主动请求的DC流程 */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* 添加到DC 业务消息队列 */
            i = device_srv_msg_add(NULL, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* 添加到录像业务消息队列 */
            i = record_srv_msg_add(MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* 获取消息来源的ip和端口号 */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                SIP_AnswerToBye(dialog_index, 503, (char*)"Get User IP Error");
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                SIP_AnswerToBye(dialog_index, 503, (char*)"Get User Port Error");
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 2.1、查找用户处理线程队列 */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* 添加到用户业务消息队列 */
                    i = user_srv_msg_add(user_srv_thread, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 2.2 检测是否来自前端设备*/
#if 1
            /* 查找设备处理线程队列 */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* 添加到设备线程业务消息队列 */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* 添加到设备业务消息队列 */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* 添加到设备业务消息队列 */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 2.3、检查是否来自上级CMS */
#if 1
            /* 查找上机平台处理线程队列 */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* 添加到上级平台线程业务消息队列 */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* 添加到互联路由业务消息队列 */
                        i = route_srv_msg_add(pRouteInfo, MSG_BYE, caller_id, callee_id, 0, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        SIP_AnswerToBye(dialog_index, 481, NULL);
        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Bye Message Proc failed:caller_id=%s, callee_id=%s, dialog_index=%d, Cause=%s", caller_id, callee_id, dialog_index, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Bye消息处理失败:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, dialog_index=%d, 原因=%s", caller_id, caller_ip, caller_port, callee_id, dialog_index, (char*)"找不到对应的处理线程");

    SIP_AnswerToBye(dialog_index, 481, NULL);
    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "bye_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : bye_response_received_proc
 功能描述  : 收到Bye回应消息的处理函数
 输入参数  : char* caller_id
             char* caller_host
             int dialog_index
             int status_code
             int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int bye_response_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int status_code, int user_data)
{
    int i = 0;
    int pos = -1;
    int cr_pos = -1;
    cr_t* pCrData = NULL;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;
    char* caller_ip = NULL;
    int caller_port = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "bye_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "bye_response_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n status_code=%d \
    \r\n", call_id, caller_id, callee_id, dialog_index, status_code);

    /* Bye响应消息可能是主叫发送的也可能是被叫发送的 */

    /* 1、根据callee index 查找呼叫记录信息 */
    cr_pos = call_record_find_by_callee_index(dialog_index); /* 被叫发起的Bye 消息 */
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_response_received_proc() call_record_find_by_callee_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS主动请求的DC流程 */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* 添加到DC 业务消息队列 */
            i = device_srv_msg_add(NULL, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_response_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, status_code=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, status_code, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* 添加到录像业务消息队列 */
            i = record_srv_msg_add(MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_response_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, status_code=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, status_code, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* 获取消息来源的ip和端口号 */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 1.1、查找用户处理线程队列 */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* 添加到用户业务消息队列 */
                    i = user_srv_msg_add(user_srv_thread, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 1.2 检测是否来自前端设备*/
#if 1
            /* 查找设备处理线程队列 */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* 添加到设备线程业务消息队列 */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* 添加到设备业务消息队列 */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* 添加到设备业务消息队列 */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 1.3、检查是否来自上级CMS */
#if 1
            /* 查找上机平台处理线程队列 */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* 添加到上级平台线程业务消息队列 */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* 添加到互联路由业务消息队列 */
                        i = route_srv_msg_add(pRouteInfo, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    /* 2、根据caller index 查找呼叫记录信息 */
    cr_pos = call_record_find_by_caller_index(dialog_index); /* 主叫发起的Bye 消息 */
    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_response_received_proc() call_record_find_by_caller_index:cr_pos=%d \r\n", cr_pos);

    if (cr_pos >= 0)
    {
        pCrData = call_record_get(cr_pos);

        if (NULL == pCrData)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: Get Call Record Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
            return -1;
        }

        //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() pCrData:caller_ua_index=%d,callee_ua_index=%d,call_type=%d \r\n", pCrData->caller_ua_index, pCrData->callee_ua_index, pCrData->call_type);
        /* CMS主动请求的DC流程 */
        if (CALL_TYPE_DC == pCrData->call_type)
        {
            /* 添加到DC 业务消息队列 */
            i = device_srv_msg_add(NULL, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_response_received_proc() device_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, status_code=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, status_code, cr_pos, i);
            return 0;
        }
        else if (CALL_TYPE_RECORD == pCrData->call_type)
        {
            /* 添加到录像业务消息队列 */
            i = record_srv_msg_add(MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "bye_response_received_proc() record_srv_msg_add:caller_id=%s, callee_id=%s, dialog_index=%d, status_code=%d, cr_pos=%d, i=%d \r\n", caller_id, callee_id, dialog_index, status_code, cr_pos, i);
            return 0;
        }
        else if ((CALL_TYPE_REALTIME == pCrData->call_type)
                 || (CALL_TYPE_RECORD_PLAY == pCrData->call_type)
                 || (CALL_TYPE_DOWNLOAD == pCrData->call_type)
                 || (CALL_TYPE_AUDIO == pCrData->call_type))
        {
            /* 获取消息来源的ip和端口号 */
            caller_ip = SIP_GetDialogFromHost(pCrData->caller_ua_index);

            if (NULL == caller_ip)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: Get User IP Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            caller_port = SIP_GetDialogFromPort(pCrData->caller_ua_index);

            if (caller_port <= 0)
            {
                i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
                i = call_record_remove(cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: Get User Port Error:call_record_remove, cr_pos=%d \r\n", cr_pos);
                return -1;
            }

            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() SIP_GetDialogFromHost:caller_ip=%s,caller_port=%d \r\n", caller_ip, caller_port);

            /* 2.1、查找用户处理线程队列 */
            user_srv_thread = get_user_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != user_srv_thread)
            {
                if (NULL != user_srv_thread->pUserSrvMsgQueue)
                {
                    /* 添加到用户业务消息队列 */
                    i = user_srv_msg_add(user_srv_thread, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() user_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }

            /* 2.2 检测是否来自前端设备*/
#if 1
            /* 查找设备处理线程队列 */
            device_srv_thread = get_device_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != device_srv_thread)
            {
                /* 添加到设备线程业务消息队列 */
                i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(pCrData->caller_id, caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* 添加到设备业务消息队列 */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                    if (NULL != pGBDeviceInfo)
                    {
                        /* 添加到设备业务消息队列 */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
            }

            /* 2.3、检查是否来自上级CMS */
#if 1
            /* 查找上机平台处理线程队列 */
            route_srv_thread = get_route_srv_proc_thread(pCrData->caller_id, caller_ip, caller_port);

            if (NULL != route_srv_thread)
            {
                /* 添加到上级平台线程业务消息队列 */
                i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                return 0;
            }
            else
#endif
            {
                pos = route_info_find_by_host_and_port(caller_ip, caller_port);
                //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() route_info_find_by_host_and_port:pos=%d \r\n", pos);

                if (pos >= 0)
                {
                    pRouteInfo = route_info_get(pos);

                    if (NULL != pRouteInfo)
                    {
                        /* 添加到互联路由业务消息队列 */
                        i = route_srv_msg_add(pRouteInfo, MSG_BYE_RESPONSE, caller_id, callee_id, status_code, NULL, dialog_index, NULL, 0, cr_pos);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "bye_response_received_proc() route_srv_msg_add:i=%d \r\n", i);
                        return 0;
                    }
                }
            }
        }

        i = call_record_set_call_status(cr_pos, CALL_STATUS_WAIT_RELEASE);
        i = call_record_remove(cr_pos);
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "bye_response_received_proc() exit---: No Matching Thread Found To Put:call_record_remove, cr_pos=%d \r\n", cr_pos);
        return -1;
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Bye Response Message Proc failed:caller_id=%s, callee_id=%s, dialog_index=%d, Cause=%s", caller_id, callee_id, dialog_index, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Bye响应消息处理失败:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, dialog_index=%d, 原因=%s", caller_id, caller_ip, caller_port, callee_id, dialog_index, (char*)"找不到对应的处理线程");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "bye_response_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : message_received_proc
 功能描述  : 收到message消息的处理函数
 输入参数  : char* caller_id
             char* caller_ip
             int caller_port
             char* callee_id
             char* caller_ip
             int caller_port,
             char* call_id
             int dialog_index
             char* msg_body
             int msg_len
             int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int message_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* callee_ip, int callee_port, char* call_id, int dialog_index, char* msg_body, int msg_len, int user_data)
{
    int i = 0;
    int pos = -1;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "message_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "message_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n caller_ip=%s \
    \r\n caller_port=%d \
    \r\n callee_id=%s \
    \r\n callee_ip=%s \
    \r\n callee_port=%d \
    \r\n dialog_index=%d \
    \r\n ", call_id, caller_id, caller_ip, caller_port, callee_id, callee_ip, callee_port, dialog_index);

    if ((0 == sstrcmp(caller_id, (char*)"wiscomCallerID"))
        && (0 == sstrcmp(callee_id, (char*)"wiscomCalleeID"))) /* 查询服务器ID和用户ID */
    {
        //printf("\r\n\r\n ******************** User Get ServerID And UserID Begin ******************* \r\n");
        i = message_get_service_id_proc(caller_id, caller_ip, caller_port, callee_id, callee_ip, callee_port, msg_body, msg_len);
        //printf("\r\n ******************** User Get ServerID And UserID End ******************* \r\n\r\n");
        return i;
    }

    /* 查找用户处理线程队列 */
    user_srv_thread = get_user_srv_proc_thread(caller_id, caller_ip, caller_port);

    if (NULL != user_srv_thread)
    {
        if (NULL != user_srv_thread->pUserSrvMsgQueue)
        {
            /* 添加到用户业务消息队列 */
            i = user_srv_msg_add(user_srv_thread, MSG_MESSAGE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_received_proc() user_srv_msg_add:i=%d \r\n", i);
            return 0;
        }
    }
    else
    {
        /* 可能是物理设备上报信息的消息，所以，先查找物理设备 */
#if 1
        /* 查找设备处理线程队列 */
        device_srv_thread = get_device_srv_proc_thread(caller_id, caller_ip, caller_port);

        if (NULL != device_srv_thread)
        {
            /* 添加到设备线程业务消息队列 */
            i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_MESSAGE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
            return 0;
        }
        else
#endif
        {
            pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(caller_id, caller_ip, caller_port);

            if (NULL != pGBDeviceInfo)
            {
                /* 查看设备的注册状态 */
                if (pGBDeviceInfo->reg_status > 0)
                {
                    /* 添加到设备业务消息队列 */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_MESSAGE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_received_proc() device_message_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
            }
            else
            {
                pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* 查看设备的注册状态 */
                    if (pGBDeviceInfo->reg_status > 0)
                    {
                        /* 添加到设备业务消息队列 */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_MESSAGE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_received_proc() device_message_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
                else
                {
                    /* 如果不是来自本服务器下面的,检查是否来自route */
#if 1
                    /* 查找上机平台处理线程队列 */
                    route_srv_thread = get_route_srv_proc_thread(caller_id, caller_ip, caller_port);

                    if (NULL != route_srv_thread)
                    {
                        /* 添加到上级平台线程业务消息队列 */
                        i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_MESSAGE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                        return 0;
                    }
                    else
#endif
                    {
                        pos = route_info_find_by_host_and_port(caller_ip, caller_port);

                        if (pos >= 0)
                        {
                            pRouteInfo = route_info_get(pos);

                            if (NULL != pRouteInfo)
                            {
                                /* 添加到互联路由业务消息队列 */
                                i = route_srv_msg_add(pRouteInfo, MSG_MESSAGE, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_received_proc() route_srv_msg_add:i=%d \r\n", i);
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Message Proc failed:caller_id=%s, caller_ip=%s, caller_port=%d, Cause=%s", caller_id, caller_ip, caller_port, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Message消息处理失败:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, 原因=%s", caller_id, caller_ip, caller_port, callee_id, (char*)"找不到对应的处理线程");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "message_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : message_response_received_proc
 功能描述  : 收到message回应消息的处理函数
 输入参数  : char* caller_id
             char* callee_id
             char* call_id
             int status_code
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int message_response_received_proc(char* caller_id, char* callee_id, char* call_id, int status_code, int user_data)
{
    int iRet = 0;
    int pos = -1;
    route_info_t* pRouteInfo = NULL;
    char strCallID[MAX_128CHAR_STRING_LEN + 4] = {0};

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "message_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "message_response_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n status_code=%d \
    \r\n ", call_id, caller_id, callee_id, status_code);

    /* 可能是其他消息(比如状态上报返回的消息，不一定是保活消息)返回的错误，所以，暂时屏蔽，需要增加判断条件 */
    if (status_code != 200)
    {
        pos = route_info_find(callee_id);

        if (pos < 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_response_received_proc() exit---: Find Route Info Error:callee_id=%s \r\n", callee_id);
            return -1;
        }

        pRouteInfo = route_info_get(pos);

        if (NULL == pRouteInfo)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_response_received_proc() exit---: Get Route Info Error:pos=%d \r\n", pos);
            return -1;
        }

        DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "message_response_received_proc() RouteID=%s \r\n", pRouteInfo->server_id);

        /* 匹配callID,看是否是保活消息 */
        snprintf(strCallID, MAX_128CHAR_STRING_LEN + 4, "%u", pRouteInfo->keep_alive_sn);

        if (0 == sstrcmp(strCallID, call_id))/* 如果是保活消息 */
        {
            pRouteInfo->failed_keep_alive_count++; /* 保活失败计数 */

            if (pRouteInfo->failed_keep_alive_count >= local_failed_keep_alive_count_get())
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "上级路保活失败次数达到%d次, 主动注销掉上级注册信息: 上级路由ID=%s, 上级路由IP=%s, 上级路由Port=%d", pRouteInfo->failed_keep_alive_count, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

                pRouteInfo->reg_status = 0;
                pRouteInfo->reg_info_index = -1;
                pRouteInfo->expires = 0;
                pRouteInfo->min_expires = 0;
                pRouteInfo->reg_interval = 0;
                pRouteInfo->keep_alive_count = local_keep_alive_interval_get();
                pRouteInfo->failed_keep_alive_count = 0;

                /* 移除上级锁定的点位信息 */
                iRet = RemoveGBLogicDeviceLockInfoByRouteInfo(pRouteInfo);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_response_received_proc() RemoveGBLogicDeviceLockInfoByRouteInfo OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }

                iRet = StopAllServiceTaskByCallerIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_response_received_proc() StopAllServiceTaskByCallerIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_response_received_proc() StopAllServiceTaskByCallerIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }

                /* 还有向上级CMS的请求 */
                iRet = StopAllServiceTaskByCalleeIPAndPort(pRouteInfo->server_ip, pRouteInfo->server_port);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_response_received_proc() StopAllServiceTaskByCalleeIPAndPort Error:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "message_response_received_proc() StopAllServiceTaskByCalleeIPAndPort OK:server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }

                /* 回收业务处理线程 */
                iRet = route_srv_proc_thread_recycle(pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);

                if (0 != iRet)
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_response_received_proc() route_srv_proc_thread_recycle Error:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
                else
                {
                    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "message_response_received_proc() route_srv_proc_thread_recycle OK:server_id=%s, server_ip=%s, server_port=%d, iRet=%d \r\n", pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port, iRet);
                }
            }
            else
            {
                pRouteInfo->keep_alive_count = local_keep_alive_interval_get();
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级路保活失败次数=%d次: 上级路由ID=%s, 上级路由IP=%s, 上级路由Port=%d", pRouteInfo->failed_keep_alive_count, pRouteInfo->server_id, pRouteInfo->server_ip, pRouteInfo->server_port);
            }
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : subscribe_received_proc
 功能描述  : 收到Subscribe消息的处理函数
 输入参数  : char* caller_id
             char* caller_ip
             int caller_port
             char* callee_id
             char* caller_ip
             int caller_port,
             char* call_id
             char* event_type 事件类型
             char* id_param 事件类型ID
             int subscribe_expires 超时时间
             char* msg_body
             int msg_len
             int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int subscribe_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* callee_ip, int callee_port, char* call_id, char* event_type, char* id_param, int subscribe_expires, char* msg_body, int msg_len, int user_data)
{
    int i = 0;
    int pos = -1;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    route_info_t* pRouteInfo = NULL;
    int event_id = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "subscribe_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "subscribe_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n caller_ip=%s \
    \r\n caller_port=%d \
    \r\n callee_id=%s \
    \r\n callee_ip=%s \
    \r\n callee_port=%d \
    \r\n event_type=%s \
    \r\n id_param=%s \
    \r\n subscribe_expires=%d \
    \r\n ", call_id, caller_id, caller_ip, caller_port, callee_id, callee_ip, callee_port, event_type, id_param, subscribe_expires);

    /* 目前仅支持目录订阅消息 */
    if (0 != sstrcmp(event_type, (char*)"Catalog"))
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "subscribe_received_proc() exit---: No Support Event Type:event_type=%s \r\n", event_type);
        return -1;
    }

    if (NULL == id_param)
    {
        event_id = osip_atoi(id_param);
    }

    /* 目前目录订阅消息仅仅来自于上级平台 */
    /* 查找上级别平台处理线程队列 */
    route_srv_thread = get_route_srv_proc_thread(caller_id, caller_ip, caller_port);

    if (NULL != route_srv_thread)
    {
        /* 添加到上级平台线程业务消息队列 */
        i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_SUBSCRIBE, caller_id, callee_id, event_id, NULL, subscribe_expires, msg_body, msg_len, -1);
        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "subscribe_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
        return 0;
    }
    else
    {
        pos = route_info_find_by_host_and_port(caller_ip, caller_port);

        if (pos >= 0)
        {
            pRouteInfo = route_info_get(pos);

            if (NULL != pRouteInfo)
            {
                /* 添加到互联路由业务消息队列 */
                i = route_srv_msg_add(pRouteInfo, MSG_SUBSCRIBE, caller_id, callee_id, event_id, NULL, subscribe_expires, msg_body, msg_len, -1);
                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "subscribe_received_proc() route_srv_msg_add:i=%d \r\n", i);
                return 0;
            }
        }
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Subscribe Message Proc failed:caller_id=%s, caller_ip=%s, caller_port=%d, Cause=%s", caller_id, caller_ip, caller_port, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Subscribe消息处理失败:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, 原因=%s", caller_id, caller_ip, caller_port, callee_id, (char*)"找不到对应的处理线程");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "subscribe_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : subscribe_response_received_proc
 功能描述  : 收到Subscribe回应消息的处理函数
 输入参数  : char* caller_id
             char* callee_id
             char* call_id
             int expires
             int status_code
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int subscribe_response_received_proc(char* caller_id, char* callee_id, char* call_id, int expires, int status_code, int user_data)
{
    GBDevice_info_t* pGBDeviceInfo = NULL;
    char strCallID[MAX_128CHAR_STRING_LEN + 4] = {0};

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "subscribe_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "subscribe_response_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n expires=%d \
    \r\n status_code=%d \
    \r\n ", call_id, caller_id, callee_id, expires, status_code);

    pGBDeviceInfo = GBDevice_info_find(callee_id);

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "subscribe_response_received_proc() exit---: Find GBDevice Info Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "subscribe_response_received_proc() DeviceID=%s \r\n", pGBDeviceInfo->device_id);

    /* 匹配callID,看是否是订阅消息 */
    snprintf(strCallID, MAX_128CHAR_STRING_LEN + 4, "%u", pGBDeviceInfo->catalog_subscribe_event_id);

    if (0 != sstrcmp(strCallID, call_id))
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "subscribe_response_received_proc() exit---: Call ID Not Equal: event_id=%d \r\n", pGBDeviceInfo->catalog_subscribe_event_id);
        return -1;
    }

    if (status_code == 200)
    {
        if (expires <= 0)
        {
            pGBDeviceInfo->catalog_subscribe_flag = 0;
            pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get();
            pGBDeviceInfo->catalog_subscribe_expires = 0;
            pGBDeviceInfo->catalog_subscribe_expires_count = 0;
            pGBDeviceInfo->catalog_subscribe_event_id = 0;
        }
        else
        {
            pGBDeviceInfo->catalog_subscribe_flag = 1;
            pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get();
            pGBDeviceInfo->catalog_subscribe_expires = expires;
            pGBDeviceInfo->catalog_subscribe_expires_count = expires;
        }
    }
    else
    {
        pGBDeviceInfo->catalog_subscribe_flag = 0;
        pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get();
        pGBDeviceInfo->catalog_subscribe_expires = 0;
        pGBDeviceInfo->catalog_subscribe_expires_count = 0;
        pGBDeviceInfo->catalog_subscribe_event_id = 0;
    }

    return 0;
}

int subscribe_within_dialog_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* call_id, int dialog_index, int subscribe_expires, char* msg_body, int msg_len)
{
    int i = 0;
    int pos = -1;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    route_info_t* pRouteInfo = NULL;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "subscribe_within_dialog_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "subscribe_within_dialog_received_proc() \
    \r\n In Param: \
    \r\n caller_id=%s \
    \r\n caller_ip=%s \
    \r\n caller_port=%d \
    \r\n callee_id=%s \
    \r\n call_id=%s \
    \r\n dialog_index=%d \
    \r\n subscribe_expires=%d \
    \r\n", caller_id, caller_ip, caller_port, callee_id, call_id, dialog_index, subscribe_expires);

    if (subscribe_expires <= 0) /* 去订阅 */
    {
        pos = route_info_find_by_host_and_port(caller_ip, caller_port);

        if (pos >= 0)
        {
            pRouteInfo = route_info_get(pos);

            if (NULL != pRouteInfo)
            {
                if (pRouteInfo->catalog_subscribe_dialog_index == dialog_index)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "上级CMS过来的目录订阅信息:上级CMS ID=%s, IP地址=%s, 端口号=%d, 收到上级CMS的注销订阅消息", caller_id, pRouteInfo->server_ip, pRouteInfo->server_port);

                    /* 更新状态 */
                    pRouteInfo->catalog_subscribe_flag = 0;
                    pRouteInfo->catalog_subscribe_expires = 0;
                    pRouteInfo->catalog_subscribe_expires_count = 0;
                    pRouteInfo->catalog_subscribe_dialog_index = -1;
                    return 0;
                }
                else
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Subscribe消息处理失败:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, 原因=%s, catalog_subscribe_dialog_index=%d, dialog_index=%d", caller_id, caller_ip, caller_port, callee_id, (char*)"目录订阅的SUBSCRIBE会话索引不匹配", pRouteInfo->catalog_subscribe_dialog_index, dialog_index);
                    return -1;
                }
            }
        }
    }
    else
    {
        /* 目前目录订阅消息仅仅来自于上级平台 */
        /* 查找上级平台处理线程队列 */
        route_srv_thread = get_route_srv_proc_thread(caller_id, caller_ip, caller_port);

        if (NULL != route_srv_thread)
        {
            /* 添加到上级平台线程业务消息队列 */
            i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_SUBSCRIBE, caller_id, callee_id, subscribe_expires, NULL, dialog_index, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "subscribe_within_dialog_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
            return 0;
        }
        else
        {
            pos = route_info_find_by_host_and_port(caller_ip, caller_port);

            if (pos >= 0)
            {
                pRouteInfo = route_info_get(pos);

                if (NULL != pRouteInfo)
                {
                    /* 添加到互联路由业务消息队列 */
                    i = route_srv_msg_add(pRouteInfo, MSG_SUBSCRIBE, caller_id, callee_id, subscribe_expires, NULL, dialog_index, msg_body, msg_len, -1);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "subscribe_within_dialog_received_proc() route_srv_msg_add:i=%d \r\n", i);
                    return 0;
                }
            }
        }
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Subscribe Message Proc failed:caller_id=%s, caller_ip=%s, caller_port=%d, Cause=%s", caller_id, caller_ip, caller_port, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Subscribe消息处理失败:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, 原因=%s", caller_id, caller_ip, caller_port, callee_id, (char*)"找不到对应的处理线程");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "subscribe_within_dialog_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

int subscribe_within_dialog_response_received_proc(char* caller_id, char* callee_id, char* call_id, int dialog_index, int expires, int status_code)
{
    GBDevice_info_t* pGBDeviceInfo = NULL;
    char strCallID[MAX_128CHAR_STRING_LEN + 4] = {0};

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "subscribe_within_dialog_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "subscribe_within_dialog_response_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n expires=%d \
    \r\n status_code=%d \
    \r\n ", call_id, caller_id, callee_id, dialog_index, expires, status_code);

    pGBDeviceInfo = GBDevice_info_find(callee_id);

    if (NULL == pGBDeviceInfo)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "subscribe_within_dialog_response_received_proc() exit---: Find GBDevice Info Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "subscribe_within_dialog_response_received_proc() DeviceID=%s \r\n", pGBDeviceInfo->device_id);

    /* 匹配callID,看是否是订阅消息 */
    snprintf(strCallID, MAX_128CHAR_STRING_LEN + 4, "%u", pGBDeviceInfo->catalog_subscribe_event_id);

    if (0 != sstrcmp(strCallID, call_id))
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "subscribe_within_dialog_response_received_proc() exit---: Call ID Not Equal: event_id=%d \r\n", pGBDeviceInfo->catalog_subscribe_event_id);
        return -1;
    }

    if (pGBDeviceInfo->catalog_subscribe_dialog_index != dialog_index)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "subscribe_within_dialog_response_received_proc() exit---: dialog_index Not Equal: catalog_subscribe_dialog_index=%d, dialog_index=%d \r\n", pGBDeviceInfo->catalog_subscribe_dialog_index, dialog_index);
        return -1;
    }

    if (status_code == 200)
    {
        if (expires <= 0)
        {
            pGBDeviceInfo->catalog_subscribe_flag = 0;
            pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get();
            pGBDeviceInfo->catalog_subscribe_expires = 0;
            pGBDeviceInfo->catalog_subscribe_expires_count = 0;
            pGBDeviceInfo->catalog_subscribe_event_id = 0;
            pGBDeviceInfo->catalog_subscribe_dialog_index = -1;
        }
        else
        {
            pGBDeviceInfo->catalog_subscribe_flag = 1;
            pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get();
            pGBDeviceInfo->catalog_subscribe_expires = expires;
            pGBDeviceInfo->catalog_subscribe_expires_count = expires;
        }
    }
    else
    {
        pGBDeviceInfo->catalog_subscribe_flag = 0;
        pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get();
        pGBDeviceInfo->catalog_subscribe_expires = 0;
        pGBDeviceInfo->catalog_subscribe_expires_count = 0;
        pGBDeviceInfo->catalog_subscribe_event_id = 0;
        pGBDeviceInfo->catalog_subscribe_dialog_index = -1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : notify_received_proc
 功能描述  : 收到notify消息的处理函数
 输入参数  : char* caller_id
             char* caller_ip
             int caller_port
             char* callee_id
             char* caller_ip
             int caller_port,
             char* call_id
             char* msg_body
             int msg_len
             int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int notify_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* callee_ip, int callee_port, char* call_id, char* msg_body, int msg_len, int user_data)
{
    int i = 0;
    int pos = -1;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "notify_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "notify_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n caller_ip=%s \
    \r\n caller_port=%d \
    \r\n callee_id=%s \
    \r\n callee_ip=%s \
    \r\n callee_port=%d \
    \r\n ", call_id, caller_id, caller_ip, caller_port, callee_id, callee_ip, callee_port);

    /* 查找用户处理线程队列 */
    user_srv_thread = get_user_srv_proc_thread(caller_id, caller_ip, caller_port);

    if (NULL != user_srv_thread)
    {
        if (NULL != user_srv_thread->pUserSrvMsgQueue)
        {
            /* 添加到用户业务消息队列 */
            i = user_srv_msg_add(user_srv_thread, MSG_NOTIFY, caller_id, callee_id, 0, NULL, -1, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "notify_received_proc() user_srv_msg_add:i=%d \r\n", i);
            return 0;
        }
    }
    else
    {
        /* 可能是物理设备上报信息的消息，所以，先查找物理设备 */
#if 1
        /* 查找设备处理线程队列 */
        device_srv_thread = get_device_srv_proc_thread(caller_id, caller_ip, caller_port);

        if (NULL != device_srv_thread)
        {
            /* 添加到设备线程业务消息队列 */
            i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_NOTIFY, caller_id, callee_id, 0, NULL, -1, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "notify_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
            return 0;
        }
        else
#endif
        {
            pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(caller_id, caller_ip, caller_port);

            if (NULL != pGBDeviceInfo)
            {
                /* 查看设备的注册状态 */
                if (pGBDeviceInfo->reg_status > 0)
                {
                    /* 添加到设备业务消息队列 */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_NOTIFY, caller_id, callee_id, 0, NULL, -1, msg_body, msg_len, -1);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "notify_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
            }
            else
            {
                pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* 查看设备的注册状态 */
                    if (pGBDeviceInfo->reg_status > 0)
                    {
                        /* 添加到设备业务消息队列 */
                        i = device_srv_msg_add(pGBDeviceInfo, MSG_NOTIFY, caller_id, callee_id, 0, NULL, -1, msg_body, msg_len, -1);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "notify_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                        return 0;
                    }
                }
                else
                {
                    /* 如果不是来自本服务器下面的,检查是否来自route */
#if 1
                    /* 查找上机平台处理线程队列 */
                    route_srv_thread = get_route_srv_proc_thread(caller_id, caller_ip, caller_port);

                    if (NULL != route_srv_thread)
                    {
                        /* 添加到上级平台线程业务消息队列 */
                        i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_NOTIFY, caller_id, callee_id, 0, NULL, -1, msg_body, msg_len, -1);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "notify_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                        return 0;
                    }
                    else
#endif
                    {
                        pos = route_info_find_by_host_and_port(caller_ip, caller_port);

                        if (pos >= 0)
                        {
                            pRouteInfo = route_info_get(pos);

                            if (NULL != pRouteInfo)
                            {
                                /* 添加到互联路由业务消息队列 */
                                i = route_srv_msg_add(pRouteInfo, MSG_NOTIFY, caller_id, callee_id, 0, NULL, -1, msg_body, msg_len, -1);
                                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "notify_received_proc() route_srv_msg_add:i=%d \r\n", i);
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Notify Message Proc failed:caller_id=%s, caller_ip=%s, caller_port=%d, Cause=%s", caller_id, caller_ip, caller_port, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Notify消息处理失败:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, 原因=%s", caller_id, caller_ip, caller_port, callee_id, (char*)"找不到对应的处理线程");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "notify_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : notify_response_received_proc
 功能描述  : 收到Notify回应消息的处理函数
 输入参数  : char* caller_id
             char* callee_id
             char* call_id
             int status_code
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int notify_response_received_proc(char* caller_id, char* callee_id, char* call_id, int status_code, int user_data)
{
    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "notify_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "notify_response_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n callee_id=%s \
    \r\n status_code=%d \
    \r\n ", call_id, caller_id, callee_id, status_code);

    if (status_code != 200)
    {
        // TODO:重新发送订阅消息
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : info_received_proc
 功能描述  : 收到info消息的处理函数
 输入参数  : char* caller_id
             char* caller_ip
             int caller_port
             char* callee_id
             char* call_id
             int dialog_index
             char* msg_body
             int msg_len
             int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int info_received_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* call_id, int dialog_index, char* msg_body, int msg_len, int user_data)
{
    int i = 0;
    int pos = -1;
    user_srv_proc_tl_t* user_srv_thread = NULL;
    device_srv_proc_tl_t* device_srv_thread = NULL;
    route_srv_proc_tl_t* route_srv_thread = NULL;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    route_info_t* pRouteInfo = NULL;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "info_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "info_received_proc() \
    \r\n In Param: \
    \r\n call_id=%s \
    \r\n caller_id=%s \
    \r\n caller_ip=%s \
    \r\n caller_port=%d \
    \r\n callee_id=%s \
    \r\n dialog_index=%d \
    \r\n", call_id, caller_id, caller_ip, caller_port, callee_id, dialog_index);

    /* 查找用户处理线程队列 */
    user_srv_thread = get_user_srv_proc_thread(caller_id, caller_ip, caller_port);

    if (NULL != user_srv_thread)
    {
        if (NULL != user_srv_thread->pUserSrvMsgQueue)
        {
            /* 添加到用户业务消息队列 */
            i = user_srv_msg_add(user_srv_thread, MSG_INFO, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "info_received_proc() user_srv_msg_add:i=%d \r\n", i);
            return 0;
        }
    }
    else
    {
        /* 可能来自前端设备*/
#if 1
        /* 查找设备处理线程队列 */
        device_srv_thread = get_device_srv_proc_thread(caller_id, caller_ip, caller_port);

        if (NULL != device_srv_thread)
        {
            /* 添加到设备线程业务消息队列 */
            i = device_srv_msg_add_for_appoint(device_srv_thread, MSG_INFO, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
            DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "info_received_proc() device_srv_msg_add_for_appoint:i=%d \r\n", i);
            return 0;
        }
        else
#endif
        {
            pGBDeviceInfo = GBDevice_info_find_by_id_ip_and_port(caller_id, caller_ip, caller_port);

            if (NULL != pGBDeviceInfo)
            {
                /* 查看设备的注册状态 */
                if (pGBDeviceInfo->reg_status > 0)
                {
                    /* 添加到设备业务消息队列 */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_INFO, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "info_received_proc() device_srv_msg_add 1:i=%d \r\n", i);
                    return 0;
                }
            }
            else
            {
                pGBDeviceInfo = GBDevice_info_find_by_ip_and_port(caller_ip, caller_port);

                if (NULL != pGBDeviceInfo)
                {
                    /* 添加到设备业务消息队列 */
                    i = device_srv_msg_add(pGBDeviceInfo, MSG_INFO, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                    DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "info_received_proc() device_srv_msg_add 2:i=%d \r\n", i);
                    return 0;
                }
                else
                {
                    /* 如果不是来自本服务器下面的,检查是否来自route */
#if 1
                    /* 查找上机平台处理线程队列 */
                    route_srv_thread = get_route_srv_proc_thread(caller_id, caller_ip, caller_port);

                    if (NULL != route_srv_thread)
                    {
                        /* 添加到上级平台线程业务消息队列 */
                        i = route_srv_msg_add_for_appoint(route_srv_thread, MSG_INFO, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                        DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "info_received_proc() route_srv_msg_add_for_appoint:i=%d \r\n", i);
                        return 0;
                    }
                    else
#endif
                    {
                        pos = route_info_find_by_host_and_port(caller_ip, caller_port);

                        if (pos >= 0)
                        {
                            pRouteInfo = route_info_get(pos);

                            if (NULL != pRouteInfo)
                            {
                                /* 添加到互联路由业务消息队列 */
                                i = route_srv_msg_add(pRouteInfo, MSG_INFO, caller_id, callee_id, 0, NULL, dialog_index, msg_body, msg_len, -1);
                                DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "info_received_proc() route_srv_msg_add:i=%d \r\n", i);
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }

    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Info Message Proc failed:caller_id=%s, caller_ip=%s, caller_port=%d, dialog_index=%d, Cause=%s", caller_id, caller_ip, caller_port, dialog_index, (char*)"No Matching Thread Found To Put");
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Info消息处理失败:caller_id=%s, caller_ip=%s, caller_port=%d, callee_id=%s, dialog_index=%d, 原因=%s", caller_id, caller_ip, caller_port, callee_id, dialog_index, (char*)"找不到对应的处理线程");

    DEBUG_TRACE(MODULE_COMMON, LOG_WARN, "info_received_proc() exit---: No Matching Thread Found To Put \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : info_response_received_proc
 功能描述  : 收到info回应消息的处理函数
 输入参数  : char* caller_id
             char* callee_id
             char* call_id
             int status_code
             int user_data
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int info_response_received_proc(char* caller_id, char* callee_id, char* call_id, int status_code, int user_data)
{
    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "info_response_received_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : ua_session_expires_proc
 功能描述  : UA 会话超时的处理函数
 输入参数  : int dialog_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月3日 星期二
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int ua_session_expires_proc(int dialog_index)
{
    int iRet = 0;

    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "ua_session_expires_proc() exit---: CMS Not Run Normal Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "ua_session_expires_proc() \
    \r\n In Param: \
    \r\n dialog_index=%d \
    \r\n", dialog_index);

    //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "ua_session_expires_proc() Enter------------: dialog_index=%d \r\n", dialog_index);
    iRet = StopAllServiceTaskByCallerUAIndex(dialog_index);
    iRet = StopAllServiceTaskByCalleeUAIndex(dialog_index);
    DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "ua_session_expires_proc() Exit------------: dialog_index=%d, iRet=%d \r\n", dialog_index, iRet);

    return 0;
}

/*****************************************************************************
 函 数 名  : dbg_printf_proc
 功能描述  : 调试打印函数
 输入参数  : int iLevel
             const char* FILENAME
             const char* FUNCTIONNAME
             int CODELINE
             const char* fmt
             ...
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void dbg_printf_proc(int iLevel, const char* FILENAME, const char* FUNCTIONNAME, int CODELINE, const char* fmt)
{
    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "dbg_printf_proc() exit---: CMS Not Run Normal Error \r\n");
        return;
    }

    DebugTrace(MODULE_SIPSTACK, iLevel, FILENAME, FUNCTIONNAME, CODELINE, fmt);
    return;
}

/*****************************************************************************
 函 数 名  : sip_message_trace_proc
 功能描述  : SIP消息调试跟踪
 输入参数  : int type:
             0,正确的
             1:发送错误的
             2:接收解析错误的
             3:接收消息错误的
             4:接收创建事务错误的
             int iDirect:
             0:发送的
             1:接收的
             char* ipaddr
             int port
             char* msg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年9月4日 星期三
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void sip_message_trace_proc(int type, int iDirect, char* ipaddr, int port, char* msg)
{
    if (0 == cms_run_status)
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "sip_message_trace_proc() exit---: CMS Not Run Normal Error \r\n");
        return;
    }

    DebugSIPMessage(type, iDirect, ipaddr, port, msg);
    return;
}

/*****************************************************************************
 函 数 名  : SetCallbackProcFunc
 功能描述  : 设置上层回调处理函数
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月10日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void SetCallbackProcFunc()
{
    app_set_uas_register_received_cb(&uas_register_received_proc);
    app_set_uas_register_received_timeout_cb(&uas_register_received_timeout_proc);
    app_set_uac_register_response_received_cb(&uac_register_response_received_proc, 0);
    app_set_invite_received_cb(&invite_received_proc, 0);
    app_set_invite_response_received_cb(&invite_response_received_proc, 0);
    app_set_cancel_received_cb(&cancel_received_proc, 0);
    app_set_ack_received_cb(&ack_received_proc, 0);
    app_set_bye_received_cb(&bye_received_proc, 0);
    app_set_bye_response_received_cb(&bye_response_received_proc, 0);
    app_set_message_received_cb(&message_received_proc, 0);
    app_set_message_response_received_cb(&message_response_received_proc, 0);
    app_set_subscribe_received_cb(&subscribe_received_proc, 0);
    app_set_subscribe_response_received_cb(&subscribe_response_received_proc, 0);
    app_set_subscribe_within_dialog_received_cb(&subscribe_within_dialog_received_proc);
    app_set_subscribe_within_dialog_response_received_cb(&subscribe_within_dialog_response_received_proc);
    app_set_notify_received_cb(&notify_received_proc, 0);
    app_set_notify_response_received_cb(&notify_response_received_proc, 0);
    app_set_info_received_cb(&info_received_proc, 0);
    app_set_info_response_received_cb(&info_response_received_proc, 0);
    app_set_ua_session_expires_cb(&ua_session_expires_proc);
    app_set_dbg_printf_cb(&dbg_printf_proc);
    app_set_sip_message_trace_cb(&sip_message_trace_proc);
    return;
}

/*****************************************************************************
 函 数 名  : message_get_service_id_proc
 功能描述  : 获取服务器ID和用户ID
 输入参数  : char* caller_id
             char* caller_ip
             int caller_port
             char* callee_id
             char* callee_ip
             int callee_port
             char* msg_body
             int msg_body_len
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月26日 星期一
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int message_get_service_id_proc(char* caller_id, char* caller_ip, int caller_port, char* callee_id, char* callee_ip, int callee_port, char* msg_body, int msg_body_len)
{
    int i = 0;
    int iRet = 0;
    char strSN[32] = {0};
    char strServerIP[MAX_IP_LEN] = {0};
    char strUserName[36] = {0};
    char strServerID[36] = {0};

    xml_type_t xml_type = XML_TYPE_NULL;
    CPacket inPacket;
    vector<string> NodeName_Vector;
    CPacket outPacket;
    DOMElement* AccNode = NULL;
    char strErrorCode[32] = {0};

    if ((NULL == caller_id) || (NULL == caller_ip) || (caller_port <= 0))
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "message_get_service_id_proc() exit---: User Info Error \r\n");
        return -1;
    }

    if ((NULL == callee_id) || (NULL == callee_ip) || (callee_port <= 0))
    {
        DEBUG_TRACE(MODULE_COMMON, LOG_DEBUG, "message_get_service_id_proc() exit---: Callee Info Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "message_get_service_id_proc() Enter---: caller_id=%s, caller_ip=%s, caller_port=%d \r\n", caller_id, caller_ip, caller_port);
    //printf("\r\n ********** message_get_service_id_proc() Enter---: caller_id=%s,user_ip=%s,user_port=%d \r\n", caller_id, pUserInfo->login_ip, pUserInfo->login_port);
    //printf_system_time();
    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "设备或者用户登录前开始获取服务器ID处理:请求方IP地址=%s, 端口号=%d, 服务器IP=%s", caller_ip, caller_port, callee_ip);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Before the user log in,start to get server ID and user ID:Caller IP=%s, Port=%d, Server IP=%s", caller_ip, caller_port, callee_ip);
    //解析XML
    iRet = inPacket.BuiltTree(msg_body, msg_body_len);//生成DOM树结构.

    if (iRet < 0)
    {
        /* 回复响应,组建消息 */
        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"GetServerID");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"Error");

        AccNode = outPacket.CreateElement((char*)"ErrCode");
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_XML_BUILD_TREE_ERROR);
        outPacket.SetElementValue(AccNode, strErrorCode);

        AccNode = outPacket.CreateElement((char*)"Reason");
        outPacket.SetElementValue(AccNode, (char*)"XML Build Tree Error");

        i = SIP_SendMessage(NULL, caller_id, callee_id, callee_ip, callee_port, caller_ip, caller_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_get_service_id_proc() SIP_SendMessage Error:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "message_get_service_id_proc() SIP_SendMessage OK:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
        }

        SystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "设备或者用户登录前开始获取服务器ID处理失败:请求方IP地址=%s, 端口号=%d, 原因=%s", caller_ip, caller_port, (char*)"XML解析失败");
        EnSystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "Before the user log in,start to get server ID and user ID failed:user IP:%s , port=%d , reason= %s", caller_ip, caller_port, (char*)"XML parsing failed.");
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR,  "message_get_service_id_proc() exit---: XML Build Tree Error \r\nmsg=%s \r\n", msg_body);
        return iRet;
    }

    NodeName_Vector.clear();
    DOMDocument* pDOMDocument = inPacket.GetDOMDocument();
    DOMElement* pDOMElement = pDOMDocument->get_root();
    pDOMElement->ClearNodeNumber();

    if (pDOMElement->GetNodeName(NodeName_Vector) <= 0)
    {
        /* 回复响应,组建消息 */
        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"GetServerID");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"Error");

        AccNode = outPacket.CreateElement((char*)"ErrCode");
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_XML_GET_NODE_ERROR);
        outPacket.SetElementValue(AccNode, strErrorCode);

        AccNode = outPacket.CreateElement((char*)"Reason");
        outPacket.SetElementValue(AccNode, (char*)"Get Node Name Error");

        i = SIP_SendMessage(NULL, caller_id, callee_id, callee_ip, callee_port, caller_ip, caller_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_get_service_id_proc() SIP_SendMessage Error:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "message_get_service_id_proc() SIP_SendMessage OK:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
        }

        SystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "设备或者用户登录前开始获取服务器ID处理失败:请求方IP地址=%s, 端口号=%d, 原因=%s", caller_ip, caller_port, (char*)"XML解析失败");
        EnSystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "Before the user log in,start to get server ID and user ID failed:user IP:%s , port=%d , reason= %s", caller_ip, caller_port, (char*)"XML parsing failed.");
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_get_service_id_proc() exit---: Get Node Name Error \r\n");
        return -1;
    }

    /* 解析出xml的消息类型 */
    xml_type = get_xml_type_from_xml_body(NodeName_Vector, inPacket);

    if (XML_QUERY_SERVERID == xml_type) /* 查询命令 */
    {
        /* 取得数据*/
        inPacket.GetElementValue((char*)"SN", strSN);
        inPacket.GetElementValue((char*)"ServerIP", strServerIP);
        inPacket.GetElementValue((char*)"UserName", strUserName);

        DEBUG_TRACE(MODULE_COMMON, LOG_INFO,  "message_get_service_id_proc() \
    \r\n XML Para: \
    \r\n SN=%s, ServerIP=%s, UserName=%s \r\n", strSN, strServerIP, strUserName);

        if (pGblconf->board_id[0] == '\0' || pGblconf->center_code[0] == '\0' || pGblconf->trade_code[0] == '\0')
        {
            /* 回复响应,组建消息 */
            outPacket.SetRootTag("Response");

            AccNode = outPacket.CreateElement((char*)"CmdType");
            outPacket.SetElementValue(AccNode, (char*)"GetServerID");

            AccNode = outPacket.CreateElement((char*)"SN");
            outPacket.SetElementValue(AccNode, strSN);

            AccNode = outPacket.CreateElement((char*)"Result");
            outPacket.SetElementValue(AccNode, (char*)"Error");

            AccNode = outPacket.CreateElement((char*)"ErrCode");
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_LOCAL_CMS_ID_ERROR);
            outPacket.SetElementValue(AccNode, strErrorCode);

            AccNode = outPacket.CreateElement((char*)"Reason");
            outPacket.SetElementValue(AccNode, (char*)"CMS ID Not Config Or Not Right");

            i = SIP_SendMessage(NULL, caller_id, callee_id, callee_ip, callee_port, caller_ip, caller_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

            if (i != 0)
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_get_service_id_proc() SIP_SendMessage Error:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
            }
            else
            {
                DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "message_get_service_id_proc() SIP_SendMessage OK:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
            }

            SystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "设备或者用户登录前开始获取服务器ID处理失败:请求方IP地址=%s, 端口号=%d, 原因=%s", caller_ip, caller_port, (char*)"服务器ID没有配置或配置错误");
            EnSystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "Before the user log in,start to get server ID and user ID failed:user IP=%s, port=%d , reason= %s", caller_ip, caller_port, (char*)"Server ID is not configured or configuration errors.");
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_get_service_id_proc() exit---: CMS ID Not Config Or Not Right \r\n");
            return -1;
        }

        if (strUserName[0] != '\0')
        {
            i = user_get_service_id_proc(caller_id, caller_ip, caller_port, callee_id, callee_ip, callee_port, strSN, strServerIP, strUserName, &g_DBOper);
        }
        else /* 设备不需要获取本身的ID */
        {
            i = device_get_service_id_proc(caller_id, caller_ip, caller_port, callee_id, callee_ip, callee_port, strSN, strServerIP, &g_DBOper);
        }
    }
    else if (XML_RESPONSE_QUERY_SERVERID == xml_type) /* 查询响应命令 */
    {
        /* 取得数据*/
        inPacket.GetElementValue((char*)"SN", strSN);
        inPacket.GetElementValue((char*)"ServerID", strServerID);

        if (!sys_show_code_flag_get()) /* 非国标模式 */
        {
            i = route_get_service_id_response_proc(caller_id, caller_ip, caller_port, callee_id, callee_ip, callee_port, strSN, strServerID, &g_DBOper);
        }
        else
        {
            SystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "系统处于国标模式状态, 不对获取上级CMS ID返回消息进行处理:上级IP地址=%s, 端口号=%d, 返回的服务器ID=%s", caller_ip, caller_port, strServerID);
            EnSystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, " The system is in GB mode state, does not process the message returned by superior CMS: superior IP address=%s, port=%d, return service ID=%s", caller_ip, caller_port, strServerID);
        }
    }
    else
    {
        /* 回复响应,组建消息 */
        outPacket.SetRootTag("Response");

        AccNode = outPacket.CreateElement((char*)"CmdType");
        outPacket.SetElementValue(AccNode, (char*)"GetServerID");

        AccNode = outPacket.CreateElement((char*)"SN");
        outPacket.SetElementValue(AccNode, strSN);

        AccNode = outPacket.CreateElement((char*)"Result");
        outPacket.SetElementValue(AccNode, (char*)"Error");

        AccNode = outPacket.CreateElement((char*)"ErrCode");
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_XML_GET_MSG_TYPE_ERROR);
        outPacket.SetElementValue(AccNode, strErrorCode);

        AccNode = outPacket.CreateElement((char*)"Reason");
        outPacket.SetElementValue(AccNode, (char*)"Get XML Type Error");

        i = SIP_SendMessage(NULL, caller_id, callee_id, callee_ip, callee_port, caller_ip, caller_port, (char*)outPacket.GetXml(NULL).c_str(), outPacket.GetXml(NULL).length());

        if (i != 0)
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_get_service_id_proc() SIP_SendMessage Error:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
        }
        else
        {
            DEBUG_TRACE(MODULE_COMMON, LOG_TRACE, "message_get_service_id_proc() SIP_SendMessage OK:user_id=%s, user_ip=%s, user_port=%d \r\n", caller_id, caller_ip, caller_port);
        }

        SystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "设备或者用户登录前开始获取服务器ID处理失败:请求方IP地址=%s, 端口号=%d, 原因=%s", caller_ip, caller_port, (char*)"XML命令类型失败");
        EnSystemLog(EV9000_CMS_GET_SERVERID_ERROR, EV9000_LOG_LEVEL_ERROR, "Before the user log in,start to get server ID and user ID failed:user IP:%s , port=%d , reason= %s", caller_ip, caller_port, (char*)"XML command type failed.");
        DEBUG_TRACE(MODULE_COMMON, LOG_ERROR, "message_get_service_id_proc() exit---: Message Type Error \r\n");
        return -1;
    }

    return i;
}
#endif
