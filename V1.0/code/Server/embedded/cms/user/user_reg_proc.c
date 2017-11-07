
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

#include "user/user_reg_proc.inc"
#include "device/device_reg_proc.inc"
#include "user/user_thread_proc.inc"
#include "user/user_srv_proc.inc"

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/
extern gbl_conf_t* pGblconf;              /* 全局配置信息 */

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
user_reg_msg_queue g_UserRegMsgQueue;   /* 用户注册消息队列 */
user_reg_msg_queue g_UserUnRegMsgQueue; /* 用户注销消息队列 */
#ifdef MULTI_THR
osip_mutex_t* g_UserRegMsgQueueLock = NULL;
#endif

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#if DECS("用户注册消息队列")
/*****************************************************************************
 函 数 名  : user_reg_msg_init
 功能描述  : 用户注册消息结构初始化
 输入参数  : user_reg_msg_t ** user_reg_msg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
int user_reg_msg_init(user_reg_msg_t** user_reg_msg)
{
    *user_reg_msg = (user_reg_msg_t*)osip_malloc(sizeof(user_reg_msg_t));

    if (*user_reg_msg == NULL)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_reg_msg_init() exit---: *user_reg_msg Smalloc Error \r\n");
        return -1;
    }

    (*user_reg_msg)->register_id[0] = '\0';
    (*user_reg_msg)->login_ip[0] = '\0';
    (*user_reg_msg)->login_port = 0;
    (*user_reg_msg)->register_name[0] = '\0';
    (*user_reg_msg)->expires = 0;
    (*user_reg_msg)->reg_info_index = -1;

    return 0;
}

/*****************************************************************************
 函 数 名  : user_reg_msg_free
 功能描述  : 用户注册消息结构释放
 输入参数  : user_reg_msg_t * user_reg_msg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void user_reg_msg_free(user_reg_msg_t* user_reg_msg)
{
    if (user_reg_msg == NULL)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_reg_msg_free() exit---: Param Error \r\n");
        return;
    }

    memset(user_reg_msg->register_id, 0, MAX_ID_LEN + 4);
    memset(user_reg_msg->login_ip, 0, MAX_IP_LEN);
    user_reg_msg->login_port = 0;
    memset(user_reg_msg->register_name, 0, MAX_128CHAR_STRING_LEN + 4);
    user_reg_msg->expires = 0;
    user_reg_msg->reg_info_index = -1;

    return;
}

/*****************************************************************************
 函 数 名  : user_reg_msg_list_init
 功能描述  : 用户注册消息队列初始化
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
int user_reg_msg_list_init()
{
    g_UserRegMsgQueue.clear();
    g_UserUnRegMsgQueue.clear();

#ifdef MULTI_THR
    /* init smutex */
    g_UserRegMsgQueueLock = (osip_mutex_t*)osip_mutex_init();

    if (NULL == g_UserRegMsgQueueLock)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_reg_msg_list_init() exit---: User Register Message List Lock Init Error \r\n");
        return -1;
    }

#endif
    return 0;
}

/*****************************************************************************
 函 数 名  : user_reg_msg_list_free
 功能描述  : 用户注册消息队列释放
 输入参数  : user_reg_msg_list_t * user_reg_msg_list
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年4月11日
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void user_reg_msg_list_free()
{
    user_reg_msg_t* pUserRegMsg = NULL;

    while (!g_UserRegMsgQueue.empty())
    {
        pUserRegMsg = (user_reg_msg_t*) g_UserRegMsgQueue.front();
        g_UserRegMsgQueue.pop_front();

        if (NULL != pUserRegMsg)
        {
            user_reg_msg_free(pUserRegMsg);
            osip_free(pUserRegMsg);
            pUserRegMsg = NULL;
        }
    }

    g_UserRegMsgQueue.clear();

    while (!g_UserUnRegMsgQueue.empty())
    {
        pUserRegMsg = (user_reg_msg_t*) g_UserUnRegMsgQueue.front();
        g_UserUnRegMsgQueue.pop_front();

        if (NULL != pUserRegMsg)
        {
            user_reg_msg_free(pUserRegMsg);
            osip_free(pUserRegMsg);
            pUserRegMsg = NULL;
        }
    }

    g_UserUnRegMsgQueue.clear();

#ifdef MULTI_THR

    if (NULL != g_UserRegMsgQueueLock)
    {
        osip_mutex_destroy((struct osip_mutex*)g_UserRegMsgQueueLock);
        g_UserRegMsgQueueLock = NULL;
    }

#endif

    return;
}

/*****************************************************************************
 函 数 名  : user_reg_msg_list_clean
 功能描述  : 用户注册消息队列清除
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
void user_reg_msg_list_clean()
{
    user_reg_msg_t* pUserRegMsg = NULL;

    while (!g_UserRegMsgQueue.empty())
    {
        pUserRegMsg = (user_reg_msg_t*) g_UserRegMsgQueue.front();
        g_UserRegMsgQueue.pop_front();

        if (NULL != pUserRegMsg)
        {
            user_reg_msg_free(pUserRegMsg);
            osip_free(pUserRegMsg);
            pUserRegMsg = NULL;
        }
    }

    g_UserRegMsgQueue.clear();

    return;
}

/*****************************************************************************
 函 数 名  : user_unreg_msg_list_clean
 功能描述  : 用户注销消息队列清除
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
void user_unreg_msg_list_clean()
{
    user_reg_msg_t* pUserRegMsg = NULL;

    while (!g_UserUnRegMsgQueue.empty())
    {
        pUserRegMsg = (user_reg_msg_t*) g_UserUnRegMsgQueue.front();
        g_UserUnRegMsgQueue.pop_front();

        if (NULL != pUserRegMsg)
        {
            user_reg_msg_free(pUserRegMsg);
            osip_free(pUserRegMsg);
            pUserRegMsg = NULL;
        }
    }

    g_UserUnRegMsgQueue.clear();

    return;
}

/*****************************************************************************
 函 数 名  : user_reg_msg_add
 功能描述  : 添加用户注册消息到队列中
 输入参数  : char* user_id
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
int user_reg_msg_add(char* user_id, char* login_ip, int login_port, char* register_name, int expires, int reg_info_index)
{
    user_reg_msg_t* pUserRegMsg = NULL;
    int iRet = 0;

    if (user_id == NULL || login_ip == NULL)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "user_reg_msg_add() exit---: Param Error \r\n");
        return -1;
    }

    iRet = user_reg_msg_init(&pUserRegMsg);

    if (iRet != 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_add() exit---: Message Init Error \r\n");
        return -1;
    }

    if ('\0' != user_id[0])
    {
        osip_strncpy(pUserRegMsg->register_id, user_id, MAX_ID_LEN);
    }

    if ('\0' != login_ip[0])
    {
        osip_strncpy(pUserRegMsg->login_ip, login_ip, MAX_IP_LEN);
    }

    pUserRegMsg->login_port = login_port;

    if (NULL != register_name && '\0' != register_name[0])
    {
        osip_strncpy(pUserRegMsg->register_name, register_name, MAX_128CHAR_STRING_LEN);
    }
    else
    {
        osip_strncpy(pUserRegMsg->register_name, user_id, MAX_128CHAR_STRING_LEN);
    }

    pUserRegMsg->expires = expires;
    pUserRegMsg->reg_info_index = reg_info_index;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_UserRegMsgQueueLock);
#endif

    if (expires <= 0)
    {
        g_UserUnRegMsgQueue.push_back(pUserRegMsg);
    }
    else
    {
        g_UserRegMsgQueue.push_back(pUserRegMsg);
    }

#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_UserRegMsgQueueLock);
#endif

    return 0;
}

/*****************************************************************************
 函 数 名  : scan_user_reg_msg_list
 功能描述  : 扫描用户注册消息队列
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年5月23日 星期四
    作    者   : yanghaifeng
    修改内容   : 新生成函数

*****************************************************************************/
void scan_user_reg_msg_list(DBOper* pUser_Reg_dboper)
{
    int iRet = 0;
    user_reg_msg_t* pUserRegMsg = NULL;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_UserRegMsgQueueLock);
#endif

    while (!g_UserRegMsgQueue.empty())
    {
        pUserRegMsg = (user_reg_msg_t*) g_UserRegMsgQueue.front();
        g_UserRegMsgQueue.pop_front();

        if (NULL != pUserRegMsg)
        {
            break;
        }
    }

#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_UserRegMsgQueueLock);
#endif

    if (NULL != pUserRegMsg)
    {
        DEBUG_TRACE(MODULE_USER, LOG_INFO,  "scan_user_reg_msg_list() \
        \r\n In Param: \
        \r\n register_id=%s \
        \r\n login_ip=%s \
        \r\n login_port=%d \
        \r\n register_name=%s \
        \r\n reg_info_index=%d \
        \r\n expires=%d \
        \r\n ", pUserRegMsg->register_id, pUserRegMsg->login_ip, pUserRegMsg->login_port, pUserRegMsg->register_name, pUserRegMsg->reg_info_index, pUserRegMsg->expires);

        iRet = user_reg_msg_proc(pUserRegMsg, pUser_Reg_dboper); //chenyu
        user_reg_msg_free(pUserRegMsg);
        osip_free(pUserRegMsg);
        pUserRegMsg = NULL;
    }

    return;
}

/*****************************************************************************
 函 数 名  : scan_user_unreg_msg_list
 功能描述  : 扫描用户注销消息队列
 输入参数  : DBOper* pUser_Reg_dboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年3月7日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
void scan_user_unreg_msg_list(DBOper* pUser_Reg_dboper)
{
    int iRet = 0;
    user_reg_msg_t* pUserRegMsg = NULL;

#if 0
    CMS_GBL_SMUTEX_LOCK((struct osip_mutex*)g_UserRegMsgQueueLock);
#endif

    while (!g_UserUnRegMsgQueue.empty())
    {
        pUserRegMsg = (user_reg_msg_t*) g_UserUnRegMsgQueue.front();
        g_UserUnRegMsgQueue.pop_front();

        if (NULL != pUserRegMsg)
        {
            break;
        }
    }

#if 0
    CMS_GBL_SMUTEX_UNLOCK((struct osip_mutex*)g_UserRegMsgQueueLock);
#endif

    if (NULL != pUserRegMsg)
    {
        DEBUG_TRACE(MODULE_USER, LOG_INFO,  "scan_user_unreg_msg_list() \
        \r\n In Param: \
        \r\n register_id=%s \
        \r\n login_ip=%s \
        \r\n login_port=%d \
        \r\n register_name=%s \
        \r\n reg_info_index=%d \
        \r\n expires=%d \
        \r\n ", pUserRegMsg->register_id, pUserRegMsg->login_ip, pUserRegMsg->login_port, pUserRegMsg->register_name, pUserRegMsg->reg_info_index, pUserRegMsg->expires);

        iRet = user_unreg_msg_proc(pUserRegMsg, pUser_Reg_dboper); //chenyu
        user_reg_msg_free(pUserRegMsg);
        osip_free(pUserRegMsg);
        pUserRegMsg = NULL;
    }

    return;
}
#endif

/*****************************************************************************
 函 数 名  : GetUserCfg
 功能描述  : 获取数据库用户配置信息
 输入参数  : DBOper* pdboper
             string strUserID
             user_cfg_t& user_cfg
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年11月26日 星期三
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int GetUserCfg(DBOper* pdboper, string strUserID, user_cfg_t& user_cfg)
{
    string strSQL = "";
    int record_count = 0;

    //printf("\r\n GetUserCfg() Enter--- \r\n");

    strSQL = "select * from UserConfig WHERE UserID like '";
    strSQL += strUserID + "';";
    record_count = pdboper->DB_Select(strSQL.c_str(), 1);

    if (record_count < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "GetUserCfg() DB Oper Error:strSQL=%s, record_count=%d \r\n", strSQL.c_str(), record_count);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "GetUserCfg() ErrorMsg=%s\r\n", pdboper->GetLastDbErrorMsg());
        return -1;
    }
    else if (record_count == 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_WARN, "GetUserCfg() exit---: No Record Count \r\n");
        return 0;
    }

    //有用户
    int tmp_ivalue = 0;
    unsigned int tmp_uvalue = 0;
    string tmp_svalue = "";

    /* 用户索引 */
    tmp_uvalue = 0;
    pdboper->GetFieldValue("ID", tmp_uvalue);

    user_cfg.id = tmp_uvalue;
    //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() user_cfg.id:%u", user_cfg.id);

    /* 用户统一编号id */
    tmp_svalue.clear();
    pdboper->GetFieldValue("UserID", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        user_cfg.user_id = tmp_svalue;
        //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() user_cfg.user_id:%s", (char*)user_cfg.user_id.c_str());
    }

    /* 用户名称 */
    tmp_svalue.clear();
    pdboper->GetFieldValue("UserName", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        user_cfg.user_name = tmp_svalue;
        //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() user_cfg.user_name:%s", (char*)user_cfg.user_name.c_str());
    }

    /* 是否启用*/
    tmp_ivalue = 0;
    pdboper->GetFieldValue("Enable", tmp_ivalue);

    user_cfg.enable = tmp_ivalue;
    //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() user_cfg.enable:%d", user_cfg.enable);

    /* 用户注册账户 */
    tmp_svalue.clear();
    pdboper->GetFieldValue("UserName", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        user_cfg.register_account = tmp_svalue;
        //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() user_cfg.register_account:%s", (char*)user_cfg.register_account.c_str());
    }

    /* 用户注册密码 */
    tmp_svalue.clear();
    pdboper->GetFieldValue("Password", tmp_svalue);

    if (!tmp_svalue.empty())
    {
        user_cfg.register_password = tmp_svalue;
        //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() user_cfg.register_password:%s", (char*)user_cfg.register_password.c_str());
    }

    /* 用户等级*/
    tmp_uvalue = 0;
    pdboper->GetFieldValue("Level", tmp_uvalue);

    user_cfg.user_level = tmp_uvalue;
    //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() pUserInfo->user_level:%d", user_cfg.user_level);

    /* 用户权限*/
    tmp_ivalue = 0;
    pdboper->GetFieldValue("Permission", tmp_ivalue);

    user_cfg.user_permission = tmp_ivalue;

    //printf("\r\n GetUserCfg() Exit--- \r\n");
    //DEBUG_TRACE(MODULE_USER, LOG_INFO, "GetUserCfg() user_cfg.user_permission:%d \r\n", user_cfg.user_permission);
    return record_count;
}

/*****************************************************************************
 函 数 名  : UserRefresh
 功能描述  : 用户刷新处理
 输入参数  : user_info_t* pUserInfo
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
int UserRefresh(user_info_t* pUserInfo, DBOper* pdboper)
{
    pUserInfo->auth_count = 0;
    pUserInfo->reg_status = 1;
    return 0;
}

/*****************************************************************************
 函 数 名  : UserUnReg
 功能描述  : 用户去注册处理
 输入参数  : user_info_t* pUserInfo
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月21日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int UserUnReg(user_info_t* pUserInfo, DBOper* pdboper)
{
    string strUserID = "";
    int i = 0;

    if (NULL == pUserInfo || NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "UserUnReg() exit---: Param Error \r\n");
        return -1;
    }

    strUserID = pUserInfo->user_id;

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() Enter---:user_id=%s, login_ip=%s, login_port=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnReg() Enter---:user_id=%s, login_ip=%s, login_port=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

    SIP_UASAnswerToRegister(pUserInfo->reg_info_index, 200, NULL);

    UserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_WARNING, pUserInfo, "用户注销登录");
    EnUserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_WARNING, pUserInfo, "User log off.");
    DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserUnReg() REMOVE:user_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->reg_info_index);

    //i = shdb_user_operate_cmd_proc(pUserInfo, EV9000_SHDB_DVR_SYSTEM_STOP);

    /* 移除用户锁定的点位信息 */
    i = RemoveGBLogicDeviceLockInfoByUserInfo(pUserInfo);
    printf("UserUnReg() RemoveGBLogicDeviceLockInfoByUserInfo() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnReg() RemoveGBLogicDeviceLockInfoByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() RemoveGBLogicDeviceLockInfoByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* 查找用户的业务，停止所有业务 */
    i = StopAllServiceTaskByUserInfo(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnReg() StopAllServiceTaskByUserInfo() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnReg() StopAllServiceTaskByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() StopAllServiceTaskByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* 停止用户业务处理线程 */
    i = user_srv_proc_thread_recycle(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnReg() user_srv_proc_thread_recycle() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnReg() user_srv_proc_thread_recycle Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() user_srv_proc_thread_recycle OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* 从数据库中删除在线用户信息 */
    i = DeleteUserRegInfoFromDB(pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, pdboper);
    printf("UserUnReg() DeleteUserRegInfoFromDB() Exit--- \r\n");

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnReg() DeleteUserRegInfoFromDB Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() DeleteUserRegInfoFromDB OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    i = NotifyOnlineUserToAllClientUser(pUserInfo, 2, pdboper);
    printf("UserUnReg() NotifyOnlineUserToAllClientUser() Exit--- \r\n");

    if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送通知在线用户变化消息给在线用户失败, 移除的在线用户:用户ID=%s, IP地址=%s, 端口号=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send notification online users change to online user failure, remove online users: user ID = % s, IP address = % s, port = %d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnReg() NotifyOnlineUserToAllClientUser Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送通知在线用户变化消息给在线用户成功, 移除的在线用户:用户ID=%s, IP地址=%s, 端口号=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send notification online users change to online user failure success, remove online users: user ID = % s, IP address = % s, port = %d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() NotifyOnlineUserToAllClientUser OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* 删除内存节点和更新数据库信息 */
    i = user_info_remove(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnReg() user_info_remove() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnReg() user_info_remove Error:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() user_info_remove OK:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }

    i = UpdateUserRegInfo2DB2(strUserID, pdboper);   //更新数据库状态
    printf("UserUnReg() UpdateUserRegInfo2DB2() Exit--- \r\n");

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnReg() UpdateUserRegInfo2DB2 Error:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() UpdateUserRegInfo2DB2 OK:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }

    /* 释放掉没用的TCP链接 */
    free_unused_user_tcp_connect();
    printf("UserUnReg() free_unused_user_tcp_connect() Exit--- \r\n");

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnReg() Exit--- \r\n");
    printf("UserUnReg() Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : UserUnRegAbnormal
 功能描述  : 用户异常注销
 输入参数  : user_info_t* pUserInfo
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月17日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int UserUnRegAbnormal(user_info_t* pUserInfo, DBOper* pdboper)
{
    string strUserID = "";
    int i = 0;

    if (NULL == pUserInfo || NULL == pdboper)
    {
        DEBUG_TRACE(MODULE_USER, LOG_DEBUG, "UserUnRegAbnormal() exit---: Param Error \r\n");
        return -1;
    }

    strUserID = pUserInfo->user_id;

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() Enter---:user_id=%s, login_ip=%s, login_port=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnRegAbnormal() Enter---:user_id=%s, login_ip=%s, login_port=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

    SIP_UASAnswerToRegister(pUserInfo->reg_info_index, 200, NULL);

    UserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_WARNING, pUserInfo, "用户注销登录");
    EnUserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_WARNING, pUserInfo, "User log off.");
    DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserUnRegAbnormal() REMOVE:user_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->reg_info_index);

    //i = shdb_user_operate_cmd_proc(pUserInfo, EV9000_SHDB_DVR_ABNORMAL_STOP);

    /* 移除用户锁定的点位信息 */
    i = RemoveGBLogicDeviceLockInfoByUserInfo(pUserInfo);
    printf("UserUnRegAbnormal() RemoveGBLogicDeviceLockInfoByUserInfo() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnRegAbnormal() RemoveGBLogicDeviceLockInfoByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() RemoveGBLogicDeviceLockInfoByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* 查找用户的业务，停止所有业务 */
    i = StopAllServiceTaskByUserInfo(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnRegAbnormal() StopAllServiceTaskByUserInfo() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnRegAbnormal() StopAllServiceTaskByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() StopAllServiceTaskByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* 停止用户业务处理线程 */
    i = user_srv_proc_thread_recycle(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnRegAbnormal() user_srv_proc_thread_recycle() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnRegAbnormal() user_srv_proc_thread_recycle Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() user_srv_proc_thread_recycle OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* 从数据库中删除在线用户信息 */
    i = DeleteUserRegInfoFromDB(pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, pdboper);
    printf("UserUnRegAbnormal() DeleteUserRegInfoFromDB() Exit--- \r\n");

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnRegAbnormal() DeleteUserRegInfoFromDB Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() DeleteUserRegInfoFromDB OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    i = NotifyOnlineUserToAllClientUser(pUserInfo, 2, pdboper);
    printf("UserUnRegAbnormal() NotifyOnlineUserToAllClientUser() Exit--- \r\n");

    if (i < 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送通知在线用户变化消息给在线用户失败, 移除的在线用户:用户ID=%s, IP地址=%s, 端口号=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send notification online users change to online user failure, remove online users: user ID = % s, IP address = % s, port = %d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnRegAbnormal() NotifyOnlineUserToAllClientUser Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }
    else if (i > 0)
    {
        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送通知在线用户变化消息给在线用户成功, 移除的在线用户:用户ID=%s, IP地址=%s, 端口号=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send notification online users change to online user failure, remove online users: user ID = % s, IP address = % s, port = %d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() NotifyOnlineUserToAllClientUser OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
    }

    /* 删除内存节点和更新数据库信息 */
    i = user_info_remove(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
    printf("UserUnRegAbnormal() user_info_remove() Exit--- \r\n");

    if (0 != i)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnRegAbnormal() user_info_remove Error:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() user_info_remove OK:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }

    i = UpdateUserRegInfo2DB2(strUserID, pdboper);   //更新数据库状态
    printf("UserUnRegAbnormal() UpdateUserRegInfo2DB2() Exit--- \r\n");

    if (i < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserUnRegAbnormal() UpdateUserRegInfo2DB2 Error:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }
    else
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() UpdateUserRegInfo2DB2 OK:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
    }

    /* 释放掉没用的TCP链接 */
    free_unused_user_tcp_connect();
    printf("UserUnRegAbnormal() free_unused_user_tcp_connect() Exit--- \r\n");

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserUnRegAbnormal() Exit--- \r\n");
    printf("UserUnRegAbnormal() Exit--- \r\n");

    return 0;
}

/*****************************************************************************
 函 数 名  : UserReg
 功能描述  : 用户注册处理
 输入参数  : user_info_t* pUserInfo
             user_cfg_t& user_cfg
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月21日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int UserReg(user_info_t* pUserInfo, user_cfg_t& user_cfg, DBOper* pdboper)
{
    int reg_info_index = pUserInfo->reg_info_index;
    int user_tl_pos = 0;
    int i = 0;
    char* realm = NULL;
    int iAuth = 0;
    osip_authorization_t* pAuthorization = NULL;
    char strRegisterDomain[128] = {0};
    char* server_ip = NULL;
    int server_port;
    char* pcRegServerEthName = NULL;
    char strErrorCode[32] = {0};
    int iIsRefreshReg = 0; /* 是否是刷新注册 */
    char* call_id = NULL;

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() Enter---\r\n");
    printf("UserReg() Enter---\r\n");

    /* 检查是否有认证信息 */
    /* get authorization*/
    pAuthorization = SIP_UASGetRegisterAuthorization(reg_info_index);

    if (pAuthorization && pAuthorization->realm)
    {
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() Check Realm: Begin---\r\n");

        realm = osip_getcopy_unquoted_string(pAuthorization->realm);
        i = IsLocalAuthRealm(realm);

        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() Check Realm: End--- i=%d\r\n", i);

        if (0 == i)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_AUTH_REALM_NOT_LOCAL_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"Auth Realm Is Not Local");
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: Auth Realm Is Not Local:realm=%s \r\n", realm);
            SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"认证域不是本CMS,认证域=", realm);
            EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Authentication domain is not the current CMS, authentication domain =", realm);
            osip_free(realm);
            realm = NULL;
            return -1;
        }
        else
        {
            osip_free(realm);
            realm = NULL;

            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() SIP Auth: Begin--- :register_account=%s, register_password=%s \r\n", (char*)user_cfg.register_account.c_str(), (char*)user_cfg.register_password.c_str());

            iAuth = SIP_Auth(pAuthorization, (char*)user_cfg.register_account.c_str(), (char*)user_cfg.register_password.c_str(), (char*)"REGISTER");

            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() SIP Auth: End--- iAuth=%d\r\n", iAuth);

            if (0 == iAuth) /*认证失败*/
            {
                pUserInfo->auth_count++;

                if (pUserInfo->auth_count > 0)
                {
                    pUserInfo->auth_count = 0;
                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_AUTH_FAILD_ERROR);
                    SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                    //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"AUTH FAIL");
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: AUTH FAIL \r\n");
                    SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"用户认证失败");
                    EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"User authenticate failed.");
                    return -1;
                }
                else
                {
                    snprintf(strRegisterDomain, 128, "%s.spvmn.cn", pGblconf->register_region);
                    SIP_UASAnswerToRegister4Auth(reg_info_index, strRegisterDomain);
                    DEBUG_TRACE(MODULE_USER, LOG_WARN, "UserReg() exit---: NEED AUTH \r\n");
                    //SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"用户需要认证");
                    //EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"User need to authenticate.");
                    return 0;
                }
            }
            else      //auth success
            {
                pUserInfo->auth_count = 0;

                /* 获取注册的服务器IP地址、端口号，确定是从哪个网注册的 */
                server_ip = SIP_GetUASServerIP(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                server_port = SIP_GetUASServerPort(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                pcRegServerEthName = get_ip_eth_name(server_ip);

                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg(): user_id=%s, login_ip=%s, login_port=%d, server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, server_ip, server_port, pcRegServerEthName);
                printf("UserReg(): user_id=%s, login_ip=%s, login_port=%d, server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, server_ip, server_port, pcRegServerEthName);

                if (NULL == server_ip)
                {
                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_IP_ERROR);
                    SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                    //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server IP Error");
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: Get Register Server IP Error \r\n");
                    SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"获取用户注册的服务器IP地址失败");
                    EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Get user register server IP address failed.");
                    return -1;
                }

                if (server_port <= 0)
                {
                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_PORT_ERROR);
                    SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                    //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server Port Error");
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: Get Register Server Port Error \r\n");
                    SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"获取用户注册的服务器端口号失败");
                    EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Get user register server port failed.");
                    return -1;
                }

                if (NULL == pcRegServerEthName)
                {
                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_GET_SERVER_IP_ETHNAME_ERROR);
                    SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                    //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register Server EthName Error");
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: Get Register Server EthName Error \r\n");
                    SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s, 服务器IP=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"获取用户注册的服务器网口名称失败", server_ip);
                    EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s, server IP=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Get user register server network port name failed.", server_ip);
                    return -1;
                }

                /* 确定是否是刷新注册 */
                call_id = SIP_GetUASCallID(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

                if (NULL == call_id)
                {
                    memset(strErrorCode, 0, 32);
                    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_CALLID_ERROR);
                    SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                    //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get Register CallID Error");
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() exit---: Get Register CallID Error:server_ip=%s, server_port=%d, RegServerEthName=%s \r\n", server_ip, server_port, pcRegServerEthName);
                    SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"获取设备注册的CallID字段信息失败");
                    EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, IP address=%s, port number=%d, cause=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Access information of call id for device registration failed");

                    return -1;
                }

                if (pUserInfo->strCallID[0] == '\0')
                {
                    osip_strncpy(pUserInfo->strCallID, call_id, MAX_128CHAR_STRING_LEN);
                    iIsRefreshReg = 0;
                }
                else
                {
                    if (0 != sstrcmp(pUserInfo->strCallID, call_id))
                    {
                        memset(pUserInfo->strCallID, 0, MAX_128CHAR_STRING_LEN + 4);
                        osip_strncpy(pUserInfo->strCallID, call_id, MAX_128CHAR_STRING_LEN);
                        iIsRefreshReg = 0;
                    }
                    else
                    {
                        iIsRefreshReg = 1; /* 刷新注册 */
                    }
                }

                DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserReg() user_id=%s, login_ip=%s, login_port=%d: old callid=%s, new callid=%s, IsRefreshReg=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->strCallID, call_id, iIsRefreshReg);
                printf("UserReg() user_id=%s, login_ip=%s, login_port=%d: old callid=%s, new callid=%s, IsRefreshReg=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->strCallID, call_id, iIsRefreshReg);

                /* 确定注册服务器IP地址 */
                if (pUserInfo->strRegServerIP[0] == '\0')
                {
                    osip_strncpy(pUserInfo->strRegServerIP, server_ip, MAX_IP_LEN);
                }
                else
                {
                    if (0 != sstrcmp(pUserInfo->strRegServerIP, server_ip))
                    {
                        memset(pUserInfo->strRegServerIP, 0, MAX_IP_LEN);
                        osip_strncpy(pUserInfo->strRegServerIP, server_ip, MAX_IP_LEN);
                    }
                }

                /* 确定注册服务器端口号 */
                if (pUserInfo->iRegServerPort <= 0)
                {
                    pUserInfo->iRegServerPort = server_port;
                }
                else
                {
                    if (pUserInfo->iRegServerPort != server_port)
                    {
                        pUserInfo->iRegServerPort = server_port;
                    }
                }

                /* 确定IP地址网口名称 */
                if (pUserInfo->strRegServerEthName[0] == '\0')
                {
                    osip_strncpy(pUserInfo->strRegServerEthName, pcRegServerEthName, MAX_IP_LEN);
                }
                else
                {
                    if (0 != sstrcmp(pUserInfo->strRegServerEthName, pcRegServerEthName))
                    {
                        memset(pUserInfo->strRegServerEthName, 0, MAX_IP_LEN);
                        osip_strncpy(pUserInfo->strRegServerEthName, pcRegServerEthName, MAX_IP_LEN);
                    }
                }

                if (pUserInfo->reg_status <= 0)
                {
                    /* 创建用户业务处理线程 */
                    user_tl_pos = user_srv_proc_thread_find(pUserInfo);
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_find:user_tl_pos=%d \r\n", user_tl_pos);
                    printf("UserReg() user_srv_proc_thread_find:user_tl_pos=%d \r\n", user_tl_pos);

                    if (user_tl_pos < 0)
                    {
                        //分配处理线程
                        i = user_srv_proc_thread_assign(pUserInfo);
                        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_assign:i=%d \r\n", i);
                        printf("UserReg() user_srv_proc_thread_assign:i=%d \r\n", i);

                        if (i != 0)
                        {
                            memset(strErrorCode, 0, 32);
                            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_REG_ASSIGN_THREAD_ERROR);
                            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"User Srv Thread Start Error");
                            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: User Srv Thread Start Error \r\n");
                            SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"分配用户业务处理线程失败");
                            EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Assign user business processing threads fail.");
                            return -1;
                        }
                    }
                    else
                    {
                        /* 移除用户锁定的点位信息 */
                        i = RemoveGBLogicDeviceLockInfoByUserInfo(pUserInfo);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() RemoveGBLogicDeviceLockInfoByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() RemoveGBLogicDeviceLockInfoByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                        }

                        /* 查找用户的业务，停止所有业务 */
                        i = StopAllServiceTaskByUserInfo(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() StopAllServiceTaskByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() StopAllServiceTaskByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                        }

                        /* 释放一下之前的用户业务处理线程 */
                        i = user_srv_proc_thread_recycle(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

                        if (0 != i)
                        {
                            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() user_srv_proc_thread_recycle Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                        }
                        else
                        {
                            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_recycle OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                        }

                        //分配处理线程
                        i = user_srv_proc_thread_assign(pUserInfo);
                        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_assign:i=%d \r\n", i);

                        if (i != 0)
                        {
                            memset(strErrorCode, 0, 32);
                            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_REG_ASSIGN_THREAD_ERROR);
                            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"User Srv Thread Start Error");
                            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: User Srv Thread Start Error \r\n");
                            SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"分配用户业务处理线程失败");
                            EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Assign user business processing threads fail.");
                            return -1;
                        }
                    }

                    pUserInfo->reg_status = 1;

                    i = UpdateUserRegInfo2DB(pUserInfo->user_id, pUserInfo->reg_status, pdboper);

                    if (0 != i)
                    {
                        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() UpdateUserRegInfo2DB Error:user_id=%s, pUserInfo->reg_status=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->reg_status, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() UpdateUserRegInfo2DB OK:user_id=%s, pUserInfo->reg_status=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->reg_status, i);
                    }

                    /* 添加到在线用户数据表 */
                    i = AddUserRegInfo2DB(pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, pdboper);

                    if (i < 0)
                    {
                        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() AddUserRegInfo2DB Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                    }
                    else
                    {
                        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() AddUserRegInfo2DB OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                    }

                    i = NotifyOnlineUserToAllClientUser(pUserInfo, 1, pdboper);

                    if (i < 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送通知在线用户变化消息给在线用户失败, 增加的在线用户:用户ID=%s, IP地址=%s, 端口号=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Send notification online users change to online users failed, increase the online user: user ID = % s, IP address = % s, port = %d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() NotifyOnlineUserToAllClientUser Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                    }
                    else if (i > 0)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送通知在线用户变化消息给在线用户成功, 增加的在线用户:用户ID=%s, IP地址=%s, 端口号=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Send notification online users change to online users successfully, increase the online user: user ID = % s, IP address = % s, port = %d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() NotifyOnlineUserToAllClientUser OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                    }

                    UserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_NORMAL, pUserInfo, "用户登录成功");
                    EnUserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_NORMAL, pUserInfo, "User login successfully.");
                    DEBUG_TRACE(MODULE_USER, LOG_INFO, "UserReg() ADD:user_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->reg_info_index);
                    printf("UserReg() ADD:user_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->reg_info_index);

                    //i = shdb_user_operate_cmd_proc(pUserInfo, EV9000_SHDB_DVR_SYSTEM_START);
                }
                else
                {
                    if (iIsRefreshReg)
                    {
                        SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户注册刷新:用户ID=%s, IP地址=%s, 端口号=%d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                        EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User registration refresh: user ID = % s, = % s IP address, port number = % d", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                        //DebugRunTrace("用户注册刷新:用户ID=%s, IP地址=%s, 端口号=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() REFRESH:user_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->reg_info_index);
                        printf("UserReg() REFRESH:user_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->reg_info_index);
                    }
                    else
                    {
                        user_tl_pos = user_srv_proc_thread_find(pUserInfo);
                        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_find:user_tl_pos=%d \r\n", user_tl_pos);
                        printf("UserReg() user_srv_proc_thread_find:user_tl_pos=%d \r\n", user_tl_pos);

                        if (user_tl_pos < 0)
                        {
                            //分配处理线程
                            i = user_srv_proc_thread_assign(pUserInfo);
                            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_assign:i=%d \r\n", i);
                            printf("UserReg() user_srv_proc_thread_assign:i=%d \r\n", i);

                            if (i != 0)
                            {
                                memset(strErrorCode, 0, 32);
                                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_REG_ASSIGN_THREAD_ERROR);
                                SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                                //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"User Srv Thread Start Error");
                                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: User Srv Thread Start Error \r\n");
                                SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"分配用户业务处理线程失败");
                                EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Assign user business processing threads fail.");
                                return -1;
                            }
                        }
                        else
                        {
                            /* 移除用户锁定的点位信息 */
                            i = RemoveGBLogicDeviceLockInfoByUserInfo(pUserInfo);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() RemoveGBLogicDeviceLockInfoByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() RemoveGBLogicDeviceLockInfoByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                            }

                            /* 查找用户的业务，停止所有业务 */
                            i = StopAllServiceTaskByUserInfo(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() StopAllServiceTaskByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() StopAllServiceTaskByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                            }

                            /* 释放一下之前的用户业务处理线程 */
                            i = user_srv_proc_thread_recycle(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

                            if (0 != i)
                            {
                                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() user_srv_proc_thread_recycle Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                            }
                            else
                            {
                                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_recycle OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
                            }

                            //分配处理线程
                            i = user_srv_proc_thread_assign(pUserInfo);
                            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "UserReg() user_srv_proc_thread_assign:i=%d \r\n", i);
                            printf("UserReg() user_srv_proc_thread_assign:i=%d \r\n", i);

                            if (i != 0)
                            {
                                memset(strErrorCode, 0, 32);
                                snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_REG_ASSIGN_THREAD_ERROR);
                                SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
                                //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"User Srv Thread Start Error");
                                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: User Srv Thread Start Error \r\n");
                                SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"分配用户业务处理线程失败");
                                EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Assign user business processing threads fail.");
                                return -1;
                            }
                        }

                        UserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_WARNING, pUserInfo, "用户异常掉线后重新登录成功");
                        EnUserLog(EV9000_USER_LOG_LOGIN, EV9000_LOG_LEVEL_WARNING, pUserInfo, "User re login successfully");
                    }
                }

                SIP_UASAnswerToRegister(reg_info_index, 200, NULL);

                printf("UserReg() ADD AGAIN:user_id=%s, login_ip=%s, login_port=%d, reg_info_index=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, pUserInfo->reg_info_index);
                return 0;
            }
        }
    }
    else
    {
        pUserInfo->auth_count++;

        if (pUserInfo->auth_count > 3)
        {
            pUserInfo->auth_count = 0;
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_AUTH_FAILD_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 403, NULL);
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: AUTH FAIL \r\n");
            SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"用户认证失败");
            EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"User authenticate failed");
            return -1;
        }
        else
        {
            snprintf(strRegisterDomain, 128, "%s.spvmn.cn", pGblconf->register_region);
            SIP_UASAnswerToRegister4Auth(reg_info_index, strRegisterDomain);
            DEBUG_TRACE(MODULE_USER, LOG_WARN, "UserReg() exit---: NEED AUTH \r\n");
            //SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"用户需要认证");
            //EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_WARNING, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"User need to authenticate.");
            return 0;
        }
    }

    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SYSTEM_ERROR);
    SIP_UASAnswerToRegister(reg_info_index, 500, strErrorCode);
    //SIP_UASAnswerToRegister(reg_info_index, 500, (char*)"Server Error");
    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "UserReg() exit---: Forbidden \r\n");
    SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"未知原因");
    EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, (char*)"Unknown reason.");
    return -1;
}

/*****************************************************************************
 函 数 名  : user_reg_msg_proc
 功能描述  : 用户注册消息处理
 输入参数  : user_reg_msg_t* pMsg
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月21日 星期二
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int user_reg_msg_proc(user_reg_msg_t* pMsg, DBOper* pdboper)
{
    int i = 0;
    int user_pos = -1;
    string strUserID = "";
    int reg_info_index = 0;
    user_info_t* pUserInfo = NULL;
    int iExpires = pMsg->expires;
    user_cfg_t user_cfg;
    char strErrorCode[32] = {0};

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    printf("\r\nuser_reg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户登录处理:用户ID=%s, IP地址=%s, 端口号=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User login: user ID = % s, = % s IP address, port number = %d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    /*验证参数*/
    if (!pMsg || pMsg->reg_info_index < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() exit---: msg /reg_info_index error \r\n");
        return -1;
    }

    reg_info_index = pMsg->reg_info_index;

    if ('\0' == pMsg->register_id[0] || '\0' == pMsg->login_ip[0] || pMsg->login_port <= 0)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_MSG_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"msg /User ID/login_ip login_port error");
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() exit---: msg /User ID/login_ip login_port error \r\n");
        SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"用户ID/用户IP地/用户端口号不合法");
        EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"User ID / user IP / user port number is not legitimate.");
        return -1;
    }

    printf("user_reg_msg_proc() GetUserCfg Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    /* 查找用户*/
    if (GetUserCfg(pdboper, pMsg->register_id, user_cfg) <= 0) //非法用户或查询信息失败
    {
        //i = RegisterSetNotFoundGBDevice2GBPhyDeviceTmpDB(pMsg->register_id, pMsg->register_name, pMsg->login_ip);

        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_GET_USER_INFO_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 404, (char*)"Find User Info Error");

        /* 移除用户锁定的点位信息 */
        i = RemoveGBLogicDeviceLockInfoByUserInfo(pUserInfo);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() RemoveGBLogicDeviceLockInfoByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() RemoveGBLogicDeviceLockInfoByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
        }

        /* 移除用户业务 */
        i = StopAllServiceTaskByUserInfo(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() StopAllServiceTaskByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() StopAllServiceTaskByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }

        /* 回收用户业务线程 */
        i = user_srv_proc_thread_recycle(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() user_srv_proc_thread_recycle Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_srv_proc_thread_recycle OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }

        /* 从数据库中删除在线用户信息 */
        user_pos = user_info_find(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

        if (user_pos >= 0)
        {
            pUserInfo = user_info_get(user_pos);

            if (NULL != pUserInfo)
            {
                i = DeleteUserRegInfoFromDB(pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, pdboper);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() DeleteUserRegInfoFromDB Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() DeleteUserRegInfoFromDB OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, i);
                }

                i = NotifyOnlineUserToAllClientUser(pUserInfo, 2, pdboper);

                if (i < 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送通知在线用户变化消息给在线用户失败, 移除的在线用户:用户ID=%s, IP地址=%s, 端口号=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Users send notification that online users change  to  online user failed, remove online users: user ID = % s, IP address = % s, port = % d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() NotifyOnlineUserToAllClientUser Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                }
                else if (i > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送通知在线用户变化消息给在线用户成功, 移除的在线用户:用户ID=%s, IP地址=%s, 端口号=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Users send notification that online users change  to  online user successfully, remove online users: user ID = % s, IP address = % s, port = % d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() NotifyOnlineUserToAllClientUser OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                }
            }

            i = user_info_remove(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() user_info_remove Error:UserID=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_info_remove OK:UserID=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
            }

            /* 释放掉没用的TCP链接 */
            free_unused_user_tcp_connect();
        }

        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() exit---: Find User Info Error \r\n");
        SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"数据库中没有找到该用户");
        EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"The user was not found in the database.");
        return -1;
    }

    printf("user_reg_msg_proc() GetUserCfg Exit---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    /* 用户是否启用 */
    if (0 == user_cfg.enable)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_NOT_ENABLE_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 403, (char*)"User Not Enable");

        /* 移除用户锁定的点位信息 */
        i = RemoveGBLogicDeviceLockInfoByUserInfo(pUserInfo);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() RemoveGBLogicDeviceLockInfoByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() RemoveGBLogicDeviceLockInfoByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
        }

        /* 移除用户业务 */
        i = StopAllServiceTaskByUserInfo(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() StopAllServiceTaskByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() StopAllServiceTaskByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }

        /* 回收用户业务线程 */
        i = user_srv_proc_thread_recycle(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

        if (0 != i)
        {
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() user_srv_proc_thread_recycle Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }
        else
        {
            DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_srv_proc_thread_recycle OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
        }

        /* 从数据库中删除在线用户信息 */
        user_pos = user_info_find(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

        if (user_pos >= 0)
        {
            pUserInfo = user_info_get(user_pos);

            if (NULL != pUserInfo)
            {
                i = DeleteUserRegInfoFromDB(pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, pdboper);

                if (i < 0)
                {
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() DeleteUserRegInfoFromDB Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, i);
                }
                else
                {
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() DeleteUserRegInfoFromDB OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, i);
                }

                i = NotifyOnlineUserToAllClientUser(pUserInfo, 2, pdboper);

                if (i < 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送通知在线用户变化消息给在线用户失败, 移除的在线用户:用户ID=%s, IP地址=%s, 端口号=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Users send notification that online users change to online user failed, remove online users: user ID = % s, IP address = % s, port = % d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() NotifyOnlineUserToAllClientUser Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                }
                else if (i > 0)
                {
                    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送通知在线用户变化消息给在线用户成功, 移除的在线用户:用户ID=%s, IP地址=%s, 端口号=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Users send notification that online users change to online user successfully, remove online users: user ID = % s, IP address = % s, port = % d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() NotifyOnlineUserToAllClientUser OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
                }
            }

            i = user_info_remove(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() user_info_remove Error:UserID=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_info_remove OK:UserID=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
            }

            /* 释放掉没用的TCP链接 */
            free_unused_user_tcp_connect();
        }

        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() exit---: User Not Enable \r\n");
        SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"用户已经被禁用");
        EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"User has been disabled.");
        return -1;
    }

    user_pos = user_info_find(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_info_find:register_id=%s,login_ip=%s,login_port=%d,user_pos=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, user_pos);
    printf("user_reg_msg_proc() user_info_find:register_id=%s,login_ip=%s,login_port=%d,user_pos=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, user_pos);

    if (user_pos < 0)   //新登录用户增加内存记录
    {
        user_info_t* pNewUserInfo = new user_info_t();

        if (pNewUserInfo == NULL)
        {
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_FIND_USER_INFO_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Create User Error Error");
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() new user_info_t() fail  \r\n");
            SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"添加用户信息失败");
            EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Add user information failed.");
            return -1;
        }

        pNewUserInfo->user_index = user_cfg.id;
        pNewUserInfo->user_level = user_cfg.user_level;
        osip_strncpy(pNewUserInfo->user_id, pMsg->register_id, MAX_ID_LEN);
        osip_strncpy(pNewUserInfo->user_name, (char*)user_cfg.user_name.c_str(), MAX_32CHAR_STRING_LEN);
        osip_strncpy(pNewUserInfo->login_ip, pMsg->login_ip, MAX_IP_LEN);
        pNewUserInfo->login_port = pMsg->login_port;
        pNewUserInfo->reg_info_index = reg_info_index;
        pNewUserInfo->alarm_info_send_flag = 0;
        pNewUserInfo->tvwall_status_send_flag = 0;
        pNewUserInfo->tcp_sock = -1;
        pNewUserInfo->tcp_keep_alive_sock = -1;

        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_info_add:user_id=%s,login_ip=%s,login_port=%d,reg_info_index=%d  \r\n", pNewUserInfo->user_id, pNewUserInfo->login_ip, pNewUserInfo->login_port, pNewUserInfo->reg_info_index);

        user_pos = user_info_add(pNewUserInfo);

        if (user_pos < 0)
        {
            delete  pNewUserInfo;
            memset(strErrorCode, 0, 32);
            snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_CREAT_USER_INFO_ERROR);
            SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
            //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"add User Info Error");
            DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() exit---: add User Info Error \r\n");
            SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"添加用户信息失败");
            EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Add user information failed.");
            return -1;
        }
    }

    /* 获取用户信息 */
    pUserInfo = user_info_get(user_pos);

    if (NULL == pUserInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_GET_USER_INFO_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get User Info Error");
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() exit---: Get User Info Error \r\n");
        SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"获取用户信息失败");
        EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Failed to get user information.");
        return -1;
    }

    if (pUserInfo->user_level != user_cfg.user_level)
    {
        pUserInfo->user_level = user_cfg.user_level;
    }

    if (pUserInfo->reg_info_index != reg_info_index)
    {
        pUserInfo->reg_info_index = reg_info_index;
    }

    /* 获取超时时长 */
    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() UserInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pUserInfo->user_id, iExpires, pUserInfo->reg_status, pUserInfo->reg_info_index);
    printf("user_reg_msg_proc() UserInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pUserInfo->user_id, iExpires, pUserInfo->reg_status, pUserInfo->reg_info_index);

    if (iExpires > 0)
    {
        i = UserReg(pUserInfo, user_cfg, pdboper);

        if (i != 0)
        {
            strUserID = pUserInfo->user_id;

            /* 移除用户锁定的点位信息 */
            i = RemoveGBLogicDeviceLockInfoByUserInfo(pUserInfo);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() RemoveGBLogicDeviceLockInfoByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() RemoveGBLogicDeviceLockInfoByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
            }

            /* 查找用户的业务，停止所有业务 */
            i = StopAllServiceTaskByUserInfo(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() StopAllServiceTaskByUserInfo Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() StopAllServiceTaskByUserInfo OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
            }

            /* 停止用户业务处理线程 */
            i = user_srv_proc_thread_recycle(pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() user_srv_proc_thread_recycle Error:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_srv_proc_thread_recycle OK:user_id=%s, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port, i);
            }

            /* 从数据库中删除在线用户信息 */
            i = DeleteUserRegInfoFromDB(pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, pdboper);

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() DeleteUserRegInfoFromDB Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() DeleteUserRegInfoFromDB OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pUserInfo->user_index, pMsg->login_ip, pMsg->login_port, i);
            }

            i = NotifyOnlineUserToAllClientUser(pUserInfo, 2, pdboper);

            if (i < 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "发送通知在线用户变化消息给在线用户失败, 移除的在线用户:用户ID=%s, IP地址=%s, 端口号=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_ERROR, "Users send notification that online users change	to	online user failed, remove online users: user ID = % s, IP address = % s, port = % d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() NotifyOnlineUserToAllClientUser Error:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
            }
            else if (i > 0)
            {
                SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "发送通知在线用户变化消息给在线用户成功, 移除的在线用户:用户ID=%s, IP地址=%s, 端口号=%d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "Users send notification that online users change	to	online user successfully, remove online users: user ID = % s, IP address = % s, port = % d\r\n", pUserInfo->user_id, pUserInfo->login_ip, pUserInfo->login_port);
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() NotifyOnlineUserToAllClientUser OK:UserID=%s, UserIndex=%u, login_ip=%s, login_port=%d, i=%d \r\n", pUserInfo->user_id, pUserInfo->user_index, pUserInfo->login_ip, pUserInfo->login_port, i);
            }

            i = user_info_remove(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

            if (0 != i)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() user_info_remove Error:UserID=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() user_info_remove OK:UserID=%s, login_ip=%s, login_port=%d, i=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, i);
            }

            i = UpdateUserRegInfo2DB2(strUserID, pdboper);   //更新数据库状态

            if (i < 0)
            {
                DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() UpdateUserRegInfo2DB2 Error:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
            }
            else
            {
                DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_reg_msg_proc() UpdateUserRegInfo2DB2 OK:UserID=%s, i=%d \r\n", strUserID.c_str(), i);
            }

            /* 释放掉没用的TCP链接 */
            free_unused_user_tcp_connect();
        }

        return i;
    }

    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SYSTEM_ERROR);
    SIP_UASAnswerToRegister(reg_info_index, 500, strErrorCode);
    //SIP_UASAnswerToRegister(reg_info_index, 500, (char*)"Server Error");
    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_reg_msg_proc() exit---: Server Error \r\n");
    printf("user_reg_msg_proc() exit---: Server Error \r\n");
    return -1;
}

/*****************************************************************************
 函 数 名  : user_unreg_msg_proc
 功能描述  : 用户注销消息处理
 输入参数  : user_reg_msg_t* pMsg
             DBOper* pdboper
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年3月7日
    作    者   : 杨海锋
    修改内容   : 新生成函数

*****************************************************************************/
int user_unreg_msg_proc(user_reg_msg_t* pMsg, DBOper* pdboper)
{
    int i = 0;
    int user_pos = -1;
    int reg_info_index = 0;
    user_info_t* pUserInfo = NULL;
    int iExpires = pMsg->expires;
    char strErrorCode[32] = {0};

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_unreg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    printf("\r\nuser_unreg_msg_proc() Enter---: register_id=%s,login_ip=%s,login_port=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    SystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "用户注销登录处理:用户ID=%s, IP地址=%s, 端口号=%d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);
    EnSystemLog(EV9000_CMS_SYSTEM_RUN_INFO, EV9000_LOG_LEVEL_NORMAL, "User login: user ID = % s, = % s IP address, port number = %d", pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    /*验证参数*/
    if (!pMsg || pMsg->reg_info_index < 0)
    {
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_unreg_msg_proc() exit---: msg /reg_info_index error \r\n");
        return -1;
    }

    reg_info_index = pMsg->reg_info_index;

    if ('\0' == pMsg->register_id[0] || '\0' == pMsg->login_ip[0] || pMsg->login_port <= 0)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_REG_MSG_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"msg /User ID/login_ip login_port error");
        DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_unreg_msg_proc() exit---: msg /User ID/login_ip login_port error \r\n");
        SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户注销登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"用户ID/用户IP地/用户端口号不合法");
        EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"User ID / user IP / user port number is not legitimate.");
        return -1;
    }

    user_pos = user_info_find(pMsg->register_id, pMsg->login_ip, pMsg->login_port);

    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_unreg_msg_proc() user_pos=%d \r\n", user_pos);
    printf("user_unreg_msg_proc() user_info_find:register_id=%s,login_ip=%s,login_port=%d,user_pos=%d \r\n", pMsg->register_id, pMsg->login_ip, pMsg->login_port, user_pos);

    if (user_pos < 0)   //新登录用户增加内存记录
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_FIND_USER_INFO_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"add User Info Error");
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_unreg_msg_proc() exit---: Find User Info Error \r\n");
        SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户注销登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"查找用户信息失败");
        EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Add user information failed.");
        return -1;
    }

    /* 获取用户信息 */
    pUserInfo = user_info_get(user_pos);

    if (NULL == pUserInfo)
    {
        memset(strErrorCode, 0, 32);
        snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_USER_GET_USER_INFO_ERROR);
        SIP_UASAnswerToRegister(reg_info_index, 403, strErrorCode);
        //SIP_UASAnswerToRegister(reg_info_index, 503, (char*)"Get User Info Error");
        DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_unreg_msg_proc() exit---: Get User Info Error \r\n");
        SystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "用户注销登录失败:用户ID=%s, IP地址=%s, 端口号=%d, 原因=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"获取用户信息失败");
        EnSystemLog(EV9000_CMS_USER_REG_FAILED, EV9000_LOG_LEVEL_ERROR, "User login failed:User ID=%s, User IP=%s, Port=%d, reason=%s", pMsg->register_id, pMsg->login_ip, pMsg->login_port, (char*)"Failed to get user information.");
        return -1;
    }

    pUserInfo->reg_status = 3;

    /* 获取超时时长 */
    DEBUG_TRACE(MODULE_USER, LOG_TRACE, "user_unreg_msg_proc() UserInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pUserInfo->user_id, iExpires, pUserInfo->reg_status, pUserInfo->reg_info_index);
    printf("user_unreg_msg_proc() UserInfo:register_id=%s, iExpires=%d, reg_status=%d, reg_info_index=%d \r\n", pUserInfo->user_id, iExpires, pUserInfo->reg_status, pUserInfo->reg_info_index);

    if (iExpires < 0)             //异常
    {
        return UserUnRegAbnormal(pUserInfo, pdboper);
    }
    else if (0 == iExpires)   //注销
    {
        return UserUnReg(pUserInfo, pdboper);
    }

    memset(strErrorCode, 0, 32);
    snprintf(strErrorCode, 32, "%d", EV9000_CMS_ERR_SYSTEM_ERROR);
    SIP_UASAnswerToRegister(reg_info_index, 500, strErrorCode);
    //SIP_UASAnswerToRegister(reg_info_index, 500, (char*)"Server Error");
    DEBUG_TRACE(MODULE_USER, LOG_ERROR, "user_unreg_msg_proc() exit---: Server Error \r\n");
    printf("user_unreg_msg_proc() exit---: Server Error \r\n");
    return -1;
}

