
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
#include "common/db_proc.h"
#include "common/gblfunc_proc.inc"
#include "common/log_proc.inc"

#include "device/device_reg_proc.inc"
#include "device/device_thread_proc.inc"

#include "user/user_srv_proc.inc"
#include "service/alarm_proc.inc"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern gbl_conf_t* pGblconf;              /* 全局配置信息 */
extern BOARD_NET_ATTR  g_BoardNetConfig;
extern unsigned int g_RegistrationLimit;  /* 注册设备限制数 */

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
GBDevice_reg_msg_queue g_GBDeviceRegMsgQueue;   /* 标准物理设备注册消息队列 */
GBDevice_reg_msg_queue g_GBDeviceUnRegMsgQueue; /* 标准物理设备注销消息队列 */
#ifdef MULTI_THR
osip_mutex_t* g_GBDeviceRegMsgQueueLock = NULL;
#endif

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#if DECS("标准物理设备注册消息队列")
/*****************************************************************************
 函 数 名  : GBDevice_reg_msg_init
 功能描述  : 标准物理设备注册消息结构初始化
 输入参数  : GBDevice_reg_msg_t ** GBDevice_reg_msg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int GBDevice_reg_msg_init(GBDevice_reg_msg_t** GBDevice_reg_msg)
{
    *GBDevice_reg_msg = (GBDevice_reg_msg_t*)osip_malloc(sizeof(GBDevice_reg_msg_t));

    if (*GBDevice_reg_msg == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_reg_msg_init() exit---: *GBDevice_reg_msg Smalloc Error \r\n");
        return -1;
    }

    (*GBDevice_reg_msg)->register_id[0] = '\0';
    (*GBDevice_reg_msg)->device_type = 0;
    (*GBDevice_reg_msg)->login_ip[0] = '\0';
    (*GBDevice_reg_msg)->login_port = 0;
    (*GBDevice_reg_msg)->register_name[0] = '\0';
    (*GBDevice_reg_msg)->expires = 0;
    (*GBDevice_reg_msg)->reg_info_index = -1;

    return 0;
}

/*****************************************************************************
 函 数 名  : GBDevice_reg_msg_free
 功能描述  : 标准物理设备注册消息结构释放
 输入参数  : GBDevice_reg_msg_t * GBDevice_reg_msg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void GBDevice_reg_msg_free(GBDevice_reg_msg_t* GBDevice_reg_msg)
{
    if (GBDevice_reg_msg == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_reg_msg_free() exit---: Param Error \r\n");
        return;
    }

    memset(GBDevice_reg_msg->register_id, 0, MAX_ID_LEN + 4);
    GBDevice_reg_msg->device_type = 0;
    memset(GBDevice_reg_msg->login_ip, 0, MAX_IP_LEN);
    GBDevice_reg_msg->login_port = 0;
    memset(GBDevice_reg_msg->register_name, 0, MAX_128CHAR_STRING_LEN + 4);
    GBDevice_reg_msg->expires = 0;
    GBDevice_reg_msg->reg_info_index = -1;
    return;
}

/*****************************************************************************
 函 数 名  : GBDevice_reg_msg_list_init
 功能描述  : 标准物理设备注册消息队列初始化
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
int GBDevice_reg_msg_list_init()
{
    g_GBDeviceRegMsgQueue.clear();
    g_GBDeviceUnRegMsgQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_GBDeviceRegMsgQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_GBDeviceRegMsgQueueLock)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_reg_msg_list_init() exit---: GBDevice Register Message List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : GBDevice_reg_msg_list_free
 功能描述  : 标准物理设备注册消息队列释放
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
void GBDevice_reg_msg_list_free()
{
    GBDevice_reg_msg_t* pGBDeviceRegMsg = NULL;

    while (!g_GBDeviceRegMsgQueue.empty())
    {
        pGBDeviceRegMsg = (GBDevice_reg_msg_t*) g_GBDeviceRegMsgQueue.front();
        g_GBDeviceRegMsgQueue.pop_front();

        if (NULL != pGBDeviceRegMsg)
        {
            GBDevice_reg_msg_free(pGBDeviceRegMsg);
            osip_free(pGBDeviceRegMsg);
            pGBDeviceRegMsg = NULL;
        }
    }

    g_GBDeviceRegMsgQueue.clear();

    while (!g_GBDeviceUnRegMsgQueue.empty())
    {
        pGBDeviceRegMsg = (GBDevice_reg_msg_t*) g_GBDeviceUnRegMsgQueue.front();
        g_GBDeviceUnRegMsgQueue.pop_front();

        if (NULL != pGBDeviceRegMsg)
        {
            GBDevice_reg_msg_free(pGBDeviceRegMsg);
            osip_free(pGBDeviceRegMsg);
            pGBDeviceRegMsg = NULL;
        }
    }

    g_GBDeviceUnRegMsgQueue.clear();

#ifdef MULTI_THR

    if (NULL != g_GBDeviceRegMsgQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_GBDeviceRegMsgQueueLock);
        g_GBDeviceRegMsgQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 函 数 名  : GBDevice_reg_msg_list_clean
 功能描述  : 标准物理设备注册消息队列清除
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
void GBDevice_reg_msg_list_clean()
{
    GBDevice_reg_msg_t* pGBDeviceRegMsg = NULL;

    while (!g_GBDeviceRegMsgQueue.empty())
    {
        pGBDeviceRegMsg = (GBDevice_reg_msg_t*) g_GBDeviceRegMsgQueue.front();
        g_GBDeviceRegMsgQueue.pop_front();

        if (NULL != pGBDeviceRegMsg)
        {
            GBDevice_reg_msg_free(pGBDeviceRegMsg);
            osip_free(pGBDeviceRegMsg);
            pGBDeviceRegMsg = NULL;
        }
    }

    g_GBDeviceRegMsgQueue.clear();

    return;
}

/*****************************************************************************
 函 数 名  : GBDevice_unreg_msg_list_clean
 功能描述  : 标准物理设备注销消息队列清除
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年3月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void GBDevice_unreg_msg_list_clean()
{
    GBDevice_reg_msg_t* pGBDeviceRegMsg = NULL;

    while (!g_GBDeviceUnRegMsgQueue.empty())
    {
        pGBDeviceRegMsg = (GBDevice_reg_msg_t*) g_GBDeviceUnRegMsgQueue.front();
        g_GBDeviceUnRegMsgQueue.pop_front();

        if (NULL != pGBDeviceRegMsg)
        {
            GBDevice_reg_msg_free(pGBDeviceRegMsg);
            osip_free(pGBDeviceRegMsg);
            pGBDeviceRegMsg = NULL;
        }
    }

    g_GBDeviceUnRegMsgQueue.clear();

    return;
}

/*****************************************************************************
 函 数 名  : GBDevice_reg_msg_add
 功能描述  : 添加标准物理设备注册消息到队列中
 输入参数  : char* device_id
                            int device_type
                            char* login_ip
                            int login_port
                            char* register_name
                            int expires
                            int reg_info_index
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int GBDevice_reg_msg_add(char* device_id, int device_type, char* login_ip, int login_port, char* register_name, int expires, int reg_info_index, int link_type)
{
    GBDevice_reg_msg_t* pGBDeviceRegMsg = NULL;
    int iRet = 0;

    if (device_id == NULL)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_reg_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = GBDevice_reg_msg_init(&pGBDeviceRegMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    if ('\0' != device_id[0])
    {
        osip_strncpy(pGBDeviceRegMsg->register_id, device_id, MAX_ID_LEN);
    }

    pGBDeviceRegMsg->device_type = device_type;

    if ('\0' != login_ip[0])
    {
        osip_strncpy(pGBDeviceRegMsg->login_ip, login_ip, MAX_IP_LEN);
    }

    pGBDeviceRegMsg->login_port = login_port;

    if (NULL != register_name && '\0' != register_name[0])
    {
        osip_strncpy(pGBDeviceRegMsg->register_name, register_name, MAX_128CHAR_STRING_LEN);
    }
    else
    {
        osip_strncpy(pGBDeviceRegMsg->register_name, device_id, MAX_128CHAR_STRING_LEN);
    }

    pGBDeviceRegMsg->expires = expires;
    pGBDeviceRegMsg->reg_info_index = reg_info_index;
    pGBDeviceRegMsg->link_type = link_type;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_GBDeviceRegMsgQueueLock);
#endif

    if (expires <= 0)
    {
        g_GBDeviceUnRegMsgQueue.push_back(pGBDeviceRegMsg);
    }
    else
    {
        g_GBDeviceRegMsgQueue.push_back(pGBDeviceRegMsg);
    }


#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_GBDeviceRegMsgQueueLock);
#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : scan_GBDevice_reg_msg_list
 功能描述  : 扫描设备注册消息队列
 输入参数  : DBOper* pDevice_Reg_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void scan_GBDevice_reg_msg_list(DBOper* pDevice_Reg_dboper)
{
    int iRet = 0;
    GBDevice_reg_msg_t* pGBDeviceRegMsg = NULL;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_GBDeviceRegMsgQueueLock);
#endif

    while (!g_GBDeviceRegMsgQueue.empty())
    {
        pGBDeviceRegMsg = (GBDevice_reg_msg_t*) g_GBDeviceRegMsgQueue.front();
        g_GBDeviceRegMsgQueue.pop_front();

        if (NULL != pGBDeviceRegMsg)
        {
            break;
        }
    }

#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_GBDeviceRegMsgQueueLock);
#endif

    if (NULL != pGBDeviceRegMsg)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_INFO,  "scan_GBDevice_reg_msg_list() \
    \r\n In Param: \
    \r\n register_id=%s \
    \r\n login_ip=%s \
    \r\n login_port=%d \
    \r\n register_name=%s \
    \r\n reg_info_index=%d \
    \r\n expires=%d \
    \r\n ", pGBDeviceRegMsg->register_id, pGBDeviceRegMsg->login_ip, pGBDeviceRegMsg->login_port, pGBDeviceRegMsg->register_name, pGBDeviceRegMsg->reg_info_index, pGBDeviceRegMsg->expires);

        iRet = GBDevice_reg_msg_proc(pGBDeviceRegMsg, pDevice_Reg_dboper);
        GBDevice_reg_msg_free(pGBDeviceRegMsg);
        osip_free(pGBDeviceRegMsg);
        pGBDeviceRegMsg = NULL;
    }

    return;
}

/*****************************************************************************
 函 数 名  : scan_GBDevice_unreg_msg_list
 功能描述  : 扫描设备注销消息队列
 输入参数  : DBOper* pDevice_Reg_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年3月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void scan_GBDevice_unreg_msg_list(DBOper* pDevice_Reg_dboper)
{
    int iRet = 0;
    GBDevice_reg_msg_t* pGBDeviceRegMsg = NULL;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_GBDeviceRegMsgQueueLock);
#endif

    while (!g_GBDeviceUnRegMsgQueue.empty())
    {
        pGBDeviceRegMsg = (GBDevice_reg_msg_t*) g_GBDeviceUnRegMsgQueue.front();
        g_GBDeviceUnRegMsgQueue.pop_front();

        if (NULL != pGBDeviceRegMsg)
        {
            break;
        }
    }

#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_GBDeviceRegMsgQueueLock);
#endif

    if (NULL != pGBDeviceRegMsg)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_INFO,  "scan_GBDevice_unreg_msg_list() \
    \r\n In Param: \
    \r\n register_id=%s \
    \r\n login_ip=%s \
    \r\n login_port=%d \
    \r\n register_name=%s \
    \r\n reg_info_index=%d \
    \r\n expires=%d \
    \r\n ", pGBDeviceRegMsg->register_id, pGBDeviceRegMsg->login_ip, pGBDeviceRegMsg->login_port, pGBDeviceRegMsg->register_name, pGBDeviceRegMsg->reg_info_index, pGBDeviceRegMsg->expires);

        iRet = GBDevice_unreg_msg_proc(pGBDeviceRegMsg, pDevice_Reg_dboper);
        GBDevice_reg_msg_free(pGBDeviceRegMsg);
        osip_free(pGBDeviceRegMsg);
        pGBDeviceRegMsg = NULL;
    }

    return;
}
#endif

/*****************************************************************************
 函 数 名  : GetDevCfg
 功能描述  : 获取数据库设备配置
 输入参数  : DBOper* pdboper
             string strDevID
             GBDevice_cfg_t& GBDevice_cfg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GetDevCfg(DBOper* pdboper, string strDevID, GBDevice_cfg_t& GBDevice_cfg)
{
    string strSQL = "";
    int record_count = 0;

    if (NULL == pdboper)
    {
        return -1;
    }

    strSQL.clear();
    strSQL = "select * from GBPhyDeviceConfig WHERE DeviceID like '";
    strSQL += strDevID + "';";
    record_count = pdboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GetDevCfg() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GetDevCfg() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "GetDevCfg() exit---: No Record Count \r\n");
        return 0;
    }

    int tmp_ivalue = 0;
    string tmp_svalue = "";

    /* 设备索引*/
    tmp_ivalue = 0;
    pdboper->GetFieldValue("ID", tmp_ivalue);

    GBDevice_cfg.id = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.id:%d", GBDevice_cfg.id);


    /* 设备统一编号 */
    tmp_svalue.clear();
    pdboper->GetFieldValue("DeviceID", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.device_id = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_id:%s", (char*)GBDevice_cfg.device_id.c_str());
    }

    /* 是否启用*/
    tmp_ivalue = 0;
    pdboper->GetFieldValue("Enable", tmp_ivalue);

    GBDevice_cfg.enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.enable:%d", GBDevice_cfg.enable);


    /* 设备类型(前端摄像机、NVR、互联CMS、TSU) */
    tmp_ivalue = 0;
    pdboper->GetFieldValue("DeviceType", tmp_ivalue);

    GBDevice_cfg.device_type = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_type:%d", GBDevice_cfg.device_type);


    /* 注册账号 */
    tmp_svalue.clear();
    pdboper->GetFieldValue("UserName", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.register_account = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.register_account:%s", (char*)GBDevice_cfg.register_account.c_str());
    }

    /* 注册密码 */
    tmp_svalue.clear();
    pdboper->GetFieldValue("Password", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.register_password = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.register_password:%s", (char*)GBDevice_cfg.register_password.c_str());
    }

    /* 设备ip地址 */
    tmp_svalue.clear();
    pdboper->GetFieldValue("DeviceIP", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.device_ip = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_ip:%s", (char*)GBDevice_cfg.device_ip.c_str());
    }

    /* 设备视频输入通道数 */
    tmp_ivalue = 0;
    pdboper->GetFieldValue("MaxCamera", tmp_ivalue);

    GBDevice_cfg.device_max_camera = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_max_camera:%d", GBDevice_cfg.device_max_camera);


    /* 设备报警输入通道数 */
    tmp_ivalue = 0;
    pdboper->GetFieldValue("MaxAlarm", tmp_ivalue);

    GBDevice_cfg.device_max_alarm = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_max_alarm:%d", GBDevice_cfg.device_max_alarm);


    /* 设备生产商 */
    tmp_svalue.clear();
    pdboper->GetFieldValue("Manufacturer", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.device_manufacturer = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_manufacturer:%s", (char*)GBDevice_cfg.device_manufacturer.c_str());
    }

    /* 设备型号 */
    tmp_svalue.clear();
    pdboper->GetFieldValue("Model", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.device_model = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_model:%s", (char*)GBDevice_cfg.device_model.c_str());
    }

    /* 设备版本 */
    tmp_svalue.clear();
    pdboper->GetFieldValue("Firmware", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.device_firmware = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.device_firmware:%s \r\n", (char*)GBDevice_cfg.device_firmware.c_str());
    }

    /* 联网类型:0:上下级，1：同级，默认0 */
    tmp_ivalue = 0;
    pdboper->GetFieldValue("LinkType", tmp_ivalue);

    GBDevice_cfg.link_type = tmp_ivalue;
    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.link_type:%d", GBDevice_cfg.link_type);

    /* CMS ID */
    tmp_svalue.clear();
    pdboper->GetFieldValue("CMSID", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        GBDevice_cfg.cms_id = tmp_svalue;
        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GetDevCfg() GBDevice_cfg.cms_id:%s \r\n", (char*)GBDevice_cfg.cms_id.c_str());
    }

    /* 传输方式 */
    tmp_ivalue = 0;
    pdboper->GetFieldValue("TransferProtocol", tmp_ivalue);

    if (tmp_ivalue == 1)
    {
        GBDevice_cfg.trans_protocol = 1;
    }
    else
    {
        GBDevice_cfg.trans_protocol = 0;
    }

    //DEBUG_TRACE(MODULE_COMMON, LOG_INFO, "GetDevCfg() GBDevice_cfg.trans_protocol:%d", GBDevice_cfg.trans_protocol);

    return record_count;
}

/*****************************************************************************
 函 数 名  : DevRefresh
 功能描述  : 设备刷新处理
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DevRefresh(GBDevice_info_t* pGBDeviceInfo, DBOper* pdboper)
{
    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() REFRESH \r\n");
    pGBDeviceInfo->auth_count = 0;   //?
    pGBDeviceInfo->reg_status = 1;
    /* 发送响应消息*/
    SIP_UASAnswerToRegister(pGBDeviceInfo->reg_info_index, 200, NULL);
    return 0;
}

/*****************************************************************************
 函 数 名  : DevUnReg
 功能描述  : 设备注销处理
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DevUnReg(GBDevice_info_t* pGBDeviceInfo, DBOper* pdboper)
{
    int i = 0;
    unsigned int uType = EV9000_ALARM_ACCESS_DEVICE_ERROR;

    if (NULL == pGBDeviceInfo || NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "DevUnReg() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() Enter---\r\n");
    printf("DevUnReg() Enter---\r\n");

    /* 发送响应消息*/
    SIP_UASAnswerToRegister(pGBDeviceInfo->reg_info_index, 200, NULL);

    pGBDeviceInfo->auth_count = 0;
    pGBDeviceInfo->reg_status = 0;
    UpdateGBDeviceRegStatus2DB(pGBDeviceInfo, pdboper);

    SystemLog(EV9000_CMS_DEVICE_OFFLINE, EV9000_LOG_LEVEL_ERROR, "设备下线:设备ID=%s, IP地址=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
    EnSystemLog(EV9000_CMS_DEVICE_OFFLINE, EV9000_LOG_LEVEL_ERROR, "DevUnReg:ID=%s, IP=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DevUnReg() REMOVE:device_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->reg_info_index);
    printf("DevUnReg() REMOVE:device_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->reg_info_index);

    /* 设备故障报警 */
    if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type)
    {
        uType = EV9000_ALARM_DEC_TSU_ERROR;
    }
    else if (EV9000_DEVICETYPE_VIDEODIAGNOSIS == pGBDeviceInfo->device_type)
    {
        uType = EV9000_ALARM_DIAGNOSE_ERROR;
    }
    else if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
    {
        uType = EV9000_ALARM_INTELLIGENT_ERROR;
    }
    else if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type)
    {
        uType = EV9000_ALARM_CMS_ERROR;
    }

    i = SendGBPhyDeviceOffLineAlarmToAllClientUser(uType, pGBDeviceInfo->id);
    printf("DevUnReg() SendGBPhyDeviceOffLineAlarmToAllClientUser() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() SendGBPhyDeviceOffLineAlarmToAllClientUser Error:DeviceID=%s, iRet=%d \r\n", pGBDeviceInfo->device_id, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() SendGBPhyDeviceOffLineAlarmToAllClientUser OK:DeviceID=%s, iRet=%d \r\n", pGBDeviceInfo->device_id, i);
    }

    //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DevUnReg() Enter------------: DeviceID=%s \r\n", pGBDeviceInfo->device_id);
    /* 查找点位的业务，并停止所有业务 */
    if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type) /* 解码器根据主叫侧信息停止业务 */
    {
        i = StopAllServiceTaskByCallerIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        printf("DevUnReg() StopAllServiceTaskByCallerIPAndPort() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() StopAllServiceTaskByCallerIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() StopAllServiceTaskByCallerIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
    }
    else if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type) /* IVS根据主叫、被叫侧信息停止业务 */
    {
        i = StopAllServiceTaskByCallerIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port); /* 主动请求的视频流 */
        printf("DevUnReg() StopAllServiceTaskByCallerIPAndPort() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() StopAllServiceTaskByCallerIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() StopAllServiceTaskByCallerIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }

        i = StopAllServiceTaskByCalleeIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port); /* 被动发送的智能分析频流 */
        printf("DevUnReg() StopAllServiceTaskByCalleeIPAndPort() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
    }
    else if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type) /* 下级CMS */
    {
        i = StopAllServiceTaskByCalleeIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        printf("DevUnReg() StopAllServiceTaskByCalleeIPAndPort() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
    }
    else
    {
        i = StopAllServiceTaskByCalleeIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        printf("DevUnReg() StopAllServiceTaskByCalleeIPAndPort() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
    }

    if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
    {
        /* 通知客户端，逻辑设备智能分析掉线 */
        i = SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser(pGBDeviceInfo->id);
        printf("DevUnReg() SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
    }
    else
    {
        /* 通知客户端，逻辑设备掉线 */
        i = SendAllGBLogicDeviceStatusOffAlarmToAllClientUser(pGBDeviceInfo->id);
        printf("DevUnReg() SendAllGBLogicDeviceStatusOffAlarmToAllClientUser() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() SendAllGBLogicDeviceStatusOffAlarmToAllClientUser Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() SendAllGBLogicDeviceStatusOffAlarmToAllClientUser OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
    }

    if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
    {
        /* 发送智能分析设备状态消息给客户端 */
        i = SendAllIntelligentGBLogicDeviceStatusProc(pGBDeviceInfo->id, 0, pdboper);
        printf("DevUnReg() SendAllIntelligentGBLogicDeviceStatusProc() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() SendAllIntelligentGBLogicDeviceStatusProc Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() SendAllIntelligentGBLogicDeviceStatusProc OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
    }
    else
    {
        /* 发送设备状态消息给客户端 */
        i = SendAllGBLogicDeviceStatusProc(pGBDeviceInfo->id, 0, 1, pdboper);
        printf("DevUnReg() SendAllGBLogicDeviceStatusProc() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() SendAllGBLogicDeviceStatusProc Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() SendAllGBLogicDeviceStatusProc OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
    }

    if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
    {
        //修正逻辑设备的智能分析状态
        i = SetGBLogicDeviceIntelligentStatus(pGBDeviceInfo->id, INTELLIGENT_STATUS_NULL);
        printf("DevUnReg() SetGBLogicDeviceIntelligentStatus() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() SetGBLogicDeviceIntelligentStatus Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() SetGBLogicDeviceIntelligentStatus OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
    }
    else
    {
        //修正逻辑设备状态
        i = SetGBLogicDeviceStatus(pGBDeviceInfo->id, 0, pdboper);
        printf("DevUnReg() SetGBLogicDeviceStatus() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() SetGBLogicDeviceStatus Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() SetGBLogicDeviceStatus OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
        }
    }

    /* 回收设备业务线程 */
    if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
        || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
    {
        i = device_srv_proc_thread_recycle(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
        printf("DevUnReg() device_srv_proc_thread_recycle() Exit--- \r\n");

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevUnReg() device_srv_proc_thread_recycle Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() device_srv_proc_thread_recycle OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
        }
    }

    /* 如果是拓扑结构表里面的设备类型，那么需要更新拓扑设备表的状态，并上报给上级CMS */
    if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type
        || EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
        || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
    {
        /* 更新拓扑结构表状态 */
        i = UpdateTopologyPhyDeviceStatus2DB(pGBDeviceInfo->device_id, (char*)"0", pdboper);
        printf("DevUnReg() UpdateTopologyPhyDeviceStatus2DB() Exit--- \r\n");
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevUnReg() Exit---\r\n");
    printf("DevUnReg() Exit---\r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : DevReg
 功能描述  : 设备注册处理
 输入参数  : GBDevice_info_t* pGBDeviceInfo
             GBDevice_cfg_t& GBDevice_cfg
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int DevReg(GBDevice_info_t* pGBDeviceInfo, GBDevice_cfg_t& GBDevice_cfg, DBOper* pdboper)
{
    int i = 0;
    int device_tl_pos = 0;
    int reg_info_index = pGBDeviceInfo->reg_info_index;
    osip_authorization_t* pAuthorization = NULL;
    char* realm = NULL;
    int iAuth = 0;
    int iIsRefreshReg = 0; /* 是否是刷新注册 */
    char strRegisterDomain[128] = {0};
    char* call_id = NULL;
    char* server_ip = NULL;
    int server_port = 0;
    char* pcRegServerEthName = NULL;
    char strEthName[MAX_IP_LEN] = {0};
    char strDeviceType[16] = {0};
    char strStatus[16] = {0};
    char strErrorCode[32] = {0};

    if (NULL == pGBDeviceInfo || NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "DevReg() exit---: Param Error \r\n");
        return -1;
    }

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() Enter---\r\n");
    printf("DevReg() Enter---\r\n");

    if (is_need_auth())
    {
        /* 检查是否有认证信息 */
        /* get authorization*/
        pAuthorization = SIP_UASGetRegisterAuthorization(reg_info_index);

        if (pAuthorization  && pAuthorization->realm)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() Check Realm: Begin---\r\n");

            realm = osip_getcopy_unquoted_string(pAuthorization->realm);
            i = IsLocalAuthRealm(realm);

            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() Check Realm: End--- i=%d\r\n", i);

            if (0 == i)
            {
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_AUTH_REALM_NOT_LOCAL_ERROR);
                SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Auth Realm Is Not Local");
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Auth Realm Is Not Local:realm=%s \r\n", realm);
                SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"设备认证域不是本CMS,认证域=", realm);
                EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"The device authentication domain is not the CMS, the authentication domain=", realm);

                osip_free(realm);
                realm = NULL;
                return -1;
            }
            else
            {
                osip_free(realm);
                realm = NULL;

                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SIP Auth: Begin--- :register_account=%s, register_password=%s \r\n", (char*)GBDevice_cfg.register_account.c_str(), (char*)GBDevice_cfg.register_password.c_str());

                iAuth = SIP_Auth(pAuthorization, (char*)GBDevice_cfg.register_account.c_str(), (char*)GBDevice_cfg.register_password.c_str(), (char*)"REGISTER");

                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SIP Auth: End--- iAuth=%d\r\n", iAuth);

                if (0 == iAuth) /*认证失败*/
                {
                    pGBDeviceInfo->auth_count++;

                    if (pGBDeviceInfo->auth_count > 0)
                    {
                        pGBDeviceInfo->auth_count = 0;
                        memset(strErrorCode, 0, 32);
                        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_AUTH_FAILD_ERROR);
                        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                        //SIP_UASAnswerToRegister(reg_info_index, 403, NULL);
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: AUTH FAIL \r\n");
                        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"设备认证失败");
                        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Device authentication failed");
                        return -1;
                    }
                    else
                    {
                        snprintf(strRegisterDomain, 128, "%s.spvmn.cn", pGblconf->register_region);
                        SIP_UASAnswerToRegister4Auth(reg_info_index, strRegisterDomain);
                        DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "DevReg() exit---: NEED AUTH \r\n");
                        //SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"设备需要认证");
                        //EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Device need authentication");
                        return -1;
                    }
                }
                else
                {
                    /* 获取注册的服务器IP地址、端口号，确定是从哪个网注册的 */
                    server_ip = SIP_GetUASServerIP(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_id=%s, login_ip=%s, login_port=%d, server_ip=%s \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, server_ip);
                    printf("DevReg() device_id=%s, login_ip=%s, login_port=%d, server_ip=%s \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, server_ip);

                    if ((NULL == server_ip)
                        || (NULL != server_ip && 0 == sstrcmp(server_ip, local_cms_id_get()))
                        || (NULL != server_ip && 0 == sstrcmp(server_ip, pGblconf->register_region)))
                    {
                        memset(strEthName, 0, MAX_IP_LEN);
                        snprintf(strEthName, MAX_IP_LEN, "%s", pGblconf->default_eth_name);
                        
                        server_ip = local_ip_get(strEthName);
                        server_port = local_port_get(strEthName);
                        pcRegServerEthName = get_ip_eth_name(server_ip);
                    }
                    else
                    {
                        server_port = SIP_GetUASServerPort(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        pcRegServerEthName = get_ip_eth_name(server_ip);
                    }

                    printf("DevReg() device_id=%s, login_ip=%s, login_port=%d, server_port=%d, RegServerEthName=%s \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, server_port, pcRegServerEthName);

                    if (NULL == server_ip)
                    {
                        pGBDeviceInfo->auth_count = 0;
                        memset(strErrorCode, 0, 32);
                        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_IP_ERROR);
                        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server IP Error");
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register Server Info Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
                        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"获取设备注册的服务器IP地址信息失败");
                        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of server IP address for device registration failed");

                        return -1;
                    }

                    if (server_port <= 0)
                    {
                        pGBDeviceInfo->auth_count = 0;
                        memset(strErrorCode, 0, 32);
                        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_PORT_ERROR);
                        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server Port Error");
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register Server Info Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
                        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"获取设备注册的服务器IP地址信息失败");
                        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of server IP address for device registration failed");

                        return -1;
                    }

                    if (NULL == pcRegServerEthName)
                    {
                        pGBDeviceInfo->auth_count = 0;
                        memset(strErrorCode, 0, 32);
                        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_IP_ETHNAME_ERROR);
                        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server EthName Error");
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register Server Info Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
                        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"获取设备注册的服务器IP地址信息失败");
                        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of server IP address for device registration failed");

                        return -1;
                    }

                    /* 确定是否是刷新注册 */
                    call_id = SIP_GetUASCallID(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    if (NULL == call_id)
                    {
                        pGBDeviceInfo->auth_count = 0;
                        memset(strErrorCode, 0, 32);
                        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_CALLID_ERROR);
                        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register CallID Error");
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register CallID Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
                        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"获取设备注册的CallID字段信息失败");
                        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of call id for device registration failed");

                        return -1;
                    }

                    if ('\0' == pGBDeviceInfo->call_id[0])
                    {
                        osip_strncpy(pGBDeviceInfo->call_id, call_id, MAX_128CHAR_STRING_LEN);
                        iIsRefreshReg = 0;
                    }
                    else
                    {
                        if (0 != sstrcmp(pGBDeviceInfo->call_id, call_id))
                        {
                            memset(pGBDeviceInfo->call_id, 0, MAX_128CHAR_STRING_LEN + 4);
                            osip_strncpy(pGBDeviceInfo->call_id, call_id, MAX_128CHAR_STRING_LEN);
                            iIsRefreshReg = 0;
                        }
                        else
                        {
                            iIsRefreshReg = 1; /* 刷新注册 */
                        }
                    }

                    DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DevReg() device_id=%s, login_ip=%s, login_port=%d: old callid=%s, new callid=%s, IsRefreshReg=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->call_id, call_id, iIsRefreshReg);
                    printf("DevReg() device_id=%s, login_ip=%s, login_port=%d: old callid=%s, new callid=%s, IsRefreshReg=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->call_id, call_id, iIsRefreshReg);

                    /* 确定服务器IP地址 */
                    if (pGBDeviceInfo->strRegServerIP[0] == '\0')
                    {
                        osip_strncpy(pGBDeviceInfo->strRegServerIP, server_ip, MAX_IP_LEN);
                    }
                    else
                    {
                        if (0 != sstrcmp(pGBDeviceInfo->strRegServerIP, server_ip))
                        {
                            memset(pGBDeviceInfo->strRegServerIP, 0, MAX_IP_LEN);
                            osip_strncpy(pGBDeviceInfo->strRegServerIP, server_ip, MAX_IP_LEN);
                        }
                    }

                    /* 确定注册服务器端口号 */
                    if (pGBDeviceInfo->iRegServerPort <= 0)
                    {
                        pGBDeviceInfo->iRegServerPort = server_port;
                    }
                    else
                    {
                        if (pGBDeviceInfo->iRegServerPort != server_port)
                        {
                            pGBDeviceInfo->iRegServerPort = server_port;
                        }
                    }

                    /* 确定IP地址网口名称 */
                    if (pGBDeviceInfo->strRegServerEthName[0] == '\0')
                    {
                        osip_strncpy(pGBDeviceInfo->strRegServerEthName, pcRegServerEthName, MAX_IP_LEN);
                    }
                    else
                    {
                        if (0 != sstrcmp(pGBDeviceInfo->strRegServerEthName, pcRegServerEthName))
                        {
                            memset(pGBDeviceInfo->strRegServerEthName, 0, MAX_IP_LEN);
                            osip_strncpy(pGBDeviceInfo->strRegServerEthName, pcRegServerEthName, MAX_IP_LEN);
                        }
                    }

                    pGBDeviceInfo->auth_count = 0;
                    SIP_UASAnswerToRegister(reg_info_index, 200, NULL);

                    if (pGBDeviceInfo->reg_status <= 0)
                    {
                        /* 媒体网关和下级平台，分配单独的处理线程 */
                        if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
                            || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_MGWSERVER
                            || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_VIDEODIAGNOSIS
                            || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_INTELLIGENTANALYSIS
                            || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_DECODER)
                        {
                            /* 创建设备业务处理线程 */
                            device_tl_pos = device_srv_proc_thread_find(pGBDeviceInfo);
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_srv_proc_thread_find:device_tl_pos=%d \r\n", device_tl_pos);
                            printf("DevReg() device_srv_proc_thread_find:device_tl_pos=%d \r\n", device_tl_pos);

                            if (device_tl_pos < 0)
                            {
                                //分配处理线程
                                i = device_srv_proc_thread_assign(pGBDeviceInfo);
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);
                                printf("DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);

                                if (i != 0)
                                {
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "分配设备业务处理线程失败:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                }
                            }
                            else
                            {
                                /* 释放一下之前的用户业务处理线程 */
                                i = device_srv_proc_thread_recycle(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                                //分配处理线程
                                i = device_srv_proc_thread_assign(pGBDeviceInfo);
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);
                                printf("DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);

                                if (i != 0)
                                {
                                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "分配设备业务处理线程失败:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                                }
                            }
                        }

                        /*更新内存*/
                        pGBDeviceInfo->reg_status = 1;
                        pGBDeviceInfo->catalog_subscribe_flag = 0;                                       /* 目录订阅标识:0:没有订阅，1:已订阅 */
                        pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get(); /* 目录订阅未成功的重试间隔时间 */
                        pGBDeviceInfo->catalog_subscribe_expires = 0;                                    /* 目录订阅超时时间*/
                        pGBDeviceInfo->catalog_subscribe_expires_count = 0;                              /* 目录订阅时间计数 */
                        pGBDeviceInfo->catalog_subscribe_event_id = 0;                                   /* 目录订阅事件计数 */
                        pGBDeviceInfo->last_keep_alive_time = 0;
                        pGBDeviceInfo->keep_alive_expires = 0;
                        pGBDeviceInfo->keep_alive_expires_count = 0;
                        pGBDeviceInfo->keep_alive_count = 0;
                        pGBDeviceInfo->iGetCataLogStatus = 0;
                        pGBDeviceInfo->iGetLogicDeviceStatusCount = 0;
                        pGBDeviceInfo->iLastGetCataLogTime = 0;

                        UpdateGBDeviceRegStatus2DB(pGBDeviceInfo, pdboper);

                        /* 更新逻辑设备表信息,可能逻辑设备道前端已经删除，状态还是通过获取或者上报的到 */
                        if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_IPC
                            || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_DECODER
                            || (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER && pGBDeviceInfo->three_party_flag == 1))
                        {
                            i = SetGBLogicDeviceStatus(pGBDeviceInfo->id, 1, pdboper);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SetGBLogicDeviceStatus Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SetGBLogicDeviceStatus OK:i=%d \r\n", i);
                            }

                            /* 发送设备状态消息给客户端 */
                            i = SendAllGBLogicDeviceStatusProc(pGBDeviceInfo->id, 1, 1, pdboper);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendAllGBLogicDeviceStatusProc Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendAllGBLogicDeviceStatusProc OK:i=%d \r\n", i);
                            }
                        }

                        /* 发送获取设备状态消息 */
                        //i = SendQueryDeviceStatusMessage(pGBDeviceInfo);

                        /* 发送获取设备信息消息 */
                        i = SendQueryDeviceInfoMessage(pGBDeviceInfo);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceInfoMessage Error:i=%d \r\n", i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceInfoMessage OK:i=%d \r\n", i);
                        }

                        if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER)  /* 平台 */
                        {
                            /* 获取下级CMS 数据库IP 地址 */
                            i = SendQuerySubCMSDBIPMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQuerySubCMSDBIPMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQuerySubCMSDBIPMessage OK:i=%d \r\n", i);
                            }

                            /* 发送本级CMS重启的命令给下级CMS */
                            i = SendNotifyRestartMessageToSubCMS(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendNotifyRestartMessageToSubCMS Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendNotifyRestartMessageToSubCMS OK:i=%d \r\n", i);
                            }

                            /* 推送点位到下级平台 */
                            i = SendNotifyCatalogMessageToSubCMS(pGBDeviceInfo->device_id);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendNotifyCatalogMessageToSubCMS Error:device_id=%s, device_ip=%s, device_port=%d, iRet=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendNotifyCatalogMessageToSubCMS OK:device_id=%s, device_ip=%s, device_port=%d, iRet=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                            }

                            /* 获取下级CMS 拓扑物理设备配置表 */
                            i = SendQuerySubCMSTopologyPhyDeviceConfigMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQuerySubCMSTopologyPhyDeviceConfigMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQuerySubCMSTopologyPhyDeviceConfigMessage OK:i=%d \r\n", i);
                            }

                            /* 发送查询设备目录信息的消息 */
                            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                            }

                            /*如果是下级CMS, 并且不是同级互联，则还需获取逻辑设备分组信息和逻辑设备分组关系信息 */
                            if (pGBDeviceInfo->link_type == 0)
                            {
                                i = SendQueryDeviceGroupInfoMessage(pGBDeviceInfo);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceGroupInfoMessage Error:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceGroupInfoMessage OK:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                                }

                                i = SendQueryDeviceGroupMapInfoMessage(pGBDeviceInfo);

                                if (0 != i)
                                {
                                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceGroupMapInfoMessage Error:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                                }
                                else
                                {
                                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceGroupMapInfoMessage OK:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                                }
                            }
                        }

                        /* 如果是拓扑结构表里面的设备类型，那么需要添加到拓扑结构表信息，并上报给上级CMS */
                        if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type
                            || EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
                            || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
                        {
                            /* 添加拓扑结构表信息 */
                            snprintf(strDeviceType, 16, "%d", pGBDeviceInfo->device_type);
                            snprintf(strStatus, 16, "%d", pGBDeviceInfo->reg_status);
                            i = AddTopologyPhyDeviceInfo2DB(pGBDeviceInfo->device_id, (char*)"", strDeviceType, pGBDeviceInfo->login_ip, strStatus, local_cms_id_get(), (char*)"1", pdboper);
                        }

                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "设备上线:设备ID=%s, IP地址=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device online:device ID=%s, IP address=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);

                        DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DevReg() ADD:device_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->reg_info_index);
                    }
                    else
                    {
                        if (iIsRefreshReg)
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "设备注册刷新:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Equipment registered refresh:ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                        else
                        {
                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "设备注册的CallID发生变化, 可能是设备掉线后重新注册上线:设备ID=%s, IP地址=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Device re online:device ID=%s, IP address=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);

                            /* 发送获取设备信息消息 */
                            i = SendQueryDeviceInfoMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceInfoMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceInfoMessage OK:i=%d \r\n", i);
                            }
                        }
                    }

                    /* 获取Catalog */
                    if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
                        && pGBDeviceInfo->three_party_flag == 1) /* 第三方平台 */
                    {
                        /* 如果没有获取到逻辑通道，则重新发送获取信息 */
                        if (iIsRefreshReg && 0 == pGBDeviceInfo->iGetCataLogStatus)
                        {
                            /* 发送查询设备目录信息的消息 */
                            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                            }

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "没有检索到下级第三发平台设备Catalog, 再次发送查询Catalog消息:下级第三发平台设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }
                    else if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
                             && pGBDeviceInfo->three_party_flag == 0) /* 自己的平台 */
                    {
                        /* 如果没有获取到逻辑通道，则重新发送获取信息 */
                        if (iIsRefreshReg && 0 == pGBDeviceInfo->iGetCataLogStatus)
                        {
                            /* 发送查询设备目录信息的消息 */
                            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                            }

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "没有检索到下级CMS设备Catalog, 再次发送查询Catalog消息:下级CMS ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }
                    else if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type) /* 解码器 */
                    {
                        /* 如果是新上线或者没有获取到逻辑通道，则重新发送获取信息 */
                        if (!iIsRefreshReg || 0 == pGBDeviceInfo->iGetCataLogStatus)
                        {
                            /* 发送查询设备目录信息的消息 */
                            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                            }

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "没有检索到前端解码器设备的Catalog, 再次发送查询Catalog消息:前端解码器设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }
                    else if (EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type) /* 接入网关 */
                    {
                        /* 如果是新上线或者没有获取到逻辑通道，则重新发送获取信息 */
                        if (!iIsRefreshReg || 0 == pGBDeviceInfo->iGetCataLogStatus)
                        {
                            /* 发送查询设备目录信息的消息 */
                            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                            }

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "没有检索到前端媒体网关设备的Catalog, 再次发送查询Catalog消息:前端媒体网关设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }
                    else if (EV9000_DEVICETYPE_ALARMSERVER == pGBDeviceInfo->device_type) /* 报警服务器 */
                    {
                        /* 如果是新上线或者没有获取到逻辑通道，则重新发送获取信息 */
                        if (!iIsRefreshReg || 0 == pGBDeviceInfo->iGetCataLogStatus)
                        {
                            /* 发送查询设备目录信息的消息 */
                            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                            }

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "没有检索到前端报警服务器的Catalog, 再次发送查询Catalog消息:前端报警服务器ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }
                    else if (EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type
                             && EV9000_DEVICETYPE_VIDEODIAGNOSIS != pGBDeviceInfo->device_type
                             && EV9000_DEVICETYPE_DECODER != pGBDeviceInfo->device_type
                             && EV9000_DEVICETYPE_MGWSERVER != pGBDeviceInfo->device_type
                             && EV9000_DEVICETYPE_ALARMSERVER != pGBDeviceInfo->device_type) /* 其他设备 */
                    {
                        /* 如果是新上线或者没有获取到逻辑通道，则重新发送获取信息 */
                        if (!iIsRefreshReg || 0 == pGBDeviceInfo->iGetCataLogStatus)
                        {
                            /* 发送查询设备目录信息的消息 */
                            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                            }

                            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "没有检索到前端设备的Catalog, 再次发送查询Catalog消息:前端设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                        }
                    }

                    printf("DevReg() Exit---\r\n");
                    return 0;
                }
            }
        }
        else
        {
            pGBDeviceInfo->auth_count++;

            if (pGBDeviceInfo->auth_count > 3)
            {
                pGBDeviceInfo->auth_count = 0;
                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_AUTH_FAILD_ERROR);
                SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                //SIP_UASAnswerToRegister(reg_info_index, 403, NULL);
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() exit---: AUTH FAIL \r\n");
                SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"设备认证失败");
                EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Device authentication failed");
                return -1;
            }
            else
            {
                snprintf(strRegisterDomain, 128, "%s.spvmn.cn", pGblconf->register_region);
                SIP_UASAnswerToRegister4Auth(reg_info_index, strRegisterDomain);
                DEBUG_TRACE(MODULE_DEVICE, LOG_WARN, "DevReg() exit---: NEED AUTH \r\n");
                //SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"设备需要认证");
                //EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Device need authentication");
                return -1;
            }
        }
    }
    else
    {
        /* 获取注册的服务器IP地址、端口号，确定是从哪个网注册的 */
        server_ip = SIP_GetUASServerIP(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_id=%s, login_ip=%s, login_port=%d, server_ip=%s \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, server_ip);
        printf("DevReg() device_id=%s, login_ip=%s, login_port=%d, server_ip=%s \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, server_ip);

        if ((NULL == server_ip)
            || (NULL != server_ip && 0 == sstrcmp(server_ip, local_cms_id_get()))
            || (NULL != server_ip && 0 == sstrcmp(server_ip, pGblconf->register_region)))
        {
            memset(strEthName, 0, MAX_IP_LEN);
            snprintf(strEthName, MAX_IP_LEN, "%s", pGblconf->default_eth_name);

            server_ip = local_ip_get(strEthName);
            server_port = local_port_get(strEthName);
            pcRegServerEthName = get_ip_eth_name(server_ip);
        }
        else
        {
            server_port = SIP_GetUASServerPort(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            pcRegServerEthName = get_ip_eth_name(server_ip);
        }

        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_id=%s, login_ip=%s, login_port=%d, server_port=%d, RegServerEthName=%s \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, server_port, pcRegServerEthName);
        printf("DevReg() device_id=%s, login_ip=%s, login_port=%d, server_port=%d, RegServerEthName=%s \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, server_port, pcRegServerEthName);

        if (NULL == server_ip)
        {
            pGBDeviceInfo->auth_count = 0;
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_IP_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server IP Error");
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register Server Info Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"获取设备注册的服务器IP地址信息失败");
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of server IP address for device registration failed");

            return -1;
        }

        if (server_port <= 0)
        {
            pGBDeviceInfo->auth_count = 0;
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_PORT_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server Port Error");
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register Server Info Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"获取设备注册的服务器IP地址信息失败");
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of server IP address for device registration failed");

            return -1;
        }

        if (NULL == pcRegServerEthName)
        {
            pGBDeviceInfo->auth_count = 0;
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_IP_ETHNAME_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server EthName Error");
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register Server Info Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"获取设备注册的服务器IP地址信息失败");
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of server IP address for device registration failed");

            return -1;
        }

        /* 确定是否是刷新注册 */
        call_id = SIP_GetUASCallID(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

        if (NULL == call_id)
        {
            pGBDeviceInfo->auth_count = 0;
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_CALLID_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register CallID Error");
            DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() exit---: Get Register CallID Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"获取设备注册的CallID字段信息失败");
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Access information of call id for device registration failed");

            return -1;
        }

        if ('\0' == pGBDeviceInfo->call_id[0])
        {
            osip_strncpy(pGBDeviceInfo->call_id, call_id, MAX_128CHAR_STRING_LEN);
            iIsRefreshReg = 0;
        }
        else
        {
            if (0 != sstrcmp(pGBDeviceInfo->call_id, call_id))
            {
                memset(pGBDeviceInfo->call_id, 0, MAX_128CHAR_STRING_LEN + 4);
                osip_strncpy(pGBDeviceInfo->call_id, call_id, MAX_128CHAR_STRING_LEN);
                iIsRefreshReg = 0;
            }
            else
            {
                iIsRefreshReg = 1; /* 刷新注册 */
            }
        }

        DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DevReg() device_id=%s, login_ip=%s, login_port=%d: old callid=%s, new callid=%s, IsRefreshReg=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->call_id, call_id, iIsRefreshReg);
        printf("DevReg() device_id=%s, login_ip=%s, login_port=%d: old callid=%s, new callid=%s, IsRefreshReg=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->call_id, call_id, iIsRefreshReg);

        /* 确定服务器IP地址 */
        if (pGBDeviceInfo->strRegServerIP[0] == '\0')
        {
            osip_strncpy(pGBDeviceInfo->strRegServerIP, server_ip, MAX_IP_LEN);
        }
        else
        {
            if (0 != sstrcmp(pGBDeviceInfo->strRegServerIP, server_ip))
            {
                memset(pGBDeviceInfo->strRegServerIP, 0, MAX_IP_LEN);
                osip_strncpy(pGBDeviceInfo->strRegServerIP, server_ip, MAX_IP_LEN);
            }
        }

        /* 确定注册服务器端口号 */
        if (pGBDeviceInfo->iRegServerPort <= 0)
        {
            pGBDeviceInfo->iRegServerPort = server_port;
        }
        else
        {
            if (pGBDeviceInfo->iRegServerPort != server_port)
            {
                pGBDeviceInfo->iRegServerPort = server_port;
            }
        }

        /* 确定IP地址网口名称 */
        if (pGBDeviceInfo->strRegServerEthName[0] == '\0')
        {
            osip_strncpy(pGBDeviceInfo->strRegServerEthName, pcRegServerEthName, MAX_IP_LEN);
        }
        else
        {
            if (0 != sstrcmp(pGBDeviceInfo->strRegServerEthName, pcRegServerEthName))
            {
                memset(pGBDeviceInfo->strRegServerEthName, 0, MAX_IP_LEN);
                osip_strncpy(pGBDeviceInfo->strRegServerEthName, pcRegServerEthName, MAX_IP_LEN);
            }
        }

        pGBDeviceInfo->auth_count = 0;
        SIP_UASAnswerToRegister(reg_info_index, 200, NULL);

        if (pGBDeviceInfo->reg_status <= 0)
        {
            /* 媒体网关和下级平台，分配单独的处理线程 */
            if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
                || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_MGWSERVER
                || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_VIDEODIAGNOSIS
                || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_INTELLIGENTANALYSIS
                || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_DECODER)
            {
                /* 创建设备业务处理线程 */
                device_tl_pos = device_srv_proc_thread_find(pGBDeviceInfo);
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_srv_proc_thread_find:device_tl_pos=%d \r\n", device_tl_pos);
                printf("DevReg() device_srv_proc_thread_find:device_tl_pos=%d \r\n", device_tl_pos);

                if (device_tl_pos < 0)
                {
                    //分配处理线程
                    i = device_srv_proc_thread_assign(pGBDeviceInfo);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);
                    printf("DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "分配设备业务处理线程失败:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }
                }
                else
                {
                    /* 释放一下之前的用户业务处理线程 */
                    i = device_srv_proc_thread_recycle(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                    //分配处理线程
                    i = device_srv_proc_thread_assign(pGBDeviceInfo);
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);
                    printf("DevReg() device_srv_proc_thread_assign:i=%d \r\n", i);

                    if (i != 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "分配设备业务处理线程失败:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                    }
                }
            }

            /*更新内存*/
            pGBDeviceInfo->reg_status = 1;
            pGBDeviceInfo->catalog_subscribe_flag = 0;                                       /* 目录订阅标识:0:没有订阅，1:已订阅 */
            pGBDeviceInfo->catalog_subscribe_interval = local_register_retry_interval_get(); /* 目录订阅未成功的重试间隔时间 */
            pGBDeviceInfo->catalog_subscribe_expires = 0;                                    /* 目录订阅超时时间*/
            pGBDeviceInfo->catalog_subscribe_expires_count = 0;                              /* 目录订阅时间计数 */
            pGBDeviceInfo->catalog_subscribe_event_id = 0;                                   /* 目录订阅事件计数 */
            pGBDeviceInfo->last_keep_alive_time = 0;
            pGBDeviceInfo->keep_alive_expires = 0;
            pGBDeviceInfo->keep_alive_expires_count = 0;
            pGBDeviceInfo->keep_alive_count = 0;
            pGBDeviceInfo->iGetCataLogStatus = 0;
            pGBDeviceInfo->iGetLogicDeviceStatusCount = 0;
            pGBDeviceInfo->iLastGetCataLogTime = 0;

            UpdateGBDeviceRegStatus2DB(pGBDeviceInfo, pdboper);

            /* 更新逻辑设备表信息,可能逻辑设备道前端已经删除，状态还是通过获取或者上报的到 */
            if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_IPC
                || pGBDeviceInfo->device_type == EV9000_DEVICETYPE_DECODER
                || (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER && pGBDeviceInfo->three_party_flag == 1))
            {
                i = SetGBLogicDeviceStatus(pGBDeviceInfo->id, 1, pdboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SetGBLogicDeviceStatus Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SetGBLogicDeviceStatus OK:i=%d \r\n", i);
                }

                /* 发送设备状态消息给客户端 */
                i = SendAllGBLogicDeviceStatusProc(pGBDeviceInfo->id, 1, 1, pdboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendAllGBLogicDeviceStatusProc Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendAllGBLogicDeviceStatusProc OK:i=%d \r\n", i);
                }
            }

            /* 发送获取设备状态消息 */
            //i = SendQueryDeviceStatusMessage(pGBDeviceInfo);

            /* 发送获取设备信息消息 */
            i = SendQueryDeviceInfoMessage(pGBDeviceInfo);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceInfoMessage Error:i=%d \r\n", i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceInfoMessage OK:i=%d \r\n", i);
            }

            if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER) /* 平台 */
            {
                /* 获取下级CMS 数据库IP 地址 */
                i = SendQuerySubCMSDBIPMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQuerySubCMSDBIPMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQuerySubCMSDBIPMessage OK:i=%d \r\n", i);
                }

                /* 发送本级CMS重启的命令给下级CMS */
                i = SendNotifyRestartMessageToSubCMS(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendNotifyRestartMessageToSubCMS Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendNotifyRestartMessageToSubCMS OK:i=%d \r\n", i);
                }

                /* 推送点位到下级平台 */
                i = SendNotifyCatalogMessageToSubCMS(pGBDeviceInfo->device_id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendNotifyCatalogMessageToSubCMS Error:device_id=%s, device_ip=%s, device_port=%d, iRet=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendNotifyCatalogMessageToSubCMS OK:device_id=%s, device_ip=%s, device_port=%d, iRet=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }

                /* 获取下级CMS 拓扑物理设备配置表 */
                i = SendQuerySubCMSTopologyPhyDeviceConfigMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQuerySubCMSTopologyPhyDeviceConfigMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQuerySubCMSTopologyPhyDeviceConfigMessage OK:i=%d \r\n", i);
                }

                /* 发送查询设备目录信息的消息 */
                i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                }

                /*如果是下级CMS, 并且不是同级互联，则还需获取逻辑设备分组信息和逻辑设备分组关系信息 */
                if (pGBDeviceInfo->link_type == 0)
                {
                    i = SendQueryDeviceGroupInfoMessage(pGBDeviceInfo);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceGroupInfoMessage Error:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceGroupInfoMessage OK:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                    }

                    i = SendQueryDeviceGroupMapInfoMessage(pGBDeviceInfo);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceGroupMapInfoMessage Error:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceGroupMapInfoMessage OK:DeviceID=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                    }
                }
            }

            /* 如果是拓扑结构表里面的设备类型，那么需要添加到拓扑结构表信息，并上报给上级CMS */
            if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type
                || EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
                || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
            {
                /* 添加拓扑结构表信息 */
                snprintf(strDeviceType, 16, "%d", pGBDeviceInfo->device_type);
                snprintf(strStatus, 16, "%d", pGBDeviceInfo->reg_status);
                i = AddTopologyPhyDeviceInfo2DB(pGBDeviceInfo->device_id, (char*)"", strDeviceType, pGBDeviceInfo->login_ip, strStatus, local_cms_id_get(), (char*)"1", pdboper);
            }

            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "设备上线:设备ID=%s, IP地址=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "Device online:device ID=%s, IP address=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);

            DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "DevReg() ADD:device_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->reg_info_index);
            printf("DevReg() ADD:device_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, pGBDeviceInfo->reg_info_index);
        }
        else
        {
            if (iIsRefreshReg)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "设备注册刷新:设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Equipment registered refresh:ID=%s, IP=%s, port=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
            else
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "设备注册的CallID发生变化, 可能是设备掉线后重新注册上线:设备ID=%s, IP地址=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Device re online:device ID=%s, IP address=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip);

                /* 发送获取设备信息消息 */
                i = SendQueryDeviceInfoMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceInfoMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceInfoMessage OK:i=%d \r\n", i);
                }
            }
        }

        /* 获取Catalog */
        if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
            && pGBDeviceInfo->three_party_flag == 1) /* 第三方平台 */
        {
            /* 如果没有获取到逻辑通道，则重新发送获取信息 */
            if (iIsRefreshReg && 0 == pGBDeviceInfo->iGetCataLogStatus)
            {
                /* 发送查询设备目录信息的消息 */
                i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                }

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "没有检索到下级第三发平台设备Catalog, 再次发送查询Catalog消息:下级第三发平台设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER
                 && pGBDeviceInfo->three_party_flag == 0) /* 自己的平台 */
        {
            /* 如果没有获取到逻辑通道，则重新发送获取信息 */
            if (iIsRefreshReg && 0 == pGBDeviceInfo->iGetCataLogStatus)
            {
                /* 发送查询设备目录信息的消息 */
                i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                }

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "没有检索到下级CMS设备Catalog, 再次发送查询Catalog消息:下级CMS ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type) /* 解码器 */
        {
            /* 如果是新上线或者没有获取到逻辑通道，则重新发送获取信息 */
            if (!iIsRefreshReg || 0 == pGBDeviceInfo->iGetCataLogStatus)
            {
                /* 发送查询设备目录信息的消息 */
                i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                }

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "没有检索到前端解码器设备的Catalog, 再次发送查询Catalog消息:前端解码器设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else if (EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type) /* 接入网关 */
        {
            /* 如果是新上线或者没有获取到逻辑通道，则重新发送获取信息 */
            if (!iIsRefreshReg || 0 == pGBDeviceInfo->iGetCataLogStatus)
            {
                /* 发送查询设备目录信息的消息 */
                i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                }

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "没有检索到前端媒体网关设备的Catalog, 再次发送查询Catalog消息:前端媒体网关设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else if (EV9000_DEVICETYPE_ALARMSERVER == pGBDeviceInfo->device_type) /* 报警服务器 */
        {
            /* 如果是新上线或者没有获取到逻辑通道，则重新发送获取信息 */
            if (!iIsRefreshReg || 0 == pGBDeviceInfo->iGetCataLogStatus)
            {
                /* 发送查询设备目录信息的消息 */
                i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                }

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "没有检索到前端报警服务器的Catalog, 再次发送查询Catalog消息:前端媒体网关设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else if (EV9000_DEVICETYPE_CAMERA == pGBDeviceInfo->device_type
                 || EV9000_DEVICETYPE_IPC == pGBDeviceInfo->device_type) /* IPC */
        {
            /* 如果是新上线，则重新发送获取信息 */
            if (!iIsRefreshReg)
            {
                /* 发送查询设备目录信息的消息 */
                i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
                }

                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "没有检索到前端媒体网关设备的Catalog, 再次发送查询Catalog消息:前端媒体网关设备ID=%s, IP地址=%s, 端口号=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_WARNING, "No Catalog response message from front device, send query catalog message again:front-end device ID=%s, IP address=%s, Port number=%d", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);
            }
        }
        else if (EV9000_DEVICETYPE_SIPSERVER != pGBDeviceInfo->device_type
                 && EV9000_DEVICETYPE_VIDEODIAGNOSIS != pGBDeviceInfo->device_type
                 && EV9000_DEVICETYPE_DECODER != pGBDeviceInfo->device_type
                 && EV9000_DEVICETYPE_MGWSERVER != pGBDeviceInfo->device_type
                 && EV9000_DEVICETYPE_ALARMSERVER != pGBDeviceInfo->device_type
                 && EV9000_DEVICETYPE_CAMERA != pGBDeviceInfo->device_type
                 && EV9000_DEVICETYPE_IPC != pGBDeviceInfo->device_type) /* 其他设备 */
        {
            /* 发送查询设备目录信息的消息 */
            i = SendQueryDeviceCatalogMessage(pGBDeviceInfo);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() SendQueryDeviceCatalogMessage Error:i=%d \r\n", i);
            }
            else
            {
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "DevReg() SendQueryDeviceCatalogMessage OK:i=%d \r\n", i);
            }
        }

        printf("DevReg() Exit---\r\n");
        return 0;
    }

    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SYSTEM_ERROR);
    SIP_UASAnswerToRegister(reg_info_index, 500, strErrorCode);
    //SIP_UASAnswerToRegister(reg_info_index, 500, (char*)"Server Error");
    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "DevReg() exit---: Forbidden \r\n");
    SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"未知原因");
    EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, (char*)"Unknown reason");

    return -1;
}

/*****************************************************************************
 函 数 名  : GBDevice_reg_msg_proc
 功能描述  : 设备注册消息处理
 输入参数  : GBDevice_reg_msg_t* pMsg
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GBDevice_reg_msg_proc(GBDevice_reg_msg_t* pMsg, DBOper* pdboper)
{
    char strErrorCode[32] = {0};

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    printf("\r\nGBDevice_reg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "设备注册处理:设备ID=%s, 设备IP地址=%s, 端口号=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device registration processing:ID=%s, IP=%s, port=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    /*验证参数*/
    if (!pMsg || pMsg->reg_info_index < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_reg_msg_proc() exit---: pMsg/ reg_info_indext Error \r\n");
        return -1;
    }

    int reg_info_index = pMsg->reg_info_index;

    if (!(pMsg->login_ip) || !(pMsg->register_id) | (pMsg->login_port <= 0))
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_MSG_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"register_id/ IP /port Error");
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: register_id/ IP /port  Error \r\n");
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, 设备IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"设备ID/设备IP地/设备端口号不合法");
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"device IP device ID/ the port number is not valid");

        return -1;
    }

    int i = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    int iExpires = pMsg->expires;
    bool bIsNewDev = false;   //新注册设备
    GBDevice_cfg_t GBDevice_cfg;

    if (!is_need_auth()) /* 如果不需要认证，则不用登记，直接入库 */
    {
        /* 获取物理设备信息 */
        if (GetDevCfg(pdboper, pMsg->register_id, GBDevice_cfg) <= 0) //获取信息失败
        {

#if 0

            if (iExpires > 0 && checkNumberOfGBDeviceInfo() > g_RegistrationLimit)
            {
                SIP_UASAnswerToRegister(reg_info_index, 403, "Number of Registration Limited");
                DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() exit---: Number of Registration Limited \r\n");
                SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"设备注册数达到上限");
                return -1;
            }

#endif
            /* 将物理设备信息直接写入物理设备表 */
            i = WriteGBPhyDeviceInfoToDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip, pMsg->link_type, pdboper);

            /* 再次获取物理设备信息 */
            if (GetDevCfg(pdboper, pMsg->register_id, GBDevice_cfg) <= 0) //获取信息失败
            {
                i = RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip, pMsg->link_type, pdboper);

                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_GET_DEVICE_INFO_ERROR);
                SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                //SIP_UASAnswerToRegister(reg_info_index, 404, (char*)"Find GBDevice Info Error");
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: Find GBDevice Info Error \r\n");
                SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, 设备IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"数据库中没有找到该设备");
                EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"The device was not found in the database.");

                return -1;
            }
        }
        else
        {
            /* 如果本地CMS ID不符合，则更新一下数据库 */
            if (0 != sstrcmp(GBDevice_cfg.cms_id.c_str(), local_cms_id_get()))
            {
                UpdateGBDeviceCMSID2DB(pMsg->register_id, pdboper);
            }
        }
    }
    else if (IsLocalAuthRealm(pMsg->login_ip)) /* 如果是和CMS同一个IP上来的设备，则不用登记，直接入库 */
    {
        if (GetDevCfg(pdboper, pMsg->register_id, GBDevice_cfg) <= 0) //获取信息失败
        {
            /* 将物理设备信息直接写入物理设备表 */
            i = WriteGBPhyDeviceInfoToDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip, pMsg->link_type, pdboper);

            /* 再次获取物理设备信息 */
            if (GetDevCfg(pdboper, pMsg->register_id, GBDevice_cfg) <= 0) //获取信息失败
            {
                i = RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip, pMsg->link_type, pdboper);

                memset(strErrorCode, 0, 32);
                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_GET_DEVICE_INFO_ERROR);
                SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                //SIP_UASAnswerToRegister(reg_info_index, 404, (char*)"Find GBDevice Info Error");
                DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: Find GBDevice Info Error \r\n");
                SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, 设备IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"数据库中没有找到该设备");
                EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"The device was not found in the database.");

                return -1;
            }
        }
        else/* 如果不是和CMS同一个IP上来的设备，则需要先登记，再入库 */
        {
            /* 如果本地CMS ID不符合，则更新一下数据库 */
            if (0 != sstrcmp(GBDevice_cfg.cms_id.c_str(), local_cms_id_get()))
            {
                UpdateGBDeviceCMSID2DB(pMsg->register_id, pdboper);
            }
        }
    }
    else
    {
        if (GetDevCfg(pdboper, pMsg->register_id, GBDevice_cfg) <= 0) //获取信息失败
        {
            i = RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip, pMsg->link_type, pdboper);

            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_GET_DEVICE_INFO_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 404, (char*)"Find GBDevice Info Error");
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: Find GBDevice Info Error \r\n");
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册, 请先登记设备:设备ID=%s, 设备IP地址=%s, 端口号=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration, please register device first:device ID=%s, device IP address =%s, port number=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

            return -1;
        }
        else
        {
            /* 如果本地CMS ID不符合，则更新一下数据库 */
            if (0 != sstrcmp(GBDevice_cfg.cms_id.c_str(), local_cms_id_get()))
            {
                UpdateGBDeviceCMSID2DB(pMsg->register_id, pdboper);
            }
        }
    }

    /* 检查设备是否启用 */
    if (0 == GBDevice_cfg.enable)/*设备没有启用*/
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_NOT_ENABLE_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"GBDevice Not Enable");

        /* 写入临时库 */
        i = RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip, pMsg->link_type, pdboper);

        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备未启用:设备ID=%s, IP地址=%s", pMsg->register_id, pMsg->login_ip);
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "The equipment is not enabled:ID=%s, IP=%s", pMsg->register_id, pMsg->login_ip);

        //DEBUG_TRACE(MODULE_DEVICE, LOG_INFO, "GBDevice_reg_msg_proc() StopAllServiceTaskByGBDeviceIndex() Enter------------: DeviceID=%s \r\n", pGBDeviceInfo->device_id);

        pGBDeviceInfo = GBDevice_info_find(pMsg->register_id);

        if (NULL != pGBDeviceInfo)
        {
            /* 如果是拓扑结构表里面的设备类型，那么需要删除拓扑结构表信息，并上报给上级CMS */
            if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type
                || EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
                || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
            {
                /* 删除拓扑结构表信息 */
                i = DeleteTopologyPhyDeviceInfoFromDB(pGBDeviceInfo->device_id, pdboper);
            }

            /* 查找点位的业务，并停止所有业务 */
            if (EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type) /* 解码器根据主叫侧信息停止业务 */
            {
                i = StopAllServiceTaskByCallerIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() StopAllServiceTaskByCallerIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() StopAllServiceTaskByCallerIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
            }
            else if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type) /* IVS根据主叫、被叫侧信息停止业务 */
            {
                i = StopAllServiceTaskByCallerIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port); /* 主动请求的视频流 */

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() StopAllServiceTaskByCallerIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() StopAllServiceTaskByCallerIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }

                i = StopAllServiceTaskByCalleeIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port); /* 被动发送的智能分析频流 */

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
            }
            else if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type) /* 下级CMS */
            {
                i = StopAllServiceTaskByCalleeIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
            }
            else
            {
                i = StopAllServiceTaskByCalleeIPAndPort(pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() StopAllServiceTaskByCalleeIPAndPort Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() StopAllServiceTaskByCalleeIPAndPort OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
            }

            if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
            {
                /* 通知客户端，逻辑设备智能分析掉线 */
                i = SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser(pGBDeviceInfo->id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() SendAllGBLogicDeviceIntelligentStatusOffAlarmToAllClientUser OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
            }
            else
            {
                /* 通知客户端，逻辑设备掉线 */
                i = SendAllGBLogicDeviceStatusOffAlarmToAllClientUser(pGBDeviceInfo->id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() SendAllGBLogicDeviceStatusOffAlarmToAllClientUser Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() SendAllGBLogicDeviceStatusOffAlarmToAllClientUser OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
            }

            if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
            {
                /* 发送智能分析设备状态消息给客户端 */
                i = SendAllIntelligentGBLogicDeviceStatusProc(pGBDeviceInfo->id, 0, pdboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() SendAllIntelligentGBLogicDeviceStatusProc Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() SendAllIntelligentGBLogicDeviceStatusProc OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
            }
            else
            {
                /* 发送设备状态消息给客户端 */
                i = SendAllGBLogicDeviceStatusProc(pGBDeviceInfo->id, 0, 0, pdboper);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() SendAllGBLogicDeviceStatusProc Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() SendAllGBLogicDeviceStatusProc OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
            }

            /* 回收设备业务线程 */
            if (EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type
                || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
            {
                i = device_srv_proc_thread_recycle(pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc device_srv_proc_thread_recycle Error:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() device_srv_proc_thread_recycle OK:device_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pGBDeviceInfo->device_id, pGBDeviceInfo->login_ip, pGBDeviceInfo->login_port, i);
                }
            }

            /* 移除物理设备 */
            GBDevice_info_remove(pMsg->register_id);

            /* 移除逻辑设备 */
            if (EV9000_DEVICETYPE_INTELLIGENTANALYSIS == pGBDeviceInfo->device_type)
            {
                //修正逻辑设备的智能分析状态
                i = SetGBLogicDeviceIntelligentStatus(pGBDeviceInfo->id, INTELLIGENT_STATUS_NULL);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() SetGBLogicDeviceIntelligentStatus Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() SetGBLogicDeviceIntelligentStatus OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
            }
            else
            {
                //移除下面对应的逻辑设备
                i = RemoveGBLogicDevice(pGBDeviceInfo->id);

                if (0 != i)
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() RemoveGBLogicDevice Error:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() RemoveGBLogicDevice OK:device_id=%s, i=%d \r\n", pGBDeviceInfo->device_id, i);
                }
            }

            GBDevice_info_free(pGBDeviceInfo);
            delete pGBDeviceInfo;
            pGBDeviceInfo = NULL;
        }

        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: GBDevice Not Enable:register_id=%s, register_name=%s, login_ip=%s \r\n", pMsg->register_id, pMsg->register_name, pMsg->login_ip);
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, 设备IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"设备已经被禁用");
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Device has been disabled");
        return -1;
    }

#if 0
    if (iExpires > 0 && checkNumberOfGBDeviceInfo() > g_RegistrationLimit)
    {
        SIP_UASAnswerToRegister(reg_info_index, 403, "Number of Registration Limited");
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() exit---: Number of Registration Limited \r\n");
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"设备注册数达到上限");
        return -1;
    }
#endif

    /* 如果是本机上报的设备，就不检查IP地址是否有变化了 */
    if (GBDevice_cfg.device_ip.length() > 0 && !IsLocalHost(pMsg->login_ip))
    {
        /* 查看设备的登录ip地址是否有变化 */
        if (0 != sstrcmp(GBDevice_cfg.device_ip.c_str(), pMsg->login_ip))
        {
#if 0
            /* 写入临时库 */
            i = RegisterSetIPConflictGBDevice2GBPhyDeviceTmpDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip, pMsg->link_type, pdboper);

            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_REG_IP_CONFLICT_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Login IP Conflict");
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: Login IP Conflict \r\n");
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, 设备IP地址=%s, 端口号=%d, 原因=%s%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"设备IP地址和之前注册的冲突,原注册IP=", GBDevice_cfg.device_ip.c_str());
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"device IP address is conflict with one registered before,old registration IP=", GBDevice_cfg.device_ip.c_str());
            return -1;
#endif
            /* 适应互联网应用，IP地址可能变化 */
            SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "设备注册IP地址发生变化, 可能存在ID重复配置:设备ID=%s, 设备原有注册IP地址=%s, 新的注册IP地址=%s", pMsg->register_id, GBDevice_cfg.device_ip.c_str(), pMsg->login_ip);
            EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Equipment registered IP address changes, there may be repeat configuration ID:ID=%s, old IP=%s, new IP=%s", pMsg->register_id, GBDevice_cfg.device_ip.c_str(), pMsg->login_ip);
        }
    }

    if (!GBDevice_info_find(pMsg->register_id))   //新注册设备增加内存记录
    {
        GBDevice_info_t* pNewGBDeviceInfo = NULL;

        i = GBDevice_info_init(&pNewGBDeviceInfo);

        if (i != 0 || pNewGBDeviceInfo == NULL)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_CREAT_DEVICE_INFO_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Create Device Error");
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc new GBDevice_info_t() fail  \r\n");
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, 设备IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"添加设备信息失败");
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Add device information failed.");
            return -1;
        }

        pNewGBDeviceInfo->id = GBDevice_cfg.id;
        osip_strncpy(pNewGBDeviceInfo->device_id, pMsg->register_id, MAX_ID_LEN);
        pNewGBDeviceInfo->device_type = pMsg->device_type;
        pNewGBDeviceInfo->link_type = GBDevice_cfg.link_type;
        osip_strncpy(pNewGBDeviceInfo->login_ip, pMsg->login_ip, MAX_IP_LEN);
        pNewGBDeviceInfo->login_port = pMsg->login_port;
        //pNewGBDeviceInfo->reg_info_index = reg_info_index;
        bIsNewDev = true;   //新dev标识

        /* 平台设备，默认是第三方，防止DeviceInfo消息不回应 */
        if (EV9000_DEVICETYPE_SIPSERVER == pNewGBDeviceInfo->device_type)
        {
            pNewGBDeviceInfo->three_party_flag = 1;
        }

        if (0 != GBDevice_info_add(pNewGBDeviceInfo))  /* 添加到队列*/
        {
            GBDevice_info_free(pNewGBDeviceInfo);
            delete pNewGBDeviceInfo;
            pNewGBDeviceInfo = NULL;

            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_CREAT_DEVICE_INFO_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"add Dev Info fail");
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc add Dev Info Error fail  \r\n");
            SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, 设备IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"添加设备信息失败");
            EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Add device information failed.");

            return -1;
        }
    }

    /* 获取设备信息 */
    pGBDeviceInfo = GBDevice_info_find(pMsg->register_id);

    if (NULL == pGBDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_FIND_DEVICE_INFO_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get GBDevice Info Error");
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: Get GBDevice Info Error \r\n");
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, 设备IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"获取设备信息失败");
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Access device information failed.");

        return -1;
    }

    /* 注册IP地址 */
    if (0 != sstrcmp(pGBDeviceInfo->login_ip, pMsg->login_ip))
    {
        memset(pGBDeviceInfo->login_ip, 0, MAX_IP_LEN);
        osip_strncpy(pGBDeviceInfo->login_ip, pMsg->login_ip, MAX_IP_LEN);
    }

    /* 注册端口号 */
    pGBDeviceInfo->login_port = pMsg->login_port;

    /* 注册索引重新赋值 */
    if (pGBDeviceInfo->reg_info_index != reg_info_index)
    {
        pGBDeviceInfo->reg_info_index = reg_info_index;
    }

    /* 设备索引重新赋值 */
    if (pGBDeviceInfo->id != GBDevice_cfg.id)
    {
        pGBDeviceInfo->id = GBDevice_cfg.id;
    }

    if ((EV9000_DEVICETYPE_SIPSERVER == pGBDeviceInfo->device_type && 0 == pGBDeviceInfo->three_party_flag)
        || EV9000_DEVICETYPE_DECODER == pGBDeviceInfo->device_type
        || EV9000_DEVICETYPE_MGWSERVER == pGBDeviceInfo->device_type)
    {

    }
    else
    {
        /* 传输方式重新赋值 */
        if (pGBDeviceInfo->trans_protocol != GBDevice_cfg.trans_protocol)
        {
            pGBDeviceInfo->trans_protocol = GBDevice_cfg.trans_protocol;
        }
    }

    /* 设备类型重新赋值 */
    if (pGBDeviceInfo->device_type != pMsg->device_type)
    {
        pGBDeviceInfo->device_type = pMsg->device_type;
    }

    /* 联网类型重新赋值 */
    if (pGBDeviceInfo->link_type != pMsg->link_type)
    {
        pGBDeviceInfo->link_type = pMsg->link_type;

        if (pGBDeviceInfo->device_type == EV9000_DEVICETYPE_SIPSERVER)
        {
            UpdateGBDeviceLinkType2DB(pGBDeviceInfo, pdboper);
        }
    }

    /* 获取超时时长 */
    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_reg_msg_proc() DeviceInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, iExpires, pGBDeviceInfo->reg_status, pGBDeviceInfo->reg_info_index);
    printf("GBDevice_reg_msg_proc() DeviceInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, iExpires, pGBDeviceInfo->reg_status, pGBDeviceInfo->reg_info_index);

    if (iExpires > 0)       //上线操作
    {
        i = DevReg(pGBDeviceInfo, GBDevice_cfg, pdboper);

        if (i != 0)
        {
            pGBDeviceInfo->reg_info_index = -1;
            pGBDeviceInfo->reg_status = 0;
            //memset(pGBDeviceInfo->login_ip, 0, 16);
            //pGBDeviceInfo->login_port = 0;
        }

        return i;
    }

    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SYSTEM_ERROR);
    SIP_UASAnswerToRegister(reg_info_index, 500, strErrorCode);
    //SIP_UASAnswerToRegister(reg_info_index, 500, (char*)"Server Error");
    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_reg_msg_proc() exit---: Server Error \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : GBDevice_unreg_msg_proc
 功能描述  : 设备注销消息处理
 输入参数  : GBDevice_reg_msg_t* pMsg
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年3月16日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GBDevice_unreg_msg_proc(GBDevice_reg_msg_t* pMsg, DBOper* pdboper)
{
    int i = 0;
    int reg_info_index = 0;
    GBDevice_info_t* pGBDeviceInfo = NULL;
    int iExpires = pMsg->expires;
    char strErrorCode[32] = {0};

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_unreg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    printf("\r\nGBDevice_unreg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "设备注册处理:设备ID=%s, 设备IP地址=%s, 端口号=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Device registration processing:ID=%s, IP=%s, port=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    /*验证参数*/
    if (!pMsg || pMsg->reg_info_index < 0)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "GBDevice_unreg_msg_proc() exit---: pMsg/ reg_info_indext Error \r\n");
        return -1;
    }

    reg_info_index = pMsg->reg_info_index;

    if (!(pMsg->login_ip) || !(pMsg->register_id) | (pMsg->login_port <= 0))
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_MSG_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"register_id/ IP /port Error");
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_unreg_msg_proc() exit---: register_id/ IP /port  Error \r\n");
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, 设备IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"设备ID/设备IP地/设备端口号不合法");
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"device IP device ID/ the port number is not valid");

        return -1;
    }

    /* 获取设备信息 */
    pGBDeviceInfo = GBDevice_info_find(pMsg->register_id);

    if (NULL == pGBDeviceInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_DEVICE_FIND_DEVICE_INFO_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get GBDevice Info Error");
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_unreg_msg_proc() exit---: Get GBDevice Info Error \r\n");
        SystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "设备注册失败:设备ID=%s, 设备IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"获取设备信息失败");
        EnSystemLog(EV9000_CMS_DEVICE_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "Device registration failed:device ID=%s, IP address=%s, port number=%d, cause=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Access device information failed.");
        return -1;
    }

    /* 获取超时时长 */
    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "GBDevice_unreg_msg_proc() DeviceInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, iExpires, pGBDeviceInfo->reg_status, pGBDeviceInfo->reg_info_index);
    printf("GBDevice_unreg_msg_proc() DeviceInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pGBDeviceInfo->device_id, iExpires, pGBDeviceInfo->reg_status, pGBDeviceInfo->reg_info_index);

    if (iExpires <= 0)   //注销  离线操作
    {
        i = DevUnReg(pGBDeviceInfo, pdboper);
        pGBDeviceInfo->reg_info_index = -1;
        pGBDeviceInfo->reg_status = 0;
        //memset(pGBDeviceInfo->login_ip, 0, 16);
        //pGBDeviceInfo->login_port = 0;
        return i;
    }

    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SYSTEM_ERROR);
    SIP_UASAnswerToRegister(reg_info_index, 500, strErrorCode);
    //SIP_UASAnswerToRegister(reg_info_index, 500, (char*)"Server Error");
    DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "GBDevice_unreg_msg_proc() exit---: Server Error \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB
 功能描述  : 将没有找到的设备注册信息写入标准物理设备临时库
 输入参数  : char* device_id
             char* pUserName
             char* pIPAddr
             int link_type
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月8日 星期六
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB(char* device_id, char* pUserName, char* pIPAddr, int link_type, DBOper* pdboper)
{
    int iRet = 0;

    if (NULL == device_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB() exit---: Param Error \r\n");
        return -1;
    }

    iRet = WriteErrorInfo2GBPhyDeviceTmpDB(device_id, pUserName, pIPAddr, DEVICE_ERROR_REASON_NOT_FOUND, link_type, pdboper);

    if (0 != iRet)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB() WriteErrorInfo2GBPhyDeviceTmpDB Error:iRet=%d \r\n", iRet);
    }
    else
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB() WriteErrorInfo2GBPhyDeviceTmpDB OK:iRet=%d \r\n", iRet);
    }

    return iRet;
}

/*****************************************************************************
 函 数 名  : RegisterSetIPConflictGBDevice2GBPhyDeviceTmpDB
 功能描述  : 将地址冲突的设备注册信息写入标准物理设备临时库
 输入参数  : char* device_id
             char* pUserName
             char* pIPAddr
             int link_type
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月27日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int RegisterSetIPConflictGBDevice2GBPhyDeviceTmpDB(char* device_id, char* pUserName, char* pIPAddr, int link_type, DBOper* pdboper)
{
    int iRet = 0;

    if (NULL == device_id)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "RegisterSetIPConflictGBDevice2GBPhyDeviceTmpDB() exit---: Param Error \r\n");
        return -1;
    }

    iRet = WriteErrorInfo2GBPhyDeviceTmpDB(device_id, pUserName, pIPAddr, DEVICE_ERROR_REASON_IP_CONFLICT, link_type, pdboper);
    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "RegisterSetIPConflictGBDevice2GBPhyDeviceTmpDB() WriteErrorInfo2GBPhyDeviceTmpDB:device_id=%s, iRet=%d \r\n", device_id, iRet);
    return iRet;
}

/*****************************************************************************
 函 数 名  : WriteErrorInfo2GBPhyDeviceTmpDB
 功能描述  : 将错误的设备信息写入标准物理设备临时库
 输入参数  : char* device_id
             char* user_name
             char* ip_addr
             device_error_reason_type_t eReason
             int link_type
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年6月8日 星期六
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int WriteErrorInfo2GBPhyDeviceTmpDB(char* device_id, char* user_name, char* ip_addr, device_error_reason_type_t eReason, int link_type, DBOper* pdboper)
{
    int iRet = 0;
    int record_count = 0;
    string strQuerySQL = "";
    string strInsertSQL = "";
    string strUpdateSQL = "";
    char strReason[16] = {0};
    char strLinkType[16] = {0};
    char strDeviceType[4] = {0};
    char* pTmp = NULL;

    if (NULL == device_id || NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "WriteErrorInfo2GBPhyDeviceTmpDB() exit---: Param Error \r\n");
        return -1;
    }

    pTmp = &device_id[10];
    osip_strncpy(strDeviceType, pTmp, 3);

    snprintf(strReason, 16, "%d", eReason);

    snprintf(strLinkType, 16, "%d", link_type);

    /* 1、查询SQL语句 */
    strQuerySQL.clear();
    strQuerySQL = "select * from GBPhyDeviceTempConfig WHERE DeviceID like '";
    strQuerySQL += device_id;
    strQuerySQL += "'";

    /* 2、插入SQL语句 */
    strInsertSQL.clear();
    strInsertSQL = "insert into GBPhyDeviceTempConfig (DeviceID,CMSID,UserName,DeviceIP,Enable,DeviceType,LinkType) values (";

    strInsertSQL += "'";
    strInsertSQL += device_id;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    strInsertSQL += local_cms_id_get();
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";

    if (NULL != user_name)
    {
        strInsertSQL += user_name;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";

    if (NULL != ip_addr && ip_addr[0] != '\0')
    {
        strInsertSQL += ip_addr;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "1";

    strInsertSQL += ",";

    strInsertSQL += strDeviceType;

    strInsertSQL += ",";

    strInsertSQL += strLinkType;

    strInsertSQL += ")";

    /* 3、更新SQL */
    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE GBPhyDeviceTempConfig SET ";

    /*  CMSID */
    strUpdateSQL += "CMSID = ";
    strUpdateSQL += "'";
    strUpdateSQL += local_cms_id_get();
    strUpdateSQL += "'";
    strUpdateSQL += ",";

    /*  UserName */
    strUpdateSQL += "UserName = ";
    strUpdateSQL += "'";
    strUpdateSQL += user_name;
    strUpdateSQL += "'";
    strUpdateSQL += ",";


    /*  DeviceIP  */
    strUpdateSQL += "DeviceIP = " ;
    strUpdateSQL += "'";
    strUpdateSQL += ip_addr;
    strUpdateSQL += "'";

    strUpdateSQL += " WHERE DeviceID like '";
    strUpdateSQL += device_id;
    strUpdateSQL += "'";

    /* 查询数据库 */
    record_count = pdboper->DB_Select(strQuerySQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "WriteErrorInfo2GBPhyDeviceTmpDB() DB Select:strSQL=%s,record_count=%d \r\n", strQuerySQL.c_str(), record_count);

    if (record_count <= 0)
    {
        iRet = pdboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteErrorInfo2GBPhyDeviceTmpDB() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteErrorInfo2GBPhyDeviceTmpDB() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
        }
    }
    else
    {
        iRet = pdboper->DB_Update(strUpdateSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteErrorInfo2GBPhyDeviceTmpDB() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteErrorInfo2GBPhyDeviceTmpDB() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
        }
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : WriteGBPhyDeviceInfoToDB
 功能描述  : 将物理设备信息写入标准物理设备表
 输入参数  : char* device_id
             char* user_name
             char* ip_addr
             int link_type
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月13日 星期四
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int WriteGBPhyDeviceInfoToDB(char* device_id, char* user_name, char* ip_addr, int link_type, DBOper* pdboper)
{
    int iRet = 0;
    int record_count = 0;
    string strQuerySQL = "";
    string strInsertSQL = "";
    string strUpdateSQL = "";
    char strLinkType[16] = {0};
    char strDeviceType[4] = {0};
    char* pTmp = NULL;

    if (NULL == device_id || NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_DEVICE, LOG_DEBUG, "WriteGBPhyDeviceInfoToDB() exit---: Param Error \r\n");
        return -1;
    }

    pTmp = &device_id[10];
    osip_strncpy(strDeviceType, pTmp, 3);

    snprintf(strLinkType, 16, "%d", link_type);

    /* 1、查询SQL语句 */
    strQuerySQL.clear();
    strQuerySQL = "select * from GBPhyDeviceConfig WHERE DeviceID like '";
    strQuerySQL += device_id;
    strQuerySQL += "'";

    /* 2、插入SQL语句 */
    strInsertSQL.clear();
    strInsertSQL = "insert into GBPhyDeviceConfig (DeviceID,CMSID,UserName,DeviceIP,Enable,DeviceType,LinkType) values (";

    strInsertSQL += "'";
    strInsertSQL += device_id;
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";
    strInsertSQL += local_cms_id_get();
    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";

    if (NULL != user_name)
    {
        strInsertSQL += user_name;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "'";

    if (NULL != ip_addr && ip_addr[0] != '\0')
    {
        strInsertSQL += ip_addr;
    }

    strInsertSQL += "'";

    strInsertSQL += ",";

    strInsertSQL += "1";

    strInsertSQL += ",";

    strInsertSQL += strDeviceType;

    strInsertSQL += ",";

    strInsertSQL += strLinkType;

    strInsertSQL += ")";

    /* 3、更新SQL */
    strUpdateSQL.clear();
    strUpdateSQL = "UPDATE GBPhyDeviceConfig SET ";

    /*  CMSID */
    strUpdateSQL += "CMSID = ";
    strUpdateSQL += "'";
    strUpdateSQL += local_cms_id_get();
    strUpdateSQL += "'";
    strUpdateSQL += ",";

    /*  UserName */
    strUpdateSQL += "UserName = ";
    strUpdateSQL += "'";
    strUpdateSQL += user_name;
    strUpdateSQL += "'";
    strUpdateSQL += ",";


    /*  DeviceIP  */
    strUpdateSQL += "DeviceIP = " ;
    strUpdateSQL += "'";
    strUpdateSQL += ip_addr;
    strUpdateSQL += "'";

    strUpdateSQL += " WHERE DeviceID like '";
    strUpdateSQL += device_id;
    strUpdateSQL += "'";

    /* 查询数据库 */
    record_count = pdboper->DB_Select(strQuerySQL.c_str(), 1);

    DEBUG_TRACE(MODULE_DEVICE, LOG_TRACE, "WriteGBPhyDeviceInfoToDB() DB Select:strSQL=%s,record_count=%d \r\n", strQuerySQL.c_str(), record_count);

    if (record_count <= 0)
    {
        iRet = pdboper->DB_Insert("", "", strInsertSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteGBPhyDeviceInfoToDB() DB Oper Error:strInsertSQL=%s, iRet=%d \r\n", strInsertSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteGBPhyDeviceInfoToDB() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
        }
    }
    else
    {
        iRet = pdboper->DB_Update(strUpdateSQL.c_str(), 1);

        if (iRet < 0)
        {
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteGBPhyDeviceInfoToDB() DB Oper Error:strUpdateSQL=%s, iRet=%d \r\n", strUpdateSQL.c_str(), iRet);
            DEBUG_TRACE(MODULE_DEVICE, LOG_ERROR, "WriteGBPhyDeviceInfoToDB() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
        }
    }

    return 0;
}
